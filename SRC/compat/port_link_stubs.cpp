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

/* GetFileNum(name): filename->FileNum resolver used only by the OCX controls'
   string-file setters (NormalFileNumString etc.). The controls also accept numeric
   FileNums (NormalFileNum), which is the path the game uses; return FIL_NULL(0) here
   until the FileMan name index is wired. C++ linkage (matches `extern int GetFileNum`). */
int GetFileNum(const char* /*name*/) { return 0; }

#endif /* FF_LINUX */
