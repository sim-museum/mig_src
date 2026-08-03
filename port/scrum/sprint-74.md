# Sprint 74 — "Face the debrief" (I1/I2 — parity #12) — ⚠️ CLOSED PARTIAL 2026-08-02

**Planned 2026-08-02 (PO pre-approved ceremonies). Autonomous. DoD: capture parity #12
(debrief) and close the gold-shot inventory, or a precisely-located blocker + reusable tooling.**

## Context
After S73 closed the 3D cockpit/external parity (#10/#11), the gold-shot inventory (I1) has
exactly one uncaptured shot: **#12 debrief (Claims table)**, noted "renders natively since
S21–35 — capture after a flight exit." No headless reach-hook existed.

## Sprint Goal
Reach + capture the debrief natively, A/B vs gold #12; else deliver a reusable capture hook and
a precise characterization of what #12 actually is.

## Execution log

### S74-1 — Reusable Overview hook added; #12-proper precisely characterized — PARTIAL
- **New headless hook `MA_OOB_OVERVIEW`** (`MIG.CPP` campaign-map idle → `CMainToolbar::
  OnClickedOverview` → the Overview stats panel), mirroring the existing `MA_OOB_PLAYERLOG`
  pattern. Captured `port/ref/native/campaign_overview.png` GL-free (`SDL_VIDEODRIVER=dummy
  MA_OOB_OVERVIEW=1 MA_SHOT=200`, CAMP nav).
- **The Overview / `CAC_view` claims table renders correctly** = gold #12's **"Ac Stats"
  SUB-VIEW**: "OVERVIEW" title chrome + star roundel + `?`/`✓`, Ac Stats / Ground Stats tabs,
  Kills + Losses tables (MiG 15 / Yak 9 / AAA × F86 1 / F86 2 / F80 / F84 / F51 / B26 / B29 /
  All), yellow sans headers (S69 fonts), translucent briefing photo (S71). All the front-end
  parity work (fonts, tabs, icons, translucency) composes here too.
- **FINDING — gold #12 is a DIFFERENT screen from what the note implied.** The A/B shows gold
  #12 is the **post-mission DEBRIEF**: full-screen pilot-in-cockpit photo, mission header
  ("Mission: Landing/Takeoff practice / Name: Kimpo Airfield / Status: Operational"), a
  **ground-target Claims** table (Supply Point/Vehicle/Train, Marshalling Yard, Bridge,
  Airfields, Artillery, Troops, Tank × Player/UN/Red) and BACK / AC STATS / GROUND STATS /
  REPLAY. It is reached only via the mission-end path (`FULLPANE.CPP:2674`
  `MMC.indebrief=TRUE` + `MMC.NextMission()`), which depends on live mission state — there is
  no clean 1–2-line headless trigger. **The Overview I captured is the gold's "Ac Stats"
  sub-view, not the main debrief.**
- **Honest close:** capturing #12 proper requires a real mission→debrief run (display-dependent,
  and the display was held by the concurrent Julia session for much of the sprint). Scoped to a
  dedicated stable-display session. Minor residual noted on the Overview: two small black
  rectangles on the panel's right edge (likely an un-palette'd inset — same *class* as the S73
  cockpit-black but a different, low-priority element).

## Gates
Only code change is the `MA_OOB_OVERVIEW` hook — a **gated `getenv` no-op when unset**.
Verified: a normal `campaign_map` capture (hook unset) is **byte-identical** to its ref
(0 px), so no normal path (2D / flight / campaign / ASan / stress) can be affected. No ASan/
stress re-run needed for a branch that cannot execute without the opt-in env var.

## Result
Reusable Overview capture hook + native ref delivered; the Ac-Stats claims table verified = gold
sub-view; #12-proper precisely relocated to the post-mission debrief and scoped for a mission
run. Partial: the main #12 capture did not land (needs a stable-display mission→debrief run).
