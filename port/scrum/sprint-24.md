# Sprint 24 — F1 padlock crash, part 2: it was a VERTICAL OOB (off-surface scanline)

**Bug:** Turkey Shoot → F1 padlock still crashed after Sprint 23's horizontal-span clamp.
Same backtrace (`XASM_ImageHoriLine1.loop2`).

## How it was finally caught: instrument the crash, don't guess
I could not reproduce the crash headlessly with injected view keys (the padlock view renders fine
when the bogie is centered; the user's crash needs extreme camera/scene geometry from active
maneuvering). Two moves cracked it:

1. **Enhanced the crash handler** (`bob_main.cpp`) to `SA_SIGINFO` — it now prints `fault_addr` +
   the full i386 register file. For an XASM span-filler fault you can then read off *which* access
   died.
2. **Added `MA_FORCE_PADLOCK`** (`Viewsel.cpp`, gated) to force the ENEMYVIEW command headlessly,
   and combined it with `BOB_AUTOFLY=sweep` (wild stick input) → reproduced a SIGSEGV.

The dump was decisive:
```
=== CRASH: signal 11  fault_addr=0xc46fd410 ===
  eip=081d33f7 ... ecx=007f0000 edx=004e0000 esi=c3cbebf0 edi=c46fd410 ...
```
`fault_addr == edi` → it's the **destination write** `mov word [edi],bx`, NOT the texture read
(`esi+ebx`=0xc3cc8b8e). Since Sprint 23 already clamps the span's screen X to `[0,639]`, `edi` can
only be wild if **`scradr` (the scanline base) is off-surface**: `edi ≈ scradr`, and
`(0xc46fd410 − surface)/pitch ≈ 83000` → the poly's top scanline was **~83000**, i.e. projected far
below the 480-line screen and never clipped.

## Root cause
`polygon::drawpoly` builds the scanline list as `whll.starty = miny`, `whll.length = maxy-miny+1`
straight from the projected vertex Y, then `scradr = surface + starty*pitch` in the converter. The
engine **top-clips** normal polys (`miny>=0`, hence no crash in normal play) but does **not**
bottom-clip these extreme-angle polys, so a poly entirely below the screen (`miny≈83000`) produced
`scradr` tens of MB past the framebuffer → OOB write. Horizontal-only clamping (Sprint 23) can't help
a bad scanline base.

## Fix (`Polygon.cpp`, `drawpoly`, after the min/max-Y loop)
```c
SLong _h = currscreen->PhysicalHeight;          // 0-based surface height (480)
if (miny >= _h || maxy < 0) return;             // fully off-screen vertically -> skip
if (maxy >= _h) maxy = _h - 1;                   // bottom overhang -> correct clip, no distortion
if (miny < 0)   miny = 0;                         // degenerate top only; ASM_Call_clamp bounds edge X
```
Covers both the gouraud/image and flat dispatch (single point, before the `miny!=maxy` branch).
Clamping `maxy` is proper clipping (draws the on-screen part, cuts the bottom) — verified no
distortion. The `miny<0` arm never triggers for normally-projected polys.

Together with Sprint 23 (horizontal span clamp) the software rasterizer is now bounded on **both**
axes: X by `ASM_Call_clamp` per span, Y by the `drawpoly` scanline clip.

## Validation
- **Reproduced** the crash on the pre-fix build (`MA_FORCE_PADLOCK` + `BOB_AUTOFLY=sweep`, ~1 in 4
  runs) and read the exact fault from the new handler.
- **Post-fix:** same config, **0 crashes in 14 runs** (P(14 clean by luck at the ~25% pre-fix rate) ≈ 2%).
- **Frame integrity** (no distortion from the maxy clamp): 640×480 dump non-black top/bottom/left/
  right all populated.
- (3D launch regression from Sprint 23 still green.)

## Tooling left in (gated, default off)
- `bob_main.cpp` crash handler now always dumps `fault_addr` + registers on any signal — future
  crashes are self-diagnosing.
- `MA_FORCE_PADLOCK=<frame>` (Viewsel.cpp) — force the enemy/padlock view at a given frame for repro.
- `MA_QUICKMISS` / `MA_TRACE_BOGIE` (Sprint 22) still present.
