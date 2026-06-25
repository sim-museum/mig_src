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
(filled as the sprint runs)
