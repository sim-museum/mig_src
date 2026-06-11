/* FreeFalcon Linux Port - dplay.h stub (Windows-only API) */
#ifndef FF_STUB_dplay_h
#define FF_STUB_dplay_h
#endif

// Linux/GCC port: DirectPlay status/flag constants used by the engine (COMMS).
// Multiplayer is stubbed; exact values matter only at runtime.
#ifndef DP_OK
#define DP_OK                       0
#define DPERR_BUFFERTOOSMALL        ((HRESULT)0x88770000L|1)
#define DPERR_NOMESSAGES            ((HRESULT)0x88770000L|2)
#define DPERR_PENDING               ((HRESULT)0x88770000L|3)
#define DPRECEIVE_ALL               0x00000001L
#define DPRECEIVE_TOPLAYER          0x00000002L
#define DPRECEIVE_FROMPLAYER        0x00000004L
#define DPRECEIVE_PEEK              0x00000008L
#define DPSEND_GUARANTEED           0x00000001L
#define DPSEND_ASYNC                0x00000200L
#define DPSEND_NOSENDCOMPLETEMSG    0x00000400L
#endif
