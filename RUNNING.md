# MiG Alley — Run & Check Progress

## Run the game

```bash
cd /home/admin/sgl/TUE/MigAlley/WP/drive_c/rowan/mig && ./wmig
```

Bare launch (no env vars) from the install dir — boots to the title menu; Single
Player → Hot Shot is two clicks to a flyable 3D mission. Requires a healthy GL
display session.

Rebuild after source changes: `cd /home/admin/ma/build && ninja` (fallback:
`bash port/rebuild.sh`).

## Check progress

| What | Where |
|---|---|
| Backlog + burndown (per-sprint rows) | `scrum.md` — EPIC I is the screen-parity epic; §6 is the burndown |
| Per-gold-shot parity verdicts | `port/scrum/screen-parity.md` (one verdict line per Wine gold shot) |
| Native captures for side-by-side | `port/ref/native/` |
| Per-sprint boards | `port/scrum/sprint-NN.md` (latest: sprint-57) |
| Live product snapshot | `STATUS.md` |
| History | `git log --oneline` on `linux-port` |

Gold standard: `/run/media/admin/BEA6-BBCE/ma/` (14 PNGs) + the Player Log shot
`/home/admin/Pictures/Screenshots/Screenshot From 2026-07-19 20-33-27.png` (I4).
Oracle ruling: the gold shots as-is = the BDG 0.85F patched build
(resources read from `English/TEXT/miglang.dll` + patched `Mig.exe` since S57).

## Current state (2026-08-03, after Sprint 76)

- **S77: the flyable multi-mission loop's blocker is located** — the campaign debrief
  (`indebrief`→Next-Period→next mission) needs `OnFlyingClosed` (`FULLPANE.CPP:2603`) to take its
  campaign/`WAR` branch; the exit-key path takes the HOT/QUICK branch → `quickmissiondebrief`
  (`indebrief=0`). Next G2 step: verify/fix the campaign frag-fly `gamestate` (or complete the
  mission, not just exit), then the loop is drivable.
- **S76: the campaign (G2) is far more complete than the backlog implied — re-scoped ⬜→🔨.**
  Headless-verified (no display): the single-mission flow works end-to-end (map→frag→briefing→
  **campaign flight**→flight-close→**debrief**) and **multi-mission chaining works** — advancing
  (`MA_CAMP_NEXTDAY`) opens **"MISSION 2 BRIEFING"**. Remaining G2: state persistence across
  missions, the full flyable multi-mission loop, edge/polish. Test recipe: `MA_CAMP_FLY=1
  BOB_AUTOEXIT=60` (fly a frag→debrief) / `MA_CAMP_NEXTDAY=1` (advance) under `SDL_VIDEODRIVER=dummy`.
- ⭐ **S75: parity #12 (debrief) CAPTURED and matches gold — the I1 gold-shot inventory is
  COMPLETE (all 15 shots have native captures).** Reached the post-mission debrief HEADLESS with
  no code change: `MA_ENABLE_3D=1 BOB_AUTOEXIT=60 MA_SHOT=220` under `SDL_VIDEODRIVER=dummy` (3D
  flight runs headless — proven by the ASan camp-fly mode — so no `gl-lock` needed). Fly Hot Shot
  → `BOB_AUTOEXIT` → `ma_request_flight_exit`→`quit3d`→`CloseWindow(IDOK)`→`OnFlyingClosed`→debrief.
  Ref `port/ref/native/flight_debrief.png`. Match is strong (same layout/photo/chrome/fonts);
  differences are mission-type data only (Hot Shot air claims vs gold's ground claims).
- **S74: parity #12 groundwork — reusable `MA_OOB_OVERVIEW` capture hook (Ac-Stats sub-view).** The Overview/Ac-Stats claims table (`OnClickedOverview`→`CAC_view`) renders correctly
  = gold #12's "Ac Stats" sub-view (`port/ref/native/campaign_overview.png`, GL-free
  `MA_OOB_OVERVIEW=1 MA_SHOT=200`). Finding: gold #12 proper is the **post-mission DEBRIEF**
  (mission header + ground-target Claims + REPLAY), reached via the mission-end path
  (`FULLPANE.CPP:2674`), so a real mission→debrief run is needed to capture it — scoped to a
  stable-display session.
- ⭐ **S73: the 3D cockpit-black is FIXED — parity #10 (cockpit) + #11 (external) → CLOSE.** The
  in-flight cockpit now renders fully textured (metallic canopy, instrument panel, gunsight drum
  10-40, ADI inset content) = gold #10; the external F-86 renders textured (silver/yellow skin,
  "FU-908"). **Root cause (all 3 of S72's hypotheses refuted with gl-lock data):** on the
  software raster path the active 8→16bpp LUT (nasm `palette_table`) is left stale/empty at
  cockpit-draw time and the cockpit's cached `SelectPalette(0)` no-ops → cockpit imagemap/flat
  texels index an empty LUT → near-0 (black) 565. Terrain is immune (uses `LandFadeData`). **Fix:**
  re-enable the engine's original per-object palette reset at `BTREE.CPP:580` (disabled there as
  `//dead POLYGON.SelectPalette(0)`), forced past the cache. 2D parity byte-identical (3D-only
  change). Capture recipe for #10: `MA_ENABLE_3D=1 BOB_CLICKSEQ='40,r1;95,r0' MA_DUMP_BACK=220`
  under `gl-lock`; #11 adds `BOB_KEYSEQ='12,0x40'` (F6) + `MA_DUMP_BACK=320`.
- ✅ **EPIC I front-end 2D parity is essentially COMPLETE.** S71 resolved the last two chrome
  residuals: **(a) OOB-listbox translucency** — a context flag (`ma_oob_lb_draw`, set only while
  `ma_ole_draw_toolbar` draws an OOB listbox) skips the black fill on the OOB path so the Player
  Log Career/Log tables show the photo through (gold's translucency), while the front-end menu
  (same control) keeps its opaque box and stays byte-identical; **(b) combo border colour** —
  MEASURED away: `AXC_*EDGE=RGB(103,132,198)` blue and `m_bCircularStyle=FALSE`, so native
  already matches gold's blue rectangular border (the "white" reading was AA at 800-res). The
  open parity frontier is now **3D-view parity (I3, #10/#11 — cockpit/ADI black boxes)** and
  campaign **G2**.
- ✅ **Parity #15 (Player Log) is CLOSED — the Career CONTENT TABLE renders (S70), now
  translucent (S71).** The
  per-aircraft-type Sorties/Combats/Kills/Losses table (F86 1/F86 2/F80/F84/F51/All, populated
  from `MMC.debrief.playertotals`) was **populated but never drawn**: the OOB dialog draw path
  (`ma_ole_draw_toolbar`) had **no `CT_LISTBOX` case** (the front-end draws listboxes via a
  separate path). Added the case → table + the Log of Missions log listbox render. Residuals
  named: opaque listbox box vs gold's translucent (the fill is load-bearing for the front-end
  menu, so it can't be skipped globally — needs an OOB-only flag), and a doubled "F86 1" header
  cell (source-vs-BDG data delta). This was the last open half of I4, deferred since S56.
- ⭐ **Both cross-cutting front-end deviations were CLOSED in S69** (font + combo):
  - **#1 font FACE** (colour was S63): the port was silently running as a **Japanese system**
    — compat `EnumFontFamiliesA` **always** invoked the enum proc, so `MIG.CPP`'s
    localization probe took the CJK branch and asked for MS Mincho everywhere (unshipped →
    art-face fallback), so it **never requested Arial**. Fixed the stub to report a face
    present only for a pure-ASCII name; plus `ma_gdi_font_create` now honours the face via a
    cached ART(Intel.ttf)/SANS(LiberationSans≈Arial)/SERIF registry (unknown→ART). Runtime
    faces are now `Intel`+`Arial` as on gold. Gold-verified: Prefs #2/#8 + QuickMission #9 =
    blue sans labels + yellow sans values; campaign #13 phase list yellow sans. `MA_TRACE_FONT`.
  - **#2 combo chrome**: the combo was the one hosted control still filling its box **opaque
    black** (`RCOMBOC.CPP:355`); gold's are transparent (panel through a thin border). Skipped
    the black `FillRect` on the Linux path → translucent = gold.
- **Third instance of "a compat stub that returns *success* is invisible"**: after S68's
  `DrawIcon`-noop and S64's `GetFileNum`-returns-0, S69's `EnumFontFamilies`-always-true.
  Standing check: for any compat function whose **return value gates engine behaviour**,
  verify it returns the truth, not just a non-crashing value.
- **Parity #15 (Player Log) has no chrome deviations left.** Tab bar + centring + tab
  switching (S61), authored colours (S63), title bar (S65), title trimmed to the dialog
  (S67), **`?`/`✓` buttons (S68)**, and the sans "Name" label (S69 font). Only the Career
  **content table** remains — the half of I4 never pulled.
- ⭐ **S68 found a whole missing subsystem: icons.** `CDC::DrawIcon` was a no-op stub and
  `LoadIconA` returned NULL, so **no icon anywhere in the port had ever rendered** —
  silently, for the port's entire life. A stub that returns *success* never gets reported.
  Now real: `RT_GROUP_ICON` → `RT_ICON` decoding (the group is a directory naming the image
  by id; `biHeight` is **doubled** — XOR bitmap then 1bpp AND mask, both bottom-up, mask
  bit 1 = transparent), cached, blitted alpha-keyed through the viewport/clip path.
  **Icons live in `Rbutton.ocx` (828–832), not Mig.exe** — third instance of "inside a
  control, `AfxGetInstanceHandle()` is that control's own module" (after `Intel.ttf` and
  the RTabs art). `MA_TRACE_ICON` traces resolution.
- ⚠️ **ASan finding: DOWNGRADED TO A WATCH ITEM — not attributed, not fixed, not closed.**
  S68 did the A/B properly (pre-S66 binary built via `git worktree add /tmp/ma-s65
  0a69f94`; alternating S65↔HEAD across all modes, `detect_stack_use_after_return=1`
  forced): **S65 0 hits / 12 runs, HEAD 0 hits / 12 runs.** That is a *negative* — no
  difference between arms, so nothing supports S66 having introduced it — but it does not
  show the bug is gone: the single sighting was 2 reports in one 8-run suite and it has not
  recurred in ~50 runs, entirely consistent with a ~1-in-50 defect.
  **If `worldinc.h:257` / `worldinc.h:565` reports again, treat it as the SECOND sighting of
  a known intermittent and preserve the log immediately** — the S66 logs were lost to the
  next run's `rm -f`, which is why there is still no stack trace for it.
- ⚠️ **ASan watch item (from S66) unchanged**: `worldinc.h:257`/`:565` packed-item accessors,
  a ~1-in-50 `stack-use-after-return`, not attributed/fixed/closed. If it reports again,
  treat it as the SECOND sighting and preserve the log immediately. S69's diff (fonts+combo)
  is unrelated; ASan gate PASS 4/4 paths 0 reports.
- Gates (S70): **2D parity byte-identical sweep RESUMED + PASSES** (title/prefs_3d/prefs_others/
  quickmission/campaign_map 0px vs S69 refs — the `CT_LISTBOX` case only touches the OOB path);
  `map_playerlog` + `map_playerlog_tab1` rebased for the now-rendering tables. `campaign_map`
  confirmed byte-identical (art-face date readout). ASan **PASS 4/4 0 reports**. Stress under
  gl-lock (see sprint-70 board). `prefs_controls` remains an unstable oracle (live joystick).
- ⚠️ **ASan watch item (from S66) still open**: `worldinc.h:257`/`:565` packed-item accessors,
  ~1-in-50 stack-use-after-return, not attributed. Preserve the log if it reprises.
- **S72 opened the 3D-view parity frontier (I3).** A `gl-lock` cockpit A/B vs gold #10 shows the
  cockpit frame + instrument panel render a **crisp flat-black silhouette** (geometry rasterizes;
  only the fill is black) + a native-only black rectangle top-right (padlock-ADI inset).
  **Root cause narrowed:** the software rasterizer HAS the image-span fillers
  (`XASM_ImageHoriLine*`), world terrain + gunsight texture render, and `textureQuality` (High)
  doesn't gate it ⇒ the **cockpit-specific imagemaps resolve to black (not loaded/bound)** on the
  `btree::drw_cockpit` (`COCKPIT_OBJECT`) shape path. Fix NOT landed (deferred to a focused
  session — a deep per-poly texture-binding change).
- Next (S73): **the cockpit-black fix** — trace the cockpit poly's `Image_Map.GetImageMapPtr`
  binding vs a rendering world poly; then #11 external + the padlock-ADI inset. Smaller
  carry-overs: campaign **G2**, RScrlBar hosting, `ma_tabs_hit` click routing, #12 debrief
  capture, the doubled "F86 1" header cell (source-vs-BDG data delta).
