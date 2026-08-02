/* Mig Alley — native Linux (SDL2) port: first-link stubs.
 *
 * Definitions for the last symbols that have no compilable game source on Linux:
 *   - the unassembled GRAPHICS asm primitives (XASM_* / ASM_PlotPixel), which the
 *     software-framebuffer path will replace later;
 *   - the Smacker video API (OpenSmack/CloseSmack/DoSmack/SmackSoundUseMSS/
 *     SmackVolumePan) — video is stubbed first (smack.h's `extern "C"` wrapper
 *     pulls a C++ system header => "template with C linkage", so WINSMACK.CPP
 *     can't compile as-is);
 *   - PostGameMessage — the window-message router, a no-op until the SDL runtime
 *     message queue exists.
 * These let `wmig` link; each gets a real implementation in the runtime phase. */
#ifdef FF_LINUX

#include <stdio.h>
#include <stdlib.h>
#include "windows.h"     /* Win32 compat: WPARAM/LPARAM */
#include "dosdefs.h"     /* base types: UWord */
#include "fileman.h"     /* FileNum (mangling must match the real enum type) */

struct Smack;            /* opaque — only ever used through Smack* here */

/* ---- Smacker video (stubbed; no playback for the first link) -------------- */
Smack* OpenSmack(FileNum, int, int, int, int, int) { return 0; }
void   CloseSmack()                                 {}
UWord  DoSmack(int)                                 { return 0; }
/* smack.h declares these inside an extern "C" block (Smacker SDK) */
extern "C" unsigned char SmackSoundUseMSS(void*)                     { return 0; }
extern "C" void          SmackVolumePan(void*, unsigned, unsigned, unsigned) {}

/* ---- window-message router (stubbed until the SDL runtime queue) ---------- */
void PostGameMessage(unsigned int, WPARAM, LPARAM) {}

/* ---- unassembled GRAPHICS asm primitives (GRAFPRIM.CPP externs) ----------- *
 * The flat-shade subset is now REAL — ported to SRC/GRAPHICS/ma_xasm.nasm:
 * XASM_SetColour/SetPixelWidth/GetTransparency/Get{Land,Horizon}FadeTable/HoriLineAddr,
 * the PlainHoriLine1/2 span fillers + dispatch tables, and the palette LUT primitives.
 * (Gouraud/textured fillers are no-op stubs in ma_xasm.nasm until ported.)            */
extern "C" short ASM_PlotPixel(long, long, long, short) { return 0; }

/* GetFileNum(name): filename->FileNum resolver used by the OCX controls' string-file
   setters (SetNormalFileNumString etc.).
   S64: no longer a stub. It resolves against the F_GRAFIX.G "FIL_* = 0xNNNN" table that
   ma_dlgtmpl.cpp already parses for the template artmap. Returning 0 meant every control
   whose art is named rather than numbered silently lost its artwork — the visible case
   being the Player Log's IDJ_TITLE title bar, which has no FIL_ entry in the template
   artmap because its art arrives via the persisted NormalFileNumString instead.
   This is the "resolve art by NAME" half of BoB's trap 2: the persisted numeric FileNums
   are authoring-install indices and are discarded, so the NAME is the only sound source.
   MA_TRACE_FILENUM reports each lookup. C++ linkage (matches `extern int GetFileNum`). */
extern "C" int ma_fil_lookup(const char* name);
int GetFileNum(const char* name) {
    int fn = ma_fil_lookup(name);
    if (getenv("MA_TRACE_FILENUM")) { static int n = 0; if (n++ < 40)
        fprintf(stderr, "[filenum] \"%s\" -> 0x%x\n", name ? name : "(null)", fn); }
    return fn;
}

#endif /* FF_LINUX */
