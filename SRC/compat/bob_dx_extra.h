// bob_dx_extra.h - Linux port: supplementary Win32 / DirectDraw / Direct3D
// declarations that the original headers had but the reused FreeFalcon compat
// layer didn't. Pulled in at the end of the compat windows.h umbrella.
#ifndef BOB_DX_EXTRA_H
#define BOB_DX_EXTRA_H
#ifdef FF_LINUX

#include "compat_types.h"
#include <cstdarg>     // va_start / va_end / va_list
#include <unistd.h>
#include <time.h>

// ---- Win32 DLL entry reasons -------------------------------------------------
#ifndef DLL_PROCESS_DETACH
#define DLL_PROCESS_DETACH 0
#define DLL_PROCESS_ATTACH 1
#define DLL_THREAD_ATTACH  2
#define DLL_THREAD_DETACH  3
#endif

#ifndef NOERROR
#define NOERROR 0
#endif

// ---- ChangeDisplaySettings / EnumDisplaySettings + DEVMODE -------------------
#ifndef DM_PELSWIDTH
#define DM_PELSWIDTH   0x00080000L
#define DM_PELSHEIGHT  0x00100000L
#endif
#ifndef DM_BITSPERPEL
#define DM_BITSPERPEL       0x00040000L
#define DM_DISPLAYFLAGS     0x00200000L
#define DM_DISPLAYFREQUENCY 0x00400000L
#endif
#ifndef CDS_FULLSCREEN
#define CDS_FULLSCREEN 0x00000004
#endif
#ifndef CDS_TEST
#define CDS_TEST       0x00000002
#endif
#ifndef ENUM_CURRENT_SETTINGS
#define ENUM_CURRENT_SETTINGS  ((DWORD)-1)
#define ENUM_REGISTRY_SETTINGS ((DWORD)-2)
#endif
#ifndef DISP_CHANGE_SUCCESSFUL
#define DISP_CHANGE_SUCCESSFUL 0
#define DISP_CHANGE_RESTART    1
#define DISP_CHANGE_FAILED    (-1)
#endif
#ifndef BOB_HAVE_DEVMODE
#define BOB_HAVE_DEVMODE
typedef struct _devicemodeA {
    char  dmDeviceName[32];
    WORD  dmSpecVersion, dmDriverVersion, dmSize, dmDriverExtra;
    DWORD dmFields;
    DWORD dmPositionX, dmPositionY, dmDisplayOrientation, dmDisplayFixedOutput;
    short dmColor, dmDuplex, dmYResolution, dmTTOption, dmCollate;
    char  dmFormName[32];
    WORD  dmLogPixels;
    DWORD dmBitsPerPel, dmPelsWidth, dmPelsHeight, dmDisplayFlags;
    DWORD dmDisplayFrequency;
} DEVMODE, *LPDEVMODE;
#endif
static inline LONG ChangeDisplaySettings(LPDEVMODE, DWORD) { return 0; /*DISP_CHANGE_SUCCESSFUL*/ }
static inline BOOL EnumDisplaySettings(const char*, DWORD, LPDEVMODE) { return FALSE; }

// ---- Shell AppBar API (TwoDPref auto-hide taskbar probing) ------------------
#ifndef ABM_GETSTATE
#define ABM_NEW            0x00000000
#define ABM_REMOVE         0x00000001
#define ABM_QUERYPOS       0x00000002
#define ABM_SETPOS         0x00000003
#define ABM_GETSTATE       0x00000004
#define ABM_GETTASKBARPOS  0x00000005
#define ABM_ACTIVATE       0x00000006
#define ABM_GETAUTOHIDEBAR 0x00000007
#define ABM_SETAUTOHIDEBAR 0x00000008
#define ABE_LEFT           0
#define ABE_TOP            1
#define ABE_RIGHT          2
#define ABE_BOTTOM         3
#define ABS_AUTOHIDE       0x0000001
#define ABS_ALWAYSONTOP    0x0000002
typedef struct _AppBarData {
    DWORD  cbSize;
    HWND   hWnd;
    UINT   uCallbackMessage;
    UINT   uEdge;
    RECT   rc;
    LPARAM lParam;
} APPBARDATA, *PAPPBARDATA;
static inline UINT SHAppBarMessage(DWORD, PAPPBARDATA) { return 0; }
#endif

// ---- VerQueryValue-style version info pulled from a resource ----------------
#ifndef VS_VERSION_INFO
#define VS_VERSION_INFO 1
#define RT_VERSION      ((const char*)16)
typedef struct tagVS_FIXEDFILEINFO {
    DWORD dwSignature, dwStrucVersion;
    DWORD dwFileVersionMS, dwFileVersionLS;
    DWORD dwProductVersionMS, dwProductVersionLS;
    DWORD dwFileFlagsMask, dwFileFlags, dwFileOS, dwFileType, dwFileSubtype;
    DWORD dwFileDateMS, dwFileDateLS;
} VS_FIXEDFILEINFO;
#endif
#ifndef BOB_HAVE_RESOURCE_FNS
#define BOB_HAVE_RESOURCE_FNS
static inline HRSRC   FindResource(HMODULE, const char*, const char*) { return (HRSRC)0; }
static inline HGLOBAL LoadResource(HMODULE, HRSRC) { return (HGLOBAL)0; }
static inline DWORD   GlobalSize(HGLOBAL) { return 0; }
static inline void*   LockResource(HGLOBAL) { return (void*)0; }
#endif

// ---- misc Win32 funcs missing from the compat layer -------------------------
// (Sleep / QueryPerformanceCounter / timeGetTime / GetWindowRect are already in
//  compat_winbase.h, so they are NOT redefined here.)
static inline int  StretchDIBits(HDC,int,int,int,int,int,int,int,int,const void*,const void*,UINT,DWORD) { return 0; }
static inline char* _i64toa(long long v, char* s, int radix) {
    if (radix==16) sprintf(s, "%llx", (unsigned long long)v); else sprintf(s, "%lld", v); return s;
}

// ---- DirectDraw extras -------------------------------------------------------
#ifndef DDERR_NODRIVERSUPPORT
#define DDERR_NODRIVERSUPPORT 0x80004001L
#endif
#ifndef DDERR_NOTLOADED
#define DDERR_NOTLOADED 0x80004002L
#endif
#ifndef DDENUM_NONDISPLAYDEVICES
#define DDENUM_NONDISPLAYDEVICES        0x00000004L
#define DDENUM_DETACHEDSECONDARYDEVICES 0x00000002L
#endif
#ifndef DDCAPS2_NOPAGELOCKREQUIRED
#define DDCAPS2_NOPAGELOCKREQUIRED 0x00000800L
#endif
#ifndef BOB_HAVE_DDGAMMARAMP
#define BOB_HAVE_DDGAMMARAMP
typedef struct _DDGAMMARAMP { WORD red[256], green[256], blue[256]; } DDGAMMARAMP, *LPDDGAMMARAMP;
struct IDirectDrawGammaControl {
    virtual long QueryInterface(const GUID&, void**) { return 0; }
    virtual unsigned long AddRef()  { return 1; }
    virtual unsigned long Release() { return 0; }
    virtual long GetGammaRamp(DWORD, LPDDGAMMARAMP) { return 0; }
    virtual long SetGammaRamp(DWORD, LPDDGAMMARAMP) { return 0; }
};
typedef struct IDirectDrawGammaControl *LPDIRECTDRAWGAMMACONTROL;
#endif

// ---- Direct3D extras ---------------------------------------------------------
#ifndef D3DFVF_TEXCOORDSIZE2
#define D3DFVF_TEXCOORDSIZE2(n) (0x00000000)
#endif
// D3DERR_* codes the error-string table references. On Linux these are never
// returned by a real device; they just need to exist and be distinct.
#ifndef D3DERR_INBEGIN
#define D3DERR_BASE 0x88760000L
#define D3DERR_INBEGIN                     (D3DERR_BASE+142)
#define D3DERR_NOTINBEGIN                  (D3DERR_BASE+143)
#define D3DERR_NOVIEWPORTS                 (D3DERR_BASE+144)
#define D3DERR_VIEWPORTHASNODEVICE         (D3DERR_BASE+145)
#define D3DERR_VIEWPORTDATANOTSET          (D3DERR_BASE+146)
#define D3DERR_NOCURRENTVIEWPORT           (D3DERR_BASE+147)
#define D3DERR_INVALIDPALETTE              (D3DERR_BASE+148)
#define D3DERR_SURFACENOTINVIDMEM          (D3DERR_BASE+149)
#define D3DERR_ZBUFF_NEEDS_SYSTEMMEMORY    (D3DERR_BASE+150)
#define D3DERR_ZBUFF_NEEDS_VIDEOMEMORY     (D3DERR_BASE+151)
#define D3DERR_CONFLICTINGRENDERSTATE      (D3DERR_BASE+152)
#define D3DERR_TOOMANYPRIMITIVES           (D3DERR_BASE+153)
#define D3DERR_INVALIDMATRIX               (D3DERR_BASE+154)
#define D3DERR_INVALIDRAMPTEXTURE          (D3DERR_BASE+155)
#define D3DERR_LIGHTHASVIEWPORT            (D3DERR_BASE+156)
#define D3DERR_LIGHTNOTINTHISVIEWPORT      (D3DERR_BASE+157)
#define D3DERR_TEXTURE_BADSIZE             (D3DERR_BASE+158)
#define D3DERR_INVALIDSTATEBLOCK           (D3DERR_BASE+159)
#define D3DERR_INBEGINSTATEBLOCK           (D3DERR_BASE+160)
#define D3DERR_NOTINBEGINSTATEBLOCK        (D3DERR_BASE+161)
#define D3DERR_VERTEXBUFFEROPTIMIZED       (D3DERR_BASE+162)
#define D3DERR_VERTEXBUFFERUNLOCKFAILED    (D3DERR_BASE+163)
#define D3DERR_COLORKEYATTACHED            (D3DERR_BASE+164)
#define D3DERR_DEVICEAGGREGATED            (D3DERR_BASE+165)
#endif

// ---- well-known IIDs (dummy GUIDs are fine; not used to talk to a real COM) --
#ifndef BOB_HAVE_IID_IUNKNOWN
#define BOB_HAVE_IID_IUNKNOWN
static const GUID IID_IUnknown =
  {0x00000000,0x0000,0x0000,{0xC0,0,0,0,0,0,0,0x46}};
static const GUID IID_IDirectDrawGammaControl =
  {0x69C11C3E,0xB46B,0x11D1,{0xAD,0x7A,0,0xC0,0x4F,0xC2,0x9B,0x4E}};
#endif

#endif /* FF_LINUX */
#endif /* BOB_DX_EXTRA_H */
