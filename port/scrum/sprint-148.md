# Sprint 148 — "The hardware path still flies" (verification) — ✅ CLOSED 2026-08-16

**Planned 2026-08-16, after S145 resized the map view and S146 began destroying panels.**
**Sprint Goal:** confirm the renderer the PO actually plays on is unharmed by the night's work.

| Story | Pts | Result |
|---|---|---|
| S148-1 hardware flights | 3 | ✅ 5/5 reached and sustained 100 3D frames |
| S148-2 hardware campaign map | 2 | ✅ reached and presented, no SIGSEGV |
| S148-3 stop losing time to the display lock | 1 | ✅ warning added where it belongs |

## Why

The PO plays on the **hardware** renderer, and two of tonight's changes touch things the 3D path
shares: S145 resized the map view (from the frame's border-inset rect to the whole canvas), and
S146 started destroying panel objects that used to be leaked. Neither is covered by the software
gates.

## Result

- **Flights: 5/5** reached and sustained 100 3D frames with `MA_NO_HARDWARE=0 MA_TRY_HARDWARE=1`.
- **Campaign map on hardware:** title → Campaign → load → map, frame presented at 600, exit 0, no
  SIGSEGV.

Together with S147's clean ASan run over the campaign path, that is the coverage this many changes
in one night warranted.

## The display lock, twice

`hw_gate.sh` takes the display lock **itself** for its parity arm, and `flock` is not reentrant —
so `gl-lock port/hw_gate.sh` deadlocks until the timeout kills it, and leaves an **orphaned lock
holder** that then blocks every later GL run. That is what silently stalled two attempts and one
unrelated capture tonight, and it needed `fuser -v /tmp/.gl-display.lock` and a manual kill to
clear.

The warning now sits in `hw_gate.sh`'s own header, where the next person will read it before
running it. `map_filter.sh` hit the same trap earlier tonight and already carries the note.

*Two of the three times I lost this evening went to a tool's own conventions rather than to the
game: nesting the lock, and `pkill -f <pattern>` matching my own waiting shell — which
`CLAUDE.md` has warned about since Phase 5.1.*

## Gates (full suite, current)

parity 5/5 byte-identical · sweep 9 OPEN/0 CRASH · map icon click · map drag · map filter ·
sysbox exit · help click · dialog scroll · panel click · stress 10/10 software · **stress 5/5
hardware** · **campaign map on hardware** · ASan campaign path clean.
