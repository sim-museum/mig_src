# Sprint 77 — "Fly the campaign loop" (G2) — ⚠️ CLOSED PARTIAL 2026-08-03 (boundary located; verification blocked, precisely)

**Planned 2026-08-03 (PO pre-approved ceremonies). Autonomous. DoD: verify the flyable
multi-mission loop (fly M1 → debrief → advance → fly M2), or a precise blocker.**

## Context
S76 re-scoped G2 ⬜→🔨: the single-mission flow and the *no-flyable* NextDay advance both work.
The open question was the **flyable** multi-mission loop — fly a frag, get the campaign debrief,
advance via "Next Period", fly the next mission.

## Execution log

### S77-1 — Flyable-loop boundary located (verification blocked, precisely) — PARTIAL
- **The Next-Period drive is trivial and correct.** `DebriefToolBar()` (`MAINFRM.H:126`) returns
  the debrief toolbar (`m_toolbar5` / `CDebriefToolbar`); `OnClickedNextPeriod()`
  (`DBRFTLBR.CPP:226`) → `MMC.EndDebrief()` → `ChkEndCampaign()` → (if continuing)
  `UpdateToolbars()`+`CloseLoggedChildren()` → back to the operational map for the next mission.
  A gated `MA_CAMP_LOOP` hook driving it was written and builds cleanly.
- **But the campaign debrief is never reached on the tested path.** With `MA_CAMP_FLY=1
  BOB_AUTOEXIT=30 MA_CAMP_LOOP=1` under dummy, a trace of `MMC.indebrief` at the post-flight
  panel showed **`indebrief=0`** — so `OnClickedNextPeriod` never fired (the hook correctly gates
  on `indebrief`).
- **Root cause — a `gamestate` branch in `OnFlyingClosed`.** `RFullPanelDial::OnFlyingClosed`
  (`FULLPANE.CPP:2603`) branches on `gamestate`:
  - `gamestate==HOT || gamestate==QUICK` → `LaunchScreen(&quickmissiondebrief)` — **no
    `indebrief`** (this is the debrief the exit-key path lands on; the S75 #12 capture).
  - `else` (campaign / `WAR`) → `FULLPANE.CPP:2674` **`MMC.indebrief=TRUE` + `MMC.NextMission()`**
    → the campaign debrief with the Next-Period toolbar.
  `MMC.indebrief=TRUE` is set at exactly one site (`FULLPANE.CPP:2674`), in that else branch.
  The `MA_CAMP_FLY`+`BOB_AUTOEXIT` flight exited into the **HOT/QUICK** branch (`indebrief=0`), so
  the campaign debrief — and thus the flyable loop — was never reached.

**Concrete G2 next step (precisely located):** verify/fix the **`gamestate`** on the campaign
frag-fly path so `OnFlyingClosed` takes its campaign/`WAR` branch. Then the flyable loop is
drivable end-to-end (the `MA_CAMP_LOOP` → `DebriefToolBar().OnClickedNextPeriod()` drive is
correct, ready to re-add once `indebrief` is reachable). Open questions for that step: does
`FragFly`/`StartFlying` from the campaign map set the campaign gamestate, or does the port's
frag-fly path default to QUICK? And does a campaign mission need to *complete* (objectives/time),
not just exit, to take the else branch?

## Gates
**No code change** (the `MA_CAMP_LOOP` hook was reverted as unverified on the current path — it
is correct but unreached until `indebrief` is produced). Build unchanged → gates unaffected.

## Result
The flyable multi-mission loop is not yet verified, but its blocker is now precisely located: the
`gamestate` branch in `OnFlyingClosed` routes the exit-key exit to the quick-mission debrief, not
the campaign `indebrief` debrief. This sharpens S76's "full flyable loop" sub-story into a
specific, actionable G2 task. (measure-don't-assume: the loop *looked* one hook away and is
actually gated on a mission-state distinction.)
