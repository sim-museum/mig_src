# Sprint 109 — "The art was always there" (PO-14 → PO-11) — ✅ CLOSED 2026-08-15 (goal MET) — ⭐ the campaign map has its filter toolbars

**Planned 2026-08-15 (PO pre-approved ceremonies; continuous-sprint directive). Autonomous. ~8 pts.**
**Sprint Goal:** clear the blocker S108 named, and give the campaign map the widgets the PO is
missing.

| Story | Pts | Result |
|---|---|---|
| S109-1 fix the double-open that blocked design-time button art | 3 | ✅ `CMIGView::DrawIcon` |
| S109-2 apply the art without S57's caption regression | 3 | ✅ art and captions split into two predicates |
| S109-3 lay the chrome out and route its clicks | 2 | ✅ filter rows + misc toolbar drawn and clickable |

## ⭐ Three findings, one visible result

**1. The double-open opener was none of the ones I guessed.** S108 left `MA_BTN_ART_ALL=1` crashing
on `Opened file block (6a48) again without closing`. I guessed `CRToolBar::OnGetFile`, then
`CMIGView::OnGetFile`, guarded both, and it still crashed — two runs wasted. `MA_TRACE_FILEOPEN`
(added in S84 *for exactly this*, with the note "print the stack rather than reason from the call
graph") named it in one run:

```
fileblock::fileblock(FileNum,...)
CMIGView::DrawIcon(void*, int, int, FileNum)
CMIGView::DrawIcons(...)
CMIGView::UpdateBitmaps(CDC*)
```

`DrawIcon` constructs a stack-local `fileblock` per map symbol per frame. Harmless until something
else holds that art — and the map-filter toolbar caches its button icons for the life of the
process, from the *same files*: `FIL_ICON_R_SUPPLY_ON` (0x6a48) is both a map symbol and the red
supply filter button. **I had the right instrument, written by a previous sprint for this exact
failure, and reached for guesses first.**

**2. Art and captions were one predicate, and only the captions were dangerous.** The design-time
bag carries both, and `ma_dlg_artnum()` gated both — so S57's caption regression (system-box buttons
materialising as "Quit"/"Size", doubled art captions) forced the art off too. Split:
`ma_dlg_artnum_any()` applies art, `ma_dlg_artnum()` still gates captions to the tickbox family.
The filter toolbar's 30 buttons get their icons; not one caption changed.

**3. Widening the art exposed a ghost S97 had only half-fixed.** With art, the system box appeared
**twice** — correctly on the map, and again at its raw template origin in the top-left corner of the
*title* and *Preferences* screens (parity caught it immediately: 4 screens differing in exactly
rows 0–47, cols 0–71). S97 diagnosed this and registered the box as parent-scoped *at first map
draw*, which leaves every screen before that exposed; it stayed invisible only because the buttons
had no art. The four map-chrome dialogs are now registered scoped **at creation**, and the
S97 note's own caveat — that the toolbars escape the global pass only because their parent happens
to be created hidden — no longer stands unaddressed.

## Layout

- Filter rows (2×15, extent 393×48) → **right-aligned at the top edge**, so the map's date readout
  keeps the top-left corner it has always had. Drawing them at x=4 covered it — the same rule S97
  recorded: *a widget must not change the state of the screen it draws on*.
- Main toolbar stays at (4,52); the previous (4,26) placement **overlapped it by 22 px**, which is
  why the red row looked like one stray icon.
- Misc toolbar (6 buttons) → right-aligned on the main toolbar's band, from its own extent.
- Clicks follow the paint offsets for both (a widget that draws but cannot be clicked is half
  hosted — S106).

## Gates

parity **5/5 after a justified rebaseline of `campaign_map`** — the diff is confined to rows 4–99,
cols 451–747 (the chrome band; **nothing below row 100 changed**), and the other four screens went
back to byte-identical once the ghost was fixed, which is what proves the widening is contained ·
sweep 9 OPEN/0 CRASH · map click · map drag · sysbox exit · help click · overlay text 3/3 ·
stress 20/20 · ASan 0.

## Result

The campaign map now carries the chrome the gold video shows: the blue and red filter rows with
their icons, the main toolbar, the misc toolbar and the system box — five clusters where S108 found
one and a half. PO-11's remaining item is the scale ruler (`CScaleBar`, 0 hosted controls: it draws
itself and nothing calls it), plus any layout question that only B6 can settle.
