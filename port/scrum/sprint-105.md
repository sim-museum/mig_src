# Sprint 105 — "The map's own words" (PO-6) — ✅ CLOSED 2026-08-15 (goal MET) — ⭐ overlay text was drawn through a centre-origin window

**Planned 2026-08-15 (PO pre-approved ceremonies; continuous-sprint directive). Autonomous. ~8 pts.**
**Sprint Goal:** the in-flight map window shows its text.

| Story | Pts | Result |
|---|---|---|
| S105-1 reach the map screen and see what it draws | 2 | ✅ `M` opens `firstMapScr`; options resolve ("Accel/Waypoints/Radio/Zoom/Exit") |
| S105-2 find out where the text went | 4 | ✅ **drawn, and displaced by exactly (320,240)** |
| S105-3 fix it | 2 | ✅ text now lands in the kneeboard: "1.Accel / 2.Waypoints / 3.Radio / 4.Zoom / 0.Exit" + the clock/waypoint line "9:00 E. Pyongyang City" |

## ⭐ The defect: absolute coordinates drawn through a centre-origin window

The map view renders through a `Window` created with `WINSH_MID`, whose constructor does

```
logicalscreenptr -= PhysicalMinX*BytesPerPixel + PhysicalMinY*BytesPerScanLine;   // WRAPPER.CPP:444
```

with `PhysicalMinX = -width/2`, `PhysicalMinY = -height/2` — i.e. **its origin is the screen
centre** (+307840 bytes on a 640×480×16 surface, the exact offset measured in the trace). The 3D
renderer wants that. **Overlay text does not:** `RenderInfoPanel` lays its text out in absolute
top-left coordinates computed from `physicalWidth`/`physicalHeight`. The port's software text path
inherited whatever window the renderer left current, so on the map screen every glyph was displaced
by (320,240) and landed on the map instead of the kneeboard. The HUD text was fine only because its
current window happens to be the master screen.

**Fix:** `polygon::DoPutC` now blits through `currscreen->Master()` — the top-left-origin screen the
text coordinates are expressed in. `MA_TEXT_WINBASE=1` restores the old behaviour for A/B.

## ⭐ How it was found: mark the cell, don't hunt the glyph

Four different measurements said the text was drawn (`[uiscr] opt 0 key=0 at (380,44) …
text="Accel"`), the colour was right (`fontColour=0 entry=0x0020`, black; the highlight
`fontColour=120 entry=0xF800`, red), and the fast path was not declining — yet the notepad was
uniformly blank (mean 236.6, zero dark pixels). Screenshots cannot distinguish *drawn somewhere
else* from *drawn then covered*, and both were plausible.

`MA_TEXT_MARK=1` paints every glyph cell as a solid magenta rectangle. One run: **1924 magenta
pixels, rows 243–256, cols 344–481** — the text was on screen all along, offset by the window
origin. A colour that appears nowhere else in the game turns "it's missing" into a coordinate
readout.

Second instrument, added when the first decline trace lied: the "why did the fast path decline"
counter deduped **per reason**, so the front end's early declines (reasons 1 and 2) silently
swallowed anything the map screen would have reported. Re-keyed per *(reason, colour)*. That is
"filter, don't cap" in the shape it takes when the filter is too coarse — the fifth booking of that
lesson in this port, and the first where the filter itself was the trap. The same trap had already
bitten once this sprint in `[doputc]` (`if (n++<4)`, spent entirely by the front end).

## Also fixed by the same change

The map's **clock + waypoint name** ("9:00 E. Pyongyang City", gold shows "5:24 Koesan") now renders
top-left, from the same displaced path.

## Open (shared with PO-7)

Selecting an option **inside** an in-flight menu is still not verified headlessly — `2` on the map
menu (→ `waypointMapScr`, the screen whose `UpdateWaypointDisplay` draws the gold's
Rendezvous/Ingress/Initial-Point table) produced no promote, exactly like `3` in the radio menu.
Same mechanism, one open item, now stated once in the backlog rather than twice.

## Gates

parity 5/5 · sweep 9 OPEN/0 CRASH · map click · map drag · sysbox exit · help click · overlay text
(radio letters + info line drawing) · stress 20/20 · ASan 0. New reference capture
`port/ref/native/map_window.png`.

## Result

Three PO defects (PO-4, PO-7, PO-6) and three different mechanisms — a span filler that ignored the
alpha plane, a five-second timer nobody had photographed, and a window whose origin is the screen
centre. All three presented identically to the player: *the text is not there*.
