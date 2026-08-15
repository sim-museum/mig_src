# Sprint 124 — "The canvas, not the dialogs" (B6 / PO-17) — ✅ CLOSED 2026-08-15 (goal MET, one artifact open)

**Planned 2026-08-15 (PO: "keep going, fix B6"). Autonomous. ~8 pts.**
**Sprint Goal:** run the 2D front end at the selected resolution, so the campaign UI is usable.

| Story | Pts | Result |
|---|---|---|
| S124-1 what pins the canvas to 800×600? | 2 | ✅ nothing does — the first background blit establishes it |
| S124-2 establish the canvas at the display resolution | 3 | ✅ dialogs land at gold's size and position |
| S124-3 let the map fill it | 3 | ✅ the map fills 1920×1080; one missing tile column |

## ⭐ The measurement that made this a one-line diagnosis

Gold's Player Log: ~340×420 in a 1920×1080 front end. Ours: **the same 339×400** in an 800×600
canvas. Identical absolute size; the canvas is what differs. On an 800×600 surface that dialog is
42%×67% of the screen, so three open dialogs cannot avoid colliding — the PO's screenshot. The
dialogs were never misplaced.

With the canvas established at the display resolution, the Player Log lands at **(783,340)
339×400** against gold's **~(780,330) ~340×420**. Position and size both match without touching a
single placement calculation.

## What was actually wrong

The canvas *grows to fit whatever is drawn*, and the first thing drawn is an 800×600 front-end
background — so the entire 2D UI lived on an 800×600 surface that was then upscaled to the window.
Everything sized in absolute pixels was magnified with it. Gold's model is the opposite: a
full-resolution screen with fixed-size art centred on it (the briefing panel at 20s has dark
margins) and the map drawn procedurally to fill.

Two changes, both behind `MA_CANVAS_FULLRES=1` while this is brought up:

- `ma_gdi_set_screen_size(w,h)` establishes the canvas from `Save_Data.displayW/H`.
- `MaViewRectScope` keeps the view rect installed. S61 restored it because a real rect made the
  map's tile loop draw one more row and inflate an 800×600 canvas; at the display resolution that
  extra row is exactly what should be drawn. **A workaround for a too-small canvas stops being
  correct once the canvas is right.**

## Evidence

`port/ref/native/b6_fullres_map.png` — the campaign planning map at 1920×1080: all of Korea
visible, toolbars at native size, Player Log correctly proportioned. Compare gold at 12s.

## Open: one missing tile column

A black vertical band at roughly x 860–940 — a tile column the map loop does not draw at the wider
client width. Next sprint's first task; it is a tile-range bound, not a rendering fault.

## Gates

Default path unchanged — the flag is off by default because the committed 2D references were
captured on the art-sized canvas: parity 5/5 byte-identical · sweep 9 OPEN/0 CRASH.

Flipping the default is deliberate future work: it invalidates every 2D reference at once, so the
references must be re-captured in the same sprint that flips it.
