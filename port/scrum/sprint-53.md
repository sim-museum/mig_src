# Sprint 53 — "OOB Squads dialog renders with content" (autonomous, headless DoD)

**Goal:** render the OOB Squads dialog that S52 made *build* (it was invisible — the map idle drove only the
toolbar draw, not the logged-child RDialog paint). Roadmap: BoB S113/S114/S116 (recipe in note 7-05g).

## Implementation (mirrors BoB `bob_map_paint_oob`/`bob_oob_paint_tree`)
- `ma_map_paint_oob()` (MIG.CPP, called each map idle after the toolbar draw): iterates the main toolbar's
  logged-child slots (`m_toolbar2.LoggedChild(slot)`, 0..MAX_CHILD_DIALS); for each open OOB dialog →
  `ma_oob_paint_tree`.
- `ma_oob_paint_tree`: descends the `fchild` chain to the first art-bearing tab page, then renders it fully —
  `ma_oob_render_node` on the tab background + its content dialogs (`fchild` + siblings). Only the first tab
  for now.
- `ma_oob_render_node`: `MaOnPaint()` (background art, self-positioned via `OnGetXYOffset`) +
  `ma_ole_draw_toolbar((void*)node, (void*)1, offX, offY)` (hosted RStatic/RCombo/REdit/RListBox controls).
- Added public `RDialog::MaOnPaint()` / `MaXYOffset()` wrappers (RDIALOG.H, MA_LINUX) — the real
  `OnPaint`/`OnGetXYOffset` are protected `afx_msg` handlers.

## Tree structure (traced, `MA_TRACE_OOB`)
`d0→d1(CSquads)→d2(HTabBox)→d3` = **5 tab-page backgrounds** (`CSqdnListBack`, art 26626–26629, linked via
`sibling`); each tab's `fchild`+sibling = its `CSqdnlist` (roster list) + `CSqdnlistBut` (fields) dialogs.

## Result (headless DoD)
Clicking Squads renders, over the map, the squadron **background photo** + **"Available Aircraft: 0",
"Rotate Flights: Every 2 Days" (RCombo), "Bingo Fuel, lbs: 1500" (REdit)** — real campaign data from hosted
controls. **The OOB info dialog is a working feature.** ASan-clean: load→map→click-Squads→render 0 reports;
`asan_campaign` gate PASS.

## Next (S54)
Selected-tab-only render (CRTabs hosting + tab-click); faithful panel placement; the roster RListBox; then the
Authorise/Directives deeper `OnInitDialog` crashes.
