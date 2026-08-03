# Sprint 72 — "Light up the 3D overlays" (investigation)

**Planned 2026-08-02 (PO pre-approved ceremonies). Autonomous, DoD: A/B parity evidence.**

## Context
The front-end 2D parity epic (EPIC I) closed in S69–S71. The frontier is 3D-view parity (I3),
whose two open shots are #10 (cockpit) and #11 (external), both with "black box" deviations the
parity doc had left vaguely as "palette/texture upload for overlay imagemaps."

## Sprint Goal
Ground the 3D-parity epic with a gl-lock A/B capture, precisely characterize the #10/#11 black
boxes against gold, and narrow the cockpit-black root cause enough to make the fix a scoped,
scheduleable next step (investigation with a fix stretch — S64 pattern).

## Execution log

### S72-1 Investigation — DONE (root cause NARROWED, not yet fixed)
Captured the current cockpit via `gl-lock` (`MA_ENABLE_3D=1 BOB_CLICKSEQ='40,r1;95,r0'
MA_DUMP_BACK=220`, GL-run per the parity doc). The GL run also confirmed **S69's per-face fonts
work in the GL path** (`[gdifont] loaded Intel.ttf` + `LiberationSans-Regular.ttf`).

**Precise A/B vs gold #10 (17-02-21):**
- **Cockpit frame + instrument panel render as a crisp FLAT-BLACK silhouette** where gold shows
  a fully textured metallic canopy arch + detailed panel (gauges, gunsight drum, EXHAUST/VOLTS,
  "RUDDER TRIM" light). The silhouette is *sharp and correct in shape* — so the geometry
  rasterizes fine; only the FILL is black.
- **A solid black rectangle top-right** (the padlock-ADI inset) that gold does not have there.
- The gunsight reflector glass + reticle render; the top-left **enemy-disk** padlock indicator
  (a port addition, default-on since S25) renders.

**Root cause narrowed (a real advance on "palette/texture upload"):**
1. The software rasterizer **HAS textured-span fillers** — `XASM_ImageHoriLine{1,2,4}`,
   `XASM_MImage*`, `XASM_SImage*`, `XASM_TFImage*`, `XASM_AImage*`, `XASM_CImage*` in
   `ma_xasm.nasm`'s scanline dispatch table. So the cockpit-black is **NOT a missing rasterizer
   primitive** (the S5 "DoHardPoly stubbed → black" note is about the *hardware* path; the
   software path this port forces has the image fillers).
2. **World terrain and the gunsight texture render fine**, so the textured pipeline works
   end-to-end for other content. The cockpit is a `COCKPIT_OBJECT` shape drawn by
   `btree::drw_cockpit` (`Btree.cpp:703`) through the same shape/poly path.
3. `textureQuality` (default 4/High) does **not** gate the cockpit to flat/black — ruled out.
4. ⇒ The cockpit shape's polygons rasterize but their **texture/imagemap data resolves to black
   (palette index 0)** — i.e. the **cockpit-specific imagemaps aren't providing pixel data**
   (not loaded / not bound / wrong palette) while the world/gunsight imagemaps are. That is the
   scoped fix target for the follow-up: trace the cockpit shape's per-poly imagemap binding
   (`Image_Map.GetImageMapPtr` for the cockpit set) and compare to a world poly that renders.

**Why the fix wasn't landed this sprint:** it is a deep 3D-render change (per-poly texture
binding on the shape path) that wants a focused session and iterative GL captures under low
display contention — forcing a speculative render change into a long autonomous run would risk
a wrong, parity-poisoning change. Closed honestly as an investigation with the fix scoped.

## Gates
No code changed (pure investigation) → build unchanged, no ASan/stress needed. The deliverable
is the A/B evidence (`port/scrum/screen-parity.md` #10/#11 updated) captured via `gl-lock`.

## Result: investigation delivered — #10/#11 characterized + cockpit-black root cause narrowed to
the cockpit imagemap binding. Fix scoped for a focused follow-up (S73).
