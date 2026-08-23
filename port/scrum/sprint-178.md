# Sprint 178 — "PO-54 was not a bug, and the premise was mine" — ✅ CLOSED 2026-08-22 (goal MET, 8/8)

**PO-driven.** The PO: *"fix PO-54 so the brake keys don't freeze the sim"*, then, on seeing the test
runs: *"gold standard behavior is no movement until both brakes are tapped."*

| Story | Pts | Result |
|---|---|---|
| S178-1 find what pauses the sim | 3 | ✅ `View3d::drawloop`, by return address |
| S178-2 fix it | 3 | ❌ **nothing to fix — not a defect** |
| S178-3 retract cleanly | 2 | ✅ PO-54 closed with the full explanation |

## PO-54 does not exist

Two things were wrong with it, and both were mine.

**1. The "freeze" was the flight ending.** `Inst3d::Paused(bool)` has a dozen callers. Reading them
produced two confident wrong guesses — the cockpit map (`MapScr::FirstMapInit` sets `KF_PAUSEON`) and
the accel map (`SelectFromAccelMap`) — and **neither fires**. Printing
`__builtin_return_address(0)` on every transition and running it through `addr2line` named the
caller in a single run:

```
[paused] 1 -> 0  caller=0x836a04c  ->  Rtestsh1::Launch3d(bool)     <- flight starts
[paused] 0 -> 1  caller=0x8393f95  ->  View3d::drawloop(void*)      <- flight ends
```

and the second sits at **log line 89104 of 89113**, immediately before
`[timeproc] instances=0 currinst=(nil)`. That is the normal **teardown**, not a mid-flight freeze.

What S175 recorded as *"the sim stops being stepped and never resumes"* was the aircraft
**ground-looping off the strip and dying** under PO-53's full-left rudder. The flight ended, so the
world stopped; the renderer kept presenting the last frame.

**2. The correlation was read backwards.** S175's A/B looked decisive:

| | brake taps | "pause" |
|---|---|---|
| no synthetic input | – | never |
| takeoff drive | yes | cycle 707, 2/2 |
| takeoff drive, `MA_NO_BRAKE_TAP` | no | never |

The chain is: **taps → brakes released → aircraft rolls → full-left rudder → crash → flight ends.**
No taps → never rolls → never crashes → nothing to end. The brake keys are upstream of the crash,
not of a pause. Two samples, causation inferred in the wrong direction, and it produced a backlog
item for a bug that does not exist.

**3. And the standing-still was correct all along.** With the rudder fixed, a throttle-only run with
`MA_NO_BRAKE_TAP=1` still sat at 0 kt for 1400 frames — which I was about to log as a *new*
defect. The PO: *"gold standard behavior is no movement until both brakes are tapped."* The parking
brakes are on at mission start. The game was working; I did not know what working looked like.

## Techniques worth keeping

**When a value is set from many places, do not read the places — print who set it.**
`__builtin_return_address(0)` + `addr2line` cost about five minutes and replaced two wrong readings
of a dozen call sites. This port has `assert_no_crash` symbolising frames already (S171); the same
trick applies to any "who did this?" question about shared state.

**Ask what the system is SUPPOSED to do, not only what the reporter saw.** `§8-MA129` came out of
PO-52 this morning — *ask what they saw*. Today produced the other half. Twice in one day a sentence
of the PO's domain knowledge beat a sprint of instrumentation:

- PO-52: *"spinning into the ground"* — named a behaviour no scalar in my logs expressed.
- PO-54: *"no movement until both brakes are tapped"* — named the **correct** behaviour, which no
  amount of measuring the port could have supplied.

I was measuring a system whose specification I did not have, and treating deviations from *my*
expectation as defects. The gold video and the PO are the specification; the traces only say what
the port does.

## Net effect

- **PO-54 closed, not a defect.** No code change.
- **PO-52's history now reads correctly**: PO-53 (axis enumeration) was the single cause of the
  runway failure, and the two intermediate explanations were artefacts of it.
- Kept: the `[paused]` return-address trace, `[hatbtn]` raw-code trace and `[configmenu]` marker, all
  env-gated. `KEY_CONFIGMENU` being bound to both **F12** and a **joystick hat direction**
  (`A1_hat3NW`) is real and worth knowing — it pauses and closes the flight — but on a 1-hat stick
  the hat maps to base 292, not the 276–283 block, so it is not reachable here. Measured:
  `[hatbtn] dwData=0xffffffff base=292 pressing=8 -> raw -1`.
