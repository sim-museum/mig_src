/* FreeFalcon Linux Port - dplobby.h: minimal IDirectPlayLobby3 stub.
   The real DirectPlay lobby is Windows-only; multiplayer is deferred. The lobby
   object is never created at runtime (CoCreateInstance returns a failure stub),
   so these methods exist only to let the COMMS code compile. */
#ifndef FF_STUB_dplobby_h
#define FF_STUB_dplobby_h
#ifdef FF_LINUX

#include "compat_types.h"
#include "objbase.h"

struct IDirectPlayLobby3 {
#ifdef __cplusplus
    HRESULT QueryInterface(REFIID, void**)              { return E_NOINTERFACE; }
    ULONG   AddRef()                                    { return 1; }
    ULONG   Release()                                   { return 0; }
    HRESULT GetConnectionSettings(DWORD, LPVOID, LPDWORD){ return E_FAIL; }
    HRESULT SetConnectionSettings(DWORD, DWORD, LPVOID) { return E_FAIL; }
    HRESULT ConnectEx(DWORD, REFIID, LPVOID*, IUnknown*){ return E_FAIL; }
#endif
};
typedef struct IDirectPlayLobby3 *LPDIRECTPLAYLOBBY3, *LPDIRECTPLAYLOBBY3A;
typedef struct IDirectPlayLobby3  IDirectPlayLobby3A;

static const GUID CLSID_DirectPlayLobby =
    { 0x2fe8f810, 0xb2a5, 0x11d0, { 0xa7, 0x87, 0x0, 0x0, 0xf8, 0x3, 0xab, 0xfc } };
static const GUID IID_IDirectPlayLobby3A =
    { 0x2db72491, 0x652c, 0x11d1, { 0xa7, 0xa8, 0x0, 0x0, 0xf8, 0x3, 0xab, 0xfc } };

#endif /* FF_LINUX */
#endif
