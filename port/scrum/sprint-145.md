# Sprint 145 — "The band was there before the click" (PO-19) — ✅ CLOSED 2026-08-16 (goal MET) — ⭐ the map fills the screen

**Planned 2026-08-16 (PO: continuous sprints on the campaign GUI).**
**Sprint Goal:** the map zoom control does not mess up the map.

| Story | Pts | Result |
|---|---|---|
| S145-1 reproduce "the zoom icon messes up the map" | 3 | ⭐ the click changes nothing at all |
| S145-2 explain the black band | 3 | ✅ the view is sized to leave room for docked toolbars |
| S145-3 give the map the screen | 2 | ✅ 242,558 black pixels → 42,055 |

## The click was innocent

The PO: *"small zoom icon ... messes up the map"*. Clicking it (`IDC_ZOOMIN`, id 7) produces a
map with a wide black band down the right — so far so consistent. But running the same recipe
**without** the click produces **the same picture**: 242,558 black pixels either way, the first
fully-black column at x=1752 in both. The band is not caused by the zoom; it is the map's normal
state at 1920×1080, and the zoom control merely drew attention to it. (`CMainFrame::OnGoNormal` is
`ShowWindow`/`SetWindowPos`/`ModifyStyle` — all no-ops in this port, so the button genuinely does
nothing yet. That is a separate, smaller gap.)

*Always run the control arm* — booked again, and this time it turned a "zoom is broken" report
into a screen-filling fix.

## Why the map stopped short

```
[maptile] client 1728x888 -> min-zoom applied, m_zoom=1.692383 size=1728x3027
```

The map view is 1728×888 on a 1920×1080 screen. `CMainFrame::RecalcLayout` sizes it to the frame
minus `m_borderRect` — **the space the docked toolbars occupy**. On Windows those are real docked
windows and they fill that band. This port has no dock manager: it composites the toolbars over
the map at offsets the idle loop computes, so the reserved band belongs to nothing and is never
painted. Four bars at 48px = 192px in each axis, and the map's own "min zoom for full screen map"
then dutifully fits the *smaller* box.

The view now takes the whole client area — and specifically the whole **canvas**, not the frame's
client rect. The first attempt used the frame and made it far worse (`client 800x600`, 1.28M black
pixels): the frame is still a compat 800×600 default that stopped meaning anything when B6 made
the canvas the display resolution. The canvas is what actually gets presented.

```
[maptile] client 1920x1080 -> min-zoom applied, m_zoom=1.879883 size=1920x3363
```

**Black pixels 242,558 → 42,055**, and the remainder is the distance ruler's own black strip.
`port/ref/native/map_fullscreen.png` — the Korean peninsula across the full 1920×1080.

## Gates

parity 5/5 byte-identical (campaign_map rebased — the map is larger now, and shows the front line
and airfields that were previously off the bottom) · sweep 9 OPEN/0 CRASH · map icon click · map
drag · map filter · sysbox exit (92.8%) · dialog scroll.
