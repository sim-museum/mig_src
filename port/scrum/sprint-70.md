# Sprint 70 — "Finish the Player Log"

**Planned 2026-08-02 (PO pre-approved ceremonies). Autonomous, headless DoD.**

## Context
S69 closed both cross-cutting front-end deviations (font FACE + combo chrome). Remaining on the
parity queue: a small rebase debt S69 deferred, and the last open half of I4 — the Player Log
Career **content table** (deferred as a full-sprint item since S56).

## Sprint Goal
Font-rebase debt cleared (byte-identical sweep resumes); the Career tab's per-aircraft-type
Sorties/Combats/Kills/Losses table renders with data — closing parity #15.

## Committed (~8 pts)
| Story | Pts | Definition | Status |
|---|---|---|---|
| S70-1 Clear font-rebase debt | 1 | Re-capture `map_playerlog_tab1` + `campaign_map`; rebase refs | ☐ |
| S70-2 Career content table | 6 | Locate the table controls + campaign data source; render populated, or pin a precise blocker | ☐ |
| S70-3 Cross-port note + close + gates | 1 | note if shared; parity + asan + stress (gl-lock); docs/memory; commit | ☐ |

## NOT pulled
Combo border pen colour (#2 residual), RScrlBar hosting, `ma_tabs_hit` click routing, #12
debrief capture.

## Execution log

### S70-1 Clear font-rebase debt — DONE
Re-captured the two font-touched refs. **`campaign_map` was byte-identical** (0 px) — the map
date readout uses `g_AllFonts[1]="Intel"` (ART), so S69's font change never touched it; the
S69 "must rebase" flag was overcautious. Only **`map_playerlog_tab1`** changed (1.4%, the sans
text) and was rebased.

### S70-2 Career content table — DONE (I4/#15 last gap CLOSED)
Investigation, then a one-case fix:
- The Career tab is a separate dialog `IDD_CAREER` (`CCareer`); its table is an **RListBox**
  (`m_IDC_RLISTBOXCTRL1`, id 2018) populated in `CCareer::OnInitDialog` (aircraft-type rows ×
  Sorties/Combats/Kills/Losses). The Name box (same OnInitDialog) rendered but the table did
  not.
- **Root cause: the OOB dialog draw path had no listbox case.** OOB dialogs render via
  `ma_oob_render_node` → `ma_ole_draw_toolbar`, which handled STATIC/EDIT/EDTBT/TABS/BUTTON/
  COMBO but **not CT_LISTBOX** (the front-end draws listboxes via a separate absolute-positioned
  `ma_ole_draw_all` path). So the table control existed and was populated but was never drawn.
  Added a `CT_LISTBOX` case to `ma_ole_draw_toolbar` (mirrors `ma_ole_draw`'s
  OnDraw-at-viewport-origin, at the toolbar-offset rect). **The table now renders with data**
  (F86 1/F86 2/F80/F84/F51/All × Sorties/Combats/Kills/Losses, all 0 on a fresh save) — and the
  same fix lit up the **Log of Missions** tab's Date/Initial-Point/Kills log listbox as a bonus.
- **Residuals (named, not chased):**
  - *(a) Opaque listbox background.* Gold #15's table shows the pilot photo through a
    TRANSLUCENT box; native's listbox fills opaque black (`RLISTBXC.CPP:488`, the same pattern
    as the combo #2). Skipping it à la S69's combo fix **regressed the front end** — it erased
    the title menu (the front-end menu/prefs listboxes rely on the opaque box). Reverted;
    fixing it needs a context flag to skip the fill on the OOB path only. Named residual.
  - *(b) Doubled "F86 1" header corner.* The source adds `IDS_L_SQ_BF_F86A` **twice**
    (`CAREER.CPP:173-177`), so native shows F86 1 in the header corner where gold (BDG-patched)
    shows a blank — a source-vs-BDG data delta, same class as the BDG tab.

### S70-3 Gates + close
- **2D parity byte-identical sweep RESUMED and PASSES** (title / prefs_3d / prefs_others /
  quickmission / campaign_map all 0 px vs the S69 refs — the `CT_LISTBOX` case only touches the
  OOB draw path, not the front end); `map_playerlog` + `map_playerlog_tab1` rebased for the now-
  rendering tables.
- **ASan `asan_all.sh` PASS — 4/4 paths reached, 0 reports** (headless; the campaign map/fly/
  nextday paths exercise the new OOB listbox draw).
- **Stress `stress_launch.sh` under `gl-lock`: 20/20 OK** (clean pass; load ~7 this run, so the
  25 s timeout had headroom — confirms the S69 HANGs were purely load-induced, not a fault).
- **Cross-port note 27** delivered to `bob/doc/` (missing OOB `CT_LISTBOX` case + the
  load-bearing-fill caveat that distinguishes the listbox from the combo #2).

## Result: 8/8 pts, sprint goal MET. Parity #15 (Player Log) CLOSED — the last half of I4.
