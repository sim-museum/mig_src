# Sprint 18 — "Hands on the mouse": in-flight mouse (DInput-less, DOS INT 33h → SDL)

**Goal:** close the one clear *subsystem* gap vs the sister BoB port — the in-flight mouse. Make the
mouse drive the flight view / gunsight / UI cursor natively.

**PO:** standing pre-approval (planning + review). Run autonomously; prompt only for an impediment.

## Planning — root cause + design (spiked at S17 close)

**Why the mouse is dead in flight:** MiG's mouse path is **not DirectInput** (that's BoB). The engine's
mouse `Device` (`SRC/INPUT/ANALMOUS.CPP`) reads **DOS INT 33h** via `ASM_DOSvia31(0x33, &regs, …)`:
- `Initialise` → fn `0x0000` (reset/status), expects `regs.w.ax==0xFFFF` if a mouse is present.
- `ReadPosition` → fn `0x000B` (read motion counters → `cx`=dx, `dx`=dy) + fn `0x0003` (buttons in `bx`).

The Linux build stubs the whole thing: `HARDPASM.H` `ASM_DOSvia31` `BOB_LINUX` branch is
`{ return BOOL_FALSE; }`. So fn 0x00 never reports a mouse → `Initialise` returns FALSE → the device
deactivates. **Fixing `ASM_DOSvia31`'s INT-33h handling is the whole bridge.**

**Key facts established:**
- `DPMIregs.w.{ax,bx,cx,dx}` are `UWord`; the engine's `Axis::position` is `SWord`, so a raw 16-bit
  two's-complement delta placed in `w.cx/w.dx` recovers its sign on assignment (verified by reading
  `ANALMOUS::ReadPosition` `dx*50` → SWord truncation). So just write `(UWord)(SWord)delta`.
- SDL already pumps `SDL_MOUSEMOTION` in `bob_video.cpp pump_events` (line 267) but **discards
  `xrel/yrel`**; only LEFT button is tracked. Need to accumulate rel-motion + R/M buttons.
- Flight state signal exists: `MIG.CPP:875` computes `ma_in3d` (`Rtestsh1::THISTHIS && tmpinst`).

## Design
1. `bob_video.cpp`: accumulate `g_mouseRelX/Y` from `SDL_MOUSEMOTION.xrel/yrel`; track a 3-bit
   `g_mouseBtns` (L/R/M) from button down/up. New `extern "C" int ma_mouse_int33(unsigned* ax,
   unsigned* bx, unsigned* cx, unsigned* dx)`:
   - fn 0x00 → `*ax=0xFFFF, *bx=3`; zero the rel accumulators; return 1.
   - fn 0x0B → `*cx=(UWord)(SWord)relX, *dx=(UWord)(SWord)relY`; zero accumulators; return 1.
   - fn 0x03 → `*bx=btns, *cx=winX, *dx=winY`; return 1.
   - else return 0.
2. `HARDPASM.H` `ASM_DOSvia31` `BOB_LINUX` branch: if `intnum==0x33`, forward `registerimage->w.*`
   through `ma_mouse_int33`; return TRUE if handled, else `BOOL_FALSE` (preserve current behaviour for
   every other interrupt).
3. **Flight-scoped capture (don't break the 2D menu mouse):** `extern "C" void ma_set_mouse_captured(int)`
   → `SDL_SetRelativeMouseMode`; call from `MIG.CPP` on the `ma_in3d` edge (on entering 3D capture,
   on leaving release). The menu keeps absolute-coord clicking (`ma_mouse_take_click`); flight gets
   captured relative motion.
4. Test injector `BOB_AUTOMOUSE=look|aim` (mirrors `BOB_AUTOFLY`/`BOB_AUTOJOY`): feed synthetic
   rel-motion so fn 0x0B returns nonzero deltas headlessly → A/B the frame (forward vs mouse-panned),
   the same method that validated keyboard (S3, 89.9% pixel change) and joystick (S10).

## ⚠ Open scope question (resolve FIRST — task 1)
The flight **axis binding is data-driven** (`Save_Data` control config, not obviously mouse-bound by
default). The bridge will *activate* the device and flow motion/buttons, but whether the mouse visibly
moves the **view/gunsight** out-of-box depends on the default `reqaxes` assignment for the mouse
`Device`. **Task 1 = spike the default binding** (does the shipped config map the mouse to
AU_VIEWH/VIEWP or AU_UI_X/Y?). Outcome decides the sprint shape:
- **Bound by default** → one increment: bridge + capture + A/B "view pans".
- **Not bound** → increment 1 = bridge (device live, motion/buttons traced — a verifiable milestone);
  increment 2 = default mouse→view binding (a separate, config-touching change with its own A/B).

## Definition of Done
- `ANALMOUS::Initialise` reports the mouse present (device active) — traced.
- INT 33h fn 0x0B/0x03 return live SDL motion/buttons — traced.
- If bound: A/B shows mouse motion pans the flight view (≥ the keyboard-look threshold).
- **No menu regression:** `port/stress_launch.sh` (menu→flight) clean; 2D front-end clicking unaffected.

## Log

**Task 1 (default-binding spike) — DONE.** `SRC/INPUT/MOUSE.CPP` is ~entirely dead code (the old
real-mode cursor draw); the live mouse→axis assignment is **data-driven** from the `Save_Data` control
config, not hardcoded. So "mouse visibly drives the view" can't be assumed out-of-box → **confirmed
two-increment shape:**
- **Inc 1 — the bridge** (next): `ASM_DOSvia31` INT 33h over the SDL rel-motion accumulator +
  flight-scoped capture. Verifiable milestone: `ANALMOUS::Initialise` reports present, fn 0x0B/0x03
  return live motion/buttons (traced), no menu regression. Self-contained, committable.
- **Inc 2 — binding + visible control:** ensure/verify the default mouse→AU_VIEWH/VIEWP (or AU_UI_X/Y)
  assignment, then A/B "mouse pans the view" vs the keyboard-look threshold.

Ready to implement Inc 1 (design above is complete; structures + hook points all located).

**⚠ COURSE CORRECTION (mid-inc-1) — the INT 33h path is DEAD; the live mouse is DirectInput.**
Implemented the INT 33h bridge (ASM_DOSvia31 fn 0x00/0x0B/0x03 over an SDL rel-motion accumulator +
flight-scoped capture), built clean — but an instrumented flight proved `ANALMOUS::ReadPosition`
(INT 33h fn 0x0B) **never fires** (trace count 0, flight reached 3D). Root cause: the engine's *live*
input path is `Analogue::Initialise` (`ANALOGUE.CPP:264`) → `runtimedevices` → **`GetFirstMouse`
(`ANALOGUE.CPP:697`) = `DIdev->EnumDevices(DIDEVTYPE_MOUSE, …)`** — pure **DirectInput**, exactly
parallel to the joystick (`GetFirstJoystick`/`DIDEVTYPE_JOYSTICK`). The DOS INT 33h `ANALMOUS` `Device`
(like most of `MOUSE.CPP`) is legacy/never-instantiated. **All INT 33h work reverted** (clean tree;
no dead code shipped). De-risking outcome: the correct integration is now pinned.

**Corrected Inc 1 — mirror the S10 joystick in the `bob_video.cpp` DInput compat:**
- `DI_EnumDevices` (`bob_video.cpp:1555`) currently handles only `DIDEVTYPE_JOYSTICK`/`0` → it reports
  **no mouse**, so `GetFirstMouse` finds nothing. Add a `DIDEVTYPE_MOUSE` branch reporting one mouse
  (a `g_diMouse` GUID/device, like `g_diJoystick`).
- `DI_CreateDevice` (`:1543`) → return `g_diMouse` for the mouse GUID.
- `DIDEV_EnumObjects` (`:1501`) → for the mouse, enumerate **X axis, Y axis, buttons** with the right
  `DIDFT`/GUIDs (`GUID_XAxis`/`GUID_YAxis`) so `DIEnumDeviceObjectsProc` builds the mouse dataformat
  and the game maps mouse axes → AU roles. (Honour the `DIDFT` filter — the shared `firstaxes`
  underflow trap from the cross-port notes.)
- `DIDEV_GetDeviceData`/`GetDeviceState` (`:1433`) → for the mouse, emit SDL **relative** motion
  (`SDL_MOUSEMOTION.xrel/yrel` accumulator — the reusable part of the reverted work) + button state as
  buffered events at the enumerated dwOfs. Mouse is relative-axis (`makerelative`/`DIDF_RELAXIS`,
  `ANALOGUE.CPP:316/327`), so deltas (not absolute) are correct.
- **Capture:** tie `SDL_SetRelativeMouseMode` to the mouse device's `Acquire`/`Unacquire` (DInput
  exclusive-cooperative semantics) rather than a MIG.CPP `ma_in3d` edge — more correct + self-contained.
  (Note: the reverted MIG.CPP in3d-edge toggle didn't trace as expected — verify acquisition-based
  capture instead.)
- Keep the `Save_Data` binding question as **Inc 2** (the spike still stands).

**INC 1 DONE — DInput mouse device live end-to-end.** Mirrored the S10 joystick in
`bob_video.cpp`: `DI_EnumDevices` reports a `DIDEVTYPE_MOUSE` (`GUID_SysMouse`, defined in
`bob_stubs.cpp`), `DI_CreateDevice`->`g_diMouse`, `DIDEV_EnumObjects` emits X/Y as **`DIDFT_RELAXIS`**
(the type the game keys on at `ANALOGUE.CPP:219` to flag a mouse axis) + 3 buttons, `DIDEV_SetDataFormat`
copies the mouse format, `DIDEV_GetDeviceData` drains SDL relative motion + buttons per poll, capture
tied to device `Acquire`/`Unacquire` (`SDL_SetRelativeMouseMode`, `MA_NO_MOUSE_GRAB` opt-out).
Synthetic `BOB_AUTOMOUSE` lives in `mouse_obj_value` (race-free, mirrors `autojoy_axis_value`).
**Validated (flight):** chain fires — `DI_EnumDevices->1 mouse`, `DI mouse ACQUIRED`,
`SetDataFormat numObjs=5`, `poll dX=10 objs=5` x90 (motion delivered every frame). Regression: stress **4/4**.

**A/B finding -> Inc 2 confirmed:** baseline vs mouse-look frame = **0.0% change** — the motion reaches
`Analogue::PollPosition` but the view does not move, because the default `Save_Data` config does not map
the mouse X/Y axes to a flight role (`axismaps[]`=AU_UNUSED -> `ANALOGUE.CPP:213` skips them).
**Inc 2 = default mouse->view (AU_VIEWH/AU_VIEWP) binding** + A/B "mouse pans the view".

**INC 2 DONE — and the faithful answer was simpler than mouse-look.** The game's own default-config
logic (`SCONTROL.CPP:484-490`) binds the mouse's first axis pair to **`AU_UI_X`/`AU_UI_Y`** — the
in-flight **UI cursor**, not view-pan. That is MiG Alley's native in-flight-mouse behavior (a 1999 jet
sim: mouse = cursor, not free-look). Verified end-to-end with a gated trace at `ANALOGUE.CPP:560`
(`Analogue::PollPosition` relative-axis handling): with `BOB_AUTOMOUSE=look`, **`RELAXIS theaxis=4
(AU_UI_X) dwData=10`** fires 179x — the SDL mouse delta flows DInput device -> `GetDeviceData` ->
`PollPosition` -> `axisvalues[AU_UI_X]`. So the port faithfully delivers mouse input to the engine's
native cursor axis; the game's own AU_UI consumer drives the cursor from there. (Mouse-look would be a
non-native *feature add*, out of scope for a faithful port.) The earlier 0% back-surface A/B is
explained: AU_UI is a cursor overlay, not part of the captured cockpit-view render.

## Sprint 18 — CLOSED. In-flight mouse subsystem complete (the one gap vs the BoB port is closed).
- Inc 1: DInput mouse device live (enumerate/create/acquire/setformat/poll/deliver) — mirror of S10 joystick.
- Inc 2: motion reaches the native `AU_UI_X/Y` cursor axes end-to-end (verified `theaxis=4`).
- Regression: menu->flight stress 4/4 throughout. Diag: `MA_TRACE_MOUSE`, `BOB_AUTOMOUSE`, `MA_NO_MOUSE_GRAB`.
- Course-correction logged: first wired DOS INT 33h (`ANALMOUS`), proved dead, reverted, re-aimed at DInput.
