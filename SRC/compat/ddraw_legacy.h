/*
 * Mig Alley Linux port — legacy DirectDraw DX1/DX2 interface bodies.
 *
 * The DX7 compat (FreeFalcon/BoB) only gives IDirectDrawSurface7 a real body;
 * the DX1 IDirectDrawSurface and DX2 IDirectDraw2/IDirectDrawSurface2 are
 * forward-only "opaque" stubs because FF never called their methods. Mig Alley's
 * HARDWARE module drives the DX1/DX2 surface API directly (IsLost/Lock/Blt/Flip/
 * GetAttachedSurface/...), so those interfaces need member functions.
 *
 * Compile-time stubs (return DD_OK). The real DirectDraw runtime is SDL-backed in
 * the GRAPHICS layer; wire these up when the hardware path goes live.
 *
 * Included from the end of compat/ddraw.h.
 */
#ifndef MA_COMPAT_DDRAW_LEGACY_H
#define MA_COMPAT_DDRAW_LEGACY_H

#if (defined(FF_LINUX) || defined(MA_LINUX)) && defined(__cplusplus)

#ifndef DD_OK
#define DD_OK S_OK
#endif

#ifndef DDCAPS_CANBLTSYSMEM
#define DDCAPS_CANBLTSYSMEM 0x80000000L
#endif

typedef HRESULT (WINAPI *LPDDENUMMODESCALLBACK)(LPDDSURFACEDESC, LPVOID);

struct IDirectDrawSurface2;
typedef struct IDirectDrawSurface2 *LPDIRECTDRAWSURFACE2;

/* ---- DX1 IDirectDrawSurface ---- */
struct IDirectDrawSurface {
    void *lpVtbl;
    virtual ~IDirectDrawSurface() {}
    HRESULT QueryInterface(REFIID, void** p)          { if(p)*p=0; return DD_OK; }
    ULONG   AddRef()                                  { return 1; }
    ULONG   Release()                                 { return 0; }
    HRESULT AddAttachedSurface(LPDIRECTDRAWSURFACE)   { return DD_OK; }
    HRESULT AddOverlayDirtyRect(LPRECT)               { return DD_OK; }
    HRESULT Blt(LPRECT, LPDIRECTDRAWSURFACE, LPRECT, DWORD, LPVOID) { return DD_OK; }
    HRESULT BltBatch(LPVOID, DWORD, DWORD)            { return DD_OK; }
    HRESULT BltFast(DWORD, DWORD, LPDIRECTDRAWSURFACE, LPRECT, DWORD) { return DD_OK; }
    HRESULT DeleteAttachedSurface(DWORD, LPDIRECTDRAWSURFACE) { return DD_OK; }
    HRESULT EnumAttachedSurfaces(LPVOID, LPVOID)      { return DD_OK; }
    HRESULT EnumOverlayZOrders(DWORD, LPVOID, LPVOID) { return DD_OK; }
    HRESULT Flip(LPDIRECTDRAWSURFACE, DWORD)          { return DD_OK; }
    HRESULT GetAttachedSurface(LPDDSCAPS, LPDIRECTDRAWSURFACE* s) { if(s)*s=0; return DD_OK; }
    HRESULT GetBltStatus(DWORD)                       { return DD_OK; }
    HRESULT GetCaps(LPDDSCAPS)                        { return DD_OK; }
    HRESULT GetClipper(LPDIRECTDRAWCLIPPER*)          { return DD_OK; }
    HRESULT GetColorKey(DWORD, LPDDCOLORKEY)          { return DD_OK; }
    HRESULT GetDC(HDC*)                               { return DD_OK; }
    HRESULT GetFlipStatus(DWORD)                      { return DD_OK; }
    HRESULT GetOverlayPosition(LPLONG, LPLONG)        { return DD_OK; }
    HRESULT GetPalette(LPDIRECTDRAWPALETTE*)          { return DD_OK; }
    HRESULT GetPixelFormat(LPDDPIXELFORMAT)           { return DD_OK; }
    HRESULT GetSurfaceDesc(LPDDSURFACEDESC)           { return DD_OK; }
    HRESULT Initialize(LPDIRECTDRAW, LPDDSURFACEDESC) { return DD_OK; }
    HRESULT IsLost()                                  { return DD_OK; }
    HRESULT Lock(LPRECT, LPDDSURFACEDESC, DWORD, HANDLE) { return DD_OK; }
    HRESULT ReleaseDC(HDC)                            { return DD_OK; }
    HRESULT Restore()                                 { return DD_OK; }
    HRESULT SetClipper(LPDIRECTDRAWCLIPPER)           { return DD_OK; }
    HRESULT SetColorKey(DWORD, LPDDCOLORKEY)          { return DD_OK; }
    HRESULT SetOverlayPosition(LONG, LONG)            { return DD_OK; }
    HRESULT SetPalette(LPDIRECTDRAWPALETTE)           { return DD_OK; }
    HRESULT Unlock(LPVOID)                            { return DD_OK; }
    HRESULT UpdateOverlay(LPRECT, LPDIRECTDRAWSURFACE, LPRECT, DWORD, LPVOID) { return DD_OK; }
    HRESULT UpdateOverlayDisplay(DWORD)               { return DD_OK; }
    HRESULT UpdateOverlayZOrder(DWORD, LPDIRECTDRAWSURFACE) { return DD_OK; }
};

/* ---- DX2 IDirectDrawSurface2 (adds PageLock/PageUnlock/GetDDInterface) ---- */
struct IDirectDrawSurface2 {
    void *lpVtbl;
    virtual ~IDirectDrawSurface2() {}
    HRESULT QueryInterface(REFIID, void** p)          { if(p)*p=0; return DD_OK; }
    ULONG   AddRef()                                  { return 1; }
    ULONG   Release()                                 { return 0; }
    HRESULT AddAttachedSurface(LPDIRECTDRAWSURFACE2)  { return DD_OK; }
    HRESULT AddOverlayDirtyRect(LPRECT)               { return DD_OK; }
    HRESULT Blt(LPRECT, LPDIRECTDRAWSURFACE2, LPRECT, DWORD, LPVOID) { return DD_OK; }
    HRESULT BltBatch(LPVOID, DWORD, DWORD)            { return DD_OK; }
    HRESULT BltFast(DWORD, DWORD, LPDIRECTDRAWSURFACE2, LPRECT, DWORD) { return DD_OK; }
    HRESULT DeleteAttachedSurface(DWORD, LPDIRECTDRAWSURFACE2) { return DD_OK; }
    HRESULT EnumAttachedSurfaces(LPVOID, LPVOID)      { return DD_OK; }
    HRESULT EnumOverlayZOrders(DWORD, LPVOID, LPVOID) { return DD_OK; }
    HRESULT Flip(LPDIRECTDRAWSURFACE2, DWORD)         { return DD_OK; }
    HRESULT GetAttachedSurface(LPDDSCAPS, LPDIRECTDRAWSURFACE2* s) { if(s)*s=0; return DD_OK; }
    HRESULT GetBltStatus(DWORD)                       { return DD_OK; }
    HRESULT GetCaps(LPDDSCAPS)                        { return DD_OK; }
    HRESULT GetClipper(LPDIRECTDRAWCLIPPER*)          { return DD_OK; }
    HRESULT GetColorKey(DWORD, LPDDCOLORKEY)          { return DD_OK; }
    HRESULT GetDC(HDC*)                               { return DD_OK; }
    HRESULT GetFlipStatus(DWORD)                      { return DD_OK; }
    HRESULT GetOverlayPosition(LPLONG, LPLONG)        { return DD_OK; }
    HRESULT GetPalette(LPDIRECTDRAWPALETTE*)          { return DD_OK; }
    HRESULT GetPixelFormat(LPDDPIXELFORMAT)           { return DD_OK; }
    HRESULT GetSurfaceDesc(LPDDSURFACEDESC)           { return DD_OK; }
    HRESULT Initialize(LPDIRECTDRAW, LPDDSURFACEDESC) { return DD_OK; }
    HRESULT IsLost()                                  { return DD_OK; }
    HRESULT Lock(LPRECT, LPDDSURFACEDESC, DWORD, HANDLE) { return DD_OK; }
    HRESULT ReleaseDC(HDC)                            { return DD_OK; }
    HRESULT Restore()                                 { return DD_OK; }
    HRESULT SetClipper(LPDIRECTDRAWCLIPPER)           { return DD_OK; }
    HRESULT SetColorKey(DWORD, LPDDCOLORKEY)          { return DD_OK; }
    HRESULT SetOverlayPosition(LONG, LONG)            { return DD_OK; }
    HRESULT SetPalette(LPDIRECTDRAWPALETTE)           { return DD_OK; }
    HRESULT Unlock(LPVOID)                            { return DD_OK; }
    HRESULT UpdateOverlay(LPRECT, LPDIRECTDRAWSURFACE2, LPRECT, DWORD, LPVOID) { return DD_OK; }
    HRESULT UpdateOverlayDisplay(DWORD)               { return DD_OK; }
    HRESULT UpdateOverlayZOrder(DWORD, LPDIRECTDRAWSURFACE2) { return DD_OK; }
    HRESULT GetDDInterface(LPVOID*)                   { return DD_OK; }
    HRESULT PageLock(DWORD)                           { return DD_OK; }
    HRESULT PageUnlock(DWORD)                         { return DD_OK; }
};

/* ---- DX2 IDirectDraw2 ---- */
struct IDirectDraw2 {
    void *lpVtbl;
    virtual ~IDirectDraw2() {}
    HRESULT QueryInterface(REFIID, void** p)          { if(p)*p=0; return DD_OK; }
    ULONG   AddRef()                                  { return 1; }
    ULONG   Release()                                 { return 0; }
    HRESULT Compact()                                 { return DD_OK; }
    HRESULT CreateClipper(DWORD, LPDIRECTDRAWCLIPPER*, IUnknown*) { return DD_OK; }
    HRESULT CreatePalette(DWORD, LPPALETTEENTRY, LPDIRECTDRAWPALETTE* p, IUnknown*) { if(p)*p=0; return DD_OK; }
    HRESULT CreateSurface(LPDDSURFACEDESC, LPDIRECTDRAWSURFACE* s, IUnknown*) { if(s)*s=0; return DD_OK; }
    HRESULT DuplicateSurface(LPDIRECTDRAWSURFACE, LPDIRECTDRAWSURFACE*) { return DD_OK; }
    HRESULT EnumDisplayModes(DWORD, LPDDSURFACEDESC, LPVOID, LPVOID) { return DD_OK; }
    HRESULT EnumSurfaces(DWORD, LPDDSURFACEDESC, LPVOID, LPVOID) { return DD_OK; }
    HRESULT FlipToGDISurface()                        { return DD_OK; }
    HRESULT GetCaps(LPDDCAPS, LPDDCAPS)               { return DD_OK; }
    HRESULT GetDisplayMode(LPDDSURFACEDESC)           { return DD_OK; }
    HRESULT GetFourCCCodes(LPDWORD, LPDWORD)          { return DD_OK; }
    HRESULT GetGDISurface(LPDIRECTDRAWSURFACE*)       { return DD_OK; }
    HRESULT GetMonitorFrequency(LPDWORD)              { return DD_OK; }
    HRESULT GetScanLine(LPDWORD)                      { return DD_OK; }
    HRESULT GetVerticalBlankStatus(LPBOOL)            { return DD_OK; }
    HRESULT Initialize(GUID*)                         { return DD_OK; }
    HRESULT RestoreDisplayMode()                      { return DD_OK; }
    HRESULT SetCooperativeLevel(HWND, DWORD)          { return DD_OK; }
    HRESULT SetDisplayMode(DWORD, DWORD, DWORD, DWORD, DWORD) { return DD_OK; }
    HRESULT WaitForVerticalBlank(DWORD, HANDLE)       { return DD_OK; }
    HRESULT GetAvailableVidMem(LPDDSCAPS, LPDWORD, LPDWORD) { return DD_OK; }
};

#endif /* (FF_LINUX || MA_LINUX) && __cplusplus */
#endif /* MA_COMPAT_DDRAW_LEGACY_H */
