# Sprint 172 — "The port had never dragged anything" (K8) — ✅ CLOSED 2026-08-22 (goal MET, 8/8)

**Planned 2026-08-22** (PO ceremonies pre-approved). The PO: *"keep going with K8."*

| Story | Pts | Result |
|---|---|---|
| S172-1 drive a real drag through the map's own handlers | 3 | ✅ `CMapDlg::MaDriveDrag` |
| S172-2 address waypoints by name, not by pixel | 2 | ✅ `MA_MAP_DRAG`, `MA_MAP_ITEM_SCAN` list |
| S172-3 gate it, including what must NOT drag | 3 | ✅ `port/route_drag.sh` |

## The finding

**Nothing in the port had ever dragged anything** — and that was deliberate. S95 wired map clicks by
driving `CMapDlg::OnLButtonDown` and `OnLButtonUp` **in the same tick**, with a comment saying why:

> *Down+Up in the same tick keeps `m_bDragging` FALSE, which also avoids `CMapDlg::OnMouseMove` — it
> dereferences `GetDC()` unchecked.*

That was true when it was written. Compat's `CWnd::GetDC` now returns a real static `CDC`, so the
hazard is gone — and the entire engine drag chain had been sitting there intact and unreachable ever
since:

```
OnLButtonDown   FindMapItem -> m_buttonid
OnMouseMove     starts the drag once the pointer leaves a 3px box AND AllowDragItem() agrees,
                then recomputes the item's World position from the cursor on EVERY move
OnLButtonUp     OnDragItem() -> clamp into theatre, RecalcRoutePositions, CalcWPTimes, CalcFuel
```

`MaDriveDrag(from,to)` drives it in **eight steps**, not one jump: the world position is recomputed
per move, so a single-step drag exercises the drop and not the dragging.

*A workaround's comment records the hazard as it was on the day it was written. When the story that
needs the real path finally arrives, re-check the hazard before designing around it — this one had
expired.*

## Addressing waypoints

`MA_MAP_DRAG="<frame>,<wp>@<dest>"` or `"<frame>,<wp>+<dx>,<dy>"`. Both name halves resolve through
the map's own `FindMapItem`, never from pixels: icon positions move with zoom, scroll and campaign
state, and a recipe naming a pixel is testing that pixel (S95's rule).

`MA_MAP_ITEM_SCAN` now takes a **comma-separated list of frames**. It was one-shot, which can only
ever describe the map as it *opens* — but the whole of EPIC K **edits** the map: the six route
waypoints (`Regroup / Initial Point / Egress / Ingress / Disperse / Rendezvous`) do not exist until a
mission is authorised. Only the **first** scan clicks; a later scan is pure observation, or the run
would diverge because we looked at it.

The scan geometry (`MA_MAP_SCAN_PITCH`, `MA_MAP_SCAN_TOPCLEAR`) is now **one definition** shared by
the S158 scanner and the S172 finder. A finder with a coarser pitch than the scanner would miss an
icon the scanner reports, and the gate would read *"the waypoint is not there"* about an item that
plainly is — the two-paths-one-fact shape again.

## The world position is the oracle

`info_waypoint::World` is what the flight reads. Screen coordinates are a rendering artefact. World
units are **centimetres** (`RANGES.H: METRES250KM = 25000000`), which makes the PO's *"drag the IP to
within 4 miles of the target"* a directly checkable number rather than a vibe — and it has to be
measured **after** the drop, because `OnDragItem` clamps the waypoint into the theatre and
recalculates the route, so where it lands is not where it was dropped.

Result: **4925 m — 3.06 miles.** Inside the script's 4.

## ⭐ My own instrumentation lied first

The first two-drag run printed:

```
[mapdrag] released ... world (72208160,57594405) -> (71183770,59535093)   <- Initial Point
[mapdrag] released ... world (57230421,58018282) -> (71183770,59535093)   <- Egress
```

Two different waypoints, **byte-identical** final coordinates. That is not a thing that happens, and
it is the only reason this was caught: a plausible-but-wrong pair of numbers would have gone
straight into the gate.

The drags were fine. The *trace* read the after-position through `m_buttonid` — a member that
`OnLButtonUp` → `OnDragItem` → repaint is free to change — so the second drag measured whichever
waypoint was under the cursor last. Capturing the uid at press time fixed it, and the map's own
re-scan confirms both waypoints ended up where they were dropped.

*A trace is code. It can be wrong in exactly the way the thing it measures cannot be.*

## The gate asserts a negative

`AllowDragItem()` is `uid in [WayPointBAND, WayPointBANDEND)`. The gate therefore drags the **Wonju
supply dump itself** and requires `allowdrag=0 dragging=0 moved=0`. Without that, a hit-test that
dragged whatever happened to be under the cursor would pass every other assertion in the file.

```
  "Initial Point": allowdrag=1 dragging=1 moved=1
  "Egress":        allowdrag=1 dragging=1 moved=1
  "Wonju":         allowdrag=0 dragging=0 moved=0
  separation "Initial Point" to "Wonju": 4925 m (3.06 miles)
  "Initial Point" redrew 24px from where it was dropped: yes
  "Egress" redrew 4px from where it was dropped: yes
```

Assertion 3 — *the map agrees afterwards* — exists because assertions 1 and 2 cannot see a world
that moved while nothing redrew.

## Residual, named not claimed

The script's step 13 also says *"drag the two AAA waypoints over the target area"*. Those belong to a
**second suppression wave** (gold's timeline shows `2.Flak Supp. 08:20 F86 1 (4)`), not to the
AAA-cover slot of wave 1 — assigning that slot adds a flight to the existing route and creates no new
waypoints. `Ins Wave` fires and produces no route, because a wave with no squadron has no waypoints,
and there are **no spare aircraft** on this save's day one: the same availability arithmetic K7
documented (`F80: 1`, everything else `0`, and K7 has already spent the F80 flight).

So the AAA-waypoint clause is **unreachable on this save**, not broken. Reaching it needs a later
campaign date with more aircraft — which is also the honest way to test K7's F84.

*"The edited route is what the flight flies"* is asserted as far as the data: the edit reaches
`info_waypoint::World`. Actually flying it is **K10–K13**.
