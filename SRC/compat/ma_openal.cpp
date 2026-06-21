// ma_openal.cpp — MiG Alley Linux port: Miles Sound System (AIL) DIGITAL-SAMPLE
// subset implemented on OpenAL. The game (SRC/HARDWARE/MILES.CPP) drives audio via
// the Miles AIL_* C API; this file replaces the silent no-op stubs for the digital
// (SFX / UI / engine / sample) path with real OpenAL output. AILCALL == cdecl
// (caller cleans the stack), so these definitions just need matching arg layout and
// the same C-linkage names the call sites reference.
//
// Implemented here: digital driver init (AIL_startup / AIL_waveOutOpen), sample
// handles (allocate/init/release), sample data (AIL_set_sample_file = RIFF/WAV image,
// AIL_set_sample_address = raw PCM for radio), playback (start/stop/resume/end),
// volume/pan/rate/loop, and status. MIDI/XMIDI music + DLS sequences stay no-op in
// miles_ail_stub.cpp (a later increment). Degrades gracefully to silence if OpenAL
// can't open a device (dig stays NULL, exactly like the old stubs).
#if defined(MA_LINUX) || defined(FF_LINUX)
#include <AL/al.h>
#include <AL/alc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <time.h>

typedef int32_t  S32;
typedef uint32_t U32;
struct _DIG_DRIVER; typedef struct _DIG_DRIVER* HDIGDRIVER;
struct _SAMPLE;     typedef struct _SAMPLE*     HSAMPLE;

/* SMP_ status flags (MSSW.H) */
#define SMP_FREE     0x0001
#define SMP_DONE     0x0002
#define SMP_PLAYING  0x0004
#define SMP_STOPPED  0x0008

struct MaSample {
	ALuint src;
	ALuint buf;
	int    hasbuf;
	int    native_rate;     /* sample rate of the loaded buffer */
	int    want_rate;       /* AIL_set_sample_playback_rate override (0 = native) */
	float  gain;            /* 0..1 from AIL_set_sample_volume (0..127) */
	int    pan;             /* 0..127, 64 = centre */
	int    loopcount;       /* Miles: 0 = infinite, 1 = once, n = n times */
	int    raw_chans, raw_bits; /* for AIL_set_sample_address (raw PCM, radio) */
};

static ALCdevice*  g_dev = 0;
static ALCcontext* g_ctx = 0;
static int         g_ready = 0;
static int         g_traced_play = 0;
static int         g_play_count = 0;

static int al_trace() { static int t=-1; if(t<0) t = getenv("MA_TRACE_AUDIO")?1:0; return t; }

static int al_init() {
	if (g_ready) return 1;
	if (getenv("MA_NO_AUDIO")) { static int once=0; if(!once++) fprintf(stderr,"[al] MA_NO_AUDIO -> silent\n"); return 0; }
	g_dev = alcOpenDevice(NULL);
	if (!g_dev) { fprintf(stderr, "[al] alcOpenDevice failed -> silent\n"); return 0; }
	g_ctx = alcCreateContext(g_dev, NULL);
	if (!g_ctx || !alcMakeContextCurrent(g_ctx)) {
		fprintf(stderr, "[al] alcCreateContext/MakeCurrent failed -> silent\n");
		if (g_ctx) alcDestroyContext(g_ctx);
		alcCloseDevice(g_dev); g_dev = 0; g_ctx = 0; return 0;
	}
	alGetError();
	/* 2D-mixer default: listener at origin, sources placed relative for panning. */
	alListener3f(AL_POSITION, 0, 0, 0);
	const ALfloat ori[6] = { 0,0,-1, 0,1,0 };
	alListenerfv(AL_ORIENTATION, ori);
	g_ready = 1;
	fprintf(stderr, "[al] OpenAL ready: %s / %s\n",
		alGetString(AL_RENDERER), alcGetString(g_dev, ALC_DEVICE_SPECIFIER));
	return 1;
}

/* little-endian readers for WAV chunk walking */
static inline U32 rd32(const unsigned char* p){ return p[0]|(p[1]<<8)|(p[2]<<16)|((U32)p[3]<<24); }
static inline U32 rd16(const unsigned char* p){ return p[0]|(p[1]<<8); }

/* Parse a RIFF/WAVE image: find fmt + data chunks. Returns 1 on success. */
static int parse_wav(const unsigned char* d,
                     const unsigned char** pcm, U32* pcmlen,
                     int* chans, int* rate, int* bits) {
	if (memcmp(d, "RIFF", 4) || memcmp(d+8, "WAVE", 4)) return 0;
	U32 riffEnd = 12 + rd32(d+4) - 4;     /* generous bound */
	const unsigned char* p = d + 12;
	int haveFmt = 0; const unsigned char* data = 0; U32 dlen = 0;
	/* walk chunks: 4-byte id, 4-byte size, body (word-aligned) */
	for (int guard = 0; guard < 64; guard++) {
		U32 id = rd32(p); U32 sz = rd32(p+4);
		const unsigned char* body = p + 8;
		if (id == 0x20746d66 /* "fmt " */) {
			int fmtTag = rd16(body);
			*chans = rd16(body+2);
			*rate  = rd32(body+4);
			*bits  = rd16(body+14);
			if (fmtTag != 1 /*PCM*/ && fmtTag != 0xFFFE /*EXTENSIBLE*/) return 0;
			haveFmt = 1;
		} else if (id == 0x61746164 /* "data" */) {
			data = body; dlen = sz;
		}
		p = body + sz + (sz & 1);         /* chunks are word-aligned */
		if (p >= d + riffEnd) break;
		if (haveFmt && data) break;
	}
	if (!haveFmt || !data || !dlen) return 0;
	*pcm = data; *pcmlen = dlen;
	return 1;
}

static ALenum al_format(int chans, int bits) {
	if (bits == 16) return chans >= 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
	return chans >= 2 ? AL_FORMAT_STEREO8 : AL_FORMAT_MONO8;
}

static void apply_pan(MaSample* s) {
	/* 2D constant-power pan via relative source position. pan 0..127, 64 = centre. */
	float x = (s->pan - 64) / 64.0f;          /* -1 left .. +1 right */
	if (x >  1) x =  1; if (x < -1) x = -1;
	float z = -sqrtf(1.0f - x*x);             /* keep on the unit circle, in front */
	alSourcei(s->src, AL_SOURCE_RELATIVE, AL_TRUE);
	alSource3f(s->src, AL_POSITION, x, 0.0f, z);
}
static void apply_rate(MaSample* s) {
	float pitch = 1.0f;
	if (s->want_rate > 0 && s->native_rate > 0)
		pitch = (float)s->want_rate / (float)s->native_rate;
	alSourcef(s->src, AL_PITCH, pitch);
}

/* forward decls (the AIL functions are defined in the extern "C" block below) */
extern "C" {
	S32 AIL_waveOutOpen(HDIGDRIVER*, void*, S32, void*);
	HSAMPLE AIL_allocate_sample_handle(HDIGDRIVER);
	void AIL_init_sample(HSAMPLE);
	S32 AIL_set_sample_file(HSAMPLE, const void*, S32);
	void AIL_set_sample_volume(HSAMPLE, S32);
	void AIL_start_sample(HSAMPLE);
	U32 AIL_sample_status(HSAMPLE);
	S32 AIL_sample_position(HSAMPLE);
}

/* Optional self-test (MA_AUDIO_SELFTEST=<path-to-wav>): load a real WAV via the same
   AIL path the game uses and confirm OpenAL renders it (source state PLAYING -> DONE,
   byte offset advancing). Proves the backend end-to-end independent of engine triggers. */
static void audio_selftest(const char* path) {
	FILE* f = fopen(path, "rb"); if (!f) { fprintf(stderr, "[al] selftest: cannot open %s\n", path); return; }
	fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
	unsigned char* buf = (unsigned char*)malloc(n);
	if (fread(buf, 1, n, f) != (size_t)n) { fclose(f); free(buf); return; } fclose(f);
	HDIGDRIVER dig = 0; AIL_waveOutOpen(&dig, 0, 0, 0);
	HSAMPLE s = AIL_allocate_sample_handle(dig);
	if (!s) { fprintf(stderr, "[al] selftest: no handle\n"); free(buf); return; }
	AIL_init_sample(s);
	if (!AIL_set_sample_file(s, buf, -1)) { fprintf(stderr, "[al] selftest: WAV parse failed\n"); free(buf); return; }
	AIL_set_sample_volume(s, 127);
	AIL_start_sample(s);
	fprintf(stderr, "[al] selftest: playing %s\n", path);
	for (int i = 0; i < 30; i++) {
		U32 st = AIL_sample_status(s); S32 pos = AIL_sample_position(s);
		fprintf(stderr, "[al] selftest t=%dms status=%s bytepos=%d\n", i*100,
			st==SMP_PLAYING?"PLAYING":st==SMP_STOPPED?"PAUSED":"DONE", pos);
		if (st == SMP_DONE && i > 2) break;
		struct timespec ts = {0, 100*1000*1000}; nanosleep(&ts, 0);
	}
	free(buf);
}

extern "C" {

/* ---- digital driver init -------------------------------------------------- */
S32  AIL_startup(void) {
	const char* stp = getenv("MA_AUDIO_SELFTEST");
	static int st_done = 0;
	if (stp && !st_done) { st_done = 1; audio_selftest(stp); }
	return 1;                                   /* nonzero "timer/thread" handle */
}
void AIL_shutdown(void)         { /* keep the context for the process lifetime */ }
S32  AIL_last_error(void)       { return 0; }

S32 AIL_waveOutOpen(HDIGDRIVER* drvr, void* /*mdi*/, S32 /*wave_id*/, void* /*fmt*/) {
	if (!al_init()) { if (drvr) *drvr = 0; return -1; }
	if (drvr) *drvr = (HDIGDRIVER)1;            /* non-null sentinel = "open" */
	return 0;                                   /* Miles: 0 = success */
}
void AIL_waveOutClose(HDIGDRIVER) {}

/* ---- sample handles ------------------------------------------------------- */
HSAMPLE AIL_allocate_sample_handle(HDIGDRIVER dig) {
	if (!dig || !g_ready) return 0;
	MaSample* s = (MaSample*)calloc(1, sizeof(MaSample));
	if (!s) return 0;
	alGetError();
	alGenSources(1, &s->src);
	if (alGetError() != AL_NO_ERROR) { free(s); return 0; }
	s->gain = 1.0f; s->pan = 64; s->loopcount = 1; s->native_rate = 22050;
	static int nalloc = 0; nalloc++;
	if (al_trace()) fprintf(stderr, "[al] allocate_sample_handle #%d\n", nalloc);
	return (HSAMPLE)s;
}
void AIL_release_sample_handle(HSAMPLE h) {
	MaSample* s = (MaSample*)h; if (!s) return;
	alSourceStop(s->src);
	alSourcei(s->src, AL_BUFFER, 0);
	alDeleteSources(1, &s->src);
	if (s->hasbuf) alDeleteBuffers(1, &s->buf);
	free(s);
}
void AIL_init_sample(HSAMPLE h) {
	MaSample* s = (MaSample*)h; if (!s) return;
	alSourceStop(s->src);
	alSourcei(s->src, AL_BUFFER, 0);
	if (s->hasbuf) { alDeleteBuffers(1, &s->buf); s->hasbuf = 0; }
	alSourcef(s->src, AL_GAIN, 1.0f);
	alSourcef(s->src, AL_PITCH, 1.0f);
	alSourcei(s->src, AL_LOOPING, AL_FALSE);
	apply_pan(s);
	s->gain = 1.0f; s->pan = 64; s->loopcount = 1; s->want_rate = 0;
}

/* ---- sample data ---------------------------------------------------------- */
S32 AIL_set_sample_file(HSAMPLE h, const void* image, S32 /*block*/) {
	MaSample* s = (MaSample*)h; if (!s || !image) return 0;
	const unsigned char* pcm; U32 pcmlen; int chans, rate, bits;
	if (!parse_wav((const unsigned char*)image, &pcm, &pcmlen, &chans, &rate, &bits))
		return 0;
	if (s->hasbuf) { alSourcei(s->src, AL_BUFFER, 0); alDeleteBuffers(1, &s->buf); s->hasbuf = 0; }
	alGetError();
	alGenBuffers(1, &s->buf);
	alBufferData(s->buf, al_format(chans, bits), pcm, (ALsizei)pcmlen, rate);
	if (alGetError() != AL_NO_ERROR) { s->hasbuf = 0; return 0; }
	s->hasbuf = 1; s->native_rate = rate;
	alSourcei(s->src, AL_BUFFER, s->buf);
	if (al_trace() && g_traced_play < 6) {
		fprintf(stderr, "[al] sample_file: %dch %dHz %dbit %u bytes\n", chans, rate, bits, pcmlen);
		g_traced_play++;
	}
	return 1;                                   /* nonzero = success */
}
/* raw PCM (radio chatter via PlaySampleRadio). Format from prior set_sample_type/rate;
   default mono-16 at want_rate (or native) if unset. */
void AIL_set_sample_address(HSAMPLE h, const void* data, U32 len) {
	MaSample* s = (MaSample*)h; if (!s || !data || !len) return;
	int chans = s->raw_chans ? s->raw_chans : 1;
	int bits  = s->raw_bits  ? s->raw_bits  : 16;
	int rate  = s->want_rate > 0 ? s->want_rate : (s->native_rate > 0 ? s->native_rate : 11025);
	if (s->hasbuf) { alSourcei(s->src, AL_BUFFER, 0); alDeleteBuffers(1, &s->buf); s->hasbuf = 0; }
	alGetError();
	alGenBuffers(1, &s->buf);
	alBufferData(s->buf, al_format(chans, bits), data, (ALsizei)len, rate);
	if (alGetError() != AL_NO_ERROR) { s->hasbuf = 0; return; }
	s->hasbuf = 1; s->native_rate = rate;
	alSourcei(s->src, AL_BUFFER, s->buf);
}
/* sample type flags (DIG_F_MONO_8 etc.) -> remember channel/bit width for raw PCM. */
void AIL_set_sample_type(HSAMPLE h, S32 format, U32 /*flags*/) {
	MaSample* s = (MaSample*)h; if (!s) return;
	/* Miles DIG_F_*: bit0 = stereo, bit1 = 16-bit (engine uses mono-8/16 for radio). */
	s->raw_chans = (format & 0x01) ? 2 : 1;
	s->raw_bits  = (format & 0x02) ? 16 : 8;
}

/* ---- playback controls ---------------------------------------------------- */
void AIL_start_sample(HSAMPLE h) {
	MaSample* s = (MaSample*)h; if (!s || !s->hasbuf) return;
	alSourcef(s->src, AL_GAIN, s->gain);
	apply_pan(s); apply_rate(s);
	alSourcei(s->src, AL_LOOPING, (s->loopcount == 0) ? AL_TRUE : AL_FALSE);
	alSourceRewind(s->src);
	alSourcePlay(s->src);
	g_play_count++;
	if (al_trace() && g_play_count <= 12)
		fprintf(stderr, "[al] start_sample #%d gain=%.2f pan=%d loop=%d\n",
			g_play_count, s->gain, s->pan, s->loopcount);
}
void AIL_stop_sample(HSAMPLE h)   { MaSample* s=(MaSample*)h; if (s) alSourcePause(s->src); }
void AIL_resume_sample(HSAMPLE h) { MaSample* s=(MaSample*)h; if (s) alSourcePlay(s->src); }
void AIL_end_sample(HSAMPLE h)    { MaSample* s=(MaSample*)h; if (s) alSourceStop(s->src); }

void AIL_set_sample_volume(HSAMPLE h, S32 volume) {
	MaSample* s = (MaSample*)h; if (!s) return;
	if (volume < 0) volume = 0; if (volume > 127) volume = 127;
	s->gain = volume / 127.0f;
	alSourcef(s->src, AL_GAIN, s->gain);
}
void AIL_set_sample_pan(HSAMPLE h, S32 pan) {
	MaSample* s = (MaSample*)h; if (!s) return;
	if (pan < 0) pan = 0; if (pan > 127) pan = 127;
	s->pan = pan; apply_pan(s);
}
void AIL_set_sample_playback_rate(HSAMPLE h, S32 rate) {
	MaSample* s = (MaSample*)h; if (!s) return;
	s->want_rate = rate; apply_rate(s);
}
void AIL_set_sample_loop_count(HSAMPLE h, S32 count) {
	MaSample* s = (MaSample*)h; if (!s) return;
	s->loopcount = count;
	alSourcei(s->src, AL_LOOPING, (count == 0) ? AL_TRUE : AL_FALSE);
}
U32 AIL_sample_status(HSAMPLE h) {
	MaSample* s = (MaSample*)h; if (!s) return SMP_DONE;
	ALint st = AL_STOPPED;
	alGetSourcei(s->src, AL_SOURCE_STATE, &st);
	if (st == AL_PLAYING) return SMP_PLAYING;
	if (st == AL_PAUSED)  return SMP_STOPPED;
	return SMP_DONE;
}
S32 AIL_sample_position(HSAMPLE h) {
	MaSample* s = (MaSample*)h; if (!s) return 0;
	ALint off = 0; alGetSourcei(s->src, AL_BYTE_OFFSET, &off); return off;
}
void AIL_set_digital_master_volume(HDIGDRIVER, S32 volume) {
	if (!g_ready) return;
	float g = volume / 127.0f; if (g < 0) g = 0; if (g > 1) g = 1;
	alListenerf(AL_GAIN, g);
}

} /* extern "C" */
#endif
