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

/* S104: armed frame dump. Set to N (frames) by whatever is being tested; the N-th back->primary
   Blt after that writes /tmp/maback.ppm and clears it. Defined in bob_video.cpp. */
extern "C" int ma_dump_arm;
/* S107: armed key press (see bob_video.cpp) -- the N-th frame after arming injects the DIK. */
extern "C" int ma_uiscr_key_arm; extern "C" int ma_uiscr_key_dik;
extern "C" void ma_inject_dik(int dik);

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
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
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
        /* MA_DUMP_BACK=N: write the N-th back->primary Blt source (16-bit 565) as a PPM to
           /tmp/maback.ppm, to verify the software 3D render independent of the present path. */
        {
            const char* df = (src && src->sbits && src->sbpp==16) ? getenv("MA_DUMP_BACK") : 0;
            bool ma_want_dump = false;
            if (df) { static long bcount = 0; long want = atol(df); ++bcount; if (bcount == want) ma_want_dump = true; }
            /* S104: an ARMED dump. A frame counter cannot aim at an event whose frame number is
               not known in advance -- the in-flight UI screens open when a key is pressed and
               close themselves after five seconds, and the pump counter that delivers the key and
               the Blt counter that numbers the frames run at completely different rates during
               flight. So the thing being tested arms the capture (`ma_dump_arm = N frames`) and
               this fires it. Same lesson as S80: arm the capture from the drive, never from an
               absolute idle number. */
            if (ma_dump_arm > 0 && src && src->sbits && src->sbpp==16 && --ma_dump_arm == 0)
                ma_want_dump = true;
            /* S107: the armed key press fires on the same per-frame clock as the armed dump. */
            if (ma_uiscr_key_arm > 0 && --ma_uiscr_key_arm == 0 && ma_uiscr_key_dik) {
                fprintf(stderr,"[uiscr] injecting armed key dik=0x%02X\n", (unsigned)ma_uiscr_key_dik);
                ma_inject_dik(ma_uiscr_key_dik);
            }
            if (ma_want_dump) {
                {
                    /* raw POSIX write: the compat layer #defines fopen->fopen_nocase, which
                       resolves under BOB_DRIVE_C and can't create /tmp paths. */
                    int fd = ::open("/tmp/maback.ppm", O_WRONLY|O_CREAT|O_TRUNC, 0644);
                    if (fd >= 0) {
                        char hdr[64]; int hl = snprintf(hdr,sizeof(hdr),"P6\n%d %d\n255\n",src->sw,src->sh);
                        ssize_t wr = ::write(fd, hdr, hl); (void)wr;
                        unsigned char* rgb = (unsigned char*)malloc((size_t)src->sw*3);
                        for (int y=0;y<src->sh;++y){
                            const unsigned short* row=(const unsigned short*)(src->sbits+(long)y*src->spitch);
                            for (int x=0;x<src->sw;++x){
                                unsigned short v=row[x];
                                rgb[x*3]=((v>>11)&0x1F)<<3; rgb[x*3+1]=((v>>5)&0x3F)<<2; rgb[x*3+2]=(v&0x1F)<<3;
                            }
                            wr = ::write(fd, rgb, (size_t)src->sw*3);
                        }
                        free(rgb); ::close(fd);
                        /* keep the "dumped back-surface Blt" wording: port/asan_flight.sh and
                           port/stress_launch.sh classify a run by grepping for it. */
                        fprintf(stderr,"[dd] dumped back-surface Blt to /tmp/maback.ppm (src bits=%p %dx%d)\n",
                                (void*)src->sbits, src->sw, src->sh);
                        {   /* S105: did the last marked glyph pixel survive to the handover? */
                            extern unsigned short* ma_mark_addr; extern unsigned short ma_mark_val;
                            if (ma_mark_addr) {
                                long off = (long)((char*)ma_mark_addr - (char*)src->sbits);
                                fprintf(stderr,"[dd] mark @%p (offset %ld into this surface, %s): wrote 0x%04X, now 0x%04X -> %s\n",
                                        (void*)ma_mark_addr, off,
                                        (off>=0 && off < (long)src->spitch*src->sh) ? "INSIDE" : "OUTSIDE",
                                        (unsigned)ma_mark_val, (unsigned)*ma_mark_addr,
                                        (*ma_mark_addr==ma_mark_val) ? "survived" : "OVERWRITTEN");
                            }
                        }
                    } else fprintf(stderr,"[dd] dump open failed errno=%d\n",errno);
                }
            }
        }
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
        /* Must include every mode the resolution combo offers (Win3d.cpp
           ma_populate_software_modes), else the selected mode has no matching DD.DDModes entry
           and the windowed software flight can't apply it (DDRWINIT matches Save_Data.displayW/H). */
        static const int dims[][2] = { {640,480}, {800,600}, {1024,768}, {1280,960}, {1280,1024}, {1600,1200}, {1920,1080} };
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
