/*
 * Mig Alley Linux port — legacy Direct3D "execute buffer" API (DirectX 5/6).
 *
 * The FreeFalcon/BoB compat layer targets Direct3D 7 (DrawPrimitive). Mig Alley
 * (1999) predates that and drives the hardware path through the older execute-
 * buffer immediate mode: IDirect3DDevice::CreateExecuteBuffer / Execute, with
 * D3DINSTRUCTION / D3DSTATE / D3DPROCESSVERTICES opcode streams. None of that
 * exists in the DX7 compat, so this header recreates the legacy type surface
 * and interface vtables.
 *
 * The interface methods are COMPILE-TIME STUBS (return D3D_OK). The real D3D
 * hardware path is not yet wired to OpenGL; the game runs software-rendered
 * first, so these are never exercised at runtime. Flesh them out alongside the
 * GRAPHICS/SDL runtime when enabling hardware acceleration.
 *
 * Included from compat/d3d.h, AFTER d3dtypes.h and ddraw.h.
 */
#ifndef MA_COMPAT_D3D_EXECBUF_H
#define MA_COMPAT_D3D_EXECBUF_H

#if defined(FF_LINUX) || defined(MA_LINUX)

/* ============================================================
 * Forward decls + interface pointer typedefs
 * ============================================================ */
struct IDirect3D;
struct IDirect3DDevice;
struct IDirect3DViewport;
struct IDirect3DExecuteBuffer;
struct IDirect3DTexture;
struct IDirect3DMaterial;
struct IDirect3DLight;

typedef struct IDirect3D              *LPDIRECT3D;
typedef struct IDirect3DDevice        *LPDIRECT3DDEVICE;
typedef struct IDirect3DViewport      *LPDIRECT3DVIEWPORT;
typedef struct IDirect3DExecuteBuffer *LPDIRECT3DEXECUTEBUFFER;
typedef struct IDirect3DTexture       *LPDIRECT3DTEXTURE;
typedef struct IDirect3DMaterial      *LPDIRECT3DMATERIAL;
typedef struct IDirect3DLight         *LPDIRECT3DLIGHT;

/* ============================================================
 * IIDs (by-value statics, like the rest of the compat layer)
 * ============================================================ */
static const GUID IID_IDirect3D            = {0x3BBA0080,0x2421,0x11CF,{0xA3,0x1A,0x00,0xAA,0x00,0xB9,0x33,0x56}};
static const GUID IID_IDirect3DTexture     = {0x2CDCD9E0,0x25A0,0x11CF,{0xA3,0x1A,0x00,0xAA,0x00,0xB9,0x33,0x56}};
static const GUID IID_IDirectDraw2         = {0xB3A6F3E0,0x2B43,0x11CF,{0xA2,0xDE,0x00,0xAA,0x00,0xB9,0x33,0x56}};
static const GUID IID_IDirectDrawSurface2  = {0x57805885,0x6eec,0x11cf,{0x94,0x41,0xa8,0x23,0x03,0xc1,0x0e,0x27}};

/* ============================================================
 * Execute-buffer opcode + flag constants
 * ============================================================ */
typedef enum _D3DOPCODE {
    D3DOP_POINT           = 1,
    D3DOP_LINE            = 2,
    D3DOP_TRIANGLE        = 3,
    D3DOP_MATRIXLOAD      = 4,
    D3DOP_MATRIXMULTIPLY  = 5,
    D3DOP_STATETRANSFORM  = 6,
    D3DOP_STATELIGHT      = 7,
    D3DOP_STATERENDER     = 8,
    D3DOP_PROCESSVERTICES = 9,
    D3DOP_TEXTURELOAD     = 10,
    D3DOP_EXIT            = 11,
    D3DOP_BRANCHFORWARD   = 12,
    D3DOP_SPAN            = 13,
    D3DOP_SETSTATUS       = 14,
    D3DOP_FORCE_DWORD     = 0x7fffffff
} D3DOPCODE;

/* D3DPROCESSVERTICES dwFlags */
#define D3DPROCESSVERTICES_TRANSFORMLIGHT 0x00000000L
#define D3DPROCESSVERTICES_TRANSFORM      0x00000001L
#define D3DPROCESSVERTICES_COPY           0x00000002L
#define D3DPROCESSVERTICES_OPMASK         0x00000007L
#define D3DPROCESSVERTICES_UPDATEEXTENTS  0x00000008L
#define D3DPROCESSVERTICES_NOCOLOR        0x00000010L

/* D3DSTATUS / SETSTATUS */
#define D3DSETSTATUS_STATUS   0x00000001L
#define D3DSETSTATUS_EXTENTS  0x00000002L
#define D3DSETSTATUS_ALL      (D3DSETSTATUS_STATUS | D3DSETSTATUS_EXTENTS)
#ifndef D3DSTATUS_CLIPINTERSECTIONALL
#define D3DSTATUS_CLIPINTERSECTIONALL 0x00001FE0L
#endif

/* Execute clip flags */
#define D3DEXECUTE_CLIPPED    0x00000001L
#define D3DEXECUTE_UNCLIPPED  0x00000002L

/* D3DEXECUTEBUFFERDESC dwFlags */
#define D3DDEB_BUFSIZE        0x00000001L
#define D3DDEB_CAPS           0x00000002L
#define D3DDEB_LPDATA         0x00000004L
/* D3DEXECUTEBUFFERDESC dwCaps */
#define D3DDEBCAPS_SYSTEMMEMORY 0x00000001L
#define D3DDEBCAPS_VIDEOMEMORY  0x00000002L
#define D3DDEBCAPS_MEM          (D3DDEBCAPS_SYSTEMMEMORY|D3DDEBCAPS_VIDEOMEMORY)

/* Triangle edge/strip flags */
#define D3DTRIFLAG_START        0x00000000L
#define D3DTRIFLAG_STARTFLAT(len) (len) /* 1..29 */
#define D3DTRIFLAG_ODD          0x0000001eL
#define D3DTRIFLAG_EVEN         0x0000001fL
#define D3DTRIFLAG_EDGEENABLE1  0x00000100L
#define D3DTRIFLAG_EDGEENABLE2  0x00000200L
#define D3DTRIFLAG_EDGEENABLE3  0x00000400L
#define D3DTRIFLAG_EDGEENABLETRIANGLE 0x00000700L

/* Texture-blend caps bit used by Win3d's cap test */
#ifndef D3DPTBLENDCAPS_MODULATEALPHA
#define D3DPTBLENDCAPS_MODULATEALPHA 0x00000010L
#endif

/* ============================================================
 * Texture filter / blend enums (legacy)
 * ============================================================ */
typedef enum _D3DTEXTUREFILTER {
    D3DFILTER_NEAREST          = 1,
    D3DFILTER_LINEAR           = 2,
    D3DFILTER_MIPNEAREST       = 3,
    D3DFILTER_MIPLINEAR        = 4,
    D3DFILTER_LINEARMIPNEAREST = 5,
    D3DFILTER_LINEARMIPLINEAR  = 6,
    D3DFILTER_FORCE_DWORD      = 0x7fffffff
} D3DTEXTUREFILTER;

typedef enum _D3DTEXTUREBLEND {
    D3DTBLEND_DECAL         = 1,
    D3DTBLEND_MODULATE      = 2,
    D3DTBLEND_DECALALPHA    = 3,
    D3DTBLEND_MODULATEALPHA = 4,
    D3DTBLEND_DECALMASK     = 5,
    D3DTBLEND_MODULATEMASK  = 6,
    D3DTBLEND_COPY          = 7,
    D3DTBLEND_ADD           = 8,
    D3DTBLEND_FORCE_DWORD   = 0x7fffffff
} D3DTEXTUREBLEND;

/* ============================================================
 * Execute-buffer data structures (canonical DX6 layout)
 * ============================================================ */
/* D3DRECT / LPD3DRECT already come from compat/d3dtypes.h */

typedef struct _D3DSTATUS {
    DWORD   dwFlags;
    DWORD   dwStatus;
    D3DRECT drExtent;
} D3DSTATUS, *LPD3DSTATUS;

typedef struct _D3DINSTRUCTION {
    BYTE bOpcode;   /* D3DOPCODE */
    BYTE bSize;     /* size of each operand */
    WORD wCount;    /* number of operands */
} D3DINSTRUCTION, *LPD3DINSTRUCTION;

typedef struct _D3DTRIANGLE {
    union { WORD v1; WORD wV1; };
    union { WORD v2; WORD wV2; };
    union { WORD v3; WORD wV3; };
    WORD wFlags;
} D3DTRIANGLE, *LPD3DTRIANGLE;

typedef struct _D3DLINE {
    union { WORD v1; WORD wV1; };
    union { WORD v2; WORD wV2; };
} D3DLINE, *LPD3DLINE;

typedef struct _D3DSPAN {
    WORD wCount;
    WORD wFirst;
} D3DSPAN, *LPD3DSPAN;

typedef struct _D3DPOINT {
    WORD wCount;
    WORD wFirst;
} D3DPOINT, *LPD3DPOINT;

/* Legacy light-state enum (absent from the DX7 compat) */
typedef enum _D3DLIGHTSTATETYPE {
    D3DLIGHTSTATE_MATERIAL      = 1,
    D3DLIGHTSTATE_AMBIENT       = 2,
    D3DLIGHTSTATE_COLORMODEL    = 3,
    D3DLIGHTSTATE_FOGMODE       = 4,
    D3DLIGHTSTATE_FOGSTART      = 5,
    D3DLIGHTSTATE_FOGEND        = 6,
    D3DLIGHTSTATE_FOGDENSITY    = 7,
    D3DLIGHTSTATE_COLORVERTEX   = 8,
    D3DLIGHTSTATE_FORCE_DWORD   = 0x7fffffff
} D3DLIGHTSTATETYPE;

/* Legacy render-state value aliases used in execute-buffer D3DSTATE ops.
 * (The DX7 D3DRENDERSTATETYPE enum lacks some legacy spellings; expose the
 * canonical numeric values so D3DSTATE.drstRenderStateType assignments compile.) */
#ifndef D3DRENDERSTATE_BLENDENABLE
#define D3DRENDERSTATE_BLENDENABLE   27
#endif
#ifndef D3DRENDERSTATE_TEXTUREHANDLE
#define D3DRENDERSTATE_TEXTUREHANDLE 1
#endif
#ifndef D3DRENDERSTATE_FOGTABLESTART
#define D3DRENDERSTATE_FOGTABLESTART 36
#endif
#ifndef D3DRENDERSTATE_FOGTABLEEND
#define D3DRENDERSTATE_FOGTABLEEND   37
#endif
#ifndef D3DRENDERSTATE_FOGTABLEMODE
#define D3DRENDERSTATE_FOGTABLEMODE  35
#endif
#ifndef D3DRENDERSTATE_FOGTABLEDENSITY
#define D3DRENDERSTATE_FOGTABLEDENSITY 38
#endif
#ifndef D3DRENDERSTATE_TEXTUREMAPBLEND
#define D3DRENDERSTATE_TEXTUREMAPBLEND 21
#endif

typedef struct _D3DSTATE {
    union {
        D3DTRANSFORMSTATETYPE dtstTransformStateType;
        D3DLIGHTSTATETYPE     dlstLightStateType;
        D3DRENDERSTATETYPE    drstRenderStateType;
    };
    union {
        DWORD    dwArg[1];
        D3DVALUE dvArg[1];
    };
} D3DSTATE, *LPD3DSTATE;

typedef struct _D3DMATRIXLOAD {
    D3DMATRIXHANDLE hDestMatrix;
    D3DMATRIXHANDLE hSrcMatrix;
} D3DMATRIXLOAD, *LPD3DMATRIXLOAD;

typedef struct _D3DMATRIXMULTIPLY {
    D3DMATRIXHANDLE hDestMatrix;
    D3DMATRIXHANDLE hSrcMatrix1;
    D3DMATRIXHANDLE hSrcMatrix2;
} D3DMATRIXMULTIPLY, *LPD3DMATRIXMULTIPLY;

typedef struct _D3DPROCESSVERTICES {
    DWORD dwFlags;
    WORD  wStart;
    WORD  wDest;
    DWORD dwCount;
    DWORD dwReserved;
} D3DPROCESSVERTICES, *LPD3DPROCESSVERTICES;

typedef struct _D3DEXECUTEDATA {
    DWORD     dwSize;
    DWORD     dwVertexOffset;
    DWORD     dwVertexCount;
    DWORD     dwInstructionOffset;
    DWORD     dwInstructionLength;
    DWORD     dwHVertexOffset;
    D3DSTATUS dsStatus;
} D3DEXECUTEDATA, *LPD3DEXECUTEDATA;

typedef struct _D3DEXECUTEBUFFERDESC {
    DWORD  dwSize;
    DWORD  dwFlags;
    DWORD  dwCaps;
    DWORD  dwBufferSize;
    LPVOID lpData;
} D3DEXECUTEBUFFERDESC, *LPD3DEXECUTEBUFFERDESC;

/* Legacy viewport descriptor */
typedef struct _D3DVIEWPORT {
    DWORD    dwSize;
    DWORD    dwX;
    DWORD    dwY;
    DWORD    dwWidth;
    DWORD    dwHeight;
    D3DVALUE dvScaleX;
    D3DVALUE dvScaleY;
    D3DVALUE dvMaxX;
    D3DVALUE dvMaxY;
    D3DVALUE dvMinZ;
    D3DVALUE dvMaxZ;
} D3DVIEWPORT, *LPD3DVIEWPORT;
typedef D3DVIEWPORT d3dviewportdata;

/* Legacy color model */
typedef enum _D3DCOLORMODEL {
    D3DCOLOR_MONO        = 1,
    D3DCOLOR_RGB         = 2,
    D3DCOLORMODEL_FORCE_DWORD = 0x7fffffff
} D3DCOLORMODEL;

/* D3DPRIMCAPS / LPD3DPRIMCAPS already come from compat/d3d.h */

typedef struct _D3DTransformCaps { DWORD dwSize; DWORD dwCaps; } D3DTRANSFORMCAPS, *LPD3DTRANSFORMCAPS;
typedef struct _D3DLightingCaps {
    DWORD dwSize; DWORD dwCaps; DWORD dwLightingModel; DWORD dwNumLights;
} D3DLIGHTINGCAPS, *LPD3DLIGHTINGCAPS;

/* Legacy device-description (EnumDevices callback consumes it) */
typedef struct _D3DDeviceDesc {
    DWORD            dwSize;
    DWORD            dwFlags;
    D3DCOLORMODEL    dcmColorModel;
    DWORD            dwDevCaps;
    D3DTRANSFORMCAPS dtcTransformCaps;
    BOOL             bClipping;
    D3DLIGHTINGCAPS  dlcLightingCaps;
    D3DPRIMCAPS      dpcLineCaps;
    D3DPRIMCAPS      dpcTriCaps;
    DWORD            dwDeviceRenderBitDepth;
    DWORD            dwDeviceZBufferBitDepth;
    DWORD            dwMaxBufferSize;
    DWORD            dwMaxVertexCount;
} D3DDEVICEDESC, *LPD3DDEVICEDESC;

/* Callback typedefs used by Enum* */
typedef HRESULT (STDMETHODCALLTYPE *LPD3DENUMDEVICESCALLBACK)(GUID*, char*, char*, LPD3DDEVICEDESC, LPD3DDEVICEDESC, LPVOID);
typedef HRESULT (STDMETHODCALLTYPE *LPD3DENUMTEXTUREFORMATSCALLBACK)(LPDDSURFACEDESC, LPVOID);

/* ============================================================
 * Legacy COM interfaces — compile-time stubs (return D3D_OK)
 * ============================================================ */
#ifdef __cplusplus

/* S110 (PO-12, hardware graphics): the stubs below are where the DX5/6 hardware path would live,
   and the first question for scoping it is WHICH of them the game actually calls, and how often.
   MA_TRACE_D3D=1 prints each method the first time it is called and a per-method total at exit, so
   the work list is measured rather than read off the header. Nothing here renders yet. */
extern "C" void ma_d3d_note(const char* method);
/* S118: is the hardware driver offered? (compat/ma_d3d_device.cpp) */
extern "C" int ma_hardware_available(void);
/* S115: the execute-buffer walk (compat/ma_d3d_exec.cpp). Declared, not included, so this header
   stays free of GL and of the walk's own headers. */
extern "C" void ma_d3d_exec_run(void* buf, unsigned long bufSize, const void* execData);
extern "C" void ma_gl_exec_begin(void);
/* S115: remember which surface a texture handle names, so S116 can bind its pixels. */
extern "C" void ma_d3d_texture_register(unsigned long handle, void* surf2);
extern "C" void ma_gl_exec_end(void);

struct IDirect3DTexture {
    /* S115: the handle lives HERE, in the base. S113 put it on the MaD3DTexture subclass, but
       none of these methods are virtual -- the game holds an LPDIRECT3DTEXTURE, so GetHandle
       dispatched statically to this class and every texture reported handle 0. Zero means "no
       texture" to the engine, which is why the S115 census counted 136433 TEXTUREHANDLE sets and
       not one textured triangle. */
    D3DTEXTUREHANDLE            mHandle;
    struct IDirectDrawSurface2* mSurf;
    IDirect3DTexture() : mHandle(0), mSurf(0) {}
    virtual ~IDirect3DTexture() {}
    HRESULT QueryInterface(REFIID, void**)            { return D3D_OK; }
    ULONG   AddRef()                                  { return 1; }
    ULONG   Release()                                 { return 0; }
    HRESULT Initialize(LPDIRECT3DDEVICE, LPDIRECTDRAWSURFACE) { ma_d3d_note("IDirect3DTexture::Initialize"); return D3D_OK; }
    HRESULT GetHandle(LPDIRECT3DDEVICE, D3DTEXTUREHANDLE* h) { ma_d3d_note("IDirect3DTexture::GetHandle"); if(h)*h=mHandle; return D3D_OK; }
    HRESULT PaletteChanged(DWORD, DWORD) { ma_d3d_note("IDirect3DTexture::PaletteChanged"); return D3D_OK; }
    /* S116: defined out of line, below IDirectDrawSurface2 -- it needs the surfaces. */
    HRESULT Load(LPDIRECT3DTEXTURE src);
    HRESULT Unload() { ma_d3d_note("IDirect3DTexture::Unload"); return D3D_OK; }
};

struct IDirect3DLight {
    virtual ~IDirect3DLight() {}
    HRESULT QueryInterface(REFIID, void**)            { return D3D_OK; }
    ULONG   AddRef()                                  { return 1; }
    ULONG   Release()                                 { return 0; }
    HRESULT Initialize(LPDIRECT3D)                    { return D3D_OK; }
    HRESULT SetLight(LPVOID)                          { return D3D_OK; }
    HRESULT GetLight(LPVOID)                          { return D3D_OK; }
};

struct IDirect3DMaterial {
    virtual ~IDirect3DMaterial() {}
    HRESULT QueryInterface(REFIID, void**)            { return D3D_OK; }
    ULONG   AddRef()                                  { return 1; }
    ULONG   Release()                                 { return 0; }
    HRESULT Initialize(LPDIRECT3D)                    { return D3D_OK; }
    HRESULT SetMaterial(LPVOID)                       { return D3D_OK; }
    HRESULT GetMaterial(LPVOID)                       { return D3D_OK; }
    HRESULT GetHandle(LPDIRECT3DDEVICE, D3DMATERIALHANDLE* h) { if(h)*h=0; return D3D_OK; }
    HRESULT Reserve()                                 { return D3D_OK; }
    HRESULT Unreserve()                               { return D3D_OK; }
};

struct IDirect3DViewport {
    virtual ~IDirect3DViewport() {}
    HRESULT QueryInterface(REFIID, void**)            { return D3D_OK; }
    ULONG   AddRef()                                  { return 1; }
    ULONG   Release()                                 { return 0; }
    HRESULT Initialize(LPDIRECT3D) { ma_d3d_note("IDirect3DViewport::Initialize"); return D3D_OK; }
    HRESULT GetViewport(LPD3DVIEWPORT) { ma_d3d_note("IDirect3DViewport::GetViewport"); return D3D_OK; }
    HRESULT SetViewport(LPD3DVIEWPORT v) {
        ma_d3d_note("IDirect3DViewport::SetViewport");
        /* S115: the walk found 58% of world triangles sitting ABOVE the screen (y down to -480,
           exactly one screen height). What the game asks its viewport to be is the first place
           to look for the missing offset. */
        if (v && getenv("MA_D3D_EXEC")) { static int n=0; if (n++<4)
            fprintf(stderr, "[exec] SetViewport x=%lu y=%lu w=%lu h=%lu scale=(%.2f,%.2f) max=(%.2f,%.2f) z=%.3f..%.3f\n",
                (unsigned long)v->dwX,(unsigned long)v->dwY,(unsigned long)v->dwWidth,(unsigned long)v->dwHeight,
                (double)v->dvScaleX,(double)v->dvScaleY,(double)v->dvMaxX,(double)v->dvMaxY,
                (double)v->dvMinZ,(double)v->dvMaxZ); }
        return D3D_OK;
    }
    HRESULT TransformVertices(DWORD, LPVOID, DWORD, LPDWORD) { ma_d3d_note("IDirect3DViewport::TransformVertices"); return D3D_OK; }
    HRESULT LightElements(DWORD, LPVOID) { ma_d3d_note("IDirect3DViewport::LightElements"); return D3D_OK; }
    HRESULT SetBackground(D3DMATERIALHANDLE) { ma_d3d_note("IDirect3DViewport::SetBackground"); return D3D_OK; }
    HRESULT GetBackground(D3DMATERIALHANDLE*, BOOL*) { ma_d3d_note("IDirect3DViewport::GetBackground"); return D3D_OK; }
    HRESULT SetBackgroundDepth(LPDIRECTDRAWSURFACE) { ma_d3d_note("IDirect3DViewport::SetBackgroundDepth"); return D3D_OK; }
    HRESULT GetBackgroundDepth(LPDIRECTDRAWSURFACE*, BOOL*) { ma_d3d_note("IDirect3DViewport::GetBackgroundDepth"); return D3D_OK; }
    HRESULT Clear(DWORD, LPD3DRECT, DWORD) { ma_d3d_note("IDirect3DViewport::Clear"); return D3D_OK; }
    HRESULT AddLight(LPDIRECT3DLIGHT) { ma_d3d_note("IDirect3DViewport::AddLight"); return D3D_OK; }
    HRESULT DeleteLight(LPDIRECT3DLIGHT) { ma_d3d_note("IDirect3DViewport::DeleteLight"); return D3D_OK; }
    HRESULT NextLight(LPDIRECT3DLIGHT, LPDIRECT3DLIGHT*, DWORD) { ma_d3d_note("IDirect3DViewport::NextLight"); return D3D_OK; }
};

struct IDirect3DExecuteBuffer {
    /* S111 (PO-12 phase 1): a REAL buffer. This is the first stub the DX5/6 path cannot survive as
       a no-op: the game asks for a buffer of a given size, Locks it, writes its whole instruction
       stream and vertex array into `lpData`, Unlocks, records the extents with SetExecuteData and
       hands the buffer to IDirect3DDevice::Execute. With lpData NULL it wrote through a null
       pointer inside SetInitialRenderStatesLand (S110's measured crash).
       Ownership: the buffer owns its allocation for its lifetime; Lock hands back the same
       pointer every time (DirectDraw semantics for a system-memory execute buffer) and Unlock
       does not free it -- the game re-Locks the same buffer every frame. */
    unsigned char* mBuf;
    unsigned long  mSize;
    D3DEXECUTEDATA mData;
    int            mRef;

    IDirect3DExecuteBuffer() : mBuf(0), mSize(0), mRef(1) { memset(&mData, 0, sizeof(mData)); }
    virtual ~IDirect3DExecuteBuffer() { if (mBuf) { free(mBuf); mBuf = 0; } }

    HRESULT QueryInterface(REFIID, void**)            { return D3D_OK; }
    /* S115: real refcounting. The game creates ONE execute buffer PER FRAME (S113 measured 9140
       for 9114 frames) and releases it through WIN3D.H's RELEASE macro; with Release() a no-op
       that leaked a buffer and its allocation every frame -- survivable for a measurement run,
       not for a shipped option. */
    ULONG   AddRef()                                  { return (ULONG)++mRef; }
    ULONG   Release()                                 { if (--mRef > 0) return (ULONG)mRef; delete this; return 0; }
    HRESULT Initialize(LPDIRECT3DDEVICE, LPD3DEXECUTEBUFFERDESC d) {
        ma_d3d_note("IDirect3DExecuteBuffer::Initialize");
        if (d && (d->dwFlags & D3DDEB_BUFSIZE) && d->dwBufferSize) ma_alloc(d->dwBufferSize);
        return D3D_OK;
    }
    void ma_alloc(unsigned long n) {
        if (mBuf && mSize >= n) return;
        if (mBuf) free(mBuf);
        mSize = n;
        mBuf  = (unsigned char*)calloc(1, n ? n : 1);
    }
    HRESULT Lock(LPD3DEXECUTEBUFFERDESC d) {
        ma_d3d_note("IDirect3DExecuteBuffer::Lock");
        if (!d) return D3D_OK;
        if (!mBuf) ma_alloc((d->dwFlags & D3DDEB_BUFSIZE) && d->dwBufferSize ? d->dwBufferSize : 65536);
        d->dwBufferSize = mSize;
        d->lpData       = mBuf;
        d->dwFlags     |= (D3DDEB_BUFSIZE | D3DDEB_LPDATA);
        d->dwCaps       = D3DDEBCAPS_SYSTEMMEMORY;
        return D3D_OK;
    }
    HRESULT Unlock() { ma_d3d_note("IDirect3DExecuteBuffer::Unlock"); return D3D_OK; }
    HRESULT SetExecuteData(LPD3DEXECUTEDATA d) {
        ma_d3d_note("IDirect3DExecuteBuffer::SetExecuteData");
        if (d) mData = *d;
        return D3D_OK;
    }
    HRESULT GetExecuteData(LPD3DEXECUTEDATA d) {
        ma_d3d_note("IDirect3DExecuteBuffer::GetExecuteData");
        if (d) *d = mData;
        return D3D_OK;
    }
    HRESULT Validate(LPDWORD, LPVOID, LPVOID, DWORD) { ma_d3d_note("IDirect3DExecuteBuffer::Validate"); return D3D_OK; }
    HRESULT Optimize(DWORD) { ma_d3d_note("IDirect3DExecuteBuffer::Optimize"); return D3D_OK; }
};

struct IDirect3DDevice {
    virtual ~IDirect3DDevice() {}
    HRESULT QueryInterface(REFIID, void**)            { return D3D_OK; }
    ULONG   AddRef()                                  { return 1; }
    ULONG   Release()                                 { return 0; }
    HRESULT Initialize(LPDIRECT3D, GUID*, LPVOID) { ma_d3d_note("IDirect3DDevice::Initialize"); return D3D_OK; }
    HRESULT GetCaps(LPD3DDEVICEDESC, LPD3DDEVICEDESC) { ma_d3d_note("IDirect3DDevice::GetCaps"); return D3D_OK; }
    HRESULT SwapTextureHandles(LPDIRECT3DTEXTURE, LPDIRECT3DTEXTURE) { ma_d3d_note("IDirect3DDevice::SwapTextureHandles"); return D3D_OK; }
    HRESULT CreateExecuteBuffer(LPD3DEXECUTEBUFFERDESC d, LPDIRECT3DEXECUTEBUFFER* b, IUnknown*) {
        ma_d3d_note("IDirect3DDevice::CreateExecuteBuffer");
        if (!b) return D3D_OK;
        /* S111: hand back a real buffer instead of NULL (see IDirect3DExecuteBuffer). */
        IDirect3DExecuteBuffer* eb = new IDirect3DExecuteBuffer();
        eb->Initialize(0, d);
        *b = eb;
        return D3D_OK;
    }
    HRESULT GetStats(LPVOID) { ma_d3d_note("IDirect3DDevice::GetStats"); return D3D_OK; }
    /* S115 (PO-12 phase 3): this is where the hardware path finally draws. Phases 1-2 made the
       buffer and its textures real; until now Execute read none of it. The walk lives in
       compat/ma_d3d_exec.cpp -- out of line because it needs GL and this header is inlined into
       dozens of TUs. */
    HRESULT Execute(LPDIRECT3DEXECUTEBUFFER b, LPDIRECT3DVIEWPORT, DWORD) {
        ma_d3d_note("IDirect3DDevice::Execute");
        if (b) ma_d3d_exec_run(b->mBuf, b->mSize, &b->mData);
        return D3D_OK;
    }
    HRESULT AddViewport(LPDIRECT3DVIEWPORT) { ma_d3d_note("IDirect3DDevice::AddViewport"); return D3D_OK; }
    HRESULT DeleteViewport(LPDIRECT3DVIEWPORT) { ma_d3d_note("IDirect3DDevice::DeleteViewport"); return D3D_OK; }
    HRESULT NextViewport(LPDIRECT3DVIEWPORT, LPDIRECT3DVIEWPORT*, DWORD) { ma_d3d_note("IDirect3DDevice::NextViewport"); return D3D_OK; }
    HRESULT Pick(LPDIRECT3DEXECUTEBUFFER, LPDIRECT3DVIEWPORT, DWORD, LPD3DRECT) { ma_d3d_note("IDirect3DDevice::Pick"); return D3D_OK; }
    HRESULT GetPickRecords(LPDWORD, LPVOID) { ma_d3d_note("IDirect3DDevice::GetPickRecords"); return D3D_OK; }
    /* S110 (PO-12): report the two formats direct_3d::EnumTextureFormats looks for, or the game
       stops with "3D Hardware acceleration is not enabled" before any drawing is attempted:
         (1) 8-bit PALETTIZED  -- its opaque textures
         (2) 16-bit with alpha -- its transparent textures (it prefers 4-bit alpha, ARGB4444)
       Measured requirement, not a guess: those are the two loop conditions in Win3d.cpp. Both are
       trivially expressible in GL later (paletted via a shader or CPU expansion; ARGB4444 direct).
       Only under MA_TRY_HARDWARE while the path is being scoped. */
    HRESULT EnumTextureFormats(LPD3DENUMTEXTUREFORMATSCALLBACK cb, LPVOID ctx) {
        ma_d3d_note("IDirect3DDevice::EnumTextureFormats");
        if (cb && ma_hardware_available()) {
            DDSURFACEDESC sd;
            /* (1) 8-bit palettized */
            memset(&sd, 0, sizeof(sd));
            sd.dwSize = sizeof(sd);
            sd.dwFlags = DDSD_PIXELFORMAT | DDSD_CAPS;
            sd.ddpfPixelFormat.dwSize  = sizeof(DDPIXELFORMAT);
            sd.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED8;
            sd.ddpfPixelFormat.dwRGBBitCount = 8;
            cb(&sd, ctx);
            /* (2) ARGB4444 */
            memset(&sd, 0, sizeof(sd));
            sd.dwSize = sizeof(sd);
            sd.dwFlags = DDSD_PIXELFORMAT | DDSD_CAPS;
            sd.ddpfPixelFormat.dwSize  = sizeof(DDPIXELFORMAT);
            sd.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
            sd.ddpfPixelFormat.dwRGBBitCount   = 16;
            sd.ddpfPixelFormat.dwRBitMask      = 0x0F00;
            sd.ddpfPixelFormat.dwGBitMask      = 0x00F0;
            sd.ddpfPixelFormat.dwBBitMask      = 0x000F;
            sd.ddpfPixelFormat.dwRGBAlphaBitMask = 0xF000;
            cb(&sd, ctx);
        }
        return D3D_OK;
    }
    HRESULT CreateMatrix(D3DMATRIXHANDLE* h) { ma_d3d_note("IDirect3DDevice::CreateMatrix"); if(h)*h=0; return D3D_OK; }
    HRESULT SetMatrix(D3DMATRIXHANDLE, const D3DMATRIX*) { ma_d3d_note("IDirect3DDevice::SetMatrix"); return D3D_OK; }
    HRESULT GetMatrix(D3DMATRIXHANDLE, D3DMATRIX*) { ma_d3d_note("IDirect3DDevice::GetMatrix"); return D3D_OK; }
    HRESULT DeleteMatrix(D3DMATRIXHANDLE) { ma_d3d_note("IDirect3DDevice::DeleteMatrix"); return D3D_OK; }
    /* S115: start a hardware frame -- bind the context to this thread and clear colour+depth,
       so a frame's geometry does not accumulate on top of the last one's. */
    HRESULT BeginScene() { ma_d3d_note("IDirect3DDevice::BeginScene"); ma_gl_exec_begin(); return D3D_OK; }
    HRESULT EndScene() { ma_d3d_note("IDirect3DDevice::EndScene"); ma_gl_exec_end(); return D3D_OK; }
    HRESULT GetDirect3D(LPDIRECT3D*) { ma_d3d_note("IDirect3DDevice::GetDirect3D"); return D3D_OK; }
};

struct IDirect3D {
    virtual ~IDirect3D() {}
    HRESULT QueryInterface(REFIID, void**)            { return D3D_OK; }
    ULONG   AddRef()                                  { return 1; }
    ULONG   Release()                                 { return 0; }
    HRESULT Initialize(REFIID) { ma_d3d_note("IDirect3D::Initialize"); return D3D_OK; }
    /* S110 (PO-12): with MA_TRY_HARDWARE=1, report ONE hardware device so the rest of the DX5/6
       chain becomes reachable and can be measured. Without this the callback is never invoked,
       `bPrimaryDisplayDriverDoesHw3D` stays FALSE, DDRWINIT leaves DD.lpDirect3D NULL and
       Display::HardPoly returns FALSE on its first line -- so the census sees exactly one method
       and the work list stays invisible. The caps advertised are the ones Win3d's enumeration
       callback reads (colour model, raster caps, render bit depth); everything else stays zero
       until something is measured to need it. */
    HRESULT EnumDevices(LPD3DENUMDEVICESCALLBACK cb, LPVOID ctx) {
        ma_d3d_note("IDirect3D::EnumDevices");
        if (cb && ma_hardware_available()) {
            static D3DDEVICEDESC hw, hel;
            memset(&hw, 0, sizeof(hw)); memset(&hel, 0, sizeof(hel));
            hw.dwSize = sizeof(hw);
            hw.dcmColorModel = D3DCOLOR_RGB;
            hw.dwDeviceRenderBitDepth  = DDBD_16;
            hw.dwDeviceZBufferBitDepth = DDBD_16;
            hw.dwMaxBufferSize = 0;
            hw.dwMaxVertexCount = 65535;
            hw.dpcTriCaps.dwSize = sizeof(D3DPRIMCAPS);
            hw.dpcTriCaps.dwTextureCaps = D3DPTEXTURECAPS_PERSPECTIVE;
            hw.dpcTriCaps.dwTextureBlendCaps = D3DPTBLENDCAPS_MODULATEALPHA;
            hel = hw;
            static GUID g = {0x84E63DE0,0x46AA,0x11CF,{0x81,0x6F,0x00,0x00,0xC0,0x20,0x15,0x6E}};
            static char nm[] = "ma-gl", ds[] = "MiG Alley GL device (S110 measurement)";
            cb(&g, ds, nm, &hw, &hel, ctx);
        }
        return D3D_OK;
    }
    HRESULT CreateLight(LPDIRECT3DLIGHT* l, IUnknown*) { ma_d3d_note("IDirect3D::CreateLight"); if(l)*l=0; return D3D_OK; }
    HRESULT CreateMaterial(LPDIRECT3DMATERIAL* m, IUnknown*) { ma_d3d_note("IDirect3D::CreateMaterial"); if(m)*m=0; return D3D_OK; }
    HRESULT CreateViewport(LPDIRECT3DVIEWPORT* v, IUnknown*) { ma_d3d_note("IDirect3D::CreateViewport"); if(v)*v=0; return D3D_OK; }
    HRESULT FindDevice(LPVOID, LPVOID) { ma_d3d_note("IDirect3D::FindDevice"); return D3D_OK; }
};

/* ============================================================
 * S113 (PO-12 phase 2): the surfaces' QueryInterface, defined HERE because it has to hand back
 * IDirect3DTexture objects and this header is included after ddraw.h. The texture path is
 *     CreateSurface (DX1) -> QI IID_IDirectDrawSurface2 -> QI IID_IDirect3DTexture -> GetHandle
 *     -> PrepTexture Locks the DX2 face and writes texels
 * so all three faces must be views of ONE allocation. Ownership: the DX1 surface owns the pixels
 * and both views; the views borrow.
 * ============================================================ */
struct MaD3DTexture : public IDirect3DTexture { };   /* state now lives in the base (S115) */

/* S116: copy the texels from a system-memory texture into this one.
 *
 * This is how the engine actually fills a texture (Win3d.cpp:4412):
 *
 *     PrepTexture(lpDD2TSurf, ...)             // writes texels into the SYSTEM surface
 *     pVrt->lpd3dTexture->Unload();
 *     pVrt->lpd3dTexture->Load(lpD3DText);     // dest->Load(src): system -> video
 *
 * With Load a no-op the destination -- the surface the texture HANDLE names, and therefore the one
 * the renderer uploads to GL -- stayed exactly as allocated: all zero. The S116 measurement said
 * so plainly ("0/4096 non-zero texels" for every texture), which is a different failure from a
 * wrong pixel format and would have been easy to misread as one.
 */
inline HRESULT IDirect3DTexture::Load(LPDIRECT3DTEXTURE src)
{
    ma_d3d_note("IDirect3DTexture::Load");
    if (!src || !src->mSurf || !mSurf) return D3D_OK;
    IDirectDrawSurface* d = mSurf->sowner;
    IDirectDrawSurface* s = src->mSurf->sowner;
    if (!d || !s) return D3D_OK;
    s->salloc(); d->salloc();
    if (!d->sbits || !s->sbits) return D3D_OK;
    /* dimensions and format should match (the engine creates the pair from one description), but
       copy defensively: rows, clipped to the smaller of the two. */
    int h = d->sh < s->sh ? d->sh : s->sh;
    long row = d->spitch < s->spitch ? d->spitch : s->spitch;
    for (int y = 0; y < h; ++y)
        memcpy(d->sbits + (size_t)y * d->spitch, s->sbits + (size_t)y * s->spitch, (size_t)row);
    d->sdirty = 1;                      /* re-upload to GL on next bind */
    if (!d->spfSet && s->spfSet) {      /* inherit the format the texels were written in */
        d->smaskR = s->smaskR; d->smaskG = s->smaskG;
        d->smaskB = s->smaskB; d->smaskA = s->smaskA; d->spfSet = 1;
    }
    /* S119 (PO: "terrain is black but the huts and the landing strip on it are visible"):
       ALSO carry the palette across. 8-bit texels are indices and mean nothing without one, and
       the engine sets the palette on the SYSTEM surface it writes through --
           lpDD2TSurf->SetPalette(lpDDPalette[palIndex]);   (Win3d.cpp:4391)
           pVrt->lpd3dTexture->Load(lpD3DText);             (Win3d.cpp:4412)
       -- while the renderer binds the DESTINATION, whose palette this copy never touched. It
       then expanded every index through an all-zero fallback table: black. Copying the texels
       without their palette is only half a texture load. The art that DID look right is the art
       whose palette happened to be set on the destination surface directly. */
    if (s->spal) d->spal = s->spal;
    return D3D_OK;
}

inline HRESULT IDirectDrawSurface2::GetAttachedSurface(LPDDSCAPS caps, LPDIRECTDRAWSURFACE2* s)
{
    if (!s) return DD_OK;
    *s = 0;
    if (!sowner) return DDERR_NOTFOUND;
    LPDIRECTDRAWSURFACE b = 0;
    HRESULT hr = sowner->GetAttachedSurface(caps, &b);
    if (hr != DD_OK || !b) return (hr == DD_OK) ? DDERR_NOTFOUND : hr;
    void* v = 0;
    b->QueryInterface(IID_IDirectDrawSurface2, &v);
    *s = (LPDIRECTDRAWSURFACE2)v;
    return *s ? DD_OK : DDERR_NOTFOUND;
}

inline HRESULT IDirectDrawSurface2::QueryInterface(REFIID riid, void** p)
{
    if (!p) return DD_OK;
    *p = 0;
    if (IsEqualGUID(riid, IID_IDirect3DTexture)) {
        /* S131 (PO: "after a few passes all the objects turned white"): ONE texture object per
           surface. This minted a fresh MaD3DTexture with a fresh handle on every call, and the
           engine asks repeatedly -- so handles climbed without bound, walked past the registry's
           4096 entries, and every lookup after that returned "no surface". An unbound texture
           draws with the vertex colour, which for these models is white. It also leaked a texture
           object per call. The DX1 surface already had `stex` reserved for this; use it. */
        static int nocache = -1;
        if (nocache < 0) nocache = getenv("MA_NO_TEXCACHE") ? 1 : 0;   /* control arm */
        if (!nocache && sowner && sowner->stex) { *p = sowner->stex; return DD_OK; }
        static long s_next = 1;
        MaD3DTexture* t = new MaD3DTexture();
        t->mSurf   = this;
        t->mHandle = (D3DTEXTUREHANDLE)(s_next++);  /* non-zero: 0 means "no texture" to the game */
        if (sowner) sowner->stex = (void*)t;
        /* register the OWNER, not this view: the uploader needs the pixel format and the
           dirty flag, and only the DX1 surface carries them (S116). */
        ma_d3d_texture_register((unsigned long)t->mHandle, (void*)sowner);
        *p = (void*)t;
    }
    return DD_OK;
}

inline HRESULT IDirectDrawSurface::QueryInterface(REFIID riid, void** p)
{
    if (!p) return DD_OK;
    *p = 0;
    if (IsEqualGUID(riid, IID_IDirectDrawSurface2)) {
        if (!sview) {
            salloc();                               /* make sure the pixels exist first */
            sview = new IDirectDrawSurface2();
            sview->sbits = sbits; sview->sw = sw; sview->sh = sh;
            sview->sbpp = sbpp;   sview->spitch = spitch;
            sview->sowner = this;                   /* S116: for the texture uploader */
        }
        *p = (void*)sview;
        return DD_OK;
    }
    if (IsEqualGUID(riid, IID_IDirect3DTexture)) {
        if (!sview) { void* v = 0; QueryInterface(IID_IDirectDrawSurface2, &v); }
        if (sview) return sview->QueryInterface(riid, p);
        return DD_OK;
    }
    /* the remaining caller is direct_3d::CreateDevice asking the BACK surface for the 3D device
       with the driver's own GUID (S111). */
    if (ma_hardware_available()) *p = ma_d3d_device();
    return DD_OK;
}

#endif /* __cplusplus */

#endif /* FF_LINUX || MA_LINUX */
#endif /* MA_COMPAT_D3D_EXECBUF_H */
