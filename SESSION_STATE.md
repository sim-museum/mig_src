# MiG Alley — native Linux (SDL2) port: session state

_Last updated: 2026-08-08, after Sprint 82. Branch: `linux-port`._

This file is a point-in-time state snapshot. Living docs: `RUNNING.md` (run + current state),
`scrum.md` (backlog + per-sprint reviews), `port/scrum/screen-parity.md` (gold-shot verdicts),
`port/scrum/sprint-NN.md` (boards), memory `migalley-port-state`.

## Where the port stands

The native 32-bit i386 ELF build (`gcc -m32` + SDL2) boots to the title screen, navigates the
full front-end, flies 3D missions, and plays the campaign across missions — all native, no Wine.

- **S82: the campaign-map OOB dialogs accept clicks** (tabs switch, title-bar ✓ dismisses, stray
  clicks swallowed). They had been render-only for the port's whole life; the tell was that
  `ma_tabs_hit` existed with **no caller** and a scaffold env hook stood in for tab switching.
- **EPIC I (Wine-parity) — essentially COMPLETE.**
  - I1 gold-shot inventory COMPLETE: all 15 PO gold shots have native captures in
    `port/ref/native/` (last one, #12 debrief, captured S75).
  - I2 front-end 2D parity done (fonts, tabs, icons, combos, translucency).
  - I3 3D-view parity: #10 cockpit + #11 external CLOSE (S73 cockpit-black fix); #12 debrief CLOSE (S75).
  - I4/#15 Player Log CLOSE (Career content table, S70/S71).
- **G2 (campaign playability) — 🔨 the campaign lifecycle now runs END-TO-END (S80).**
  - Single-mission flow works end-to-end: map (icons/frontline/routes/date) → frag → briefing →
    campaign flight → flight-close → debrief.
  - Multi-mission chaining works: `NextDay`/`NextMission` advances (opens "MISSION 2 BRIEFING").
  - **Flying a campaign mission now completes the debrief and advances the campaign** (map date
    "Morning, planning" → "Morning, debrief") — S79 fixed the fileblock-corruption crash that
    previously hung the campaign debrief.
  - ⭐ **The FLYABLE MULTI-MISSION LOOP runs (S80):** two campaign missions flown back-to-back in
    one process, each debriefed, the period advanced between them (the genuine
    `CDebriefToolbar::OnClickedNextPeriod`), and the campaign carried on to its **end-of-campaign
    screen**. Recipe: `MA_CAMP_FLY=1 MA_CAMP_LOOP=N BOB_AUTOEXIT=40 MA_ENABLE_3D=1` under
    `SDL_VIDEODRIVER=dummy`. The residual blocker was three one-shot `++n == N` statics in the
    **test harness**, not game code.
  - ⭐ **State persistence CLOSED (S81):** the campaign round-trips across processes under the
    canonical `SaveGame/Auto Save.sav` (run A advances 6/25/50 → 7/3/50 → 7/8/50; a fresh run B
    resumes at 7/3/50). `fileman::namenumberedfilelessfail` lacked the hard variant's "fake long
    file name" branch and fell through to DIR.DIR's fixed **12-byte** 8.3 name, so the 13-char
    `"Auto Save.sav"` was written *and read* as `Auto Save.sa` — self-consistent, hence
    symptomless, while the canonical file went untouched. **Remaining G2: edge/polish only.**

## Headline fixes this session (Sprints 73–79)

1. **3D cockpit-black FIXED (S73, `a5f614c`)** — the software rasterizer's active palette LUT
   (`palette_table`) was stale/empty at cockpit-draw time and the cockpit's cached
   `SelectPalette(0)` no-op'd → texels indexed an empty LUT → black. Fix (`BTREE.CPP:580`):
   re-enable the engine's own disabled per-object palette reset, forced past the cache. Cockpit +
   external F-86 now fully textured = gold #10/#11. Gold-verified; gates green.
2. **Campaign flyable-loop crash FIXED (S79, `09b4136`)** — the campaign-debrief map preload
   re-opened an already-open `FIL_ICON_BASES` fileblock, creating a duplicate `fileblocklink`
   that corrupted the openfiles accounting and hung the debrief. Fix: read-only
   `fileman::MA_IsFileOpen` + guard the preload (`FULLPANE.CPP:2706`) to skip already-open files.
   Campaign now advances after a flown mission. Gates green.
3. **#12 debrief captured, I1 inventory COMPLETE (S75, `1485a62`)** — reached headless via the
   existing `BOB_AUTOEXIT` hook under `SDL_VIDEODRIVER=dummy` (no display lock). Strong A/B match.

Also: S74 `MA_OOB_OVERVIEW` capture hook + #12 characterization; S76 G2 scoping (re-scoped ⬜→🔨);
S77/S78 the investigation chain that located the flyable-loop blocker (measure-don't-assume:
S77's gamestate guess was wrong; S78 measured gamestate=CAMP and found the fileblock double-open).

## Reusable techniques discovered

- **3D flight runs headless under `SDL_VIDEODRIVER=dummy`** (the ASan camp-fly mode relies on it),
  so flight → debrief → 2D capture needs no `gl-lock` — no display contention with the sibling
  BoB/Julia sessions.
- Reach the post-mission debrief: `MA_ENABLE_3D=1 BOB_AUTOEXIT=60` (→ `ma_request_flight_exit` →
  `OverLay.quit3d` → `CloseWindow(IDOK)` → `OnFlyingClosed` → debrief). Set flight-exit flags from
  the MAIN thread (BOB_AUTOEXIT does), not the draw thread (a draw-thread `quit3d` starves the
  main-thread `ma_process_flight_close`).
- Drive the campaign headless: `MA_CAMP_FLY=1` (frag→fly), `MA_CAMP_NEXTDAY=1` (advance), all under
  dummy; capture 2D screens GL-free with `MA_SHOT=N`.

## Gate status (S79)

- 2D parity byte-identical: title / prefs_3d / campaign_map all 0 px.
- Stress `port/stress_launch.sh` under `gl-lock`: 20/20 OK.
- ASan `port/asan_all.sh`: 0 reports across all 4 paths (flight + campaign map/fly/nextday, 2/2 each).

## Next work (S80+)

1. **Full flyable loop** — the debrief crash is gone, so re-add the `MA_CAMP_LOOP` drive
   (`DebriefToolBar().OnClickedNextPeriod()`) and verify fly M1 → debrief → Next Period → fly M2.
2. **Campaign state persistence** across missions (save/load resumes at the right mission).
3. Minor: the two ~6px black rects on the Overview panel (likely unhosted RScrlBar); the doubled
   "F86 1" Player Log header cell (source-vs-BDG data delta).
4. Standing watch: the S66 ASan intermittent `worldinc.h:257`/`:565` (~1-in-50), not attributed —
   preserve the log if it reprises.

## Coordination note

One display shared with sibling BoB-scrum and Julia-Racer sessions — every GL render/capture/gate
goes through `gl-lock` (`export PATH="$HOME/bin:$PATH"`). 2D/flight captures under
`SDL_VIDEODRIVER=dummy` are GL-free and need no lock. `port/BOB_PORT_LESSONS.md` (BoB's shared
lessons file) and `CONCURRENCY.md` (parallel-session rules) are intentionally left uncommitted.
