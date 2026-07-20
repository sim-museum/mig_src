/* BoB Linux port - CRT/file + DirectX/GUID external stubs.
 *
 *  - Case-insensitive file resolution + MSVC _findfirst family + _itoa
 *    (game data has Windows-case names; ported from the FreeFalcon linux_stubs).
 *  - DirectDraw/DirectInput/DirectSound creation entry points: stubbed to fail
 *    (the real device bring-up is SDL2/GL/OpenAL, done later).
 *  - The GUIDs the game references by name: zero-filled definitions (only the
 *    stub DX paths consume them, so the values don't matter yet).
 */
#ifdef FF_LINUX

#define FF_NO_FOPEN_REDIRECT	/* this file implements fopen_nocase itself */

/* CRITICAL: the whole game builds with -fpack-struct=1 (MSVC /Zp1). That packing
   must NOT reach libc system structs -- struct stat/dirent are filled by glibc
   with the NATIVE layout, so reading their fields through a packed declaration
   misaligns every member (st_mode garbage -> S_ISDIR wrong; and stat() overruns
   the smaller packed struct -> stack smash). #pragma pack(push,8) around the
   system headers overrides -fpack-struct=1 for just these structs and restores
   the native ABI; game-facing structs (io.h _finddata_t, GUID, ...) stay packed. */
#pragma pack(push,8)
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fnmatch.h>
/* pthread.h MUST be inside the pack(8) region for the same reason as struct stat:
   the path-cache mutex is filled in by glibc with the NATIVE pthread_mutex_t layout,
   and -fpack-struct=1 would misalign every member of it. */
#include <pthread.h>
#pragma pack(pop)

#include "compat_types.h"
#include "compat_winbase.h"
#include "io.h"
#include "ddraw.h"
#include "dsound.h"
#include "dinput.h"

/* ===== case-insensitive path resolution + fopen/open ===================== */
/* Wine-style drive_c mapping: the game's stored paths are Windows drive-absolute
   (e.g. "\Program Files\Rowan Software\Battle Of Britain\landscap\DIR.DIR" or
   "C:\..."), which become "/Program Files/..." after backslash conversion and
   don't exist from the Linux filesystem root. Set BOB_DRIVE_C to the Wine drive_c
   directory (containing "Program Files") and such paths resolve under it. */
static int resolve_nocase_uncached(const char *filepath, char *resolved, size_t resolvedSize) {
	char work[2048];
	size_t i = 0;
	for (; filepath[i] && i < sizeof(work) - 1; i++)
		work[i] = (filepath[i] == '\\') ? '/' : filepath[i];
	work[i] = '\0';

	/* Map a Windows drive-absolute path onto $BOB_DRIVE_C. */
	const char *drive_c = getenv("BOB_DRIVE_C");
	const char *rem = NULL;
	if (((work[0]>='A'&&work[0]<='Z')||(work[0]>='a'&&work[0]<='z')) && work[1]==':')
		rem = work + 2;					/* "C:/..." -> "/..." */
	else if (work[0] == '/')
		rem = work;						/* drive-relative "/..." */
	if (drive_c && drive_c[0] && rem && rem[0]=='/') {
		char mapped[2048];
		snprintf(mapped, sizeof(mapped), "%s%s", drive_c, rem);
		strncpy(work, mapped, sizeof(work)-1); work[sizeof(work)-1]='\0';
	}

	if (access(work, F_OK) == 0) {
		strncpy(resolved, work, resolvedSize - 1); resolved[resolvedSize - 1] = '\0'; return 0;
	}
	char out[2048];
	const char *p = work;
	if (*p == '/') { out[0] = '/'; out[1] = '\0'; p++; } else out[0] = '\0';
	char comp[512];
	while (*p) {
		size_t ci = 0;
		while (*p && *p != '/' && ci < sizeof(comp) - 1) comp[ci++] = *p++;
		comp[ci] = '\0';
		while (*p == '/') p++;
		if (ci == 0) continue;
		char candidate[2048];
		snprintf(candidate, sizeof(candidate), "%s%s%s", out, (out[0] && out[strlen(out)-1] != '/') ? "/" : "", comp);
		if (access(candidate, F_OK) == 0) { strncpy(out, candidate, sizeof(out)-1); out[sizeof(out)-1]='\0'; continue; }
		const char *dirPath = out[0] ? out : ".";
		DIR *d = opendir(dirPath);
		if (!d) return -1;
		struct dirent *e; int found = 0;
		while ((e = readdir(d)) != NULL) {
			if (strcasecmp(e->d_name, comp) == 0) {
				snprintf(candidate, sizeof(candidate), "%s%s%s", out, (out[0] && out[strlen(out)-1] != '/') ? "/" : "", e->d_name);
				strncpy(out, candidate, sizeof(out)-1); out[sizeof(out)-1]='\0'; found = 1; break;
			}
		}
		closedir(d);
		if (!found) {
			snprintf(candidate, sizeof(candidate), "%s%s%s", out, (out[0] && out[strlen(out)-1] != '/') ? "/" : "", comp);
			if (*p) { strncat(candidate, "/", sizeof(candidate)-strlen(candidate)-1); strncat(candidate, p, sizeof(candidate)-strlen(candidate)-1); }
			strncpy(resolved, candidate, resolvedSize-1); resolved[resolvedSize-1]='\0'; return -1;
		}
	}
	strncpy(resolved, out, resolvedSize-1); resolved[resolvedSize-1]='\0'; return 0;
}

/* ===== resolved-path cache (MA_LINUX) ====================================
   resolve_nocase_uncached() opendir()s EVERY path component on every miss of the
   fast access() path -- and the engine re-resolves the same few hundred asset
   paths thousands of times (per-dialog artwork, per-frame shape/texture loads).
   This memoises the (input -> resolved) mapping.

   CORRECTNESS RULES (the game creates and writes files -- savegames, prefs -- so a
   naive permanent cache silently breaks saves):
     1. ONLY successful resolutions (rc == 0) are cached. A negative result is
        exactly the case that a later create/write turns positive, so caching it
        would be the staleness bug; misses always re-walk the directories.
     2. ANY write/create-mode open flushes the WHOLE cache (not just the one key):
        creating a file changes the directory listing that every *sibling* lookup
        under that directory depends on. Writes are rare (a few per session --
        save, prefs, log) so a full flush is far cheaper than being subtly wrong.
     3. The key is the raw pre-mapping input string; the mapping only depends on
        it and on $BOB_DRIVE_C, which bob_main.cpp sets once before any I/O.
   Escape hatch (per the port's A/B convention): MA_NO_PATHCACHE=1 bypasses the
   cache entirely, restoring the original resolve-every-time behaviour.
   Trace: MA_TRACE_PATHCACHE=1 logs hit/miss/flush + a periodic summary. */
#define MA_PC_SLOTS   8192			/* power of two; open addressing */
#define MA_PC_MAXFILL 6144			/* 75% -- keep probe chains short */
struct MaPathCacheEnt { char *key; char *val; };
static MaPathCacheEnt ma_pc_tab[MA_PC_SLOTS];
static int   ma_pc_used = 0;
static int   ma_pc_on = -1;			/* -1 = not probed yet */
static int   ma_pc_trace = 0;
static unsigned long ma_pc_hits = 0, ma_pc_misses = 0, ma_pc_flushes = 0;
static double        ma_pc_walk_ms = 0.0;	/* time spent in the uncached walk (trace only) */
static pthread_mutex_t ma_pc_lock = PTHREAD_MUTEX_INITIALIZER;

static unsigned ma_pc_hash(const char *s) {			/* FNV-1a */
	unsigned h = 2166136261u;
	while (*s) { h ^= (unsigned char)*s++; h *= 16777619u; }
	return h;
}
/* caller holds ma_pc_lock. Periodic summary: total walk time is the ceiling on what
   the cache can ever save, i.e. the honest measure of whether it is worth anything. */
static void ma_pc_stats_locked(void) {
	if (((ma_pc_hits + ma_pc_misses) % 500) != 0) return;
	fprintf(stderr, "[pathcache] stats: hits=%lu misses=%lu entries=%d flushes=%lu walk=%.1fms\n",
	        ma_pc_hits, ma_pc_misses, ma_pc_used, ma_pc_flushes, ma_pc_walk_ms);
}
/* caller holds ma_pc_lock */
static void ma_pc_clear_locked(void) {
	for (int i = 0; i < MA_PC_SLOTS; i++) {
		free(ma_pc_tab[i].key); free(ma_pc_tab[i].val);
		ma_pc_tab[i].key = ma_pc_tab[i].val = NULL;
	}
	ma_pc_used = 0;
}
extern "C" void ma_pathcache_flush(void) {
	pthread_mutex_lock(&ma_pc_lock);
	if (ma_pc_used) {
		ma_pc_flushes++;
		if (ma_pc_trace > 0)
			fprintf(stderr, "[pathcache] FLUSH (%d entries; hits=%lu misses=%lu)\n",
			        ma_pc_used, ma_pc_hits, ma_pc_misses);
		ma_pc_clear_locked();
	}
	pthread_mutex_unlock(&ma_pc_lock);
}
static int resolve_nocase(const char *filepath, char *resolved, size_t resolvedSize) {
	if (!filepath) return -1;
	pthread_mutex_lock(&ma_pc_lock);
	if (ma_pc_on < 0) {			/* one-time probe, under the lock */
		ma_pc_on    = getenv("MA_NO_PATHCACHE") ? 0 : 1;
		ma_pc_trace = getenv("MA_TRACE_PATHCACHE") ? 1 : 0;
		if (ma_pc_trace)
			fprintf(stderr, "[pathcache] %s\n", ma_pc_on ? "enabled" : "DISABLED (MA_NO_PATHCACHE)");
	}
	if (!ma_pc_on) {
		/* Bypassed, but still counted/timed so MA_NO_PATHCACHE=1 gives a directly
		   comparable A/B against the cached run. */
		int t = ma_pc_trace; ma_pc_misses++;
		if (t) { fprintf(stderr, "[pathcache] BYPASS [%s]\n", filepath); ma_pc_stats_locked(); }
		pthread_mutex_unlock(&ma_pc_lock);
		struct timespec b0, b1;
		if (t) clock_gettime(CLOCK_MONOTONIC, &b0);
		int brc = resolve_nocase_uncached(filepath, resolved, resolvedSize);
		if (t) {
			clock_gettime(CLOCK_MONOTONIC, &b1);
			double bms = (b1.tv_sec - b0.tv_sec) * 1e3 + (b1.tv_nsec - b0.tv_nsec) / 1e6;
			pthread_mutex_lock(&ma_pc_lock); ma_pc_walk_ms += bms; pthread_mutex_unlock(&ma_pc_lock);
		}
		return brc;
	}

	unsigned h = ma_pc_hash(filepath) & (MA_PC_SLOTS - 1);
	unsigned slot = h;
	for (int probe = 0; probe < MA_PC_SLOTS; probe++) {
		MaPathCacheEnt *e = &ma_pc_tab[slot];
		if (!e->key) break;					/* empty -> miss, `slot` is the insert point */
		if (strcmp(e->key, filepath) == 0) {
			strncpy(resolved, e->val, resolvedSize - 1); resolved[resolvedSize - 1] = '\0';
			ma_pc_hits++;
			if (ma_pc_trace) { fprintf(stderr, "[pathcache] HIT  [%s] -> %s\n", filepath, resolved); ma_pc_stats_locked(); }
			pthread_mutex_unlock(&ma_pc_lock);
			return 0;
		}
		slot = (slot + 1) & (MA_PC_SLOTS - 1);
	}
	ma_pc_misses++;
	if (ma_pc_trace) { fprintf(stderr, "[pathcache] MISS [%s]\n", filepath); ma_pc_stats_locked(); }
	pthread_mutex_unlock(&ma_pc_lock);

	/* Resolve OUTSIDE the lock: the directory walk is the slow part and must not
	   serialise the sim thread against the loader thread. */
	struct timespec t0, t1;
	if (ma_pc_trace) clock_gettime(CLOCK_MONOTONIC, &t0);
	int rc = resolve_nocase_uncached(filepath, resolved, resolvedSize);
	if (ma_pc_trace) {
		clock_gettime(CLOCK_MONOTONIC, &t1);
		double ms = (t1.tv_sec - t0.tv_sec) * 1e3 + (t1.tv_nsec - t0.tv_nsec) / 1e6;
		pthread_mutex_lock(&ma_pc_lock); ma_pc_walk_ms += ms; pthread_mutex_unlock(&ma_pc_lock);
	}
	if (rc != 0) return rc;					/* rule 1: never cache a negative */

	pthread_mutex_lock(&ma_pc_lock);
	if (ma_pc_used >= MA_PC_MAXFILL) ma_pc_clear_locked();	/* full: cheapest correct policy */
	slot = ma_pc_hash(filepath) & (MA_PC_SLOTS - 1);
	for (int probe = 0; probe < MA_PC_SLOTS; probe++) {
		MaPathCacheEnt *e = &ma_pc_tab[slot];
		if (!e->key) {
			e->key = strdup(filepath); e->val = strdup(resolved);
			if (e->key && e->val) ma_pc_used++;
			else { free(e->key); free(e->val); e->key = e->val = NULL; }
			break;
		}
		if (strcmp(e->key, filepath) == 0) break;	/* raced with another thread */
		slot = (slot + 1) & (MA_PC_SLOTS - 1);
	}
	pthread_mutex_unlock(&ma_pc_lock);
	return 0;
}

/* Path resolver for std stream users (BIStream/BOStream in bstream.h) that bypass
   fopen_nocase: maps Windows '\' / drive-absolute / case to a real Linux path.
   Always fills `out` with the best-effort resolved-or-mapped path. */
extern "C" int bob_resolve_path(const char *in, char *out, unsigned long outsz) {
	if (!in || !out || outsz == 0) return 0;
	out[0] = '\0';
	int rc = resolve_nocase(in, out, (size_t)outsz);
	if (getenv("BOB_TRACE_FOPEN"))
		fprintf(stderr, "[resolve] [%s] -> [%s] rc=%d\n", in, out, rc);
	return out[0] ? 1 : 0;
}

/* This install ships only the 800/1024/1280-res dialog artwork (Artwork/DIAL800 etc.), not
   DIAL640. When a "dial640" asset isn't found, retry against "dial800" so dialog screens get
   a background instead of a (now non-fatal) file-not-found. Case-insensitive substitution. */
static int redirect_dial640(const char *in, char *out, size_t outsz) {
	const char *l = in; const char *hit = NULL;
	for (; *l; l++) if ((l[0]=='d'||l[0]=='D') && strncasecmp(l, "dial640", 7) == 0) { hit = l; break; }
	if (!hit) return 0;
	size_t pre = (size_t)(hit - in);
	if (pre + 7 >= outsz) return 0;
	memcpy(out, in, pre);
	memcpy(out + pre, "dial800", 7);
	strncpy(out + pre + 7, hit + 7, outsz - pre - 8); out[outsz-1] = '\0';
	return 1;
}

extern "C" FILE *fopen_nocase(const char *filepath, const char *mode) {
	if (!filepath || !mode) return NULL;
	static const int trace = getenv("BOB_TRACE_FOPEN") ? 1 : 0;
	/* Rule 2 (see the path-cache block): any create/write invalidates the cached
	   directory-derived resolutions, so flush BEFORE resolving this one. Covers
	   savegames ("w"/"wb"), prefs, logs ("a") and read-write ("r+") reopens. */
	if (strchr(mode, 'w') || strchr(mode, 'a') || strchr(mode, '+'))
		ma_pathcache_flush();
	char resolved[2048];
	if (resolve_nocase(filepath, resolved, sizeof(resolved)) == 0) {
		if (trace) fprintf(stderr, "[fopen] OK   [%s] (%s) -> %s\n", filepath, mode, resolved);
		struct stat st;
		if (stat(resolved, &st) == 0 && S_ISDIR(st.st_mode)) return NULL;
		return fopen(resolved, mode);
	}
	/* dial640 -> dial800 fallback (read paths only) */
	if (!strchr(mode, 'w') && !strchr(mode, 'a')) {
		char alt[2048];
		if (redirect_dial640(filepath, alt, sizeof(alt)) && resolve_nocase(alt, resolved, sizeof(resolved)) == 0) {
			if (trace) fprintf(stderr, "[fopen] OK   [%s] -> dial800 -> %s\n", filepath, resolved);
			struct stat st; if (stat(resolved, &st) == 0 && S_ISDIR(st.st_mode)) return NULL;
			return fopen(resolved, mode);
		}
	}
	if (trace) fprintf(stderr, "[fopen] MISS [%s] (%s)\n", filepath, mode);
	if (strchr(mode, 'w') || strchr(mode, 'a')) return fopen(resolved, mode);
	return NULL;
}
extern "C" int open_nocase(const char *filepath, int flags, int mode) {
	if (!filepath) return -1;
	if (flags & (O_CREAT | O_WRONLY | O_RDWR | O_TRUNC))	/* rule 2: see fopen_nocase */
		ma_pathcache_flush();
	char resolved[2048];
	if (resolve_nocase(filepath, resolved, sizeof(resolved)) == 0) return open(resolved, flags, mode);
	if (flags & O_CREAT) return open(resolved, flags, mode);
	return -1;
}

/* ===== MSVC _findfirst family =========================================== */
struct BOB_FIND_CTX { DIR *dir; char dirPath[1024]; char pattern[512]; };
static void fill_finddata(BOB_FIND_CTX *ctx, const char *name, struct _finddata_t *out) {
	memset(out, 0, sizeof(*out));
	strncpy(out->name, name, sizeof(out->name) - 1);
	char full[2048]; snprintf(full, sizeof(full), "%s/%s", ctx->dirPath, name);
	struct stat st;
	if (stat(full, &st) == 0) {
		out->attrib = S_ISDIR(st.st_mode) ? _A_SUBDIR : _A_NORMAL;
		out->size = (unsigned long)st.st_size;
		out->time_write = (long)st.st_mtime; out->time_access = (long)st.st_atime; out->time_create = (long)st.st_ctime;
	}
}
extern "C" intptr_t _findfirst(const char *filespec, struct _finddata_t *fileinfo) {
	if (!filespec) return -1;
	char work[1536]; size_t i = 0;
	for (; filespec[i] && i < sizeof(work)-1; i++) work[i] = (filespec[i]=='\\')?'/':filespec[i];
	work[i] = '\0';
	BOB_FIND_CTX *ctx = new BOB_FIND_CTX;
	char *slash = strrchr(work, '/');
	if (slash) {
		*slash = '\0'; char rd[1024];
		if (resolve_nocase(work, rd, sizeof(rd)) == 0) strncpy(ctx->dirPath, rd, sizeof(ctx->dirPath)-1);
		else strncpy(ctx->dirPath, work, sizeof(ctx->dirPath)-1);
		ctx->dirPath[sizeof(ctx->dirPath)-1]='\0'; strncpy(ctx->pattern, slash+1, sizeof(ctx->pattern)-1);
	} else { strcpy(ctx->dirPath, "."); strncpy(ctx->pattern, work, sizeof(ctx->pattern)-1); }
	ctx->pattern[sizeof(ctx->pattern)-1]='\0';
	ctx->dir = opendir(ctx->dirPath);
	if (getenv("MA_TRACE_FIND")) fprintf(stderr,"[find] _findfirst spec=[%s] dir=[%s] pat=[%s] opendir=%s\n",filespec,ctx->dirPath,ctx->pattern,ctx->dir?"ok":"FAIL");
	if (!ctx->dir) { delete ctx; return -1; }
	struct dirent *e;
	while ((e = readdir(ctx->dir)) != NULL) {
		if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
		if (fnmatch(ctx->pattern, e->d_name, FNM_CASEFOLD) == 0) { fill_finddata(ctx, e->d_name, fileinfo);
			if (getenv("MA_TRACE_FIND")) fprintf(stderr,"[find] _findfirst -> [%s]\n",e->d_name);
			return (intptr_t)ctx; }
	}
	if (getenv("MA_TRACE_FIND")) fprintf(stderr,"[find] _findfirst -> NONE in [%s]\n",ctx->dirPath);
	closedir(ctx->dir); delete ctx; return -1;
}
extern "C" int _findnext(intptr_t handle, struct _finddata_t *fileinfo) {
	if (handle == -1 || handle == 0) return -1;
	BOB_FIND_CTX *ctx = (BOB_FIND_CTX *)handle; struct dirent *e;
	while ((e = readdir(ctx->dir)) != NULL) {
		if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
		if (fnmatch(ctx->pattern, e->d_name, FNM_CASEFOLD) == 0) { fill_finddata(ctx, e->d_name, fileinfo); return 0; }
	}
	return -1;
}
extern "C" int _findclose(intptr_t handle) {
	if (handle == -1 || handle == 0) return -1;
	BOB_FIND_CTX *ctx = (BOB_FIND_CTX *)handle; closedir(ctx->dir); delete ctx; return 0;
}
extern "C" char *_itoa(int value, char *str, int radix) {
	if (radix == 16) sprintf(str, "%x", value);
	else if (radix == 8) sprintf(str, "%o", value);
	else sprintf(str, "%d", value);
	return str;
}

/* ===== DirectX creation entry points (stub: all fail) =================== */
/* DirectDrawCreateEx + DirectDrawEnumerateExA + DirectInputCreateA now live in
 * bob_video.cpp (the SDL2/OpenGL DirectDraw7/D3D7 backend + DirectInput stub). */
HRESULT DirectSoundCreate(GUID *, LPDIRECTSOUND *ppDS, IUnknown *) { if (ppDS) *ppDS = NULL; return E_FAIL; }
HRESULT DirectSoundEnumerateA(LPDSENUMCALLBACKA, void *) { return 0; }

/* ===== GUIDs the game references (zero-filled; only stub-DX paths use) ====
   `extern` is required: a file-scope `const` has internal linkage in C++, so
   without it these wouldn't be visible to the TUs that reference them. */
#define BOBGUID(n) extern const GUID n; extern const GUID n = {0,0,0,{0,0,0,0,0,0,0,0}}
BOBGUID(BOB_GUID);
BOBGUID(CLSID_DirectMusicSegment);
BOBGUID(IID_IDirectMusicObject);
BOBGUID(IID_IDirectMusicSegment);
BOBGUID(GUID_StandardMIDIFile);
BOBGUID(GUID_PerfMasterVolume);
BOBGUID(DPSPGUID_IPX);
BOBGUID(DPSPGUID_TCPIP);
BOBGUID(DPSPGUID_SERIAL);
BOBGUID(DPSPGUID_MODEM);
BOBGUID(GUID_SysKeyboard);
/* GUID_Joystick must be DISTINCT from the all-zero BOBGUIDs (esp. GUID_SysKeyboard),
   else DI_CreateDevice's rguid==GUID_SysKeyboard test would steal the joystick. */
extern const GUID GUID_Joystick;
extern const GUID GUID_Joystick = {0x6f1d2b70,0xd5a0,0x11cf,{0xbf,0xc7,0x44,0x45,0x53,0x54,0x00,0x00}};
extern const GUID GUID_SysMouse;
extern const GUID GUID_SysMouse = {0x6f1d2b60,0xd5a0,0x11cf,{0xbf,0xc7,0x44,0x45,0x53,0x54,0x00,0x00}};
/* Axis GUIDs need DISTINCT real DirectInput values (NOT the all-zero BOBGUID): the
   joystick object-enum classifier (SCONTROL DIEnumDeviceObjectsProc) maps each axis to a
   flight role by guidType==GUID_XAxis / GUID_YAxis / GUID_Rz... . If they were all the
   same all-zero GUID, every axis reads as GUID_XAxis -> the stick pair never forms and
   aileron/elevator/rudder/throttle are mis-assigned. Values from SRC/H/DINPUT.H. */
extern const GUID GUID_XAxis;  extern const GUID GUID_XAxis  = {0xA36D02E0,0xC9F3,0x11CF,{0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00}};
extern const GUID GUID_YAxis;  extern const GUID GUID_YAxis  = {0xA36D02E1,0xC9F3,0x11CF,{0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00}};
extern const GUID GUID_ZAxis;  extern const GUID GUID_ZAxis  = {0xA36D02E2,0xC9F3,0x11CF,{0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00}};
extern const GUID GUID_RxAxis; extern const GUID GUID_RxAxis = {0xA36D02F4,0xC9F3,0x11CF,{0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00}};
extern const GUID GUID_RyAxis; extern const GUID GUID_RyAxis = {0xA36D02F5,0xC9F3,0x11CF,{0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00}};
extern const GUID GUID_RzAxis; extern const GUID GUID_RzAxis = {0xA36D02E3,0xC9F3,0x11CF,{0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00}};
#undef BOBGUID
/* GUID_Key/Button/POV need DISTINCT non-zero values: the joystick object-enum
   (ANALOGUE DIEnumDeviceObjectsProc) classifies objects by guidType==GUID_Button/
   Key/POV; if they were all the all-zero BOBGUID, every object would read as a
   button and axes would never map. Real DirectInput object-type GUIDs: */
#define BOBGUID(n) extern const GUID n; extern const GUID n = {0,0,0,{0,0,0,0,0,0,0,0}}
extern const GUID GUID_Key;    extern const GUID GUID_Key    = {0x55728220,0xd33c,0x11cf,{0xbf,0xc7,0x44,0x45,0x53,0x54,0x00,0x00}};
extern const GUID GUID_Button; extern const GUID GUID_Button = {0xa36d02f0,0xc9f3,0x11cf,{0xbf,0xc7,0x44,0x45,0x53,0x54,0x00,0x00}};
extern const GUID GUID_POV;    extern const GUID GUID_POV    = {0xa36d02f2,0xc9f3,0x11cf,{0xbf,0xc7,0x44,0x45,0x53,0x54,0x00,0x00}};
BOBGUID(GUID_ConstantForce);
BOBGUID(GUID_RampForce);
BOBGUID(GUID_Square);
BOBGUID(GUID_Sine);
BOBGUID(GUID_Triangle);
BOBGUID(GUID_SawtoothUp);
BOBGUID(GUID_SawtoothDown);
BOBGUID(GUID_Spring);
BOBGUID(GUID_Damper);
BOBGUID(GUID_Inertia);
BOBGUID(GUID_Friction);
BOBGUID(GUID_RandomNoise);
BOBGUID(GUID_Download);
BOBGUID(GUID_Unload);
#undef BOBGUID

/* ===== Win32 FindFirstFile — real opendir/fnmatch enumeration ============
   The game enumerates save games (loadgame, "*.sav"), replays, etc. via
   FindFirstFile/FindNextFile (Fileman.cpp, __MSVC__ branch). This was stubbed
   to "no matches", so the loadgame/replay lists were always empty. Implement
   it over opendir/readdir + fnmatch, reusing resolve_nocase for the Wine
   C:\rowan\mig path mapping (same machinery as _findfirst above). Fills
   cFileName with the full POSIX name (Fileman reads cFileName under MA_LINUX;
   cAlternateFileName is the 14-byte 8.3 slot the original Win build read). */
#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)
#endif
static void fill_find32(BOB_FIND_CTX *ctx, const char *name, LPWIN32_FIND_DATAA out) {
	memset(out, 0, sizeof(*out));
	strncpy(out->cFileName, name, sizeof(out->cFileName) - 1);
	/* cAlternateFileName is only 14 bytes (8.3); truncate defensively */
	strncpy(out->cAlternateFileName, name, sizeof(out->cAlternateFileName) - 1);
	char full[2048]; snprintf(full, sizeof(full), "%s/%s", ctx->dirPath, name);
	struct stat st;
	if (stat(full, &st) == 0) {
		out->dwFileAttributes = S_ISDIR(st.st_mode) ? 0x10 /*FILE_ATTRIBUTE_DIRECTORY*/ : 0x80 /*NORMAL*/;
		out->nFileSizeLow = (DWORD)st.st_size;
	}
}
HANDLE FindFirstFileA(LPCSTR lpFileName, LPWIN32_FIND_DATAA lpFindFileData) {
	if (!lpFileName || !lpFindFileData) return INVALID_HANDLE_VALUE;
	char work[1536]; size_t i = 0;
	for (; lpFileName[i] && i < sizeof(work)-1; i++) work[i] = (lpFileName[i]=='\\')?'/':lpFileName[i];
	work[i] = '\0';
	BOB_FIND_CTX *ctx = new BOB_FIND_CTX;
	char *slash = strrchr(work, '/');
	if (slash) {
		*slash = '\0'; char rd[1024];
		if (resolve_nocase(work, rd, sizeof(rd)) == 0) strncpy(ctx->dirPath, rd, sizeof(ctx->dirPath)-1);
		else strncpy(ctx->dirPath, work, sizeof(ctx->dirPath)-1);
		ctx->dirPath[sizeof(ctx->dirPath)-1]='\0'; strncpy(ctx->pattern, slash+1, sizeof(ctx->pattern)-1);
	} else { strcpy(ctx->dirPath, "."); strncpy(ctx->pattern, work, sizeof(ctx->pattern)-1); }
	ctx->pattern[sizeof(ctx->pattern)-1]='\0';
	if (ctx->pattern[0] == '\0') strcpy(ctx->pattern, "*");
	ctx->dir = opendir(ctx->dirPath);
	if (!ctx->dir) { delete ctx; return INVALID_HANDLE_VALUE; }
	struct dirent *e;
	while ((e = readdir(ctx->dir)) != NULL) {
		if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
		if (fnmatch(ctx->pattern, e->d_name, FNM_CASEFOLD) == 0) { fill_find32(ctx, e->d_name, lpFindFileData);
			if (getenv("MA_TRACE_FIND")) fprintf(stderr,"[find] FindFirstFile dir=[%s] pat=[%s] -> first=[%s]\n",ctx->dirPath,ctx->pattern,e->d_name);
			return (HANDLE)ctx; }
	}
	if (getenv("MA_TRACE_FIND")) fprintf(stderr,"[find] FindFirstFile dir=[%s] pat=[%s] -> NONE\n",ctx->dirPath,ctx->pattern);
	closedir(ctx->dir); delete ctx; return INVALID_HANDLE_VALUE;
}
BOOL FindNextFileA(HANDLE hFindFile, LPWIN32_FIND_DATAA lpFindFileData) {
	if (hFindFile == INVALID_HANDLE_VALUE || !hFindFile || !lpFindFileData) return FALSE;
	BOB_FIND_CTX *ctx = (BOB_FIND_CTX *)hFindFile; struct dirent *e;
	while ((e = readdir(ctx->dir)) != NULL) {
		if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
		if (fnmatch(ctx->pattern, e->d_name, FNM_CASEFOLD) == 0) { fill_find32(ctx, e->d_name, lpFindFileData); return TRUE; }
	}
	return FALSE;
}
BOOL FindClose(HANDLE hFindFile) {
	if (hFindFile == INVALID_HANDLE_VALUE || !hFindFile) return FALSE;
	BOB_FIND_CTX *ctx = (BOB_FIND_CTX *)hFindFile; if (ctx->dir) closedir(ctx->dir); delete ctx; return TRUE;
}

/* DirectInput keyboard data format (consumed only by the stub DInput path) */
extern const DIDATAFORMAT c_dfDIKeyboard;
const DIDATAFORMAT c_dfDIKeyboard = {0};

/* ===== MFC app accessors ================================================= */
/* g_pBobApp is set to &theApp in MIG.CPP; AfxGetApp() returns the real app so
   framework code that reaches back through AfxGetApp() sees the live object. */
class CWinApp; class CWnd;
extern CWinApp* g_pBobApp;
CWinApp*   AfxGetApp()            { return g_pBobApp; }
CWnd*      g_pBobMainWnd = 0;     /* set to theApp.m_pMainWnd in MIG.CPP (Linux SDI bring-up) */
CWnd*      AfxGetMainWnd()        { return g_pBobMainWnd; }
HINSTANCE  AfxGetInstanceHandle() { return 0; }

/* ===== HTML Help (no help engine on Linux) =============================== */
/* MAINFRM.CPP's OnHelp() calls HtmlHelp() (-> HtmlHelpA, declared in H/HTMLHELP.H
   with no impl). It was dead-stripped before -fno-delete-null-pointer-checks kept
   the call alive. Return 0 (failure) -> the caller shows "failed to launch help". */
extern "C" HWND WINAPI HtmlHelpA(HWND, LPCSTR, UINT, DWORD) { return 0; }
extern "C" HWND WINAPI HtmlHelpW(HWND, LPCWSTR, UINT, DWORD) { return 0; }

/* ===== misc globals ===================================================== */
int BAD_RV = (int)0x80000000;
char *compiledate = (char *)__DATE__;

#endif /* FF_LINUX */
