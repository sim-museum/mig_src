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
static int resolve_nocase(const char *filepath, char *resolved, size_t resolvedSize) {
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

extern "C" FILE *fopen_nocase(const char *filepath, const char *mode) {
	if (!filepath || !mode) return NULL;
	static const int trace = getenv("BOB_TRACE_FOPEN") ? 1 : 0;
	char resolved[2048];
	if (resolve_nocase(filepath, resolved, sizeof(resolved)) == 0) {
		if (trace) fprintf(stderr, "[fopen] OK   [%s] (%s) -> %s\n", filepath, mode, resolved);
		struct stat st;
		if (stat(resolved, &st) == 0 && S_ISDIR(st.st_mode)) return NULL;
		return fopen(resolved, mode);
	}
	if (trace) fprintf(stderr, "[fopen] MISS [%s] (%s)\n", filepath, mode);
	if (strchr(mode, 'w') || strchr(mode, 'a')) return fopen(resolved, mode);
	return NULL;
}
extern "C" int open_nocase(const char *filepath, int flags, int mode) {
	if (!filepath) return -1;
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
	if (!ctx->dir) { delete ctx; return -1; }
	struct dirent *e;
	while ((e = readdir(ctx->dir)) != NULL) {
		if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
		if (fnmatch(ctx->pattern, e->d_name, FNM_CASEFOLD) == 0) { fill_finddata(ctx, e->d_name, fileinfo); return (intptr_t)ctx; }
	}
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
BOBGUID(GUID_XAxis);
BOBGUID(GUID_YAxis);
BOBGUID(GUID_Key);
BOBGUID(GUID_Button);
BOBGUID(GUID_POV);
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

/* ===== Win32 FindFirstFile (stub: no matches; runtime scanning uses fopen) === */
#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)
#endif
HANDLE FindFirstFileA(LPCSTR, LPWIN32_FIND_DATAA) { return INVALID_HANDLE_VALUE; }
BOOL   FindNextFileA(HANDLE, LPWIN32_FIND_DATAA) { return FALSE; }

/* DirectInput keyboard data format (consumed only by the stub DInput path) */
extern const DIDATAFORMAT c_dfDIKeyboard;
const DIDATAFORMAT c_dfDIKeyboard = {0};

/* ===== MFC app accessors ================================================= */
/* g_pBobApp is set to &theApp in MIG.CPP; AfxGetApp() returns the real app so
   framework code that reaches back through AfxGetApp() sees the live object. */
class CWinApp; class CWnd;
extern CWinApp* g_pBobApp;
CWinApp*   AfxGetApp()            { return g_pBobApp; }
CWnd*      AfxGetMainWnd()        { return 0; }
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
