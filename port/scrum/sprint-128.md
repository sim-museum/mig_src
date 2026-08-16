# Sprint 128 — "A panel must replace, not layer" (PO-21) — ✅ CLOSED 2026-08-15 (goal MET)

**Unplanned, PO-reported with screenshots minutes after B6 shipped. Autonomous. ~5 pts.**
**Sprint Goal:** a campaign panel shows itself and nothing else.

## What the PO saw

Clicking the aircraft icon gave "a messy window with no Fly button", and clicking around "only
overlays more text". Two screenshots showed the Dossier, Player Log, briefing text and the
Preferences 3D panel all composited on top of each other.

## Cause — mine, from B6

```
[panelart] art 800x600 at (0,0) on canvas 1920x1080
```

Front-end panel art is a fixed 800×600 bitmap drawn at (0,0). **Before B6 the canvas WAS the art
size**, so each panel's art covered the whole screen and wiped the previous one. B6 made the canvas
the display resolution; the art now covers one quadrant and the other three keep the last panel's
pixels. Nothing was leaking — the new panel simply never painted over the old one.

This is the shape of regression worth naming: a change that is correct in itself (the canvas
*should* be the display size) breaks a behaviour that was resting on the old invariant (art covers
canvas). The same sentence describes S122's `GetWindowRect` fix exposing `SetViewParams`.

## Fix, from the gold video

Gold's briefing panel sits **centred with dark margins** on a 1920×1080 screen. So: centre the art,
clear the margins, and give hosted controls the same origin — they are positioned in the panel's own
coordinate space and would otherwise sit in the corner while the art sits in the middle.

`port/ref/native/panel_centred.png` — the panel centred, margins black, controls on the art.

## Gates

parity **5/5 byte-identical** — the panel origin is 0 at the default resolution, where the canvas
still equals the art size, so nothing changes on the reference path · sweep 9 OPEN/0 CRASH · map
drag PASS · map click · sysbox · help click.

## Still open on this screen

Two panels' menus still overlap in the capture ("BACK LOAD" over "…eferences"), i.e. a previous
panel's CONTROLS are still registered when the next opens. Distinct from the art problem fixed here
and next in line, since it is what stands between the PO and a working Fly button.
