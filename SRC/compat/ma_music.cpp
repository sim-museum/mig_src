// ma_music.cpp — MiG Alley Linux port: Miles AIL XMIDI SEQUENCE (music) path on
// FluidSynth + the game's shipped SoundFont (MUSIC/fieldsnr.sf2).
//
// The game's music is XMIDI (.xmi, loaded into tune[].xmiPtr) played via the Miles
// AIL sequence API (AIL_midiOutOpen / allocate_sequence_handle / init_sequence(xmi) /
// start/stop/volume/status). FluidSynth plays Standard MIDI (SMF), not XMI, so we
// convert XMI->SMF in memory (parse_xmi) and hand it to a fluid_player; FluidSynth's
// own audio driver renders it (separate from the OpenAL SFX path in ma_openal.cpp).
// AILCALL == cdecl, so these are drop-in C-linkage defs matching the call sites.
//
// Degrades to silence if FluidSynth can't init or the SoundFont isn't found (mdi stays
// NULL exactly like the old stub -> the game zeroes music volume and runs without music).
#if defined(MA_LINUX) || defined(FF_LINUX)
#include <fluidsynth.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

typedef int32_t  S32;
typedef uint32_t U32;
struct _MDI_DRIVER; typedef struct _MDI_DRIVER* HMDIDRIVER;
struct _SEQUENCE;   typedef struct _SEQUENCE*   HSEQUENCE;

/* SEQ_ status flags (MSSW.H) */
#define SEQ_FREE     0x0001
#define SEQ_DONE     0x0002
#define SEQ_PLAYING  0x0004
#define SEQ_STOPPED  0x0008

static fluid_settings_t* g_set = 0;
static fluid_synth_t*    g_syn = 0;
static fluid_audio_driver_t* g_drv = 0;
static int g_sfont = -1;
static int g_ready = 0;
static float g_master = 0.4f;   /* base FluidSynth gain (scaled by sequence volume) */

static int mtrace() { static int t=-1; if(t<0) t = getenv("MA_TRACE_AUDIO")?1:0; return t; }

struct MaSeq {
	fluid_player_t* player;
	int vol;            /* 0..127 */
	int loopcount;      /* Miles: 0 = infinite */
};

/* ---- XMIDI -> SMF conversion ------------------------------------------------ */
/* dynamic byte buffer */
struct Buf { unsigned char* d; size_t n, cap; };
static void bput(Buf* b, unsigned char v){ if(b->n>=b->cap){ b->cap=b->cap?b->cap*2:1024; b->d=(unsigned char*)realloc(b->d,b->cap);} b->d[b->n++]=v; }
static void bput_be32(Buf* b, U32 v){ bput(b,v>>24); bput(b,v>>16); bput(b,v>>8); bput(b,v); }
static void bput_be16(Buf* b, U32 v){ bput(b,v>>8); bput(b,v); }
static void bput_vlq(Buf* b, U32 v){ unsigned char s[5]; int i=0; s[i++]=v&0x7F; v>>=7; while(v){ s[i++]=0x80|(v&0x7F); v>>=7; } while(i) bput(b,s[--i]); }

/* a single timed event (status + up to data bytes, or a meta/sysex blob) */
struct Ev { U32 t; int order; unsigned char* bytes; U32 len; };

static U32 rd_be32(const unsigned char* p){ return (p[0]<<24)|(p[1]<<16)|(p[2]<<8)|p[3]; }

/* Find the first EVNT chunk, recursing into IFF containers (FORM/CAT/LIST). XMI nests
   the event stream as FORM XDIR / CAT XMID / FORM XMID / EVNT, so a flat scan misses it. */
static const unsigned char* find_evnt(const unsigned char* p, const unsigned char* end, U32* len) {
	while (p + 8 <= end) {
		U32 sz = rd_be32(p+4);
		const unsigned char* body = p + 8;
		if (body + sz > end) sz = (U32)(end - body);          /* clamp to bounds */
		if (!memcmp(p, "EVNT", 4)) { *len = sz; return body; }
		if (!memcmp(p, "FORM", 4) || !memcmp(p, "CAT ", 4) || !memcmp(p, "LIST", 4)) {
			/* container body = 4-byte form-type then sub-chunks */
			const unsigned char* r = find_evnt(body + 4, body + sz, len);
			if (r) return r;
		}
		p = body + sz + (sz & 1);
	}
	return 0;
}

/* Convert one XMIDI EVNT stream to a format-0 SMF. Returns malloc'd buffer + *outlen,
   or NULL on failure. */
static unsigned char* xmi_to_smf(const unsigned char* xmi, U32 xmilen, U32* outlen) {
	const unsigned char* end = xmi + xmilen;
	/* The EVNT chunk holds the event stream (inside FORM XMID). Scan for it directly —
	   robust across the FORM XDIR / CAT XMID wrapping. */
	U32 elen = 0;
	const unsigned char* ev = find_evnt(xmi, end, &elen);
	if (!ev) return 0;
	const unsigned char* p = ev;
	const unsigned char* eend = ev + elen;

	int ppqn = getenv("MA_MUSIC_PPQN") ? atoi(getenv("MA_MUSIC_PPQN")) : 60;
	if (ppqn <= 0) ppqn = 60;

	/* collect events at absolute ticks; note-on schedules a deferred note-off */
	Ev* evs = 0; size_t nev = 0, cap = 0;
	int order = 0;
	U32 abst = 0;
	#define ADD(_t, ...) do { unsigned char _b[] = {__VA_ARGS__}; \
		if(nev>=cap){cap=cap?cap*2:256; evs=(Ev*)realloc(evs,cap*sizeof(Ev));} \
		evs[nev].t=(_t); evs[nev].order=order++; evs[nev].len=sizeof(_b); \
		evs[nev].bytes=(unsigned char*)malloc(sizeof(_b)); memcpy(evs[nev].bytes,_b,sizeof(_b)); nev++; } while(0)
	#define ADDBLOB(_t, _ptr, _len) do { \
		if(nev>=cap){cap=cap?cap*2:256; evs=(Ev*)realloc(evs,cap*sizeof(Ev));} \
		evs[nev].t=(_t); evs[nev].order=order++; evs[nev].len=(_len); \
		evs[nev].bytes=(unsigned char*)malloc(_len); memcpy(evs[nev].bytes,(_ptr),(_len)); nev++; } while(0)

	while (p < eend) {
		/* XMIDI delay: consecutive bytes < 0x80 accumulate */
		while (p < eend && *p < 0x80) { abst += *p; p++; }
		if (p >= eend) break;
		unsigned char st = *p++;
		int hi = st & 0xF0;
		if (st == 0xFF) {                          /* meta */
			if (p >= eend) break;
			unsigned char mt = *p++;
			/* read SMF VLQ length */
			U32 ln = 0; while (p < eend) { unsigned char c=*p++; ln=(ln<<7)|(c&0x7F); if(!(c&0x80)) break; }
			unsigned char hdr[2] = { 0xFF, mt };
			/* blob = FF mt vlq(len) data */
			Buf mb = {0,0,0};
			bput(&mb,0xFF); bput(&mb,mt); bput_vlq(&mb, ln);
			for (U32 i=0;i<ln && p<eend;i++) bput(&mb,*p++);
			ADDBLOB(abst, mb.d, mb.n); free(mb.d);
			if (mt == 0x2F) break;                 /* end of track */
		} else if (st == 0xF0 || st == 0xF7) {     /* sysex */
			U32 ln = 0; while (p < eend) { unsigned char c=*p++; ln=(ln<<7)|(c&0x7F); if(!(c&0x80)) break; }
			Buf sb = {0,0,0}; bput(&sb, st); bput_vlq(&sb, ln);
			for (U32 i=0;i<ln && p<eend;i++) bput(&sb,*p++);
			ADDBLOB(abst, sb.d, sb.n); free(sb.d);
		} else if (hi == 0x90) {                   /* note on (XMIDI: + duration) */
			if (p+1 >= eend) break;
			unsigned char note=*p++, vel=*p++;
			ADD(abst, st, note, vel);
			/* duration as standard VLQ */
			U32 dur=0; while (p < eend) { unsigned char c=*p++; dur=(dur<<7)|(c&0x7F); if(!(c&0x80)) break; }
			ADD(abst+dur, (unsigned char)(0x80|(st&0x0F)), note, 0x40);
		} else if (hi == 0x80 || hi == 0xA0 || hi == 0xB0 || hi == 0xE0) {
			if (p+1 >= eend) break;
			unsigned char d1=*p++, d2=*p++;
			ADD(abst, st, d1, d2);
		} else if (hi == 0xC0 || hi == 0xD0) {
			if (p >= eend) break;
			unsigned char d1=*p++;
			ADD(abst, st, d1);
		} else {
			break;                                 /* unknown -> stop */
		}
	}
	if (!nev) { free(evs); return 0; }

	/* stable sort by (tick, order) */
	for (size_t i=1;i<nev;i++){ Ev key=evs[i]; long j=i-1;
		while (j>=0 && (evs[j].t>key.t || (evs[j].t==key.t && evs[j].order>key.order))) { evs[j+1]=evs[j]; j--; }
		evs[j+1]=key; }

	/* write SMF: MThd + one MTrk */
	Buf trk = {0,0,0};
	U32 prev = 0;
	for (size_t i=0;i<nev;i++) {
		bput_vlq(&trk, evs[i].t - prev); prev = evs[i].t;
		for (U32 k=0;k<evs[i].len;k++) bput(&trk, evs[i].bytes[k]);
		free(evs[i].bytes);
	}
	free(evs);
	/* ensure end-of-track */
	bput_vlq(&trk, 0); bput(&trk,0xFF); bput(&trk,0x2F); bput(&trk,0x00);

	Buf out = {0,0,0};
	out.d=(unsigned char*)malloc(0);
	const char* MThd="MThd"; for(int i=0;i<4;i++) bput(&out,MThd[i]);
	bput_be32(&out, 6); bput_be16(&out, 0); bput_be16(&out, 1); bput_be16(&out, ppqn);
	const char* MTrk="MTrk"; for(int i=0;i<4;i++) bput(&out,MTrk[i]);
	bput_be32(&out, trk.n);
	for (size_t i=0;i<trk.n;i++) bput(&out, trk.d[i]);
	free(trk.d);
	*outlen = out.n;
	return out.d;
}

/* ---- FluidSynth init -------------------------------------------------------- */
static int music_init() {
	if (g_ready) return 1;
	if (getenv("MA_NO_AUDIO") || getenv("MA_NO_MUSIC")) return 0;
	g_set = new_fluid_settings();
	/* pick an audio backend that exists on this box (pipewire-pulse is up). */
	const char* drv = getenv("MA_FLUID_DRIVER");
	if (drv) fluid_settings_setstr(g_set, "audio.driver", drv);
	else     fluid_settings_setstr(g_set, "audio.driver", "pulseaudio");
	fluid_settings_setnum(g_set, "synth.gain", g_master);
	g_syn = new_fluid_synth(g_set);
	if (!g_syn) { fprintf(stderr,"[mus] new_fluid_synth failed\n"); return 0; }
	/* Instrument bank: the game's own MUSIC/fieldsnr.sf2 is a single custom preset
	   (bank0/prog1 "fieldsnares") -- its real bank was WAVETAB.WVL (proprietary Miles DLS),
	   not a General MIDI set. The XMIDI tunes are GM, so render them through a standard GM
	   SoundFont (the DOSBox/ScummVM approach). Prefer the user override, then the system
	   default-GM, then the common packaged banks; the game's tiny SF2 is the last resort. */
	const char* mso = getenv("MA_SOUNDFONT");
	if (mso && *mso) g_sfont = fluid_synth_sfload(g_syn, mso, 1);
	const char* cands[] = {
		"/usr/share/sounds/sf2/default-GM.sf2",
		"/usr/share/sounds/sf2/FluidR3_GM.sf2",
		"/usr/share/sounds/sf2/TimGM6mb.sf2",
		"MUSIC/fieldsnr.sf2",
		0 };
	for (int i=0; cands[i] && g_sfont<0; i++)
		g_sfont = fluid_synth_sfload(g_syn, cands[i], 1);
	if (g_sfont < 0) { fprintf(stderr,"[mus] no GM SoundFont found -> no music (set MA_SOUNDFONT)\n"); return 0; }
	g_drv = new_fluid_audio_driver(g_set, g_syn);
	if (!g_drv) { fprintf(stderr,"[mus] no audio driver -> no music\n"); return 0; }
	g_ready = 1;
	fprintf(stderr,"[mus] FluidSynth music ready (sf2 id=%d)\n", g_sfont);
	return 1;
}

/* fwd decls for the self-test (real defs in the extern "C" block below) */
extern "C" { HSEQUENCE AIL_allocate_sequence_handle(HMDIDRIVER);
	S32 AIL_init_sequence(HSEQUENCE,void*,S32); void AIL_start_sequence(HSEQUENCE);
	void AIL_set_sequence_volume(HSEQUENCE,S32,S32); U32 AIL_sequence_status(HSEQUENCE); }

/* MA_MUSIC_SELFTEST=<path-to-xmi>: load a real game .xmi through the full
   XMI->SMF->FluidSynth path and confirm it plays (status PLAYING). */
static void music_selftest(const char* path) {
	FILE* f=fopen(path,"rb"); if(!f){fprintf(stderr,"[mus] selftest: cannot open %s\n",path);return;}
	fseek(f,0,SEEK_END); long n=ftell(f); fseek(f,0,SEEK_SET);
	unsigned char* buf=(unsigned char*)malloc(n);
	if(fread(buf,1,n,f)!=(size_t)n){fclose(f);free(buf);return;} fclose(f);
	HSEQUENCE s=AIL_allocate_sequence_handle((HMDIDRIVER)1);
	if(!s){fprintf(stderr,"[mus] selftest: no handle\n");free(buf);return;}
	if(!AIL_init_sequence(s,buf,0)){fprintf(stderr,"[mus] selftest: init_sequence failed\n");free(buf);return;}
	AIL_set_sequence_volume(s,127,0); AIL_start_sequence(s);
	fprintf(stderr,"[mus] selftest: playing %s\n",path);
	for(int i=0;i<25;i++){ U32 st=AIL_sequence_status(s);
		fprintf(stderr,"[mus] selftest t=%dms status=%s\n",i*200,
			st==SEQ_PLAYING?"PLAYING":st==SEQ_STOPPED?"STOPPED":"DONE");
		if(st!=SEQ_PLAYING && i>2) break;
		struct timespec ts={0,200*1000*1000}; nanosleep(&ts,0); }
	free(buf);
}

extern "C" {

/* ---- MIDI driver init (StartUpMiles) ---------------------------------------- */
S32 AIL_midiOutOpen(HMDIDRIVER* mdi, void* /*midiOutDev*/, U32 /*driverid*/) {
	if (!music_init()) { if (mdi) *mdi = 0; return -1; }   /* nonzero = fail (Miles) */
	if (mdi) *mdi = (HMDIDRIVER)1;
	const char* stp = getenv("MA_MUSIC_SELFTEST");
	static int st_done = 0;
	if (stp && !st_done) { st_done = 1; music_selftest(stp); }
	return 0;                                              /* 0 = success */
}
void AIL_midiOutClose(HMDIDRIVER) {}

/* ---- sequence handles ------------------------------------------------------- */
HSEQUENCE AIL_allocate_sequence_handle(HMDIDRIVER mdi) {
	if (!mdi || !g_ready) return 0;
	MaSeq* s = (MaSeq*)calloc(1, sizeof(MaSeq));
	s->vol = 100; s->loopcount = 1;
	return (HSEQUENCE)s;
}
void AIL_release_sequence_handle(HSEQUENCE h) {
	MaSeq* s=(MaSeq*)h; if(!s) return;
	if (s->player) { fluid_player_stop(s->player); delete_fluid_player(s->player); }
	free(s);
}
S32 AIL_init_sequence(HSEQUENCE h, void* start, S32 /*num*/) {
	MaSeq* s=(MaSeq*)h; if(!s || !start || !g_ready) return 0;
	/* `start` is the raw XMI image (tune[].xmiPtr); we aren't given its length, but IFF
	   chunks self-size. An XMI file is FORM(XDIR) followed by CAT(XMID) [which holds the
	   FORM XMID / EVNT tracks]. Walk both top-level chunks to get the full extent — using
	   only the first FORM's size (~22 B) would miss the entire CAT where EVNT lives. */
	const unsigned char* x = (const unsigned char*)start;
	if (memcmp(x, "FORM", 4)) return 0;
	U32 xlen = 8 + rd_be32(x+4);                       /* FORM XDIR extent */
	const unsigned char* nx = x + xlen;
	if (!memcmp(nx, "CAT ", 4) || !memcmp(nx, "FORM", 4))
		xlen += 8 + rd_be32(nx+4);                     /* + CAT XMID (the tracks) */
	U32 smflen = 0;
	unsigned char* smf = xmi_to_smf(x, xlen, &smflen);
	if (!smf) { if (mtrace()) fprintf(stderr,"[mus] XMI->SMF failed\n"); return 0; }
	if (s->player) { fluid_player_stop(s->player); delete_fluid_player(s->player); }
	s->player = new_fluid_player(g_syn);
	int rc = fluid_player_add_mem(s->player, smf, smflen);
	free(smf);
	if (rc != FLUID_OK) { if (mtrace()) fprintf(stderr,"[mus] player_add_mem failed\n"); return 0; }
	if (mtrace()) fprintf(stderr,"[mus] init_sequence: XMI %u B -> SMF %u B\n", xlen, smflen);
	return 1;                                              /* nonzero = success */
}
void AIL_start_sequence(HSEQUENCE h) {
	MaSeq* s=(MaSeq*)h; if(!s || !s->player) return;
	fluid_player_set_loop(s->player, s->loopcount == 0 ? -1 : 1);
	fluid_player_play(s->player);
	if (mtrace()) fprintf(stderr,"[mus] start_sequence (vol=%d loop=%d)\n", s->vol, s->loopcount);
}
void AIL_stop_sequence(HSEQUENCE h)   { MaSeq* s=(MaSeq*)h; if(s&&s->player) fluid_player_stop(s->player); }
void AIL_resume_sequence(HSEQUENCE h) { MaSeq* s=(MaSeq*)h; if(s&&s->player) fluid_player_play(s->player); }
void AIL_end_sequence(HSEQUENCE h)    { MaSeq* s=(MaSeq*)h; if(s&&s->player) fluid_player_stop(s->player); }

void AIL_set_sequence_volume(HSEQUENCE h, S32 volume, S32 /*ms*/) {
	MaSeq* s=(MaSeq*)h; if(!s) return;
	if (volume<0) volume=0; if (volume>127) volume=127;
	s->vol = volume;
	/* single music stream -> scale the global synth gain. */
	if (g_ready) fluid_synth_set_gain(g_syn, g_master * (volume/127.0f) * 2.0f);
}
void AIL_set_sequence_loop_count(HSEQUENCE h, S32 count) {
	MaSeq* s=(MaSeq*)h; if(!s) return;
	s->loopcount = count;
	if (s->player) fluid_player_set_loop(s->player, count==0 ? -1 : count);
}
U32 AIL_sequence_status(HSEQUENCE h) {
	MaSeq* s=(MaSeq*)h; if(!s || !s->player) return SEQ_DONE;
	int st = fluid_player_get_status(s->player);
	if (st == FLUID_PLAYER_PLAYING) return SEQ_PLAYING;
	if (st == FLUID_PLAYER_DONE)    return SEQ_DONE;
	return SEQ_STOPPED;
}
S32 AIL_sequence_volume(HSEQUENCE h) { MaSeq* s=(MaSeq*)h; return s?s->vol:0; }
void AIL_set_XMIDI_master_volume(HMDIDRIVER, S32 master_volume) {
	if (!g_ready) return;
	int v=master_volume; if(v<0)v=0; if(v>127)v=127;
	fluid_synth_set_gain(g_syn, g_master * (v/127.0f) * 2.0f);
}

} /* extern "C" */
#endif
