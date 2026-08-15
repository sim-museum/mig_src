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


## Rebinding keys (H2, S88)

The game's key table is data (`FIL_3D_KEYBOARD_TABLE`, file 0x7501) loaded into
`KeyMap3d::mappings[scancode][shiftstate]`. The port can now override it from a text file:

```bash
# 1. write out what is currently bound (uses the game's own action names)
cd <install dir> && MA_DUMP_BINDINGS=1 ./wmig      # writes ./controls.cfg
# 2. edit a line, e.g.   RESETVIEW = 0x0F
# 3. just run - controls.cfg is applied automatically at startup
./wmig
```

Format: `ACTION = <scancode>[, <shiftstate>]`, `#`/`;` comments. The dump is itself a valid input
file, which is the point: DirectInput scancodes are unguessable, but a line you can see is easy to
edit. `MA_CONTROLS=<path>` overrides the location; `MA_TRACE_KEY=1` logs each applied binding.

Applied **after** the game's own `Reg3dConv` load, never by editing the table: `Reg3dConv`
checksums what it loads and quits with *"Key table has changed between loads???"* if two loads
disagree, so the game must always see its own unmodified file.

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

## Current state (2026-08-14, after Sprint 102)

- ⭐ **S102: 3D overlay text is legible** (PO-4/PO-5 CLOSED). The in-flight menus, HUD readouts and
  map text render as letters. The font map keeps its glyphs in an `alpha` plane that the software
  span fillers never sampled — `Polygon.cpp`'s `ma_putc_alpha_blit` now renders them the way the
  hardware path does. `MA_NO_ALPHATEXT=1` reverts to the old (solid-bar) engine dispatch;
  `MA_NO_GLYPHS=1` removes glyphs entirely — the two controls that make a text capture provable.
  Diagnostics: `MA_GLYPH_DUMP=<char>` prints one atlas cell as ASCII art, `MA_GLYPH_SELFTEST=1`
  counts atlas ink and `PutC3` calls, `MA_TRACE_FONT=1` reports the font map and font colour.
- **Gold VIDEOS** (PO, 2026-08-14) are the oracle for the open play-test defects:
  `port/tools/gold_video.sh list|frame|crop|sheet|geom`.
- **Open PO defects (EPIC J in `scrum.md`):** PO-6 map window text · PO-7 radio menu ("R") ·
  PO-8 info line · PO-9 mission result after ALT+X · PO-10 "?" documentation · PO-11 missing
  widgets · PO-12 a hardware-graphics option in Preferences (BoB already runs hardware).

- ⭐ **S85: the Directives dialog opens too** — Auto Generate/Auto Display/Alpha Strikes tickboxes
  and the category table (Air Superiority, Choke, Supply, Airfields, Rail, Road, Army) with live
  Strike/Fighters/Targets/Missions values. Both dialogs deferred since S52 now open on real clicks.
- **S85: recipes can NAME a control — `f,#ID@Class[:COL]`** (e.g. `250,#2074@CMainToolbar`). Needed
  because `RESOURCE.H` reuses ids: five symbols are 2074, so a plain `#2074` hit the map-filters
  toolbar's twin and fired at a class with no handler — a silent no-op. An ambiguous unqualified id
  now prints every candidate (host class + rect) unconditionally.
- ⭐ **S84: the Intelligence (Authorise) OOB dialog OPENS, fully populated** — five tabs, sort
  combo, real objective table — after being deferred since S52. Two blockers, both ours: (a) eight
  more half-applied for-scope hoists (`char i` shadowing the hoisted `i`) in `CSupply` and
  `DirControl`; (b) `RDialog::OnGetFile` holds its fileblock **per dialog** while the engine allows
  one open per FileNum, so the map toolbar and the dialog's identically-arted button collided on
  `FIL_ICON_MISSIONRESULTS` — now served from the already-open block
  (`fileman::MA_GetOpenFileData`; `MA_NO_SHARED_FILEBLOCK=1` reverts).
- **S84: `#ID` recipes now resolve toolbar buttons correctly** (`Hosted` records the offset paint
  drew at). ⚠ But numeric ids are **ambiguous** — `RESOURCE.H` has five symbols for 2074 — so an
  `#ID` drive that "does nothing" may have hit a different control entirely.
- **S83: the port's Rowan-message dispatcher answers 0 for six routes it never implemented**
  (`RDialog::OnRowanMessage` covers 8 of the engine's 14 `ON_MESSAGE` routes; the rest hit
  `default: return 0`, which callers deref). The unrouted six are now listed in-code with why, and
  `MA_TRACE_MSG=1` names any unrouted message + its class. Four unguarded derefs hardened
  (`CRButtonCtrl::OnLButtonUp`/`OnMouseMove`, both `CRComboCtrl` sites).
- **S83: `MA_OOB_NO_DEFER=1`** lifts the guard on the two OOB dialogs the click path defers
  (Authorise 2023 / Directives 2074). Their **SEGV is fixed** — a half-applied for-scope hoist left
  `int i;` shadowed by the loop's own `int i` in `CSupply::AddSupplyMission`, so `target[i]` indexed
  uninitialised stack. Both dialogs now build and paint all five tabs. They remain deferred for a
  **new** reason: `[SysError] Opened file block (6a78) again without closing!` → SayAndQuit (same
  double-open family S79 fixed for 0x6a63). Top of the S84 backlog.
- ⭐ **S82: the campaign-map OOB dialogs are INTERACTIVE.** Player Log / Squads / Bases / DIS /
  Overview / Weather had been **render-only** — the map idle routed clicks to the two toolbars and
  nothing else, so they drew perfectly and ignored every click. Now: an open dialog gets first
  refusal on the click, its **tab bar switches on a real click**, the title bar's **✓ dismisses it**
  (through the owning dialog's derived `OnOK`), and a click inside it that hits no control is
  swallowed instead of panning the map behind. `MA_NO_OOB_CLICK=1` reverts.
  ⚠ Do **not** drive `CRButtonCtrl::OnLButtonUp` — it opens by dereferencing
  `GetParent()->SendMessage(WM_GETHINTBOX,…)` and `ON_MESSAGE` is an empty macro in compat (NULL
  deref). Drive the DOWN half and report the dispid the UP half would have fired.
- ⭐⭐ **S80: the G2 FLYABLE MULTI-MISSION LOOP RUNS — the campaign lifecycle is end-to-end.**
  Two campaign missions flown back-to-back in one process, each debriefed, the period advanced
  between them, and the campaign carried on to its own **end-of-campaign screen**. Recipe (headless,
  under `gl-lock`): `MA_CAMP_FLY=1 MA_CAMP_LOOP=2 BOB_AUTOEXIT=40 MA_ENABLE_3D=1` with
  `SDL_VIDEODRIVER=dummy`; add `MA_CAMP_LOOP_SHOT=1 MA_SHOT_PATH=…` for a capture the drive arms
  itself. The campaign clock is logged each step (`7/8/50 planning → debrief → 7/19/50 planning →
  … → 7/20/50 → campend`). **The residual blocker was our own test harness**, not the game: three
  one-shot `if (++n == N)` statics (frag drive, Fly click, `BOB_AUTOEXIT`) can each fire only once
  per process, so mission 2 launched into 3D and flew forever. `BOB_AUTOEXIT` is now per-flight.
- ⭐ **S81: G2 state persistence CLOSED — the campaign round-trips across processes.** Fly/advance
  a campaign, exit, and a NEW process resumes the same state from the canonical
  `SaveGame/Auto Save.sav` (verified: run A advances 6/25/50 → 7/3/50 → 7/8/50; a fresh run B comes
  up at 7/3/50). Root cause of the old behaviour: `fileman::namenumberedfilelessfail` lacks the
  hard variant's "fake long file name" branch, so it fell through to DIR.DIR's fixed **12-byte**
  8.3 name — and `"Auto Save.sav"` is 13 chars, so it was written *and read* as `Auto Save.sa`.
  Self-consistent, hence symptomless. `MA_TRACE_SAVE=1` traces the whole name assembly;
  `MA_NO_LONGNAME=1` reverts the fix.
- **S80: the 2D parity gate is one command — `port/parity_2d.sh`** (captures + pixel-compares vs
  `port/ref/native/`). **5/5 byte-identical as of S81** — `campaign_map` is back in the default set:
  the gate pins `port/ref/save/campaign_pristine.sav` around that capture and restores the player's
  own save afterwards, so the screen is reproducible again.
- ⭐ **S79: the G2 flyable multi-mission loop's blocker is FIXED.** Flying a campaign mission now
  completes the debrief and **advances the campaign** (map date "Morning, planning"→"debrief",
  `NextMission` called, operational map returns cleanly) — before, the campaign debrief hung.
  Root cause (S77→S78→S79 chain): a duplicate-`fileblocklink` corruption when the debrief map
  preload re-opens an already-open `FIL_ICON_BASES` (0x6a63). Fix (14 lines): read-only
  `fileman::MA_IsFileOpen` + guard the preload (`FULLPANE.CPP:2706`) to skip already-open files.
  Gates: 2D byte-identical (title/prefs_3d/campaign_map 0px), stress 20/20. Left: drive the
  debrief Next Period → next flyable mission (now reachable); state persistence.
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
