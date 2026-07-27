/* BoB Linux port - minimal Win32 PE resource loader.
 *
 * The game keeps its UI strings/dialogs/bitmaps in a resource-only DLL
 * (boblang.dll, loaded via LoadLibrary in MIG.cpp). On Windows these are read
 * with LoadString/FindResource/LoadResource against that module handle. There
 * is no LoadLibrary on Linux, so this parses the PE .rsrc section directly and
 * backs those APIs. LoadString (RT_STRING) is the hot path (~199 call sites);
 * a NULL/empty string here used to spin the font setup (MIG.cpp:613 for(;;)).
 *
 * Format refs: PE/COFF spec, IMAGE_RESOURCE_DIRECTORY tree (Type/Name/Lang).
 * All parsing is offset-based (no packed struct overlays) so it is independent
 * of the global -fpack-struct=1.
 */
#ifdef FF_LINUX

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>

/* Use the case-insensitive resolver (handles Windows '\' paths, case folding,
 * and the BOB_DRIVE_C mapping) -- the raw libc fopen can't open the game's
 * drive-absolute "\Program Files\...\boblang.dll" paths. */
extern "C" FILE* fopen_nocase(const char* path, const char* mode);

typedef void* HMODULE_T;

/* A loaded resource module: the whole DLL image in memory + the parsed bits we
 * need to resolve RVAs and walk the resource tree. */
struct BobResModule {
	uint8_t* buf;            /* whole-file image */
	size_t   size;
	uint32_t resBaseOff;     /* file offset of the resource directory root */
	/* section table for RVA->file-offset translation */
	struct Sec { uint32_t vaddr, vsize, rawoff, rawsize; } sec[32];
	int nsec;
};

static BobResModule* g_resModule = NULL;   /* current AfxGetResourceHandle() */

static inline uint16_t rd16(const uint8_t* p){ return (uint16_t)(p[0]|(p[1]<<8)); }
static inline uint32_t rd32(const uint8_t* p){ return (uint32_t)(p[0]|(p[1]<<8)|(p[2]<<16)|((uint32_t)p[3]<<24)); }

static uint32_t rva2off(BobResModule* m, uint32_t rva){
	for (int i=0;i<m->nsec;i++)
		if (rva >= m->sec[i].vaddr && rva < m->sec[i].vaddr + m->sec[i].vsize)
			return rva - m->sec[i].vaddr + m->sec[i].rawoff;
	return 0;
}

/* ---- resource-directory walk -------------------------------------------- */
/* A directory at file offset `dirOff` has 16-byte header then 8-byte entries:
 *   { DWORD NameOrId, DWORD OffsetToData }.   (offsets are relative to resBase)
 * High bit of NameOrId set => named entry (we ignore names; we match IDs).
 * High bit of OffsetToData set => points at a subdirectory, else a data leaf. */
static const uint8_t* res_find_entry(BobResModule* m, uint32_t dirOff, uint32_t id, uint32_t* outOff, int* outIsDir){
	const uint8_t* dir = m->buf + m->resBaseOff + dirOff;
	uint16_t nNamed = rd16(dir+12), nId = rd16(dir+14);
	const uint8_t* e = dir + 16 + (size_t)nNamed*8;   /* skip named, match IDs */
	for (int i=0;i<nId;i++,e+=8){
		uint32_t nameId = rd32(e);
		if ((nameId & 0x7fffffff) == id && !(nameId & 0x80000000)){
			uint32_t off = rd32(e+4);
			*outOff = off & 0x7fffffff; *outIsDir = (off & 0x80000000)?1:0;
			return e;
		}
	}
	return NULL;
}
/* first child of a directory (used to descend the language level) */
static int res_first_child(BobResModule* m, uint32_t dirOff, uint32_t* outOff, int* outIsDir){
	const uint8_t* dir = m->buf + m->resBaseOff + dirOff;
	uint16_t nNamed = rd16(dir+12), nId = rd16(dir+14);
	if (nNamed + nId == 0) return 0;
	const uint8_t* e = dir + 16;          /* first entry (named or id) */
	uint32_t off = rd32(e+4);
	*outOff = off & 0x7fffffff; *outIsDir = (off & 0x80000000)?1:0;
	return 1;
}

/* Resolve Type/Id -> pointer+size of the raw resource data (NULL if absent). */
static const uint8_t* res_get(BobResModule* m, uint32_t type, uint32_t id, uint32_t* outSize){
	if (!m) return NULL;
	uint32_t off; int isDir;
	if (!res_find_entry(m, 0, type, &off, &isDir) || !isDir) return NULL;      /* Type level */
	uint32_t typeDir = off;
	if (!res_find_entry(m, typeDir, id, &off, &isDir) || !isDir) return NULL;  /* Name/Id level */
	uint32_t nameDir = off;
	if (!res_first_child(m, nameDir, &off, &isDir) || isDir) return NULL;      /* Lang level -> leaf */
	const uint8_t* leaf = m->buf + m->resBaseOff + off;  /* IMAGE_RESOURCE_DATA_ENTRY */
	uint32_t dataRva = rd32(leaf), sz = rd32(leaf+4);
	uint32_t doff = rva2off(m, dataRva);
	if (!doff || doff + sz > m->size) return NULL;
	if (outSize) *outSize = sz;
	return m->buf + doff;
}

/* ---- PE parse ----------------------------------------------------------- */
static BobResModule* parse_pe(uint8_t* buf, size_t size){
	if (size < 0x40 || buf[0]!='M' || buf[1]!='Z') return NULL;
	uint32_t e_lfanew = rd32(buf+0x3C);
	if (e_lfanew + 24 > size || memcmp(buf+e_lfanew, "PE\0\0", 4)!=0) return NULL;
	uint32_t fh = e_lfanew + 4;
	uint16_t nsec = rd16(buf+fh+2);
	uint16_t optsz = rd16(buf+fh+16);
	uint32_t opt = fh + 20;
	if (rd16(buf+opt) != 0x10b) return NULL;          /* PE32 only */
	uint32_t resRva = rd32(buf+opt+0x70);             /* DataDirectory[2].VirtualAddress */
	if (!resRva) return NULL;

	BobResModule* m = (BobResModule*)calloc(1, sizeof(BobResModule));
	m->buf = buf; m->size = size;
	uint32_t sec = opt + optsz;
	m->nsec = nsec > 32 ? 32 : nsec;
	for (int i=0;i<m->nsec;i++){
		uint32_t b = sec + i*40;
		m->sec[i].vsize  = rd32(buf+b+8);
		m->sec[i].vaddr  = rd32(buf+b+12);
		m->sec[i].rawsize= rd32(buf+b+16);
		m->sec[i].rawoff = rd32(buf+b+20);
	}
	m->resBaseOff = rva2off(m, resRva);
	if (!m->resBaseOff) { free(m); return NULL; }
	return m;
}

/* ---- public C entry points (called from the compat headers/impl) -------- */
extern "C" HMODULE_T bob_LoadLibrary(const char* path){
	if (!path) return NULL;
	FILE* f = fopen_nocase(path, "rb");
	if (!f) return NULL;
	fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
	if (sz <= 0) { fclose(f); return NULL; }
	uint8_t* buf = (uint8_t*)malloc(sz);
	size_t got = fread(buf, 1, sz, f); fclose(f);
	if (got != (size_t)sz) { free(buf); return NULL; }
	BobResModule* m = parse_pe(buf, sz);
	if (!m) { free(buf); return NULL; }
	if (getenv("BOB_TRACE_RES"))
		fprintf(stderr, "[res] loaded %s (%ld bytes, %d sections)\n", path, sz, m->nsec);
	return (HMODULE_T)m;
}

extern "C" void  bob_SetResourceHandle(HMODULE_T h){ if (h) g_resModule = (BobResModule*)h; }
extern "C" HMODULE_T bob_GetResourceHandle(void){ return (HMODULE_T)g_resModule; }

/* The main instance module (Mig.exe): holds the dialog templates / bitmaps that on
   Windows live in the .exe (AfxGetInstanceHandle). The localized miglang.dll only has a
   few resources, so resource lookups fall back to this. Loaded at startup (bob_main.cpp). */
static BobResModule* g_mainModule = NULL;
extern "C" void bob_set_main_module(HMODULE_T h){ if (h) g_mainModule = (BobResModule*)h; }

/* LoadString: RT_STRING (type 6). Strings are grouped 16 per bundle; the bundle
 * resource id is (id>>4)+1, the string is at index (id&15). Each entry in a
 * bundle is a WORD char-count followed by that many UTF-16LE code units. */
extern "C" int bob_load_string(HMODULE_T h, unsigned id, char* buf, int maxlen){
	if (buf && maxlen>0) buf[0]=0;
	BobResModule* m = h ? (BobResModule*)h : g_resModule;
	if (!m || !buf || maxlen<=0) return 0;
	uint32_t size=0;
	const uint8_t* p = res_get(m, 6 /*RT_STRING*/, (id>>4)+1, &size);
	if (getenv("MA_TRACE_STR")) { static int n=0; if(n++<30) fprintf(stderr,"[str] id=%u(0x%x) bundle=%u found=%d\n", id, id, (id>>4)+1, p!=0); }
	if (!p) return 0;
	const uint8_t* end = p + size;
	unsigned idx = id & 15;
	for (unsigned k=0;k<idx;k++){               /* skip preceding strings */
		if (p+2 > end) return 0;
		uint16_t n = rd16(p); p += 2 + (size_t)n*2;
	}
	if (p+2 > end) return 0;
	uint16_t n = rd16(p); p += 2;
	int o=0;
	for (uint16_t i=0;i<n && o<maxlen-1; i++){
		if (p+2 > end) break;
		uint16_t wc = rd16(p); p += 2;
		/* UTF-16 -> Latin-1-ish: pass <256 through, else '?' (UI text is Western) */
		buf[o++] = (wc < 256) ? (char)wc : '?';
	}
	buf[o]=0;
	return o;
}

/* Generic resource fetch (RT_BITMAP, custom, etc.) for FindResource/LoadResource. */
extern "C" const void* bob_res_get(HMODULE_T h, unsigned type, unsigned id, unsigned* outSize){
	BobResModule* m = h ? (BobResModule*)h : g_resModule;
	uint32_t sz=0; const uint8_t* p = res_get(m, type, id, &sz);
	if (!p && g_mainModule && m != g_mainModule)   /* fall back to Mig.exe (dialogs/bitmaps) */
		p = res_get(g_mainModule, type, id, &sz);
	if (outSize) *outSize = sz;
	return p;
}

/* ==========================================================================
 * S57 (ported from BoB S124): DIALOG (RT=5) + DLGINIT (RT=240) enumeration
 * over the INSTALLED build's resources (miglang.dll + Mig.exe = the BDG 0.85F
 * patched data — the PO's parity oracle). MA's per-IDD template loader
 * (ma_dlgtmpl.cpp) stays the primary consumer via bob_res_get; these
 * enumerators serve whole-module sweeps (membership audits, harnesses).
 * MA adaptation vs BoB: resources are split across TWO modules (the language
 * DLL and Mig.exe); enumeration walks miglang.dll first, then Mig.exe,
 * skipping any dlgId the language DLL already served — the same precedence
 * bob_res_get applies per-resource. All parsing stays offset-based (this TU
 * is built with -fpack-struct=1).
 * ======================================================================== */

/* enumerate the id-entries (skip named) one level below dirOff */
static int res_enum_ids(BobResModule* m, uint32_t dirOff,
                        int (*cb)(void* ctx, uint32_t id, uint32_t off, int isDir), void* ctx){
	const uint8_t* dir = m->buf + m->resBaseOff + dirOff;
	uint16_t nNamed = rd16(dir+12), nId = rd16(dir+14);
	const uint8_t* e = dir + 16 + (size_t)nNamed*8;
	int n = 0;
	for (int i=0;i<nId;i++,e+=8){
		uint32_t nameId = rd32(e);
		if (nameId & 0x80000000) continue;
		uint32_t off = rd32(e+4);
		if (!cb(ctx, nameId, off & 0x7fffffff, (off & 0x80000000)?1:0)) return n;
		n++;
	}
	return n;
}
/* leaf data of the first language under a name-level directory */
static const uint8_t* res_leaf(BobResModule* m, uint32_t nameDirOff, uint32_t* outSize){
	uint32_t off; int isDir;
	if (!res_first_child(m, nameDirOff, &off, &isDir) || isDir) return NULL;
	const uint8_t* leaf = m->buf + m->resBaseOff + off;
	uint32_t dataRva = rd32(leaf), sz = rd32(leaf+4);
	uint32_t doff = rva2off(m, dataRva);
	if (!doff || doff + sz > m->size) return NULL;
	if (outSize) *outSize = sz;
	return m->buf + doff;
}

/* DLGTEMPLATE sz_Or_Ord: 0x0000 none / 0xFFFF+WORD ordinal / UTF-16LE string.
 * Narrows the string into out (Latin-1-ish); ordinal formats as "#%u".
 * Returns the new offset, or -1 on overrun. */
static int sz_or_ord(const uint8_t* b, uint32_t n, uint32_t o, char* out, int outsz){
	if (out && outsz>0) out[0]=0;
	if (o+2 > n) return -1;
	uint16_t w = rd16(b+o);
	if (w == 0x0000) return (int)(o+2);
	if (w == 0xFFFF){
		if (o+4 > n) return -1;
		if (out) snprintf(out, outsz, "#%u", rd16(b+o+2));
		return (int)(o+4);
	}
	int k = 0;
	while (o+2 <= n){
		uint16_t c = rd16(b+o); o += 2;
		if (c == 0) return (int)o;
		if (out && k < outsz-1) out[k++] = (c < 256) ? (char)c : '?';
		if (out) out[k] = 0;
	}
	return -1;
}

/* per-dlgId dedup between the two modules (language DLL wins) */
#define MA_ENUM_MAX_DLGS 512
struct SeenDlgs { uint32_t id[MA_ENUM_MAX_DLGS]; int n; };
static int seen_has(SeenDlgs* s, uint32_t id){ for (int i=0;i<s->n;i++) if (s->id[i]==id) return 1; return 0; }
static void seen_add(SeenDlgs* s, uint32_t id){ if (s->n < MA_ENUM_MAX_DLGS) s->id[s->n++]=id; }

struct DlgEnumCtx {
	BobResModule* m;
	void (*itemcb)(void* ctx, int dlgId, int ctrlId, int x, int y, int w, int h, const char* cls);
	void* ctx;
	int count;
	SeenDlgs* seen;   /* dlgIds already served by an earlier module (skip) */
	SeenDlgs* mine;   /* dlgIds this pass serves (record) */
};

/* parse one DLGTEMPLATE / DLGTEMPLATEEX resource, firing itemcb per control */
static int dlg_enum_one(void* vc, uint32_t dlgId, uint32_t off, int isDir){
	DlgEnumCtx* c = (DlgEnumCtx*)vc;
	if (!isDir) return 1;
	if (c->seen && seen_has(c->seen, dlgId)) return 1;
	uint32_t n=0; const uint8_t* b = res_leaf(c->m, off, &n);
	if (!b || n < 18) return 1;
	if (c->mine) seen_add(c->mine, dlgId);
	int ex = (rd16(b) == 1 && rd16(b+2) == 0xFFFF);
	uint32_t style, o; uint16_t cdit;
	if (ex){ style = rd32(b+12); cdit = rd16(b+16); o = 26; }
	else   { style = rd32(b);    cdit = rd16(b+8);  o = 18; }
	char tmp[128];
	int r = sz_or_ord(b, n, o, NULL, 0); if (r < 0) return 1; o = r;      /* menu  */
	r = sz_or_ord(b, n, o, NULL, 0);     if (r < 0) return 1; o = r;      /* class */
	r = sz_or_ord(b, n, o, NULL, 0);     if (r < 0) return 1; o = r;      /* title */
	if (style & 0x40){                                                    /* DS_SETFONT */
		o += 2;                                                           /* point size */
		if (ex) o += 4;                                                   /* weight+italic+charset */
		r = sz_or_ord(b, n, o, NULL, 0); if (r < 0) return 1; o = r;      /* face */
	}
	for (uint16_t i=0;i<cdit;i++){
		o = (o+3) & ~3u;                                                  /* DWORD align */
		int32_t x,y,w,h; uint32_t cid;
		if (ex){
			if (o+24 > n) return 1;
			x=(int16_t)rd16(b+o+12); y=(int16_t)rd16(b+o+14);
			w=(int16_t)rd16(b+o+16); h=(int16_t)rd16(b+o+18);
			cid = rd32(b+o+20); o += 24;
		} else {
			if (o+18 > n) return 1;
			x=(int16_t)rd16(b+o+8);  y=(int16_t)rd16(b+o+10);
			w=(int16_t)rd16(b+o+12); h=(int16_t)rd16(b+o+14);
			cid = rd16(b+o+16); o += 18;
		}
		r = sz_or_ord(b, n, o, tmp, sizeof tmp); if (r < 0) return 1; o = r;   /* class */
		c->itemcb(c->ctx, (int)dlgId, (int)cid, x, y, w, h, tmp);
		c->count++;
		r = sz_or_ord(b, n, o, NULL, 0);         if (r < 0) return 1; o = r;   /* text  */
		if (o+2 > n) return 1;
		/* creation data: classic nonzero count INCLUDES the size WORD; EX count excludes it */
		{ uint16_t cx = rd16(b+o); o += cx ? (ex ? 2u+cx : (uint32_t)cx) : 2u; }
	}
	return 1;
}

/* Enumerate every control of every DIALOG template across both modules.
 * Returns the item count (0 when no module / no dialogs). */
extern "C" int bob_res_enum_dialog_items(
		void (*itemcb)(void* ctx, int dlgId, int ctrlId, int x, int y, int w, int h, const char* cls),
		void* ctx){
	SeenDlgs seen; seen.n = 0;
	int total = 0;
	BobResModule* mods[2] = { g_resModule, g_mainModule };
	for (int mi = 0; mi < 2; mi++){
		BobResModule* m = mods[mi];
		if (!m || (mi == 1 && m == mods[0])) continue;
		uint32_t off; int isDir;
		if (!res_find_entry(m, 0, 5 /*RT_DIALOG*/, &off, &isDir) || !isDir) continue;
		DlgEnumCtx c = { m, itemcb, ctx, 0, mi ? &seen : NULL, &seen };
		res_enum_ids(m, off, dlg_enum_one, &c);
		total += c.count;
	}
	return total;
}

/* DLGINIT (type 240): per-dialog stream of {WORD ctrlId, WORD msg, DWORD len, bytes}. */
struct InitEnumCtx {
	BobResModule* m;
	void (*initcb)(void* ctx, int dlgId, int ctrlId, const unsigned char* data, int len);
	void* ctx;
	int count;
	SeenDlgs* seen;
	SeenDlgs* mine;
};
static int init_enum_one(void* vc, uint32_t dlgId, uint32_t off, int isDir){
	InitEnumCtx* c = (InitEnumCtx*)vc;
	if (!isDir) return 1;
	if (c->seen && seen_has(c->seen, dlgId)) return 1;
	uint32_t n=0; const uint8_t* b = res_leaf(c->m, off, &n);
	if (!b) return 1;
	if (c->mine) seen_add(c->mine, dlgId);
	uint32_t o = 0;
	while (o+8 <= n){
		uint16_t cid = rd16(b+o);
		if (cid == 0) break;
		uint32_t len = rd32(b+o+4);
		if (o+8+len > n) break;
		c->initcb(c->ctx, (int)dlgId, (int)cid, b+o+8, (int)len);
		c->count++;
		o += 8 + len;
	}
	return 1;
}
extern "C" int bob_res_enum_dlginit(
		void (*initcb)(void* ctx, int dlgId, int ctrlId, const unsigned char* data, int len),
		void* ctx){
	SeenDlgs seen; seen.n = 0;
	int total = 0;
	BobResModule* mods[2] = { g_resModule, g_mainModule };
	for (int mi = 0; mi < 2; mi++){
		BobResModule* m = mods[mi];
		if (!m || (mi == 1 && m == mods[0])) continue;
		uint32_t off; int isDir;
		if (!res_find_entry(m, 0, 240 /*DLGINIT*/, &off, &isDir) || !isDir) continue;
		InitEnumCtx c = { m, initcb, ctx, 0, mi ? &seen : NULL, &seen };
		res_enum_ids(m, off, init_enum_one, &c);
		total += c.count;
	}
	return total;
}

#endif /* FF_LINUX */
