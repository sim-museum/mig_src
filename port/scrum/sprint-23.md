# Sprint 23 — F1 enemy-view/padlock crash: unclamped span SIGSEGV in the image filler

**Bug (user report):** Quick Mission → Turkey Shoot → F1 (padlock the bogie) → the window exits
(crash). Earlier sessions flagged "F1 enemy-view over-fills the cloud layer" — now root-caused.

## Root cause
The crash backtrace resolved to `XASM_ImageHoriLine1.loop2` (the textured/image horizontal-span
filler in `ma_xasm.nasm`) on a worker thread. The image filler writes `word [edi]` per column with
`edi = scanline_base + 2*sx`, **with no clipping** — it trusts the caller to pass an in-bounds span.

The poly/image scan converters (`cpoly/gpoly/ipoly/sipoly/wideipoly/widesipoly` in `Polygon.cpp`)
emit screen X in **0-based surface coords** (a full-width poly is `sx = 0..639`) and do **not** clamp
X, relying on the polygon being frustum-clipped. The F1 enemy-view/padlock re-aims the camera and
produces a textured cloud poly that extends off-screen horizontally (measured: spans with `rx` up to
639+ when the surface is 640 wide); the unclamped span makes the image filler run `edi` past the
scanline → out-of-bounds write → SIGSEGV. (The sphere/halo converters already clamp, in a separate
*centered* coordinate space, which is why only the image polys crashed.)

## Fix
`polygon::ASM_Call_clamp` (static member in `POLYGON.H`, so it inherits `polygon`'s friendship with
`Graphic` to read the protected `PhysicalWidth`): clamps the span to the 0-based surface range
`[0, PhysicalWidth-1]`, skips fully off-screen spans, and **fast-paths fully on-screen spans through
untouched** (no copy, DDA undisturbed). Applied to the 8 `ASM_Call(horilinertn,…)` sites in the
**0-based image/poly converters only** (line ranges `[2996,3381)` and `[3712,4994)`); the sphere/halo
converters (centered coords, already self-clamping) are deliberately left alone.

### A wrong first attempt, caught by measurement
The first version clamped to `[PhysicalMinX, PhysicalMinX+PhysicalWidth-1]` = `[-320, 319]`, assuming
`PhysicalMinX` described the filler's X range. It does **not** — that's the *centered* space used by
the sphere converters. A frame dump exposed it immediately: the **right third of the screen rendered
black** (every full-width poly clipped at x=320). Corrected to `[0, PhysicalWidth-1]` and re-verified
the frame renders full width. Lesson: the poly/image fillers are 0-based (`sx=0..width-1`); only the
sphere/halo path is centered.

## Validation
- **Full-width render restored:** frame-dump nonblack thirds `L=6275 M=6273 R=6164` (was `R=0` under
  the wrong bounds; matches a correct cockpit/sky/terrain frame).
- **3D regression:** `stress_launch.sh 5` → 5/5 reached & sustained 100 frames.
- **Enemy-view/padlock cycling** (inject ENEMYVIEW 0x42 ×4 + PADLOCK 0x52 + F1 0x3b over a Turkey
  Shoot flight) ×3 → 0 crashes. (The exact in-flight geometry that triggers a wild span is
  position-dependent and wasn't reliably reproducible headlessly; the fix is structural — the image
  filler can no longer receive an out-of-bounds span — so the OOB write is prevented regardless.)

## Note / likely bonus
A normal (no-view-change) Turkey Shoot flight was measured clamping ~1200 off-screen spans over 22 s
— i.e. the image converters were emitting out-of-bounds spans every frame, which pre-fix wrote past
their scanlines into adjacent memory. This was likely an intermittent corruptor in normal flight too
(cf. the Sprint 15–16 ASan heap-bug hunt), not only the F1 view; the clamp removes all of them.
