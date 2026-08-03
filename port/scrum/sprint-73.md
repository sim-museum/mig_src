# Sprint 73 — "Unmask the cockpit" (I3 — cockpit-black fix)

**Planned 2026-08-02 (PO pre-approved ceremonies). Autonomous. DoD: the cockpit-black
deviation (#10) fixed and gold-verified, or a precisely-located blocker.**

## Context
S72 opened the 3D-view parity frontier (I3) and closed *partial*: it characterized #10/#11's
"black box" deviations and narrowed the cockpit-black to "cockpit-specific imagemaps resolving
to black (not loaded/bound)", scoping the fix for a focused follow-up. A prior partial session
had also left an uncommitted `MA_TRACE_PITLIGHT` probe in `3DCOM.CPP` testing a *lighting*
hypothesis (cockpitAmbientCol ~0).

## Sprint Goal
Land the cockpit-black fix (#10) with gl-lock A/B verification, or hand off a precisely-located
blocker. Measure, don't assume (S64 discipline); don't force a speculative parity-poisoning
render change (S72 warning).

## Execution log

### S73-1 — Root cause: CONFIRMED (and all three prior hypotheses REFUTED) — DONE
Every step via `gl-lock` captures (frame 220 cockpit, `MA_DUMP_BACK=220 BOB_CLICKSEQ='40,r1;95,r0'`):

1. **Lighting REFUTED.** The prior session's `MA_TRACE_PITLIGHT` probe reported
   `cockpitAmbient=(255,255,255) landAmbient=(255,255,255)` — identical, healthy, full-white.
   The world renders fine with that same ambient ⇒ not a lighting problem.
2. **Imagemap-load REFUTED.** `MA_TRACE_LBM`: 236 LBM imagemap bodies load, **none all-black**
   — the cockpit texel *index* data is present. (Overturns S72's "not loaded/bound".)
3. **Palette-not-populated REFUTED (globally).** A `MA_TRACE_PAL` probe in
   `polygon::createpoly` showed `XX_PalChange` runs (`branch=software-buffer`, `lpDirect3D=nil`
   — the port forces `fSoftware`, so `DDRWINIT.CPP:772` leaves `lpDirect3D` NULL), and in
   flight `palette_table` is populated with real 565 colours for **world** object polys.
4. **The actual mechanism.** Tagging polys with `shape::fPolyPitShade` (bound by mangled symbol
   — `class shape` is only fwd-declared in `Polygon.cpp`) isolated the cockpit polys. At
   cockpit-draw time the active `palette_table` LUT is **stale/empty** while it is populated for
   world polys — same slot 0, different draw-time state. The cockpit's own
   `createpoly→SelectPalette(0)` **no-ops** because `polygon::selectedPalette` cache already
   reads 0, so cockpit imagemap/flat texels index an empty LUT → **near-0 (black) 565 pixel**.
   Terrain is immune: it renders via `LandFadeData`, not `palette_table` (`ma_xasm.nasm:1354`).
5. **Proof.** A diagnostic that forced `currscreen->SelectPalette(0)` for cockpit polys turned
   the flat-black cockpit **fully textured** (metallic canopy + panel + gunsight drum). And
   `BTREE.CPP:580` carries `//dead POLYGON.SelectPalette(0)` — the engine's *original* per-object
   palette reset, disabled (fine for hardware D3D where palettes are per-texture; broken for the
   software port). Every object case in `drw_obj` has the same disabled reset.

**Correction recorded:** the deviation was NOT the S72 dense-black "pure (0,0,0)" it first
looked like. Dense re-sampling of the fixed frame shows rich metallic detail (8.35% exact-black,
tan/brown/white/gray texels); the early "pure black" reading was a mis-sample of a few dark
points (an S64-family "measure the whole field, not a point" lesson).

### S73-2 — Fix LANDED + gold-verified — DONE
`BTREE.CPP:580` (`COCKPIT_OBJECT` case, `MA_LINUX`): re-enable the engine's per-object palette
reset, forcing a real LUT reload past the stale cache:
```c
POLYGON.selectedPalette = -1;   // defeat the cache
POLYGON.SelectPalette(0);        // re-enable the //dead reset, forced
```
Clean (no-diagnostic) `gl-lock` capture at frame 220: **fully-textured cockpit = gold #10** —
metallic canopy arch, instrument panel, gunsight range drum (10/20/30/40), side knob, gunsight
glass, ADI padlock inset now showing attitude (was a black rectangle).
- **Bonus #11:** external F-86 (`BOB_KEYSEQ='12,0x40'` F6) renders fully textured — silver/white
  skin, yellow ID bands, "FU-908", drop tanks; ADI inset content. The S72 "aircraft
  near-silhouette dark" is not present (the aircraft/`MOBILE_OBJECT` was already fine; the
  palette-reset bug prominently hit only the cockpit). #10 **and** #11 → CLOSE.

### S73-3 — Gates + close — DONE
- **2D parity byte-identical:** `title` 0 px, `prefs_3d` 0 px vs `port/ref/native/` (GL-free
  `MA_SHOT`). The fix is a single `MA_LINUX` block in the 3D object dispatcher; the 2D front-end
  never enters `drw_obj`/`COCKPIT_OBJECT`, so by construction + verified, 2D is untouched.
- **ASan `asan_all.sh`: PASS — 0 AddressSanitizer reports across all 4 paths** (flight +
  campaign map/fly/nextday), all modes reached 2/2. Both cockpit-exercising paths (flight,
  camp-fly) ran clean under ASan.
- **Stress `stress_launch.sh` under `gl-lock`: PASS — 20/20** reached & sustained 100 3D
  frames, 0 crashes (clean sweep; the fix doesn't touch startup/threading).
- Cross-port note (shared engine: the disabled per-object `SelectPalette` reset + the
  software-path `palette_table` staleness) — **DEFERRED**: the shared lessons file
  (`port/BOB_PORT_LESSONS.md` / `bob/doc/…`) was being live-edited by the concurrent BoB
  session (uncommitted change present), so appending would risk a write-conflict. The finding
  is fully captured here + in the memory note; sync the note when the shared file is quiescent.

## Result
The 3D-view parity frontier's headline deviation (#10 cockpit-black) is **fixed and landed**,
gold-verified — not merely scoped as in S72. #11 confirmed clean. Diagnostics reverted; the
committed diff is the 12-line `BTREE.CPP` block only (the prior session's `3DCOM.CPP` pitlight
probe + banner-encoding noise were reverted, lighting having been refuted).
