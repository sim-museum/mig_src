# Sprint 79 — "Land the loop fix" (G2) — ✅ CLOSED 2026-08-03 (fix LANDED) — ⭐ campaign advances after a flown mission

**Planned 2026-08-03 (PO pre-approved ceremonies). Autonomous. DoD: fix the S78 flyable-loop
blocker (fatal double-open of a leaked fileblock) and verify the campaign debrief completes.**

## Context
S78 located the flyable-loop blocker: the campaign-debrief map reload (`FULLPANE.CPP:2706-2709`,
`delete new fileblock(f)` preloads) re-opens an already-open `FIL_ICON_BASES` (0x6a63) and the
campaign debrief hangs/dies. This sprint found the mechanism and fixed it.

## Execution log

### S79-1 — Fixed: guard the debrief preload against already-open files — DONE
- **Mechanism nailed.** `ReallyEmitSysErr` = `ExitMode().Say(...).SayAndQuit` (`MYERROR.H:88`),
  but in the port the "again without closing" message is **informational** (`[For your
  information.]`, non-fatal — seen once at boot for 0x6a63 with no ill effect). The real damage
  was the *duplicate* `fileblocklink` the re-open creates (`Fileman.cpp:1602`) plus the paired
  `delete` — it corrupts the openfiles accounting and the campaign-debrief setup then dies.
  `CMIGView::DrawIcon` (`MIGVIEW.CPP:871`) uses a **stack** `fileblock` (RAII), so the icon file
  isn't leaked there; 0x6a63 (the map "Bases" icon) is held open by the map-render/cache path
  and is *within* the preload loop's own range (`FIL_MAP_BUTTON1 0x6a04..FIL_ICON_NEXT_PERIOD
  0x6aa8`).
- **The fix (targeted, low-risk).**
  - New read-only `fileman::MA_IsFileOpen(FileNum)` (`Fileman.cpp`, decl `FILEMAN.H`) — walks the
    active `openfiles` list (mirrors the existing double-open detection); no accounting mutation.
  - Guard the two debrief preload loops (`FULLPANE.CPP:2706-2709`, `MA_LINUX`): skip
    `delete new fileblock(f)` when `FILEMAN.MA_IsFileOpen(f)` — an already-open file is already
    loaded, so the preload is a no-op for it, and we never create the corrupting duplicate.
- **Verified — the campaign now ADVANCES after a flown mission.** `MA_CAMP_FLY=1 BOB_AUTOEXIT=40`
  under dummy: `[debrief] CAMP branch: indebrief=TRUE set, calling NextMission` → `back in
  front-end` → **the operational map returns** (`currentpage=0`, 96 renders — before the fix the
  debrief hung after ~1 line). The map date readout advanced **"6/25/50: Morning, planning" →
  "Morning, debrief"** — the campaign progressed. The map renders cleanly (toolbar, target icons,
  frontline, routes) — no accounting corruption. Artifact: `port/ref/native/campaign_postdebrief.png`.

## Gates
- **2D parity byte-identical:** `title` 0px / `prefs_3d` 0px / `campaign_map` 0px — the guard is
  `MA_LINUX` in the campaign-debrief path and doesn't touch the front-end or the plain map render.
- **Stress `stress_launch.sh` under `gl-lock`: PASS 20/20** (clean, even under the concurrent
  ASan-build CPU load).
- **ASan `asan_all.sh`: PASS — 0 reports across all 4 paths** (flight + campaign map/fly/nextday, all 2/2; rebuilt with the fix, the campaign modes exercise the fileblock preload).

## Result
The G2 flyable multi-mission loop's blocker is **fixed and landed** — flying a campaign mission
now completes the debrief and advances the campaign (planning → debrief), returning to the map
cleanly, where before it hung on a corrupted fileblock. A three-sprint investigation chain
(S77 gamestate → S78 leaked-fileblock → S79 duplicate-link corruption) converged on a
14-line targeted fix. Remaining G2: drive the debrief's Next Period → the next flyable mission
(now reachable, crash gone; the S77 `MA_CAMP_LOOP` drive is ready to re-add), and state persistence.
