# Sprint 127 — "Only the axis with no room" (B6 shipped) — ✅ CLOSED 2026-08-15 (goal MET)

**Planned 2026-08-15 (PO: keep going). Autonomous. ~8 pts.**
**Sprint Goal:** make panning lossless at full resolution and ship B6.

| Story | Pts | Result |
|---|---|---|
| S127-1 why is panning not lossless? | 4 | ✅ the scroll axis with no room is never clamped |
| S127-2 fix it | 2 | ✅ round trip 0 px differ |
| S127-3 flip the default | 2 | ✅ shipped, whole suite green |

## ⭐ Measure the state, not the picture

Two sprints described this artifact three different ways — "a missing tile column", "a 107px seam",
"a stale canvas repaint" — all from looking at pixels. The answer came from printing the scroll:

```
baseline    scroll (0,642)   of max (0,2139)
one-way     scroll (0,562)   of max (0,2139)     <- moved, correct
round trip  scroll (100,642) of max (0,2139)     <- Y restored; X is 100 with a maximum of 0
```

**The Y axis, which has scroll room, round-tripped perfectly. Only the X axis, which has none,
did not.** Once the map is zoomed to cover the client there can be zero horizontal room
(`m_size.cx == client width`), and the engine clamps only the low end: the drag left is held at 0,
the drag back adds its full delta unopposed, and the map ends at x=100 against a maximum of 0.

Clamping the scroll to the map's real extent on every paint — not just on resize — makes the round
trip exact. `map_drag`: **0 px differ**.

## B6 is shipped

The full-resolution canvas is now the default (`MA_CANVAS_ARTSIZE=1` restores the old art-sized
behaviour). At 1920×1080 the campaign map fills the screen and the Player Log lands at (783,340)
339×400 against gold's ~(780,330) ~340×420. **PO-17 closes with it**: the dialogs were never
misplaced, they were drawn on a canvas a third of the intended size.

Parity stayed 5/5 byte-identical throughout, because the gate runs at the default resolution where
the canvas still equals the art size — so shipping this did not cost the regression net.

## What the two vetoes bought

S125 and S126 both flipped this default and both reverted it on gate evidence. Either would have
shipped a campaign map that looks correct and corrupts when dragged. The gate that caught it —
`map_drag`'s round-trip assertion — is worth more than the three sprints it delayed.

## Gates (flip ON)

parity 5/5 byte-identical · map drag PASS · sweep 9 OPEN/0 CRASH · map click · sysbox · help click ·
stress 12/12 · hw_gate PASS.

## Next

PO-18 (map zoom "produces tiles") should be re-checked — it may have been the same canvas scaling.
Then PO-19 (recon zoom keys) and confirming terrain in the recon view.
