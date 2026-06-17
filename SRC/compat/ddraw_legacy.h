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

/* Linux/GCC port: present the legacy DX1/DX2 primary surface through bob_video's
   SDL2/GL machinery (see SRC/compat/bob_video.cpp). */
#include <stdlib.h>
#include <string.h>
extern "C" void ma_ddraw_ensure_window(int w, int h);
extern "C" void ma_ddraw_present(const void* bits, int w, int h, int bpp);
#include <stdio.h>
#define MA_DDTRACE(...) do{ if(getenv("MA_TRACE_DD")) fprintf(stderr, "[dd] " __VA_ARGS__); }while(0)
/* Display mode the primary surface inherits (set via IDirectDraw2::SetDisplayMode). */
extern int ma_dd_dispW, ma_dd_dispH, ma_dd_dispBpp;
extern "C" long ma_asmcall_count, ma_asmcall_nullfn;   /* span-filler counters (MA_TRACE_FILL) */

typedef HRESULT (WINAPI *LPDDENUMMODESCALLBACK)(LPDDSURFACEDESC, LPVOID);

struct IDirectDrawSurface2;
typedef struct IDirectDrawSurface2 *LPDIRECTDRAWSURFACE2;

/* ---- DX1 IDirectDrawSurface (real software framebuffer) ---- */
struct IDirectDrawSurface {
    void *lpVtbl;
    int   sw, sh, sbpp; long spitch; unsigned char* sbits; int sprimary;
    IDirectDrawSurface(): lpVtbl(0), sw(0), sh(0), sbpp(8), spitch(0), sbits(0), sprimary(0) {}
    virtual ~IDirectDrawSurface() { if (sbits) free(sbits); }
    void salloc() {
        if (!sbits && sw > 0 && sh > 0) {
            spitch = (long)sw * ((sbpp + 7) / 8);
            sbits  = (unsigned char*)calloc(1, (size_t)spitch * sh);
        }
    }
    void spresent() { if (sprimary) { salloc(); ma_ddraw_present(sbits, sw, sh, sbpp); } }
    HRESULT QueryInterface(REFIID, void** p)          { if(p)*p=0; return DD_OK; }
    ULONG   AddRef()                                  { return 1; }
    ULONG   Release()                                 { return 0; }
    HRESULT AddAttachedSurface(LPDIRECTDRAWSURFACE)   { return DD_OK; }
    HRESULT AddOverlayDirtyRect(LPRECT)               { return DD_OK; }
    HRESULT Blt(LPRECT, LPDIRECTDRAWSURFACE src, LPRECT, DWORD, LPVOID) {
        if (src) { src->salloc(); salloc();
            if (sbits && src->sbits) {
                size_t n = (size_t)spitch * sh, sn = (size_t)src->spitch * src->sh;
                memcpy(sbits, src->sbits, n < sn ? n : sn);
            }
        }
#if defined(MA_LINUX)
        if (getenv("MA_TRACE_DD") && src && src->sbits) {
            size_t sn = (size_t)src->spitch * src->sh, nz = 0;
            for (size_t i = 0; i < sn; ++i) if (src->sbits[i]) { nz++; }
            fprintf(stderr,"[dd] Blt prim=%d src=%dx%d bpp=%d bits=%p nonzero=%zu/%zu fills=%ld null=%ld\n",
                    sprimary, src->sw, src->sh, src->sbpp, (void*)src->sbits, nz, sn,
                    ma_asmcall_count, ma_asmcall_nullfn);
        } else MA_DDTRACE("Blt prim=%d\n",sprimary);
#endif
        spresent();   /* a Blt onto the primary is the frontend's present */
        return DD_OK;
    }
    HRESULT BltBatch(LPVOID, DWORD, DWORD)            { return DD_OK; }
    HRESULT BltFast(DWORD, DWORD, LPDIRECTDRAWSURFACE, LPRECT, DWORD) { return DD_OK; }
    HRESULT DeleteAttachedSurface(DWORD, LPDIRECTDRAWSURFACE) { return DD_OK; }
    HRESULT EnumAttachedSurfaces(LPVOID, LPVOID)      { return DD_OK; }
    HRESULT EnumOverlayZOrders(DWORD, LPVOID, LPVOID) { return DD_OK; }
    HRESULT Flip(LPDIRECTDRAWSURFACE, DWORD) { MA_DDTRACE("Flip prim=%d\n",sprimary); spresent(); return DD_OK; }
    HRESULT GetAttachedSurface(LPDDSCAPS, LPDIRECTDRAWSURFACE* s) { if(s)*s=0; return DD_OK; }
    HRESULT GetBltStatus(DWORD)                       { return DD_OK; }
    HRESULT GetCaps(LPDDSCAPS)                        { return DD_OK; }
    HRESULT GetClipper(LPDIRECTDRAWCLIPPER*)          { return DD_OK; }
    HRESULT GetColorKey(DWORD, LPDDCOLORKEY)          { return DD_OK; }
    HRESULT GetDC(HDC*)                               { return DD_OK; }
    HRESULT GetFlipStatus(DWORD)                      { return DD_OK; }
    HRESULT GetOverlayPosition(LPLONG, LPLONG)        { return DD_OK; }
    HRESULT GetPalette(LPDIRECTDRAWPALETTE*)          { return DD_OK; }
    /* fill an RGB pixel-format for this surface's bpp. The 3D mode-set (Hardwin.cpp:203-)
       derives RGB shift/bits by scanning the mask for its low set bit — a ZERO mask spins
       forever, so non-8bpp surfaces MUST report real masks (565 for 16-bit). */
    void ma_fillpf(LPDDPIXELFORMAT pf) {
        if (!pf) return;
        pf->dwSize = sizeof(DDPIXELFORMAT);
        pf->dwFlags = DDPF_RGB;
        if (sbpp == 8)       { pf->dwRGBBitCount=8;  pf->dwRBitMask=0; pf->dwGBitMask=0; pf->dwBBitMask=0; }  /* palettized */
        else if (sbpp == 32) { pf->dwRGBBitCount=32; pf->dwRBitMask=0xFF0000; pf->dwGBitMask=0x00FF00; pf->dwBBitMask=0x0000FF; }
        else                 { pf->dwRGBBitCount=16; pf->dwRBitMask=0xF800;   pf->dwGBitMask=0x07E0;   pf->dwBBitMask=0x001F; }
        /* default (incl. sbpp==0/unset) -> 16-bit 565: the mask-derivation loop in the 3D mode-set
           (Hardwin.cpp:203-) is only reached for non-8bpp modes, so a non-zero RGB mask is required. */
    }
    HRESULT GetPixelFormat(LPDDPIXELFORMAT pf)        { ma_fillpf(pf); return DD_OK; }
    HRESULT GetSurfaceDesc(LPDDSURFACEDESC d) {
        if (d) { d->dwWidth = sw; d->dwHeight = sh; d->lPitch = spitch; d->lpSurface = sbits; ma_fillpf(&d->ddpfPixelFormat); }
        return DD_OK;
    }
    HRESULT Initialize(LPDIRECTDRAW, LPDDSURFACEDESC) { return DD_OK; }
    HRESULT IsLost()                                  { return DD_OK; }
    HRESULT Lock(LPRECT, LPDDSURFACEDESC d, DWORD, HANDLE) {
        salloc();
        if (d) { d->lpSurface = sbits; d->lPitch = spitch; d->dwWidth = sw; d->dwHeight = sh; ma_fillpf(&d->ddpfPixelFormat); }
        return DD_OK;
    }
    HRESULT ReleaseDC(HDC)                            { return DD_OK; }
    HRESULT Restore()                                 { return DD_OK; }
    HRESULT SetClipper(LPDIRECTDRAWCLIPPER)           { return DD_OK; }
    HRESULT SetColorKey(DWORD, LPDDCOLORKEY)          { return DD_OK; }
    HRESULT SetOverlayPosition(LONG, LONG)            { return DD_OK; }
    HRESULT SetPalette(LPDIRECTDRAWPALETTE)           { return DD_OK; }
    HRESULT Unlock(LPVOID)                            { spresent(); return DD_OK; }
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
    HRESULT CreateSurface(LPDDSURFACEDESC d, LPDIRECTDRAWSURFACE* s, IUnknown*) {
        if (!s) return DD_OK;
        IDirectDrawSurface* surf = new IDirectDrawSurface(); MA_DDTRACE("CreateSurface caps=0x%lx\n",(unsigned long)(d?d->ddsCaps.dwCaps:0));
        DWORD caps = d ? d->ddsCaps.dwCaps : 0;
        surf->sprimary = (caps & DDSCAPS_PRIMARYSURFACE) ? 1 : 0;
        surf->sbpp = (d && d->ddpfPixelFormat.dwRGBBitCount) ? (int)d->ddpfPixelFormat.dwRGBBitCount
                                                             : ma_dd_dispBpp;
        surf->sw = (d && (d->dwFlags & DDSD_WIDTH))  ? (int)d->dwWidth  : ma_dd_dispW;
        surf->sh = (d && (d->dwFlags & DDSD_HEIGHT)) ? (int)d->dwHeight : ma_dd_dispH;
        surf->salloc();
        if (surf->sprimary) ma_ddraw_ensure_window(surf->sw, surf->sh);
        *s = surf;
        return DD_OK;
    }
    HRESULT DuplicateSurface(LPDIRECTDRAWSURFACE, LPDIRECTDRAWSURFACE*) { return DD_OK; }
    /* Feed the game's mode-enumeration callback a set of software modes. The 3D Display init
       (DirectDD ctor, WIN3D.CPP) needs a 640x480x8 mode to exist or it SayAndQuits; the flat-shade
       rasterizer outputs 16-bit 565, so offer 8- and 16-bit at the common resolutions. */
    HRESULT EnumDisplayModes(DWORD, LPDDSURFACEDESC, LPVOID ctx, LPVOID cbv) {
        LPDDENUMMODESCALLBACK cb = (LPDDENUMMODESCALLBACK)cbv;
        if (getenv("MA_TRACE_3D")) fprintf(stderr,"[3d] IDirectDraw2::EnumDisplayModes called cb=%p\n",(void*)cb);
        if (!cb) return DD_OK;
        static const int dims[][2] = { {640,480}, {800,600}, {1024,768}, {1280,1024} };
        static const int bpps[] = { 8, 16 };
        for (unsigned i = 0; i < sizeof(dims)/sizeof(dims[0]); ++i)
        for (unsigned b = 0; b < sizeof(bpps)/sizeof(bpps[0]); ++b) {
            DDSURFACEDESC d; memset(&d, 0, sizeof(d));
            d.dwSize  = sizeof(d);
            d.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
            d.dwWidth = dims[i][0]; d.dwHeight = dims[i][1];
            d.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
            d.ddpfPixelFormat.dwFlags = DDPF_RGB;
            d.ddpfPixelFormat.dwRGBBitCount = bpps[b];
            if (bpps[b] == 16) {   /* 5-6-5 */
                d.ddpfPixelFormat.dwRBitMask = 0xF800;
                d.ddpfPixelFormat.dwGBitMask = 0x07E0;
                d.ddpfPixelFormat.dwBBitMask = 0x001F;
            }   /* 8-bit: palettized, masks 0 */
            if (cb(&d, ctx) == DDENUMRET_CANCEL) return DD_OK;
        }
        return DD_OK;
    }
    HRESULT EnumSurfaces(DWORD, LPDDSURFACEDESC, LPVOID, LPVOID) { return DD_OK; }
    HRESULT FlipToGDISurface()                        { return DD_OK; }
    HRESULT GetCaps(LPDDCAPS, LPDDCAPS)               { return DD_OK; }
    HRESULT GetDisplayMode(LPDDSURFACEDESC d) {
        /* Only the 3D Display init (WIN3D.CPP:1610) calls this, to learn the "desktop" mode
           (winmode_w/h/bpp). A no-op left those 0, collapsing mode selection onto a bogus
           0x0x0 entry whose colourdepth=0 spins XX_SetGraphicsMode's mask loop. Report the
           window at 16-bit 565 — the depth the software rasterizer renders and the window
           presents — so selection matches the enumerated 640x480x16 mode. */
        if (d) {
            d->dwWidth  = ma_dd_dispW;
            d->dwHeight = ma_dd_dispH;
            d->lPitch   = (long)ma_dd_dispW * 2;
            d->ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
            d->ddpfPixelFormat.dwFlags = DDPF_RGB;
            d->ddpfPixelFormat.dwRGBBitCount = 16;
            d->ddpfPixelFormat.dwRBitMask = 0xF800;
            d->ddpfPixelFormat.dwGBitMask = 0x07E0;
            d->ddpfPixelFormat.dwBBitMask = 0x001F;
        }
        return DD_OK;
    }
    HRESULT GetFourCCCodes(LPDWORD, LPDWORD)          { return DD_OK; }
    HRESULT GetGDISurface(LPDIRECTDRAWSURFACE*)       { return DD_OK; }
    HRESULT GetMonitorFrequency(LPDWORD)              { return DD_OK; }
    HRESULT GetScanLine(LPDWORD)                      { return DD_OK; }
    HRESULT GetVerticalBlankStatus(LPBOOL)            { return DD_OK; }
    HRESULT Initialize(GUID*)                         { return DD_OK; }
    HRESULT RestoreDisplayMode()                      { return DD_OK; }
    HRESULT SetCooperativeLevel(HWND, DWORD)          { ma_ddraw_ensure_window(ma_dd_dispW, ma_dd_dispH); return DD_OK; }
    HRESULT SetDisplayMode(DWORD w, DWORD h, DWORD bpp, DWORD, DWORD) {
        if (w) ma_dd_dispW = (int)w; if (h) ma_dd_dispH = (int)h; if (bpp) ma_dd_dispBpp = (int)bpp;
        ma_ddraw_ensure_window(ma_dd_dispW, ma_dd_dispH);
        return DD_OK;
    }
    HRESULT WaitForVerticalBlank(DWORD, HANDLE)       { return DD_OK; }
    HRESULT GetAvailableVidMem(LPDDSCAPS, LPDWORD, LPDWORD) { return DD_OK; }
};

#endif /* (FF_LINUX || MA_LINUX) && __cplusplus */
#endif /* MA_COMPAT_DDRAW_LEGACY_H */
