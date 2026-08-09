# Sprint 82 — "Click the dialogs" (I4/G2) — ✅ CLOSED 2026-08-08 — ⭐ the campaign-map OOB dialogs are INTERACTIVE

**Planned 2026-08-08 (PO pre-approved ceremonies). Autonomous. Committed ~8 pts.**
**Sprint Goal:** act on BoB's S145 inbound (a scaffold that fires OK can hit the panel *wrapper* and
silently skip the derived handler) — check MA for the same trap, and make the OOB dialogs respond to
real clicks.

| Story | Pts | Result |
|---|---|---|
| S82-1 check MA for BoB's panel-wrapper event trap | 3 | ✅ **N/A for MA — measured**, and it found something bigger |
| S82-2 route clicks into open OOB dialogs | 3 | ✅ tick dismisses, tabs switch |
| S82-3 gates + cross-port reply | 2 | ✅ |

## Execution log

### S82-1 — the trap check, and what it actually turned up — DONE
BoB's trap needs the scaffold to hold the *panel wrapper*. Traced it (`MA_TRACE_OOBCLICK` on both
`CPlyr_log::OnOK` and `RDialog::OnOK`): in MA the title bar's owner is **`CPlyr_log`** — the derived
dialog, printed straight from `typeid(*parent).name()` → `9CPlyr_log`. **MA is not affected**,
because the host records each control's *own* parent node rather than the logged child, and
`ma_evt_fire` matches on that node's runtime type.

**But the check exposed a much bigger gap: the OOB dialogs were RENDER-ONLY.** The map idle routed
clicks to the two toolbars and nothing else (`MIG.CPP`, the `ma_ole_toolbar_click` pair) — painted
dialogs got none. So every OOB dialog in the game (Player Log, Squads, Bases, DIS, Overview,
Weather, Mission Folder) drew correctly and **ignored every click**: no tab switching, no tick, no
rows. That is why S61 needed the `MA_OOB_PLAYERLOG_TAB` env hook to prove tab switching, why
`ma_tabs_hit` had sat **declared with no caller at all**, and why BoB's note-18 question about
dismissing a dialog had no in-game answer on this side either — the toolbar route I sent them is the
*scaffold* route, not the user's.

### S82-2 — clicks routed — DONE
- **`ma_oob_click_tree_rec` mirrors `ma_oob_paint_tree_rec` exactly** — same tree walk, same
  `MaXYOffset()` offsets, delivered through the existing `ma_ole_toolbar_click`. Hit rects therefore
  cannot drift from drawn rects. Children are tested before parents (they paint on top).
- **An open dialog gets first refusal on the click**, and a click inside its rect that hits no
  control is **swallowed** — otherwise it falls through and pans the map behind the dialog.
- **The title bar's ✓ now dismisses the dialog, through the DERIVED handler.** A title bar is a
  `CRButtonCtrl` with the tick/help flags set, and the genuine control already owns the band
  arithmetic (`ICONWIDTH` = 22) and fires OK/Cancel/Clicked accordingly. So the port asks the
  control (`MaButtonHit`) instead of inventing regions — the same "drive the real handler, don't
  recompute its inputs" rule that made MA immune to BoB's §8u column bug. Measured chain:
  ```
  [tbclick] id=1001 TITLE local=(327,11) of 336x27 -> dispid 3 (OK) on 9CPlyr_log
  [tbclick] no OK handler registered -> virtual OnOK on 9CPlyr_log
  [oobclick] CPlyr_log::OnOK (DERIVED) reached          <-- the line BoB's trap never prints
  [oobclick] RDialog::OnOK (BASE ...) x5                <-- the EndDialog cascade
  ```
  Capture after the click: the dialog is gone and the operational map renders cleanly.
  `CPlyr_log` registers **no** `ON_EVENT` for `IDJ_TITLE` — it overrides the virtual `CDialog::OnOK`
  — so the fallback calls `OnOK()` *virtually on the owning node*. Calling it on the panel instead
  is precisely BoB's bug, hence the trace that names the class.
- **⚠ Do NOT drive `CRButtonCtrl::OnLButtonUp`.** Its first act is
  `((CDialog*)GetParent()->SendMessage(WM_GETHINTBOX,...))->ShowWindow(...)`, and `ON_MESSAGE` is an
  **empty macro** in the compat layer — the send returns 0 and that derefs NULL. `MaButtonHit` runs
  the DOWN half (which sets the flags and returns early) and reports the dispid the UP half would
  have fired. Same "handler that may not exist" family as the S63 `WM_GETSTRING` bug.
- **Tab bars switch on a real click** — `ma_tabs_click` hits via the control's own
  `m_rectList`/`m_tabList` and calls its own `SelectTab`. Verified: clicking "Log of Missions"
  switches the page (Date / Initial Point / Kills headers, tab raised).
- **Scoped so nothing existing changes:** `ma_button_title_hit` returns −1 for any button without
  the tick/close/help flags — i.e. every toolbar and dialog button already working — and those keep
  firing plain `Clicked` down the identical path. `MA_NO_OOB_CLICK` reverts the routing wholesale.

## Gates — all under `gl-lock`
- **Build:** clean, 0 undefined symbols.
- **2D parity: 5/5 byte-identical** (`title`/`prefs_3d`/`prefs_others`/`quickmission`/`campaign_map`).
- **Stress `stress_launch.sh`: 20/20 PASS.**
- **ASan `asan_all.sh 2 80`: PASS — 0 reports, all 4 paths reached 2/2.**
  The diff touches a shared click path, so the byte-identical parity sweep is the load-bearing gate
  here: it proves the map/toolbar/menu click routes are untouched by the OOB addition.

## Cross-port
- **MA note 31** answers BoB's S145 correction with MA's measured verdict (not affected, and *why*
  structurally), plus the two things this sprint learned that transfer: the compat-`ON_MESSAGE`
  null-deref hiding inside the genuine control's UP handler, and "a check for someone else's bug
  found a bigger one of our own".

## Result
The campaign map's information dialogs are **interactive** for the first time: tabs switch, the tick
dismisses, and stray clicks no longer leak through to the map. `ma_tabs_hit` finally has a caller,
and the `MA_OOB_PLAYERLOG_TAB` scaffold hook is no longer the only way to change a tab.
