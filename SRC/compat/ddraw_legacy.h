/*
 * Mig Alley Linux port â legacy DirectDraw DX1/DX2 interface bodies.
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
/* S111 (PO-12): the one IDirect3DDevice, handed out by the back surface's QueryInterface. Defined
   in compat/ma_d3d_device.cpp, where the legacy D3D types are complete (this header is included
   from ddraw.h, before d3d.h). */
extern "C" void* ma_d3d_device(void);

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

extern "C" void ma_d3d_texture_forget(void* surf2);   /* S328-S3 */
struct IDirectDrawSurface2;
typedef struct IDirectDrawSurface2 *LPDIRECTDRAWSURFACE2;

/* ---- S116: a real IDirectDrawPalette ---------------------------------------------------------
 * The palette interface is vtbl-based (compat/ddraw.h), so the object is a struct whose first
 * member is the vtbl pointer and whose entries follow. The engine creates MAX_PALS of these for
 * its 8-bit texture art and updates them in place with SetEntries.
 */
struct MaDDPalette {
    IDirectDrawPaletteVtbl* lpVtbl;
    PALETTEENTRY            pe[256];
    unsigned                rgb[256];   /* 0x00RRGGBB, kept in step for the texture uploader */
};

static void ma_pal_sync(MaDDPalette* p)
{
    for (int i = 0; i < 256; ++i)
        p->rgb[i] = ((unsigned)p->pe[i].peRed << 16) | ((unsigned)p->pe[i].peGreen << 8)
                  |  (unsigned)p->pe[i].peBlue;
}
static HRESULT STDMETHODCALLTYPE ma_pal_QI(IDirectDrawPalette*, REFIID, LPVOID* pv) { if (pv) *pv = 0; return DD_OK; }
static ULONG   STDMETHODCALLTYPE ma_pal_AddRef(IDirectDrawPalette*)  { return 1; }
static ULONG   STDMETHODCALLTYPE ma_pal_Release(IDirectDrawPalette*) { return 0; }
static HRESULT STDMETHODCALLTYPE ma_pal_GetCaps(IDirectDrawPalette*, LPDWORD c) { if (c) *c = 0; return DD_OK; }
static HRESULT STDMETHODCALLTYPE ma_pal_GetEntries(IDirectDrawPalette* This, DWORD, DWORD base,
                                                   DWORD count, LPPALETTEENTRY e)
{
    MaDDPalette* p = (MaDDPalette*)This;
    if (!p || !e) return DD_OK;
    for (DWORD i = 0; i < count && base + i < 256; ++i) e[i] = p->pe[base + i];
    return DD_OK;
}
static HRESULT STDMETHODCALLTYPE ma_pal_SetEntries(IDirectDrawPalette* This, DWORD, DWORD base,
                                                   DWORD count, LPPALETTEENTRY e)
{
    MaDDPalette* p = (MaDDPalette*)This;
    if (!p || !e) return DD_OK;
    for (DWORD i = 0; i < count && base + i < 256; ++i) p->pe[base + i] = e[i];
    ma_pal_sync(p);
    return DD_OK;
}
static HRESULT STDMETHODCALLTYPE ma_pal_Initialize(IDirectDrawPalette*, LPDIRECTDRAW, DWORD, LPPALETTEENTRY) { return DD_OK; }

static IDirectDrawPaletteVtbl ma_pal_vtbl = {
    ma_pal_QI, ma_pal_AddRef, ma_pal_Release, ma_pal_GetCaps,
    ma_pal_GetEntries, ma_pal_Initialize, ma_pal_SetEntries
};

static inline LPDIRECTDRAWPALETTE ma_dd_palette_create(LPPALETTEENTRY e)
{
    MaDDPalette* p = (MaDDPalette*)calloc(1, sizeof(MaDDPalette));
    if (!p) return 0;
    p->lpVtbl = &ma_pal_vtbl;
    if (e) for (int i = 0; i < 256; ++i) p->pe[i] = e[i];
    ma_pal_sync(p);
    return (LPDIRECTDRAWPALETTE)p;
}

/* The 0x00RRGGBB table a surface's palette holds, or 0 if it has none. */
static inline const unsigned* ma_dd_palette_rgb(void* pal)
{
    return pal ? ((MaDDPalette*)pal)->rgb : 0;
}

/* ---- DX1 IDirectDrawSurface (real software framebuffer) ---- */
/* PO-82 (S350): CREATE-vs-DESTROY CENSUS, the instrument S349 said the fix needs first.
   S349 established that Release() is a no-op stub here, so surfaces are never destroyed: 5000
   created and 0 destroyed in one four-minute sortie, which is what drives the texture-handle
   runaway behind the PO's white explosions. Any fix means giving these stubs real reference
   counting, and the failure mode of a WRONG refcount is a use-after-free -- strictly worse than
   the white texture it would be curing. So count first, per class, and make the numbers visible
   before and after: a leak fix that cannot be measured is a leak fix nobody should trust.
   BoB carries exactly this (BOB_TRACE_LIFETIME, bob_video.cpp:947) and reads 968 made / 137 freed
   / 831 live, stable -- the shape a healthy port produces.
   MA_TRACE_LIFETIME=1. */
extern "C" void ma_lifetime_note(const char* cls, int made);
struct IDirectDrawSurface {
    void *lpVtbl;
    int   sw, sh, sbpp; long spitch; unsigned char* sbits; int sprimary;
    /* S116: the pixel format the surface was CREATED with (0 masks = not specified -> 565). */
    unsigned long smaskR, smaskG, smaskB, smaskA; int spfSet;
    /* S116: this surface's GL texture, and whether its texels have changed since the upload.
       The engine writes texels by Locking the surface (PrepTexture), so Unlock is the dirty
       edge. */
    unsigned      sglTex; int sdirty;
    /* S117: texels carry coverage in ALPHA with RGB deliberately blank (the engine's font and
       alpha maps). Measured at upload, not assumed from the call site. */
    int           salphaonly;
    /* S116: the palette this surface's 8-bit texels index (SetPalette). The engine keeps
       MAX_PALS of them and picks per texture, so this cannot come from a global. */
    void*         spal;
    IDirectDrawSurface(): lpVtbl(0), sw(0), sh(0), sbpp(8), spitch(0), sbits(0), sprimary(0),
        smaskR(0), smaskG(0), smaskB(0), smaskA(0), spfSet(0), sglTex(0), sdirty(1), salphaonly(0), spal(0),
        sflipback(0), sview(0), stex(0) { ma_lifetime_note("Surface", 1); }
    /* S328-S3: tell the texture-handle registry BEFORE the pixels go away, or it keeps a dangling
       pointer that ma_tex_desc will dereference (PO-82's white textures). */
    virtual ~IDirectDrawSurface() { ma_lifetime_note("Surface", 0); ma_d3d_texture_forget(this); if (sbits) free(sbits); }
    void salloc() {
        if (!sbits && sw > 0 && sh > 0) {
            spitch = (long)sw * ((sbpp + 7) / 8);
            sbits  = (unsigned char*)calloc(1, (size_t)spitch * sh);
        }
    }
    void spresent() { if (sprimary) { salloc(); ma_ddraw_present(sbits, sw, sh, sbpp); } }
    /* S111 (PO-12 phase 1): the DX5/6 path asks the BACK SURFACE for the 3D device --
       `lpDDSBack->QueryInterface(Driver[n].Guid, &lpD3DDevice)` in direct_3d::CreateDevice -- and
       a NULL there makes BeginScene stop with "3D Hardware acceleration is not enabled". Hand back
       the device object while the hardware path is being brought up. One device per process is
       correct here: the game creates exactly one. */
    /* S113: dispatch on the IID. S111 returned the 3D device for ANY request, which was enough to
       get a device but wrong the moment the texture path asks the same surface for its DX2 face
       (direct_3d::CreateTexture) -- it would have handed back the device and then written texels
       through it. Three requests matter here:
         IID_IDirectDrawSurface2 -> a DX2 view sharing THESE pixels
         IID_IDirect3DTexture    -> the texture object bound to this surface
         the driver GUID          -> the one 3D device
       Anything else stays NULL, which is what an unimplemented interface should look like. */
    /* S119: the flip chain's back buffer. A FULLSCREEN primary is created COMPLEX|FLIP with a
       backbuffer count, and the game then asks the primary for it with GetAttachedSurface. */
    struct IDirectDrawSurface*  sflipback;
    struct IDirectDrawSurface2* sview;   /* lazily created DX2 face (owned) */
    void*                       stex;    /* lazily created IDirect3DTexture (owned) */
    HRESULT QueryInterface(REFIID riid, void** p);
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
    /* S119: a flip must show what the game just DREW, which is the back buffer -- not the
       primary's own (untouched) bits. Copy back -> front, then present, which is the same thing
       the windowed path does with an explicit Blt. */
    HRESULT Flip(LPDIRECTDRAWSURFACE override, DWORD) {
        MA_DDTRACE("Flip prim=%d\n",sprimary);
        IDirectDrawSurface* src = override ? override : sflipback;
        if (src && src != this) {
            src->salloc(); salloc();
            if (sbits && src->sbits) {
                size_t n = (size_t)spitch * sh, sn = (size_t)src->spitch * src->sh;
                memcpy(sbits, src->sbits, n < sn ? n : sn);
            }
        }
        spresent();
        return DD_OK;
    }
    /* S119 (found by the PO play-testing the hardware option): this used to write NULL into the
       caller's pointer and return DD_OK -- reporting success while handing back nothing. The
       FULLSCREEN path the hardware renderer takes does
           DD.lpDDSPrimary->GetAttachedSurface(&caps, &DD.lpDDSBack);   (Hardwin.cpp:1110)
       and then Locks the result unconditionally, so a NULL back buffer is an immediate SIGSEGV
       inside the very next call. The windowed path creates its back surface directly, which is
       why software mode never touched this and it stayed a stub.
       The port models one attached surface -- the flip chain's back buffer -- and says so
       honestly: anything else returns DDERR_NOTFOUND rather than a successful NULL. */
    HRESULT GetAttachedSurface(LPDDSCAPS caps, LPDIRECTDRAWSURFACE* s) {
        if (!s) return DD_OK;
        *s = 0;
        if (caps && !(caps->dwCaps & DDSCAPS_BACKBUFFER)) return DDERR_NOTFOUND;
        /* ONLY a primary owns a flip chain. Callers WALK the chain --
             while (lpDDS) { ...clear...; lpDDS->GetAttachedSurface(&caps, &lpDDS); }
           (HARDWIN.CPP:103-128, which ignores the HRESULT and stops on a NULL out-pointer) -- so
           a back buffer that hands out another back buffer is an unbounded chain. The first cut of
           this fix did exactly that and allocated surfaces until calloc failed, turning one crash
           into a different one. The terminator is what makes the walk finite. */
        if (!sprimary) return DDERR_NOTFOUND;
        if (!sflipback) {
            salloc();
            sflipback = new IDirectDrawSurface();
            sflipback->sw = sw; sflipback->sh = sh; sflipback->sbpp = sbpp;
            sflipback->sprimary = 0;
            sflipback->salloc();
            MA_DDTRACE("GetAttachedSurface: created flip back buffer %dx%d bpp%d\n", sw, sh, sbpp);
        }
        *s = sflipback;
        return DD_OK;
    }
    HRESULT GetBltStatus(DWORD)                       { return DD_OK; }
    HRESULT GetCaps(LPDDSCAPS)                        { return DD_OK; }
    HRESULT GetClipper(LPDIRECTDRAWCLIPPER*)          { return DD_OK; }
    HRESULT GetColorKey(DWORD, LPDDCOLORKEY)          { return DD_OK; }
    HRESULT GetDC(HDC*)                               { return DD_OK; }
    HRESULT GetFlipStatus(DWORD)                      { return DD_OK; }
    HRESULT GetOverlayPosition(LPLONG, LPLONG)        { return DD_OK; }
    HRESULT GetPalette(LPDIRECTDRAWPALETTE*)          { return DD_OK; }
    /* fill an RGB pixel-format for this surface's bpp. The 3D mode-set (Hardwin.cpp:203-)
       derives RGB shift/bits by scanning the mask for its low set bit â a ZERO mask spins
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
    HRESULT SetPalette(LPDIRECTDRAWPALETTE p)         { spal = (void*)p; sdirty = 1; return DD_OK; }
    HRESULT Unlock(LPVOID)                            { sdirty = 1; spresent(); return DD_OK; }
    HRESULT UpdateOverlay(LPRECT, LPDIRECTDRAWSURFACE, LPRECT, DWORD, LPVOID) { return DD_OK; }
    HRESULT UpdateOverlayDisplay(DWORD)               { return DD_OK; }
    HRESULT UpdateOverlayZOrder(DWORD, LPDIRECTDRAWSURFACE) { return DD_OK; }
};

/* ---- DX2 IDirectDrawSurface2 (adds PageLock/PageUnlock/GetDDInterface) ---- */
struct IDirectDrawSurface2 {
    void *lpVtbl;
    /* S113 (PO-12 phase 2): a DX2 VIEW of a DX1 surface -- same pixels, different interface.
       direct_3d::CreateTexture creates the texture as a DX1 surface, asks it for its DX2 face, then
       hands that to PrepTexture, which Locks it and writes the texture's texels into
       `tmsd.lpSurface`. With a stub Lock that address is whatever was on the stack: S111's measured
       crash. The view does NOT own the bits -- the DX1 surface does. */
    unsigned char* sbits; int sw, sh, sbpp; long spitch;
    /* S116: the DX1 surface these pixels belong to. The texture uploader needs its pixel format
       and its dirty flag, and only the owner has them. */
    struct IDirectDrawSurface* sowner;
    IDirectDrawSurface2(): lpVtbl(0), sbits(0), sw(0), sh(0), sbpp(16), spitch(0), sowner(0) { ma_lifetime_note("Surface2", 1); }
    virtual ~IDirectDrawSurface2() { ma_lifetime_note("Surface2", 0); }
    HRESULT QueryInterface(REFIID riid, void** p);   /* defined after IDirect3DTexture exists */
    ULONG   AddRef()                                  { return 1; }
    ULONG   Release()                                 { return 0; }
    HRESULT AddAttachedSurface(LPDIRECTDRAWSURFACE2)  { return DD_OK; }
    HRESULT AddOverlayDirtyRect(LPRECT)               { return DD_OK; }
    /* S119 (PO: "terrain is all black", objects on it fine): a REAL Blt.
       This was `{ return DD_OK; }` -- success, nothing copied -- and it is how the LANDSCAPE
       loads its textures (Win3d.cpp:5806):
           PrepLandMap(sysTex, lpImageMap);                              // fill the SYSTEM texture
           vidTex.lpdds2TSurf->Blt(&rect, sysTex.lpdds2TSurf, &rect, ...) // copy to the VIDEO one
       The renderer binds the video texture, which therefore stayed all zeros. Index 0 is the
       engine's transparent key, so every land tile uploaded fully transparent and the cleared
       black showed through -- while aircraft and buildings, which load through
       IDirect3DTexture::Load, drew correctly. That difference is exactly what the PO reported.
       Third stub of this shape found today (GetAttachedSurface, Load's palette, this): a compat
       method that returns DD_OK without doing the work leaves no evidence at the call site. */
    unsigned char* ma_bits() { return sowner ? sowner->sbits : sbits; }
    int  ma_w()     const { return sowner ? sowner->sw    : sw; }
    int  ma_h()     const { return sowner ? sowner->sh    : sh; }
    int  ma_bpp()   const { return sowner ? sowner->sbpp  : sbpp; }
    long ma_pitch() const { return sowner ? sowner->spitch: spitch; }
    HRESULT Blt(LPRECT dstr, LPDIRECTDRAWSURFACE2 src, LPRECT srcr, DWORD, LPVOID) {
        if (!src) return DD_OK;
        if (sowner) sowner->salloc();
        if (src->sowner) src->sowner->salloc();
        unsigned char* d = ma_bits();
        unsigned char* s2 = src->ma_bits();
        if (!d || !s2) return DD_OK;
        int bpp = ma_bpp();
        if (bpp != src->ma_bpp()) return DD_OK;          /* no format conversion here */
        const int bytes = (bpp + 7) / 8;
        int dx = dstr ? dstr->left : 0, dy = dstr ? dstr->top : 0;
        int sx = srcr ? srcr->left : 0, sy = srcr ? srcr->top : 0;
        int w = srcr ? (srcr->right - srcr->left) : src->ma_w();
        int h = srcr ? (srcr->bottom - srcr->top) : src->ma_h();
        if (w > ma_w() - dx)      w = ma_w() - dx;
        if (h > ma_h() - dy)      h = ma_h() - dy;
        if (w > src->ma_w() - sx) w = src->ma_w() - sx;
        if (h > src->ma_h() - sy) h = src->ma_h() - sy;
        if (w <= 0 || h <= 0) return DD_OK;
        for (int y = 0; y < h; ++y)
            memcpy(d + (size_t)(dy + y) * ma_pitch() + (size_t)dx * bytes,
                   s2 + (size_t)(sy + y) * src->ma_pitch() + (size_t)sx * bytes,
                   (size_t)w * bytes);
        if (getenv("MA_TRACE_TEX")) {
            static long calls = 0, nonzero = 0;
            long nz = 0;
            for (int y = 0; y < h; ++y) {
                const unsigned char* r = s2 + (size_t)(sy+y)*src->ma_pitch() + (size_t)sx*bytes;
                for (int x = 0; x < w*bytes; ++x) if (r[x]) { nz++; break; }
            }
            calls++; if (nz) nonzero++;
            if (calls <= 6 || (calls % 200) == 0)
                fprintf(stderr, "[tex] DX2 Blt #%ld %dx%d bpp%d dst=%p src=%p rows-with-data=%ld  (%ld/%ld blits carried data)\n",
                        calls, w, h, bpp, (void*)this, (void*)src, nz, nonzero, calls);
        }
        if (sowner) {
            sowner->sdirty = 1;                       /* re-upload to GL */
            if (!sowner->spal && src->sowner) sowner->spal = src->sowner->spal;
        }
        return DD_OK;
    }
    HRESULT BltBatch(LPVOID, DWORD, DWORD)            { return DD_OK; }
    HRESULT BltFast(DWORD dx, DWORD dy, LPDIRECTDRAWSURFACE2 src, LPRECT srcr, DWORD) {
        RECT d; d.left = (LONG)dx; d.top = (LONG)dy; d.right = 0; d.bottom = 0;
        return Blt(&d, src, srcr, 0, 0);
    }
    HRESULT DeleteAttachedSurface(DWORD, LPDIRECTDRAWSURFACE2) { return DD_OK; }
    HRESULT EnumAttachedSurfaces(LPVOID, LPVOID)      { return DD_OK; }
    HRESULT EnumOverlayZOrders(DWORD, LPVOID, LPVOID) { return DD_OK; }
    HRESULT Flip(LPDIRECTDRAWSURFACE2, DWORD)         { return DD_OK; }
    /* S119: same as the DX1 face -- return the DX2 VIEW of the one back buffer, not a NULL.
       Defined out of line (compat/d3d_execbuf.h) because it needs IID_IDirectDrawSurface2, which
       does not exist yet at this point in the header chain. */
    HRESULT GetAttachedSurface(LPDDSCAPS caps, LPDIRECTDRAWSURFACE2* s);
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
    HRESULT Lock(LPRECT, LPDDSURFACEDESC d, DWORD, HANDLE) {
        /* S120 (PO-15, black hardware terrain): fill the PIXEL FORMAT too, and report the
           owner's live pixels rather than this view's copy.
           The landscape rasterises its tiles through exactly this descriptor:
               pSurf->Lock(NULL,&sd,...);
               rsd.lpSurface = sd.lpSurface;  rsd.lPitch = sd.lPitch;
               rsd.dwRGBBitCount = sd.ddpfPixelFormat.dwRGBBitCount;   <-- was always 0
               Three_Dee.pTMake->RenderTile2Surface(pTileData,&rsd);   (Win3d.cpp:12919)
           With a zero bit count the tile writer has no format to write in, so every land tile
           came back blank -- which is why the sys->vid blits measurably carried no data and the
           terrain drew as transparent black while objects, which never take this path, were
           fine. */
        if (sowner) sowner->salloc();
        if (d) {
            d->dwFlags |= (DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH | DDSD_LPSURFACE | DDSD_PIXELFORMAT);
            d->dwWidth = ma_w(); d->dwHeight = ma_h();
            d->lPitch  = ma_pitch() ? ma_pitch() : (long)ma_w() * ((ma_bpp() + 7) / 8);
            d->lpSurface = ma_bits();
            if (sowner) sowner->ma_fillpf(&d->ddpfPixelFormat);
            else { d->ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
                   d->ddpfPixelFormat.dwFlags = DDPF_RGB;
                   d->ddpfPixelFormat.dwRGBBitCount = sbpp; }
        }
        return DD_OK;
    }
    HRESULT ReleaseDC(HDC)                            { return DD_OK; }
    HRESULT Restore()                                 { return DD_OK; }
    HRESULT SetClipper(LPDIRECTDRAWCLIPPER)           { return DD_OK; }
    HRESULT SetColorKey(DWORD, LPDDCOLORKEY)          { return DD_OK; }
    HRESULT SetOverlayPosition(LONG, LONG)            { return DD_OK; }
    HRESULT SetPalette(LPDIRECTDRAWPALETTE p)         { if (sowner) { sowner->spal = (void*)p; sowner->sdirty = 1; } return DD_OK; }
    /* S116: PrepTexture writes texels through THIS face, so this is the dirty edge that
       matters -- forward it to the surface that owns the pixels and the GL texture. */
    HRESULT Unlock(LPVOID)                            { if (sowner) sowner->sdirty = 1; return DD_OK; }
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
    /* S116: a REAL palette. The 8-bit textures are the game's opaque art, and their colours live
       here -- with CreatePalette handing back NULL the engine had nowhere to put them and every
       palettized texture rendered black. The engine keeps MAX_PALS palettes and calls SetEntries
       to update them in place, so the object has to persist and be writable. */
    HRESULT CreatePalette(DWORD, LPPALETTEENTRY e, LPDIRECTDRAWPALETTE* p, IUnknown*) {
        if (!p) return DD_OK;
        *p = ma_dd_palette_create(e);
        return DD_OK;
    }
    HRESULT CreateSurface(LPDDSURFACEDESC d, LPDIRECTDRAWSURFACE* s, IUnknown*) {
        if (!s) return DD_OK;
        IDirectDrawSurface* surf = new IDirectDrawSurface(); MA_DDTRACE("CreateSurface caps=0x%lx\n",(unsigned long)(d?d->ddsCaps.dwCaps:0));
        DWORD caps = d ? d->ddsCaps.dwCaps : 0;
        surf->sprimary = (caps & DDSCAPS_PRIMARYSURFACE) ? 1 : 0;
        surf->sbpp = (d && d->ddpfPixelFormat.dwRGBBitCount) ? (int)d->ddpfPixelFormat.dwRGBBitCount
                                                             : ma_dd_dispBpp;
        surf->sw = (d && (d->dwFlags & DDSD_WIDTH))  ? (int)d->dwWidth  : ma_dd_dispW;
        surf->sh = (d && (d->dwFlags & DDSD_HEIGHT)) ? (int)d->dwHeight : ma_dd_dispH;
        /* S116: remember the format the caller ASKED for. The game picks from the two formats
           EnumTextureFormats offers (8-bit palettized, ARGB4444), and a texture written as 4444
           but read back as 565 is unrecognisable art with no alpha. */
        if (d && (d->dwFlags & DDSD_PIXELFORMAT)) {
            surf->smaskR = d->ddpfPixelFormat.dwRBitMask;
            surf->smaskG = d->ddpfPixelFormat.dwGBitMask;
            surf->smaskB = d->ddpfPixelFormat.dwBBitMask;
            surf->smaskA = d->ddpfPixelFormat.dwRGBAlphaBitMask;
            surf->spfSet = 1;
        }
        surf->salloc();
        if (surf->sprimary) ma_ddraw_ensure_window(surf->sw, surf->sh);
        if (getenv("MA_TRACE_TEX")) fprintf(stderr,
            "[tex] CreateSurface %dx%d %dbpp caps=0x%lx pf%s R=%08lx G=%08lx B=%08lx A=%08lx\n",
            surf->sw, surf->sh, surf->sbpp, (unsigned long)caps, surf->spfSet ? "" : "(default)",
            (unsigned long)surf->smaskR, (unsigned long)surf->smaskG,
            (unsigned long)surf->smaskB, (unsigned long)surf->smaskA);
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
           window at 16-bit 565 â the depth the software rasterizer renders and the window
           presents â so selection matches the enumerated 640x480x16 mode. */
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
