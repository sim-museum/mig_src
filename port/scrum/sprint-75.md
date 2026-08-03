# Sprint 75 — "Capture the debrief" (I1/#12) — ✅ CLOSED 2026-08-02 (goal MET) — ⭐ I1 INVENTORY COMPLETE

**Planned 2026-08-02 (PO pre-approved ceremonies). Autonomous. DoD: capture parity #12 (the
post-mission debrief) and close the gold-shot inventory (I1), or a precise blocker.**

## Context
S74 established that gold #12 is the **post-mission debrief** (mission header + Claims table +
BACK/AC STATS/GROUND STATS/REPLAY), reached only via the mission-end path, and scoped it to a
"stable-display mission→debrief run". It was the one remaining uncaptured gold shot.

## Sprint Goal
Reach the post-mission debrief natively and capture it, A/B vs gold #12.

## Execution log

### S75-1 — Debrief captured, matches gold — DONE (⭐ NO code change, NO display lock)
- **Found the exit path:** `View3d::CloseWindow`'s default id is **`IDOK`** (`STUB3D.H:314`);
  the flight-exit → `Rtestsh1::OnOK` → `OnFlyingClosed` → `LaunchScreen(debrief)`
  (`MIG.CPP:245`). The trigger is `OverLay.quit3d=1`.
- **The proper scriptable hook already exists:** `BOB_AUTOEXIT=N` (`MIG.CPP:1004`) →
  `ma_request_flight_exit()` (`= OverLay.quit3d=1`), fired from the **main thread** idle loop
  right after `ma_process_flight_close()`, so the exit processes promptly (a first attempt with
  a bespoke draw-thread hook starved the main thread — 58k spin frames — before the close
  drained; reverted in favour of `BOB_AUTOEXIT`).
- **Captured HEADLESS** (the ASan camp-fly mode already proves 3D flight runs under
  `SDL_VIDEODRIVER=dummy`): `MA_ENABLE_3D=1 BOB_AUTOEXIT=60 MA_SHOT=220` under dummy →
  fly Hot Shot → auto-exit → debrief → GL-free `MA_SHOT` of the 2D debrief canvas. **No
  `gl-lock` needed, no display contention.**
- **A/B vs gold #12 = strong match:** identical layout, the **same pilot briefing photo**,
  mission header (Mission/Objective/Status), Claims table (Player/UN/Red), and yellow small-caps
  BACK/AC STATS/GROUND STATS/REPLAY chrome (S69 fonts). **The only differences are mission-type
  DATA, not render deviations:** native flew Hot Shot (air-to-air) → aircraft Claims
  (F51/F80/F84/F86/B26/B29/MiG 15/Yak 9) + "Objective: Pyongyang Main Airfield"; gold flew a
  ground-attack mission → ground-target Claims (Supply/Bridge/Troops/Tank) + "Name: Kimpo
  Airfield". The AC-Stats-vs-Ground-Stats default view follows the mission type; both are
  selectable via the buttons. Ref: `port/ref/native/flight_debrief.png`.
- **#12 → CLOSE. The I1 gold-shot inventory is now COMPLETE** — all 15 gold shots have native
  captures in `port/ref/native/`.

## Gates
**No code change** (the bespoke hook was reverted; the capture uses the existing `BOB_AUTOEXIT`
+ `MA_SHOT`). The only tree delta is the new ref `port/ref/native/flight_debrief.png` + docs —
nothing in the build changed, so ASan/stress/2D-parity are unaffected by construction.

## Result
The last uncaptured gold shot (#12 debrief) is captured and matches gold; I1 complete. Delivered
with zero code change and zero display contention by recognising the existing `BOB_AUTOEXIT` hook
and that 3D flight runs headless under `SDL_VIDEODRIVER=dummy`.
