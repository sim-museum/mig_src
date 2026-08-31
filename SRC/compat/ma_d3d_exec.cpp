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
extern "C" void ma_force_texrelease_tick(void);   /* S328-S3: defined in 3DCOM.CPP */


/* PO-82 (S350): the create/destroy census. One counter pair per class, reported periodically and
   at exit, so a leak fix can be judged by the same number before and after. Deliberately reports
   even when nothing has leaked -- a census that only appears on failure cannot prove it was
   running, which is the mistake this port has made repeatedly (S131's gated warning, PO-82's
   silent registry drop, R12's silent skips). */
static const char* g_lfNames[8]; static long g_lfMade[8], g_lfFreed[8]; static int g_lfN = 0;
static void ma_lifetime_dump(const char* where)
{
    if (!getenv("MA_TRACE_LIFETIME")) return;
    fprintf(stderr, "[lifetime] %s:", where);
    if (g_lfN == 0) fprintf(stderr, "   <-- NO EVENTS: this census proves nothing");
    for (int k = 0; k < g_lfN; k++)
        fprintf(stderr, " %s made=%ld freed=%ld live=%ld |",
                g_lfNames[k], g_lfMade[k], g_lfFreed[k], g_lfMade[k]-g_lfFreed[k]);
    fprintf(stderr, "\n"); fflush(stderr);
}
static void ma_lifetime_atexit(void) { ma_lifetime_dump("FINAL"); }
extern "C" void ma_lifetime_report(void);
extern "C" void ma_lifetime_note(const char* cls, int made)
{
    static int on = -1;
    if (on < 0) { on = getenv("MA_TRACE_LIFETIME") ? 1 : 0; if (on) atexit(ma_lifetime_atexit); }
    if (!on) return;
    const char** names = g_lfNames; long* madeN = g_lfMade; long* freeN = g_lfFreed; int& n = g_lfN;
    int i = 0;
    for (; i < n; i++) if (names[i] == cls || (names[i] && cls && !strcmp(names[i], cls))) break;
    if (i == n) { if (n >= 8) return; names[n] = cls; madeN[n] = freeN[n] = 0; n++; }
    if (made) madeN[i]++; else freeN[i]++;
    /* S350: report every 2000 events AND at exit. The first cut used 20000 and printed NOTHING --
       a whole run makes fewer events than that, so the instrument was silent and looked like
       "no surfaces were created", which is the exact failure this census exists to expose. */
    static long ticks = 0;
    if ((++ticks % 2000) == 0) ma_lifetime_report();
}
extern "C" void ma_lifetime_report(void)
{
    ma_lifetime_dump("periodic");
}


/* PO-82 (S358): AddRef/Release census. Counts only -- nothing is freed. */
static void ma_ref_dump(const char* where, const char** names, long* addN, long* relN, int n);
static const char* g_refNames[8]; static long g_refAdd[8], g_refRel[8]; static int g_refN = 0;
static void ma_ref_atexit(void) { ma_ref_dump("FINAL", g_refNames, g_refAdd, g_refRel, g_refN); }
/* S358: register the exit report AT STARTUP, not on first call. The previous version registered it
   inside ma_ref_note -- the very function whose silence was the question -- so "the game never calls
   AddRef/Release" and "the env was not seen" produced identical empty logs. A census must be able to
   report that it counted NOTHING; that is a finding, not an absence of one. */
static struct MaRefCensusInit { MaRefCensusInit() {
    if (getenv("MA_TRACE_REFS")) atexit(ma_ref_atexit);
} } g_maRefCensusInit;
extern "C" void ma_ref_note(const char* cls, int add)
{
    const char** names = g_refNames; long* addN = g_refAdd; long* relN = g_refRel; int& n = g_refN;
    static int on = -1;
    if (on < 0) on = getenv("MA_TRACE_REFS") ? 1 : 0;
    if (!on) return;
    int i = 0;
    for (; i < n; i++) if (names[i] == cls || (names[i] && cls && !strcmp(names[i], cls))) break;
    if (i == n) { if (n >= 8) return; names[n] = cls; addN[n] = relN[n] = 0; n++; }
    if (add) addN[i]++; else relN[i]++;
    /* S358: report every 100 AND at exit. The first cut used 2000 and printed NOTHING -- which was
       itself the answer (the game barely calls these at all) but arrived as silence, and silence
       here is indistinguishable from "the hook never ran". Same mistake the lifetime census made at
       20000. An instrument that cannot speak on a short run is not an instrument. */
    /* S358: announce the FIRST call as well as every hundredth. The exit report cannot be relied on
       here -- the harness kills wmig with SIGKILL, which never runs atexit handlers -- so without
       this, "called twice" and "never called" both produce an empty log, and I spent two runs
       unable to tell them apart. */
    static long ticks = 0;
    ++ticks;
    if (ticks == 1 || (ticks % 100) == 0) ma_ref_dump(ticks == 1 ? "first call" : "periodic", names, addN, relN, n);
}
static void ma_ref_dump(const char* where, const char** names, long* addN, long* relN, int n)
{
    fprintf(stderr, "[refs] %s:", where);
    if (n == 0) fprintf(stderr, "   <-- NO AddRef/Release CALLS AT ALL");
    for (int k = 0; k < n; k++)
        fprintf(stderr, " %s addref=%ld release=%ld net=%+ld |", names[k], addN[k], relN[k], addN[k]-relN[k]);
    fprintf(stderr, "\n"); fflush(stderr);
}

/* ---- texture handle -> surface -------------------------------------------------------------
 * The stream names textures by handle (D3DRENDERSTATE_TEXTUREHANDLE). Binding one means finding
 * the DirectDraw surface whose pixels it stands for; this is that map. Registration happens in
 * IDirectDrawSurface2::QueryInterface, where the texture object is minted.
 */
/* PO-82 (S348) — ROOT CAUSE, MEASURED. This registry was a FIXED 4096-ENTRY ARRAY, and handles
   are minted by a monotonic `s_next++` (d3d_execbuf.h), one per texture QueryInterface. Once the
   counter passes 4096 every further texture is DROPPED here -- and ma_tex_desc then cannot resolve
   it, so the draw goes out untextured, in its vertex colour. A white object is an untextured one,
   which is exactly the PO's report (2026-08-29): impact flashes as flat white blocks, some YELLOW
   (yellow vertex colour, no texture), smoke as an opaque white ribbon, an explosion as a flat
   white quad -- while correctly textured objects render in the SAME frame, because THEIR handles
   were minted below the ceiling.
   Measured in one ordinary sortie, no white textures visible to the tester:
       resolved=2035284  FAILED=820000  (unregistered=820000)   -- 29% of all lookups
       31 distinct failing handles, every one in 6000..7765, ALL ABOVE 4096
   S131 wrote the prediction next to this code -- "handles used to climb without bound; if this
   ever fires again the cache has been defeated" -- and made the warning conditional on
   MA_TRACE_TEX, so the failure stayed silent for anyone not already looking for it.
   A map has no ceiling, so the class of bug is removed rather than moved: raising 4096 to some
   larger number would only postpone it to a longer sortie.
   NOTE the cache defeat itself is a SEPARATE issue and is NOT fixed here: handles should be
   reused via `sowner->stex` and something is minting new ones instead. That wastes registry
   entries and upload work even with an unbounded map, and it deserves its own item. */
#include <unordered_map>
static std::unordered_map<unsigned long, void*>& texmap()
{
    static std::unordered_map<unsigned long, void*> m; return m;
}
static unsigned long s_texCount;
static unsigned long s_texMaxHandle;

extern "C" void ma_d3d_texture_register(unsigned long handle, void* surf2)
{
    if (!handle) return;                       /* 0 means "no texture" to the game */
    texmap()[handle] = surf2;
    if (handle > s_texCount)     s_texCount = handle;
    if (handle > s_texMaxHandle) s_texMaxHandle = handle;
}
extern "C" void* ma_d3d_texture_surface(unsigned long handle)
{
    if (!handle) return 0;
    std::unordered_map<unsigned long, void*>::iterator it = texmap().find(handle);
    return it == texmap().end() ? 0 : it->second;
}
/* S328-S3 (PO-82) — THE DEFECT. This registry was WRITE-ONLY: ma_d3d_texture_register filled a
   slot and NOTHING ever cleared it. `sbits` is freed only in ~IDirectDrawSurface
   (ddraw_legacy.h:139), which destroys the whole object -- so a destroyed texture surface left
   s_texSurf[handle] holding a DANGLING POINTER, and ma_tex_desc then dereferenced freed memory to
   read s->sbits / s->sw / s->sh.
   That is why the S328 instrument reported "FAILED 0" through a forced DoReleaseTextures(): the
   `!s->sbits` guard reads freed memory, which usually still contains the old non-null pointer, so
   the check PASSES and hands the GL uploader a descriptor pointing at freed pixels. The three
   failure causes it counts never included the real one.
   It also fits every property of the PO's report: intermittent (depends on whether the freed block
   has been reused), follows a graphics-preference change (which destroys surfaces), and shows up
   seconds later rather than instantly.
   Clear every slot that names a dying surface. O(4096) once per destruction, off the hot path. */
/* ---- PO-82-leak (S369): SESSION OWNERSHIP of texture surfaces ------------------------------
   S358 established the leak is ABANDONMENT: the game creates ~4000 textures a sortie (two
   objects each -- surface + Surface2 twin) and releases none, and refcounting cannot help
   because the game makes ONE ref call per sortie. So the port owns them instead: every surface
   created with DDSCAPS_TEXTURE belongs to the 3D session that created it, and is freed when
   that session tears down (MIG.CPP, immediately after Launch3d returns).

   Only DDSCAPS_TEXTURE is tracked. The primary and back buffers come through the same
   CreateSurface and MUST outlive a flight -- freeing those is a black screen, not a leak fix.

   Behind MA_FREE_TEX_SURFACES=1 until measured: this is the change S349/S350 declined to make
   blind, and the failure mode of getting it wrong is a use-after-free rather than a wasted
   megabyte. The S350 census (made vs freed) is the measurement; the gate suite is the check. */
#include <vector>
static std::vector<void*>& surfsession() { static std::vector<void*> v; return v; }

extern "C" void ma_surf_track_texture(void* surf)
{
    if (surf) surfsession().push_back(surf);
}

extern "C" int ma_surf_free_session(void)
{
    std::vector<void*>& v = surfsession();
    int n = 0;
    for (size_t i = 0; i < v.size(); ++i) {
        /* ~IDirectDrawSurface disposes of the twin (sfreeview) and the twin's destructor clears
           the S348 handle registry, so a swept surface leaves nothing pointing at it. */
        delete (IDirectDrawSurface*)v[i];
        ++n;
    }
    v.clear();
    if (getenv("MA_TRACE_LIFETIME") || getenv("MA_TRACE_TEXFAIL"))
        fprintf(stderr, "[surfsweep] freed %d texture surface(s) at 3D teardown\n", n);
    return n;
}

extern "C" void ma_d3d_texture_forget(void* surf2)
{
    if (!surf2) return;
    /* S348: walk the map, not a fixed 4096 span -- the array is gone. Erase while iterating is
       done with the post-increment idiom so the erased node's iterator is never reused. */
    for (std::unordered_map<unsigned long, void*>::iterator it = texmap().begin(); it != texmap().end(); ) {
        if (it->second == surf2) {
            unsigned long h = it->first;
            texmap().erase(it++);
            if (getenv("MA_TRACE_TEXFAIL"))
                fprintf(stderr, "[texfail] handle %lu unregistered (surface destroyed)\n", h);
        } else ++it;
    }
}

/* Resolve a handle to the description the GL uploader wants. Returns 0 for an unknown handle or a
   surface with no pixels yet -- both mean "draw this untextured" rather than "fail". */
/* S328 (PO-82, PO 2026-08-29 "30 seconds into dogfight lose object texture"): this function had
   ONE silent `return 0` covering three unrelated failures, and bob_video.cpp turns a 0 into
   glDisable(GL_TEXTURE_2D) -- the draw then shows its vertex colour, which is the white (and
   yellow) the PO photographed. S131 hit the same wall and could not reproduce it.
   Separate the causes, because they have DIFFERENT fixes:
     (a) handle never registered      -> the mint/QueryInterface path
     (b) registered but sbits == NULL -> the surface was released under a live handle
                                         (ReleaseTextures() is called from SetTextureQuality(),
                                          i.e. the PO's graphics-preference change)
     (c) degenerate dimensions        -> the upload path
   COUNT SUCCESSES TOO. A zero here must be distinguishable from "this function never ran" -- the
   recurring failure in this project is an instrument that is silent because it is not reached, read
   as evidence that nothing is wrong. MA_TRACE_TEXFAIL=1 reports; it also names the first few
   offending handles, since "which texture" is what picks the fix. */
static long s_texOK = 0, s_texUnreg = 0, s_texNoBits = 0, s_texBadDim = 0;
extern "C" void ma_tex_fail_report(const char* where)
{
    if (!getenv("MA_TRACE_TEXFAIL")) return;
    const long bad = s_texUnreg + s_texNoBits + s_texBadDim;
    fprintf(stderr, "[texfail] %s: resolved=%ld  FAILED=%ld  (unregistered=%ld  released/no-pixels=%ld  bad-dims=%ld)%s\n",
            where ? where : "?", s_texOK, bad, s_texUnreg, s_texNoBits, s_texBadDim,
            (s_texOK == 0 && bad == 0) ? "   <-- NEVER CALLED: this instrument proves nothing" : "");
    fflush(stderr);
}
/* S328b: the periodic report fires every 20000 calls, so the LAST partial block was never
   reported -- the first run ended showing "40000" when the true total was somewhere in
   40000..60000. A counter that only speaks on round numbers understates by up to a full period,
   and the interesting case (a late collapse) lives exactly in that unreported tail. Report once
   more at exit, which is also the only place the FINAL split can be read. */
static void ma_tex_fail_atexit(void) { ma_tex_fail_report("FINAL"); }
static const MaTexDesc* ma_tex_desc(unsigned long handle)
{
    if (getenv("MA_TRACE_TEXFAIL")) {
        static int reg = 0;
        if (!reg) { reg = 1; atexit(ma_tex_fail_atexit); }
    }
    /* S328-S3: drive the suspected trigger from here, where texture handles are actually being
       resolved, so "before" and "after" are separated by the release and nothing else. */
        ma_force_texrelease_tick();
    IDirectDrawSurface* s = (IDirectDrawSurface*)ma_d3d_texture_surface(handle);
    if (!s || !s->sbits || s->sw <= 0 || s->sh <= 0) {
        if (!s)                              s_texUnreg++;
        else if (!s->sbits)                  s_texNoBits++;
        else                                 s_texBadDim++;
        if (getenv("MA_TRACE_TEXFAIL")) {
            /* PO-82 (S348): report DISTINCT handles, not the first twelve hits.
               The first cut printed the first 12 failures and they were all handle 6081, which
               says nothing about whether the 820k failures are one texture or a thousand -- and
               those have completely different fixes (one missing registration versus a broken
               registration path). Dedupe, and count how many distinct handles are involved. */
            static unsigned long seen[64]; static int nSeen = 0; static long distinctOver = 0;
            int already = 0;
            for (int i = 0; i < nSeen; i++) if (seen[i] == handle) { already = 1; break; }
            if (!already) {
                if (nSeen < 64) {
                    seen[nSeen++] = handle;
                    fprintf(stderr, "[texfail] DISTINCT handle %lu -> %s\n", handle,
                            !s ? "NOT REGISTERED" : (!s->sbits ? "registered but sbits==NULL (surface released?)"
                                                               : "degenerate dimensions"));
                    fflush(stderr);
                } else if ((++distinctOver % 64) == 1) {
                    fprintf(stderr, "[texfail] more than 64 distinct failing handles (%ld beyond the cap)\n",
                            distinctOver); fflush(stderr);
                }
            }
            /* periodic, so a mid-flight collapse is visible without waiting for exit */
            static long n = 0;
            if ((++n % 20000) == 0) ma_tex_fail_report("periodic");
        }
        return 0;
    }
    s_texOK++;
    /* S328: report from the SUCCESS path too, so a healthy run still proves the instrument ran.
       Otherwise "no failures reported" and "never called" look identical, which is the exact
       misreading this whole item exists to avoid. */
    if (getenv("MA_TRACE_TEXFAIL")) {
        /* S363: report every 2000 resolves, and the FIRST one. 20000 was too coarse: a short Hot
           Shot sortie does not reach it, so a HEALTHY run produced no output at all and the gate
           built on this could not tell "zero failures" from "never ran" -- it returned INCONCLUSIVE
           three times while the code was fine. The atexit FINAL report does not rescue it either,
           because the harness terminates the process without running atexit handlers.
           An instrument that only speaks on long runs cannot certify short ones. */
        static long nOK = 0; ++nOK;
        if (nOK == 1 || (nOK % 2000) == 0) ma_tex_fail_report("periodic");
    }
    static MaTexDesc d;
    d.bits = s->sbits; d.w = s->sw; d.h = s->sh; d.bpp = s->sbpp; d.pitch = s->spitch;
    d.mR = s->smaskR; d.mG = s->smaskG; d.mB = s->smaskB; d.mA = s->smaskA;
    d.glTex = &s->sglTex; d.dirty = &s->sdirty;
    d.pal   = ma_dd_palette_rgb(s->spal);
    d.alphaOnly = &s->salphaonly;
    return &d;
}

extern "C" int ma_exec_land = 0;   /* S119: set while the landscape buffer is being executed */

/* ---- census ------------------------------------------------------------------------------- */
static long s_op[16];          /* per-opcode instruction count (index = opcode, 1..14) */
static long s_unknownOp;
static long s_tris, s_verts, s_batches, s_calls, s_emptyCalls, s_badCalls;
static float s_minx = 1e30f, s_maxx = -1e30f, s_miny = 1e30f, s_maxy = -1e30f;
static long s_stateSeen[64];   /* how often each render state (< 64) was set */
static long s_texturedTris, s_blendedTris;
static long s_degenerate, s_offscreen;
static long s_lines, s_points;
static long s_glyphTris;
static float s_glyphMinY=1e30f, s_glyphMaxY=-1e30f, s_glyphMinX=1e30f, s_glyphMaxX=-1e30f;
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
    fprintf(stderr, "[exec]   lines %ld, points %ld\n", s_lines, s_points);
    if (s_glyphTris)
        fprintf(stderr, "[exec]   small alpha-textured (glyph-sized) tris %ld, x %.0f..%.0f y %.0f..%.0f\n",
                s_glyphTris, s_glyphMinX, s_glyphMaxX, s_glyphMinY, s_glyphMaxY);
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
    /* S348: the registry is unbounded now, so report what it actually holds rather than a
       ceiling that no longer exists. The old line printed "(registry holds 4096)" next to a
       highest-handle figure well above it, which was the defect stated plainly in the log and
       read past for several sessions. */
    fprintf(stderr, "[exec] highest texture handle issued: %lu (registry holds %lu entries, no ceiling)\n",
            s_texCount, (unsigned long)texmap().size());
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

    if (exec_census() && ma_exec_land) {
        static int n = 0;
        if (n++ < 4)
            fprintf(stderr, "[exec] LAND execute: nverts=%lu instLen=%lu vtxOff=%lu  v0=(%.1f,%.1f,%.4f) argb=%08x\n",
                    nverts, (unsigned long)d->dwInstructionLength, (unsigned long)d->dwVertexOffset,
                    nverts ? verts[0].sx : 0.f, nverts ? verts[0].sy : 0.f, nverts ? verts[0].sz : 0.f,
                    nverts ? *(const unsigned*)&verts[0].color : 0u);
    }
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

    /* S117: render state is PERSISTENT ACROSS EXECUTE BUFFERS, exactly as on a real device.
       This was a per-call local, reset to invented defaults every Execute -- and the engine sets
       D3DRENDERSTATE_ZENABLE exactly ONCE in a whole flight (the census: state 7, x1), because it
       expects the device to remember it. Re-asserting `zEnable = 1` on every buffer therefore
       turned depth testing back on behind the engine's back, and the overlay batches -- which are
       drawn last and are meant to sit on top -- were depth-rejected. That cost the info line and
       the lower cockpit coaming, which looked like two unrelated missing features.

       The initial values are D3D's own defaults, applied once. SRCBLEND/DESTBLEND matter: zeroing
       them would ask for blend factor 0, which is not a legal D3DBLEND value at all. */
    static MaExecState st = { 0, 0, 2 /*D3DBLEND_ONE*/, 1 /*D3DBLEND_ZERO*/, 0,
                              0 /*zEnable: off until the game asks*/, 1 /*zWrite*/,
                              4 /*D3DCMP_LESSEQUAL*/, 0, 0, 0 };

    /* index scratch: three WORDs per triangle. A 1024-byte buffer cannot hold more than ~128
       triangles, but the batch buffers are larger, so size from what we are handed. */
    static unsigned short* idx = 0;
    static unsigned long   idxCap = 0;
    struct Scratch {   /* grow the index scratch; false means the allocation failed */
        static bool grow(unsigned short*& p, unsigned long& cap, unsigned long need) {
            if (cap >= need) return p != 0;
            unsigned long n = need + 256;
            unsigned short* q = (unsigned short*)realloc(p, n * sizeof(unsigned short));
            if (!q) return false;
            p = q; cap = n; return true;
        }
    };
#define ensure_idx(n) Scratch::grow(idx, idxCap, (n))

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
                /* the depth states are set once or twice for a whole flight, so their VALUES are
                   what decide whether the overlay survives. Print them. */
                if (exec_census() && (type == 7 || type == 23)) {
                    static int n = 0;
                    if (n++ < 8) fprintf(stderr, "[exec] render state %u = %lu %s\n", type, arg,
                        type == 7 ? "(ZENABLE)" : "(ZFUNC)");
                }
                switch (type) {
                case 1:  st.texHandle = arg; st.tex = arg ? ma_tex_desc(arg) : 0; break;
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
            if (!ensure_idx((unsigned long)cnt * 3)) break;
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
                    /* S117: glyph quads. direct_3d::PutC builds one small alpha-textured quad
                       per character (~67 per scene, which is an info line's worth), so where the
                       SMALL alpha-textured triangles land tells us where the text went. */
                    if (st.tex && ar < 400.0f && st.texHandle) {
                        s_glyphTris++;
                        if (mny < s_glyphMinY) s_glyphMinY = mny;
                        if (mxy > s_glyphMaxY) s_glyphMaxY = mxy;
                        if (mnx < s_glyphMinX) s_glyphMinX = mnx;
                        if (mxx > s_glyphMaxX) s_glyphMaxX = mxx;
                    }
                    if (mxx < 0 || mnx > 640 || mxy < 0 || mny > 480) {
                        s_offscreen++;
                        if (mxy < 0)   s_offAbove++;
                        if (mny > 480) s_offBelow++;
                        if (mxx < 0)   s_offLeft++;
                        if (mnx > 640) s_offRight++;
                    }
                }
            }
            /* S117: flag the glyph batches (small, alpha-textured quads from direct_3d::PutC) so
               the renderer can be asked about them specifically. */
            st.glyphBatch = 0;
            if (st.tex && nidx == 6) {
                const D3DTLVERTEX& a = verts[idx[0]];
                const D3DTLVERTEX& b = verts[idx[1]];
                const D3DTLVERTEX& c = verts[idx[2]];
                float ar = (b.sx-a.sx)*(c.sy-a.sy) - (c.sx-a.sx)*(b.sy-a.sy);
                if (ar < 0) ar = -ar;
                if (ar * 0.5f < 400.0f) st.glyphBatch = 1;
            }
            if (nidx && !exec_nodraw()) ma_gl_exec_prims(MA_EXEC_TRIS, verts, nverts, idx, nidx, &st);
            break;
        }

        /* S117: the stream carries lines and points as well as triangles -- 76224 LINE and 5329
           POINT instructions in one flight, all of them stepped over until now. They are the
           engine's thin geometry: wires, tracer/point sprites and the cockpit's line work. */
        case D3DOP_LINE: {
            if (!cnt || sz < sizeof(D3DLINE)) break;
            if (!ensure_idx((unsigned long)cnt * 2)) break;
            unsigned nidx = 0;
            for (unsigned i = 0; i < cnt; ++i) {
                const D3DLINE* l = (const D3DLINE*)(buf + opnd + (unsigned long)i * sz);
                if (l->v1 >= nverts || l->v2 >= nverts) continue;
                idx[nidx++] = l->v1; idx[nidx++] = l->v2;
            }
            if (exec_census()) s_lines += nidx / 2;
            if (nidx && !exec_nodraw()) ma_gl_exec_prims(MA_EXEC_LINES, verts, nverts, idx, nidx, &st);
            break;
        }

        case D3DOP_POINT: {
            /* one D3DPOINT operand describes a RUN: wCount vertices starting at wFirst. */
            if (!cnt || sz < sizeof(D3DPOINT)) break;
            for (unsigned i = 0; i < cnt; ++i) {
                const D3DPOINT* pt = (const D3DPOINT*)(buf + opnd + (unsigned long)i * sz);
                unsigned first = pt->wFirst, run = pt->wCount;
                if (first >= nverts) continue;
                if (first + run > nverts) run = (unsigned)(nverts - first);
                if (!run || !ensure_idx(run)) continue;
                for (unsigned k = 0; k < run; ++k) idx[k] = (unsigned short)(first + k);
                if (exec_census()) s_points += run;
                if (!exec_nodraw()) ma_gl_exec_prims(MA_EXEC_POINTS, verts, nverts, idx, run, &st);
            }
            break;
        }

        default:
            break;   /* stepped over by `next` -- an opcode we do not implement is not an error */
        }
        p = next;
    }
#undef ensure_idx
}
