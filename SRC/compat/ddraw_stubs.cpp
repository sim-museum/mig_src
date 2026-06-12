/*
 * Mig Alley Linux port — DirectDraw legacy entry-point stubs.
 * ddraw.h declares DirectDrawCreate / DirectDrawEnumerateA but the compat layer
 * only implements DirectDrawCreateEx (bob_video.cpp). Mig Alley's hardware-init
 * path uses the DX1 DirectDrawCreate + enumeration. Compile-time/link stubs so the
 * game links and falls through to the software (SDL) renderer.
 */
#if defined(MA_LINUX) || defined(FF_LINUX)
#include "ddraw.h"

extern "C" {

HRESULT DirectDrawCreate(GUID * /*lpGUID*/, LPDIRECTDRAW *lplpDD, IUnknown * /*pUnkOuter*/)
{
    if (lplpDD) *lplpDD = new IDirectDraw();   /* compat stub object; methods return DD_OK */
    return DD_OK;
}

HRESULT DirectDrawEnumerateA(LPDDENUMCALLBACKA /*cb*/, LPVOID /*ctx*/)
{
    /* Enumerate no hardware devices -> game uses the primary/software path. */
    return DD_OK;
}

}
#endif
