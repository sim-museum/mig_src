# Sprint 175 — "It was never the ground roll, and the second cause was mine" (PO-52) — ✅ CLOSED 2026-08-22 (goal MET, 8/8)

**Planned 2026-08-22** (PO ceremonies pre-approved). PO-52 is the critical path for K10–K13.

| Story | Pts | Result |
|---|---|---|
| S175-1 trace thrust where it becomes force | 2 | ✅ `MA_TRACE_THRUST` in `ENGINE.CPP` |
| S175-2 find the ground-roll defect | 2 | ⭐ **there isn't one** — S174's cause was wrong |
| S175-3 the real cause | 4 | ⚠️ **two wrong causes eliminated**; correctly aimed, still open |

## This sprint corrects S174, and then corrects itself

That is the whole story, and it is worth stating plainly rather than presenting the last version as
if it were the first.

**S174 said:** *"the defect is the **ground roll** — on the ground, at full military power, thrust is
not producing acceleration past 20 kt."*
**Wrong.** One trace inside the engine model:

```
[thrust] throttle=1.00 rpm=11750/11750 thrust=20168 desired=20475 airspeed=0.1
[thrust] throttle=1.00 rpm=11933/11750 thrust=19749 desired=19956 airspeed=10.6
```

~19.7 kN of thrust and airspeed climbing 0.1 → 10.6 m/s, **still rising** when the trace stops. The
aircraft was never failing to accelerate.

**Then I said:** *"the flight model stops being stepped"* — true, and the measurement behind it was
sound (campaign flight **50** engine ticks vs Hot Shot **5060**; the "20 kt plateau" is the last model
state redrawn 400+ times). But I was about to hand that over as **the defect**.

**It is my own test driver.** Walking the gate chain inward — the sim thread runs 13,650 iterations,
`timeout=0`, `accelcountdown=1`, the world-step gate wide open — the last gate is `Paused()`:

```
[movestep] cycle=0   paused=1 num=1     <- startup
[movestep] cycle=9   paused=0 num=1
[movestep] cycle=707 paused=1 num=1     <- and never runs again
```

A/B, and it reproduces:

| run | brake-key taps | pause | result |
|---|---|---|---|
| no synthetic input | – | never | 0 kt (nothing driving it) |
| `BOB_AUTOFLY=takeoff` | yes | **cycle 707, 2/2 runs** | 0 → 20 kt, then frozen |
| `BOB_AUTOFLY=takeoff` + `MA_NO_BRAKE_TAP` | no | never | **0 kt over 13,600 frames** at full throttle |

**Tapping the wheel-brake keys pauses the simulation.** Reproducibly, 38 of 41 samples in each of two
runs.

## I wrote the note that predicts this, one sprint ago

`§8-MA124`, S174, in my own words: *"a synthetic **driver** is code, and it fails in the shape of the
bug you are hunting."* Then I walked into it again — and the driver's failure once more looked
exactly like the defect under investigation, twice in two sprints.

What saved it this time was the note's own prescription, applied: **A/B the driver against no
driver**, and against a driver with one input removed. That is three runs, and it converted a
confident wrong answer into a measurement.

*Writing the lesson down does not install it. Running the check does.*

## What is actually established

1. The campaign mission **flies** and starts on the runway (S174, unchanged).
2. The engine produces **~20 kN** of thrust at 100% RPM (measured, both cases).
3. **Tapping `,`/`.` pauses the sim** — reproducible, 2/2.
4. **Without those taps the aircraft does not move at all** — 0 kt over 13,600 frames at full
   thrust, sim running, not paused.

(3) and (4) together point at the **wheel-brake key path**, in both directions: something holds the
aircraft on the ground, and touching the brake keys both unsticks it and pauses the sim.
`KEYFLY.CPP:1189` applies the brakes under `KeyHeld3d` — *held*, not toggled — so the first thing to
check is whether `KeyHeld3d(LEFTWHEELBRAKE)` reads **true when nothing is held**, which would park the
aircraft permanently and make a tap the only thing that ever clears it.

**I am not publishing that as the cause.** It is the next measurement. Two causes have already been
published too early in two sprints.

## Two wrong causes, one pattern

S174 measured the **inputs** (throttle command, control mode, brakes) and the **rendered output**
(HUD), and concluded about the layer in between. S175 measured the **sim scheduling** and concluded
about the game, when the input was mine.

Both times the mistake was the same shape: **the conclusion named a layer that had not been
measured.** The fix is not more caution, it is one habit — *before writing a cause, name the
measurement that is of the cause itself, not of its neighbours.*

## Reproducer

```
BOB_AUTOFLY=takeoff  MA_TRACE_HUD=100  MA_TRACE_THRUST=1  MA_TRACE_TIMER=1
   [+ MA_NO_BRAKE_TAP=1 to remove the brake taps]
```
on the campaign path (frag → Fly), with `BOB_CLICKSEQ="40,r1;95,r0"` (Hot Shot) as the control.
All tracing is env-gated and default-off.
