# Sprint 174 — "The mission flies, and stops at 20 knots" (K10) — ⚠️ CLOSED 2026-08-22 (goal PARTLY MET, 5/8)

> ⚠ **CORRECTED BY S175. The cause stated below is WRONG.** This sprint concluded that the
> defect was in the **ground roll** ("on the ground, thrust is not producing acceleration past
> 20 kt"). S175 traced the engine model itself and found the aircraft accelerating perfectly
> normally — `thrust=19749N, airspeed 0.1 → 10.6 m/s and still rising` — and **the flight model
> simply stopping**. The 20 kt plateau is the last model state redrawn 400+ times. Every
> measurement below is accurate; the conclusion drawn from them is not. See `sprint-175.md`.

**Planned 2026-08-22** (PO ceremonies pre-approved). Continuing EPIC K in script order into the
flying half: step 15, *"Fly → you're on the runway. 100% thrust, release wheel brakes (, and .),
pull back around 100 knots."*

| Story | Pts | Result |
|---|---|---|
| S174-1 fly the mission we built | 3 | ✅ frag → Fly → 3D, on the runway at 0 kt / 4 ft |
| S174-2 an oracle for takeoff | 2 | ✅ `MA_TRACE_HUD`, `BOB_AUTOFLY=takeoff` |
| S174-3 take off | 3 | ❌ **plateaus at 20 kt** — logged as **PO-52**, cause isolated |

## What works

The Wonju strike this epic has spent six sprints building **flies**. `MA_CAMP_FLY` drives
frag → `FragFly` → `StartFlying` → the 3D world, and the player starts **on the runway**:

```
[hud] frame=0  speed=0 Kts alt=0 ft mach=0.00
[hud] frame=20 speed=0 Kts alt=4 ft mach=0.00
```

4 ft is the cockpit eye height above the strip — wheels down, stationary. That is the first half of
K10, and it is the first time the port has flown a mission the port itself built.

Throttle input reaches the flight model. The A/B is unambiguous:

| | frame 0 | 120 | 200 | 440 |
|---|---|---|---|---|
| no input | 0 kt | 0 | 0 | 0 (still 0 at 420) |
| `BOB_AUTOFLY=takeoff` | 0 kt | 12 | 20 | 20 |

## What does not

**At 100% thrust the aircraft accelerates to 20 kt and stays there, indefinitely.** Rotation speed is
~100 kt. Logged as **PO-52**.

The value of this sprint is that the obvious explanations are already **excluded by measurement**,
not by argument:

| suspect | measurement | verdict |
|---|---|---|
| the throttle command never arrives | `[thr] RPM_00 -> thrustpercent=100`, 188× in one run | **lands** |
| the player is on the AI takeoff rail | `movecode=0` (`AUTO_FOLLOWWP`), `controlmode=1` (`MANUAL`) | **not on it** |
| the wheel brakes are on | `KEYFLY.CPP:1189` applies them under `KeyHeld3d` — off unless held | **not on** |
| the flight model is broken | Hot Shot airborne start: **503 kt, Mach 0.84, 15,966 ft**, evolving normally | **model is fine** |

So the defect is specifically the **ground roll**: on the ground, at full military power, with the
player in manual control, thrust is not producing acceleration past 20 kt. Next candidates are
undercarriage friction / ground reaction, and whether weight-on-wheels ever clears — but those are
the *next* sprint's measurements, not this one's conclusions.

## My first hypothesis was wrong, and it cost one run to find out

*"The player's aircraft at a runway start is still on the AI's `AUTOMOVE` taxi/takeoff sequence, so
the throttle lands in `fly.thrustpercent` while the AI still owns the movement."*

That was plausible, fitted every symptom, and is **false**. `movecode` and `controlmode` are
**identical** on the runway start and on the airborne flight that works perfectly.

I had `AUTOMOVE.CPP` open and was reading toward a conclusion. Two lines of trace answered it in one
run. *When a hypothesis is about a value, print the value.*

## A driver bug that failed in the shape of the bug I was hunting

The first takeoff drive released the wheel brakes at **two** points (`t3d == 60 || t3d == 90`). They
toggle — so it released them and put them straight back on. The symptom was *full throttle, a plateau
at 20 kt, no lift-off*: **exactly** the defect under investigation.

It happened to be harmless (the brakes are `KeyHeld3d` and were never on), but it was on course to
become the answer. This is `§8-MA121` — *a trace is code* — one sprint later and in the other
direction: **a synthetic driver is code too, and it fails in the shape of the thing you are looking
for.**

## New tooling, reusable for K11–K13

- **`MA_TRACE_HUD=<n>`** — samples `COverlay::DrawTopText`'s own `speed2 / altitude2 / mach`: the
  flight model's numbers, in the player's units, from the line that draws them. No pixels, no
  re-derivation. Filtered by frame interval rather than capped, so the whole run is represented
  (S158's "filter, don't cap").
- **`BOB_AUTOFLY=takeoff`** — full throttle held for the life of the flight, counting from
  **`g_ma_in3d`** (the sim being up) rather than from process start. The existing `throttle` mode
  counts pumps from process start and stops at 600, so on the campaign path — hundreds of idles of
  front end before a flight exists — **every one of its taps was spent before takeoff**. It was never
  wrong for the Quick Mission path it was written for; it was simply unusable for this one.

## Correction to K10's wording

The criterion says *"wheel brakes release on `,`/`.`"*. The game has no such action: `KEYFLY.CPP`
sets `LeftWheelBrake`/`RightWheelBrake` under **`KeyHeld3d`**, so they apply while held and release
when let go. There is nothing to "release" at a runway start because nothing is holding them.

Same class as K9's *"callsign edit accepts text"* (`§8-MA123`): a criterion written from the PO's
prose that inherited a **mechanism** the prose never claimed and the game never had. The PO's script
is a player's description of what they did, and "release wheel brakes (, and .)" is exactly what
tapping a hold-to-brake key feels like from the cockpit.
