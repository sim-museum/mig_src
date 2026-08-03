# Sprint 76 — "Scope the campaign" (G2 investigation) — ✅ CLOSED 2026-08-03 (goal MET)

**Planned 2026-08-03 (PO pre-approved ceremonies). Autonomous. DoD: a precise, tested scope of
the G2 campaign epic — what works vs what's missing.**

## Context
EPIC I (Wine-parity) closed in S73–S75. The largest remaining backlog item is **G2 — play the
campaign across missions** (21 pts, was ⬜). Its actual state in the port was untested: the ASan
camp-fly / camp-nextday modes run without crashing, but whether the campaign *progresses* was
unknown. This sprint scopes it — headless, since 3D flight runs under `SDL_VIDEODRIVER=dummy`
(the S75 technique) and the display was Julia-held.

## Sprint Goal
Determine, by testing (not assuming), what of the G2 campaign flow works today, and break the
21-pt epic into concrete remaining sub-stories.

## Execution log

### S76-1 — G2 scoped: the campaign is FAR more complete than "⬜ not started" — DONE
All headless (`SDL_VIDEODRIVER=dummy`, CAMP nav `30,r3;65,#1055;100,#2063:1`):

- **Single-mission flow works end-to-end.** `MA_CAMP_FLY=1 MA_ENABLE_3D=1 BOB_AUTOEXIT=60`
  drove: operational map (target icons, frontline, supply routes, date "6/25/50: Morning,
  planning") → `OnClickedFrag2` → **singlefrag briefing** → `FragFly`→`StartFlying` → **campaign
  3D flight** → `BOB_AUTOEXIT` → `flight close (id=1) OnOK` → `OnFlyingClosed` → **debrief**
  (currentpage=1 panel). Every stage traced and reached.
- **Multi-mission CHAINING works.** `MA_CAMP_NEXTDAY=1` (→`OnClickedFrag2` no-flyable →
  `Campaign::NextMission`/`NextDay`) advanced the campaign — the D.I.S. (Daily Intelligence
  Summary) dialog opened showing **"MISSION 2 BRIEFING"** with the briefing-room art and a
  mission code. So the campaign progresses from Mission 1 to Mission 2. Artifact:
  `port/ref/native/campaign_mission2_brief.png`.
- **Existing test hooks make G2 headlessly drivable:** `MA_CAMP_FLY` (frag→fly), `BOB_AUTOEXIT`
  (flight→debrief), `MA_CAMP_NEXTDAY` (advance) — all already in the ASan suite.

**Re-scoped G2 (⬜ → 🔨).** What remains for full G2:
1. **State persistence across missions** — verify the campaign save/load resumes at the right
   mission/day (save infra exists since S11–S14; the ASan suite loads `Auto Save.sav`).
2. **Full multi-mission fly-loop** — fly Mission 2 → debrief → Mission 3, i.e. the chain with a
   *flyable* mission each period (not just the no-flyable NextDay advance).
3. **Edge cases / polish** — debrief "Next Period" driving, campaign-end conditions, the small
   Overview black-rect (likely unhosted RScrlBar).

## Gates
**No code change** (pure investigation + two capture artifacts). Nothing in the build changed →
ASan/stress/2D-parity unaffected by construction.

## Result
G2 is substantially functional already — the single-mission flow and Mission-1→Mission-2 chaining
both work headless. The 21-pt epic is de-risked and re-scoped to *verification + the flyable
multi-mission loop + persistence*, not a from-scratch build. A strong scoping outcome (measure,
don't assume): the epic was assumed empty and is mostly there.
