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
#include <stdlib.h>

/* S118 (PO-12 phase 4): does this build offer a hardware driver at all?
 *
 * Until now the whole DX5/6 path was gated on MA_TRY_HARDWARE, which was right while it was being
 * scoped -- an unfinished renderer must not be reachable by a player. It draws the cockpit view to
 * parity with the software renderer now (S115-S117), so the driver is offered to the game's own
 * Preferences instead, and the CHOICE lives where the PO asked for it: Save_Data.fSoftware,
 * persisted in settings.mig.
 *
 * MA_NO_HARDWARE=1 withdraws the offer entirely -- the escape hatch if a machine's GL cannot cope,
 * and the switch the software-path gates use to pin their environment.
 */
extern "C" int ma_hardware_available(void)
{
    static int v = -1;
    if (v < 0) {
        const char* e = getenv("MA_NO_HARDWARE");
        /* value-sensitive on purpose: the gates set MA_NO_HARDWARE=1 to pin the software path and
           MA_NO_HARDWARE=0 to pin hardware, so a plain "is it set" test would make the second
           form mean the opposite of what it says. */
        v = (e && e[0] && e[0] != '0') ? 0 : 1;
    }
    return v;
}

extern "C" void* ma_d3d_device(void)
{
    static IDirect3DDevice s_dev;
    return (void*)&s_dev;
}
