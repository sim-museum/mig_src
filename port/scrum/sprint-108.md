# Sprint 108 — "Count the widgets first" (PO-11) — ⚠️ CLOSED PARTIAL 2026-08-15 — inventory delivered, one blocker named

**Planned 2026-08-15 (PO pre-approved ceremonies; continuous-sprint directive). Autonomous. ~8 pts.**
**Sprint Goal:** turn "many widgets are missing" into a work list with a mechanism per item.

| Story | Pts | Result |
|---|---|---|
| S108-1 make a gold-vs-native comparison legitimate | 3 | ✅ measured — and it is **not** legitimate yet; see below |
| S108-2 inventory the campaign map's widgets with a cause each | 4 | ✅ five clusters, three distinct mechanisms |
| S108-3 fix the cheap ones | 1 | ⬜ **not done** — every one needs the art mechanism first |

## The resolution question, settled by measurement

The gold recordings are **1280×1024** (short video) / 1200×1080 (full). This engine picks its panel
art set by resolution, so judging "missing" across that boundary is the exact mistake S64 recorded.
New hook `MA_FORCE_RES=WxH` selects a mode by writing the two fields the resolution combo writes
(`Save_Data.displayW/H`, consumed by `DDRWINIT`), so the question could be asked directly.

**Answer: the port's 2D canvas stays 800×600 whatever mode is selected** — `MA_FORCE_RES=1280x960`
still captures 800×600. That is B6 (high-res 2D layer, ⬜ not started) and it means **a faithful
widget comparison against these golds is blocked on B6**, not on the widgets. Worth knowing before
anyone spends a sprint diffing screenshots.

## The inventory (measured, `MA_TRACE_TOOLBARS=1`)

| cluster | hosted controls | extent | state | mechanism |
|---|---|---|---|---|
| filters `m_toolbar1` (gold: blue + red icon rows) | **30** | 393×48 | drawn, **blank** | buttons have no art: `ma_button_apply_icon` maps ids by hand and only knows the main toolbar + system box |
| main `m_toolbar2` | 10 | 529×48 | ✅ correct | ids hand-mapped (S48/S49/S97) |
| misc `m_toolbar3` (gold: zoom in/out, save…) | **6** | 264×48 | **never drawn** | enumeration gap — the map idle draws t1 and t2 only |
| scale bar `m_toolbar4` (gold: 0–350 Nm ruler down the left) | **0** | — | not hosted at all | `CScaleBar` is not an OCX-control dialog; it draws itself and nothing calls it |
| debrief `m_toolbar5` | 6 | 270×48 | ✅ since S106 | was the same enumeration gap |

Two further facts fall straight out of the extents: the three top clusters need **393+529+264 ≈
1190 px** of width, which is why gold lays them side by side at 1280 and why they cannot all fit the
port's 800-wide canvas. And the port currently draws t1 at y=26 and t2 at y=52 — **they overlap by
22 px**, which is why the filter rows look like one stray icon rather than two rows.

## The blocker, with evidence

The buttons' art is a **design-time property** the template parser already extracts
(`ma_dlg_artnum` → `artmap`), but S57 found that applying it to every button regressed live screens
(toolbar/system-box buttons whose art is runtime-managed drew their design-bag state), so it is
restricted to `FIL_ICON_TICKBOX*`. The re-widening switch has been sitting there since — and it
**crashes**:

```
MA_BTN_ART_ALL=1 → [SysError] Opened file block (6a48) again without closing!
```

That is the S79/S84 double-open family (one open per FileNum; two owners of the same art file →
`SayAndQuit`). So "give the filter toolbar its icons" is really "make button art resolvable without
double-opening the art file", and it wants its own sprint rather than a patch at the end of this one.

**Deliberately not done here:** drawing `m_toolbar3` before its art works would have added six blank
rectangles to the map. S94 already recorded the rule — *positioned and clickable but invisible is
not a fix* — and the same rule applies to a widget that is merely present.

## Gates

parity 5/5 · sweep 9 OPEN/0 CRASH · map click · map drag · sysbox exit · help click ·
overlay text 3/3 · stress 20/20 · ASan 0. (This sprint added one read-only counter, one trace and
one env hook; no render path changed.)

## Result

PO-11 goes from "many widgets are missing" to five named clusters with a mechanism each, one
measured blocker (the art double-open), and one honest dependency (B6, for any pixel comparison
against these golds). The next sprint has a single clear target.
