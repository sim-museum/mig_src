# Sprint 42 — "Day-advance strategic-sim ASan sweep" (autonomous, headless DoD) — clean

**Context:** Autonomous (PO unavailable). Closes the last un-swept campaign path from the S41 coverage map:
the in-campaign **day-advance / strategic simulation**. (Investigation showed the `SaveBin`/`SaveGame`
*writeback* was already swept incidentally in S41 — it runs in `OnClickedFrag2`'s *flyable* branch that
`MA_CAMP_FLY` drives. The genuinely un-swept code was the *no-flyable* branch's `Campaign::NextMission`.)

**Sprint Goal:** Drive the campaign strategic sim (day advance) under ASan; fix anything it finds.

## Mechanism
`Campaign::NextMission` (`MISSINIT.CPP:3641`) → `NextDay` (date advance, `ChkRotateMiGSquadrons`,
`ModifyMiGAggressionLevel`, `ReplenishAcStock`) is reached from `CMainToolbar::OnClickedFrag2`'s
`!FlyableAircraftAvailable()` branch — unreachable with the flyable "Auto Save". Added a headless driver:
- **`MAINTBAR.CPP` `OnClickedFrag2`:** under `MA_LINUX`, if `getenv("MA_CAMP_NEXTDAY")`, force the
  no-flyable sequence (`CloseLoggedChildren` ×5 → `MMC.UpdateAllDis/ZeroDisDay` → `MMC.NextMission()` →
  `SupplyTree::ProcessAirFields()` → `m_toolbar5.OnClickedNextPeriod()`) and return. Faithful replica of
  the game's own branch; symbols (`MMC`=`Miss_Man.camp`, `SupplyTree`) already in scope there.
- **`MIG.CPP` map idle:** a `MA_CAMP_NEXTDAY` hook (parallel to `MA_CAMP_FLY`) drives
  `MainToolBar().OnClickedFrag2()` after 40 map-render idles.

Recipe: `MA_DISABLE_3D=1 MA_IGNORE_SAVE_DATE=1 MA_CAMP_NEXTDAY=1 BOB_CLICKSEQ="30,588,263;65,40,108;100,68,565"`.

## Validation (headless DoD)
- **0 ASan reports across 3 runs**; each drove `[map] driving NextDay` and continued (no crash).
- **No regression:** `port/asan_flight.sh` PASS, `port/asan_campaign.sh` PASS.
- Compiles clean (`MAINTBAR`/`MIG` MFC fragments); `wmig` links (8.7 MB), 0 undefined symbols.

## Increment
The **campaign path is now ASan-swept end to end** — load → strategic map → mission generation → fly →
`SaveBin`/`SaveGame` writeback → day-advance strategic sim — all clean (with the two S41 fixes). Unlike
S41, the strategic sim needed no fixes. A reusable `MA_CAMP_NEXTDAY` harness is added for future regression.

**Remaining (low priority):** multi-day rollover / raid-planning over several successive days — the hook
advances one day; subsequent day-advances leave the map for the orders screen, so a multi-day drive would
need extra screen-nav handling. Documented in `asan-findings.md`.
