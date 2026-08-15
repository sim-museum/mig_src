# Sprint 104 — "The menu was always opening" (PO-7) — ✅ CLOSED 2026-08-15 (goal MET)

**Planned 2026-08-15 (PO pre-approved ceremonies; continuous-sprint directive). Autonomous. ~8 pts.**
**Sprint Goal:** find out what R actually does, and get the radio command menu on screen with a
capture that proves it.

| Story | Pts | Result |
|---|---|---|
| S104-1 follow R through the chain, gate by gate | 3 | ✅ every gate passes — the menu opens |
| S104-2 capture it | 3 | ✅ **"1.Group Info / 2.Precombat / 3.Combat / 4.Postcombat / 5.Tower / 6.FAC/Bomb / 0.Exit"** |
| S104-3 make it a gate | 2 | ✅ `port/overlay_text.sh radio`, three arms: 848 / 351 / 207 |

## ⭐ The answer: R was never broken. The menu was drawing bars.

The whole chain works and always did: `[key] DOWN scancode=0x13 -> action index=500` →
`KeyPress3d(RADIOCOMMS) fired` → `SetToRadioScreen: deadtime=0 dplay=0 pCurScr=(nil)` →
`SetToUIScreen(...) accepted` → `promote pNewScr=...`. The screen then lives its full five seconds
(`TimeLimitedDisplay: budget=500 -= FrameTime()=2`, 250 frames) and closes itself.

What the PO saw is exactly reproducible today with **`MA_NO_ALPHATEXT=1`** — S102's control arm: an
opaque grey box with white blocks in it, over a busy cockpit, gone in five seconds. That is what
"typing R yields nothing" was. **The defect was PO-4's, and PO-4's fix closed this one too.**

Both captures are kept side by side (`port/ref/native/radio_menu.png` and the `bars` arm in the
gate's output) because the pair is the whole story.

## ⭐ The instrument that made it possible: arm the capture from the drive

The first four attempts to photograph this menu all missed it, and the reason is worth keeping.
`MA_DUMP_BACK=N` aims at the N-th frame — but these screens are opened by a key and close
themselves after five seconds, and **the pump counter that delivers the key and the Blt counter
that numbers frames run at wildly different rates in flight** (a tap at pump 500 and a dump at Blt
560 were seconds apart; the log line order proved the dump landed after the teardown). No fixed
frame number can hit a five-second window whose start is decided at runtime.

New hook: `MA_UISCR_SHOT=N` arms `ma_dump_arm` when a UI screen is promoted, and the N-th
back→primary Blt after that writes the frame. **The thing being tested arms the capture** — the
S80 lesson, now with a reusable mechanism. It will serve PO-9 (mission result) and any other
event-shaped screen directly.

## Also established

- **The timer is honest.** `Orders3DInit` gives the screen `5*100` units and `TimeLimitedDisplay`
  counts down by `FrameTime()`, measured at 2/frame → ~250 frames ≈ 5 s at 50 fps. A wrong-units
  `FrameTime()` here would have shut the menu in a frame or two and looked identical to "the key
  does nothing" — worth having ruled out by measurement rather than by reading.
- **`KeyPress3d` is a test-and-CLEAR.** The trace wrapper `MA_KP()` wraps the single existing call
  rather than calling it again to report on it: a second call would consume the hit bit and the
  feature would stop working *because it was being watched*.
- **Number-key selection is not yet verified headlessly** (see below). Recorded as open rather than
  claimed — the sprint proves the menu opens and renders, not that a selection is delivered.

## Open, precisely stated

Tapping an option key (`3` → `RPM_30`, DIK 0x04, confirmed in the binding dump) while the menu is up
does not produce a selection in the trace. The taps ARE delivered (`[key] DOWN scancode=0x04 ->
action index=108`), and the throttle consumer in `KEYFLY.CPP` is correctly guarded by
`if (!OverLay.pCurScr)` so it is not eating them. Complication found while testing: the pump rate in
flight is low enough that taps 30 pumps apart land *seconds* apart, so several attempts simply
arrived after the menu had closed. Next attempt should drive the option key from inside the screen's
own frame loop rather than from the pump counter.

## Gates

parity 5/5 · sweep 9 OPEN/0 CRASH · map click · map drag · sysbox exit · help click ·
**new overlay text gate PASS** (radio: letters 848 edges vs blocks 351 vs blank 207) ·
stress 20/20 · ASan 0.

## Result

Two PO defects in two sprints turned out to be one defect (PO-4) plus a way of looking at it. The
sprint's durable output is the **armed capture** — the port can now photograph any event-triggered
screen, which is exactly what the remaining PO items (mission result, map window, help viewer) need.
