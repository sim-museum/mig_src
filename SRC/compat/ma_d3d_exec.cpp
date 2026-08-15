/* S115 (PO-12, hardware graphics phase 3): walk the execute-buffer opcode stream.
 *
 * Phases 1 and 2 got the DX5/6 path to survive a whole mission -- a real execute buffer to write
 * into (S111) and real memory behind the texture surfaces (S113), 9114 BeginScene/EndScene cycles
 * with no crash. But `Execute` returned D3D_OK without reading a byte of what the game had just
 * written, so nothing was ever drawn. This is that read.
 *
 * What the game hands us (Win3d.cpp, e.g. DrawSunFlare / the terrain and object batches) is a
 * textbook DX5 execute buffer:
 *
 *     [ D3DTLVERTEX v0 .. vN-1 ]      at dwVertexOffset -- ALREADY TRANSFORMED to screen space
 *     [ instruction stream ]           at dwInstructionOffset, dwInstructionLength bytes
 *         D3DOP_STATERENDER    n x D3DSTATE       (texture handle, blend mode, fog, specular)
 *         D3DOP_PROCESSVERTICES 1 x D3DPROCESSVERTICES   with D3DPROCESSVERTICES_COPY
 *         D3DOP_TRIANGLE       n x D3DTRIANGLE    (three WORD indices into the vertex array)
 *         D3DOP_STATERENDER    ...
 *         D3DOP_EXIT
 *
 * Each instruction is a 4-byte D3DINSTRUCTION header followed by wCount operands of bSize bytes,
 * so the stream is walkable without understanding every opcode -- an unknown opcode is stepped
 * over, not a parse failure. That matters: the census below is the evidence that the stream is
 * being read correctly, and it has to keep working when the game emits something we don't handle.
 *
 * The vertices are PRE-TRANSFORMED (rhw set, screen x/y), which is why D3DOP_PROCESSVERTICES
 * always carries D3DPROCESSVERTICES_COPY here: there is no transform or lighting for us to do.
 * The triangles go straight to GL through an ortho projection in screen coordinates -- the same
 * treatment bob_video's DX7 path gives XYZRHW geometry.
 *
 * MA_D3D_EXEC=1   census: per-opcode counts, triangles, vertices, screen extents, render states.
 * MA_D3D_NODRAW=1 walk and count but submit nothing (the control arm for "did drawing change it").
 */
#include "windows.h"
#include "ddraw.h"
#include "d3d.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* GL submission lives in bob_video.cpp, which owns the window and the context. */
#include "ma_d3d_exec.h"

/* ---- texture handle -> surface -------------------------------------------------------------
 * The stream names textures by handle (D3DRENDERSTATE_TEXTUREHANDLE). Binding one means finding
 * the DirectDraw surface whose pixels it stands for; this is that map. Registration happens in
 * IDirectDrawSurface2::QueryInterface, where the texture object is minted.
 */
#define MA_MAX_TEXHANDLES 4096
static void* s_texSurf[MA_MAX_TEXHANDLES];
static unsigned long s_texCount;

extern "C" void ma_d3d_texture_register(unsigned long handle, void* surf2)
{
    if (handle < MA_MAX_TEXHANDLES) { s_texSurf[handle] = surf2; if (handle > s_texCount) s_texCount = handle; }
}
extern "C" void* ma_d3d_texture_surface(unsigned long handle)
{
    return (handle && handle < MA_MAX_TEXHANDLES) ? s_texSurf[handle] : 0;
}

/* ---- census ------------------------------------------------------------------------------- */
static long s_op[16];          /* per-opcode instruction count (index = opcode, 1..14) */
static long s_unknownOp;
static long s_tris, s_verts, s_batches, s_calls, s_emptyCalls, s_badCalls;
static float s_minx = 1e30f, s_maxx = -1e30f, s_miny = 1e30f, s_maxy = -1e30f;
static long s_stateSeen[64];   /* how often each render state (< 64) was set */
static long s_texturedTris, s_blendedTris;
static long s_degenerate, s_offscreen;
static long s_offAbove, s_offBelow, s_offLeft, s_offRight;
static double s_area;

static int exec_census(void)
{
    static int v = -1;
    if (v < 0) v = getenv("MA_D3D_EXEC") ? 1 : 0;
    return v;
}
static int exec_nodraw(void)
{
    static int v = -1;
    if (v < 0) v = getenv("MA_D3D_NODRAW") ? 1 : 0;
    return v;
}

extern "C" void ma_d3d_exec_report(void)
{
    if (!exec_census()) return;
    static const char* nm[15] = { "?", "POINT", "LINE", "TRIANGLE", "MATRIXLOAD", "MATRIXMULTIPLY",
        "STATETRANSFORM", "STATELIGHT", "STATERENDER", "PROCESSVERTICES", "TEXTURELOAD", "EXIT",
        "BRANCHFORWARD", "SPAN", "SETSTATUS" };
    fprintf(stderr, "[exec] ---- execute-buffer census ----\n");
    fprintf(stderr, "[exec] Execute calls %ld  (empty %ld, unusable %ld)\n",
            s_calls, s_emptyCalls, s_badCalls);
    for (int i = 1; i < 15; ++i)
        if (s_op[i]) fprintf(stderr, "[exec]   %-16s %ld\n", nm[i], s_op[i]);
    if (s_unknownOp) fprintf(stderr, "[exec]   (unknown opcodes) %ld\n", s_unknownOp);
    fprintf(stderr, "[exec] triangles %ld in %ld batches, vertices %ld\n", s_tris, s_batches, s_verts);
    fprintf(stderr, "[exec]   textured %ld, alpha-blended %ld\n", s_texturedTris, s_blendedTris);
    fprintf(stderr, "[exec]   zero-area %ld (%.1f%%), wholly off-screen %ld (%.1f%%), mean area %.2f px\n",
            s_degenerate, s_tris ? 100.0 * s_degenerate / s_tris : 0.0,
            s_offscreen,  s_tris ? 100.0 * s_offscreen  / s_tris : 0.0,
            s_tris ? s_area / s_tris : 0.0);
    fprintf(stderr, "[exec]   off-screen breakdown: above %ld, below %ld, left %ld, right %ld\n",
            s_offAbove, s_offBelow, s_offLeft, s_offRight);
    if (s_minx <= s_maxx)
        fprintf(stderr, "[exec] vertex screen extent x %.1f..%.1f  y %.1f..%.1f\n",
                s_minx, s_maxx, s_miny, s_maxy);
    fprintf(stderr, "[exec] render states set:");
    for (int i = 0; i < 64; ++i) if (s_stateSeen[i]) fprintf(stderr, " %d(x%ld)", i, s_stateSeen[i]);
    fprintf(stderr, "\n");
    long glTris = 0, glFrames = 0;
    ma_gl_exec_stats(&glTris, &glFrames);
    fprintf(stderr, "[exec] reached GL: %ld triangles over %ld scenes\n", glTris, glFrames);
}

/* ---- the walk ---------------------------------------------------------------------------- */

extern "C" void ma_d3d_exec_run(void* bufv, unsigned long bufSize, const void* datav)
{
    const D3DEXECUTEDATA* d = (const D3DEXECUTEDATA*)datav;
    unsigned char* buf = (unsigned char*)bufv;
    if (exec_census()) s_calls++;
    if (!buf || !d || !bufSize) { if (exec_census()) s_badCalls++; return; }
    if (!d->dwInstructionLength) { if (exec_census()) s_emptyCalls++; return; }

    /* Everything below is bounds-checked against the buffer the game actually allocated: this
       stream is written by game code we did not compile against a real driver, and a walk that
       trusts wCount would turn a stream bug into a wild read. */
    unsigned long insBeg = d->dwInstructionOffset;
    unsigned long insEnd = insBeg + d->dwInstructionLength;
    if (insEnd > bufSize) insEnd = bufSize;
    if (insBeg >= insEnd) { if (exec_census()) s_badCalls++; return; }

    const D3DTLVERTEX* verts = (const D3DTLVERTEX*)(buf + d->dwVertexOffset);
    unsigned long nverts = d->dwVertexCount;
    if (d->dwVertexOffset + (unsigned long)nverts * sizeof(D3DTLVERTEX) > bufSize)
        nverts = (bufSize - d->dwVertexOffset) / sizeof(D3DTLVERTEX);

    if (exec_census()) {
        s_verts += nverts;
        for (unsigned long i = 0; i < nverts; ++i) {
            float x = verts[i].sx, y = verts[i].sy;
            if (x < s_minx) s_minx = x;
            if (x > s_maxx) s_maxx = x;
            if (y < s_miny) s_miny = y;
            if (y > s_maxy) s_maxy = y;
        }
    }

    /* D3D's own defaults for the states the stream does not set. SRCBLEND/DESTBLEND matter:
       zeroing them would ask for blend factor 0, which is not a legal D3DBLEND value at all. */
    MaExecState st;
    memset(&st, 0, sizeof(st));
    st.zFunc    = 4 /* D3DCMP_LESSEQUAL */;
    st.zEnable  = 1; st.zWrite = 1;
    st.srcBlend = 2 /* D3DBLEND_ONE  */;
    st.dstBlend = 1 /* D3DBLEND_ZERO */;

    /* index scratch: three WORDs per triangle. A 1024-byte buffer cannot hold more than ~128
       triangles, but the batch buffers are larger, so size from what we are handed. */
    static unsigned short* idx = 0;
    static unsigned long   idxCap = 0;

    unsigned long p = insBeg;
    while (p + sizeof(D3DINSTRUCTION) <= insEnd) {
        const D3DINSTRUCTION* in = (const D3DINSTRUCTION*)(buf + p);
        unsigned op   = in->bOpcode;
        unsigned sz   = in->bSize;
        unsigned cnt  = in->wCount;
        unsigned long opnd = p + sizeof(D3DINSTRUCTION);
        unsigned long next = opnd + (unsigned long)sz * cnt;
        if (next > insEnd) break;                 /* truncated stream: stop, do not guess */
        if (exec_census()) { if (op < 15) s_op[op]++; else s_unknownOp++; }

        switch (op) {
        case D3DOP_EXIT:
            p = insEnd;
            continue;

        case D3DOP_STATERENDER: {
            for (unsigned i = 0; i < cnt; ++i) {
                const D3DSTATE* e = (const D3DSTATE*)(buf + opnd + (unsigned long)i * sz);
                unsigned type = (unsigned)e->drstRenderStateType;
                unsigned long arg = e->dwArg[0];
                if (exec_census() && type < 64) s_stateSeen[type]++;
                switch (type) {
                case 1:  st.texHandle = arg; break;   /* TEXTUREHANDLE  */
                case 7:  st.zEnable   = (int)arg; break;
                case 14: st.zWrite    = (int)arg; break;
                case 19: st.srcBlend  = arg; break;
                case 20: st.dstBlend  = arg; break;
                case 21: st.texBlend  = arg; break;
                case 23: st.zFunc     = arg; break;
                case 27: st.blendEnable = (int)arg; break;
                case 28: st.fogEnable = (int)arg; break;
                default: break;
                }
            }
            break;
        }

        case D3DOP_PROCESSVERTICES:
            /* D3DPROCESSVERTICES_COPY only, which is all this engine emits: the vertices are
               already in screen space with rhw set, so "processing" is the identity. */
            break;

        case D3DOP_TRIANGLE: {
            if (!cnt || sz < sizeof(D3DTRIANGLE)) break;
            if (idxCap < (unsigned long)cnt * 3) {
                idxCap = (unsigned long)cnt * 3 + 256;
                idx = (unsigned short*)realloc(idx, idxCap * sizeof(unsigned short));
            }
            unsigned nidx = 0;
            for (unsigned i = 0; i < cnt; ++i) {
                const D3DTRIANGLE* t = (const D3DTRIANGLE*)(buf + opnd + (unsigned long)i * sz);
                if (t->v1 >= nverts || t->v2 >= nverts || t->v3 >= nverts) continue;
                idx[nidx++] = t->v1; idx[nidx++] = t->v2; idx[nidx++] = t->v3;
            }
            if (exec_census()) {
                s_tris += nidx / 3; s_batches++;
                if (st.texHandle) s_texturedTris += nidx / 3;
                if (st.blendEnable) s_blendedTris += nidx / 3;
                /* A triangle that reaches GL and covers nothing is indistinguishable from one
                   that was never submitted. Separate the two: signed area, and whether the
                   triangle lies wholly outside the 640x480 screen. */
                for (unsigned k = 0; k + 2 < nidx; k += 3) {
                    const D3DTLVERTEX& a = verts[idx[k]];
                    const D3DTLVERTEX& b = verts[idx[k+1]];
                    const D3DTLVERTEX& c = verts[idx[k+2]];
                    float ar = (b.sx - a.sx) * (c.sy - a.sy) - (c.sx - a.sx) * (b.sy - a.sy);
                    if (ar < 0) ar = -ar;
                    ar *= 0.5f;
                    s_area += ar;
                    if (ar < 0.25f) s_degenerate++;
                    float mnx = a.sx < b.sx ? (a.sx < c.sx ? a.sx : c.sx) : (b.sx < c.sx ? b.sx : c.sx);
                    float mxx = a.sx > b.sx ? (a.sx > c.sx ? a.sx : c.sx) : (b.sx > c.sx ? b.sx : c.sx);
                    float mny = a.sy < b.sy ? (a.sy < c.sy ? a.sy : c.sy) : (b.sy < c.sy ? b.sy : c.sy);
                    float mxy = a.sy > b.sy ? (a.sy > c.sy ? a.sy : c.sy) : (b.sy > c.sy ? b.sy : c.sy);
                    if (mxx < 0 || mnx > 640 || mxy < 0 || mny > 480) {
                        s_offscreen++;
                        if (mxy < 0)   s_offAbove++;
                        if (mny > 480) s_offBelow++;
                        if (mxx < 0)   s_offLeft++;
                        if (mnx > 640) s_offRight++;
                    }
                }
            }
            if (nidx && !exec_nodraw()) ma_gl_exec_tris(verts, nverts, idx, nidx, &st);
            break;
        }

        default:
            break;   /* stepped over by `next` -- an opcode we do not implement is not an error */
        }
        p = next;
    }
}
