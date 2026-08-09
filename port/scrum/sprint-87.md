# Sprint 87 — "Pick a row" — ✅ CLOSED 2026-08-09 — ⭐ dialog CONTENTS respond: listbox rows select, and range-registered buttons exist at all

**Planned 2026-08-09 (PO pre-approved ceremonies; continuous-sprint directive). Autonomous. ~8 pts.**
**Sprint Goal:** S86 proved every campaign-map dialog *opens*. This sprint makes what is inside them
respond.

| Story | Pts | Result |
|---|---|---|
| S87-1 route listbox row clicks in OOB dialogs | 3 | ✅ + `ON_EVENT_RANGE` implemented |
| S87-2 verify a selection has a visible effect | 3 | ✅ measured, 700 px on exactly one row |
| S87-3 gates | 2 | ✅ |

## Execution log

### S87-1 — rows, and then a bigger gap — DONE
`ma_ole_toolbar_click` handled `CT_BUTTON` and `CT_TABS` and **skipped `CT_LISTBOX`**, so every row
in Bases / Squads / D.I.S. / Intelligence was inert: the dialogs listed real campaign data that could
not be selected. Added an offset-aware listbox branch that drives the control's genuine
`OnLButtonDown/Up` (`MaMouse`) so **its own** logic picks row and column, then fires `Select` with
**both** args — the rule that kept MA clear of BoB's hardcoded-column bug (§8u). The front-end path
(`ma_ole_listbox_click`) could not serve these: it assumes absolutely-positioned listboxes, while an
OOB dialog's are drawn at the walk's `(ox,oy)`.

**Then Bases turned out not to use a listbox at all** — its airfield rows are `IDC_AFBUTTON1..30`
registered with `ON_EVENT_RANGE`… **and `ON_EVENT_RANGE` was an empty macro in the compat layer.**
So every range-registered handler in the game was dead. There are 9 live registrations across 4
classes, including `CBases`' 30 airfield buttons and `CMapFilters`' map-layer filters — two dialogs
whose entire purpose is being clicked. Same family as S83's empty `ON_MESSAGE` and §8z's
base-class-registered `ON_EVENT`: **the registration exists in the game source and the port silently
dropped it.**

Implemented: the macro registers the thunk for each id in the span, and `ma_evt_fire` passes the
**fired id** as the handler's first argument (what MFC does for a range handler,
`void OnClickedAfButtonID(long id)`). Registration is per-id so the fire path stays a plain lookup.
Confirmed live at runtime: `ids 2420..2478 CBases`, `1015..1046 CCommsPaint`, `2350..2397 CSqdnlist`.

**An upstream bug fell out of it.** `CSqdnlist`'s eventsink map registers *its own* handlers under
**`CBases`** (`SQDNLIST.CPP:246-248`) — a copy-paste slip in the shipped source. It was inert while
the macro was empty; implementing the macro turned it into a compile error, which is how it
surfaced. The enclosing `BEGIN_EVENTSINK_MAP(CSqdnlist, …)` says what was meant.

### S87-2 — the visible effect — DONE
`#2023@CMainToolbar` then `#2018@CSupply`:
```
[tbclick] id=2023 rect=(229,52,48,48) -> fire
[tbclick] listbox id=2018 local=(243,116) -> row=7 col=1 on 7CSupply
```
Measured against the pre-click capture: **700 px changed, bounded to y 353-363** — one row band. The
clicked row ("Kilchu") goes from the list's yellow to the selection white. Real effect, correctly
scoped, and the column argument is genuine rather than a hardcoded 0.

### ⚠ A parser bug this sprint's own test found
The two-step recipe failed at first, and re-running `oob_sweep.sh` (which passed) proved the code was
fine and the **recipe** was not. Cause: S85's `#ID@Class` parser used `%63s` for the class name, and
`%s` runs to whitespace — so with a **following** step it swallowed
`CMainToolbar;340,#2018@CSupply` whole and the step silently never matched. Scanset now excludes
`;` and `:`. Only reproducible with a 5th sequence entry, which is exactly why the 4-entry sweep
recipes never showed it. *A test harness needs the same suspicion as the code it tests.*

## Gates — all under `gl-lock`
- **2D parity: 5/5 byte-identical.**
- **OOB sweep: 9 OPEN / 0 CRASH** — now run as a regression gate, not just a one-off report.
- **Stress: 20/20 PASS.**  **ASan `asan_all.sh 2 80`: 0 reports, all 4 paths 2/2.**
- The diff touches the shared eventsink (every registration in the game now goes through the
  extended `EvtEntry`), so parity + sweep together are the load-bearing evidence that nothing
  previously working changed.

## Result
The campaign map's information layer is now not just visible but *usable*: rows select with real
row+column, and an entire class of registrations that the port had silently discarded is live.
