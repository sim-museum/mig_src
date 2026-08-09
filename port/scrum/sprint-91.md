# Sprint 91 — "Send the findings" — ✅ CLOSED 2026-08-09 (cross-port delivered; B7 attempt = another honest negative)

**Planned 2026-08-09 (PO pre-approved ceremonies; continuous-sprint directive). Autonomous. ~6 pts.**
**Sprint Goal:** clear the cross-port debt from S86–S90 (BoB had received nothing since note 34),
and make one more attempt at B7's final observation.

| Story | Pts | Result |
|---|---|---|
| S91-1 cross-port catch-up | 3 | ✅ §8-MA91 + note 35 |
| S91-2 B7 close-range lock attempt | 3 | ✗ **negative result, recorded** |

## Execution log

### S91-1 — the debt, and the reason it was worth paying first — DONE
BoB had nothing from S86–S90, and one item is **actionable for them today**:
**`ON_EVENT_RANGE` was an empty macro in MA's compat layer**, so every range-registered handler in
the game was dead. It is how this engine wires *grids* of controls — `CBases`' 30 airfield buttons,
`CMapFilters`' map-layer filters — so two dialogs whose entire purpose is being clicked were inert,
with nothing erroring. BoB uses the same compat approach and can check it with one `grep -c`.

The section (**§8-MA91**) frames it as a **class, not a bug**: the compat layer's empty map macros
each silently discard a registration the game source makes, and MA has now hit it three times
(`ON_MESSAGE` §8-MA83, base-class `ON_EVENT` §8z, `ON_EVENT_RANGE`) — each found the slow way, one
broken screen at a time. So it carries the audit MA should have done earlier, with counts:

| macro | live registrations | verdict |
|---|---|---|
| `ON_EVENT_RANGE` | 9 across 4 classes | implement |
| `ON_COMMAND` | 29 | **skip** — all MFC framework menu ids with no port equivalent |
| `ON_BN_CLICKED` | 14 | skip — 13 commented out upstream |

**Not every dead registration deserves reviving**; the point is deciding that from a count in one
pass rather than paying per feature. Note 35 also carries the two-builders trap, and S89/S90's three
rules (is the feature switched on; is the value clamped; is your own trace lying).

### S91-2 — B7: a third negative — RECORDED, NOT DRESSED UP
Attempt: inject a sustained pitch-down (60 `ELEVATOR_FORWARD` taps via `BOB_KEYSEQ`) with ground
lock on, to bring the close ground objects the trace *does* see (7 500–8 200 units) into the radar
cone. Result: `RequiredRange=100000`, **one distinct value** — unchanged.

So across four flights and three approaches (air targets, ground lock, forced dive), **every lock is
still ~1.2 M and nothing inside the 20 000–100 000 clamp window has entered the cone.** That
consistency is itself the finding: the problem is not how the aircraft is flown but *what is near
it*. The next attempt should change the **scenario** — a mission that starts within gun range, or
driving the padlock's selected target into the radar path — rather than the flying.

**B7 remains open.** Three sprints have added hooks and eliminated wrong observables without
producing the acceptance evidence; that is worth stating plainly rather than reporting motion as
progress.

## Gates
- **No source diff this sprint** (`git diff HEAD -- SRC/` empty — the only changes are the shared
  doc and the note), so the binary is the one S90 gated and the gate set was **not re-run for a
  build that cannot have changed**. Stated, not implied.
- `tools/check_notes_sync.sh` ✓ — both copies of the shared doc byte-identical.

## Result
The cross-port channel is current again, and BoB has the one finding most likely to be live in their
tree. B7 gained a fourth negative data point and a clearer next step, and no claim it does not
support.
