/* S115 (PO-12 phase 3): the render state carried from the execute-buffer walk to GL.
 *
 * Shared by SRC/compat/ma_d3d_exec.cpp (which walks the opcode stream and fills this in from
 * D3DOP_STATERENDER) and SRC/compat/bob_video.cpp (which owns the window/context and turns it
 * into GL state). One struct so the walk does not have to know GL and bob_video does not have to
 * know the legacy D3D render-state numbering.
 */
#ifndef MA_D3D_EXEC_H
#define MA_D3D_EXEC_H

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
};

#ifdef __cplusplus
extern "C" {
#endif

/* Draw `nidx` indices of `nverts` D3DTLVERTEX (32-byte, pre-transformed screen space) under `st`.
 * Implemented in bob_video.cpp; a no-op when there is no window (headless gates). */
void ma_gl_exec_tris(const void* verts, unsigned nverts,
                     const unsigned short* idx, unsigned nidx,
                     const struct MaExecState* st);

/* Walk one execute buffer. Called from IDirect3DDevice::Execute. */
void ma_d3d_exec_run(void* buf, unsigned long bufSize, const void* execData);

/* Census dump (MA_D3D_EXEC=1), called alongside ma_d3d_report at exit. */
void ma_d3d_exec_report(void);

/* What actually reached GL (bob_video's own counters), for the census. */
void ma_gl_exec_stats(long* tris, long* frames);

/* Start a hardware frame: bind the context, clear colour+depth. */
void ma_gl_exec_begin(void);

#ifdef __cplusplus
}
#endif

#endif /* MA_D3D_EXEC_H */
