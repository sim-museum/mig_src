# Sprint 92 — "Read the bogey" (C4) — ⚠️ CLOSED PARTIAL 2026-08-09 — C4d written, NOT verified; backlog corrected

**Planned 2026-08-09 (PO pre-approved ceremonies; continuous-sprint directive). Autonomous. ~7 pts.**
**Sprint Goal:** advance C4's padlock enhancements after B7 stalled on scenario, not code.

| Story | Pts | Result |
|---|---|---|
| S92-1 C4 enhancement | 5 | ◐ **C4a found already done**; C4d written but **unverified** |
| S92-2 gates | 2 | ✅ |

## Execution log

### The backlog was stale — C4a is already implemented
The sprint opened intending C4a ("box scales with range, encloses rather than intersects"). It is
**already in the tree**: an `MA_LINUX` block in `OVERLAY.CPP` sizes the box from a projected world
half-extent (`R_WORLD`) through the same perspective divisor as the position, clamped to sane pixel
bounds — i.e. it already grows as the bogey closes. The C4 backlog row still listed it as
outstanding. Corrected there.

Remaining C4 work is therefore **C4c** (adaptive black/white telemetry colour) and **C4d** (bogey
kts + closure + own kts in the readout).

### C4d — written, and honestly not verified
Added to the padlock telemetry block:
- **Own speed**, using `DrawTopText`'s exact formula (`speed * Save_Data.speed.mmpcs2perhr` with
  `speedUnitStr`) so the two readouts cannot disagree.
- **Closure**, as d(range)/dt from successive samples timed by the engine's own
  `RealFrameTime()` (ms) rather than an assumed frame rate; positive means closing. The sample is
  dropped when the frame time is implausible (`>500 ms`) or the padlock target changed, so a
  padlock switch cannot print a spike.
- **Bogey speed was NOT added.** No per-target speed field was reachable from this scope without
  spelunking the flight-model structures, and inventing one would be worse than omitting it.

**Verification failed, and the feature is therefore unproven.** To exercise the readout headlessly
the sprint added `MA_PADLOCK_TELEM` / `MA_PADLOCK_BOX` env defaults (both toggles are
modifier-driven — ALT+D vs plain D — and a synthesised DIK tap carries no SDL modifier state, so
neither is reachable from `BOB_KEYSEQ`). A Hot Shot flight with `ENEMYVIEW` (0x3B) tapped via
`BOB_KEYSEQ` produced a clean cockpit capture — canopy bow, gunsight, ADI — **and no padlock box or
telemetry**, because `trackeditem2` was never set: no `[keyseq]` trace fired at all, so the view key
never took effect.

**So C4d ships unverified and is recorded that way.** The code is inert unless a padlock target
exists *and* telemetry is enabled, so the risk is contained; but no capture shows it, and the
backlog says so rather than claiming the story.

## Gates — all under `gl-lock`
- **2D parity: 5/5 byte-identical. Stress: 20/20 PASS. ASan: 0 reports, 4/4 paths 2/2.**
  (3D overlay + an env-read static initialiser; the 2D front end is untouched, which the
  byte-identical sweep confirms.)

## Result
One real correction (C4a was done; the backlog said otherwise), one feature written to the point of
"needs an eye on it", and a named blocker for the next attempt: **headless padlock engagement**.
`BOB_KEYSEQ` taps are not reaching the view-selection path, and until that is understood neither
C4c nor C4d can be shown working — which makes *that*, not more telemetry code, the next thing to fix.
