//==============================================================================
// windowsx.h -- minimal shim.  The MS header is a grab-bag of message-cracker
// and GDI convenience macros; the game uses only a handful.  Grown as needed.
//==============================================================================
#ifndef MA_PORT_WINDOWSX_H
#define MA_PORT_WINDOWSX_H

#include <windows.h>

// GlobalAlloc/Free flavour helpers occasionally used via windowsx.
#define GlobalAllocPtr(flags,cb)   (malloc((size_t)(cb)))
#define GlobalFreePtr(p)           (free((void*)(p)), 0)
#define GlobalReAllocPtr(p,cb,fl)  (realloc((void*)(p),(size_t)(cb)))

// Message-cracker positional helpers (LOWORD/HIWORD live in windows.h-land).
#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp) ((int)(short)((unsigned)(lp) & 0xffff))
#define GET_Y_LPARAM(lp) ((int)(short)(((unsigned)(lp) >> 16) & 0xffff))
#endif

#endif // MA_PORT_WINDOWSX_H
