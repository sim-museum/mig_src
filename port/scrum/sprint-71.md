# Sprint 71 — "Polish the chrome"

**Planned 2026-08-02 (PO pre-approved ceremonies). Autonomous, headless DoD.**

## Context
S69/S70 closed the front-end 2D parity headliners (font, combo, Player Log table). Two named
chrome residuals remain: the OOB-listbox opaque box (Player Log tables read opaque vs gold's
translucent) and the combo border pen colour (gold rounded-blue vs native rectangular edge).

## Sprint Goal
Player Log Career/Log tables show the photo through (gold's translucency) without regressing the
front-end menu; combo border moves toward gold — both held to the byte-identical sweep.

## Committed (~6 pts)
| Story | Pts | Definition | Status |
|---|---|---|---|
| S71-1 OOB-listbox translucency | 3 | Context flag skips the listbox black fill on the OOB path only; tables translucent = gold; front-end byte-identical | ☐ |
| S71-2 Combo border pen colour | 2 | Move the combo border toward gold's rounded-blue if low-risk, else PO-waive | ☐ |
| S71-3 Gates + close + note | 1 | parity + asan + stress (gl-lock); docs/memory; commit | ☐ |

## NOT pulled
RScrlBar hosting, `ma_tabs_hit` click routing, #12 debrief capture, 3D-view parity (I3).

## Execution log

### S71-1 OOB-listbox translucency — DONE (gold-matched)
Added a context flag `ma_oob_lb_draw` (defined in `ma_olecontrol.cpp`, read in `RLISTBXC.CPP`)
set to 1 only while `ma_ole_draw_toolbar` draws an OOB-path listbox. `CRListBoxCtrl::OnDraw`
skips its opaque black `FillRect` when the flag is set, so the Player Log Career/Log-of-Missions
tables composite over the dialog's already-painted background (the pilot photo shows through) =
gold's translucency. The front-end menu/prefs listboxes draw via `ma_ole_draw_all`, which never
sets the flag, so they keep the opaque box they rely on — **title byte-identical (0 px)**, the
S70 regression avoided. Verified: Career table translucent, front-end unregressed.

### S71-2 Combo border pen colour — MEASURED, no fix needed (residual retired)
The S69 residual named native's combo border "rectangular light/white vs gold's rounded-blue".
Measured: `AXC_DARKEDGE = AXC_LITEDGE = AXC_CIRCULARCOMBOBOXCOLOR = RGB(103,132,198)` — a blue —
and `m_bCircularStyle` is `FALSE` always (not persisted), so both native and gold draw the same
rectangular blue border + round `FIL_COMBO_BUTTON`. Cropped native vs gold #2 at matching scale:
both show the authored blue border and round button; the earlier "white" reading was the
anti-aliased blue edge at 800-res. **No code-level deviation — the residual is retired, not
fixed** (an S64-style "measure, don't assume" result). Cross-cutting #2 is fully matched.

### S71-3 Gates + close
- **Front-end 2D parity byte-identical sweep PASSES** (title / prefs_3d / prefs_others /
  quickmission / campaign_map all 0 px — the `ma_oob_lb_draw` flag only affects OOB listboxes);
  `map_playerlog` + `map_playerlog_tab1` rebased for the translucent tables.
- **ASan `asan_all.sh` PASS — 4/4 paths reached, 0 reports** (headless; the campaign paths
  exercise the OOB listbox draw with the new flag).
- **Stress `stress_launch.sh` under `gl-lock`: 20/20 OK** (clean).
- **Cross-port note 28** delivered to `bob/doc/` (the OOB-only context-flag technique completing
  note 27's deferred fix; plus the combo-border residual that measured away).

## Result: 6/6 pts, sprint goal MET. EPIC I front-end 2D parity essentially complete.
