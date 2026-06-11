//==============================================================================
// windows.h  -- minimal Win32 compatibility shim for the Linux/SDL2 port of
//               Mig Alley.  32-bit (i386) target: sizeof(long)==sizeof(void*)==4.
//
// This is NOT the Microsoft SDK header.  It provides just enough of the Win32
// surface that the game's source compiles under gcc/clang.  It is grown
// compile-error-driven; see port/src/win32_compat.cpp for implementations.
//==============================================================================
#ifndef MA_PORT_WINDOWS_H
#define MA_PORT_WINDOWS_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

// The game's DOSDEFS.H imposes a global `#pragma pack(1)`.  Protect every type
// this header defines with natural (4-byte, i386) alignment so the shim's view
// of these structs is self-consistent and stable regardless of include order.
#pragma pack(push, 4)

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
// Calling-convention / linkage macros.  On the ELF target these are all no-ops
// (everything is the platform default cdecl).  Consistency is what matters --
// the shim implementations use the same (empty) conventions.
//------------------------------------------------------------------------------
#ifndef WINAPI
#define WINAPI
#endif
#ifndef APIENTRY
#define APIENTRY
#endif
#ifndef CALLBACK
#define CALLBACK
#endif
#ifndef WINAPIV
#define WINAPIV
#endif
#ifndef PASCAL
#define PASCAL
#endif
#ifndef WINGDIAPI
#define WINGDIAPI
#endif
#ifndef FAR
#define FAR
#endif
#ifndef NEAR
#define NEAR
#endif
#ifndef CONST
#define CONST const
#endif
#ifndef __stdcall
#define __stdcall
#endif
#ifndef __cdecl
#define __cdecl
#endif
#ifndef __fastcall
#define __fastcall
#endif
#ifndef _stdcall
#define _stdcall
#endif
#ifndef _cdecl
#define _cdecl
#endif
#ifndef IN
#define IN
#endif
#ifndef OUT
#define OUT
#endif
#ifndef OPTIONAL
#define OPTIONAL
#endif

#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef NULL
#define NULL 0
#endif

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

//------------------------------------------------------------------------------
// Fundamental types.  32-bit ABI: long/pointer are 4 bytes (matches Watcom).
//------------------------------------------------------------------------------
typedef void                 VOID;
typedef int                  BOOL;       // Win32 BOOL (NOT the game's Bool enum)
typedef unsigned char        BYTE;
typedef unsigned short       WORD;
typedef unsigned long        DWORD;      // 32-bit on i386
typedef unsigned int         UINT;
typedef int                  INT;
typedef char                 CHAR;
typedef signed char          SCHAR;
typedef unsigned char        UCHAR;
typedef short                SHORT;
typedef unsigned short       USHORT;
typedef long                 LONG;
typedef unsigned long        ULONG;
typedef float                FLOAT;
typedef int                  INT32;
typedef unsigned int         UINT32;
typedef long long            LONGLONG;
typedef unsigned long long   ULONGLONG;
typedef unsigned short       WCHAR;
typedef unsigned char        UBYTE;

typedef BYTE  *LPBYTE,  *PBYTE;
typedef WORD  *LPWORD,  *PWORD;
typedef DWORD *LPDWORD, *PDWORD;
typedef LONG  *LPLONG,  *PLONG;
typedef int   *LPINT,   *PINT;
typedef UINT  *LPUINT,  *PUINT;
typedef void  *LPVOID,  *PVOID;
typedef const void *LPCVOID;
typedef char  *LPSTR,   *PSTR,  *NPSTR;
typedef const char *LPCSTR, *PCSTR;
typedef char  *LPTSTR,  *PTSTR, *LPTCH;
typedef const char *LPCTSTR;
typedef CHAR  TCHAR;
typedef BOOL  *LPBOOL,  *PBOOL;
typedef float *LPFLOAT;

typedef unsigned int   UINT_PTR;
typedef int            INT_PTR;
typedef unsigned long  ULONG_PTR, *PULONG_PTR;
typedef long           LONG_PTR;
typedef ULONG_PTR      DWORD_PTR;
typedef ULONG_PTR      SIZE_T;
typedef LONG_PTR       SSIZE_T;

typedef UINT_PTR       WPARAM;
typedef LONG_PTR       LPARAM;
typedef LONG_PTR       LRESULT;
typedef DWORD          COLORREF, *LPCOLORREF;
typedef WORD           ATOM;
typedef int            HFILE;
typedef long           HRESULT;

//------------------------------------------------------------------------------
// Handles.  Opaque pointers.
//------------------------------------------------------------------------------
#define DECLARE_HANDLE(name) typedef void *name
typedef void *HANDLE, *PHANDLE, *LPHANDLE;
DECLARE_HANDLE(HWND);
DECLARE_HANDLE(HINSTANCE);
DECLARE_HANDLE(HMODULE);
DECLARE_HANDLE(HDC);
DECLARE_HANDLE(HGDIOBJ);
DECLARE_HANDLE(HBITMAP);
DECLARE_HANDLE(HPALETTE);
DECLARE_HANDLE(HMENU);
DECLARE_HANDLE(HICON);
DECLARE_HANDLE(HCURSOR);
DECLARE_HANDLE(HBRUSH);
DECLARE_HANDLE(HPEN);
DECLARE_HANDLE(HFONT);
DECLARE_HANDLE(HRGN);
DECLARE_HANDLE(HKEY);
DECLARE_HANDLE(HGLOBAL);
DECLARE_HANDLE(HLOCAL);
DECLARE_HANDLE(HACCEL);
DECLARE_HANDLE(HMETAFILE);
DECLARE_HANDLE(HENHMETAFILE);
DECLARE_HANDLE(HDROP);
typedef HICON HCURSOR_DUP;

//------------------------------------------------------------------------------
// GUID (DOSDEFS.H forward-declares struct _GUID / typedef GUID).
//------------------------------------------------------------------------------
#ifndef GUID_DEFINED
#define GUID_DEFINED
typedef struct _GUID {
    DWORD Data1;
    WORD  Data2;
    WORD  Data3;
    BYTE  Data4[8];
} GUID;
#endif
typedef GUID *LPGUID, *PGUID;
typedef const GUID *LPCGUID;
typedef GUID IID, CLSID, *LPIID, *LPCLSID;
typedef const GUID *REFGUID, *REFIID, *REFCLSID;

#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C extern
#endif
// DEFINE_GUID: declaration form (no INITGUID in this port); the GUID objects are
// defined in the DirectX shim implementation. Matches the MS SDK signature used
// by the vendored ddraw.h/dinput.h/dplay.h.
#ifndef DEFINE_GUID
#define DEFINE_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        EXTERN_C const GUID name
#endif
#ifndef DEFINE_PTR_TYPE
#define DEFINE_PTR_TYPE
#endif

//------------------------------------------------------------------------------
// Common structs.
//------------------------------------------------------------------------------
typedef struct tagPOINT { LONG x, y; } POINT, *PPOINT, *LPPOINT;
typedef struct tagPOINTS { SHORT x, y; } POINTS, *LPPOINTS;
typedef struct tagSIZE { LONG cx, cy; } SIZE, *PSIZE, *LPSIZE;
typedef struct tagRECT { LONG left, top, right, bottom; } RECT, *PRECT, *LPRECT;
typedef const RECT *LPCRECT;

typedef struct tagMSG {
    HWND   hwnd;
    UINT   message;
    WPARAM wParam;
    LPARAM lParam;
    DWORD  time;
    POINT  pt;
} MSG, *PMSG, *LPMSG;

typedef struct _RGNDATA { char dummy; } RGNDATA, *LPRGNDATA;

typedef struct tagPALETTEENTRY {
    BYTE peRed, peGreen, peBlue, peFlags;
} PALETTEENTRY, *LPPALETTEENTRY;

typedef struct tagLOGPALETTE {
    WORD         palVersion;
    WORD         palNumEntries;
    PALETTEENTRY palPalEntry[1];
} LOGPALETTE, *LPLOGPALETTE;

typedef struct _SYSTEMTIME {
    WORD wYear, wMonth, wDayOfWeek, wDay, wHour, wMinute, wSecond, wMilliseconds;
} SYSTEMTIME, *LPSYSTEMTIME;

typedef struct _FILETIME { DWORD dwLowDateTime, dwHighDateTime; } FILETIME, *LPFILETIME;

typedef struct _SECURITY_ATTRIBUTES {
    DWORD nLength; LPVOID lpSecurityDescriptor; BOOL bInheritHandle;
} SECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;

typedef union _LARGE_INTEGER {
    struct { DWORD LowPart; LONG HighPart; };
    LONGLONG QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER;

//------------------------------------------------------------------------------
// Window procedure / callback typedefs.
//------------------------------------------------------------------------------
typedef LRESULT (CALLBACK *WNDPROC)(HWND, UINT, WPARAM, LPARAM);
typedef BOOL    (CALLBACK *DLGPROC)(HWND, UINT, WPARAM, LPARAM);
typedef void    (CALLBACK *TIMERPROC)(HWND, UINT, UINT_PTR, DWORD);
typedef int     (CALLBACK *FARPROC)(void);
typedef int     (CALLBACK *PROC)(void);

typedef struct tagWNDCLASSA {
    UINT      style;
    WNDPROC   lpfnWndProc;
    int       cbClsExtra;
    int       cbWndExtra;
    HINSTANCE hInstance;
    HICON     hIcon;
    HCURSOR   hCursor;
    HBRUSH    hbrBackground;
    LPCSTR    lpszMenuName;
    LPCSTR    lpszClassName;
} WNDCLASSA, *LPWNDCLASSA, WNDCLASS, *LPWNDCLASS;

typedef struct tagPAINTSTRUCT {
    HDC  hdc;
    BOOL fErase;
    RECT rcPaint;
    BOOL fRestore;
    BOOL fIncUpdate;
    BYTE rgbReserved[32];
} PAINTSTRUCT, *LPPAINTSTRUCT;

//------------------------------------------------------------------------------
// HRESULT helpers.
//------------------------------------------------------------------------------
#define S_OK            ((HRESULT)0)
#define S_FALSE         ((HRESULT)1)
#define E_FAIL          ((HRESULT)0x80004005L)
#define E_NOINTERFACE   ((HRESULT)0x80004002L)
#define E_OUTOFMEMORY   ((HRESULT)0x8007000EL)
#define E_INVALIDARG    ((HRESULT)0x80070057L)
#define E_NOTIMPL       ((HRESULT)0x80004001L)
#define SUCCEEDED(hr)   (((HRESULT)(hr)) >= 0)
#define FAILED(hr)      (((HRESULT)(hr)) < 0)
#define MAKE_HRESULT(s,f,c) ((HRESULT)(((unsigned long)(s)<<31)|((unsigned long)(f)<<16)|((unsigned long)(c))))

//------------------------------------------------------------------------------
// A small, frequently-used slice of the Win32 API.  Implemented in
// port/src/win32_compat.cpp (or stubbed).  More are added as the build needs.
//------------------------------------------------------------------------------
DWORD  WINAPI GetTickCount(void);
void   WINAPI Sleep(DWORD ms);
DWORD  WINAPI GetLastError(void);
void   WINAPI SetLastError(DWORD);
HMODULE WINAPI GetModuleHandleA(LPCSTR);
DWORD  WINAPI GetModuleFileNameA(HMODULE, LPSTR, DWORD);

int    WINAPI MessageBoxA(HWND, LPCSTR text, LPCSTR caption, UINT type);
#define MB_OK 0
#define MB_ICONERROR 0x10
#define MB_ICONEXCLAMATION 0x30
#define MB_ICONINFORMATION 0x40
#define MB_YESNO 0x4
#define IDOK 1
#define IDYES 6
#define IDNO 7

void   WINAPI OutputDebugStringA(LPCSTR);

#ifdef __cplusplus
}
#endif

#pragma pack(pop)

// ANSI/Unicode name resolution (the game is ANSI-only).
#define WNDCLASSEX WNDCLASSA
#define GetModuleHandle  GetModuleHandleA
#define GetModuleFileName GetModuleFileNameA
#define MessageBox       MessageBoxA
#define OutputDebugString OutputDebugStringA

#endif // MA_PORT_WINDOWS_H
