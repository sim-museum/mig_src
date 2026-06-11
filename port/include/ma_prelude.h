//==============================================================================
// ma_prelude.h -- force-included (gcc -include) before every translation unit.
// Establishes the compiler-compat environment for the Linux/GCC 32-bit port so
// the original Watcom/MSVC source dialect parses.  Keep this minimal and self
// contained; it must precede even DOSDEFS.H.
//==============================================================================
#ifndef MA_PRELUDE_H
#define MA_PRELUDE_H

// --- System headers pulled in FIRST, at default packing ----------------------
// DOSDEFS.H sets a global `#pragma pack(1)`.  By including the libc headers we
// rely on here (force-included before the TU's own first line, hence before
// DOSDEFS), their structs keep natural alignment / ABI compatibility with libc.
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// --- Calling-convention / linkage keywords -> no-ops on the ELF target -------
#ifndef __cdecl
#define __cdecl
#endif
#ifndef __stdcall
#define __stdcall
#endif
#ifndef __fastcall
#define __fastcall
#endif
#ifndef __pascal
#define __pascal
#endif
#ifndef _pascal
#define _pascal
#endif
#ifndef pascal
#define pascal
#endif
#ifndef __export
#define __export
#endif
#ifndef _export
#define _export
#endif
#ifndef __loadds
#define __loadds
#endif
#ifndef __huge
#define __huge
#endif
#ifndef huge
#define huge
#endif
#ifndef cdecl
#define cdecl
#endif
#ifndef __declspec
#define __declspec(x)
#endif
#ifndef _declspec
#define _declspec(x)
#endif

// --- COM/GUID scaffolding needed by the vendored ddraw/dinput/dplay headers ---
// These DX headers use DEFINE_GUID() at file scope and may be included before
// windows.h, so the macro must exist globally up front.
// The game targets Win32 (Windows 95) and the shim provides that API surface;
// declare the platform so headers like MSS.H / the Miles wrappers detect it.
#ifndef WIN32
#define WIN32 1
#endif

struct _GUID;
typedef struct _GUID GUID;
#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C extern
#endif
#ifndef DEFINE_GUID
#define DEFINE_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        EXTERN_C const GUID name
#endif

// --- x87 FPU control-word access (Watcom GETFPCW/SETFPCW intrinsics) ----------
#if defined(__GNUC__) && defined(__cplusplus)
static inline unsigned short GETFPCW(void)
{ unsigned short cw; __asm__ __volatile__("fnstcw %0" : "=m"(cw)); return cw; }
static inline void SETFPCW(unsigned short cw)
{ __asm__ __volatile__("fldcw %0" : : "m"(cw)); }
#endif

#endif // MA_PRELUDE_H
