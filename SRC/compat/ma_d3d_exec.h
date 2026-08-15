/* S115 (PO-12 phase 3): the render state carried from the execute-buffer walk to GL.
 *
 * Shared by SRC/compat/ma_d3d_exec.cpp (which walks the opcode stream and fills this in from
 * D3DOP_STATERENDER) and SRC/compat/bob_video.cpp (which owns the window/context and turns it
 * into GL state). One struct so the walk does not have to know GL and bob_video does not have to
 * know the legacy D3D render-state numbering.
 */
#ifndef MA_D3D_EXEC_H
#define MA_D3D_EXEC_H

/* S116: everything the GL uploader needs about one texture, so bob_video never has to know a
 * DirectDraw type and the walk never has to know GL. Filled by the walk from the surface the
 * texture handle names. */
struct MaTexDesc {
    const void*   bits;
    int           w, h, bpp;
    long          pitch;
    unsigned long mR, mG, mB, mA;  /* the masks the surface was CREATED with; 0 = 565 default */
    unsigned*     glTex;           /* the surface's GL texture name, created on first upload */
    int*          dirty;           /* set when the game rewrites the texels (surface Unlock) */
    const unsigned* pal;           /* 256 x 0x00RRGGBB for 8-bit texels; 0 if the surface has none */
    int*          alphaOnly;       /* S117: coverage-in-alpha texture -> colour comes from the vertex */
};

struct MaExecState {
    unsigned long texHandle;   /* D3DRENDERSTATE_TEXTUREHANDLE   (1)  -- 0 = untextured */
    int           blendEnable; /* D3DRENDERSTATE_BLENDENABLE    (27) */
    unsigned long srcBlend;    /* D3DRENDERSTATE_SRCBLEND       (19) */
    unsigned long dstBlend;    /* D3DRENDERSTATE_DESTBLEND      (20) */
    unsigned long texBlend;    /* D3DRENDERSTATE_TEXTUREMAPBLEND(21) */
    int           zEnable;     /* D3DRENDERSTATE_ZENABLE         (7) */
    int           zWrite;      /* D3DRENDERSTATE_ZWRITEENABLE   (14) */
    unsigned long zFunc;       /* D3DRENDERSTATE_ZFUNC          (23) */
    int           fogEnable;   /* D3DRENDERSTATE_FOGENABLE      (28) */
    const struct MaTexDesc* tex;   /* S116: resolved from texHandle; 0 = untextured */
    int           glyphBatch;      /* S117: every triangle in this batch is glyph-sized */
};

#ifdef __cplusplus
extern "C" {
#endif

/* Draw `nidx` indices of `nverts` D3DTLVERTEX (32-byte, pre-transformed screen space) under `st`.
 * `prim` is MA_EXEC_TRIS / _LINES / _POINTS -- the execute buffer carries all three (S117).
 * Implemented in bob_video.cpp; a no-op when there is no window (headless gates). */
#define MA_EXEC_TRIS   0
#define MA_EXEC_LINES  1
#define MA_EXEC_POINTS 2
void ma_gl_exec_prims(int prim, const void* verts, unsigned nverts,
                      const unsigned short* idx, unsigned nidx,
                      const struct MaExecState* st);

/* Walk one execute buffer. Called from IDirect3DDevice::Execute. */
void ma_d3d_exec_run(void* buf, unsigned long bufSize, const void* execData);

/* Census dump (MA_D3D_EXEC=1), called alongside ma_d3d_report at exit. */
void ma_d3d_exec_report(void);

/* What actually reached GL (bob_video's own counters), for the census. */
void ma_gl_exec_stats(long* tris, long* frames);

extern int ma_exec_land;   /* S119: inside a landscape Execute */

/* Start a hardware frame: bind the context, clear colour+depth. */
void ma_gl_exec_begin(void);

#ifdef __cplusplus
}
#endif

#endif /* MA_D3D_EXEC_H */
