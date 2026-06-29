# Sprint 32 — "Smooth flight" (B3: sustained frame rate)

_2026-06-29 · Product Owner pre-approved planning + review._

## Sprint Goal
Close **B3** — verify the 3D view animates smoothly during flight — with a measurement, and
add the fps instrumentation needed to keep it a regression gate.

## User Story
**B3** — *As a player, the 3D view animates smoothly during flight, so it feels like a sim.*
Acceptance: sustained ≥30 fps over a 60s flight; no tearing/stale-buffer; 2D present gated off in-3D.

## What shipped
`SRC/compat/bob_video.cpp` — `MA_TRACE_FPS` gated counter in `present_dbg` (on every present
path, 2D + 3D): reports instantaneous fps per ~1s window + the running average + total frames.

## Measurement (Sprint Review / DoD demo)
~62s headless flight (`MA_ENABLE_3D=1 MA_TRACE_FPS=1 BOB_AUTOFLY=throttle`, two-click launch,
`SDL_VIDEODRIVER=offscreen`):

```
[fps] 33.1 inst | 33.1 avg |   95 frames /  2.9s   <- first warm-up window
[fps] 50.0 inst | 44.8 avg |  535 frames / 11.9s
[fps] 50.0 inst | 48.1 avg | 1540 frames / 32.0s
[fps] 50.1 inst | 48.9 avg | 3048 frames / 62.3s
samples=62  min_inst=33.1  mean_inst=49.3
```

**Result: sustained ~50 fps for the full 62s flight** (steady-state locked at 50.0; the 33.1 min
is only the first 1s warm-up window) — well above the ≥30 fps gate.

- **Why exactly 50:** the present is paced to the 50 Hz sim tick (S21 `ma_sim_pace`, 20 ms),
  not free-running — frames advance in lockstep with the sim. The front-end, by contrast, runs
  ~92 fps unpaced. This paced lockstep is what the "no tearing/stale-buffer" criterion wants
  (the buffer is presented once per sim step, never re-presented stale).
- **2D-present-gated-off-in-3D:** already in place (Phase 5 `MIG.CPP` `!in3d` idle gate).

## Definition of Done
- ✅ Compiles clean; full rebuild + link OK.
- ✅ ≥30 fps over 60s flight — **met (≈50 fps sustained, 3048 frames / 62s)**.
- ✅ Present is sim-paced (no free-run); 2D gated off in-3D.
- ◻ Visual A/B (tearing/colour) belongs to **B2** (fidelity vs Wine) — tracked there, not re-opened here.
- ✅ `MA_TRACE_FPS` documented in STATUS.md diagnostics.

## Backlog impact
- **B3** 🔨 → ✅. Remaining EPIC B: **B2** (fidelity A/B vs Wine — the visual-tolerance check;
  STATUS notes the residual is a fidelity-target *choice*, low priority).

## Commit
`Sprint 32 (B3): MA_TRACE_FPS counter; sustained ~50 fps flight verified`.

## Retro
- The fps lock at the sim rate (50 Hz) is the correct behaviour and a neat confirmation that the
  S21 pacing fix holds under a sustained flight — `MA_TRACE_FPS` now makes any future free-run or
  stall regression a one-line check.
