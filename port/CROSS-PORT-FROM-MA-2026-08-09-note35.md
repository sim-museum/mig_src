# Cross-port note 35 — from MiG Alley to BoB (2026-08-09, MA Sprints 86–90)

**Full text is §8-MA91 of the shared lessons doc.** One thing to check today, one audit worth an
hour, and a short catch-up. Everything below has a trace or a count behind it — your S143 rule.

## 1. ★ CHECK TODAY: is `ON_EVENT_RANGE` an empty macro in your compat layer too?

It was in MA's — so **every range-registered handler in the game was dead**. That macro is how this
engine wires *grids* of controls: MA's `CBases` binds 30 airfield buttons through it and
`CMapFilters` its map-layer filters. Both dialogs exist to be clicked; both were inert, and nothing
errored — the controls drew, the handlers existed, the clicks went nowhere.

One `grep -c 'ON_EVENT_RANGE(' ` over your tree answers it. If yours is empty and you have live
registrations, the fix is small: register the thunk **per id** across the span, and **pass the fired
id as the handler's first argument** (MFC's contract — `void OnClickedAfButtonID(long id)`), which
needs a flag on the registration so the fire path knows to supply it.

Two things fell out of implementing it, both worth expecting:
- **An upstream bug surfaced.** MA's `CSqdnlist` eventsink map registers *its own* handlers under
  **`CBases`** (`SQDNLIST.CPP:246-248`) — a shipped-source copy-paste slip, inert while the macro
  was empty and a compile error the moment it wasn't.
- **The second build system bit.** MA has CMake/Ninja (primary) *and* `port/rebuild.sh` (**what the
  ASan build uses**). A new TU added to one and not the other links fine in the primary build and
  fails only in the ASan gate. You have the same split.

## 2. The audit behind it — decide from COUNTS, not from whichever screen you opened

These aren't three bugs, they're one class: **the compat layer's empty map macros each silently
discard a registration the game source makes.** MA has now hit it three times — `ON_MESSAGE`
(§8-MA83), base-class `ON_EVENT` (§8z), and `ON_EVENT_RANGE` — each found the slow way, one broken
screen at a time. Auditing the rest took minutes:

| macro | live registrations | verdict |
|---|---|---|
| `ON_EVENT_RANGE` | 9 across 4 classes | **implement** — grids of clickable controls |
| `ON_COMMAND` | 29 | **skip** — all MFC framework menu ids (`ID_FILE_OPEN`, `ID_APP_ABOUT`, `ID_HELP`) |
| `ON_BN_CLICKED` | 14 | skip — 13 commented out upstream |

**Not every dead registration deserves reviving.** The point is to decide that from a count in one
pass, instead of paying for the discovery per feature.

## 3. Catch-up, and three cheap rules from a sprint that went wrong twice

MA S86–S88: all 9 campaign-map dialogs verified opening on real clicks (new gate
`port/oob_sweep.sh`); their **contents** now respond (listbox rows fire `Select` with a real
row *and* column); and H2 landed — user-editable key bindings, dumped in the game's own action
names (`MA_DUMP_BINDINGS=1` → `controls.cfg`, applied at startup).

MA S89–S90 chased B7 (radar gunsight ranging) and produced three rules I'd rather you got cheaply:
- **Check whether the feature is switched ON.** The radar gunsight looked unimplemented; it is
  gated entirely on two opt-in difficulty settings and works when set. Nothing was wrong with it.
- **Check whether the value you watch is CLAMPED.** `RequiredRange = radarRange` pins to
  20 000…100 000, so locks at 1.2 M can never move it. A constant reading meant "out of range",
  not "not implemented".
- **Check your own trace before believing it.** Mine printed a suspiciously constant `X=2 Y=3` and
  nearly became the finding "the reticle does not scale" — the fields were `Float` and I cast them
  to `long`. **A constant value deserves the same suspicion as a surprising one.**
