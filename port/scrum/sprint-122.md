# Sprint 122 — "The surface is the mode, not the window" (PO-20) — ✅ CLOSED 2026-08-15 (goal MET) — ⭐ high resolution works in both renderers

**Planned 2026-08-15 (PO play-test; continuous-sprint directive). Autonomous. ~8 pts.**
**Sprint Goal:** the HUD belongs on the screen the player chose.

| Story | Pts | Result |
|---|---|---|
| S122-1 why is the HUD laid out small? | 3 | ✅ the overlay was told the screen is 640×480 |
| S122-2 size the render surface from the mode | 3 | ✅ one assignment; fixes hardware **and** software |
| S122-3 make a reported configuration reproducible | 2 | ✅ `MA_FORCE_DETAIL`, preference autosave |

## ⭐ The finding

The PO, at 1920×1080: *"the info line is at middle left, as if the screen were about 1/6 of its
actual size."* One trace answered it:

```
[3d] XX_SetGraphicsMode: DDCurrMode=6 ...        <- mode 6 = 1920x1080
[overlay] GetSurfaceDimensions -> 640x480        <- what the HUD lays itself out from
```

`COverlay` asks `DoGetSurfaceDimensions` and places the info line, messages and instruments from
that answer. It was being told 640×480 on a 1920×1080 screen, so the whole HUD sat in a corner —
while the terrain looked right, because the GL path projects from the drawable rather than the
surface. Two different notions of "the screen", disagreeing.

## Root cause — and it was S115's

`Display::SetDirectDrawMode` sized the offscreen render surface from the **window rect**:

```c
if (!fullScreen) ::GetWindowRect(DD.hWnd,&rect);
...
if (window_width<=0) window_width = DD.DDModes[curmode].width;   // fallback
```

The window rect still holds the PREVIOUS size when the mode is set — SDL is resized afterwards —
so choosing 1920×1080 produced a 640×480 surface.

**Before S115 this was accidentally correct.** `::GetWindowRect` was a zero-fill stub, so the
fallback always fired and used the mode. S115 made that stub real (rightly — `SetViewParams`
computes the 3D view origin from it, and a zero there put the whole world off-screen), and in doing
so removed the fallback and exposed the dependency. *Correct behaviour that rests on a stub being
wrong is a fault waiting for the stub to be fixed.* The surface is now sized from the mode
directly, which is what the port means: it renders at the chosen resolution and lets GL scale to
the window.

## It fixed the software renderer too

S119 measured software at 1920×1080 as tiled 3× horizontally and squashed into the top 160 rows —
the signature of rendering 640 wide and presenting 1920 wide. Same cause, same fix: content now
fills the frame (bbox y 0–1075, 95% non-black). **High resolution now works in both renderers**,
which had been listed as a pre-existing defect in both.

## Reproducibility, which cost more than the bug

The PO also hit a hang at max settings. I could not reproduce it: their configuration lived only in
memory, and killing the hung process to read its stacks destroyed the very state being reported.
Two changes so that never happens again:

- **`MA_FORCE_DETAIL=max`** — turns on every 3D detail flag and maxes filtering, so "all settings to
  max" is a configuration a script can enter.
- **Preference autosave**, once a minute from the event pump (`MA_NO_PREF_AUTOSAVE=1` disables).
  Settings used to reach disk only on a clean exit.

With those, the max-settings run reproduced clean (96.1% non-black at 1920×1080), and the PO
confirms the configuration now works apart from the HUD placement fixed here.

## Evidence

`port/ref/native/hud_1920.png` — 1920×1080 hardware: info line and message line along the bottom,
attitude gizmo top-left, artificial horizon top-right, all at the corners of the full frame.

## Gates

parity 5/5 byte-identical · sweep 9 OPEN/0 CRASH · map click · map drag · sysbox · help click ·
stress 15/15 · hw_gate PASS.

## Next

PO-17 (campaign dialogs overlapping), PO-18 (map zoom tiling), PO-19 (recon zoom keys), and
confirming terrain in the recon view.
