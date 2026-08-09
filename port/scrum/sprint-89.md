# Sprint 89 — "Range the sight" (B7) — ⚠️ CLOSED PARTIAL 2026-08-09 — B7 characterized: the chain is intact and was never a port bug

**Planned 2026-08-09 (PO pre-approved ceremonies; continuous-sprint directive). Autonomous. ~8 pts.**
**Sprint Goal:** B7 — "the F-86 radar gunsight ranges/expands with target range".

| Story | Pts | Result |
|---|---|---|
| S89-1 scope the ranging path | 3 | ✅ measured end to end |
| S89-2 deliver the increment | 3 | ◐ reachability + traces landed; **a live lock not yet observed** |
| S89-3 gates | 2 | ✅ |

## Execution log

### S89-1 — the chain, and where it stops — DONE
The engine path is complete and **compiled**: `shape::GetRadarItem(ItemPtr, SLong)`
(`3DCOM.CPP:19218`) picks the nearest target inside the radar cone and records
`radarTmpItm`/`radarRange`; `CalcRadarRange` feeds the gunsight; `SHAPE.SphereXScale/YScale` scale
the reticle (`3DCODE.CPP:1445`). Confirmed present in the binary: `nm` shows
`shape::GetRadarItem(item*, long)`.

*(Two greps found nothing before that. The definition lives behind a high-byte licence banner —
`grep -a` is mandatory in this tree, exactly as `CLAUDE.md` warns. Worth re-learning cheaply: a
"missing" function in this codebase usually means the wrong grep.)*

**Measured in flight** (`MA_TRACE_GUNSIGHT`, Hot Shot, 990 target sightings):
```
[gunsight] target rng=144370 radarOn=0 polypit=1
```
- `polypit=1` — the cockpit gate is satisfied.
- **`radarOn=0` — always.** So `GetRadarItem` is never called and the reticle *cannot* range.

**And that is the game's own design, not a port defect.** `radarOn` is set from exactly two places,
both difficulty settings: `GD_PERFECTRADARASSISTEDGUNSIGHT` and
`GD_REALISTICRADARASSISTEDGUNSIGHT` (`3DCODE.CPP:327-334`), which the player chooses through the
Game tab's *Gunsight Ranging* combo (`IDC_CBO_GUNSIGHTRANGING`). They are off in a default save. So
B7's premise — "the gunsight doesn't range" — was never a bug to fix; the feature is opt-in.

### S89-2 — what landed, and what did not — PARTIAL
- **`MA_FORCE_RADARSIGHT=1`** (`=2` also enables ground lock) opens the gate for headless
  verification without touching the player's save. Verified: `radarOn=1` and `GetRadarItem` is now
  exercised on every sighting.
- **`MA_TRACE_GUNSIGHT`** now also prints the definitive event — a radar **LOCK** — from inside
  `GetRadarItem` where `radarTmpItm` is set.
- **NOT achieved: a live lock, and therefore no observed reticle scaling.** Every target traced sat
  at ~144 km, far outside `radarRange`, and a 150 s headless Hot Shot never closed to gun range.
  Claiming B7 done on "the code path is reachable" would be exactly the kind of inference this
  project keeps banning — the acceptance criterion is *the reticle scales with range*, and that has
  not been seen. **B7 stays open**; the next sprint needs a closing engagement (a longer flight, or
  a mission/scenario that starts within radar range) and then a before/after capture of the reticle.

## Gates — all under `gl-lock`
- **2D parity: 5/5 byte-identical. Stress: 20/20 PASS. ASan: 0 reports, 4/4 paths 2/2.** (All new code is behind
  `getenv` guards and default-off, so the flight path is unchanged unless a trace is requested.)

## Result
B7 moves from "⬜ not started, assumed broken" to **precisely characterized**: the engine chain is
intact, compiled and reachable; it is gated on an opt-in difficulty setting, so nothing about it was
ever a port defect; and the remaining work is *observation*, for which the hooks now exist.
