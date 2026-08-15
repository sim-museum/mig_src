/* S111 (PO-12, hardware graphics phase 1): the process's single legacy Direct3D device.
 *
 * `direct_3d::CreateDevice` obtains the device by asking the DirectDraw BACK SURFACE for it --
 * `lpDDSBack->QueryInterface(Driver[n].Guid, &lpD3DDevice)` -- and a NULL there makes
 * `direct_3d::BeginScene` stop the game with "3D Hardware acceleration is not enabled". The
 * surface's QueryInterface lives in compat/ddraw_legacy.h, which is included from ddraw.h BEFORE
 * the legacy D3D types exist, so it calls this accessor instead of naming the type.
 *
 * The game creates exactly one device, so one static instance is the whole lifetime story.
 */
#include "windows.h"
#include "ddraw.h"
#include "d3d.h"

extern "C" void* ma_d3d_device(void)
{
    static IDirect3DDevice s_dev;
    return (void*)&s_dev;
}
