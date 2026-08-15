# Sprint 107 — "Press it when it's there" (PO-13) — ✅ CLOSED 2026-08-15 (goal MET)

**Planned 2026-08-15 (PO pre-approved ceremonies; continuous-sprint directive). Autonomous. ~8 pts.**
**Sprint Goal:** find out why an option key inside an in-flight menu never selects — and reach the
screen the gold video actually shows.

| Story | Pts | Result |
|---|---|---|
| S107-1 name the consumer of the option key | 3 | ✅ the throttle, **before the menu existed** — correct game behaviour |
| S107-2 make the key arrive while the menu is up | 3 | ✅ `MA_UISCR_KEY` — a key press armed by the screen opening |
| S107-3 the gold's waypoint screen, with its table | 2 | ✅ captured; new gate line |

## ⭐ PO-13 was never a game defect

`KeyPress3d` is a test-and-CLEAR, so the only honest way to find who eats a key is to watch the
function itself: `MA_TRACE_KEYEAT=<action index>` logs every call for one action and whether it
found the bit set. One line settled it:

```
[keyeat] KeyPress3d(106) bit=1 ret=1        <- the digit was consumed HERE
[uiscr] promote firstMapScr (0x8552880)     <- ...and the menu opened AFTER that
```

The `2` arrived **before the map menu existed**, so `KEYFLY.CPP`'s throttle handler took it — which
is exactly right (its `if (!OverLay.pCurScr)` guard is there for this, and it holds once a screen is
up). Every other measurement in S104/S105 had been the same story: the taps were landing outside the
menu's lifetime.

**The real fault was in the harness.** `BOB_KEYSEQ` schedules taps on the **pump** counter, and in
flight pumps run far slower than frames — taps 20 pumps apart land *seconds* apart, and these menus
live five seconds. No pump number can reliably hit the window.

**Fix: `MA_UISCR_KEY="0xNN[,frames]"`** — arm a key press when a UI screen is promoted, fire it N
frames later, through the same buffered-keyboard queue real input uses (`ma_inject_dik`). The input
twin of S104's `MA_UISCR_SHOT`. With it, first try:

```
[uiscr] injecting armed key dik=0x03
[uiscr] option key=1 (KeyVal3D 106) selected -> pNewScr=0x8554680
[uiscr] promote waypointMapScr (0x8554680) over firstMapScr (0x8553880)
```

## ⭐ And that lands on the gold's screen

`waypointMapScr` — the screen the PO's video shows at ~90 s — now renders completely
(`port/ref/native/map_waypoints.png`):

- right panel: **"1.Next WP = Highlighted WP" / "2.Accel To Next WP"** (red) **/ "0.Exit"**
  — the gold's list;
- bottom strip: the **waypoint table**, "Waypoint (1) 12000ft 0:00 355 5.1Nm" / "Waypoint (2) …"
  — the gold's Rendezvous/Ingress/Initial-Point table, with this Hot Shot mission's own (unnamed,
  zero-ETA) waypoints instead of a campaign mission's;
- top-left: the clock + place, "9:00 E. Pyongyang City".

So **PO-6 is complete, not just partial** — the residual it was left with was this harness gap.

## Gate

`port/overlay_text.sh` gains the `waypoint` screen (letters 1179 edges vs **0** with
`MA_NO_ALPHATEXT=1`). Three screens now: radio · waypoint · info line.

## Gates

parity 5/5 · sweep 9 OPEN/0 CRASH · map click · map drag · sysbox exit · help click ·
overlay text 3/3 · stress 20/20 · ASan 0.

## Result

Two sprints had recorded "the option key does not select" as an open defect. It was the measuring
apparatus. The general lesson is now recorded twice in two forms — **arm the capture from the
drive** (S104) and **arm the input from the drive** (S107): whenever an event-shaped screen is
involved, anything scheduled on an unrelated counter is a coin toss.
