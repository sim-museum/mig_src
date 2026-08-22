# Sprint 163 — "The combos were drawn and inert" (EPIC K, K3) — ✅ CLOSED 2026-08-21 (goal MET, 8/8) — ⭐ the whole Task/Payload half of the epic was behind one missing control type

**Planned 2026-08-21** (PO ceremonies pre-approved), taking the K3 item deferred from S162.
**Sprint Goal:** script step 6 — the dossier's Damage tab lists what the Wonju dump is made of.

| Story | Pts | Result |
|---|---|---|
| S163-1 address a TAB by index | 2 | ✅ `#1002:r1`, through `CRTabsCtrl`'s own rect list |
| S163-2 K3: the Damage tab's element list | 3 | ✅ 8 Warehouses ×4 groups + 10 flak sites, `Fully / functional` |
| S163-3 make combos work inside an OOB dialog | 3 | ⭐ they were **drawn and inert** — the widest of this class yet |

## The finding

Clicking the Damage tab's combo did nothing, and the trace said why in one line:

```
[oobclick] swallowed (1737,77) inside dialog rect
```

No control took the point, so the dialog-rect catch-all consumed it. The cause is one line in
`ma_ole_toolbar_click`:

```c
if (h.type != CT_BUTTON && h.type != CT_TABS && h.type != CT_LISTBOX &&
    h.type != CT_RADIO && h.type != CT_SCROLL) continue;
```

**`CT_COMBO` is not in the list.** Every combo box inside every campaign-map dialog has been drawn
and inert for the port's whole life — the same shape as **S87** (listbox rows in OOB dialogs) and
**S140** (`RScrlBar` never hosted), one control type later, and by far the widest: the Wonju
walkthrough's TASKS dialog alone drives **five** of them (Squadron, Attack Method, Attack Pattern,
Group Formation, Escort Position), PAYLOAD another, the frag two more. **K5, K6 and K7 were all
sitting behind this.**

It needed three parts, because a combo is not one click:

1. **the click** — `CT_COMBO` joins the walk; >1 item opens the dropdown, ≤1 keeps the old cycle.
2. **the draw** — an open dropdown must be painted **after the whole OOB tree**, not per dialog:
   drawn inside the per-dialog pass it is painted over by the next dialog in the walk, which is the
   one thing a dropdown must never be. `ma_ole_draw_dropdown` is called once at the end of
   `ma_map_paint_oob`.
3. **the dismiss** — an open list gets **first refusal** on the next click and consumes it either
   way, mirroring the paint order. Windows does not pass the dismissing click through to whatever
   is behind an open combo. (S82's "topmost gets first refusal", one layer higher.)

The row arithmetic is not reimplemented: `ma_ole_dropdown_take` uses the same geometry and fires the
same `TextChanged` as the front-end path in `ma_ole_click`. Two implementations of "which row is
under the cursor" would drift.

## Recipes: `:rN` now means "the Nth item of this control"

One form, resolved by whichever control type is hosting it — and **never a pixel**:

| host | `:rN` means | resolved by |
|---|---|---|
| listbox | the Nth row | the control's own `GetRowFromY` (S162) |
| tab bar | the Nth tab | `CRTabsCtrl::m_rectList`, filled by its own `OnDraw` |
| combo | the Nth row of its **open** dropdown | the geometry the last paint recorded |

The combo form deliberately requires **two entries** — `500,#2398` opens the list, `580,#2398:r0`
picks a row — rather than one scaffold click that opens and selects at once. A recipe that reaches a
result by a route no player can take is the S82 trap, and this project has shipped it before.

An unsatisfiable `:rN` says so and clicks nothing (`"needs its dropdown OPEN first"`,
`"dropdown has N rows, asked for M"`). The unqualified `#ID:rN` form had to be parsed **before** the
generic `#ID:%d`, because `%d` fails on `r0` and the entry would otherwise fall through to the bare
`#ID` match and **silently drop the index** — the click lands on the control's centre and the recipe
still looks like it worked. That is the third time that exact shape has appeared in this parser.

## What step 6 actually shows

Damage tab → the combo lists `Wonju Supply Dump: All elements` / `Wonju Supply Dump: NO DAMAGE`
(two entries, which is what `CDamage::DoDataExchange` builds when `Dead_Stream.DecodeDamage` returns
zero damaged elements — correct for a day-one save). Picking **All elements** fills the list:

```
                        Level     Elements
8 Warehouses            Fully     functional
SB Flak Site (SE)       Fully     functional
SB Flak Site (N)  (W)  (E)  (S)×5 …
8 Warehouses (NE)  (S)  (N)
4 Warehouses (SE)  (SW)  (S)
```

Eight warehouse groups (four of 8, four of 4) and ten `SB Flak Site` rows are visible in the capture — and the list is cut off at the bottom of the screen, so those are lower bounds, not counts. The script's *"Groups of warehouses; you don't know which
hold stores, so plan to hit as many as possible"*, and independent confirmation of the dossier's
*"large AAA presence"*.

⚠ **The list overflows its dialog**, running past the buttons and down over the map. That is
**PO-43** — `CRListBoxCtrl::ResizeToFit` grows the control to hold every row and nothing constrains
it (S155 located it; two fixes tried and reverted). Useful new information: **PO-43 is not
Intelligence-specific**, it is any long list. Not fixed here, and the new gate deliberately does not
assert the list fits, so it cannot start passing for the wrong reason later.

## New gate

`port/damage_elements.sh` — target by name; the tab takes the click; the combo opens with ≥2 rows;
row 0 is selected; **and the element list then carries real row ink** (6,562 px). Without that last
one the first four all pass on a dialog that switched mode and drew nothing.

## Gates (all re-run on the final binary)

| Gate | Result |
|---|---|
| `damage_elements.sh` (new) | **PASS** — tab clicked, combo opened (2 rows), row 0 selected, 6,562 px of row ink |
| `authorize_mission.sh` | PASS — the mission is created and listed |
| `recon_photo.sh` | PASS — 193 colours / 31.8 % top |
| `parity_2d.sh` | 5/5 byte-identical |
| `oob_sweep.sh` | OPEN=9 NONE=0 CRASH=0 |
| `map_icon_click.sh` | PASS |
| `map_filter.sh` | PASS |
| `dialog_scroll.sh` | PASS |
| `help_click.sh` | PASS |
| `sysbox_exit.sh` | PASS — 99.1 % of the map area changed |

Two process notes, because both cost time and would cost it again:

- **A rebuild landed in the middle of the first suite run**, so those results described two different
  binaries. They were discarded and the whole set re-run. A gate result is only a claim about *one*
  binary; if the binary moved under it, the result is not evidence.
- In the batched run `recon_photo` printed no verdict line while every other gate did; run on its
  own it **PASSes**. The suspect is the batch wrapper (a `grep | head -1` inside a `for` loop under
  `gl-lock`), not the gate — recorded rather than shrugged off, because "the gate that prints
  nothing" is exactly how S159's INCONCLUSIVE looked.
