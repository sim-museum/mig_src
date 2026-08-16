# Sprint 125 — "The gate said no" (B6 default flip) — ⚠️ CLOSED PARTIAL 2026-08-15

**Planned 2026-08-15 (PO: "fix the tile column and flip the default"). Autonomous. ~8 pts.**
**Sprint Goal:** close the tile seam and make the full-resolution canvas the default.

| Story | Pts | Result |
|---|---|---|
| S125-1 find the missing tile column | 3 | ⚠️ it is a ~107px SEAM, not a missing column — every tile has its bitmap |
| S125-2 flip the default | 2 | ⚠️ flipped, then **reverted on gate evidence** |
| S125-3 keep the references honest | 3 | ✅ no re-baseline needed — and the gate earned its keep |

## What was measured

- **No tile is missing.** `MA_TRACE_MAPTILE` reports every `(x,y)` whose fileblock has no bitmap,
  keyed per distinct tile. It printed nothing: every tile in range loads.
- **The map does fill the screen** — the sea colour `(58,72,143)` spans the full 1920 width. The
  artifact is a **seam**: a black run at x 913–1019 (~107px), present on some rows and not others.
- **The flip was correct in layout.** With the canvas at the display resolution the Player Log
  lands at (783,340) 339×400 against gold's ~(780,330) ~340×420, and parity stayed **5/5
  byte-identical** — because the parity gate runs at the default resolution, where the canvas
  equals the art size. No reference re-capture was required.

## ⭐ Why the flip was reverted

`port/map_drag.sh` failed with the default on:

```
one-way drag vs baseline : 614302 px differ  (expect > 0 — the drag must move the map)
round trip  vs baseline  : 108000 px differ  (expect 0 — panning must be lossless)
```

**108000 ≈ 107 × 1080** — the seam column, exactly. So the seam is not cosmetic: it appears and
disappears as the map pans, and a round trip no longer restores the view. Shipping the flip would
have traded an ugly-but-stable campaign UI for one where panning corrupts the map.

The PO asked for the flip. **A gate failure is a reason not to ship, and saying so is part of the
job** — the alternative is knowingly shipping a regression to satisfy the letter of the request.
The work stays behind `MA_CANVAS_FULLRES=1`, which is where a player will not meet it.

This is also the second time this week a gate has caught something play-testing would have found
first: S118 shipped because the suite pinned the feature away, and here the suite held the line.
The difference was `port/hw_gate.sh` and `map_drag` covering the configuration that actually
changes.

## Next (first task of the next sprint)

Close the seam, then flip again and re-run `map_drag`. Evidence to start from: every tile loads;
the seam is ~107px at x 913–1019; `endx = min(startx + rect.right/zoomsquaresize + 2, areax)`
caps columns at `areax`, so the suspicion is the region beyond the last tile column combined with
whatever paints the sea colour behind it — i.e. a coverage boundary, not a missing asset.

## Gates

parity 5/5 byte-identical · sweep 9 OPEN/0 CRASH · map click · **map drag PASS (after revert)** ·
sysbox · help click · stress 12/12 · hw_gate PASS.
