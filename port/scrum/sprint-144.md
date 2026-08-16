# Sprint 144 — "Two widgets in one corner" (PO-21) — ✅ CLOSED 2026-08-16 (goal MET)

**Planned 2026-08-16, continuing the gold-comparison method.**
**Sprint Goal:** the campaign map's upper-right controls respond to clicks.

| Story | Pts | Result |
|---|---|---|
| S144-1 measure gold's chrome layout | 3 | ✅ date 0–280, filters from x≈300 |
| S144-2 stop the two toolbars sharing a corner | 3 | ✅ filters go beside the date |
| S144-3 the gate had the old position baked in | 2 | ✅ sweeps the full width now |

The PO, twice: *"most of the controls at upper right do nothing when clicked, and there are two
'X' buttons"*, and later *"red and blue buttons at upper right do nothing"*. PO-21 had been open
for several sprints, blocked on an unexplained 659px discrepancy between where the menu was drawn
and where the registry said it was.

## The gold capture answered it in one look

Cropping the gold campaign map's top strip and measuring:

| element | gold x |
|---|---|
| "MIG ALLEY" + date plate | 0 – 280 |
| **two filter rows** | **≈300 – 680** |
| main toolbar | 700 – 1200 |
| right-hand group | 1230 – 1460 |
| system box | 1855 – 1915 |

The filter rows sit **immediately right of the date**. S109 instead right-aligned them to the
canvas edge — for a good reason at the time, to keep them off the date readout — and at 1920 that
puts them in the **system box's** corner. The two overlap, and a click lands on whichever the walk
reaches first. That is both halves of the report: controls that "do nothing", and "two X buttons"
(the system box's close, plus the filter grid's own right-hand column drawn under it).

Placing them beside the date fixes it at 1920 **and** at 800×600, where the grid's last column
becomes visible for the first time. Right-alignment is kept only as the fallback for a canvas too
narrow to fit the rows beside the date.

## A gate that encoded a position

`map_filter.sh` probed x 1500..1910 for the red "all" button, because that is where the row was
when it was written. Moving the row made it report the feature broken. It now sweeps the full
canvas width and reads back the id the toolbar reports — which is what it should have done
originally, and what its own header says about hardcoded coordinates. Third gate this week to
fail on a correct change; the pattern is always the same, and always mine.

## Evidence

`port/ref/native/map_chrome.png` beside `port/ref/gold/map_chrome_gold.png`: date, filter rows,
main toolbar and system box, none overlapping.

## Gates

parity 5/5 byte-identical (campaign_map rebased — the chrome moved deliberately) · sweep 9 OPEN/0
CRASH · map icon click · map drag · map filter (button now located at x=396) · sysbox exit (83.1%).
