# Sprint 130 — "Seeing it and pressing it are two fixes" (PO-21 / Fly) — ✅ CLOSED 2026-08-15 (goal MET)

**Unplanned, PO-directed: "get fly working". Autonomous. ~5 pts.**
**Sprint Goal:** the campaign briefing shows MAP / FLY / PREFERENCES and the player can press Fly.

## What was left after S129

S129 filtered stale hosted controls out of the paint, but two menus still overlapped. The reason is
one line of it: the filter only applied to **template-relative** controls, and the front-end menu
(`IDC_RLISTBOX`) is **game-positioned** via `PositionRListBox` — absolute, not relative. A control
belongs to a panel however it was positioned, so the condition was simply wrong. With it dropped,
the briefing shows exactly one menu: **MAP FLY PREFERENCES**, matching the gold video's briefing
screen (`port/ref/native/briefing_fly.png`).

## ⭐ Drawing it is only half

S128 centres a panel's art on the larger canvas and offsets its controls to match. The **click**
path had no such offset, so the menu would have rendered in the middle of the screen and responded
only in the top-left corner where it used to be — visible, and dead. That is the S82 lesson in a new
place: *the click walk must mirror the paint walk.* Hit-testing now adds the same panel origin.

Verified at 1920×1080: a title-menu click reports `OnSelectRlistbox row=0 col=0` and navigates, so
paint and hit-test agree at the resolution where the offset is non-zero.

## Gates

parity 5/5 byte-identical · sweep 9 OPEN/0 CRASH · map click · map drag · sysbox · help click ·
stress 10/10.

## Honest status

The Fly ACTION was never broken — the campaign automation has driven
`frag -> singlefrag -> Fly -> StartFlying -> Launch3d` since S123. What was broken was the player's
ability to see and hit it. Both halves are fixed and gated; interactive confirmation is the PO's.

Still open on that screen: a stray "DEBRIEF" line from another panel's menu, and the upper-right
controls the PO reports as dead (including two X buttons).
