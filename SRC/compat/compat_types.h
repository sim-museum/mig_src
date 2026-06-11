/*
 * FreeFalcon Linux Port - Windows base types
 *
 * Fixed-width mappings of the Win32 base types for 64-bit Linux.
 * IMPORTANT: DWORD/LONG/ULONG stay 32-bit (as on Windows) so that
 * binary file formats and struct layouts match the original game data.
 */

#ifndef FF_COMPAT_TYPES_H
#define FF_COMPAT_TYPES_H

#ifdef FF_LINUX

/* MSVC/COM keyword used by the DirectX interface headers (dmusicc.h, etc.) */
#ifndef interface
#define interface struct
#endif

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

/* ============================================================
 * Calling conventions - all empty on Linux (System V ABI)
 * ============================================================ */
#ifndef __cdecl
#define __cdecl
#endif
#ifndef __stdcall
#define __stdcall
#endif
#ifndef __fastcall
#define __fastcall
#endif
#ifndef CALLBACK
#define CALLBACK
#endif
#ifndef WINAPI
#define WINAPI
#ifndef WINBASEAPI
#define WINBASEAPI
#endif
#ifndef WINUSERAPI
#define WINUSERAPI
#endif
#ifndef WINGDIAPI
#define WINGDIAPI
#endif
#endif
#ifndef WINAPIV
#define WINAPIV
#endif
#ifndef APIENTRY
#define APIENTRY
#endif
#ifndef PASCAL
#define PASCAL
#endif
/* Win32 SAL-style parameter direction annotations (windef.h: empty macros) */
#ifndef IN
#define IN
#endif
#ifndef OUT
#define OUT
#endif
#ifndef OPTIONAL
#define OPTIONAL
#endif
/* winnt.h field offset macro */
#ifndef FIELD_OFFSET
#define FIELD_OFFSET(type, field) ((LONG)(intptr_t)&(((type *)0)->field))
#endif
/* RPC far-pointer annotations (rpcndr.h) — no-ops */
#ifndef __RPC_FAR
#define __RPC_FAR
#endif
#ifndef FAR
#define FAR
#endif
#ifndef NEAR
#define NEAR
#endif
#ifndef FAR
#define FAR
#endif
#ifndef NEAR
#define NEAR
#endif
#ifndef _cdecl
#define _cdecl
#endif
#ifndef _stdcall
#define _stdcall
#endif
#ifndef _fastcall
#define _fastcall
#endif
#ifndef _pascal
#define _pascal
#endif
#ifndef __forceinline
#define __forceinline inline
#endif
#ifndef __declspec
#define __declspec(x)
#endif
#ifndef _declspec
#define _declspec(x)
#endif

/* MSVC integer suffix types */
typedef int64_t  __int64;
typedef int32_t  __int32;
typedef int16_t  __int16;
typedef int8_t   __int8;

/* ============================================================
 * Base integer types (Windows sizes!)
 * ============================================================ */
typedef int                 BOOL;
typedef unsigned char       BOOLEAN;
typedef BOOL               *PBOOL, *LPBOOL;
typedef unsigned char       BYTE, *PBYTE, *LPBYTE;
typedef unsigned char       UCHAR, *PUCHAR;
typedef char                CHAR, *PCHAR;
typedef unsigned short      WORD, *PWORD, *LPWORD;
typedef short               SHORT, *PSHORT;
typedef unsigned short      USHORT, *PUSHORT;
typedef unsigned long       DWORD, *PDWORD, *LPDWORD;	/* Win32 ABI: DWORD is unsigned long (matches bob ULong) */
typedef long int            LONG, *PLONG, *LPLONG;	/* Win32: LONG is long (matches cstring.h) */
typedef unsigned long       ULONG, *PULONG;
typedef int                 INT, *PINT, *LPINT;
typedef unsigned int        UINT, *PUINT;
typedef float               FLOAT, *PFLOAT;
typedef double              DOUBLE;
typedef void               *PVOID, *LPVOID;
typedef const void         *LPCVOID;
typedef int64_t             LONGLONG;
typedef uint64_t            ULONGLONG, DWORDLONG;
typedef uint64_t            DWORD64, ULONG64;
typedef int64_t             LONG64, INT64;
typedef uint64_t            UINT64;
typedef uint32_t            UINT32;
typedef int32_t             INT32;
typedef int16_t             INT16;
typedef uint16_t            UINT16;
typedef int8_t              INT8;
typedef uint8_t             UINT8;
/* WCHAR maps to native wchar_t (32-bit) so wcslen/wcscpy/L"" work.
 * The game does not persist WCHAR in binary files. */
#include <wchar.h>
typedef wchar_t             WCHAR, *PWCHAR;

/* Pointer-sized integers */
typedef intptr_t            INT_PTR, *PINT_PTR;
typedef uintptr_t           UINT_PTR, *PUINT_PTR;
typedef intptr_t            LONG_PTR, *PLONG_PTR;
typedef uintptr_t           ULONG_PTR, *PULONG_PTR;
typedef uintptr_t           DWORD_PTR, *PDWORD_PTR;
typedef size_t              SIZE_T;
typedef ptrdiff_t           SSIZE_T;

/* String types */
typedef char               *PSTR, *LPSTR;
typedef const char         *PCSTR, *LPCSTR;
typedef WCHAR              *PWSTR, *LPWSTR;
typedef const WCHAR        *PCWSTR, *LPCWSTR;
typedef char                TCHAR, *PTCHAR, *PTSTR, *LPTSTR;
typedef const char         *PCTSTR, *LPCTSTR;

/* Message-loop types */
typedef UINT_PTR            WPARAM;
typedef LONG_PTR            LPARAM;
typedef LONG_PTR            LRESULT;

typedef int32_t             HRESULT;
typedef WORD                ATOM;
typedef DWORD               COLORREF, *LPCOLORREF;
typedef LONG               *LPLONG_T;

/* Identity macros: legacy headers guard their own typedefs with
 * "#ifndef ULONG" etc., which only works if these are macros. */
#define BYTE   BYTE
#define ULONG  ULONG
#define SHORT  SHORT
#define LONG   LONG
#define DWORD  DWORD
#define WORD   WORD
#define UINT   UINT
#define BOOL   BOOL
#define CHAR   CHAR
#define UCHAR  UCHAR
#define USHORT USHORT
#define FLOAT  FLOAT

#ifndef VOID
#define VOID void
#endif

#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#ifndef CONST
#define CONST const
#endif

/* ============================================================
 * Handle types - all opaque pointers
 * ============================================================ */
typedef void *HANDLE, **PHANDLE, **LPHANDLE;
#define DECLARE_HANDLE(name) typedef void *name
DECLARE_HANDLE(HWND);
DECLARE_HANDLE(HINSTANCE);
typedef HINSTANCE HMODULE;
DECLARE_HANDLE(HICON);
typedef HICON HCURSOR;
DECLARE_HANDLE(HBRUSH);
DECLARE_HANDLE(HMENU);
DECLARE_HANDLE(HACCEL);
DECLARE_HANDLE(HDC);
DECLARE_HANDLE(HBITMAP);
DECLARE_HANDLE(HFONT);
DECLARE_HANDLE(HPEN);
DECLARE_HANDLE(HRGN);
DECLARE_HANDLE(HPALETTE);
DECLARE_HANDLE(HGDIOBJ);
DECLARE_HANDLE(HKEY);
typedef HKEY *PHKEY;
DECLARE_HANDLE(HGLOBAL);
DECLARE_HANDLE(HLOCAL);
DECLARE_HANDLE(HHOOK);
DECLARE_HANDLE(HMONITOR);
DECLARE_HANDLE(HDROP);
DECLARE_HANDLE(HRSRC);
DECLARE_HANDLE(HMMIO);
DECLARE_HANDLE(HWAVEOUT);
DECLARE_HANDLE(HWAVEIN);

#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)

/* ============================================================
 * Geometry structs
 * ============================================================ */
typedef struct tagPOINT {
    LONG x;
    LONG y;
} POINT, *PPOINT, *LPPOINT;

typedef struct tagPOINTS {
    SHORT x;
    SHORT y;
} POINTS, *PPOINTS, *LPPOINTS;

typedef struct tagRECT {
    LONG left;
    LONG top;
    LONG right;
    LONG bottom;
} RECT, *PRECT, *LPRECT;
typedef const RECT *LPCRECT;

typedef struct tagSIZE {
    LONG cx;
    LONG cy;
} SIZE, *PSIZE, *LPSIZE;

/* ============================================================
 * Time / misc structs
 * ============================================================ */
typedef union _LARGE_INTEGER {
    struct {
        DWORD LowPart;
        LONG  HighPart;
    };
    struct {
        DWORD LowPart;
        LONG  HighPart;
    } u;
    LONGLONG QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER;

typedef union _ULARGE_INTEGER {
    struct {
        DWORD LowPart;
        DWORD HighPart;
    };
    ULONGLONG QuadPart;
} ULARGE_INTEGER, *PULARGE_INTEGER;

typedef struct _FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} FILETIME, *PFILETIME, *LPFILETIME;

typedef struct _SYSTEMTIME {
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
} SYSTEMTIME, *PSYSTEMTIME, *LPSYSTEMTIME;

/* ============================================================
 * GUID
 * ============================================================ */
#ifndef GUID_DEFINED
#define GUID_DEFINED
typedef struct _GUID {
    DWORD Data1;
    WORD  Data2;
    WORD  Data3;
    BYTE  Data4[8];
} GUID;
#endif

typedef GUID IID, CLSID, FMTID;
typedef GUID *LPGUID, *LPIID, *LPCLSID;
typedef const GUID *LPCGUID;

#ifdef __cplusplus
#define REFGUID  const GUID &
#define REFIID   const IID &
#define REFCLSID const CLSID &
#else
#define REFGUID  const GUID *
#define REFIID   const IID *
#define REFCLSID const CLSID *
#endif

/* ============================================================
 * OLE automation base types (wtypes.h / oaidl.h)
 * ============================================================ */
typedef DWORD       OLE_COLOR;
typedef unsigned short VARTYPE;
struct IDispatch;
typedef struct IDispatch *LPDISPATCH;
typedef short       VARIANT_BOOL;
typedef VARIANT_BOOL _VARIANT_BOOL;
typedef double      DATE;
typedef union tagCY { struct { unsigned long Lo; long Hi; } s; LONGLONG int64; } CY, CURRENCY;
typedef WCHAR      *BSTR;
typedef OLE_COLOR  *LPOLE_COLOR;
typedef long        SCODE;
typedef long        DISPID;
typedef long        OLE_HANDLE;
typedef long        OLE_XPOS_PIXELS;
typedef long        OLE_YPOS_PIXELS;
#ifndef VARIANT_TRUE
#define VARIANT_TRUE  ((VARIANT_BOOL)-1)
#define VARIANT_FALSE ((VARIANT_BOOL)0)
#endif

#ifndef _VARIANT_DEFINED
#define _VARIANT_DEFINED
typedef struct tagVARIANT {
    VARTYPE vt; WORD wReserved1, wReserved2, wReserved3;
    union {
        LONG     lVal;
        BYTE     bVal;
        short    iVal;
        float    fltVal;
        double   dblVal;
        VARIANT_BOOL boolVal;
        SCODE    scode;
        BSTR     bstrVal;
        void    *byref;
        struct IDispatch *pdispVal;
        struct IUnknown  *punkVal;
    };
} VARIANT, *LPVARIANT, VARIANTARG, *LPVARIANTARG;

typedef struct tagDISPPARAMS {
    VARIANTARG *rgvarg;
    DISPID     *rgdispidNamedArgs;
    UINT        cArgs;
    UINT        cNamedArgs;
} DISPPARAMS;

typedef struct tagEXCEPINFO {
    WORD  wCode, wReserved;
    BSTR  bstrSource, bstrDescription, bstrHelpFile;
    DWORD dwHelpContext;
    void *pvReserved, *pfnDeferredFillIn;
    SCODE scode;
} EXCEPINFO, *LPEXCEPINFO;
#endif

#ifndef _VARENUM_DEFINED
#define _VARENUM_DEFINED
enum VARENUM {
    VT_EMPTY = 0, VT_NULL = 1, VT_I2 = 2, VT_I4 = 3, VT_R4 = 4, VT_R8 = 5,
    VT_CY = 6, VT_DATE = 7, VT_BSTR = 8, VT_DISPATCH = 9, VT_ERROR = 10,
    VT_BOOL = 11, VT_VARIANT = 12, VT_UNKNOWN = 13, VT_DECIMAL = 14,
    VT_I1 = 16, VT_UI1 = 17, VT_UI2 = 18, VT_UI4 = 19, VT_I8 = 20, VT_UI8 = 21,
    VT_INT = 22, VT_UINT = 23, VT_VOID = 24, VT_HRESULT = 25, VT_PTR = 26,
    VT_SAFEARRAY = 27, VT_CARRAY = 28, VT_USERDEFINED = 29, VT_LPSTR = 30,
    VT_LPWSTR = 31, VT_BYREF = 0x4000
};
#endif

/* ============================================================
 * Common macros
 * ============================================================ */
#ifndef MAKEWORD
#define MAKEWORD(a, b)   ((WORD)(((BYTE)((DWORD_PTR)(a) & 0xff)) | ((WORD)((BYTE)((DWORD_PTR)(b) & 0xff))) << 8))
#endif
#ifndef MAKELONG
#define MAKELONG(a, b)   ((LONG)(((WORD)((DWORD_PTR)(a) & 0xffff)) | ((DWORD)((WORD)((DWORD_PTR)(b) & 0xffff))) << 16))
#endif
#ifndef LOWORD
#define LOWORD(l)        ((WORD)((DWORD_PTR)(l) & 0xffff))
#endif
#ifndef HIWORD
#define HIWORD(l)        ((WORD)((DWORD_PTR)(l) >> 16))
#endif
#ifndef LOBYTE
#define LOBYTE(w)        ((BYTE)((DWORD_PTR)(w) & 0xff))
#endif
#ifndef HIBYTE
#define HIBYTE(w)        ((BYTE)((DWORD_PTR)(w) >> 8))
#endif

/*
 * min/max macros: legacy game code relies on these, but they poison
 * C++ standard headers (std::numeric_limits<T>::min() etc.).
 * Pre-include the common std headers BEFORE defining the macros so their
 * include guards make later inclusions no-ops.
 */
#ifdef __cplusplus
/* PACK BOUNDARY (Phase 0): ALL std C++ types keep their native ABI despite
   -fpack-struct=1 -- libstdc++ is compiled with native layout and operates on
   these objects (streams/string/locale/containers), so a packed layout corrupts
   memory. #pragma pack(8) overrides -fpack-struct for the whole std block. */
#pragma pack(push,8)
#include <cmath>
#include <climits>
#include <limits>
#include <algorithm>
#include <utility>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <queue>
#include <deque>
#include <stack>
#include <memory>
#include <functional>
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <random>
#include <stdexcept>
#include <typeinfo>
#include <iterator>
#include <sstream>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <bitset>
#include <numeric>
#include <valarray>
#include <complex>
#include <regex>
#include <array>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#pragma pack(pop)
#endif

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#define ZeroMemory(dest, len)        memset((dest), 0, (len))
#define CopyMemory(dest, src, len)   memcpy((dest), (src), (len))
#define MoveMemory(dest, src, len)   memmove((dest), (src), (len))
#define FillMemory(dest, len, fill)  memset((dest), (fill), (len))

#define UNREFERENCED_PARAMETER(p) (void)(p)

/* ============================================================
 * MSVC CRT string function aliases
 * ============================================================ */
#include <strings.h>
#ifndef stricmp
#define stricmp    strcasecmp
#endif
#ifndef strcmpi
#define strcmpi    strcasecmp
#endif
#ifndef strnicmp
#define strnicmp   strncasecmp
#endif
#define _stricmp   strcasecmp
#define _strcmpi   strcasecmp
#define _strnicmp  strncasecmp
#define _snprintf  snprintf
#define _vsnprintf vsnprintf
#define _atoi64    atoll

#ifdef __cplusplus
extern "C" {
#endif
char *_strlwr(char *str);
char *_strupr(char *str);
char *_itoa(int value, char *str, int radix);
char *_ltoa(long value, char *str, int radix);
char *_ultoa(unsigned long value, char *str, int radix);
char *_gcvt(double value, int digits, char *buffer);
char *_fcvt(double value, int count, int *dec, int *sign);
char *_ecvt(double value, int count, int *dec, int *sign);
#ifdef __cplusplus
}
#endif
#define strlwr _strlwr
#define strupr _strupr
#define ff_max(a, b) (((a) > (b)) ? (a) : (b))
#define ff_min(a, b) (((a) < (b)) ? (a) : (b))

/* MSVC SSE alignment macro */
#ifndef _MM_ALIGN16
#define _MM_ALIGN16 __attribute__((aligned(16)))
#endif

/* MSVC math aliases */
#include <math.h>
#define _isnan(x)     isnan(x)
#define _finite(x)    isfinite(x)
#define _copysign(x, y) copysign((x), (y))
#define _hypot        hypot
#define _j0 j0
#define _j1 j1

/* MSVC ctype aliases */
#include <ctype.h>
#define _istlower(c) islower(c)
#define _istupper(c) isupper(c)
#define _istalpha(c) isalpha(c)
#define _istdigit(c) isdigit(c)
#define _istspace(c) isspace(c)

#ifndef _ASSERTE
#define _ASSERTE(x) ((void)0)
#endif

/* ============================================================
 * Global case-insensitive fopen redirect.
 * Game data comes from Windows where paths are case-insensitive;
 * redirect every fopen() to the resolving wrapper. Compilation
 * units that need the real fopen define FF_NO_FOPEN_REDIRECT.
 * ============================================================ */
#ifdef __cplusplus
extern "C" {
#endif
FILE *fopen_nocase(const char *filepath, const char *mode);
#ifdef __cplusplus
}
#endif
#ifndef FF_NO_FOPEN_REDIRECT
#define fopen fopen_nocase
#endif
#define itoa   _itoa
#define ltoa   _ltoa
#define ultoa  _ultoa

#ifndef _MAX_PATH
#define _MAX_PATH 260
#endif
#ifndef MAX_PATH
#define MAX_PATH 260
#endif
#ifndef _MAX_DIR
#define _MAX_DIR 256
#endif
#ifndef _MAX_FNAME
#define _MAX_FNAME 256
#endif
#ifndef _MAX_EXT
#define _MAX_EXT 256
#endif
#ifndef _MAX_DRIVE
#define _MAX_DRIVE 3
#endif

/* Text macro (ANSI build) */
#ifndef TEXT
#define TEXT(s) s
#endif
#ifndef _T
#define _T(s) s
#endif

/* MSVC integer-literal suffixes i64/ui64 -> user-defined literals. The non-'_'
   suffix is reserved (warning only under -w); lets `0i64`, `0x..i64` compile. */
#ifdef __cplusplus
constexpr long long          operator"" i64 (unsigned long long n) { return (long long)n; }
constexpr unsigned long long operator"" ui64(unsigned long long n) { return n; }
#endif

#endif /* FF_LINUX */
#endif /* FF_COMPAT_TYPES_H */
