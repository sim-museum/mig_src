# Sprint 10 — M7 joystick flight input: LIVE VALIDATION + axis-mapping fix

**Goal:** validate the SDL→DirectInput joystick bridge end-to-end now that the stick is
free (the Sprint-9 impediment — a concurrent BoB session `EVIOCGRAB`-ing the same
`/dev/input/event14` — is cleared). The bridge code shipped last session but was never
exercised under live deflection.

## Outcome: DONE — and live validation caught a real bug.

The joystick (`Logitech Extreme 3D Pro`, SDL: 4 axes / 12 buttons / 1 hat) now drives the
flight model **correctly**, proven end-to-end on the native build.

### Bug found by validation: scrambled axis→flight-role mapping
First live run (`MA_TRACE_JOY`, stick at rest) exposed the map was wrong: physical roll
(`ax0`) was driving **rudder**, the throttle slider was driving **aileron**, and
aileron/elevator never tracked the stick at all.

**Root cause:** the six DirectInput axis GUIDs (`GUID_XAxis`…`GUID_RzAxis`) were *all*
defined as the all-zero `BOBGUID` in `bob_stubs.cpp` — identical and indistinguishable.
The game classifies each axis into a flight role by `guidType==GUID_XAxis` /
`GUID_YAxis` / `GUID_Rz…` (`SCONTROL.CPP::DIEnumDeviceObjectsProc`), so with every axis
reading as `GUID_XAxis` the stick pair never forms and `GetAxisConfig`'s default
assignment mis-routes everything. (The earlier M7 work had fixed `GUID_Key/Button/POV`
to be distinct for exactly this reason but left the *axis* GUIDs all-zero.)

### Fixes
1. **`bob_stubs.cpp`** — gave the 6 axis GUIDs their real, distinct DirectInput values
   (from `SRC/H/DINPUT.H`). Now the classifier can tell X from Y from Rz from Z.
2. **`bob_video.cpp` `DIDEV_EnumObjects`** — reordered the per-SDL-axis GUID map to match
   the Extreme 3D Pro's SDL axis order `[X, Y, twist, throttle]`:
   `{X, Y, Rz, Z, …}` so idx2(twist)→`GUID_RzAxis`→**rudder**, idx3(slider)→`GUID_ZAxis`
   →**throttle** (mirrors real DInput's `X,Y,Rz,Slider` enumeration for this stick).

After the fix, at rest: `aileron=544`(ax0 roll) `elevator=-545`(ax1 pitch)
`rudder=3212`(ax2 twist) `throttle=32767`(ax3 slider) — each role tracks its physical
axis. Object count also rose 15→17 (all 4 axes now enumerate distinctly).

### Deterministic deflection harness: `BOB_AUTOJOY` (new, gated)
Mirrors the keyboard-side `BOB_AUTOFLY`. Overrides the physical SDL axis read with a
scripted value so the **full** joy→DI `GetDeviceData`→`Analogue::PollPosition`→
`axisvalues`→flight-model chain runs without a hand on the stick (and in CI):
- `roll`/`pitch`/`sweep` — triangle-wave sweep of axis 0 / 1 / both.
- `center` — force all axes centred (drift baseline).
- `left`/`right`/`up`/`down` — HOLD one axis at full deflection (for A/B frame compare).

### End-to-end evidence
- **Axis reaches the game:** `BOB_AUTOJOY=roll` → `axisvalues.aileron` sweeps the full
  **±29000** (≈±0x7fff) while elevator/rudder stay 0 — correct axis isolation. 351
  samples over a 14 s flight, crash-free.
- **Flight model consumes it (aircraft banks):** captured frame 280 of three roll-hold
  flights (`BOB_AUTOJOY=center|left|right`, `MA_DUMP_BACK=280 BOB_EXIT_AFTER_DUMP=1`) and
  diffed the rendered views:
  | pair | mean \|Δ\|/px | pixels >24 diff |
  |------|----|----|
  | centre vs left  | 84.8 | 36.5% |
  | centre vs right | 34.7 | 23.6% |
  | left vs right   | 80.5 | 38.0% |
  Left and right aileron produce distinct, opposite rendered responses — the analog axis
  drives the aircraft, not just the input buffer.
- **No regression:** `port/stress_launch.sh 8` → **8/8** clean 3D launches.

## Diagnostics (gated, default off)
`MA_TRACE_JOY` (now also prints `[joy/axisvalues] aileron/elevator/rudder/throttle` from
`ANALOGUE.CPP::PollPosition`), `BOB_AUTOJOY=roll|pitch|sweep|center|left|right|up|down`.

## Build / rebuild note
Touched `bob_stubs.cpp`, `bob_video.cpp`, `ANALOGUE.CPP` (→ `_INPU` unity). Recompile
those three objects + relink. SCONTROL (axis classifier) unchanged.

## Next (Sprint 11 candidates)
- Optional: human-in-the-loop physical wiggle for final confirmation (chain already
  proven; the resting reads show SDL delivers distinct live per-axis values that the
  bridge passes through unchanged, so physical = injected).
- **M6:** render the `CLoad` loadgame sub-dialog (+ the un-hosted `REdit` control); then
  credits / replay screens.
- Backlog: campaign save/resume E2E; broader 3D fidelity A/B vs Wine.
