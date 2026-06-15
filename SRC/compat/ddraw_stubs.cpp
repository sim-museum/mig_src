/*
 * Mig Alley Linux port — DirectDraw legacy entry-point stubs.
 * ddraw.h declares DirectDrawCreate / DirectDrawEnumerateA but the compat layer
 * only implements DirectDrawCreateEx (bob_video.cpp). Mig Alley's hardware-init
 * path uses the DX1 DirectDrawCreate + enumeration. Compile-time/link stubs so the
 * game links and falls through to the software (SDL) renderer.
 */
#if defined(MA_LINUX) || defined(FF_LINUX)
#include "ddraw.h"

extern "C" void ma_ddraw_ensure_window(int w, int h);   /* bob_video.cpp: create the SDL2 window */

/* Linux/GCC port: display mode the primary surface inherits (set via SetDisplayMode). */
int ma_dd_dispW = 640, ma_dd_dispH = 480, ma_dd_dispBpp = 8;

/* QueryInterface(IID_IDirectDraw2) on the DX1 object yields a real DX2 object. */
extern "C" void* ma_dd_query_dd2(void) { return (void*)new IDirectDraw2(); }

extern "C" {

HRESULT DirectDrawCreate(GUID * /*lpGUID*/, LPDIRECTDRAW *lplpDD, IUnknown * /*pUnkOuter*/)
{
    if (lplpDD) *lplpDD = new IDirectDraw();   /* compat stub object; methods return DD_OK */
    /* Linux/GCC port: bring up the SDL2 window as soon as the game inits DirectDraw,
       so the present bridge (Flip/Blt -> ma_ddraw_present) has a window to draw into. */
    ma_ddraw_ensure_window(640, 480);
    return DD_OK;
}

HRESULT DirectDrawEnumerateA(LPDDENUMCALLBACKA /*cb*/, LPVOID /*ctx*/)
{
    /* Enumerate no hardware devices -> game uses the primary/software path. */
    return DD_OK;
}

}
#endif
