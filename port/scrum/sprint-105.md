# Sprint 105 — "The map's own words" (PO-6) — IN PROGRESS 2026-08-15

**Planned 2026-08-15 (PO pre-approved ceremonies; continuous-sprint directive). Autonomous. ~8 pts.**
**Sprint Goal:** the in-flight map window shows its text — the waypoint table, the clock/waypoint
name, and the map command list — matching the gold video at ~90 s.

| Story | Pts | Result |
|---|---|---|
| S105-1 reach the screen the gold actually shows | 3 | |
| S105-2 make its text appear | 3 | |
| S105-3 gate line for the map screen | 2 | |

## The lead (S104)

The gold's map view is **`waypointMapScr`**, not the `mapViewScr` that `M` opens:

- its option list is exactly the gold's right-hand panel — `IDS_MAP_SETNEXTWP`
  ("1.NextWP=HighlightedWP"), `IDS_MAP_ACCELTONEXTWP` ("2.AccelToNextWP"), `IDS_MAP_EXIT`
  ("0.Exit");
- its `extraRtn` is `MapScr::UpdateWaypointDisplay`, which draws the Rendezvous / Ingress /
  Initial-Point table with altitude, ETA, bearing and range — the gold's bottom strip;
- it is reached from `firstMapScr` option **2** (`MapScr::SelectFromFirstMap`, `SEL_2`).

So the drive is `M` (GOTOMAPKEY, DIK 0x32) then `2` (RPM_20, DIK 0x03), captured with
`MA_UISCR_SHOT` — the armed capture from S104, since these screens are event-shaped.

`UpdateWaypointDisplay` prints nothing when `OverLay.curr_waypoint` is NULL. `FirstMapInit` sets it
from `Manual_Pilot.ControlledAC2->waypoint`, so a Hot Shot mission with no waypoints would produce
an empty table legitimately — check the campaign path too before calling an empty table a defect.
