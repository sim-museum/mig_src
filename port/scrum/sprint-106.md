# Sprint 106 — "The panel nobody enumerated" (PO-9) — ✅ CLOSED 2026-08-15 (goal MET)

**Planned 2026-08-15 (PO pre-approved ceremonies; continuous-sprint directive). Autonomous. ~8 pts.**
**Sprint Goal:** exiting a campaign mission with ALT+X shows the mission result.

| Story | Pts | Result |
|---|---|---|
| S106-1 reach the defect headlessly (ALT+X from a campaign flight) | 3 | ✅ reproduced with a capture: map returns, date says "debrief", no panel |
| S106-2 find and fix it | 3 | ✅ the panel was created on every flight and never painted |
| S106-3 evidence against gold | 2 | ✅ same fields as the gold video, down to the mission |

## ⭐ The defect: an enumeration that knew about one toolbar

The post-flight campaign path already does the right thing (`FULLPANE.CPP`, CAMP branch):

```
if (pilot is dead) MainToolBar().OnClickedMissionlog();
else               DebriefToolBar().OpenMissionresults();
LaunchMap(s, true);
```

and the trace shows it ran and produced a live dialog (`[po9] after OnClickedMissionresults:
child=0xb33f6b0`). It was simply **never painted**: the port's map idle walks
`mf->m_toolbar2` (`CMainToolbar`) for open child dialogs, and MISSION RESULTS is logged against
`mf->m_toolbar5` (`CDebriefToolbar`). Every campaign flight created it, and nothing ever drew it.

Fix: `ma_map_paint_oob` and `ma_map_click_oob` walk **both** toolbars. Clicks too, not just paint —
a dialog that is drawn but not clickable is half-hosted, and this one has four buttons (I.D. /
Debrief / Redo / Next Period).

**Same family as S82** (dialogs that existed but were never composited) with the twist that here the
tree, the art and the compositing were all fine; the *enumeration* was too narrow. Worth naming as
its own smell: **when a subsystem works for every case but one, check what enumerates the cases.**

## Evidence

Native capture (`port/ref/native/mission_results.png`), ALT+X from a campaign mission on the pinned
pristine save:

> **MISSION RESULTS** — Objective **Munsan-Seoul Rail-line** · Task **Reconn** · Result **Failure** ·
> Redo **no**, over the squadron photo, with the ?/✓ title buttons.

The gold video (`short` @ ~36 s) shows **the same four fields with the same values** — the PO's
recording and the pinned fixture are the same campaign state, so this is a content match, not just
a shape match.

## How ALT+X was delivered

`BOB_KEYSEQ` gained a third field in S105: `"500,0x2D,0x38"` queues Alt-down, X-down, X-up, Alt-up
so the engine's shift-state machine sees a real ALT+X (`EXITKEY` is DIK 0x2D **with shift state 2**;
a bare 0x2D is `RESETRECORD`). Without that, the exit route in the PO's report is unreachable from a
synthetic tap.

## Residual, stated

At 800×600 the panel's bottom **button row** (I.D. / Debrief / Redo / Next Period) is below the
visible area; the gold shows it at 1920×1080. That is a placement/size question at a different
resolution, not a missing widget — the same "never judge across a resolution boundary" caveat as
PO-11 (S64). Logged, not fixed here.

## Gates

parity 5/5 · sweep 9 OPEN/0 CRASH · map click · map drag · sysbox exit · help click · overlay text ·
stress 20/20 · ASan 0.

## Result

Five PO defects closed in five sprints (PO-4/5, PO-8, PO-7, PO-6, PO-9), and the cause was different
every time: a span filler that ignored the alpha plane, an initialisation nobody called, a five-second
timer nobody had photographed, a window whose origin is the screen centre, and a paint walk that
enumerated one toolbar. The only thing they had in common was how they looked to the player.
