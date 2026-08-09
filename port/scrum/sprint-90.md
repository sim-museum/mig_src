# Sprint 90 — "Lock the sight" (B7) — ⚠️ CLOSED PARTIAL 2026-08-09 — locks proven; the reticle pins at the range clamp

**Planned 2026-08-09 (PO pre-approved ceremonies; continuous-sprint directive). Autonomous. ~7 pts.**
**Sprint Goal:** finish B7 — obtain a live radar lock and observe the reticle ranging.

| Story | Pts | Result |
|---|---|---|
| S90-1 live lock + observed scaling | 5 | ◐ **locks achieved**; scaling still unobserved, cause now known |
| S90-2 gates | 2 | ✅ |

## Execution log

### S90-1 — three measurements, two of them corrections of my own — PARTIAL

**1. Locks achieved.** S89 could not get one (air targets ~144 km out). The ground-lock variant
does it: `MA_FORCE_RADARSIGHT=2` + a longer flight →
**715 locks across 710 distinct ranges** (911 414 … 1 215 900). The ranging chain genuinely runs.

**2. `SphereXScale/YScale` was the WRONG observable — and my first trace hid that.** The initial
trace printed a constant `X=2 Y=3` and I was one step from recording "the reticle does not scale".
Two problems, in order:
- Those fields are **`Float`** (`3DCOM.H:384`) and the trace cast them to `long` — *my* truncation,
  not the game's. Re-traced at full precision: `X=2.4142 Y=3.2190`.
- Still a single value all flight — because **they are not the gunsight reticle at all.** 2.4142 ≈
  1+√2; `PARTICLE.CPP` multiplies sphere radii by them. They are view/projection scaling.

**3. The real observable, and why it does not move.** `RequiredRange = radarRange` (`3DCOM.CPP:20661`)
is where a lock becomes the gunsight's range, feeding `CalcGunsightPos`, which places and sizes the
reticle. Traced:
```
[gunsight] RequiredRange=100000 (radarRange=1215900)      <- one distinct value, all flight
```
`RequiredRange` is **clamped to 20 000 … 100 000** (`:20662-20666`). Every lock obtained is at
~1.2 M — more than ten times the ceiling — so the gunsight sits pinned at maximum range and the
reticle correctly does not move.

**So B7 is not blocked by code.** The chain is wired end to end and demonstrably live; what is
missing is a target inside gun range. Note the flight *does* see objects at 7 500–8 200 units, but
`GetRadarItem` only considers what is inside the radar cone, and an unsteered scripted flight rarely
points at anything that close.

**B7 stays open.** Closing it needs a scenario that puts a target within the clamp window (200 m –
1 km) while pointing at it — i.e. a real merge, the C4 padlock/`BOXTARGET` path, or a purpose-built
close-start scenario. All the observation hooks now exist, so that is one run's work once the
scenario does.

## Gates — all under `gl-lock`
- **2D parity: 5/5 byte-identical. Stress: 20/20 PASS. ASan: 0 reports, 4/4 paths 2/2.**
  All additions are `getenv`-guarded and default-off, so the flight path is unchanged.

## Result
Two of this sprint's three findings corrected earlier work of mine rather than the game's: a
truncating trace, and an observable that had nothing to do with the gunsight. The useful residue is
a precise, falsifiable statement of what B7 needs — *a lock inside 20 000–100 000 units* — instead
of a vague "gunsight doesn't range".
