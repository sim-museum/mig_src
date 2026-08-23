# Sprint 176 — "It pulls to the left" (PO-53, and the answer to PO-52) — ✅ CLOSED 2026-08-22 (goal MET, 8/8)

**Unplanned, PO-driven.** The PO asked for MA to be launched to check joystick calibration: *"It seems
to pull to the left."*

| Story | Pts | Result |
|---|---|---|
| S176-1 is the pull in the hardware or the port? | 1 | ✅ hardware clean; port mis-maps |
| S176-2 fix the axis roles | 3 | ✅ enumerate in DirectInput canonical order |
| S176-3 what it means for PO-52 | 2 | ⭐ **PO-52 was this all along** |

## The defect

The port enumerated joystick axes in **SDL order** — for a Logitech Extreme 3D that is
X, Y, **twist**, **slider**. Real DirectInput enumerates objects in **canonical order**:
X, Y, Z, Rx, Ry, Rz, Slider — which for the same stick is X, Y, **slider (Z)**, **twist (Rz)**.

That ordering is load-bearing, because `SController::RemakeAxes` fills the role combos
**first-come**:

```
STICKDEV (pair)  -> X & Y
THROTDEV         -> the next unassigned analogue axis
RUDDEV           -> the one after that
```

So **whichever axis is enumerated third becomes the throttle**. In canonical order that is the
slider — correct. In SDL order it is the **twist**, which pushes the **slider** onto the rudder. The
slider rests at its minimum, so the game read a **permanent full-left rudder**.

Measured in flight, before and after:

```
before   aileron=-161  elevator=-97  rudder=-32767  throttle=16447
after    aileron=-161  elevator=-97  rudder=-643    throttle=32767
```

Fix: emit the axes in canonical rank order, with each entry's **DIDFT instance still carrying the
SDL axis index** — `joy_obj_value()` uses that to read the right physical axis, so only the order and
the advertised offsets change. `MA_JOY_SDL_ORDER=1` reverts.

Also defined **`GUID_Slider`**, declared in `dinput.h` since bring-up and never given a value —
nothing had needed it until the enumeration became canonical.

**PO confirmed from play: elevator, aileron and rudder now calibrate correctly.** Trace agrees: the
rudder covered −31740 → +29683 and returns to ~128 centred.

## ⭐ This was PO-52, and the reporter had the answer

The PO, unprompted: *"Your flight test regression was just spinning into the ground every time
because of the joystick mis-calibration."*

Exactly right. Full-left rudder ground-loops the aircraft, which is why every runway test sat at
**20 kt at full thrust**. With the axes fixed, the same test accelerates **0 → 143 Kts**, straight
through the old plateau and past rotation speed.

**Three causes had been published for PO-52 before this one, and all three were wrong:**

| sprint | published cause | what it actually was |
|---|---|---|
| S174 | "ground-roll physics: thrust is not producing acceleration" | the engine was making ~20 kN and airspeed was rising |
| S175 | "the flight model stops being stepped" | true, but caused by my own driver's brake-key taps (PO-54) |
| — | the unstated assumption that a flight defect lives in flight code | it was in the *input* layer |

The one thing none of those sprints did was **ask the person who had flown it**. The PO's single
sentence carried more diagnostic information than two sprints of instrumentation, because they had
seen the aircraft's *behaviour* — spinning — while every trace I wrote sampled a *quantity*. A
number tells you what a value was; a person who watched it tells you what it did.

**The rule: when a defect was reported from play, ask the reporter what they SAW before instrumenting
what you think it is.** It costs one question.

## The measurement chain that did work

Worth keeping, because it was fast and each step killed a hypothesis outright:

1. **Read the hardware directly** (`/dev/input/js0`) — resting X = 0. The stick is not pulling. That
   removed "it's the joystick" in about a minute, without the game.
2. **Read what the game received** (`[joy/axisvalues]`) — rudder = −32767 while the physical twist was
   centred. That is a measurement *of the fault itself*: the port is not passing through what the
   stick reports.
3. **Read the routing** (`[axmap]`) — `dwOfs=1164 -> slot 3` carrying SDL axis 2's value. Names the
   crossing precisely.
4. **Read the game's own classifier** (`SCONTROL.CPP:118`) — Rz *is* classified as the rudder-type
   axis, so the port's GUIDs were right and only the *order* could be wrong.

Step 4 mattered most: it is where the fix stopped being "swap two axes until it works" and became
"match DirectInput's enumeration contract". §8-MA126 in practice — the measurement was *of* the
cause, not of its neighbours.

## Residual

- **Throttle direction is UNVERIFIED.** The slider never left its minimum during the PO's session
  (`ax3=-32768` throughout), so `throttle=32767` is consistent with both a correct inversion and a
  backwards one. Needs one slider push to settle.
- **K10** still open: the aircraft accelerates past rotation speed but does not rotate, because
  `BOB_AUTOFLY=takeoff` applies throttle only. The PO's script says *"pull back around 100 knots"* —
  the driver needs an elevator input. That is a harness gap, not a game defect.
- **PO-54** (wheel-brake keys pause the sim) is open and unrelated to this fix.
- The regression suite was interrupted twice to free the display; `damage_elements` reported
  "the tab bar never took a click" in the partial run and **has not been re-checked**.
