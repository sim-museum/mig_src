# Scrum — Native Linux Port of Mig Alley

> **Epic:** *Complete the port of Mig Alley to Linux.*
>
> This repository (`/home/admin/ma`, branch `linux-port`) is the **Mig Alley** native Linux
> (SDL2) port of the 1999 Rowan engine. The epic is: **take the native 32-bit ELF build
> from its current state (first 3D frame rendered) to a fully playable, shippable game with
> no Wine dependency.** The runtime is built on the reused `bob_*` engine glue
> (`bob_main` / `bob_init_instance` / `bob_run` / `g_pBobApp` / `bob_video` …), shared with
> the sibling Battle of Britain port.

---

## 1. Product Vision

> *For* flight-sim enthusiasts and preservationists *who* want to run Mig Alley on modern
> Linux without Wine/DirectX, *the* native SDL2 port *is a* self-contained 32-bit ELF build
> *that* renders, simulates, and plays the full game (2D front-end, 3D flight, campaign,
> audio) at native speed *unlike* the emulated Wine path, which is fragile and non-native.

**Product Goal (this release train):** A reliably launchable build where a user can start
the app, navigate the menus, configure preferences, and fly a mission to completion with
working controls and audio — all native.

---

## 2. Scrum Framework Setup

| Element | Value |
|---|---|
| **Product Owner** | Repository maintainer (`curator`) |
| **Scrum Master** | Rotating / lead engineer |
| **Developers** | Porting engineer(s) + automated agent (Claude) |
| **Sprint length** | 2 weeks |
| **Estimation** | Story points (modified Fibonacci: 1, 2, 3, 5, 8, 13, 21) |
| **Velocity (assumed)** | ~20 pts/sprint (re-baseline after Sprint 1) |
| **Backlog tool** | This file (`scrum.md`) + git history as the audit trail |

### Ceremonies
- **Sprint Planning** — start of each sprint; pull from top of Product Backlog into the Sprint Backlog up to capacity.
- **Daily Scrum** — async standup; 3 questions captured as a one-line commit/log note.
- **Sprint Review** — demo the *shippable increment* (run the binary, show the new capability).
- **Sprint Retrospective** — append a short note to the relevant phase in `CLAUDE.md` / memory `migalley-port-state`.
- **Backlog Refinement** — mid-sprint; re-estimate, split stories that proved too big.

### Definition of Ready (DoR)
A story is ready when it has: a clear user-value statement, acceptance criteria, a rough estimate, and no unresolved blocking dependency.

### Definition of Done (DoD)
A story is done when:
1. Code compiles clean into the unity/MFC/OLE build set (`port/rebuild.sh`).
2. `wmig` links to a 32-bit ELF with **0 undefined symbols**.
3. The capability is demonstrated by running the binary from the install dir
   (`cd <drive_c>/rowan/mig && ./wmig`, bare launch since S30) — not just compiled.
4. No regression in previously-passing phases (title screen, Preferences, first 3D frame).
5. Relevant gated trace (`MA_TRACE_*`) added for any new subsystem.
6. `CLAUDE.md` status + memory note updated; change committed on `linux-port`.

---

## 3. Release Plan (Increments)

> **PO-accepted first-release target: R2 — Flyable 3D** (ratified Sprint 1 planning, 2026-06-17).
> The train can stop earlier, but the PO's first *formal acceptance* gate is R2. A brief
> increment review happens at each Sprint Review; formal accept/ship decision at R2.

| Release | Theme | Sprints | Shippable outcome |
|---|---|---|---|
| **R1** | Stable launch & front-end | 1–2 | Deterministic boot to title + Preferences; persistence to disk |
| **R2** ⭐ | Flyable 3D *(PO accept gate)* | 3–5 | Reliable 3D flight with input control, visually A/B-correct |
| **R3** | Immersion | 6–7 | Audio + video (Smacker) + HUD/cockpit complete |
| **R4** | Full game | 8–9 | Campaign / mission management playable end-to-end |
| **R5** | Polish & ship | 10 | Packaging, controls config, performance, docs — v1.0 |

Each release is a usable product; the train can stop at any release boundary and ship.

---

## 4. Product Backlog (ordered by value/risk)

> Status legend: ✅ done (baseline) · 🔨 in progress · ⬜ not started

### EPIC A — Runtime stabilization *(highest priority: nothing else is reliable without it)*

| ID | User Story | Pts | Acceptance Criteria | Status |
|---|---|---|---|---|
| A1 | As a player, I can launch into 3D flight **every time** without intermittent crash/window-close, so the game is dependable. | 13 | 20/20 consecutive launches reach the cockpit view; no SIGSEGV/SIGFPE; 3D-startup races (sim thread vs `MakePassive`/`View3d::Drawing`) hardened. | ✅ **20/20** (S1) |
| A2 | As a player, my Preferences persist across runs, so I don't reconfigure each launch. | 5 | `Save_Data` written to disk on exit; reloaded on next boot; round-trip verified. | ✅ (S2: round-trip PASS) |
| A3 | As a maintainer, the build is reproducible from one command, so onboarding is trivial. | 3 | `port/rebuild.sh` + documented link line yields `wmig` from clean tree; 0 undefined symbols. | ✅ (maintain) — plus `CMakeLists.txt` (CMake+Ninja) for **incremental** builds: identical 270 TUs / identical symbol set, 84 s full vs ~1 s single-TU. rebuild.sh stays the fallback. |
| A4 | As a maintainer, startup race conditions are diagnosable, so regressions are caught fast. | 3 | Thread-ordering invariants asserted/logged under `MA_TRACE_3D`; a stress-launch harness script in `port/`. | ✅ `port/stress_launch.sh` (S1) |

### EPIC B — 3D flight fidelity

| ID | User Story | Pts | Acceptance Criteria | Status |
|---|---|---|---|---|
| B1 | First native 3D frame renders the flight view. | 8 | Cockpit view: sky/horizon/terrain/HUD; ~95% back-surface coverage; crash-free single frame. | ✅ |
| B2 | As a player, the rendered scene matches Wine output, so visuals are correct. | 13 | A/B frame capture (`MA_DUMP_BACK`) vs Wine within tolerance for ≥3 representative views; color/palette correct. | ⬜ |
| B3 | As a player, the 3D view animates smoothly during flight, so it feels like a sim. | 8 | Sustained ≥30 fps over a 60s flight; no tearing/stale-buffer artifacts; 2D present correctly gated off in-3D. | ✅ (S32: ~50 fps sustained / 3048 frames / 62s, sim-paced; `MA_TRACE_FPS`) |
| B4 | As a player, the cockpit/HUD instruments read correctly, so I can fly on instruments. | 8 | HUD elements (airspeed, alt, heading) render and update with sim state. | ✅ (S25: enemy-disk + ADI default-on) |
| B5 | As a player, I can pick a higher resolution (incl. my display's native), so flight is crisp. | 8 | Combo offers modes up to 1920×1080 (4:3 + 16:9); selection applies to windowed flight; window centers/borderless-fills. | ✅ (S34: up to 1920×1080; ADI kaleidoscope-on-bank fixed) |
| B6 | As a player, the 2D overlays are correct at high resolution, so maps/kneeboard are usable at 1080p. | 13 | At ≥1600-wide: campaign map renders without tiling + shows icons + wheel-zoom doesn't resize the window; kneeboard page renders. (3D world already resolution-independent.) | ⬜ (high-res 2D-layer scaling; ADI done S34) |
| B7 | As a player, the F-86 radar gunsight ranges/expands with target range, so gunnery is accurate. | 8 | `DOGUNSIGHT` reticle scales with locked-target range on the software path. | ◐ **S89: characterized — NOT a port bug.** The chain (`shape::GetRadarItem` → `CalcRadarRange` → `SphereXScale/YScale`) is compiled and reachable; it is gated on the opt-in difficulty settings `GD_PERFECT/REALISTICRADARASSISTEDGUNSIGHT` (Game tab *Gunsight Ranging*), off by default. `MA_FORCE_RADARSIGHT=1|2` opens the gate headlessly and `MA_TRACE_GUNSIGHT` logs sightings + LOCKs. **S90:** locks achieved (715, ranges 911k-1216k) via `MA_FORCE_RADARSIGHT=2`; the true observable is `RequiredRange = radarRange` (`3DCOM.CPP:20661`, clamped 20000..100000), **not** `SphereXScale/YScale` (view projection). **Still open:** every lock is ~1.2M, above the clamp, so the reticle pins at max range — needs a target inside gun range. |

### EPIC C — Input

| ID | User Story | Pts | Acceptance Criteria | Status |
|---|---|---|---|---|
| C1 | As a player, I can fly with keyboard via DirectInput→SDL, so controls work in 3D. | 13 | Pitch/roll/yaw/throttle + view keys mapped SDL→engine; responsive in flight. | ✅ (S3: view-pan demo, 115 actions) |
| C2 | As a player, I can use a joystick, so flight is natural. | 8 | SDL game-controller/joystick axes→flight controls; deadzone/calibration. | ✅ (S10: live fly-validated) |
| C3 | As a player, mouse navigation works across all menus, so the UI is complete. | 5 | Click/hover hit-testing on all front-end panels (extends current listbox/button/combo). | ✅ (S2–S4 front-end; S18 in-flight `AU_UI_X/Y`) |
| C4 | As a player, SHIFT+D boxes the padlocked bogey and ALT+D shows its telemetry, so I can track targets (the Wine two-patch feature). | 13 | SHIFT+D draws a red box around the padlocked bogey (3D→2D projected); ALT+D adds text beside it: bogey kts [closure], range (ft→Nm), own kts @ relative alt. Toggleable. | 🔨 **baseline works** (engine `d`/`BOXTARGET`: red diamond + Range/Bearing/RelAlt, PO-verified). Enhancements: **C4a DONE** (box sizes from a projected world extent, grows as the bogey closes — S92 found it already implemented); **C4b DONE** (`g_adi_box`/`g_adi_telem` split); **C4c open** (adaptive black/white telemetry colour); **C4d written but UNVERIFIED (S92)** — own speed + closure (via `RealFrameTime`) added to the readout, bogey speed omitted (no reachable per-target speed field); no capture shows it because headless padlock engagement fails (`BOB_KEYSEQ` taps do not reach the view-selection path — the blocker to fix first). |

### EPIC D — Audio

| ID | User Story | Pts | Acceptance Criteria | Status |
|---|---|---|---|---|
| D1 | As a player, I hear sound effects, so flight is immersive. | 13 | Miles `AIL_*` stubs → SDL audio mixer; engine/gunfire/UI SFX audible. | ✅ (S6: `ma_openal.cpp` OpenAL) |
| D2 | As a player, I hear music/ambient tracks. | 5 | Streaming/looped audio via SDL; volume honors Sound prefs. | ✅ (S6: `ma_music.cpp` XMI→SMF→FluidSynth) |

### EPIC E — Video (Smacker)

| ID | User Story | Pts | Acceptance Criteria | Status |
|---|---|---|---|---|
| E1 | As a player, intro/briefing Smacker videos play, so cutscenes work. | 13 | `Smack*` stubs → real decode (libsmacker) → `ma_ddraw_present`; introsmack plays on title launch. | ⬜ |

### EPIC F — Front-end completeness

| ID | User Story | Pts | Acceptance Criteria | Status |
|---|---|---|---|---|
| F1 | Title screen + Preferences render and persist natively. | 21 | title.bmp + interactive settings; tab nav; click-to-change; writeback. | ✅ |
| F2 | As a player, combo boxes show a real dropdown list, not cycle-on-click. | 5 | Dropdown renders options; selection sets value. | ✅ (S2) |
| F3 | As a player, the RESOLUTIONS combo is populated, so I can pick a mode. | 5 | HW display-mode enumeration → combo entries. | ✅ (S2: 640/800/1024) |
| F4 | As a player, all front-end screens (not just Preferences) are usable. | 13 | Campaign/QuickMission/Comms panels render and navigate. | ✅ (S4: QuickMission+Campaign; Comms=MP, §8 out-of-scope) |

### EPIC G — Campaign / mission

| ID | User Story | Pts | Acceptance Criteria | Status |
|---|---|---|---|---|
| G1 | As a player, I can start a Quick Mission and fly it to completion. | 21 | Mission load → 3D flight → end-of-mission; no crash. | ✅ (Hot Shot end-to-end: kills, debrief; S21–28 hardening) |
| G2 | As a player, I can play the campaign across missions, so the game is complete. | 21 | Campaign state load/save; mission chaining; debrief. | 🔨 **S76 scoping: core works, far more complete than expected.** Headless-verified: (a) single-mission flow map(icons/frontline/routes/date)→frag→briefing→**campaign flight**→flight-close→**debrief**; (b) multi-mission **chaining** — NextDay/NextMission advance opens **"MISSION 2 BRIEFING"** (D.I.S.). Remaining: ~~(1) **flyable multi-mission loop**~~ — ✅ **DONE (S80)**: two campaign missions flown back-to-back in one process, each debriefed, Next Period driven between them (`MA_CAMP_LOOP=N` → the genuine `CDebriefToolbar::OnClickedNextPeriod`), campaign clock advancing `7/8/50 planning → debrief → 7/19/50 planning → … → 7/20/50` and on to the **end-of-campaign screen**. S79 had fixed the crash (duplicate-`fileblocklink` corruption from the debrief preload re-opening an already-open `FIL_ICON_BASES`; `fileman::MA_IsFileOpen` + skip guard at `FULLPANE.CPP:2706`); the residual blocker was **three one-shot `++n == N` statics in the test harness**, not game code. **The whole campaign lifecycle now runs end-to-end.** ~~(2) state **persistence** across missions~~ — ✅ **DONE (S81)**: campaign state round-trips across processes under the canonical `Auto Save.sav` (run A advances `6/25/50 → 7/3/50 → 7/8/50`; a fresh run B resumes at 7/3/50). Root cause was `fileman::namenumberedfilelessfail` missing the hard variant's "fake long file name" branch, so it fell through to DIR.DIR's fixed **12-byte** 8.3 name — and `"Auto Save.sav"` is 13 chars. Persistence had actually been *working* under the truncated `Auto Save.sa`, self-consistently and invisibly, while the canonical file went untouched. (3) edge/polish — **the only G2 item left**. Test recipe: `MA_CAMP_FLY=1 BOB_AUTOEXIT=60` (fly a frag→debrief) / `MA_CAMP_NEXTDAY=1` (advance) under `SDL_VIDEODRIVER=dummy`. |

### EPIC H — Ship

| ID | User Story | Pts | Acceptance Criteria | Status |
|---|---|---|---|---|
| H1 | As a user, I can install and run without manual env vars, so it's distributable. | 8 | Launcher resolves data dir; packaged artifact; README run instructions. | ✅ (S30 data-dir + bare launch; S31 README run/install; **H1-pkg**: `packaging/install.sh` + `packaging/build-appdir.sh` + `packaging/README.md`, both verified locally) — residual: native `.deb`/`.rpm` + a cross-machine AppImage test |
| H2 | As a user, I can rebind controls, so the game fits my setup. | 5 | Controls screen writes a remappable bindings file consumed by C1/C2. | ⬜ |
| H3 | As a maintainer, the port is documented for contributors. | 3 | `PORTING.md` + `scrum.md` reflect final architecture. | 🔨 |

### EPIC I — Wine-parity screens *(PO-added 2026-07-25)*

> **Gold standard:** PO-supplied captures of the Windows build running under Wine:
> `/run/media/admin/BEA6-BBCE/ma/` (14 PNGs, taken 2026-06-24). The native port's
> screens must match these. Generalizes B2's A/B idea from "3 representative views"
> to the full PO-curated screen set, with the gold shots as the fixed oracle.

| ID | User Story | Pts | Acceptance Criteria | Status |
|---|---|---|---|---|
| I1 | As the PO, I have an inventory mapping each gold screenshot to its native screen and repro path, so parity work is scoped and diffable. | 3 | All 14 shots identified (screen name + native nav/env recipe + native capture alongside); table in `port/scrum/screen-parity.md`. | ✅ **COMPLETE** (S75: the last shot, #12 debrief, captured headless via `BOB_AUTOEXIT`+`MA_SHOT` — all 15 gold shots now have native captures in `port/ref/native/`; oracle = BDG 0.85F, provenance flagged) |
| I2 | As a player, every 2D front-end screen (title, Preferences tabs, Quick Mission, Campaign panels, map) matches its Wine gold shot. | 13 | Side-by-side native-vs-gold captures agree on layout, art, fonts, colours within stated tolerance; each deviation fixed or explicitly PO-waived in the parity table. | 🔨 S58: **verdicts flipped on real captures — #7 Controls CLOSE, #8 Others CLOSE, #1 title first-captured CLOSE** (S57 fixes capture-verified); 2D captures now GL-free (`MA_SHOT`, byte-identical to GL runs) + uninit-PX ctor fix (`RLISTBXC.CPP`) cleaned tab bar/title menu. Open: #9 stray combo (in-template, runtime-hidden on Windows — mechanism unrouted), text word-wrap, cross-cutting font/chrome (#1/#2), #12 debrief capture |
| I3 | As a player, the in-flight / 3D / campaign-map views match their gold shots. | 13 | As I2 for the 3D-view shots; reuses `MA_DUMP_BACK`/frame-dump harness with `GL_PACK_ALIGNMENT=1` (S45 lesson). | 🔨 S73: **#10 cockpit + #11 external → CLOSE** (cockpit-black FIXED — stale software `palette_table` at cockpit-draw time; re-enabled the engine's `//dead` per-object `SelectPalette(0)` reset at `BTREE.CPP:580`). Remaining I3: campaign-map 3D + same-view recaptures. |
| I4 | As a player, the campaign-map **Player Log** OOB dialog matches the Wine gold shot (PO-added 2026-07-26): Career tab with pilot photo, Name edit box, per-type Sorties/Combats/Kills/Losses table (F86 1 / F86 2 / F80 / F84 / F51 / All), Career / Log of Missions / Last Mission tab bar, ?/✓ title buttons — over the strategic map with toolbar + date "6/25/50: Morning, planning". | 8 | Gold: `/home/admin/Pictures/Screenshots/Screenshot From 2026-07-19 20-33-27.png` (treat as gold shot #15). Native Playerlog capture (S54 OOB path) side-by-sides it in `port/scrum/screen-parity.md`; content populated (photo art, table rows, editable Name), all three tabs render; deviations fixed or PO-waived. | ◐ S60: **two engine root causes fixed** — template-declared OCX controls with no `DDX_Control` were never created (kind-driven hosting now covers RStatic/RButton/**RTabs**), and no RDialog in a dialog tree ever learned its own size (`MaSeedTemplateSize`). RTabs hosted; all 3 tabs register with gold captions; real tab art loads from RTabs.ocx; **Name label + edit box now render**. **Acceptance NOT met**: tab bar + title bar are drawn but not composited at the right offset (suspect `OnGetXYOffset`); content table never pulled. S56: first native capture (`MA_OOB_PLAYERLOG` hook) |

### EPIC J — PO play-test defects *(PO-added 2026-08-09, extended 2026-08-14)*

> **Gold standard for this epic: the PO's two VIDEO recordings** of the Windows build under Wine,
> `~/gold standard/ma/260814_mig_alley_start_campaign_and_exit.mp4` (45 s: start a campaign and
> exit) and `260814_mig_complete_campaign.mp4` (353 s: a complete campaign mission, including the
> map window text and the radio menus). Frames via `port/tools/gold_video.sh` (S102). These items
> are **behaviours** — what a key press does, what appears after a click — which a still gold shot
> cannot settle; the video can. Geometry differs between the two recordings (short 1280×1024,
> full 1200×1080): measure with `gold_video.sh geom`, and never judge size/density across the
> gold↔native boundary (S64).

| ID | User Story | Pts | Acceptance Criteria | Status |
|---|---|---|---|---|
| PO-1 | As a player, I can exit/resize from the campaign map widgets. | 5 | The `CSystemBox` cluster is drawn with correct art and the X returns to the title screen. | ✅ **CLOSED (S97)** |
| PO-2 | As a player, the campaign map is the right size. | 3 | Map canvas matches gold. | ✅ **CLOSED (S96)** — it had been 221 px too wide since it first rendered |
| PO-3 | As a player, clicking a map recon icon opens its dossier. | 3 | Icon click → dossier dialog. | ✅ **CLOSED (S95)** |
| PO-4 | As a player, in-flight/overlay text is visible. | 8 | HUD/menu/map overlay text renders as legible letters. | ✅ **CLOSED (S102)** — the software span fillers sample `body` (constant 31) and never the `alpha` plane where the glyph is; the shipped game draws text through the **hardware** `direct_3d::PutC`. Fixed by rendering the glyph from `alpha` with `fontColour` (`ma_putc_alpha_blit`). Three-arm A/B: letters / solid bars (`MA_NO_ALPHATEXT=1`) / nothing (`MA_NO_GLYPHS=1`) |
| PO-6 | As a player, the in-flight **map window** shows its text, so I can read waypoints and the map command list. | 5 | Gold (`full` @ ~90 s): clock + waypoint name top-left ("5:24 Koesan"), the waypoint table along the bottom (Rendezvous / Ingress / Initial Point with alt/time/heading/range, ingress in red), and the right-hand command list ("1.NextWP=HighlightedWP", "2.AccelToNextWP" in red, "0.Exit"). Native capture shows the same lines. | 🔨 **S103 drove it and localised it:** `M` (GOTOMAPKEY, DIK 0x32 per the game's own binding dump) **opens the map window** — map, route line, kneeboard panel and cockpit art all render — and **every text element is missing**. So it is not the glyph path (S102 fixed that, and the in-flight menu prints) and not key delivery: it is the map screen's own text drawing. NB `DrawInfoBar` returns early when `pCurScr==&mapViewScr`, so the map's text comes from `MapScr`, not the info bar. ✅ **CLOSED (S105)** — the map window's text renders: the command menu ("1.Accel / 2.Waypoints / 3.Radio / 4.Zoom / 0.Exit") in the kneeboard and the clock + waypoint name ("9:00 E. Pyongyang City") top-left; capture `port/ref/native/map_window.png`. **Cause: overlay text was drawn through the 3D sub-window, which is created `WINSH_MID` and therefore has its origin at the screen CENTRE** (`WRAPPER.CPP:444` shifts `logicalscreenptr` by `-PhysicalMinX*bpp -PhysicalMinY*pitch` = +307840 bytes at 640×480×16). Overlay text is laid out in absolute top-left coordinates, so every glyph was displaced by (320,240) onto the map. Found by painting each glyph cell magenta (`MA_TEXT_MARK=1`): 1924 marked pixels, rows 243–256, cols 344–481. Fixed by blitting through `currscreen->Master()`; `MA_TEXT_WINBASE=1` reverts. **Completed in S107:** the waypoint sub-screen renders fully too (`port/ref/native/map_waypoints.png`) — "1.Next WP = Highlighted WP / 2.Accel To Next WP / 0.Exit" plus the waypoint table along the bottom, which is the gold video's ~90 s screen. The residual was the harness, not the game (see PO-13). Earlier: **S104 identified the gold's actual screen: `waypointMapScr`** — its option list is exactly the gold's right-hand panel (`IDS_MAP_SETNEXTWP` "1.NextWP=HighlightedWP", `IDS_MAP_ACCELTONEXTWP` "2.AccelToNextWP", `IDS_MAP_EXIT` "0.Exit") and its `extraRtn` is `MapScr::UpdateWaypointDisplay`, which draws the Rendezvous/Ingress/Initial-Point table with alt/ETA/bearing/range. It is reached from `firstMapScr` option **2** (`SelectFromFirstMap`), so the next step is to drive M → 2 and capture with `MA_UISCR_SHOT`. Note `UpdateWaypointDisplay` draws nothing when `OverLay.curr_waypoint` is NULL |
| PO-7 | As a player, pressing **R** in a campaign mission opens the radio command menu. | 8 | Gold (`full` @ ~190 s): a translucent panel with "1.Givefreedom … 7.NotClear!" and "0.Exit" in red; number keys select. Native: R opens it and a selection is delivered. | ◐ **S104: the menu OPENS and is legible** — captured (`port/ref/native/radio_menu.png`): "1.Group Info / 2.Precombat / 3.Combat / 4.Postcombat / 5.Tower / 6.FAC/Bomb" + "0.Exit" in red. **R was never broken**: every gate in the chain passes and always did, and the screen lives its full 5 s (`TimeLimitedDisplay`, `budget=500 -= FrameTime()=2`). What the PO saw is reproduced exactly by `MA_NO_ALPHATEXT=1` — an opaque grey box of white blocks, i.e. **PO-4's defect**, which is why fixing PO-4 fixed this. Gated by `port/overlay_text.sh radio` (letters 848 edges vs blocks 351 vs blank 207). **Left open:** a number-key selection is not yet verified headlessly — the taps are delivered and the throttle consumer is correctly guarded by `if (!OverLay.pCurScr)`, but the in-flight pump rate is low enough that taps land seconds apart; drive the option key from the screen's own frame loop next |
| PO-8 | As a player, the in-flight **info line** reads out my aircraft state. | 5 | Gold: bottom line "Speed:137Kts Mach:0.21 Alt.:4715ft Hdg:98 Thrust:49". Native shows the same fields updating. | ✅ **CLOSED (S103)** — "select your own target! / Speed: 438Kts Mach: 0.73 Alt.: 16724ft Hdg: 279 Thrust: 0". Root cause was far larger than the info line: `SaveData::InitPreferences` (the game's default-setting code **and the only reader of `settings.mig` in the tree**) has only two call sites — the demo build and the intro-Smacker route — so the port never called it. Preferences were written on every exit and **never once loaded**; `infoLineCount` sat at 0 because its default of 1 never ran. Three local patches had each treated one symptom (unit factors, HUD instruments, sound volumes); all three retired |
| PO-9 | As a player, exiting a campaign mission with **ALT+X** shows the mission result. | 8 | Gold (`short` @ ~36 s): a **MISSION RESULTS** panel over the campaign map, bottom right — Objective / Task / Result / Redo ("Munsan-Seoul Rail-line · Reconn · Failure · no"), a squadron photo, buttons **I.D. · Debrief · Redo · Next Period**, ?/✓ title buttons, with the map date advanced to "Morning, **debrief**". Native: same panel after ALT+X. | ✅ **CLOSED (S106)** — the panel appears with the gold's own content: Objective **Munsan-Seoul Rail-line** · Task **Reconn** · Result **Failure** · Redo **no** (`port/ref/native/mission_results.png`). **Cause: the paint walk enumerated one toolbar.** The post-flight CAMP branch already called `DebriefToolBar().OpenMissionresults()` and it returned a live dialog every time — but the map idle's `ma_map_paint_oob` only walked `m_toolbar2` (`CMainToolbar`), and MISSION RESULTS is logged against `m_toolbar5` (`CDebriefToolbar`). Both toolbars are now walked, for painting **and** clicks. Delivered via `BOB_KEYSEQ="500,0x2D,0x38"` (S105's modifier field — `EXITKEY` is DIK 0x2D with shift state 2; a bare 0x2D is `RESETRECORD`). Residual: at 800×600 the panel's I.D./Debrief/Redo/Next Period row falls below the visible area — a placement question at a different resolution (S64 caveat) |
| PO-10 | As a player, "?" opens the documentation window. | 13 | A viewer renders the topic for the screen the "?" was pressed on. | ◐ **half** — S98 fixed the routing (four independent breakages) and `CWinApp::WinHelp` is still a stub; S99 solved 4 of 5 `MIG.HLP` decode stages (Hall opcode table for the text stream unsolved, `port/tools/hlp_extract.py --verify` prints WRONG today) |
| PO-11 | As a player, the campaign screens have all their widgets. | 13 | A widget-by-widget inventory against gold video frames; each missing widget either implemented or listed with its cause. | ⬜ **First pass (S102, gold `short` @ 36 s vs `port/ref/native/campaign_map.png`)** — apparently absent natively: the **blue + red filter toolbar rows** (~16 icons each), the **right-hand toolbar group** (zoom in/out, save…), the **"MIG ALLEY" title-bar chrome** (native draws the date alone), the **scale ruler** down the left edge (0–350 Nm) and the **vertical scrollbar**. ⚠ **Not yet a defect list:** the gold frame is **1920×1080** and the native reference is **800×600**, and this engine picks its panel art set BY RESOLUTION (S64) — so step one is a native capture at the gold's own resolution. Judging "missing" across that boundary is the exact mistake S64 recorded |
| PO-13 | As a player, pressing a number key **inside** an in-flight menu selects that option, so the menus are usable and their sub-screens reachable. | 5 | Driving `R` then `3`, or `M` then `2`, reaches the submenu (gold `full` @ ~190 s shows the combat submenu; the map's option 2 is `waypointMapScr`, whose `UpdateWaypointDisplay` draws the gold's Rendezvous/Ingress/Initial-Point table). | ✅ **CLOSED (S107) — never a game defect.** `MA_TRACE_KEYEAT=<action>` watches `KeyPress3d` itself (the only honest way to find a consumer of a test-and-CLEAR) and showed the digit consumed **before the menu existed**: `[keyeat] KeyPress3d(106) bit=1 ret=1` *precedes* the `promote firstMapScr` line, i.e. `KEYFLY.CPP`'s throttle handler took it, correctly. The fault was the harness: `BOB_KEYSEQ` schedules on the **pump** counter, which in flight runs far slower than frames, so taps 20 pumps apart land seconds apart and these menus live 5 s. Fixed with **`MA_UISCR_KEY="0xNN[,frames]"`** — a key press armed when a screen is promoted, injected through the real buffered-keyboard queue (`ma_inject_dik`); the input twin of S104's `MA_UISCR_SHOT`. First try: `option key=1 selected -> promote waypointMapScr` |
| PO-12 | As a player, I can choose **hardware** graphics in Preferences, so the game renders through the path it was written for. | 21 | A primary-graphics option in Preferences selects hardware; the D3D path (`DoHardPoly`/`direct_3d`) renders flight and 2D; software stays selectable. | ⬜ **PO-added 2026-08-14.** BoB (`~/bob`, same engine) already runs hardware, so the *approach* cross-ports — but **not the code as-is**: per `ROWAN_ENGINE_LINUX_PORT_NOTES.md`, BoB is **D3D7 + Lib3D software-T&L** while MA is the older **DX5/6 execute-buffer** path (`WIN3D.CPP`/`HARDWIN.CPP` build execute buffers; `bob_video.cpp` already has the GL surfaces BoB's device sits on). Scope = an execute-buffer→GL device, not a port of BoB's device. High value beyond the option itself: it is the engine's own text/alpha path (`direct_3d::PutC`), the one the shipped game uses, so it retires a class of software-path workarounds — S102's included |

**Backlog total (open work): ~300 pts.**

---

## 5. Sprint Plan (rolling)

### 🏃 Sprint 107 — "Press it when it's there" (PO-13) — ✅ CLOSED 2026-08-15 (goal MET)

**Sprint Review (PO pre-approved ceremony, logged 2026-08-15):** detail in
`port/scrum/sprint-107.md`.

- **⭐ PO-13 was never a game defect.** `MA_TRACE_KEYEAT=<action index>` watches `KeyPress3d`
  itself — the only honest way to find who consumes a test-and-CLEAR — and one pair of lines settled
  it: `[keyeat] KeyPress3d(106) bit=1 ret=1` appears **before** `promote firstMapScr`. The digit
  arrived before the menu existed, so `KEYFLY.CPP`'s throttle took it, which is exactly what its
  `if (!OverLay.pCurScr)` guard is for.
- **The fault was the measuring apparatus.** `BOB_KEYSEQ` schedules taps on the **pump** counter;
  in flight pumps run far slower than frames, so taps 20 pumps apart land seconds apart — and these
  menus live five seconds. **`MA_UISCR_KEY="0xNN[,frames]"`** arms a key press when a screen is
  promoted and injects it through the real buffered-keyboard queue N frames later. The input twin of
  S104's `MA_UISCR_SHOT`. It worked first try.
- **⭐ Which lands on the gold's screen.** `waypointMapScr` now renders completely
  (`port/ref/native/map_waypoints.png`): "1.Next WP = Highlighted WP / 2.Accel To Next WP / 0.Exit"
  in the kneeboard and the **waypoint table** along the bottom ("Waypoint (1) 12000ft 0:00 355
  5.1Nm" …) — the gold video's ~90 s frame. **PO-6 is therefore complete, not partial.**
- **Gate:** `port/overlay_text.sh` gains the `waypoint` screen (1179 edges vs **0** with
  `MA_NO_ALPHATEXT=1`); three screens now.
- **Gates:** parity 5/5 · sweep 9 OPEN/0 CRASH · map click · map drag · sysbox exit · help click ·
  overlay text 3/3 · stress 20/20 · ASan 0.

**Retro.** Two sprints carried "the option key does not select" as an open defect and it was the
harness both times. The pair of lessons now reads: **arm the capture from the drive** (S104) and
**arm the input from the drive** (S107) — anything scheduled on an unrelated counter is a coin toss.

### 🏃 Sprint 106 — "The panel nobody enumerated" (PO-9) — ✅ CLOSED 2026-08-15 (goal MET)

**Sprint Review (PO pre-approved ceremony, logged 2026-08-15):** detail in
`port/scrum/sprint-106.md`.

- **⭐ PO-9 CLOSED.** ALT+X from a campaign mission now shows **MISSION RESULTS** — Objective
  *Munsan-Seoul Rail-line* · Task *Reconn* · Result *Failure* · Redo *no*, over the squadron photo.
  The gold video shows the **same four values** (same campaign state as the pinned fixture), so it
  is a content match, not just a shape match.
- **Cause: an enumeration that knew about one toolbar.** The post-flight CAMP branch already called
  `DebriefToolBar().OpenMissionresults()`, and the trace proved it returned a live dialog on every
  flight — but the map idle's `ma_map_paint_oob` walked only `m_toolbar2` (`CMainToolbar`), while
  MISSION RESULTS is logged against `m_toolbar5` (`CDebriefToolbar`). Created every time, painted
  never. Both toolbars are now walked for paint **and** click (a dialog you cannot click is half
  hosted, and this one has four buttons). **Smell worth naming: when a subsystem works for every
  case but one, check what enumerates the cases.**
- **ALT+X was only reachable because of S105's `BOB_KEYSEQ` modifier field:** `EXITKEY` is DIK 0x2D
  **with shift state 2**, and a bare 0x2D is `RESETRECORD` — the PO's exit route cannot be
  synthesised without holding Alt across the tap.
- **Residual, stated not hidden:** at 800×600 the panel's I.D./Debrief/Redo/Next Period row is below
  the visible area; gold shows it at 1920×1080. Placement at a different resolution (S64 caveat).
- **Gates:** parity 5/5 · sweep 9 OPEN/0 CRASH · map click · map drag · sysbox exit · help click ·
  overlay text · stress 20/20 · ASan 0.

**Retro.** Five PO defects closed in five sprints and five different causes — a span filler that
ignored the alpha plane, an initialisation nobody called, a timer nobody had photographed, a window
whose origin is the screen centre, and a paint walk that enumerated one toolbar. The only thing they
shared was how they looked to the player.

### 🏃 Sprint 105 — "The map's own words" (PO-6) — ✅ CLOSED 2026-08-15 (goal MET) — ⭐ the text was drawn through a centre-origin window

**Sprint Review (PO pre-approved ceremony, logged 2026-08-15):** detail in
`port/scrum/sprint-105.md`.

- **⭐ PO-6 CLOSED.** The map window now shows its command menu ("1.Accel / 2.Waypoints / 3.Radio /
  4.Zoom", "0.Exit" in red) and the clock + waypoint line ("9:00 E. Pyongyang City").
- **Cause: absolute coordinates drawn through a centre-origin window.** The map view renders through
  a `Window` created `WINSH_MID`, whose constructor shifts `logicalscreenptr` by
  `-PhysicalMinX*bpp -PhysicalMinY*pitch` — origin at the screen CENTRE, +307840 bytes at
  640×480×16, the exact offset the trace showed. Overlay text is laid out in absolute top-left
  coordinates, so every glyph was displaced by (320,240) onto the map. The HUD text was fine only
  because its current window happens to be the master screen. Fix: blit through
  `currscreen->Master()` (`MA_TEXT_WINBASE=1` reverts).
- **⭐ The instrument that cracked it: `MA_TEXT_MARK=1`** paints each glyph cell solid magenta —
  a colour that appears nowhere else in the game. One run turned "the text is missing" into a
  coordinate readout: **1924 magenta pixels, rows 243–256, cols 344–481**. Four prior measurements
  had all said "drawn, right colour, not declined" while the notepad stayed blank, because a
  screenshot cannot separate *drawn elsewhere* from *drawn then covered*.
- **The filter that was too coarse.** The "why did the fast path decline" trace deduped per
  REASON, so the front end's early declines swallowed the map screen's — re-keyed per
  (reason, colour). Fifth booking of "filter, don't cap" here, and the first where the filter
  itself was the trap; it also bit in `[doputc]`'s `if (n++<4)` in the same sprint.
- **Open (shared with PO-7), now one backlog line — PO-13:** selecting an option *inside* an
  in-flight menu is still unverified headlessly.
- **Gates:** parity 5/5 · sweep 9 OPEN/0 CRASH · map click · map drag · sysbox exit · help click ·
  overlay text · stress 20/20 · ASan 0. New reference `port/ref/native/map_window.png`.

**Retro.** Three PO defects, three unrelated mechanisms, one symptom: *the text is not there*. Worth
remembering the next time a report groups defects by what the player saw.

### 🏃 Sprint 104 — "The menu was always opening" (PO-7) — ✅ CLOSED 2026-08-15 (goal MET)

**Sprint Review (PO pre-approved ceremony, logged 2026-08-15):** detail in
`port/scrum/sprint-104.md`.

- **⭐ R was never broken.** The whole chain passes and always did — scancode 0x13 → action index
  500 → `KeyPress3d(RADIOCOMMS) fired` → `SetToRadioScreen` (deadtime 0, DPlay off) →
  `SetToUIScreen accepted` → promoted — and the screen lives its full five seconds before closing
  itself. **Captured:** "1.Group Info / 2.Precombat / 3.Combat / 4.Postcombat / 5.Tower /
  6.FAC/Bomb" with "0.Exit" in red (`port/ref/native/radio_menu.png`). What the PO reported is
  reproduced exactly by `MA_NO_ALPHATEXT=1`: an opaque grey box of white blocks, five seconds, gone.
  **PO-7 was PO-4 wearing a different hat**, and S102 had already fixed it.
- **⭐ The reusable output: `MA_UISCR_SHOT=N` — arm the capture from the drive.** Four attempts to
  photograph this menu missed it, because `MA_DUMP_BACK=N` aims at a frame number and these screens
  open on a keypress and close after five seconds — and **the pump counter that delivers the key
  runs at a completely different rate from the Blt counter that numbers frames** (a tap at pump 500
  and a dump at Blt 560 were seconds apart; the log line order proved it). Now a promoted UI screen
  arms `ma_dump_arm`, and the N-th Blt after it writes the frame. Directly reusable for PO-9 and
  PO-6.
- **Ruled out by measurement, not by reading:** the menu's five-second timer is honest
  (`budget=500 -= FrameTime()=2` → ~250 frames). A wrong-units `FrameTime()` would have shut the
  menu in a frame or two and looked exactly like "the key does nothing".
- **`KeyPress3d` is a test-and-CLEAR**, so the new `MA_KP()` trace wraps the existing call instead
  of calling it again — a second call would consume the hit bit and break the feature *because it
  was being watched*.
- **Left open honestly:** a number-key selection inside the menu is not yet verified headlessly.
- **Gates:** parity 5/5 · sweep 9 OPEN/0 CRASH · map click · map drag · sysbox exit · help click ·
  **new overlay-text gate PASS** (radio: 848 edges vs 351 blocks vs 207 blank) · stress 20/20 ·
  ASan 0.

**Retro.** Two PO defects, one cause. The sprint that finally photographed the menu spent most of
its time failing to photograph it — and the fix for that (arm the capture from the event) is worth
more than the finding.

### 🏃 Sprint 103 — "The startup step nobody ran" (PO-8) — ✅ CLOSED 2026-08-15 (goal MET) — ⭐ preferences had never once been loaded

**Sprint Review (PO pre-approved ceremony, logged 2026-08-15):** detail in
`port/scrum/sprint-103.md`.

- **⭐ PO-8 CLOSED — and the cause was far larger than the info line.** `SaveData::InitPreferences`
  is both the game's default-setting code **and the only reader of `settings.mig` in the tree**.
  Its two call sites are the demo build and the top of the intro-Smacker route; the port launches
  the title directly (Smacker stubbed), so it **never ran**. Preferences were written on every exit
  and never once read back, and the game flew on a never-initialised `Save_Data` —
  `infoLineCount`'s default of 1 among them. Info line now reads
  **"Speed: 438Kts Mach: 0.73 Alt.: 16724ft Hdg: 279 Thrust: 0"**, and settings survive a restart
  (`loaded settings.mig: ok=1 infoLine=1 keysens=2 vol(125,125,64)`) for the first time in this
  port's life.
- **Three local patches, one cause.** `SetUnits()` for the zeroed unit factors, `GD_HUDINSTACTIVE`
  forced per flight, and MILES.CPP restoring all-zero sound volumes — each treating a symptom, none
  asking who was supposed to set the value. The MILES comment even blamed "a stale settings.mig
  being loaded back", which **could not have been happening**. All three retired; the last two were
  actively wrong once preferences load, because they override a real player choice.
- **A wrong prediction, kept in the record:** the sprint opened expecting `InitPreferences` to make
  `infoLineCount=1`, and it still printed 0 — the function *ends* by loading `settings.mig` over its
  own defaults, and this install's file said 0. What resolved it was a trace of the LOADED values,
  not of the call. *Print what was loaded, not that you loaded.*
- **Two follow-on fixes the finding exposed:** the settings stream's `__DATE__` build-date guard
  (which voids the file on any later-day build — the campaign stream had already been given exactly
  this treatment, the settings stream never was, because nobody could see it fire), and a one-time
  migration for installs whose `settings.mig` was written from a never-defaulted `Save_Data`. The
  migration signature was **measured** from this install's own file, not guessed — a guess of
  "everything zero" would have missed it, since MILES had patched the volumes before the save.
- **PO-6 localised:** `M` opens the in-flight map window and **every text element is missing** — not
  the glyph path, not key delivery, but the map screen's own drawing. **PO-7 narrowed:** the R tap
  is delivered and correctly bound, and the same path opens the map with M, so the fault is in
  `KeyPress3d(RADIOCOMMS)`/`SetToRadioScreen`.
- **Gates:** parity **5/5 after a justified rebaseline** (only combo VALUES moved, and
  `Gamma Correction` Minimum→**Medium** matches gold shot #2) · sweep 9 OPEN/0 CRASH · map click ·
  map drag · sysbox exit · help click · stress 20/20 · ASan 0. `parity_2d.sh` now pins
  `settings.mig` around every capture: the prefs screens were *accidentally* state-independent
  while preferences never loaded, and are not any more.

**Retro.** When a subsystem needs its third local workaround, stop patching symptoms and find who
was supposed to initialise it.

### 🏃 Sprint 102 — "Letters, not bars" (PO-5/PO-4) — ✅ CLOSED 2026-08-14 (goal MET) — ⭐ overlay text is legible

**Sprint Review (PO pre-approved ceremony, logged 2026-08-14):** detail in
`port/scrum/sprint-102.md`.

- **⭐ PO-4/PO-5 CLOSED.** `InitFont` puts the glyph SHAPE in the font map's `alpha` plane and
  fills `body` with a constant 31 — and the span fillers `DoPutC` dispatches to (`IMAPPED`,
  `IMAPPED_M`) sample `body` and **never `alpha`**. Every glyph was therefore a filled 11×14 cell:
  S101's "solid bars". Verified against the shipped `GRAFPASM.ASM`, not just the port's nasm.
- **It was never a bug on Windows.** `direct_3d::PutC` textures the quad with the alpha map and
  modulates by `fontColour`; the shipped game draws text through the **hardware** path. The port
  forces `fSoftware=true` because `DoHardPoly` is stubbed. **Sixth PO defect in a row caused by a
  stub rerouting work into a path the game never exercised — not one was a bug in the game.**
- **Fix:** `ma_putc_alpha_blit` (`Polygon.cpp`) renders the glyph as the hardware does — coverage
  from `alpha`, colour from `fontColour`'s palette entry, blended into the rasteriser's own target.
  The text quad is axis-aligned and 1:1, so it is an exact blit, not an approximation.
- **Three-arm A/B with the prediction stated first:** fix → **"1. Pincer attack. / 2. Multi-wave
  attack. / 3. Select target / 4. Continue"** (610 bright px); `MA_NO_ALPHATEXT=1` → four solid
  bars (3711); `MA_NO_GLYPHS=1` → nothing (303). *An earlier attempt dumped at Blt 250 and got
  three BYTE-IDENTICAL captures — the page-0 font map is not touched that early. Identical
  captures in an A/B mean the recipe missed the feature, not that the change does nothing.*
- **S101's named suspect killed in two minutes** by dumping the atlas cell (`MA_GLYPH_DUMP=S`):
  the 'S' is a clean, graded letter. S101's own closing note said to look rather than reason; doing
  that first was the whole sprint.
- **The PO's gold VIDEOS are now tooling:** `port/tools/gold_video.sh` (`list`/`frame`/`crop`/
  `sheet`/`geom`). Geometry measured, not assumed — the two recordings differ (1280×1024 vs
  1200×1080). PO-6…PO-11 are behaviours; only a video can adjudicate them.
- **Also logged:** the font map's mask/no-mask decision (`*body == ARTWORKMASK`) is decided by a
  **width-table byte** that happens to be 253 — an accident of a font metric, harmless now.
- **Gates:** parity 5/5 · sweep 9 OPEN/0 CRASH · map click · map drag · sysbox exit · help click ·
  stress 20/20 · ASan 0.

**Retro.** The cheapest instrument in the sprint (an ASCII dump of one glyph cell) retired the
previous sprint's headline hypothesis before any code was written. Look at the artifact before
reasoning about the code that produced it.


### 🏃 Sprint 101 — "Show the text" (PO-5 cont.) — ⚠️ CLOSED PARTIAL 2026-08-09 — atlas text reaches the screen, as blocks not letters

**Sprint Review (PO pre-approved ceremony, logged 2026-08-09):** detail in
`port/scrum/sprint-101.md`.

- **Two false positives, both caught by asking "would this look the same if the fix were absent?"**
  (1) **"RUDDER TRIM"** renders legibly in flight and looked like PO-5 closing — it renders
  *identically* with `MA_NO_GLYPHS=1`, so it comes from a different text path that was already
  working. (2) That prompted the right question — *does the atlas path run at all?* — and a counter
  in `PutC3` shows it does, drawing `S p e e d` at (0,471) every frame. **The failure was never
  "the text is not drawn".** `MA_NO_GLYPHS` has now earned its keep twice in two sprints.
- **Where it stands:** with glyphs on, the band at y≈471 fills with marks that are **absent with
  glyphs off** — S100's rasteriser reaches the screen — but they are **solid bars, not letters**.
- **PO-5 remains open, and has moved:** from "the font atlas is empty" (fixed, S100) to "the
  atlas-to-screen packing is wrong". Named suspect: `MakeChar`'s packing masks `0x40404040` to
  separate *saturated* texels, so a conversion landing too many values on exactly 64 makes every
  pixel fully opaque — precisely the symptom. Next attempt should dump one glyph's 0..64 buffer
  beside the resulting atlas cell rather than reason about the packing from source.
- **Gates:** no gate-visible change (one `getenv`-gated counter); S100's results stand.

**Retro.** Two candidate proofs rejected in one sprint, neither expensive to check, and accepting
either would have closed PO-5 wrongly.

### 🏃 Sprint 100 — "There were never any glyphs" (PO-5) — ⚠️ CLOSED PARTIAL 2026-08-09 — ⭐ root cause fixed at source; the stub that caused it had said so since bring-up

**Sprint Review (PO pre-approved ceremony, logged 2026-08-09):** detail in
`port/scrum/sprint-100.md`.

- **⭐ The cause of PO-5, after five sprints of investigation walked past it.** `COverlay` does not
  load its font as artwork — it **builds a glyph atlas at runtime** via
  `GetGlyphOutline(GGO_GRAY8_BITMAP)`. The compat layer stubbed that to `return 0`, **with a comment
  saying "blank text now"**. Every glyph's alpha stayed zero, so overlay text was laid out,
  positioned and composited perfectly and drawn **completely transparent**. *A stub whose comment
  describes a user-visible consequence is a bug report nobody filed.*
- **Fixed** against the stb_truetype faces `ma_gdi` already loads. The contract details that matter
  came from what `MakeChar` consumes, not from the docs: levels are **0..64 not 0..255**, rows are
  DWORD-padded, `gmptGlyphOrigin.y` is height *above* the baseline, and the engine's `MAT2` is a
  **non-square** scale.
- **This retires S94's conclusion.** The palette-slot-252 analysis was a true observation about the
  wrong layer: writing white into 252 changed nothing because **there were no texels** to colour.
- **⚠ Two invalid instruments before one that works — and the second would have concluded the
  sprint wrongly.** (1) A screenshot showing "10 20 30 40" — that is **cockpit art**, present with
  and without the fix. (2) A whole-frame A/B: 14187 px differ — worthless, because **two IDENTICAL
  flight runs differ by ~2700 px**. *Establish that a comparison is repeatable before concluding
  from it; running the same config twice is the cheapest experiment in this project.* (3) What
  works: count the ink in the atlas — **2666 of 16384 non-zero alpha bytes with the fix, 0 with
  `MA_NO_GLYPHS=1`**. A switch that removes exactly the feature is a claim a wrong fix cannot
  satisfy — S99's rule applied on the first attempt this time.
- **PO-5 stays OPEN, honestly:** no capture yet shows overlay text on screen. The glyph *pipeline*
  was the port defect and it is fixed; what remains is scenario state — `DrawInfoBar` returns early
  on `infoLineCount==0` (the pinned save has 0) and the padlock readout needs an enemy selected,
  the same wall B7/C4c/C4d are at.
- **Gates:** parity 5/5 (this change touches the shared compat GDI, so it mattered) · sweep
  9 OPEN/0 CRASH · map click · map drag · sysbox exit · help click · stress 20/20 · ASan 0.

**Retro.** Six sprints into the play-test defects, every single one has been a chain or a stub the
port left incomplete — never a bug in the game. And the sprint's sharpest moment was rejecting its
own evidence twice: **a plausible instrument that has not been shown to be repeatable is not
evidence.**

### 🏃 Sprint 99 — "Getting the words out" (PO-4 cont.) — ⚠️ CLOSED PARTIAL 2026-08-09 — ⭐ the oracle I designed as the safeguard reported 0.484 PLAUSIBLE about gibberish

**Sprint Review (PO pre-approved ceremony, logged 2026-08-09):** detail in
`port/scrum/sprint-99.md`.

- **PO-4 is still open and the "?" still shows nothing.** Four of five decode stages for the shipped
  `MIG.HLP` documentation are solved and independently evidenced; the fifth is not, and **nothing
  was wired into the game**. `port/tools/hlp_extract.py` states its own status in its header and its
  `--verify` prints *WRONG* today.
- **Solved, each with its own evidence:** container/internal-file B+ tree (11 files); LZ77
  (`|PhrImage` → clean alphabetical word list); `|PhrIndex` bit reader (**732 phrases with exact
  boundaries**); `|TOPIC` link chain (43 headers vs 44 titles). Two real finds: the bit reader is
  **LSB-first over 32-bit DWORDs** (the natural guess is *almost* right — only the phrase
  **boundaries** land wrong, which is the signature of a nearly-right bit order), and topic links
  are addressed by **`TopicPos` in a logical space of fixed 0x4000 blocks**, so concatenating
  decompressed blocks desynchronises at the first boundary — presenting as *"only 6 of 44 topics
  exist"*, i.e. as missing data rather than an addressing bug.
- **⭐ The lesson, and it generalises over the last four sprints.** The sprint was set up with a
  deliberate oracle — *"the output must read as English"* — implemented as the fraction of common
  English words. Decoder fixes drove it **0.016 → 0.140 → 0.282 → 0.484 "PLAUSIBLE"**. The 0.484
  text: *"airfield , different a : Summary automatically a KHowever icon have four a make,
  Patrolcampaign for a OtherNose"*. **A wrong phrase decoder emits real dictionary words in the
  wrong order — exactly what the metric rewards.** The failure mode did not evade the metric, it
  *maximised* it. Replaced with a reference the decoder does not feed: `|TTLBTREE` holds each
  topic's real title and correct text contains its own title — **0/39 today, correctly**.
  **Design the oracle by asking what the FAILURE MODE would score.** Fifth time this port has been
  fooled by a blind check (§8-MA83, S64→S65, §8-MA93, §8-MA96, here) — and the first where the check
  was the safeguard I had designed for exactly this.
- **Unsolved, precisely:** the Hall opcode table for the text stream, kept behind `--hall-guess` as
  something concrete for the next attempt to disprove. The known-plaintext route via the topic
  header is a dead end — a `TOPICHEADER`'s `data2` is structured, not the title in phrase form.
- **Gates:** no game code changed this sprint, so S98's committed results stand.

**Retro.** A sprint that did not deliver its feature and is worth more than one that did. Shipping a
decoder that produces confident nonsense would have been worse than shipping nothing — **a "?" that
shows wrong documentation is harder to notice than a "?" that shows none.**

### 🏃 Sprint 98 — "The '?' reaches the help system" (PO-4) — ⚠️ CLOSED PARTIAL 2026-08-09 — routing fixed in four places; no viewer yet

**Sprint Review (PO pre-approved ceremony, logged 2026-08-09):** detail in
`port/scrum/sprint-98.md`.

- **⚠ Read this one correctly: the click now reaches the help system, and the player still sees
  nothing**, because the port has no WinHelp viewer (`CWinApp::WinHelp` is still a stub). PO-4 is
  **half closed**, and `port/help_click.sh` prints that boundary in its own output so a green
  result is never mistaken for "help works".
- **Four independent breakages in one chain**, each invisible until the previous was fixed:
  (1) the title-bar router returned early for the help band; (2) **`WM_COMMANDHELP` was not defined
  at all** — §8-MA83 in its purest form: while `ON_MESSAGE` expanded to nothing it never evaluated
  its argument, so the symbol had never been *required* to exist; (3) `SendMessage` dispatched only
  `WM_USER+` (`>=0x400`) and this message is `0x0365`, below it; (4) `CWnd::OnCommandHelp` was a
  **non-virtual** stub returning 0 and `CDialog` overrode it back to 0, so `CMainFrame`'s override —
  the thing that opens help — was unreachable *and* undispatchable through a `CWnd*`.
- **What found the last one: the chain's return value.** The send returned **0** after fixes 1–3 and
  **1** after fix 4. *"Delivered" and "handled" are different claims, and a chain of stubs returns a
  plausible 0 at every step* — log what the handler returned, not that you sent it.
- **New recipe form `#ID@Class:?`** = "the help glyph of this title bar", resolved by asking the
  control's own hit-test where its help band is (S95's rule again). Trap hit while adding it:
  **`sscanf` returns the number of ASSIGNMENTS, not literals**, so a format ending in a literal
  `:?` matched entries that had no `:?` — the branch silently stole `#2064@CMainToolbar`.
- **Help content scoped with facts, not a guess.** New `port/tools/hlp_probe.py` reads
  `English/TEXT/MIG.HLP`: WinHelp 4, 11 internal files, **44 topics** (Map Screen, Dossier, Main
  Toolbar, Weather, Bases, Squadron Information, Flight Details, Target List…) and **35 `|CTXOMAP`
  context→topic mappings** — documentation for exactly the screens the PO was pressing "?" on.
  Remaining: `|TOPIC` is LZ77 + Hall phrase compressed, then an in-game viewer. **Logged as its own
  item; half-building it here would have produced something that neither renders help nor can be
  trusted.**
- **Gates:** parity 5/5 · sweep 9 OPEN/0 CRASH · map click PASS · map drag PASS · sysbox exit PASS ·
  **new help click PASS (routing only)** · stress 20/20 · ASan 0.

**Retro.** Four sprints, four PO defects addressed, and the pattern is now unmistakable: every one
was a chain the port had left incomplete, not a bug in the game. The sprint's most reusable output
is the habit of *reading the return value* of a route rather than trusting that sending it was
enough.

### 🏃 Sprint 97 — "A way out" (PO-1) — ✅ CLOSED 2026-08-09 (goal MET) — the exit widgets are visible, correct, and they work

**Sprint Review (PO pre-approved ceremony, logged 2026-08-09):** detail in
`port/scrum/sprint-97.md`.

- **PO-1 CLOSED.** `CSystemBox` is now drawn **on by default** (`MA_NO_SYSBOX` reverts) with correct
  art, and clicking the **X** returns the player to the title screen and its main menu. S94 had left
  it opt-in because the buttons were blank and the draw corrupted the map date; both fixed.
- **⚠ Art named after a control is not necessarily the art *for* that control.** `F_GRAFIX.G` has
  `FIL_ICON_THUMBNAIL`/`FIL_ICON_ZOOMIN`/`FIL_ICON_CLOSE1`, named after the three ids — **two of the
  three are the wrong pictures** (they render as unrelated map glyphs). The gold shot settles what
  the buttons look like; a name in a header does not. New probe hook `MA_BTN_ART="id=0xNNNN,…"` made
  it a two-minute comparison instead of a rebuild per candidate, and the result cross-checks against
  *behaviour* (`IDC_ZOOMIN` drives `OnGoBig`/`OnGoNormal`, so `FIL_ICON_SCREENSIZE` is right).
- **A widget must not change the state of the screen it draws on.** S94's parity failure was the map
  **date readout**, top left, nowhere near the box: the box draw left a different GDI font selected
  and the date inherited it. Font saved/restored around the draw — and the check that proves it is
  that the only differing pixels are **x 724–795, y 4–51**, exactly the box's rect. *"Parity still
  passes" is weaker than "the diff is exactly the shape of what I added".*
- **⚠ Giving the buttons art revealed a bug that had always been there.** A **second** copy of the
  cluster appeared top-left and **outlived the campaign, sitting on the title screen**:
  `ma_ole_draw_all` had always drawn those controls at their raw template origin as well — with no
  art it painted nothing, so nobody saw it. The map toolbars escape the global pass only because
  their parent dialog is created *hidden*, which is an accident; the port now says it explicitly
  (`ma_ole_set_parent_scoped`).
  **No gate caught this** — the parity `title` capture is a clean boot that never enters the
  campaign, so it stayed byte-identical while the title screen was visibly wrong *after an exit*.
  It was found by looking at the screenshot of the thing just built. **Transition states (screen A
  arrived at from screen B) are a systematic hole in a per-screen parity suite** — logged as a
  backlog item, not fixed here.
- **Gates:** parity 5/5 (campaign_map re-baselined to include the widgets, other four unchanged) ·
  sweep 9 OPEN/0 CRASH · map click PASS · map drag PASS · **new sysbox exit PASS** · stress 20/20 ·
  ASan 0.

**Retro.** Three sprints, three PO defects closed, and each one exposed something older and larger
than the report: a click consumer that was never in the router (S95), a screen that had never been
the right size (S96), and a ghost that only became visible when the thing in front of it got art
(S97). **Play-test findings keep out-performing autonomous investigation — and the standing lesson
of this run is that the gates are strongest where they were last burned and blind everywhere else.**

### 🏃 Sprint 96 — "The screen was the wrong size" (PO-2) — ✅ CLOSED 2026-08-09 (goal MET) — ⭐ the campaign map had been 221 px too wide since it first rendered

**Sprint Review (PO pre-approved ceremony, logged 2026-08-09):** detail in
`port/scrum/sprint-96.md`.

- **PO-2 CLOSED — root cause in the compat GDI, not the map.** `SetDIBits`/`StretchDIBits` grew the
  canvas to fit whatever was drawn; Windows *clips* a DC blit to the client area. The map is tiled,
  so the moment it scrolls, tiles hang off the edges — and each one enlarged the whole screen, every
  frame of the drag. Growth is now only accepted from a blit anchored at or above the origin
  (`MA_CANVAS_GROW_ANY=1` reverts).
- **⭐ The finding that outlives the defect: this was also happening at rest.** On a plain boot with
  no input, the front end establishes an **800×600** screen and then **30 growth events from map
  tiles inflate it to 1021×644**. **The campaign map has been 221 px wider and 44 px taller than the
  game's actual screen for as long as it has rendered** — every other screen in the port is 800×600
  and nobody asked why the map was different. It is now 800×600 and fills it correctly. This also
  explains why anything positioned from the right edge (PO-1's system box, `_cw - _bw - 4`) sat
  against an edge that was not where the screen ended.
- **`campaign_map` parity reference re-baselined, deliberately.** It encoded the bug. The other four
  screens stayed **byte-identical**, which is the evidence that clipping did not disturb the front
  end. *Standing lesson: a native-vs-native reference locks in whatever was true the day it was
  captured, bugs included — the cheap check here was "every screen should be the same size, and that
  size should be the display mode".*
- **S95 regression caught and fixed in the same area:** a drag ends in a release, which raised the
  same click edge as a tap, so **every pan finished by opening a dossier**. Press and release must
  now land together (≤4 px), as on Windows.
- **⚠ The test lied first.** The drag gate's first version reported a *perfect lossless round trip*
  while the drag did **nothing at all**: the hook pushes real SDL events on purpose, and **the event
  queue was never drained without a window** — S93 moved the synthetic hooks above
  `if (!g_win) return;` and left the guard in front of `SDL_PollEvent`. **The same bug, in its other
  half, one sprint later.** `0 px differ` and `nothing happened` are the same reading. The gate now
  asserts three things and the first exists to give the second meaning: one-way drag **≠** baseline
  (288562 px), round trip **==** baseline (0 px), release **suppressed** as a click.
- **Gates:** parity 5/5 · sweep 9 OPEN/0 CRASH · map click PASS · **new map drag PASS** ·
  stress 20/20 · ASan 0.

**Retro.** Four times now this port has been fooled by silence (§8-MA83, S64→S65, §8-MA93, and
today). The countermeasure is cheap and should be standing practice: **every "no difference"
assertion needs a companion assertion that the action happened.** Also: when you move code past a
guard, check what else is still behind it.

### 🏃 Sprint 95 — "The map was never told" (PO-3) — ✅ CLOSED 2026-08-09 (goal MET) — recon dossier opens from a map icon

**Sprint Review (PO pre-approved ceremony, logged 2026-08-09):** detail in
`port/scrum/sprint-95.md`.

- **PO-3 CLOSED, and no gameplay code was written.** The chain was already complete and correct:
  `CMapDlg::OnLButtonDown`→`FindMapItem`→`m_buttonid`, then `OnLButtonUp`→`OnClickItem`→
  `CMainToolbar::OpenDossier`→`CTargetDossier::MakeSheet` — the PO's "recon dialog". `CMIGView::
  OnLButtonDown` is empty; on Windows the map dialog got its clicks from the message queue, which
  this port does not have. The map idle offered each click to the OOB dialogs, the system box and
  the two toolbars, and **dropped it if they all declined**. `m_mapdlg` was the last unrouted click
  consumer in the game. Now it gets the fall-through (`MA_NO_MAP_ITEM_CLICK` reverts).
- **Down+Up in one call is a design choice, not a shortcut.** It keeps `m_bDragging` FALSE, so the
  click takes the `OnClickItem` path and **never enters `CMapDlg::OnMouseMove`, whose `GetDC()`
  result is dereferenced unchecked** in this port (the S82 rule: the genuine handler you drive may
  itself contain an unported call). That `OnMouseMove` is where **PO-2** will be fought.
- **Verified by capture:** the DOSSIER sheet renders with live campaign data — *Yonchon Supply
  Dispersal*, MSR Central, Threat AAA Medium / MiG 15 Low, Repairs Operational, Last Sortie
  (never) — over the recon photo, Details/Damage/Notes tabs, and the clicked icon redraws as
  selected (so `RedrawIcon`/`ConvertPtrUID`/`ScreenXY` all survive the port unaided).
- **⚠ A coordinate is not a test.** The first click, at a point read off a scan, hit **nothing**
  (`hit id=0`) — same binary, same pinned save, but the canvas had grown **800×600 → 1021×644**
  between scan and click, moving every icon ~108 px. That reads exactly like "the routing does not
  work". So the new gate `port/map_icon_click.sh` **names no coordinate**: it asks the map's own
  hit-test where the icons are at the frame it is about to click, clicks the first one clear of the
  toolbars, and PASSes on *item hit + dialog painted + survived*. It pins `campaign_pristine.sav`
  as `oob_sweep.sh` learned to in S94. **Third distinct instance of the same project failure mode:**
  a check whose result depends on state the check does not control (S81 save, S94 save, S95
  coordinate).
- **Gates:** parity 5/5 · sweep 9 OPEN/0 CRASH · stress 20/20 · ASan 0 · **new map-click gate PASS**.

**Retro.** The sprint's real work was ten lines; the rest was making the verification honest. Three
of the five PO defects are now root-caused, one fixed. PO-2 is next and it already has a named
suspect — which is what a properly-written previous sprint buys you.

### 🏃 Sprint 94 — "What the PO found" — ⚠️ CLOSED PARTIAL 2026-08-09 — five play-test defects triaged, two root-caused

**Sprint Review (PO pre-approved ceremony, logged 2026-08-09):** detail in
`port/scrum/sprint-94.md`. **The PO play-tested the port under gdb** — the first human play-test of
this session — and reported five defects. The run itself was clean (~12 min, exited normally, no
fault). All five are now backlog items.

- **PO-1 exit/resize widgets — root-caused, half-fixed.** The upper-right cluster is `CSystemBox`:
  `IDC_FILES`→`OnBye()` (**the exit**), `IDC_ZOOMIN`→resize, `IDC_THUMBNAIL`→minimise. Created by
  `CMainFrame`, enabled/disabled by `RDialog` — **and never drawn**. Now drawn + click-routed at the
  canvas top right, positioned from its **own control extent** (`ma_ole_dialog_extent`) rather than
  a hardcoded width. **Still blank**: those ids have no icon-art entry. Positioned and clickable but
  invisible is not a fix.
- **PO-5 overlay text — four-step chain, still open.** (a) `DrawInfoBar` returns early on
  `infoLineCount==0`, and the PO's save has 0 — that part is a *setting*. (b) Forcing it on shows a
  **real defect**: the layer runs, the font map resolves, glyphs blit, nothing appears. (c) Glyphs
  draw through **palette slot 252**, and since **`WHITE==252`** the engine's
  `SetPaletteEntry(252, GetPaletteEntry(fontColour))` is a **self-copy no-op**; slot 252 holds
  `0x0000` and the blit is masked, where 0 = transparent — **text rendered, drawn transparent**
  (S73's cockpit-black family). (d) Writing real white into 252 does **not** fix it, so the texels
  don't index 252 either; that change was **not shipped** (shared render path, no proven benefit).
- **PO-2 / PO-3 / PO-4** logged; PO-4's cause already known (S82 returns early for the help band,
  and `WM_COMMANDHELP` is one of the six routes the dispatcher never implemented).
- **⚠ Cost of the sprint, and a correction to my own note:** `SRC/GRAPHICS/POLYGON.CPP` (149 KB) and
  `Polygon.cpp` (159 KB) are **genuinely different files**, and the unity compiles the mixed-case
  one. The first full read/analysis/instrumentation of `DoPutC` went into a file that is **never
  built**; `ninja: no work to do` was the only clue. **S83 probed RBUTTON, found its twins identical,
  and I generalised — wrongly.** Memory corrected: the property is per-file, take the case from the
  unity's `#include`.

**Retro.** Twelve minutes of human play produced more actionable defects than the previous four
autonomous sprints combined. Both stalled stories (B7, C4) were blocked on things a player would
never care about, while five real ones sat undiscovered. **Put the build in front of someone sooner,
and more often.**

### 🏃 Sprint 93 — "Make the key arrive" — ✅ CLOSED 2026-08-09 (goal MET) — ⭐ headless key injection was dead in the mode it exists for

**Sprint Review (PO pre-approved ceremony, logged 2026-08-09):** detail in
`port/scrum/sprint-93.md`.

- **One-line ordering bug.** `pump_events()` opened with `if (!g_win) return;` — and under
  `SDL_VIDEODRIVER=dummy` `SDL_CreateWindow` **fails** (the boot log has said so all along), so
  `g_win` is NULL and the function bailed *before* the synthetic-input hooks. **`BOB_KEYSEQ` and
  `BOB_AUTOFLY` were dead in headless mode — precisely the mode they exist to serve.** Fixed: the
  hooks run before the window check; only real SDL polling needs a window.
- **Proven end to end**, which the old code could not show at all:
  `[keyseq] tap dik=0x3b at kidle=250` → `[key] DOWN scancode=0x3b -> action index=132`.
- **⚠ CORRECTION TO S91.** Its B7 "third negative" — a 60-tap dive that produced no change, from
  which it concluded *"the problem is what is near the aircraft, not how it is flown"* — rested on
  a dive that **never happened**; the taps were discarded by this bug. **That conclusion is
  withdrawn and B7's scenario question is re-opened.** S92's failed padlock verification has the
  same cause.
- **The tell was there and I misread it:** no `[keyseq]` trace in either run. I read the absence as
  "the tap had no effect" rather than "the tap never fired" — the same *"no output means the code
  never runs"* trap booked at §8-MA83 and S64→S65. **A silent no-op and an absent one look
  identical in a log and mean opposite things.**
- **Still open, honestly:** the padlock did not engage even with the key arriving —
  `CheckPadlock(currentenemyitem)` needs an enemy actually selected. C4c/C4d stay unverified, but
  the blocker has moved from "harness broken" to "no enemy in view", which is the same wall B7 is
  at — now clearly one problem, not two.
- **Gates:** parity 5/5; stress 20/20; ASan 0 reports.

**Retro.** Two sprints of in-flight conclusions rested on tests that never ran. The cheap check that
would have caught it immediately: **before believing a negative result, confirm the stimulus was
delivered.** A trace line proving the input happened is worth more than the trace of what it was
supposed to cause.

### 🏃 Sprint 92 — "Read the bogey" (C4) — ⚠️ CLOSED PARTIAL 2026-08-09 — C4d written but NOT verified; backlog corrected

**Sprint Review (PO pre-approved ceremony, logged 2026-08-09):** detail in
`port/scrum/sprint-92.md`.

- **Backlog correction: C4a is already implemented.** The sprint opened intending it; an `MA_LINUX`
  block in `OVERLAY.CPP` already sizes the padlock box from a projected world half-extent through
  the same perspective divisor as the position, so it grows as the bogey closes. The C4 row still
  listed it outstanding — fixed.
- **C4d written:** own speed via `DrawTopText`'s exact formula (so the two readouts cannot
  disagree), and closure as d(range)/dt timed by the engine's own `RealFrameTime()` rather than an
  assumed frame rate, with samples dropped on a target switch so a padlock change cannot print a
  spike. **Bogey speed deliberately omitted** — no per-target speed field was reachable from that
  scope, and inventing one is worse than leaving it out.
- **⚠ NOT VERIFIED, and recorded as such.** Added `MA_PADLOCK_TELEM`/`MA_PADLOCK_BOX` env defaults
  (both toggles are modifier-driven, and a synthesised DIK tap carries no SDL modifier state, so
  neither is reachable from `BOB_KEYSEQ`). The verification flight produced a clean cockpit capture
  and **no padlock box or telemetry**: `trackeditem2` was never set, and no `[keyseq]` trace fired
  at all — the `ENEMYVIEW` tap never took effect. The code is inert unless a padlock target exists
  and telemetry is on, so risk is contained, but **no capture shows it working**.
- **Gates:** parity 5/5; stress 20/20; ASan 0 reports.

**Retro.** The useful output is a *named blocker* rather than a feature: **`BOB_KEYSEQ` taps are not
reaching the view-selection path.** That now blocks C4c and C4d exactly as scenario blocked B7 — so
the next sprint should fix the harness (why the tap does not arrive), not write more telemetry that
cannot be shown. Also worth noting the sprint opened on a stale backlog row; **re-reading the code
before planning would have caught C4a in a minute.**

### 🏃 Sprint 91 — "Send the findings" — ✅ CLOSED 2026-08-09 — cross-port debt cleared; B7 gets a fourth honest negative

**Sprint Review (PO pre-approved ceremony, logged 2026-08-09):** detail in
`port/scrum/sprint-91.md`.

- **Cross-port debt paid** (BoB had nothing since note 34). The actionable item for them:
  **`ON_EVENT_RANGE` was an empty macro**, so every range-registered handler was dead — the way this
  engine wires *grids* of controls (`CBases`' 30 airfield buttons, `CMapFilters`' layer filters).
  One `grep -c` answers it on their side.
- **§8-MA91 frames it as a CLASS, not a bug:** the compat layer's empty map macros each silently
  discard a registration the game source makes, and MA has hit it three times (`ON_MESSAGE`,
  base-class `ON_EVENT`, `ON_EVENT_RANGE`), each found one broken screen at a time. So the section
  carries the audit MA should have done earlier — with **counts**: `ON_EVENT_RANGE` 9 (implement),
  `ON_COMMAND` 29 (**skip** — framework menu ids), `ON_BN_CLICKED` 14 (skip). *Not every dead
  registration deserves reviving; decide from a count in one pass.*
- **B7, third attempt: another negative.** A forced dive (60 `ELEVATOR_FORWARD` taps) with ground
  lock still yields `RequiredRange=100000`, one value. ~~Across four flights and three approaches every lock is ~1.2 M...~~ **⚠ WITHDRAWN by S93:** the
  dive in this attempt never happened — `BOB_KEYSEQ` taps were discarded headlessly by the
  `pump_events` window-guard bug, so this was a test that did not run. B7's scenario question is
  re-opened. **B7 stays open.**
- **Gates:** no source diff this sprint, so the binary is the one S90 gated and the set was not
  re-run for a build that cannot have changed. Notes-sync ✓.

**Retro.** Three sprints on B7 have produced hooks, eliminated two wrong observables and four
negative data points — and no acceptance evidence. Recording that as "still open" rather than
banking the motion is the right call, but the sharper lesson is about *sequencing*: the cross-port
debt paid in one hour here had been sitting for five sprints while B7 absorbed three. **Ship the
finding that helps someone else before chasing the one that only helps the burndown.**

### 🏃 Sprint 90 — "Lock the sight" (B7) — ⚠️ CLOSED PARTIAL 2026-08-09 — locks proven; reticle pins at the range clamp

**Sprint Review (PO pre-approved ceremony, logged 2026-08-09):** detail in
`port/scrum/sprint-90.md`.

- **Locks achieved** where S89 could not: `MA_FORCE_RADARSIGHT=2` (ground lock) + a longer flight →
  **715 locks across 710 distinct ranges** (911 414 … 1 215 900). The ranging chain genuinely runs.
- **⚠ `SphereXScale/YScale` was the WRONG observable, and my first trace hid it.** It printed a
  constant `X=2 Y=3` and I was one step from recording "the reticle does not scale". Those fields
  are **`Float`** (`3DCOM.H:384`) and my trace cast them to `long` — *my* truncation. Re-traced:
  `X=2.4142 Y=3.2190`, still constant — because they are **view/projection** scaling (2.4142 ≈ 1+√2;
  `PARTICLE.CPP` scales sphere radii by them), nothing to do with the gunsight.
- **The real observable:** `RequiredRange = radarRange` (`3DCOM.CPP:20661`) → `CalcGunsightPos` →
  reticle. Traced: `RequiredRange=100000 (radarRange=1215900)`, one value all flight — because
  `RequiredRange` is **clamped to 20 000…100 000** and every lock is at ~1.2 M, ten times the
  ceiling. The gunsight correctly pins at max range.
- **So B7 is not blocked by code** — it is wired end to end and live. What is missing is a target
  *inside gun range* while pointing at it. **B7 stays open**; closing it needs a merge, the C4
  padlock/`BOXTARGET` path, or a close-start scenario. Every observation hook now exists, so it is
  one run's work once the scenario does.
- **Gates:** parity 5/5; stress 20/20; ASan 0 reports. All additions `getenv`-guarded, default-off.

**Retro.** Two of three findings corrected *my own* earlier work rather than the game's — a
truncating trace, and an observable unrelated to the feature. Both were caught only because the
value looked suspiciously constant and got a second look. The residue is worth more than a green
tick: B7's requirement is now falsifiable — *a lock inside 20 000–100 000 units* — instead of "the
gunsight doesn't range".

### 🏃 Sprint 89 — "Range the sight" (B7) — ⚠️ CLOSED PARTIAL 2026-08-09 — B7 characterized: never a port bug

**Sprint Review (PO pre-approved ceremony, logged 2026-08-09):** detail in
`port/scrum/sprint-89.md`.

- **The engine chain is intact and compiled** — `shape::GetRadarItem` (`3DCOM.CPP:19218`) →
  `CalcRadarRange` → `SHAPE.SphereXScale/YScale` (`3DCODE.CPP:1445`); `nm` confirms the symbol in
  the binary. *(Two greps found nothing first: the definition sits behind a high-byte licence
  banner, so `grep -a` is mandatory — the documented `CLAUDE.md` gotcha, re-learned cheaply.)*
- **Measured in flight** (990 target sightings): `polypit=1` but **`radarOn=0` always**, so
  `GetRadarItem` is never called and the reticle cannot range.
- **⭐ And that is the game's design, not a defect.** `radarOn` comes only from the two difficulty
  settings `GD_PERFECT/REALISTICRADARASSISTEDGUNSIGHT` (`3DCODE.CPP:327-334`), chosen by the player
  through the Game tab's *Gunsight Ranging* combo. They are off in a default save. **B7's premise —
  "the gunsight doesn't range" — was never a bug to fix; the feature is opt-in.**
- **Landed:** `MA_FORCE_RADARSIGHT=1|2` opens the gate for headless verification without touching
  the player's save (verified: `radarOn=1`, `GetRadarItem` now exercised), and `MA_TRACE_GUNSIGHT`
  prints the definitive **LOCK** event from inside `GetRadarItem`.
- **NOT achieved, and B7 stays open:** no live lock, so no observed reticle scaling — every traced
  target sat ~144 km out, and a 150 s headless Hot Shot never closed to gun range. Calling B7 done
  on "the path is reachable" would be exactly the inference this project keeps banning; the
  acceptance criterion is *the reticle scales*, and that has not been seen. Next: a closing
  engagement plus a before/after capture.
- **Gates:** parity 5/5; stress 20/20; ASan 0 reports. All new code is `getenv`-guarded and
  default-off, so the flight path is unchanged unless a trace is asked for.

**Retro.** The valuable half of this sprint was refusing to bank it. Three findings (chain compiled,
gate identified, gate opened) make a tidy story that stops one step short of the acceptance
criterion — and the criterion is the whole point. Recording B7 as *characterized, still open* costs
one line and prevents a future reader inheriting "B7 done" with no reticle evidence behind it.

### 🏃 Sprint 88 — "Bind it yourself" — ✅ CLOSED 2026-08-09 (goal MET, 8/8) — ⭐ H2: key bindings are user-editable

**Sprint Review (PO pre-approved ceremony, logged 2026-08-09):** detail in
`port/scrum/sprint-88.md`. Turned from the campaign-UI arc to the ship backlog and delivered **H2**.

- **Scoping found the design constraints, not just the code path.** Bindings are table-driven
  (`Reg3dConv(FIL_3D_KEYBOARD_TABLE)` → `KeyMap3d::mappings[scancode][shift]`), and that function
  **checksums the table it loads**, quitting with *"Key table has changed between loads???"* if two
  loads disagree — so overrides are applied *after* the game's own load, into the live array, where
  the checksum never sees them. And the actions **already have names**: `KEYMAPS.H` has a 177-entry
  `KeyName(index,NAME)` list, extracted to `ma_keyactions.inc` (+ `port/gen_keyactions.py`) so the
  config speaks the game's vocabulary instead of magic numbers.
- **Delivered:** `MA_DUMP_BINDINGS=1` writes all **615** live bindings as `ACTION = 0xSC[, shift]`;
  the same file is read at startup (**576** named bindings applied). `MA_CONTROLS=<path>` relocates
  it, `MA_TRACE_KEY=1` logs each. Verified: edited `RESETVIEW` `0x01`→`0x0F`, re-ran, got
  `[keybind] RESETVIEW -> scancode 0x0F shift 0 (action 130)`.
- **⚠ Caught in the first cut:** the dump wrote shift state as a *following comment*, so reloading
  it would have bound every shifted action at shift 0 and **corrupted the user's controls with the
  tool's own output**. Fixed to one line per binding. *A dump that cannot be fed back is not a
  bindings file* — and it only surfaced because the round-trip was run rather than assumed.
- **Default behaviour unchanged** (no `controls.cfg` → the game's own table), which is what keeps
  the gates meaningful. **H3 docs:** `RUNNING.md` gains a "Rebinding keys" section.
- **Gates:** parity 5/5; sweep 9 OPEN/0 CRASH; stress 20/20. **ASan FAILED first** — the new TU
  went into `CMakeLists.txt` but not `port/rebuild.sh`, which is what the ASan build uses, so it
  failed to link while the primary Ninja build was green. Exactly the divergence a second builder
  exists to catch: **this tree has two build systems and a new file must be added to both.** Fixed;
  re-run 0 reports, 4/4 paths.

**Retro.** Two file-truncation incidents this session came from the same habit — a Python
`open(path,'w')` that truncates *before* an encoding error can abort the write (S84 `SUPPLY.CPP`,
S88 `STUB3D.CPP`). Both were recovered instantly because the work was committed, but the lesson is
cheap and permanent: **for these latin-1 sources, edit with the editor tool, not a rewrite script.**

### 🏃 Sprint 87 — "Pick a row" — ✅ CLOSED 2026-08-09 (goal MET, 8/8) — ⭐ dialog CONTENTS respond; a whole class of dead registrations revived

**Sprint Review (PO pre-approved ceremony, logged 2026-08-09):** detail in
`port/scrum/sprint-87.md`. S86 proved every campaign-map dialog opens; this sprint makes what is
inside them respond.

- **Listbox rows now select.** `ma_ole_toolbar_click` handled buttons and tabs and **skipped
  `CT_LISTBOX`**, so every row in Bases/Squads/D.I.S./Intelligence was inert — the dialogs listed
  real campaign data that could not be selected. The new branch drives the control's genuine
  `OnLButtonDown/Up` (`MaMouse`) so its own logic picks row **and column**, then fires `Select` with
  both (the rule that kept MA clear of BoB's §8u hardcoded-column bug).
- **⭐ Then the bigger find: `ON_EVENT_RANGE` was an EMPTY MACRO** — so every range-registered
  handler in the game was dead. 9 live registrations across 4 classes, including **`CBases`' 30
  airfield buttons** and **`CMapFilters`' map-layer filters**, two dialogs whose entire purpose is
  being clicked. Same family as S83's empty `ON_MESSAGE` and §8z's base-class `ON_EVENT`: *the
  registration exists in the game source and the port silently dropped it.* Implemented — the thunk
  registers per id in the span and `ma_evt_fire` passes the **fired id** as the handler's first
  argument, as MFC does. Verified live: `2420..2478 CBases`, `1015..1046 CCommsPaint`,
  `2350..2397 CSqdnlist`.
- **An upstream bug fell out:** `CSqdnlist`'s eventsink map registers *its own* handlers under
  **`CBases`** (`SQDNLIST.CPP:246-248`) — a copy-paste slip in the shipped source, inert while the
  macro was empty, a compile error once it wasn't. Fixed to the class its own
  `BEGIN_EVENTSINK_MAP` names.
- **Measured effect:** `[tbclick] listbox id=2018 → row=7 col=1 on 7CSupply`, and **700 px change
  bounded to one row band** (y 353-363) — the clicked row goes from list-yellow to selection-white.
- **⚠ The sprint's own test harness had a bug:** S85's `#ID@Class` parser used `%63s`, which runs to
  whitespace — so with a *following* step it swallowed `CMainToolbar;340,#2018@CSupply` as the class
  name and the step silently never matched. Re-running `oob_sweep.sh` (which passed) is what proved
  the code innocent and the recipe guilty. Scanset now excludes `;` and `:`.
- **Gates:** parity 5/5; OOB sweep 9 OPEN/0 CRASH; stress 20/20; ASan 0 reports.

**Retro.** Third time in four sprints that the port's compat layer was found **silently discarding a
registration the game source makes** — `ON_MESSAGE` (S83), base-class `ON_EVENT` (S83/§8z), and now
`ON_EVENT_RANGE`. That is a *pattern*, not three coincidences: worth auditing the remaining empty
macros in `afxwin.h` deliberately rather than discovering them one broken feature at a time.
**Booked as the S88 candidate.**

### 🏃 Sprint 86 — "Open them all" — ✅ CLOSED 2026-08-09 (goal MET, 8/8) — ⭐ every campaign-map dialog verified open, 0 crashes

**Sprint Review (PO pre-approved ceremony, logged 2026-08-09):** detail in
`port/scrum/sprint-86.md`. S82–S85 made the OOB dialogs clickable and fixed the two that crashed;
this sprint answers *do the rest actually work?* as a repeatable command.

- **New gate `port/oob_sweep.sh`** — drives each map-toolbar dialog as a player would (campaign nav
  → click the button addressed as `#ID@CMainToolbar`), reporting OPEN/NONE/CRASH with a capture and
  log each. Stashes and restores the campaign save, like `asan_all.sh` (the S81 rule).
- **Result: 9 OPEN, 0 CRASH** — intelligence, directives, bases, squads, weather, dis, overview,
  missionfolder, playerlog. Spot-checked rather than trusting the counter: **Bases** renders its
  airfield list (Taegu/Taegu West/Taejon/Kunsan/Pohang) with aircraft silhouettes, **D.I.S.** its
  photo + "MISSION 1 BRIEFING". "OPEN" means real content, not an empty panel.
- **The single negative is CORRECT and now documented in the script:** `IDC_MISSIONRESULTS` (2055)
  belongs to `CDebriefToolbar` (`DBRFTLBR.CPP:111/129`), which only exists while `MMC.indebrief` is
  set — so `#2055@CMainToolbar` *should* resolve to nothing. Exactly the case S85's qualifier was
  built for: unqualified, the probe would have found some other 2055 and reported a misleading
  result.
- **Gates:** no source diff this sprint, so the binary is the one S85 gated — **ASan was not re-run
  for a build that cannot have changed**, stated rather than implied; **parity 5/5 byte-identical** and **stress 20/20** re-run as cheap insurance.

**Retro.** Four sprints ago none of these dialogs accepted a click; the whole information layer of
the campaign map is now verified working. The habit worth keeping is the one that made the last five
sprints cheap: **turn each one-off investigation into a command** — `parity_2d.sh` (S80),
`asan_all.sh`'s save stash (S81), `MA_TRACE_MSG` (S83), `MA_TRACE_FILEOPEN` (S84), `#ID@Class`
(S85), `oob_sweep.sh` (S86). Each cost minutes and each paid for itself inside two sprints.

### 🏃 Sprint 85 — "Say which one" — ✅ CLOSED 2026-08-08 (goal MET, 8/8) — ⭐ the Directives dialog opens; recipes can name a control

**Sprint Review (PO pre-approved ceremony, logged 2026-08-08):** detail in
`port/scrum/sprint-85.md`. Fixed the id ambiguity S84 found and used it to finish S84's other half.

- **`f,#ID@Class[:COL]`** in `BOB_CLICKSEQ` — `ma_ole_control_point_p` filters candidates by the
  host's RTTI name (substring, so recipes say `CMainToolbar`). **And ambiguity is now LOUD**: an
  unqualified `#ID` with more than one visible host prints every candidate with its host class and
  rect — printed unconditionally, because the whole failure mode is that nobody was looking.
- **⭐ The Directives dialog opens, fully populated.** `#2074@CMainToolbar` resolves to the main
  toolbar's 48×48 button at (286,52), not the filters toolbar's 24×24 twin at (268,50) the
  unqualified form had been finding. Title bar with `?`/`✓`/`✕`, the Auto Generate / Auto Display /
  Alpha Strikes tickboxes, and the category table (Air Superiority, Choke, Supply, Airfields, Rail,
  Road, Army, Resting) with live values — Choke: 32 strike / 20 targets / 6 missions.
  Artifact `port/ref/native/oob_directives.png`.
- **S84's un-defer is now complete:** *both* dialogs deferred since S52 open on a genuine click, no
  crash, no `SysError`. This also exercises the five `DirControl::AddMission` shadowed-hoist fixes
  through the UI, where S84 could only reach them via ASan's `camp-nextday` mode.
- **Gates:** parity 5/5 byte-identical; stress 20/20; ASan 0 reports, 4/4 paths.
- **Cross-port: MA note 34** — the qualifier form and the general rule: *if a headless drive "does
  nothing", first prove it addressed the control you meant.* BoB's `BOB_AUTOCLICK` has the same
  `#ID` form and their `RESOURCE.H` reuses ids the same way.

**Retro.** This is the third sprint running where the bug was in how we *addressed* or *described*
something rather than in the game: a stale comment naming the wrong class (S83), a sweep regex that
excluded `char` (S84), and now a recipe that silently pointed at a different control (S85). The
countermeasure that keeps working is making the tool complain — an ambiguous id now lists its
candidates instead of quietly picking one.

### 🏃 Sprint 84 — "Open it once" — ✅ CLOSED 2026-08-08 (goal MET, 8/8) — ⭐ the Intelligence dialog opens, populated

**Sprint Review (PO pre-approved ceremony, logged 2026-08-08):** detail in
`port/scrum/sprint-84.md`. The crash chain that had two OOB dialogs deferred since S52 is cleared.

- **The `0x6a78` double-open, traced not guessed.** `MA_TRACE_FILEOPEN` prints a backtrace at the
  fatal branch of `makelink`; it named `ma_oob_paint_tree_rec → … → CRButtonCtrl::OnDraw →
  WM_GETFILE → RDialog::OnGetFile → new fileblock`. **Mechanism:** `OnGetFile` holds its block in a
  **per-dialog** `m_pfileblock`, but the engine allows one open per FileNum — and the map toolbar's
  Authorise button and the dialog's own button share `FIL_ICON_MISSIONRESULTS`, so whichever painted
  second opened a block the first still held. Latent until S82 made OOB dialogs paint every idle.
  **Fix:** `fileman::MA_GetOpenFileData` serves the already-open block's data (sibling of S79's
  `MA_IsFileOpen`); `MA_NO_SHARED_FILEBLOCK=1` reverts.
- **⭐ Four more shadowed hoists — S83's sweep had missed them.** Its regex matched
  `int|long|short|unsigned`; these siblings declare **`char i`**. Type-agnostic re-sweep found 3 more
  in `CSupply` and **5 in `DirControl`** (so the original stale note blaming `CComit_e` had the right
  class for the *other half* of the bug). **The hoisted type must match the original loop variable**:
  `char i = MAX_TARGETS-1` is 299 truncated to **43**, a quirk of the shipped game kept deliberately
  — gold is the oracle, and widening to `int` would silently change how many entries shift.
- **Result: the Intelligence dialog opens fully populated** — five tabs, the sort combo, and a real
  objective table (Chosin, Pungsan Supply Dispersal, Kapsan, Chongjin Marshalling Yd. …).
  Artifact `port/ref/native/oob_intelligence.png`. Defer removed.
- **Bonus: `#ID` recipes were resolving toolbar buttons ~50px off** (hand-computing them failed twice
  this sprint — the S62/S63 trap again). A toolbar control's position is the offset passed at **paint**
  time; the resolver was adding the parent `CRToolBar`'s `m_maX/m_maY`, which are 0. `Hosted` now
  records `drawOx/drawOy` — what paint actually did — and the resolver uses it.
- **⚠ Numeric control ids are AMBIGUOUS in recipes:** `RESOURCE.H` defines **five** symbols as 2074
  (`IDC_DIRECTIVES`, `IDC_AUTHORISE4`, `IDC_FILTER_RED_TROOP`, …). `#2074` resolved to the
  filters-toolbar twin, whose class registers no handler — a no-op, not a crash. `#ID` needs a parent
  qualifier; booked for S85.
- **Gates:** parity 5/5 byte-identical; stress 20/20; ASan 0 reports, 4/4 paths.

**Retro.** Three sprints of this bug were spent on *inherited descriptions* — a stale comment naming
the wrong class, and my own S83 sweep whose regex quietly excluded `char`. Both were settled in
minutes once something printed the actual state (a symbolized backtrace, then a type-agnostic
re-sweep). The recurring shape: **a search that finds nothing is only as trustworthy as its
pattern**, and a pattern is exactly the kind of assumption that deserves the same suspicion as a
hypothesis.

### 🏃 Sprint 83 — "Check every site" — ✅ CLOSED 2026-08-08 (goal MET, 8/8) — ⭐ one shadowed loop variable had two dialogs deferred since S52

**Sprint Review (PO pre-approved ceremony, logged 2026-08-08):** detail in
`port/scrum/sprint-83.md`. Acted on BoB note 19's ask (check every `SendMessage`-result deref
individually) and cleared the two OOB dialogs the click path still defers.

- **The sweep found the class's ROOT, not just its sites.** `RDialog::OnRowanMessage` — the port's
  stand-in for the `ON_MESSAGE` map the compat layer defines away — implements **8 of the 14**
  routes and ends `default: return 0`. Six routes are answered "0" indistinguishably from "the
  handler returned NULL", and **every unguarded deref is downstream of that one `default`**. Each
  is now listed in-code with *why* it is still unrouted, and `MA_TRACE_MSG=1` names any unrouted
  message + receiving class. **Measured:** on the whole campaign/OOB path exactly one fires —
  `WM_GETSTRING`, on 4 classes — confirming S63's fix is still load-bearing.
- **4 derefs hardened** (`CRButtonCtrl::OnLButtonUp`/`::OnMouseMove`, both `CRComboCtrl` sites).
  One of them survives today only because its enclosing `if (… && m_hWnd)` is false in the port —
  **an accidental guard, not an intentional one.**
- **⭐ The deferred-dialog SEGV: root-caused and FIXED in one line.** The recorded cause was wrong
  (it blamed `CComit_e`); a symbolized backtrace named `CSupply::OnInitDialog → SortIntell →
  SortSupplyNodes → AddSupplyMission`. Cause: a **half-applied for-scope hoist** — the port script
  added `int i;` at function scope but left the loop's own `int i`, which shadowed it, so
  `target[i]` after the loop indexed on uninitialised stack. MSVC's for-scope leak had left that
  variable holding `j`. Both dialogs now **build and paint all five tabs**.
- **Swept the tree for that tooling bug:** 15 matches / 7 unique files; only `AddSupplyMission`
  reads the shadowed variable after the loop, i.e. the only harmful one.
- **Still deferred, for a NEW named reason:** Authorise now trips `[SysError] Opened file block
  (6a78) again without closing!` → SayAndQuit — the same double-open family S79 fixed for `0x6a63`.
  **Top of the S84 backlog**; `MA_OOB_NO_DEFER=1` reproduces.
- **Counter-finding worth keeping:** BoB warned that `rbuttonc.cpp`/`RBUTTONC.CPP` are distinct
  stale files. Probed it here — in MA's tree they are the **same** file. But MA's `CLAUDE.md`
  records twins that *have* diverged. The property is per-file and per-tree; a two-second write
  probe settles it and `find -iname` output does not (it lists both spellings of one entry).
- **Gates (all green, under `gl-lock`):** parity **5/5 byte-identical**; **stress 20/20**;
  **ASan 0 reports, 4/4 paths 2/2**.
- **Cross-port: MA note 32 + §8-MA83.**

**Retro.** The sprint's best move was reading the *dispatcher* instead of the call sites: it turned
"audit every `SendMessage` in the tree" into a six-item list plus a trace that says which one
actually fires. Second-best was distrusting two inherited claims — a stale in-code comment naming
the wrong class, and a sibling's warning that did not hold here — both settled by a probe rather
than by argument.

### 🏃 Sprint 82 — "Click the dialogs" — ✅ CLOSED 2026-08-08 (goal MET, 8/8) — ⭐ the campaign-map OOB dialogs are INTERACTIVE

**Sprint Review (PO pre-approved ceremony, logged 2026-08-08):** detail in
`port/scrum/sprint-82.md`. Started as a check for BoB's inbound S145 trap; the check came back
**N/A for MA** and exposed something much larger.

- **BoB's trap: N/A here, measured.** Their §8z warns that firing OK on a logged child can hit the
  RDialog **panel wrapper** (whose `OnOK` is just `EndDialog`), silently skipping the derived
  handler while looking like success. Printed `typeid(*parent).name()` at MA's fire site: the owner
  is **`9CPlyr_log`**, the derived dialog. MA's host records each control's **own parent node** at
  registration, so *what you hold* is fixed when the control is registered, not when it is fired.
- **⭐ What the check found: the OOB dialogs were RENDER-ONLY.** The map idle routed clicks to the
  two toolbars and nothing else, so Player Log / Squads / Bases / DIS / Overview / Weather painted
  perfectly and **ignored every click** — no tabs, no tick, no rows. Three things had been
  *explaining* it rather than exposing it: the `MA_OOB_PLAYERLOG_TAB` scaffold hook, `ma_tabs_hit`
  sitting **declared with no caller at all**, and MA answering BoB's "how do you dismiss a dialog"
  with the *toolbar* route without noticing the **user's** route did not exist. **A capability only
  ever exercised through scaffolding is evidence the real path is missing.**
- **Fix:** `ma_oob_click_tree_rec` mirrors the paint walk exactly — same tree, same `MaXYOffset()`
  offsets, children before parents — so hit rects cannot drift from drawn rects. An open dialog gets
  first refusal on the click; a click inside it that hits no control is **swallowed** instead of
  panning the map behind it.
- **The tick dismisses the dialog through the DERIVED handler.** A title bar is a `CRButtonCtrl`
  with tick/help flags, and the genuine control owns the `ICONWIDTH`=22 band arithmetic, so the port
  asks it (`MaButtonHit`) rather than inventing regions. Traced: `dispid 3 (OK) on 9CPlyr_log` →
  `CPlyr_log::OnOK (DERIVED) reached` → the `EndDialog` cascade → map renders clean.
- **⚠ New trap banked: the genuine handler you drive may itself contain an unported call.**
  `CRButtonCtrl::OnLButtonUp` opens by dereferencing `GetParent()->SendMessage(WM_GETHINTBOX,…)`,
  and `ON_MESSAGE` is an **empty macro** in compat → 0 → NULL deref. So drive the DOWN half and
  report the dispid the UP half would have fired. *"Drive the real handler" ≠ "drive all of it".*
- **Tab bars switch on a real click** (`ma_tabs_click` → the control's own `SelectTab`); verified by
  capture on "Log of Missions". `ma_tabs_hit` finally has a caller.
- **Scoped:** `ma_button_title_hit` returns −1 for any button without those flags, so every existing
  toolbar/dialog button keeps the identical plain-`Clicked` path. `MA_NO_OOB_CLICK` reverts.
- **Gates (all green, under `gl-lock`):** parity **5/5 byte-identical** — the load-bearing gate
  here, since the diff touches a shared click path and this proves the map/toolbar/menu routes are
  untouched; **stress 20/20**; **ASan 0 reports, 4/4 paths**.
- **Cross-port: MA note 31.** Also resolved a **second** shared-doc collision (BoB's S144 and MA's
  S81 were both §8y → theirs became §8z under their own rule) and adopted their proposed
  collision-proof scheme for this sprint's section: **§8-MA82**.

**Retro.** Two sprints running, an inbound "check whether you have this too" has been worth more
than the answer: S81's check found MA's truncated-save bug had a *different* cause than guessed, and
S82's came back N/A while uncovering that an entire dialog subsystem had never accepted a click. The
lesson to keep is the tell — **scaffolding that exists to exercise a feature is evidence the real
path is missing.** `ma_tabs_hit` had been sitting there with no caller for sprints.

### 🏃 Sprint 81 — "Persist the campaign" — ✅ CLOSED 2026-08-08 (goal MET, 8/8) — ⭐ G2 state persistence CLOSED

**Sprint Review (PO pre-approved ceremony, logged 2026-08-08):** detail in
`port/scrum/sprint-81.md`. **Campaign state now persists under the canonical save name**, and the
`campaign_map` parity oracle that S80 retired is back in service at 0 px.

- **Root cause (instrumented, not reasoned — BoB S143's lesson, applied the day it arrived):**
  `fileman::namenumberedfilelessfail` lacks the "fake long file name" branch that the hard
  `namenumberedfile` has, so it always falls through to the DIR.DIR path — a fixed **12-byte** 8.3
  name, NUL-terminated at byte 12. Under `MA_LINUX` the port routes the buffered
  `FileMan::namenumberedfile(f, buf)` through *that* variant, so **the save path used the one
  function missing the branch.** Every other boot-path name is ≤ 11 chars; `"Auto Save.sav"` is 13.
- **Persistence was never broken — it was invisible.** The port saved *and* loaded under the same
  truncated `Auto Save.sa`, so the round trip was self-consistent and the campaign genuinely
  carried across runs (that is S80's map-date drift). What was broken is that it happened under a
  name nothing outside the port looks at, while the canonical `Auto Save.sav` sat untouched since
  2026-07-19. **A self-consistent wrong value produces no symptom until something outside the
  system looks** — here, a parity capture was that outside observer (banked as shared-doc §8y).
- **Blast radius measured, not assumed** (shared engine primitive): every fake-file name resolution
  in a full campaign boot, diffed before/after — **exactly one string changes**
  (`Auto Save.sa` → `Auto Save.sav`); `dcomms/dreplay/rbackup/replay/tblock/*.sav` all identical.
- **Proof:** run A (2 missions) advances `6/25/50 → 7/3/50 → 7/8/50`, autosaving at each frag;
  **run B, a fresh process, comes up at 7/3/50** — the state run A left, via `Auto Save.sav`.
- **`campaign_map` restored to the gate:** `port/parity_2d.sh` now pins
  `port/ref/save/campaign_pristine.sav` around the capture (and restores the player's own save) →
  **0 px**. The reference was never wrong; the state had drifted. Parity set back to **5 screens**.
- **Adopted from BoB:** the convention's magic numbers (`128`/`8`) were spelled out at **four**
  sites — which is how two of them drifted apart. `FILEMAN.H` now names them
  (`fakefileoffset`/`fakefileindex`) as BoB's does. BoB's *values* (800/50) deliberately not copied.
- **Cross-port:** **MA note 30** sent, leading with a **correction** — note 29 §4 asked BoB to check
  their `fileman`; checked their tree first this time and **they already have the branch (N/A)**.
  §8y appended (renumbered §8v→§8y under BoB's new §8x collision protocol); sync check ✓.
- **Gates (all green, all under `gl-lock`):** parity **5/5 byte-identical**; **stress 20/20**;
  **ASan 0 reports, 4/4 paths 2/2**. The `FILEMAN.H` change rebuilt 207 TUs, so the whole set was
  re-run on the final binary.

**Retro.** The sprint's own lesson is the one it opened with: BoB's "stop reasoning and instrument"
arrived hours before planning and was used immediately — three `fprintf`s named the mechanism in one
run, where the code reads plausibly several ways. The counterpart lesson is mine to own: note 29 §4
had sent BoB an errand on the strength of a plausible mechanism, and one grep of their tree would
have shown it was already fixed there. **Measurement precedes the ask, not just the claim.**

### 🏃 Sprint 80 — "Fly the loop" — ✅ CLOSED 2026-08-08 (goal MET, 8/8) — ⭐ the flyable multi-mission campaign loop runs

**Sprint Review (PO pre-approved ceremony, logged 2026-08-08):** detail in
`port/scrum/sprint-80.md`. **G2's flyable multi-mission loop works** — two campaign missions flown
back-to-back in one process, each debriefed, the period advanced between them, and the campaign
carried through to its own **end-of-campaign screen**.

- **The blocker was the HARNESS, not the game.** `if (++n == N)` on a function-local static fires
  exactly once per process, and this path had **three** of them (frag drive, the Fly click,
  `BOB_AUTOEXIT`). Mission 2 fragged, launched into 3D and then **flew forever** — the exit counter
  had been spent on mission 1. For the port's whole life this read as a *game* limitation ("the
  campaign only does one flyable mission"). `BOB_AUTOEXIT` is now per-flight, re-armed on each
  3D→front-end edge; the loop re-arms the frag/Fly drives after each debrief.
- **The drive** (`MA_CAMP_LOOP=N`, default off) calls the **genuine**
  `CDebriefToolbar::OnClickedNextPeriod` (`DBRFTLBR.CPP:226` → `EndDebrief` → `ChkEndCampaign`)
  rather than reimplementing it; helpers `ma_camp_indebrief/next_period/state` in `MAINTBAR.CPP`.
- **Proof is campaign PROGRESSION, not button presses** — the campaign's own date readout is logged
  each step: `7/8/50 planning` → `7/8/50 debrief` → **`7/19/50 planning`** → mission 2 →
  `7/19/50 debrief` → **`7/20/50 planning`** → `campend` → **end-of-campaign screen**
  (`port/ref/native/campaign_loop_endcamp.png`). Both missions scored *Failure* (autoexit abandons
  them after 40 frames) so the strategic sim ran the UN to defeat — correct behaviour, and it means
  the **whole campaign lifecycle now runs end-to-end natively**.
- **Cross-port:** shared lessons doc byte-identical both sides; **BoB note 18 processed** (their
  `Select(row,COLUMN)` bug → **N/A for MA**, with the structural reason recorded; their open question
  on closing a logged dialog → **answered**). **MA note 29 + §8v sent.**
- **Gates (all green, all under `gl-lock`):** 2D parity **4/4 byte-identical**
  (`title`/`prefs_3d`/`prefs_others`/`quickmission`) via the new one-command `port/parity_2d.sh`;
  **stress 20/20 PASS**; **ASan `asan_all.sh`: 0 reports, 4/4 paths reached 2/2**.

**Retro.** Two lessons, both about trusting a record instead of measuring. (1) A limitation that
had been written down as the game's was three lines of *our own* test scaffolding — the smell test
is now banked: **a drive counter declared inside the block it drives can only ever run once.**
(2) The parity gate's tab-click pixel in `screen-parity.md` was stale, so the "Others" capture
silently grabbed the **Game** tab — the exact trap S62/S63 documented, re-sprung by trusting the
documented pixels. Recipes now live in the gate script in font-independent `#ID:COL` form.
**Also:** `campaign_map` is no longer a valid byte-identical oracle (it renders live save state that
our own campaign test runs advance); the S60 A/B settled it in one step — the pre-S80 binary
produced a byte-identical capture, so the 8095px delta is state, not code.

### 🏃 Sprint 79 — "Land the loop fix" — ✅ CLOSED 2026-08-03 (fix LANDED) — ⭐ the campaign advances after a flown mission

**Sprint Review (PO pre-approved ceremony, logged 2026-08-03):** detail in
`port/scrum/sprint-79.md`. **The G2 flyable multi-mission loop's blocker is fixed** — flying a
campaign mission now completes the debrief and advances the campaign, where before it hung.

- **Mechanism nailed:** the "again without closing" message is actually *informational* in the
  port (`[For your information.]`, non-fatal); the real damage was the **duplicate `fileblocklink`**
  the debrief preload's re-open of an already-open `FIL_ICON_BASES` creates + its paired `delete`,
  corrupting the openfiles accounting → the campaign-debrief setup died. (`DrawIcon` uses a stack
  RAII fileblock, so the icon isn't leaked there; the map render/cache holds 0x6a63 open.)
- **The fix (14 lines, targeted):** a read-only `fileman::MA_IsFileOpen` (walks `openfiles`) +
  guard the debrief preload loops (`FULLPANE.CPP:2706-2709`, `MA_LINUX`) to skip already-open
  files (already loaded → the preload is a no-op, and no corrupting duplicate is created).
- **Verified:** `MA_CAMP_FLY=1 BOB_AUTOEXIT=40` → CAMP branch (`indebrief=TRUE`+`NextMission`) →
  **the operational map returns** (96 renders vs ~1 line before), date advanced **"Morning,
  planning" → "Morning, debrief"** — the campaign progressed; map renders cleanly, no corruption.
- **Gates:** 2D parity byte-identical (title/prefs_3d/campaign_map 0px — `MA_LINUX`, debrief-path
  only); **stress 20/20 PASS** under `gl-lock`; **ASan `asan_all.sh` PASS — 0 reports, 4/4 paths**.

**Retro.** A three-sprint chain converged a vague "campaign loop broken" into a 14-line fix:
S77 *assumed* gamestate (wrong); S78 *measured* gamestate=CAMP + found the fileblock double-open;
S79 found the double-open was informational and the real damage was the duplicate-link
corruption, and guarded exactly the redundant preload. Measure-don't-assume, then fix the
*specific* thing.

### 🏃 Sprint 78 — "Chase the loop blocker" — ⚠️ CLOSED PARTIAL 2026-08-03 (S77 corrected; real blocker = a leaked fileblock)

**Sprint Review (PO pre-approved ceremony, logged 2026-08-03):** detail in
`port/scrum/sprint-78.md`. **S77's `gamestate` hypothesis is DISPROVED; the campaign genuinely
advances on a flown mission, and the flyable-loop blocker is now a specific named bug.**

- **`gamestate=CAMP`** at `OnFlyingClosed` (traced) — not HOT/QUICK. The campaign branch executes:
  `FULLPANE.CPP:2678` `MMC.indebrief=TRUE` + `:2698` `MMC.NextMission()` both run (`[debrief] CAMP
  branch: indebrief=TRUE set, calling NextMission`). **The campaign advances on a flown mission.**
- **Real blocker: a leaked fileblock.** The campaign-debrief map reload (`:2706-2709`) re-opens
  **`FIL_ICON_BASES` (0x6a63)** — `[SysError] Opened file block (6a63) again without closing`
  (FILEMAN `:1542`) — and the debrief setup **hangs** (clean timeout, no crash). The icon file was
  already open (leaked) from the map-render icon path.
- **Concrete G2 next step:** find where the map render opens `FIL_ICON_BASES` without closing and
  close it (or make the reload tolerant); then the flyable loop should complete. The
  `DebriefToolBar().OnClickedNextPeriod()` drive (S77) is ready to re-add.
- **Gate:** the only change is two gated `MA_TRACE_3D` traces on the campaign-debrief path (no-op
  unless the env var is set) — build behaviour unchanged.

**Retro.** Measure-don't-assume, twice: S77 *assumed* gamestate and was wrong; S78 *measured*
`gamestate=CAMP` and then *measured* the actual failure (a named fileblock leak). Two investigation
sprints converged the flyable-loop blocker from "somewhere in the campaign exit" to a single leaked
`FIL_ICON_BASES` open — a fixable, specific bug.

### 🏃 Sprint 77 — "Fly the campaign loop" — ⚠️ CLOSED PARTIAL 2026-08-03 (G2 flyable-loop boundary located)

**Sprint Review (PO pre-approved ceremony, logged 2026-08-03):** detail in
`port/scrum/sprint-77.md`. **The flyable multi-mission loop's blocker is now precisely located.**

- **The Next-Period drive is trivial** — `DebriefToolBar().OnClickedNextPeriod()`
  (`DBRFTLBR.CPP:226` → `MMC.EndDebrief` → back to the map for the next mission). A gated
  `MA_CAMP_LOOP` hook driving it was written + built.
- **But the campaign debrief is never reached on the tested path:** `MMC.indebrief=0` at the
  post-flight panel (traced). Root cause — `RFullPanelDial::OnFlyingClosed` (`FULLPANE.CPP:2603`)
  branches on **`gamestate`**: `HOT`/`QUICK` → `quickmissiondebrief` (no `indebrief`, the exit-key
  debrief); **else campaign/`WAR`** → `FULLPANE.CPP:2674` `MMC.indebrief=TRUE` + `MMC.NextMission()`
  → the campaign debrief with Next-Period. `MA_CAMP_FLY`+`BOB_AUTOEXIT` exited into the HOT/QUICK
  branch, so the flyable loop was never reached.
- **Concrete G2 next step:** verify/fix the `gamestate` on the campaign frag-fly path so
  `OnFlyingClosed` takes its campaign branch (open: does `FragFly`/`StartFlying` set campaign
  gamestate, or does the port default to QUICK? does a campaign mission need to *complete*, not
  just exit?). The `MA_CAMP_LOOP` drive is correct, ready to re-add once `indebrief` is reachable.
- **Gate:** no code change (the hook was reverted as unverified) → build unchanged.

**Retro.** Measure-don't-assume again: the loop *looked* one hook away and is actually gated on a
mission-state distinction (`gamestate`→`indebrief`). Reverting the unverified hook rather than
committing dead-ish scaffolding keeps the tree honest; the deliverable is the located boundary.

### 🏃 Sprint 76 — "Scope the campaign" — ✅ CLOSED 2026-08-03 (goal MET) — G2 re-scoped ⬜→🔨

**Sprint Review (PO pre-approved ceremony, logged 2026-08-03):** detail in
`port/scrum/sprint-76.md`. **The campaign (G2) is far more complete than the backlog implied** —
tested headless (no assuming), the single-mission flow and Mission-1→Mission-2 chaining both work.

- **Single-mission flow works end-to-end** (`MA_CAMP_FLY=1 MA_ENABLE_3D=1 BOB_AUTOEXIT=60` under
  dummy): operational map (icons/frontline/routes/date) → frag → **briefing** → **campaign
  flight** → flight-close → **debrief**. Every stage traced.
- **Multi-mission chaining works** (`MA_CAMP_NEXTDAY=1`): the advance opened **"MISSION 2
  BRIEFING"** (D.I.S. dialog) — the campaign progresses Mission 1 → Mission 2. Artifact
  `campaign_mission2_brief.png`.
- **G2 re-scoped ⬜→🔨** — remaining is *verification + polish*, not a from-scratch build:
  (1) state **persistence** across missions (save/load resumes at the right mission), (2) the full
  **flyable** multi-mission loop (fly M2→debrief→M3), (3) edge cases (debrief Next-Period drive,
  campaign-end, the Overview black-rect / RScrlBar).
- **Gate:** no code change (investigation + 2 capture artifacts) → build unchanged, gates
  unaffected by construction.

**Retro.** The S72/S74/S75 lesson again: *measure, don't assume*. G2 was carried as ⬜ "not
started, 21 pts" for the whole run; one afternoon of headless drives showed the core campaign —
including cross-mission chaining — already works. The remaining work is real but bounded, and the
epic is de-risked. The enabler was the S75 realisation that the whole campaign flow (flight
included) runs under `SDL_VIDEODRIVER=dummy`, so scoping needed no display at all.

### 🏃 Sprint 75 — "Capture the debrief" — ✅ CLOSED 2026-08-02 (goal MET) — ⭐ I1 INVENTORY COMPLETE

**Sprint Review (PO pre-approved ceremony, logged 2026-08-02):** detail in
`port/scrum/sprint-75.md`. **The last uncaptured gold shot (#12 debrief) is captured and matches
gold — the I1 inventory (all 15 gold shots with native captures) is now complete.** Delivered
with **zero code change** and **zero display contention**.

- **The debrief was reachable via an existing hook.** `CloseWindow`'s default id is `IDOK`
  (`STUB3D.H:314`) → `OnOK` → `OnFlyingClosed` → `LaunchScreen(debrief)`, triggered by
  `OverLay.quit3d=1`. The scriptable path already exists: `BOB_AUTOEXIT=N` (`MIG.CPP:1004`) →
  `ma_request_flight_exit()`, fired from the **main thread** right after `ma_process_flight_close`
  so the exit drains promptly. (A first bespoke draw-thread hook starved the main thread — 58k
  spin frames — and was reverted.)
- **Captured HEADLESS** — the ASan camp-fly mode already proves 3D flight runs under
  `SDL_VIDEODRIVER=dummy`, so `MA_ENABLE_3D=1 BOB_AUTOEXIT=60 MA_SHOT=220` under dummy flies Hot
  Shot → auto-exits → GL-free `MA_SHOT` of the 2D debrief canvas. No `gl-lock`, no Julia/BoB
  contention.
- **A/B = strong match:** identical layout, the **same pilot briefing photo**, mission header,
  Claims table (Player/UN/Red), yellow small-caps BACK/AC STATS/GROUND STATS/REPLAY chrome. The
  only differences are **mission-type data** (Hot Shot air-to-air → aircraft Claims vs gold's
  ground-attack → ground-target Claims; the default AC/Ground-Stats view follows the mission) —
  not render deviations. **#12 → CLOSE; I1 COMPLETE.** Ref `port/ref/native/flight_debrief.png`.
- **Gate:** no code change (bespoke hook reverted) → nothing in the build changed; the only tree
  delta is the new ref + docs, so ASan/stress/2D-parity are unaffected by construction.

**Retro.** Two efficiency wins from *reading before building*: (1) the flight-exit hook I set out
to write already existed (`BOB_AUTOEXIT`), and my bespoke draw-thread version was strictly worse
(main-thread starvation) — checking the idle loop first would have skipped the detour; (2)
realising 3D flight runs under `SDL_VIDEODRIVER=dummy` (already true in the ASan suite) turned a
display-contended, iterative, flaky gl-lock capture into a clean headless one. The sprint's whole
deliverable landed with zero committed code.

### 🏃 Sprint 74 — "Face the debrief" — ⚠️ CLOSED PARTIAL 2026-08-02 (tooling + characterization; main capture scoped)

**Sprint Review (PO pre-approved ceremony, logged 2026-08-02):** detail in
`port/scrum/sprint-74.md`. **The last uncaptured gold shot (#12) turned out to be a different,
heavier screen than the inventory note implied** — the honest finding is the deliverable, plus a
reusable capture hook and a verified sub-view.

- **A reusable headless hook `MA_OOB_OVERVIEW`** (`MIG.CPP` campaign-map idle → `OnClickedOverview`
  → the Overview stats panel), mirroring `MA_OOB_PLAYERLOG`. Captured
  `port/ref/native/campaign_overview.png` GL-free. The Overview `CAC_view` claims table renders
  correctly (title chrome + `?`/`✓`, Ac Stats/Ground Stats tabs, Kills+Losses × aircraft type,
  yellow sans headers, translucent photo) = gold #12's **"Ac Stats" sub-view**.
- **Finding: gold #12 is the post-mission DEBRIEF**, not the Overview — full-screen pilot photo +
  mission header + **ground-target** Claims (Supply/Bridge/Troops/Tank × Player/UN/Red) + REPLAY,
  reached only via the mission-end path (`FULLPANE.CPP:2674` `MMC.indebrief`+`MMC.NextMission()`).
  No clean headless trigger; capturing it proper needs a real mission→debrief run (display-bound;
  the display was Julia-held for much of the sprint). Scoped to a dedicated session.
- **Gate:** the hook is a gated `getenv` no-op when unset — `campaign_map` byte-identical (0 px)
  with the hook unset, so no normal path (2D/flight/campaign/ASan/stress) can be affected.

**Retro.** Same shape as S64/S72: the inventory note ("Debrief (Claims table) — capture after a
flight exit") quietly conflated two screens. Doing the A/B against gold *first* — rather than
assuming the Overview *was* the debrief — caught it and turned a mislabeled "quick capture" into
an accurate scope. The reusable hook is real value regardless; the honest partial beats a
mislabeled "close" of the wrong screen.

### 🏃 Sprint 73 — "Unmask the cockpit" — ✅ CLOSED 2026-08-02 (goal MET) — ⭐ I3 #10 + #11 CLOSE

**Sprint Review (PO pre-approved ceremony, logged 2026-08-02):** detail in
`port/scrum/sprint-73.md`. **The cockpit-black deviation (#10) — the headline of the 3D-view
parity frontier S72 opened — is FIXED and gold-verified; it LANDED, unlike S72's scoped
handoff.** #11 external confirmed clean as a bonus.

- **S73-1 — root cause found, and all three of S72's hypotheses REFUTED with hard `gl-lock`
  data.** (a) *Lighting* — `MA_TRACE_PITLIGHT`: cockpitAmbient = landAmbient = (255,255,255),
  and the world renders fine with that ambient. (b) *Imagemap-not-loaded* (S72's stated
  narrowing) — `MA_TRACE_LBM`: 236 LBM bodies load, none all-black; the cockpit texel indices
  are present. (c) *Palette-not-populated* — `XX_PalChange` runs (`branch=software-buffer`,
  `lpDirect3D=NULL` because the port forces `fSoftware`), and `palette_table` is populated with
  real 565 colours for world object polys. **The actual mechanism:** on the software raster path
  the active 8→16bpp LUT (nasm `palette_table`) is left **stale/empty at cockpit-draw time**,
  and the cockpit's own `createpoly→SelectPalette(0)` **no-ops** because `polygon::selectedPalette`
  cache already reads 0 — so every cockpit imagemap/flat texel indexes an empty LUT → near-0
  (black) 565 pixel. Terrain is immune (renders via `LandFadeData`, not `palette_table`), which
  is why only the cockpit showed it. Proven by a diagnostic that forced `SelectPalette(0)` for
  cockpit polys → the flat-black cockpit turned fully textured.
- **S73-2 — the fix is the engine's own disabled reset, re-enabled.** `BTREE.CPP:580` carries
  `//dead POLYGON.SelectPalette(0)` — the original per-object palette reset (every object case
  in `drw_obj` has one, all disabled: fine for hardware D3D per-texture palettes, broken for the
  software port). Re-enabled for the cockpit, forced past the stale cache
  (`POLYGON.selectedPalette=-1; POLYGON.SelectPalette(0);`, `MA_LINUX`). Clean capture = gold #10:
  textured canopy + panel + gunsight drum (10-40) + ADI inset content. **Bonus:** external #11
  F-86 renders fully textured (silver/yellow skin, "FU-908", drop tanks); the S72 "aircraft
  near-silhouette dark" is not present. **#10 + #11 → CLOSE.**
- **S73-3 — gates + close. ALL GREEN.** 2D parity byte-identical (title 0px / prefs_3d 0px — the
  fix is a single `MA_LINUX` block in the 3D object dispatcher, 2D never enters it). **ASan
  `asan_all.sh` PASS — 0 reports across all 4 paths** (flight + campaign map/fly/nextday, 2/2
  each; both cockpit-exercising paths clean). **Stress `stress_launch.sh` under `gl-lock` PASS —
  20/20** (sustained 100 3D frames, 0 crashes). Diagnostics reverted; committed diff is the
  13-line `BTREE.CPP` block only (re-inserted byte-level to avoid the high-byte banner-encoding
  noise; the prior partial session's `3DCOM.CPP` pitlight probe + banner noise reverted).
  Cross-port note DEFERRED (shared lessons file live-edited by the concurrent BoB session).

**Retro.** Two lessons. (1) **All three prior "root causes" were plausible and wrong** — S72's
"imagemaps resolve to black (not loaded/bound)" would have sent S73 hunting a load bug that
doesn't exist. Only stepping through the *whole* pipeline with gated per-stage traces
(pitlight → LBM → palette → per-poly filler) reached the truth, and each stage cleanly refuted a
hypothesis rather than confirming a guess. (2) **"Pure black" was a mis-sample** — a few sampled
points read (0,0,0); dense sampling of the *fixed* frame showed 8.35% exact-black over rich
metallic texels. Measure the whole field, not a point (S64 family). The fix itself was one 2-line
block the engine authors had already written and disabled.

### 🏃 Sprint 72 — "Light up the 3D overlays" — ⚠️ CLOSED PARTIAL 2026-08-02 (3/6 pts — investigation delivered, fix scoped)

**Sprint Review (PO pre-approved ceremony, logged 2026-08-02):** detail in
`port/scrum/sprint-72.md`. **First sprint on the 3D-view parity frontier (I3).** Investigation
delivered a precise A/B and root-cause narrowing; the render fix itself is scoped for a focused
follow-up rather than forced into a long autonomous run.

- **Grounded the epic with a `gl-lock` A/B capture** of the current cockpit vs gold #10 (the GL
  run also confirmed **S69's per-face fonts work in the GL path**, not just headless).
- **Characterized #10/#11 precisely:** the cockpit frame + instrument panel render as a **crisp
  FLAT-BLACK silhouette** (geometry rasterizes correctly; only the fill is black), plus a
  native-only **black rectangle top-right** (the padlock-ADI inset). Gold shows a fully textured
  metallic canopy + detailed panel.
- **Root cause narrowed** (a real advance on the prior vague "palette/texture upload"): the
  software rasterizer **HAS the image-span fillers** (`XASM_ImageHoriLine*` in `ma_xasm.nasm`)
  and world terrain + the gunsight texture render — so it is NOT a missing primitive, and
  `textureQuality` (default High) does not gate it. ⇒ the **cockpit-specific imagemaps resolve
  to black (not loaded/bound)** on the `btree::drw_cockpit` (`COCKPIT_OBJECT`) shape path. Scoped
  S73 fix target: trace the cockpit shape's per-poly `Image_Map.GetImageMapPtr` binding vs a
  rendering world poly.
- **Honest close:** the fix is a deep per-poly texture-binding change that wants a focused
  session and iterative GL captures under low display contention; forcing a speculative render
  change into a long autonomous run risks a wrong, parity-poisoning result. The 3 missing points
  are the un-landed fix, stated plainly.

**Gates.** No code changed (pure investigation) → build unchanged, no ASan/stress needed; the
deliverable is the A/B evidence, captured via `gl-lock`. **Note (CONCURRENCY.md corrected
2026-08-02):** concurrent rendering does NOT corrupt captures (each reads its own framebuffer) —
the real serialization reason is GPU/CPU contention, which can manufacture a stress `HANG`. This
retroactively confirms the S69 HANGs were load artefacts, not faults.

### 🏃 Sprint 71 — "Polish the chrome" — ✅ CLOSED 2026-08-02 (6/6 pts, goal MET)

**Sprint Review (PO pre-approved ceremony, logged 2026-08-02):** detail in
`port/scrum/sprint-71.md`. **Both S70/S69 chrome residuals resolved — one fixed, one measured
away.** The front-end 2D parity epic (EPIC I) is now essentially complete.

- **S71-1 — the OOB-listbox translucency, fixed with an OOB-only context flag.** S70 established
  that skipping the `CRListBoxCtrl` black fill globally erased the front-end title menu (the
  menu is the same control and relies on the opaque box). Added `ma_oob_lb_draw` (defined in
  `ma_olecontrol.cpp`, read in `RLISTBXC.CPP`), set to 1 only while `ma_ole_draw_toolbar` draws
  an OOB-path listbox → the fill is skipped there so the Player Log Career/Log tables composite
  over the dialog background (the pilot photo shows through = gold's translucency), while the
  front-end path never sets it and stays **byte-identical** (title 0 px). Gold-matched.
- **S71-2 — the combo border residual did not exist.** S69 named native's combo border
  "rectangular light vs gold's rounded-blue". Measured: `AXC_DARKEDGE/LITEDGE/CIRCULAR =
  RGB(103,132,198)` (blue) and `m_bCircularStyle=FALSE` always, so native and gold draw the
  same blue rectangular border + round button; a matched-scale crop of native vs gold #2
  confirms it. The "white" reading was the anti-aliased blue edge at 800-res. **Retired, not
  fixed** — an S64-style "measure, don't assume" close. Cross-cutting #2 fully matched.
- **S71-3 — cross-port note** (the OOB-only context-flag technique, completing note 27's
  deferred fix) to `bob/doc/`.

**Gates.** **Front-end 2D parity byte-identical sweep PASSES** — title / prefs_3d / prefs_others
/ quickmission / campaign_map all 0 px (the flag only affects OOB listboxes); `map_playerlog` +
`map_playerlog_tab1` rebased for the translucent tables. **ASan `asan_all.sh` PASS — 4/4 paths,
0 reports. Stress `stress_launch.sh` under `gl-lock`: 20/20 OK.** Cross-port note 28.

### Sprint 71 planning — "Polish the chrome" — PLANNED 2026-08-02 (PO pre-approved ceremonies)

**Environment check:** no stray `wmig`, build current at `d92b791` (S70). Untracked
`CONCURRENCY.md` + the BoB-authored `port/BOB_PORT_LESSONS.md` working-tree change (left alone).

**Context:** S69/S70 closed the front-end 2D parity headliners (font, combo, Player Log table).
Two named residuals remain, both chrome polish: the OOB-listbox opaque box (S70 residual — the
Player Log tables read opaque vs gold's translucent) and the combo border pen colour (S69 #2
residual — gold's rounded-blue vs native's rectangular light edge).

**Sprint Goal:** the Player Log Career/Log tables show the photo through (gold's translucency)
without regressing the front-end menu, and the combo border moves toward gold — both held to
the byte-identical sweep.

**Committed (~6 pts):**
| Story | Pts | Definition |
|---|---|---|
| S71-1 OOB-listbox translucency | 3 | A context flag skips the `CRListBoxCtrl` black fill on the OOB draw path ONLY (not the front-end menu, which relies on it — S70's regression). Player Log tables translucent = gold; front-end byte-identical |
| S71-2 Combo border pen colour | 2 | Investigate the combo border pen (`AXC_*EDGE` vs circular-style); move toward gold's rounded-blue if low-risk, else name it PO-waived |
| S71-3 Gates + close + note | 1 | parity sweep + asan + stress (gl-lock); docs/memory; commit |

Board: `port/scrum/sprint-71.md`. **NOT pulled:** RScrlBar hosting, `ma_tabs_hit` click routing,
#12 debrief capture, 3D-view parity (I3, #10/#11).

### 🏃 Sprint 70 — "Finish the Player Log" — ✅ CLOSED 2026-08-02 (8/8 pts, goal MET) — I4/#15 CLOSED

**Sprint Review (PO pre-approved ceremony, logged 2026-08-02):** detail in
`port/scrum/sprint-70.md`. **The Player Log Career content table renders — the last open half
of I4/#15, deferred since S56, is closed.** Parity #15 → CLOSE.

- **S70-1 — font-rebase debt cleared, and one of the two was a false alarm.** `campaign_map`
  came back **byte-identical** (the map date readout uses `g_AllFonts[1]="Intel"` = the art
  face, untouched by S69's font change) — S69's "must rebase" flag on it was overcautious.
  Only `map_playerlog_tab1` genuinely changed (sans text) and was rebased. The byte-identical
  sweep resumes.
- **S70-2 — the Career table was populated but never DRAWN; a missing draw-case.** The Career
  tab (`IDD_CAREER`/`CCareer`) builds its Sorties/Combats/Kills/Losses table as an RListBox
  (`IDC_RLISTBOXCTRL1`) in `OnInitDialog`; the Name box on the same tab rendered but the table
  didn't. Root cause: the OOB dialog draw path (`ma_oob_render_node` → `ma_ole_draw_toolbar`)
  dispatched STATIC/EDIT/EDTBT/TABS/BUTTON/COMBO but had **no `CT_LISTBOX` case** — the
  front-end draws listboxes via a *different* path (`ma_ole_draw_all`), which masked the gap.
  Added the case (drive `CRListBoxCtrl::OnDraw` at the toolbar-offset rect). **The table now
  renders with data** (F86 1/F86 2/F80/F84/F51/All × the four columns, all 0 on a fresh save),
  and the same fix lit up the **Log of Missions** tab's log listbox as a bonus. Two residuals
  named: (a) the listbox draws over an OPAQUE box vs gold's translucent — skipping the fill à
  la the combo (#2) **erased the front-end title menu** (the menu is the same `CRListBoxCtrl`
  and relies on the opaque box), so it was reverted and needs an OOB-only context flag;
  (b) a doubled "F86 1" header cell (`CAREER.CPP` adds the label twice — a source-vs-BDG data
  delta).
- **S70-3 — cross-port note 27** (the missing-OOB-listbox-case + the load-bearing-fill caveat
  that distinguishes the listbox from the combo) delivered to `bob/doc/`.

**Gates.** **2D parity byte-identical sweep RESUMED and PASSES** — title / prefs_3d /
prefs_others / quickmission / campaign_map all 0 px vs the S69 refs (the `CT_LISTBOX` case only
touches the OOB draw path); `map_playerlog` + `map_playerlog_tab1` rebased for the tables.
**ASan `asan_all.sh` PASS — 4/4 paths reached, 0 reports** (headless; the campaign paths
exercise the new OOB listbox draw). **Stress `stress_launch.sh` under `gl-lock`: 20/20 OK**
(clean pass — lower load this run, confirming the S69 HANGs were load-induced).

**Retro.** The win was diagnostic discipline: the Name box rendering "proved" the dialog and
OnInitDialog were fine, which pointed straight at the draw layer rather than data/population —
and the fix was one `case`. The near-miss was the listbox translucency: it *looked* like the
combo fix (identical `FillRect(BLACK_BRUSH)` pattern) but the byte-identical sweep caught that
the same skip erased the title menu, because the listbox is also the front-end menu surface
while the combo never is. The combo and the listbox are not the same fix.

### Sprint 70 planning — "Finish the Player Log" — PLANNED 2026-08-02 (PO pre-approved ceremonies)

**Environment check at planning:** session display held by the Julia Racer session (its 3-min
JM_SHOTS block — expected; S70's work is 2D/headless and needs no lock), no stray `wmig`, build
current at `51e0c81` (S69). Tree carries an untracked `CONCURRENCY.md` and one working-tree
addition to `port/BOB_PORT_LESSONS.md` (BoB's §8o lesson, not this session's — left for its
author).

**Context:** S69 closed both cross-cutting front-end deviations (font + combo). Two things
remain on the parity queue: a small **rebase debt** S69 deliberately deferred, and the **last
open half of I4** — the Player Log Career **content table**, deferred as "a full sprint on its
own" since S56.

**Sprint Goal:** the font-rebase debt is cleared (byte-identical sweep can resume), and the
Player Log Career tab's Sorties/Combats/Kills/Losses table renders with data — closing #15.

**Committed (~8 pts):**
| Story | Pts | Definition |
|---|---|---|
| S70-1 Clear font-rebase debt | 1 | Re-capture `map_playerlog_tab1` + `campaign_map` (font-touched S69); rebase refs; byte-identical sweep resumes on the full set |
| S70-2 Career content table | 6 | Investigate where the per-type table (F86 1/F86 2/F80/F84/F51/All × Sorties/Combats/Kills/Losses) controls + data come from; render it with data on the Career tab. Investigation with a visual stretch (S64 retro) — acceptance is the table visible + populated, or a precisely-located blocker if a layer underneath resists |
| S70-3 Cross-port note + close + gates | 1 | Note to `bob/doc/` if a shared-engine finding; parity sweep + `asan_all.sh` + `stress_launch.sh` (gl-lock); docs/memory; commit |

Board: `port/scrum/sprint-70.md`. **NOT pulled:** combo border pen colour (#2 residual),
RScrlBar hosting, `ma_tabs_hit` click routing, #12 debrief capture.
**Order:** S70-1 first (cheap, unblocks the gate), then the S70-2 investigation.

### 🏃 Sprint 69 — "Face the type, dress the combo" — ✅ CLOSED 2026-08-02 (8/8 pts, goal MET) — ⭐ CROSS-CUTTING #1 & #2 BOTH CLOSED

**Sprint Review (PO pre-approved ceremony, logged 2026-08-02):** detail in
`port/scrum/sprint-69.md`. **The two remaining cross-cutting visual deviations are both
closed** — the font FACE (the three-sprint carry) and the combo chrome (the largest remaining
gap). Scheduling per-face first, as the board prescribed, is why it finally landed.

- **S69-1 — the port was silently running as a JAPANESE system, and that is why it never
  used Arial.** Two silent-fallback bugs, the same shape as S66 (Intel.ttf) and S68 (icons).
  (a) `ma_gdi_font_create` **ignored the requested face** — replaced with a cached ART/SANS/
  SERIF registry (ART=Intel.ttf load-order-preserved so ART screens stay byte-identical;
  SANS=LiberationSans≈Arial; unknown→ART so nothing regresses). (b) The deeper find, via
  `MA_TRACE_FONT`: the runtime faces were **mojibake CJK** (`ＭＳ 明朝`/`ゴシック`), never
  Arial. `MIG.CPP`'s localization probe calls `EnumFontFamilies(MS-Mincho)`; the compat stub
  **always** invoked the proc, so `gotfont` was always true → the Japanese branch → MS Mincho
  everywhere → collapsed to the art face. On the English box gold came from, the CJK probe
  fails → `myfont=Intel`, `straightfont=Arial`. Fixed the stub to report a face present only
  for a **pure-ASCII** name (no CJK ships). Runtime faces are now `Intel`(ART)+`Arial`(SANS)
  as on gold. **Gold-verified:** Preferences #2/#8 and Quick Mission #9 render **blue sans
  labels + yellow sans values** = gold; campaign #13 phase list yellow sans = gold; Intel bars
  byte-identical. **Font FACE half of cross-cutting #1 CLOSED** (colour was S63).
- **S69-2 — the combo box was the one hosted control still filling itself opaque black.**
  `CRComboCtrl::OnDraw` fills black (`RCOMBOC.CPP:355`) when `WM_GETARTWORK` returns 0, and
  the port deliberately returns 0 (the panel's OnPaint already composited the background;
  controls draw transparently over it). Gold's combos are **transparent** — the panel shows
  through a thin bordered outline (verified by cropping gold #2). Skipped the black `FillRect`
  on the `MA_LINUX` path; the border pens + transparent `FIL_COMBO_BUTTON` still draw the
  chrome. Combos now translucent = gold. **Cross-cutting #2 CLOSED** (residual: a fainter
  rounded-blue border pen colour, named).
- **S69-3 — cross-port note 26** (shared: the `EnumFontFamilies` Japanese-branch trap + the
  per-face registry + the combo fill) delivered to `bob/doc/`.

**Gates.** **2D parity = deliberate REBASE toward gold** (as S63/S66 — the font+combo change
every label/combo screen by design): re-captured and gold-verified, 10 refs rebased (`title`
unchanged/byte-identical; the 7 prefs tabs, `quickmission`, `campaign_select`,
`map_playerlog`). `map_playerlog_tab1` + `campaign_map` also font-touched → flagged for S70
re-capture before byte-identical resumes. `prefs_controls` remains the environment-dependent
oracle (joystick attached). **ASan `asan_all.sh` PASS — 4/4 paths reached, 0 reports**
(headless `SDL_VIDEODRIVER=dummy`; flight also 2/2). **Stress `stress_launch.sh` under
`gl-lock`: 37/40 OK across two runs, 3 HANG, 0 crashes — every HANG is a 25 s timeout under
load 8–9 (three sessions live + Julia holding the display), NOT a fault** (0 SEGV/FPE/ABORT/
NO3D, the 3D-startup crash/race classes A1 and the gate actually target; the S59 contention
artifact, not a regression).

**Retro.** The lesson repeats and is worth stating plainly: **a compat stub that returns
*success* is invisible.** `EnumFontFamilies`-always-true joins `DrawIcon`-noop (S68) and
`GetFileNum`-returns-0 (S64) — three sprints running, the root cause was a stub that lied
about succeeding, and each hid a whole class of wrong output with no error and no trace. The
standing check earns its place: for any compat function whose *return value* gates engine
behaviour, verify it returns the truth, not just a non-crashing value. And scheduling the
carried story FIRST (the S66 tactic) worked a third time — per-face had been displaced S65/66/
67-8 and landed the moment it was protected at the top of the plan.

### Sprint 69 planning — "Face the type, dress the combo" — PLANNED 2026-08-02 (PO pre-approved ceremonies)

**Environment check at planning:** session **UNLOCKED** (`gl-lock --status` → `display free`),
no stray `wmig` (`pgrep -x wmig` empty), build current (`ninja: no work to do` at `9624cbe`).
Tree clean bar untracked `CONCURRENCY.md` (the parallel-session rules file, intentionally
not committed). Two sibling sessions (BoB scrum, Julia Racer) share the one display — every
render/capture goes through `gl-lock`.

**Context:** S68 closed the **last chrome deviation on parity #15** (the Player Log `?`/`✓`,
which surfaced a whole missing subsystem — icons had never rendered). Two items now sit at
the top of the queue, and the board has been explicit about the order for three sprints:

1. **Per-face fonts — carried S65, S66, S67/S68, three times, always displaced.** The retros
   name this a *prioritisation* failure, not bad luck, and prescribe the S66 tactic that
   worked for the font FACE: **schedule it FIRST and protect it.** `ma_gdi_font_create`
   still `(void)face`s the requested face; every string draws in the single global TTF
   (Intel.ttf since S66) regardless of what the game asked for. Planning established the
   request set is **not** Intel-only: `MIG.CPP:379-390` / `:699-710` build the font table
   from **`Intel`, `Free`, `Header`, `Arial`, `Times New Roman Bold`, `MS Serif`,
   `Arial Italic`** — but **only `Intel.ttf` ships** in `drive_c/windows/Fonts/`. On Windows
   the non-Intel names resolved to real installed faces (a sans/serif distinction between
   data text and the Rowan headers); the port forces all of them through Intel, which is why
   S66 read "matches gold by luck". So this is a genuine, **measurable** story, not plumbing.
2. **Cross-cutting deviation #2 — combo chrome** — now the largest remaining visual gap
   (screen-parity.md deviation 2): native combos draw black-filled with a white border;
   gold's are translucent panels. One draw-path fix in `ma_olecombo`/`ma_gdi`.

**Sprint Goal:** `ma_gdi_font_create` honours the requested face through a small cached
face registry, verified against gold (front-end front-end stays byte-identical or moves
*closer* to gold — measured, never assumed); and the combo chrome moves toward gold's
translucent panel — held to the dummy==GL byte-identical bar where a screen is unchanged.

**Committed (~8 pts):**
| Story | Pts | Definition |
|---|---|---|
| S69-1 Per-face font selection | 5 | `ma_gdi_font_create` resolves the `face` arg through a face→TTF cache (Intel→Intel.ttf; Arial/Free/Header/system names→system sans/serif fallbacks); `MaFont` carries its face; text/extent draws route through the DC font's face not the global. `MA_TRACE_FONT` traces face resolution. Parity sweep: every front-end screen either byte-identical or a measured, gold-justified improvement — a regression away from gold is not acceptable |
| S69-2 Combo chrome toward gold | 2 | Root-cause the black-fill/white-border draw path; move it toward gold's translucent panel; re-capture the combo-bearing screens; parity table updated (fixed or PO-waived with reason) |
| S69-3 Cross-port note + close + gates | 1 | Cross-port note to `bob/doc/` (shared GDI font path); `asan_all.sh` + `stress_launch.sh` + parity sweep PASS; board/burndown/parity/RUNNING/STATUS/memory updated; committed on `linux-port` |

Board: `port/scrum/sprint-69.md`. **NOT pulled** (each substantial, consistent with prior
discipline): Career **content table** (the half of I4 never pulled), RScrlBar hosting,
`ma_tabs_hit` click routing, #12 debrief capture.
**Order is deliberate:** S69-1 lands before S69-2 — per-face first, protected, per the board's
standing instruction; and font changes ripple through every screen, so the combo re-capture
must sit on top of the settled font baseline, not race it.
**Risk noted at planning:** honouring faces could regress a screen that currently matches
gold by using Intel for text gold renders in a system face — or vice-versa. The dummy==GL /
gold parity sweep is the gate; if honouring a requested face moves a screen *away* from gold,
fall back to the art face for that face-name and record it (the S64 art-name lesson: measure,
don't assume, and be willing to ship the honouring OFF for a given name).

### 🏃 Sprint 68 — "Icons and evidence" — CLOSED 2026-08-02 (see board for points)

**Sprint Review (PO pre-approved ceremony, logged 2026-08-02):** detail in
`port/scrum/sprint-68.md`.

- **S68-2 — the `?`/`✓` are rendering, and the cause was far bigger than one screen.**
  Routed by engine logic rather than pixels: `RDialog`'s eventsink has
  `ON_EVENT(RDialog, IDJ_TITLE, 2 /*Cancel*/…)` and `3 /*OK*/`, so the **title control
  itself** raises Cancel/OK and therefore draws its own buttons (`RBUTTONC.CPP:521-536`),
  gated on persisted `CloseButton`/`TickButton` — and the Player Log's bag carries
  **`close=0 tick=1`**. The ✓ was always meant to be there.
  It never appeared because **`CDC::DrawIcon` was a no-op stub and `LoadIconA` returned
  NULL — so no icon anywhere in the port had ever rendered**, silently, for the whole life
  of the port. A stub that returns *success* never gets reported as a bug.
  Implemented real `RT_GROUP_ICON`→`RT_ICON` decoding (the group is a directory naming the
  image by id; `biHeight` is doubled — XOR bitmap then 1bpp AND mask, bottom-up, mask
  bit 1 = transparent). Icons live in **`Rbutton.ocx`** (828–832), not Mig.exe — the third
  instance of "inside a control, `AfxGetInstanceHandle()` is that control's own module".
  Parity **5/5 byte-identical**; the change touches only screens that call `DrawIcon`.
  **This closes the last chrome deviation on parity #15**; only the Career content table
  remains there.
- **A near-miss worth recording:** an early `ls *.ocx | head` truncated `Rbutton.ocx`
  (lowercase 'b') out of the listing and briefly established that RButton "wasn't
  installed" — which would have closed the story as *the resources don't ship*. Same family
  as the trace-cap traps: **a tool's own limit misread as evidence about the system.**
- **S68-1 — the ASan A/B was done properly this time.** Pre-S66 binary built via
  `git worktree add /tmp/ma-s65 0a69f94`, saved alongside HEAD's, and an alternating
  S65↔HEAD harness run across all four modes with `detect_stack_use_after_return=1` forced
  so drift hits both arms equally. **Result: S65 0 hits / 12 runs, HEAD 0 hits / 12 runs.**
  What that establishes is a *negative* — no detectable difference between pre-S66 and
  HEAD, so nothing supports S66 having introduced it. What it cannot establish is that the
  bug is gone: the original sighting was 2 reports in one 8-run suite and it has not
  reproduced in roughly **50 runs** since, which is entirely consistent with a ~1-in-50
  defect. **Downgraded to a standing watch item — not attributed, not fixed, not closed.**
  If `worldinc.h:257`/`:565` ever reports again it is the *second* sighting of a known
  intermittent and the log must be preserved immediately (the S66 logs were lost to the
  next run's `rm -f`, which is why there is still no stack trace).
- **S68-3 (per-face font selection) not started — third consecutive carry.** Flagging the
  pattern rather than the excuse: it is always the story that gets displaced, which means
  it is mis-prioritised, not unlucky. S69 should either schedule it first (the S66 tactic
  that worked for the font face) or drop it from the plan honestly.

### 🏃 Sprint 67 — "Attribute and trim" — ⚠️ CLOSED PARTIAL 2026-08-01 (4/8 pts)

**Sprint Review (PO pre-approved ceremony, logged 2026-08-01):** detail in
`port/scrum/sprint-67.md`. **The weakest sprint of the run, and worth saying so.**

- **S67-1 did NOT attribute the ASan finding.** A dedicated hunt (all four modes in
  rotation, `detect_stack_use_after_return=1` forced, logs preserved) produced **no
  recurrence** — but the hunt was **stopped early** (CPU contention with the sprint's own
  gate run) after only **one full rotation of 4 runs** plus part of a second, so it is a far
  weaker sample than intended and "rarer than 1 in 20" would over-read it. Honest tally:
  2 reports in the first 8 runs, then ~16–24 clean. A clean run on the *current* build
  cannot distinguish "S66 didn't cause it" from "it didn't fire" — and
  **I did not do the one thing that would attribute it**, which is to build the pre-S66
  ASan binary and run it the same number of times. Carried, with that stated explicitly
  rather than buried: S68 must either do the A/B or consciously downgrade it to a watch
  item.
- **S67-2 fixed the title bar width, and the cause was general.** Our DCs had **no clip
  region**; Windows clips a control's drawing to its own window. `CRButtonCtrl`'s picture
  path blits its DIB at natural size straight to the DC, so `IDJ_TITLE`'s ~550px art ran
  ~213px past the 336px dialog and over the map. (The control was correctly sized —
  `(333,122) 336x27` — so never a layout bug.) Added `ma_gdi_set_clip` /
  `ma_gdi_restore_clip`, honoured by `putpx`/`BitBlt`/`StretchBlt`, applied around each
  button's `OnDraw`. Parity **4/4 byte-identical** — contained.
- **`?`/`✓`: established where they are NOT.** IDD 276's template has only two items
  (1001, 1117), and the pre-clip capture shows the full 550px of title art contains no
  `?`/`✓` glyphs. So they are neither template controls nor part of that art — they come
  from RDialog chrome drawn elsewhere. A narrowed question rather than a guess.
- **S67-3 (per-face font selection) not started.**

**Retro — one embarrassing repeat.** The first attempt to measure the title button's draw
rect used a `static int n; if (n++ < 8)` cap, which the system-box buttons exhausted before
the Player Log opened. That is **exactly the S65 trace-cap trap, one sprint after writing
the lesson into the board, three cross-port notes and the shared lessons doc.** Writing a
lesson down is not the same as applying it. The concrete rule that replaces the vague one:
**filter, don't cap** — when the interesting event comes late in a run, a predicate
(`w > 300`) beats any line budget.

### 🏃 Sprint 66 — "Face it" — ⚠️ CLOSED PARTIAL 2026-08-01 (6/8 pts) — ⭐ CROSS-CUTTING #1 SOLVED

**Sprint Review (PO pre-approved ceremony, logged 2026-08-01):** detail in
`port/scrum/sprint-66.md`. **Cross-cutting deviation #1 — the front-end font/typeface, the
biggest single visual gap in the parity epic since S56 — is closed.** Colour landed in S63;
the FACE lands here.

- **The game ships its own typeface and the port was never loading it.**
  `drive_c/windows/Fonts/Intel.ttf` — *"Copyright (c) Rowan Software, 1998"* — is what gold
  renders with, and `MIG.CPP` asks for it by name. Two independent reasons it never
  arrived: `ma_gdi_font_create` **ignores the requested face outright**, and the single
  global TTF load was **rejecting Intel.ttf** — `stbtt_InitFont` accepts only platform-3
  cmap encodings 1/10, while Intel.ttf ships a **(3,0) SYMBOL** cmap, so init failed and
  every run silently fell back to a system serif. Fixed by accepting symbol cmaps and
  routing lookups through one `ma_cp()` helper (symbol tables address characters at
  `0xF000+c`).
- **Verified against gold, not merely "improved":** the title menu is
  `PREFERENCES / SINGLE PLAYER / …` in yellow **small caps** and the gold shot is the
  identical face; Preferences matches gold's yellow small-caps tab bar, blue labels and
  yellow values. Residual on those rows is now only the **BDG tab** (resource delta) and
  **combo chrome** (cross-cutting #2, now the largest remaining visual gap).
- **Scheduling it first is why it landed.** It had been planned in S64, S65 and reached in
  neither; S66 put it at the top and protected it. The cost is visible: S66-2 (title bar
  width + `?`/`✓`) was itself displaced and is the missing 2 points.
- Worth recording: this **retires the "GDI DejaVu fallback" phrasing** repeated across the
  parity doc and several cross-port notes since S56. It was accurate as a *symptom* but it
  read as the design, and for ten sprints nobody asked *why* the fallback was being taken.

**Gates.** The 2D parity sweep is a deliberate **REBASE of all 7 references, not a
byte-identical pass** — the typeface changes every screen by design, exactly as in S63;
byte-identical checking resumes in S67. Stress **PASS 20/20**.
⚠️ **`asan_all.sh` FAILED once, then PASSED — a real intermittent finding and the first
ASan report since the S15–S43 epic closed.** Two `stack-use-after-return` in the packed-item
proxy accessors (`worldinc.h:257` `T_size::operator ITEM_SIZE()`, `worldinc.h:565`
`T_shape::operator ShapeNum()`), the same MSVC-ism family as S41's. Then 4 single-mode runs
clean and a full second suite PASS — roughly **1 in ~20 runs**. **Deliberately NOT
attributed:** S66's diff is font-loading only, which makes causation implausible, but that
is not evidence and the pre-S66 ASan binary was not tested. Equally consistent with a
latent bug surfaced by changed per-frame timing. **S67's first task.**

**Retro:** the durable lesson is about language, not code. "Falls back to the DejaVu
fallback" had been written into the parity doc, three cross-port notes and several sprint
records; stating a symptom in the vocabulary of a design decision made it look settled and
stopped anyone asking the one-line question ("why is the fallback taken?") that would have
found this at any point since S56.

### 🏃 Sprint 65 — "Bags and faces" — ⚠️ CLOSED PARTIAL 2026-08-01 (6/8 pts, headline target MET)

**Sprint Review (PO pre-approved ceremony, logged 2026-08-01):** detail in
`port/scrum/sprint-65.md`. **The Player Log title bar renders** — the target that resisted
S60, S62 and S64.

- **S64's stated blocker was a trace artefact, and that is the more useful finding.** S64
  reported `ma_px_replay` "never fires for id 1001". It does — the `[px]` trace was capped
  at a fixed 60 lines and the boot path replays 58+ bags, so the Player Log's controls fell
  off the end. **An absence of trace output was read as an absence of behaviour.** Cap is
  now `MA_TRACE_PX_MAX`-tunable; raised, it shows `id=1001 len=178 ok=1 consumed=175/178` —
  a clean parse all along.
- **Nothing was ever missing.** Dumping IDD 276's bag showed it carries `IDS_PLAYERLOG`,
  the literal `Player Log`, and `FIL_TITLEB_BMP` ×2. **Two separate narrowing filters were
  each withholding half**: S58's tickbox-only caption rule, and S64's own art-name gate.
  Fixed by treating `IDJ_TITLE` (1001) as the **reserved engine id** it is — same family as
  `IDJ_TABCTRL` and `IDJ_PANEL0..9`, already special-cased in S61 — whose caption and art
  are design-time by definition. Parity sweep stayed 4/4 byte-identical, so the widening is
  contained.
- **S65-2 tested and REJECTED a general criterion**, which is worth as much as adopting one:
  template membership does *not* separate the system-box "Quit"/"Size" buttons from
  legitimate controls — they are `inTmpl=1` too. So `MA_BTN_ART_NAMES` stays a blanket
  opt-in and only the single reserved id was widened. Recorded so it is not re-tried.
- **S65-3 (font FACE) was not started** — ran out of sprint, carried whole. That is the
  2 points missing, and it is the third sprint running that cross-cutting #1's remaining
  half has been planned and not reached.

**Gates.** Parity **4/4 byte-identical** (`map_playerlog` re-based for the new title bar;
`prefs_controls` excluded as environment-dependent). ASan and stress per the gate log.

**Retro:** applying S64's own retro worked — scoping this as an investigation with a visual
stretch got the visual result that three rendering-scoped attempts missed. The new lesson
is sharper though: **a capped debug trace produced a confident, wrong root cause** that
went into a sprint record and a cross-port note. Traces that gate conclusions need their
caps visible or tunable, and "no output" must be verified as "no behaviour" before it is
written down.

### 🏃 Sprint 64 — "Face value" — ⚠️ CLOSED PARTIAL 2026-08-01 (6/8 pts)

**Sprint Review (PO pre-approved ceremony, logged 2026-08-01):** detail in
`port/scrum/sprint-64.md`.

- **S64-1 — the answer was "there is no bug", which is the story's value.** S63 recorded a
  residual that native "renders LARGER than gold". **Measured: it does not.** Gold's label
  glyph band is 10 px vs native's 11 px; row pitch 52 px vs 51 px — *the same absolute font
  size*. S63 had compared a **1280×1003** gold shot against an **800×600** native capture
  and read the density difference as a font defect, despite this doc's own header warning
  about exactly that. Corrected, and the caveat promoted from a header line to an explicit
  rule: **the gold set is ~1280×1024 and native front-end captures are 800×600 because the
  game picks its panel ART SET by resolution**, so no verdict may rest on relative size or
  density. Cross-cutting #1's residual is re-scoped to **font FACE only**. S65 would
  otherwise have spent a sprint chasing a font-scaling bug that does not exist.
- **S64-2 — two real bugs fixed; the target still not met.** `GetFileNum(name)` was a
  **stub returning 0** — the resolver the R* string-file setters depend on, i.e. the sound
  half of BoB's trap 2 — so **every control whose art is named rather than numbered
  silently lost its artwork**; it now resolves against the `F_GRAFIX.G` table. And
  `CString(LPCWSTR)` was **declared but never defined**, failing at link time only, so the
  gap was invisible until something read a BSTR back. **The art-name application itself is
  implemented but shipped OFF** (`MA_BTN_ART_NAMES=1`): applying it to every button
  regressed the sweep by materialising the invisible system-box "Quit"/"Size" buttons in
  every screen's top-left — *exactly* S58's documented failure when it narrowed the caption
  path to tickbox-class buttons. Caught by the sweep, not by eye.
  **The title bar still does not render**, but the blocker is now precisely located:
  `ma_px_replay` **never fires for id 1001**, so IDD 276's bag for `IDJ_TITLE` is not
  reaching `bagmap` even though its DLGINIT stream demonstrably starts with `e9 03`.
  A bag-keying question, not an art question.
  *Worth naming: this target has resisted S60, S62 and S64. Each attempt removed a real
  obstacle, but it keeps being scoped as a rendering story when the blocker has every time
  been a layer underneath.*
- **S64-3 — `prefs_controls` is not a stable oracle.** It embeds live joystick state
  (S62 saw "NOT CONNECTED"; S64 sees a Logitech Extreme 3D again). Excluded from the
  byte-identical sweep as environment-dependent; the other five screens carry the gate.

**Gates.** **2D parity sweep 5/5 byte-identical — the byte-identical check RESUMED this
sprint** against S63's re-based references, as promised, and it is what caught the
system-box regression and then confirmed its removal. `asan_all.sh` PASS 4/4 modes, 0
reports. `stress_launch.sh` PASS 20/20.

**Retro:** two of this sprint's three findings were *deletions* — a residual that wasn't
real, and a fix that had to be switched back off. Both came from measuring rather than
reasoning, and both were cheap because the parity sweep is fast. The estimate missed
because S64-2 was scoped as "render the title bar" when every previous attempt had shown
the blocker to be a layer down; a story whose acceptance is visual but whose history is
infrastructural should be planned as an investigation with a visual *stretch*.

### 🏃 Sprint 63 — "Switch the properties on" — ✅ CLOSED 2026-08-01 (8/8 pts, goal MET)

**Sprint Review (PO pre-approved ceremony, logged 2026-08-01):** detail in
`port/scrum/sprint-63.md`. **Both S62 blockers cleared; the persisted-property reader is ON
by default; the COLOUR half of cross-cutting deviation #1 is solved.**

- **S63-1 — the uninit garbage, root-caused.** Trapped the draw rather than reasoning about
  the reader: a gated non-ASCII text-draw hook plus gdb named the caller immediately.
  **`WM_GETSTRING` is an IN/OUT convention and three R* sites ignore the OUT half** —
  `CRButtonCtrl::GetParentWndInfo` (×2) and `CRStaticCtrl` (×1) assign the buffer without
  checking the returned length. `workspace[0]=99` is the IN capacity; when no parent routes
  `WM_GETSTRING`, `SendMessage` returns 0 having written nothing, leaving `'c'` (**0x63 —
  exactly the first garbage byte**) + uninitialised stack as the caption. Latent for the
  port's whole life because `m_ResourceNumber` was always the ctor default 0. Fixed at all
  three sites; zero non-ASCII draws, runs byte-identical.
- **S63-2 — recipes made font-independent, the durable fix rather than new constants.**
  `f,rN` (menu row) and `f,#ID[:COL]` (control by dialog id; column via `GetColFromX`).
  Two findings worth keeping: **`GetRowFromY` is unusable as the oracle** (its
  `m_playerList` clamp answers −1 past row 0), and the Load Game **"Back Load" bar is one
  horizontal listbox (id 2063), not two buttons** — which is why the campaign recipe still
  failed after the rows were fixed. Validated by reproducing the old hand-derived constants
  with the reader off (233 vs 231; 217 vs 217).
- **S63-3 — reader ON by default.** Values yellow **matching gold exactly**, tab bar
  yellow, labels into gold's blue family (gold's own `(103,132,198)` present natively),
  title menu yellow with its black box gone. **Residual renamed and narrowed: font FACE and
  SIZE** — native still uses the DejaVu fallback and renders *larger* than gold, loosening
  row density; labels read brighter cyan than gold's muted blue. Named, not folded into
  "solved".
- **S63-4 — note 20 delivered** (owed from S62, whose close story never landed) + lessons
  §8i, both copies md5-identical.

**Gates.** ASan and stress on the default (reader-ON) path through the migrated recipes.
**The 2D parity sweep is deliberately NOT byte-identical this sprint** — the reader changes
fonts and colours by design, so all six references were regenerated; the byte-identical
check resumes in S64 against the new baselines. That is a rebase, not a pass, and is
recorded as such.

**Oracle provenance:** the `BEA6-BBCE` gold USB was **not mounted**; all 14 gold shots are
mirrored at `/home/admin/gold standard/ma/` and that mirror was used — recorded in the
parity doc so these verdicts are not ambiguous later.

**Retro:** the sprint's leverage came from fixing the *class* rather than the instance —
resolving clicks through the controls' own metrics means the next font change cannot break
the gate, whereas re-deriving pitch constants would have bought exactly one sprint. And the
run-to-run-variance question found the uninit bug in minutes for the third sprint running;
it is now the first thing to ask.

### Sprint 63 planning — "Switch the properties on" — PLANNED 2026-08-01 (PO pre-approved ceremonies)

**Environment:** session UNLOCKED, no stray `wmig`, build current.

**Tree state needed sorting out before planning.** The working tree held an **uncommitted
full revert of Sprint 62** (−475 lines + deletion of `port/scrum/sprint-62.md` and the S62
board entry) that no session in this conversation's history made. It was verified
byte-identical to the pre-S62 commit `f40a9ee` — zero unique information, fully
reproducible from history — so restoring HEAD destroyed nothing; it was preserved in a
stash anyway rather than discarded, and HEAD was confirmed to build with its default path
still byte-identical to the `title` reference before any work began.

**Context:** S62 closed 5/8. The property-stream reader is built, parses all 58 boot-path
bags clean, and its payoff is gold-verified (Preferences → blue labels + yellow values,
solving the **colour half of cross-cutting deviation #1**) — but it ships OPT-IN behind
two blockers: an uninitialised read that paints varying garbage at the title screen's
top-left, and a persisted FontNum that changes the title menu's row pitch (~16→28px),
invalidating every fixed-coordinate `BOB_CLICKSEQ` — the parity capture recipes *and*
`asan_all.sh`'s drive recipes, i.e. the regression gate itself.

**Sprint Goal:** clear both blockers, switch the reader on by default, and re-verdict the
parity set.

**Committed (~8 pts):**
| Story | Pts | Definition |
|---|---|---|
| S63-1 Root-cause the uninit garbage | 3 | Root-caused and fixed; title capture clean at 6× contrast |
| S63-2 Font-independent click recipes | 3 | Click-by-menu-row-index helper resolves rows at runtime, so a font/pitch change can never invalidate the gate again |
| S63-3 Reader ON by default + re-verdict | 1 | Default-on (`MA_NO_DLGINIT_PROPS=1` escape retained); parity set re-captured and re-verdicted |
| S63-4 Cross-port note 20 + close | 1 | Note 20 **owed from S62** (its close story did not land) + S63 findings; docs md5-identical; gates |

Board: `port/scrum/sprint-63.md`. **NOT pulled:** Player Log title bar + `?`/`✓`; Career
content table; RScrlBar hosting; `ma_tabs_hit` click routing; #12 debrief capture.
**Order is deliberate:** S63-2 lands before S63-3 — the gate must be trustworthy *before*
the default flips, which was S62's own reasoning for shipping opt-in.

### 🏃 Sprint 62 — "Design-time properties arrive" — ⚠️ CLOSED PARTIAL 2026-08-01 (5/8 pts)

**Sprint Review (PO pre-approved ceremony, logged 2026-08-01):** detail in
`port/scrum/sprint-62.md`. **The reader is built, correct and measured — but it ships
OPT-IN (`MA_DLGINIT_PROPS=1`), not on by default, so the goal is half met.** The Player Log
title bar was not verified. Switching it off is a deliberate call with two measured
reasons, not an oversight.

- **The adoption worked.** BoB's `CPropExchange` lifted essentially verbatim, bag storage
  ported onto MA's existing RT_DLGINIT walk. **All 58 bags on the boot path parse clean**
  (`ok=1`, persisted version read, ≤8 bytes of the editor slop BoB documents). Their
  1280-bag validation transfers.
- **The payoff is real and gold-verified.** With the reader on, Preferences goes from the
  white-serif labels of cross-cutting deviation #1 to **blue labels + yellow values** —
  gold's scheme, sampled against the original gold PNG. The title menu turns yellow,
  likewise matching. **The colour half of cross-cutting #1 is solved**; the font *face*
  half remains. This is BoB's "13 of 14 screens snapped toward gold" reproducing on MA.
- **Two MA-specific divergences, both found by tracing rather than reasoning:** stock
  **Caption** is consumed but not applied (MA's persisted captions are `IDS_*` SYMBOL
  NAMES — S57 already resolves those to the shipped wording, so applying the raw value
  would overwrite a correct caption with a symbol name); stock **BackColor** likewise
  (MA composites over panel art). BoB's trap 1 deliberately skipped — MA's `OLE_COLOR` is
  already COLORREF end to end, so converting would *be* the double-conversion it warns of.
- **Why it ships off — two blockers:** (1) an **uninitialised read** surfaces as garbage
  text at the title screen's top-left; it **varies between runs**, the tell from S61's
  lesson, and is absent from the S61 reference at 6× contrast. Bisected far enough to
  exonerate the stock caption and `PX_String`; not root-caused. (2) the persisted FontNum
  **changes the title-menu row pitch (~16px → ~28px), so every fixed-coordinate
  `BOB_CLICKSEQ` recipe lands on the wrong row** — `quickmission` came back showing
  Preferences, and the campaign recipe never reaches the map. That invalidates the parity
  capture recipes *and* `asan_all.sh`'s drive recipes together, i.e. the regression gate,
  precisely when the diff is largest. Opt-in keeps the default byte-identical and the gate
  trustworthy.

**Gates.** 2D parity sweep **6/6 unregressed on the default path** — five byte-identical;
`prefs_controls` differs **environmentally, not from code**: that screen enumerates live
hardware and its reference was captured with a joystick attached, while this box now has
none (`/dev/input/js*` absent). Recorded in the parity doc — **that reference embeds
machine state and is not a stable oracle**, the S59 device-presence lesson one level out.
`asan_all.sh` **PASS 4/4 modes, 0 reports**. `stress_launch.sh` **PASS 20/20**.

**Retro:** adopting beat deriving — note 17's write-up plus BoB's source turned a
5-pt build into a working reader quickly, and their 1280-bag validation meant the parse
needed no debugging at all. The judgement call was what to do when a correct component has
unacceptable blast radius: shipping it off, with the payoff measured and the two blockers
named, keeps both the increment and the gate. The cost is honest — the sprint goal is half
met and S63 inherits well-defined work.

### Sprint 62 planning — "Design-time properties arrive" — PLANNED 2026-08-01 (PO pre-approved ceremonies)

**Environment check at planning:** session UNLOCKED, no stray `wmig`, build current, tree
clean at `f40a9ee`.

**Context:** S61 left one named blocker, and it is the same component behind the biggest
remaining parity gap. Every hosted R* control still boots from an **empty**
`CPropExchange` (MA's `PX_*` are all `{ return TRUE; }`), so all design-time properties
are lost. BoB landed the real reader in their S126 and validated the stream layout against
**all 1280 R\*-class RT240 bags, zero parse failures** (note 17 §3, lessons §8f). Note 19
asked whether it could be lifted; reading their source, it can — their `CPropExchange` is
~70 self-contained lines and their bag storage ports straight onto MA's existing
RT_DLGINIT parser.

**Sprint Goal:** hosted controls boot with their genuine design-time properties — which
lights up the Player Log title bar and the FONT/COLOR set behind cross-cutting #1.

**Committed (~8 pts):**
| Story | Pts | Definition |
|---|---|---|
| S62-1 Adopt the property-stream reader | 5 | Raw bag storage; real `CPropExchange` (licence → version → extents → stockPropMask → PX fields in source order); real `PX_*`; every hosted-control path replays its bag. BoB's 3 traps applied. `MA_NO_DLGINIT_PROPS=1` reverts |
| S62-2 Payoff: title bar + parity re-verdicts | 2 | "PLAYER LOG" title bar renders; parity set re-captured, moved verdicts re-stated |
| S62-3 Cross-port note 20 + close | 1 | Note 20 to `bob/doc/`; docs md5-identical; board/burndown/parity/RUNNING/rollup; gates |

Board: `port/scrum/sprint-62.md`. **NOT pulled:** Career content table, RScrlBar hosting,
real mouse clicks → `ma_tabs_hit`, #12 debrief capture.
**Note on S58/S59 interaction:** MA fixed the uninit-PX class with shape (a) (ctor-init);
BoB used shape (b) (default-writing exchange). (a) composes with a real reader and is
strictly safer — the ctor default is the fallback the reader overwrites. Keep the inits.

### 🏃 Sprint 61 — "The Player Log lands" — ✅ CLOSED 2026-08-01 (7/8 pts, goal MET)

**Sprint Review (PO pre-approved ceremony, logged 2026-08-01):** detail in
`port/scrum/sprint-61.md`. **The sprint goal was met** — the Player Log's tab bar renders
with the real RTabs.ocx art, the dialog is centred over the map, and tab switching is
capture-proven. **#15 PARTIAL → CLOSE-minus.**

Four distinct defects, all presenting as "the dialog draws in the wrong place":
1. **`borderwidth` was uninitialised heap.** `MIG.CPP`'s startup reads WindowMetrics and
   **never checks `RegQueryValueEx`'s return**; the compat stub writes neither `type` nor
   `buff`, so both were read from uninitialised stack. It feeds `MakeParentDialog`'s
   sizing → **a different garbage dialog origin on every run** ((978990,978859), then
   (979004,978793)). That run-to-run variance is what identified it.
2. **`OnGetXYOffset` was built on no-op `ClientToScreen`** — every subtraction `0 - 0`, so
   every dialog reported offset ~0 and the whole tree composited at the top-left.
   Replaced with the accumulated parent-chain origin, deliberately *not* by giving
   `ClientToScreen` global semantics (S60's lesson).
3. **`IDJ_PANEL0..9` placeholders were never registered** → `AddChildren` fell through to
   its "stack below the parent" branch, putting the tab box at y=396 on a 400px dialog.
   They are plain *native* template controls, so S60's OCX-kind hosting missed them.
4. **The title-height nudge double-counted**, and being gated on `artnum == artnum` it hit
   the art-less tab host but not the art-bearing page — 27px out of step, so the page art
   painted over the tab strip. Only the top 3px of the tab bar had been surviving.

**S60's scoping lesson applied prospectively, and it paid.** Fix (1) needs the view rect,
so `MakeParentDialog` syncs `m_pView` from the canvas — but left installed that changed
the campaign map (its tile loop reads the view rect; one extra tile row straddled the
bottom edge, and our auto-growing canvas turned the capture into 1021×**900**). Wrapped in
an RAII scope that restores the rect. Caught by the parity sweep, not by eye.

**Not done:** the "PLAYER LOG" title bar and the `?`/`✓` buttons. Precisely diagnosed
rather than left vague: `IDJ_TITLE` (1001) **is** in the template, **is** hosted (S61
exempts it from the caption-less skip rule) and is **not** filtered — but its art and
caption live in its **RT_DLGINIT property stream** (`idd=276 sz=188`, first id `e9 03` =
1001), which MA does not yet parse. That is BoB note 17 traps 1/2, the R* property-stream
reader — a component, not a fix. **It is now the top backlog item** (it also unlocks the
FONT/COLOR set behind cross-cutting deviation #1).

**Gates — all green.** 2D parity sweep **5/5 byte-identical**, now including
`campaign_map` (added this sprint because the view-rect change touches map drawing — and
it is exactly what caught the 900px canvas regression). `asan_all.sh` **PASS 4/4 modes, 0
reports**. `stress_launch.sh` **PASS 20/20**.

**Retro:** measuring instead of reasoning was decisive twice — the run-to-run variance in
the garbage origin identified defect (1) immediately, and a 3px sliver of tab bar in a
capture identified the 27px double-count. The estimate held this time because planning
spent its first move re-tracing S60's closing suspicion (`artnum == artnum`) and found it
was a red herring before any code was written.

### Sprint 61 planning — "The Player Log lands" — PLANNED 2026-08-01 (PO pre-approved ceremonies)

**Environment check at planning:** session **UNLOCKED**, no stray `wmig`, build current,
tree clean at `cdccb99`.

**Context:** S60 closed partial — the tab bar and title bar are created, populated, sized
and drawn, but land at the wrong place. Planning went straight at that and found the
cause: **`CWnd::ClientToScreen`/`ScreenToClient` are complete no-ops** (`afxwin.h:690-693`),
and `RDialog::OnGetXYOffset` is built entirely on them, so it computes `0 - 0 = 0` for
every node. The whole dialog tree composites at the top-left — which is both #15's
"not centred" deviation and why the tab bar is invisible (drawn at (0,0), then covered by
the Career page's art). S60's parting suspicion (the `artnum == artnum` parent-walk) is a
red herring: the arithmetic is degenerate regardless.

**Sprint Goal:** the Player Log's tab bar and title bar composite where they belong, and
#15 gets an honest re-verdict.

**Committed (~8 pts):**
| Story | Pts | Definition |
|---|---|---|
| S61-1 Dialog trees get real screen origins | 3 | `OnGetXYOffset` returns each node's true absolute origin; `RDEmptyD`'s garbage origin gone. Acceptance: tab bar + title bar **visible** in `map_playerlog`, front-end captures still byte-identical |
| S61-2 Re-capture + #15 verdict flip | 2 | Re-capture, side-by-side vs gold #15, parity table + I4 status updated |
| S61-3 Tab selection + `?`/`✓` title buttons | 2 | Tab clicks → `ma_tabs_hit` → `SelectTab` → `WM_SELECTTAB`; `?`/`✓` identified in IDD 276 |
| S61-4 Cross-port note 19 + close | 1 | Note 19 to `bob/doc/` (shared-engine finding); docs md5-identical; board/burndown/parity/RUNNING/rollup; gates |

Board: `port/scrum/sprint-61.md`. **NOT pulled:** Career content table (other half of I4),
RScrlBar hosting, #12 debrief capture, cross-cutting font/chrome.
**Risk noted at planning:** making `ClientToScreen` real is a global change to a function
the front-end panels also call — S60 was burned by exactly that shape. Prefer the scoped
fix unless the global one measures clean; the 4× byte-identical parity sweep gates it.

### 🏃 Sprint 60 — "The Player Log opens" — ⚠️ CLOSED PARTIAL 2026-08-01 (5/8 pts)

**Sprint Review (PO pre-approved ceremony, logged 2026-08-01):** detail in
`port/scrum/sprint-60.md`. **The sprint goal was NOT met: the tab bar still does not
appear, so I4/#15 stays PARTIAL.** What did land is the machinery under it, and two
engine-level root causes that had been mis-scoped as four separate screen bugs.

- **The planning hypothesis was wrong, and the trace said so on run one.** Planning
  assumed "RTabs is unhosted → `GetDlgItem(IDJ_TABCTRL)` returns NULL". In fact **RTabs
  was never CREATED** — zero `4a1e1986` in a full-path OCX trace. Real root cause:
  **template-declared OCX controls that no dialog class `DDX_Control`-binds were never
  instantiated.** S57 had fixed exactly this class *for RStatic only*; S60 makes it
  kind-driven (`ma_host_template_controls`: RStatic + RButton + RTabs). IDD 276's
  `IDJ_TITLE` is an **RButton** — a coclass hosted since Phase 4, missing only because
  nothing created it from the template.
- **★ The deeper find (S60-3): no RDialog in a dialog tree ever learned its own size.**
  The ctor zeroes `homesize`/`viewsize` and the line that would refresh them from the
  client rect is commented out (`RDIALOG.CPP:147`). So `MakeParentDialog` placed trees off
  a 0×0 client rect, `AddChildren` sized children from `homesize.Width()==0`, and
  `RDialog::OnSize` handed `IDJ_TABCTRL` a **zero-width** `MoveWindow` that the draw loop
  then skipped. Fixed with `RDialog::MaSeedTemplateSize()` + a new `ma_dlg_own_size()`
  (the parser had read the template cx/cy since S59 but never exported it). Measured: tab
  host 0×0 → 420×258, CPlyr_log → 336×396.
- **A regression I caused and backed out, on the record:** seeding the size in
  `CDialog::Create` (every dialog) broke the front end — canvas 644→600, Load-panel art
  bleeding into the map. Re-scoped to the three tree-builder `Create` sites. The comment
  at the fix says why, so it does not get "cleaned up" into `Create` later.
- **Verified working:** all three tabs register with the gold captions
  (`AddTab "Career"/"Log of Missions"/"Last Mission"`), and the real **297×31 tab art
  loads from RTabs.ocx's own PE resources** (an OCX's bitmaps are in the OCX, not Mig.exe —
  new lesson). **Unplanned bonus:** the Career tab's **"Name" label and Name edit box** now
  render — content #15 listed as missing and that S60 had explicitly not pulled.
- **Not met:** tab bar + title bar are created, populated, sized and drawn, but land at the
  wrong screen offset. Prime suspect named for S61: `RDialog::OnGetXYOffset` only
  accumulates an offset when `parent->artnum == artnum`, and every node in this tree is
  `artnum == 0` except the tab pages.

**Gates — all green.** **2D parity regression sweep CLEAN: `title`, `prefs_3d`,
`prefs_controls`, `quickmission` re-captured and byte-identical (0 changed px) to their
committed references** — the check that mattered, since the diff touches `afxwin.h`,
`RDIALOG.CPP` and `MIG.CPP`. `port/asan_all.sh` **PASS 4/4 modes** (2 runs each, 0
reports; ASan build relinked with the new `rtabs` objects — a link error there caught that
`rtabs` was missing from `port/rebuild.sh`, only added to CMake). `port/stress_launch.sh`
**PASS 20/20**, and its `WMIG` now defaults to the ninja artifact.

**Retro:** the sprint's two best moves were (a) tracing before coding — the planning
hypothesis would have produced a correct-but-inert change, and (b) diffing the untouched
parity captures against their references, which is what proved a scary-looking
`afxwin.h`/`RDIALOG.CPP` diff safe and, one iteration earlier, caught the `CDialog::Create`
regression immediately. The estimate was the failure: 3 pts assumed "host a control the
way we've hosted five others", but the story sat on top of two unfixed engine gaps. When a
story's acceptance is "a thing appears on screen", the geometry path is part of the story.

### Sprint 60 planning — "The Player Log opens" — PLANNED 2026-08-01 (PO pre-approved ceremonies)

**Environment check at planning:** desktop session **UNLOCKED** (`LockedHint=no`,
`ScreenSaver GetActive=false`), `glxinfo -B` direct rendering, no stray `wmig` on the
flock. **First action was to clear S59's deferred stress gate: PASS 20/20** — so S59 is
green across all gates and carries nothing in. Build current (`ninja: no work to do`).

**Context:** the Quick Mission screen settled in S59 (#9 → CLOSE-minus), which frees the
parity queue for I4 — the Player Log, deferred as "a full sprint on its own" for three
sprints running (S58, S59 both explicitly declined it). Planning read of
`MAINTBAR.CPP:315` shows all four of #15's named deviations descend from ONE structural
gap: the `HTabBox` arm of the dialog tree, whose `RDialog::AddChildren` variant
(`RDIALOG.CPP:612`) needs a real `CRTabs` at `IDJ_TABCTRL` (1002) — an OCX the host has
never hosted. `SRC/RTABS/` is a complete control tree with a real `CRTabsCtrl::OnDraw`,
so this is the same reuse pattern already proven on five R* controls.

**Sprint Goal:** the Player Log becomes a real tabbed dialog — RTabs hosted, the
Career / Log of Missions / Last Mission bar rendering, a proper frame + "PLAYER LOG"
title bar, placed where gold puts it — all held to the dummy==GL byte-identical bar.

**Committed (~8 pts):**
| Story | Pts | Definition |
|---|---|---|
| S60-1 Host the RTabs OCX (CRTabs) | 3 | CLSID `0x4a1e1986` → `ma_oletabs.cpp` reusing real `CRTabsCtrl::OnDraw`; build mode `rtabs`; `GetDlgItem(IDJ_TABCTRL)` real so `AddChildren`'s `SetHorzAlign`/`AttachTabToTabControl` run; tab bar in capture |
| S60-2 Player Log frame + title bar | 2 | `CPlyr_log` (IDD 276) frame + "PLAYER LOG" title bar + ?/✓ buttons render |
| S60-3 Dialog placement honoured | 2 | `MakeTopDialog(Place(x,y),…)`/`Edges` routed — dialog no longer pinned top-left |
| S60-4 Cross-port note 18 + close | 1 | MA note 18 to `bob/doc/` (same R* family — directly reusable); shared docs md5-identical; board/burndown/parity updated; `stress_launch.sh` `WMIG` default → ninja artifact |

Board: `port/scrum/sprint-60.md`. **NOT pulled:** the Career tab's content (Name edit +
Sorties/Combats/Kills/Losses table) — the other half of I4, → S61; RRadio OCX hosting;
#12 debrief capture; cross-cutting font/chrome.

### 🏃 Sprint 59 — "Quick Mission settles" — ✅ CLOSED 2026-07-27 (all gates green; stress gate cleared 2026-08-01)

**Sprint Review (PO pre-approved ceremony, logged 2026-07-27):** all 4 stories DONE, 8/8 pts
(detail: `port/scrum/sprint-59.md`). Headlines: #9 root-caused — the "stray combo" is the
dead-coded Cloud/Weather cluster parked OUTSIDE the 335-dlu dialog (Windows parent-rect
clipping, now routed via `ma_dlg_never_visible`); phantom "I.D." label was a `!WS_VISIBLE`
template control (style dword now parsed → initial show state); mission text word-wraps
(compat `CDC::DrawText` implements `DT_WORDBREAK`); uninit-PX ctor audit widened to
RSTATIC/RBUTTON/RCOMBO/REDTBT; the cmp bar caught a SECOND environment-dependence class
(DI mouse presence gated on the window — now unconditional). #9 → CLOSE-minus (RRadio row
remains, OCX not hosted). Verdict refs refreshed #3/#4/#5/#7/#9. Notes 17 exchanged both
directions; shared doc md5-identical (`d71c0db3…`).

**Gates:** build clean (regular + ASan). `port/asan_all.sh` **PASS — 4/4 modes**
(flight/camp-map/camp-fly/camp-nextday, 2/2 runs each, 0 ASan reports), run synchronously
in one-mode chunks after the session-limit interruption. `port/stress_launch.sh`
**PASS — 20/20 OK** (cleared 2026-08-01, see below). It was DEFERRED at close because the
desktop session had locked mid-day (`LockedHint=yes`); a locked session never presents new
GL windows → the swapchain fills after 3 frames and SwapBuffers blocks in a GPU sync wait
(strace: `DRM_IOCTL_SYNCOBJ_TIMELINE_WAIT`) → all 20 runs HANG at the title. NOT a code
regression: the identical binary reached 3D headless (ASan flight 2/2), and the S59 diff's
only GL-path-adjacent change left GL behavior identical to S58.

> **Deferred gate CLEARED 2026-08-01** (S60 planning, unlocked session, `LockedHint=no` +
> `ScreenSaver GetActive=false`): `WMIG=build/wmig flock /home/admin/.gl-display.lock -c
> 'bash port/stress_launch.sh'` → **PASS: 20/20 reached & sustained 100 3D frames**, tally
> `OK 20`, zero SEGV/FPE/ABORT/NO3D/HANG. Same commit (`993dafc`), same binary that scored
> 20/20 HANG while locked — the environmental diagnosis is confirmed and the S59 diff is
> fully exonerated. S59 now closes with **all gates green**.

**Retro:** the sprint survived a session-limit kill mid-gates because everything was
board-logged before the gates — keep gate runs last and chunked. New env gotcha logged:
GL gates require an UNLOCKED display session; probe `loginctl … LockedHint` before
blaming code for title-screen hangs.

**Interruption note:** sprint executed by one agent to story-complete (killed by session
limit mid-ASan), salvage-committed as `3d1d94c`, gates finished + closed by the PO session.

### Sprint 59 planning — PLANNED 2026-07-27 (PO pre-approved ceremonies)
**Context:** BoB note 17 delivered at sprint start (full property-stream layout; COLORREF
convert-once; art FileNums are authoring-install indices; settled-state erase emulation
for covered template controls; BoB passed the dummy==GL cmp bar first try). S58 carry-over
queue: #9 stray-combo hide mechanism, mission-text word-wrap, #12 debrief capture, I4,
R* uninit-PX audit.
**Sprint Goal:** the S58 carry-overs on the Quick Mission screen close — #9's runtime-hide
mechanism root-caused (note-17 trap-3 settled-state hypothesis first) and mission text
wraps — and the uninit-PX net widens to every hosted R* control, held to the dummy==GL bar.
**Committed (~8 pts):**
| Story | Pts | Definition |
|---|---|---|
| S59-1 Note 17 inbound + R* uninit-PX ctor audit | 2 | Note 17 inbound-committed/processed; RStatic/RButton/RCombo/REdtBt ctors init all PX-persisted members (S58 pattern); cmp bar holds |
| S59-2 #9 stray-combo hide mechanism | 3 | Identify the ~(590,165) control; root-cause + route/emulate the Windows settled-screen hide; re-capture; parity table updated |
| S59-3 Mission-text word-wrap | 2 | Mission text wraps within the Quick Mission panel; re-capture; parity table updated |
| S59-4 Cross-port note 17 reply + close | 1 | MA note 17 to `bob/doc/`; shared-doc copies stay md5-identical; board+burndown updated |
Board: `port/scrum/sprint-59.md`. NOT pulled: #12 debrief capture, I4 Player Log (8 pts, full-sprint item), RRadio hosting.

### 🏃 Sprint 58 — "Capture the proof" — ✅ CLOSED 2026-07-27

**Sprint Review (PO pre-approved ceremony, logged 2026-07-27):**
- **All 3 committed stories DONE (8/8 pts).** Demoed via captures + gate logs:
  - **S58-1 (3)**: `MA_SHOT` GL-free 2D capture path landed artifact-free. The salvage's
    "membership filter" diagnosis was WRONG — root cause was **uninitialised
    `DoPropExchange`-only members** (compat `PX_*` no-ops don't write defaults) in
    `CRListBoxCtrl` → environment-dependent heap garbage (black tab-bar band + clipped
    rows headless; doubled title-menu captions). Ctor now inits all persisted members to
    PX defaults. **Dummy-run canvas byte-identical (`cmp`) to GL-run canvas** — adopted
    as the standing capture acceptance bar.
  - **S58-2 (3)**: verdicts flipped on real captures — **#7 prefs Controls
    PARTIAL→CLOSE, #8 prefs Others PARTIAL→CLOSE, #1 title first-captured→CLOSE**
    (every S57 fix verified in-capture: labels, gold IDS wording, Calibrate/REdtBt,
    DI axis names incl. live Logitech Extreme 3D, tickbox art). #2–#6/#9/#13
    re-captured (verdicts unchanged; `port/ref/native/` refreshed). Gates:
    **`asan_all.sh` PASS** (0 reports, 4/4 paths reached, 2 runs/mode; ASan build
    rebuilt with the fix) + **stress 8/8 OK** (100-frame 3D sustain). Gate hardening:
    `asan_all.sh` timeout now `-k 5 -s KILL` (an ASan run ignored SIGTERM and wedged the
    suite).
  - **S58-3 (2)**: BoB note 16 processed (S125 bag-layout slices checked — MA's
    candidates are runtime-populated, no symptom, not adopted; caveat already applied in
    salvage). **MA note 16 sent** (PX-defaults trap + byte-identical acceptance bar +
    #9 filter-hypothesis post-mortem); §8f addendum "PX defaults are load-bearing" —
    both shared-doc copies byte-identical (md5-verified).
- **Carry-over:** #9 stray combo (in installed template, runtime-hidden on Windows —
  hide mechanism unrouted); mission-text word-wrap; #12 debrief capture; I4 Player Log
  (8 pts, next-sprint candidate); audit other R* controls for the same uninit-PX class.
- **Retro (one line):** trust traces over inherited hypotheses — the salvage's "filter"
  diagnosis cost nothing because we re-verified it first (zero `[filter-skip]` hits)
  before touching the filter; and a byte-identity bar between capture paths finds bug
  classes that eyeballing never will.

*(Original planning entry below, kept for the record.)*
### Sprint 58 planning — PLANNED 2026-07-26 (PO pre-approved ceremonies)
**Context:** S58 was previously interrupted mid-sprint (session limit); WIP salvaged in
`53554d4` with a KNOWN OPEN ISSUE (strip artifact in MA_SHOT captures — membership filter
skipping a load-bearing control). This sprint re-plans S58 properly and closes it.
**Environment check at planning:** the S57 machine-wide GLX wedge is HEALED
(`glxinfo -B` OK, NVIDIA direct rendering) → the S57 carry-over (re-captures, gates) is unblocked.
**Sprint Goal:** the S57 parity fixes become *proven* — the GL-free MA_SHOT capture path
works artifact-free, the parity verdicts for #7/#8/#9 flip on real re-captures, and the
regression gates run again.
**Committed (~8 pts):**
| Story | Pts | Definition |
|---|---|---|
| S58-1 MA_SHOT capture path done | 3 | Root-cause + fix the strip artifact (membership filter vs load-bearing control); GL-free 2D capture verified against the GL capture path |
| S58-2 I2 verdict flips + gates (S57 carry-over) | 3 | Re-capture #7 Controls / #8 Others / #9 Quick Mission (+#2–#6 incidentals); flip verdicts in `port/scrum/screen-parity.md`; `asan_all.sh` + stress gate PASS |
| S58-3 Cross-port note 16 processing + MA note 16 reply | 2 | Apply note 16's bag-layout lesson where MA shows the symptom class; reply as MA note 16 in BOTH shared-doc copies (byte-identical) |
Board: `port/scrum/sprint-58.md`. I4 (8 pts) again NOT pulled — it alone would fill a sprint.

### 🏃 Sprint 1 — "Dependable launch" (R1) — ✅ PLANNED & RATIFIED (PO, 2026-06-17)
**Sprint Goal:** The game launches to 3D flight reliably and remembers my settings.
**Committed (capacity ≈ 20 pts, 21 committed):** A1 + A2 + A4. Live board: `port/scrum/sprint-01.md`.

| ID | Story | Pts | Tasks |
|---|---|---|---|
| A1 | Harden intermittent 3D launch | 13 | (1) Instrument sim-thread vs main-thread startup order under `MA_TRACE_3D`. (2) Add barrier so `DoMoveCycle`/`body2screen` never run before `MakePassive` finishes surface+matrix init. (3) Guard `View3d::Drawing()` gate. (4) Run 20× stress-launch; log pass rate. |
| A2 | Persist `Save_Data` to disk | 5 | (1) Locate writeback path. (2) Serialize on clean exit + on panel `PreDestroyPanel`. (3) Reload on boot. (4) Round-trip test (change Gamma → restart → still High). |
| A4 | Stress-launch harness | 3 | `port/stress_launch.sh` looping `wmig` with timeout + crash classification. |

**Increment:** A build that boots to a flyable 3D frame on every launch and persists Preferences. Demoable.

### Sprint 2 — "Front-end finished" (R1→R2 bridge)
Stories: F2 (combo dropdown, 5), F3 (resolutions enumeration, 5), C3 (full menu mouse coverage, 5), F4 start (13, may split).
**Increment:** Complete, navigable native front-end across all settings screens.

### Sprint 3 — "Hands on the stick" (R2)
Stories: C1 (DirectInput→SDL flight controls, 13), B3 (smooth animated 3D, 8 — finish).
**Increment:** Fly the aircraft with the keyboard in a live 3D view.

### Sprint 4 — "Looks right" (R2)
Stories: B2 (A/B fidelity vs Wine, 13), B4 (HUD instruments, 8 — start).
**Increment:** Visually-correct flight view with working primary instruments.

### Sprint 5 — "Natural controls" (R2)
Stories: C2 (joystick, 8), B4 finish (8), H2 (control rebinding, 5).
**Increment:** Joystick-flyable sim with rebindable controls — **R2 ships**.

### Sprint 6–7 — "Sound & vision" (R3)
Stories: D1 (SFX, 13), D2 (music, 5), E1 (Smacker video, 13).
**Increment:** Immersive build with audio + cutscenes — **R3 ships**.

### Sprint 8–9 — "Play the game" (R4)
Stories: G1 (Quick Mission, 21), G2 (campaign, 21).
**Increment:** End-to-end playable missions & campaign — **R4 ships**.

### Sprint 10 — "Ship it" (R5)
Stories: H1 (packaging, 8), H3 (docs, 3), buffer for polish/bugs.
**Increment:** Installable v1.0 with no manual env vars — **R5 / v1.0 ships**.

---

## 6. Burndown / Tracking

Track per sprint (fill in at review):

| Sprint | Committed pts | Completed pts | Velocity | Notes |
|---|---|---|---|---|
| 1 | 21 | 16 (+~3 of A2) | ~16–19 | A1 (13) ✅ 20/20, A4 (3) ✅, A2 (5) code-complete; A2.4 live demo carried to S2 (env wedge) |
| 2 | 17 | ~14 | ~14 | Machinery restarted post-reboot. A2.4 ✅ (round-trip PASS) + A1 re-val 20/20; **F2 ✅** (combo dropdown), **F3 ✅** (resolutions combo); C3 partial (rendering panels covered) → remainder re-sliced with F4 into S3. **R1 functionally complete.** Board: `port/scrum/sprint-02.md` |
| 3 | 13 | 13 | ~13 | **C1 ✅** keyboard flight controls (DirectInput→SDL) validated + demonstrated (view-pan, 89.9% frame change; 115 actions; numpad gap closed) + HUD-SIGFPE root-cause fix (units). A1 8/8. Board: `port/scrum/sprint-03.md` |
| 4 | 13 | 13 | ~13 | **F4 single-player front-end DONE** — Quick Mission + Campaign render & navigate (Comms = out-of-scope multiplayer). + cross-port refcount-UAF insurance (BoB note). A1 8/8. Board: `port/scrum/sprint-04.md` |
| 5 | 13 | 13 | ~13 | **Menu↔flight round-trip ✅** — Quick Mission "Fly"→3D flight→exit→menu in one process. Board: `port/scrum/sprint-05.md` |
| 6–28 | — | — | — | Audio (S6, `ma_openal`+`ma_music`), campaign→Korea map (S7), colour fidelity (S8/S20), joystick (S10), save/load (S11–14), ASan heap-grind (S15–18), in-flight mouse (S18), live play-test hardening (S21–28: in-map nav, padlock crash, HUD, mission combo). See `STATUS.md` + `port/scrum/sprint-{06..24}.md`. |
| 29 | — | done | — | **Cross-port ASan hardening** — adopted 4 shared-engine fixes from BoB S46→S62 (rnd over-read, BITSET granularity, LBM unpack, CRListBoxCtrl `delete[]`); 4 verified non-shared / pre-fixed. ASan differential-validated. Board: `port/scrum/sprint-29.md` |
| 30 | 8 | 8 | — | **H1 bare launch ✅** — `./wmig` from the install dir boots with no env vars (derive `BOB_DRIVE_C` from cwd + auto-run; hatches preserved). Board: `port/scrum/sprint-30.md` |
| 31 | 5 | 5 | — | **Shippable polish ✅** — window title → "Mig Alley"; real `README.md` (H1 run/install instructions) → **H1 done**. Replay-hang flagged to PO (interactive-repro-gated). Board: `port/scrum/sprint-31.md` |
| 32 | 8 | 8 | — | **B3 smooth flight ✅** — sustained ~50 fps over a 62s flight (sim-paced, 3048 frames); added `MA_TRACE_FPS`. Board: `port/scrum/sprint-32.md` |
| 33 | 5 | 3 | — | **Play-test prep** — Replay-hang graceful-degrade (re-enable EXITKEY in playback) + `MA_TRACE_REPLAY`; play-test script for the PO session (Claims header / gunsight / wheel-zoom = diagnose-in-session). Board: `port/scrum/sprint-33.md` |
| 34 | 8 | 8 | — | **B5 high-res ✅** — resolution combo up to 1920×1080 (4:3+16:9), applies to windowed flight (`Save_Data.displayW/H` + both DD enumerators + relaxed `IsValidMode`), window centers/borderless-fills; ADI kaleidoscope-on-bank fixed (cap to native 128px). Live-validated (long 1080p dogfight). |
| 35 | 5 | 5 | — | **Play-test fixes ✅** — Claims "Player" header (`IDC_SDETAIL17`); confirmed via PO session. (Replay-hang escape S33; ADI S34.) |
| 36 | 13 | — | — | **C4 ALT+D/SHIFT+D padlock overlay** — red box + bogey/closure/range/own-speed@rel-alt telemetry (in progress; interactive/PO-gated). |
| 37 | 8 | 8 | — | **ASan S18 base-item type-confusion ✅** (autonomous, headless DoD) — `LauncherToWorld` (`3dcom.cpp:13436`, ×3 heap-overflow) + `InitROL` (`Rchatter.cpp:1671`) read `hdg/pitch/roll`/`vel` through a base 32-byte `item` (AAA ground site). Gate on `Status.size >= RotatedSize`/`MovingSize`; identity orientation / zero velocity for static items. ASan flight: LauncherToWorld →0; production stress **8/8**; no regression. |
| 38 | 5 | 5 | — | **ASan S18 lifetime UAF ✅ — ★ flight path ASan-CLEAN** (autonomous, headless DoD) — `PersonalThreat` (`Msgai.cpp:1794`) compared `trg/agg->nationality` before its own `Status.size==AIRSTRUCSIZE` guard; `agg`=bullet `Launcher` dangles if the firing AC left the world → heap-UAF (`worldinc.h:715`). Fix: early validity `return(false)` before the deref (behaviour-preserving). **Zero ASan reports across 5 flights** (baseline reliably reproduced it); stress **8/8**. **S18 sub-epic closed; the entire instrumented flight path is now ASan-clean (S15→S38).** |
| 39 | 3 | 3 | — | **ASan gate + coverage map ✅** (autonomous, headless DoD) — `port/asan_flight.sh` standing regression gate (N flights → **fail if any ASan report**; verifies 3D reached). Current **PASS** (0 reports). Cross-port completeness swept (all BoB finds ≤S82 have MA verdicts). ASan coverage mapped: boot ✅ / flight ✅ / **campaign serialiser ◻ gap** (documented recipe for the next sprint). Board: `port/scrum/sprint-39.md` |
| 40 | 8 | 8 | — | **Campaign-path ASan sweep ✅** (autonomous, headless DoD) — built the first **headless campaign-drive recipe** (title→Load Game→"Auto Save"→Load→Korea strategic map; discovered menu-row map + file-list/Load coords via `MA_TRACE_OLE`/frame dump). Added `MA_IGNORE_SAVE_DATE` (port fix: the build-date guard voided every save on recompile; format is stable) + `port/asan_campaign.sh` standing gate. The path runs `PackageList::LoadGame` (S65a site) → **S65a now ASan-validated on a live load**; **0 ASan reports**, map renders. Flight gate unregressed. Board: `port/scrum/sprint-40.md` |
| 41 | 8 | 8 | — | **Campaign mission-gen ASan sweep ✅ — 2 real bugs fixed** (autonomous, headless DoD) — drove the loaded campaign to fly (`MA_CAMP_FLY`) under ASan: surfaced (1) `make_airgrp` (`Persons3.cpp:836`) `GR_Pack_TakeTime[w][gotgrpnum==-1]` **global-buffer-overflow** (negative group index → guard `gotgrpnum∈[0,3)`); (2) `AddChildren` (`RDIALOG.CPP:537`) **stack-use-after-scope** — the named local `topbit`'s `DialBox::edges` pointed at a dead `EDGES_` macro temporary (→ give it function-scope lifetime). Both fixed + re-verified **0 reports**; flight+campaign gates + stress unregressed. Board: `port/scrum/sprint-41.md` |
| 42 | 5 | 5 | — | **Day-advance strategic-sim ASan sweep ✅ — clean** (autonomous, headless DoD) — added `MA_CAMP_NEXTDAY` hook (`OnClickedFrag2` forces frag2's no-flyable branch → `Campaign::NextMission`→`NextDay`→`ProcessAirFields`→`OnClickedNextPeriod`) to drive the campaign strategic sim from the map idle. **0 ASan reports across 3 runs.** (SaveBin/SaveGame writeback already swept by S41's frag2 else-branch.) Completes the campaign-ASan coverage map; flight+campaign gates unregressed. Board: `port/scrum/sprint-42.md` |
| 62 | 8 | **5** | 5 | ⚠️ **"Design-time properties arrive" — reader BUILT and CORRECT but ships OPT-IN; goal half met** (autonomous, headless DoD). Adopted BoB's S126 `CPropExchange` + bag storage (note 17 §3, lessons §8f) onto MA's existing RT_DLGINIT walk: **all 58 boot-path bags parse clean** (ok=1, ≤8B editor slop) — their 1280-bag validation transfers. **Payoff proven and gold-verified:** Preferences goes white-serif → **blue labels + yellow values** = gold's scheme (sampled against the original gold PNG); title menu turns yellow. **The colour half of cross-cutting #1 is solved.** Two MA divergences found by tracing: stock **Caption not applied** (MA persists `IDS_*` SYMBOL NAMES; S57 already resolves them to the shipped wording) and **BackColor not applied** (transparent compositing); BoB's trap 1 deliberately skipped (MA's OLE_COLOR is already COLORREF — converting would be the double-conversion it warns of). **Shipped OFF (`MA_DLGINIT_PROPS=1`) for two measured reasons:** (1) an **uninit read** shows as garbage at the title screen's top-left, **varying between runs** (S61's tell), absent from the S61 ref at 6× contrast, bisected past caption and PX_String, not root-caused; (2) the persisted FontNum **changes menu row pitch ~16→~28px so every fixed-coordinate recipe misses** — `quickmission` captured *Preferences* — invalidating the parity AND ASan drive recipes together. Opt-in keeps the default byte-identical and the gate trustworthy. Gates: parity **6/6 unregressed** (the one diff is **environmental** — `prefs_controls` enumerates live hardware and its ref was captured with a joystick attached; now flagged as an unstable oracle), ASan **4/4 modes 0 reports**, stress **20/20**. Note 20 + §8i, both copies md5-identical. Board: `port/scrum/sprint-62.md` |
| 72 | 6 | **3** | 3 | ⚠️ **"Light up the 3D overlays" — CLOSED PARTIAL; first sprint on the 3D-view parity frontier (I3)** (autonomous; DoD = A/B evidence). Grounded the epic with a `gl-lock` cockpit A/B vs gold #10 (also confirmed S69 per-face fonts work in the GL path). Characterized #10/#11: cockpit frame + panel render a **crisp FLAT-BLACK silhouette** (geometry rasterizes; only the fill is black) + a native-only black rectangle top-right (padlock-ADI inset); gold has a textured metallic canopy + panel. **Root cause narrowed** (advance on "palette/texture upload"): the software rasterizer HAS the image-span fillers (`XASM_ImageHoriLine*` in `ma_xasm.nasm`), world terrain + gunsight texture render, and `textureQuality` (High) doesn't gate it ⇒ the **cockpit-specific imagemaps resolve to black (not loaded/bound)** on the `btree::drw_cockpit` (`COCKPIT_OBJECT`) shape path. Scoped S73 fix: trace the cockpit poly's `Image_Map.GetImageMapPtr` binding vs a rendering world poly. Fix not landed (the 3 missing pts) — a deep per-poly texture-binding change deferred to a focused session rather than forced into a long run. No code changed → no build/gates. Board: `port/scrum/sprint-72.md` |
| 71 | 6 | **6** | 6 | ✅ **"Polish the chrome" — both S69/S70 chrome residuals resolved; EPIC I front-end essentially complete** (autonomous, headless DoD). **S71-1 OOB-listbox translucency:** added an OOB-only context flag `ma_oob_lb_draw` (set only while `ma_ole_draw_toolbar` draws an OOB listbox) so `CRListBoxCtrl::OnDraw` skips its black fill on the OOB path → the Player Log Career/Log tables composite over the dialog art (photo shows through = gold's translucency), while the front-end menu (same control, drawn via `ma_ole_draw_all`) keeps its opaque box and stays **byte-identical** (title 0px) — the S70 global-skip regression avoided. **S71-2 combo border: MEASURED, no fix needed** — `AXC_DARKEDGE/LITEDGE/CIRCULAR=RGB(103,132,198)` (blue) and `m_bCircularStyle=FALSE` always, so native already draws the same blue rectangular border + round button as gold (matched-scale crop confirms); the S69 "rectangular-white" residual was the anti-aliased blue edge at 800-res — retired, an S64-style measure-don't-assume close. #2 fully matched. Cross-port note 28 (OOB-only context-flag technique). Gates: front-end byte-identical sweep PASSES (title/prefs_3d/prefs_others/quickmission/campaign_map 0px); playerlog refs rebased for translucent tables. **ASan PASS 4/4 0 reports; stress 20/20 OK.** Board: `port/scrum/sprint-71.md` |
| 70 | 8 | **8** | 8 | ✅ **"Finish the Player Log" — I4/#15 CLOSED (Career content table renders)** (autonomous, headless DoD). **S70-2:** the Career tab's Sorties/Combats/Kills/Losses table (RListBox `IDC_RLISTBOXCTRL1` in `IDD_CAREER`, populated by `CCareer::OnInitDialog`) was **populated but never DRAWN** — the OOB dialog draw path (`ma_ole_draw_toolbar`) dispatched STATIC/EDIT/EDTBT/TABS/BUTTON/COMBO but had **no `CT_LISTBOX` case** (the front-end draws listboxes via a different path, `ma_ole_draw_all`, masking the gap; the Name box on the same tab rendering "proved" OnInitDialog fine and pointed at the draw layer). Added the case → table renders with data (F86 1/F86 2/F80/F84/F51/All × 4 cols, 0 on a fresh save); **bonus**: the Log of Missions tab's log listbox now renders too. Residuals named: (a) opaque listbox box vs gold's translucent — skipping the fill like the combo (#2) **erased the front-end title menu** (same `CRListBoxCtrl`, relies on the opaque box), reverted → needs an OOB-only context flag; (b) doubled "F86 1" header cell = source-vs-BDG data delta (`CAREER.CPP:173-177` adds the label twice). **S70-1:** font-rebase debt — `campaign_map` was **byte-identical** (date readout uses `g_AllFonts[1]="Intel"`=art face; S69's flag overcautious); only `map_playerlog_tab1` rebased. Cross-port **note 27** (missing OOB listbox case + the load-bearing-fill caveat distinguishing listbox from combo). Gates: **2D parity byte-identical sweep RESUMED + PASSES** (title/prefs_3d/prefs_others/quickmission/campaign_map 0px; playerlog refs rebased for the tables). **ASan PASS 4/4 paths 0 reports**. **Stress 20/20 OK under gl-lock** (clean — S69 HANGs confirmed load-induced). Retro: the combo and the listbox look like the same fix but aren't — the byte-identical sweep caught the difference. Board: `port/scrum/sprint-70.md` |
| 69 | 8 | **8** | 8 | ⭐ **"Face the type, dress the combo" — BOTH remaining cross-cutting visual deviations CLOSED** (autonomous, headless DoD). **S69-1 font FACE (three-sprint carry, scheduled first):** the port was silently running as a **Japanese system** — compat `EnumFontFamiliesA` **always** invoked the enum proc, so `MIG.CPP`'s localization probe took the CJK branch and asked for MS Mincho everywhere (unshipped → collapsed to the art face), so it **never requested Arial**; `MA_TRACE_FONT` showed the faces were mojibake CJK. Fixed the stub to report a face present only for a **pure-ASCII** name (no CJK ships) → English branch runs → runtime faces `Intel`(ART)+`Arial`(SANS) as on gold's box; and replaced the face-ignoring `ma_gdi_font_create` with a cached ART/SANS/SERIF registry (ART=Intel.ttf load-order-preserved; SANS=LiberationSans≈Arial; unknown→ART, never regress). **Gold-verified**: Prefs #2/#8 + QuickMission #9 = blue sans labels + yellow sans values = gold; campaign #13 phase list yellow sans; Intel bars byte-identical. **Cross-cutting #1 FULLY CLOSED** (colour S63 + face S69). **S69-2 combo chrome:** the combo was the one hosted control still filling itself **opaque black** (`RCOMBOC.CPP:355`, when `WM_GETARTWORK`=0, which the port returns deliberately); gold's combos are **transparent** (panel shows through a thin border — cropped-gold-verified). Skipped the black `FillRect` on the Linux path; combos now translucent = gold. **Cross-cutting #2 CLOSED** (residual: fainter rounded-blue border pen, named). Cross-port **note 26** (the `EnumFontFamilies` Japanese-branch trap + registry + combo fill). Gates: **2D parity = deliberate REBASE toward gold** (10 refs; `title` byte-identical; `map_playerlog_tab1`+`campaign_map` flagged for S70). **ASan PASS 4/4 paths 0 reports** (headless). **Stress 37/40 OK across two runs + 3 HANG, 0 crashes** (all HANGs = 25 s timeout under load 8–9, NOT a fault — 0 SEGV/FPE/ABORT/NO3D). Retro: a compat stub that returns *success* is invisible — `EnumFontFamilies`-always-true joins S68 `DrawIcon`-noop and S64 `GetFileNum`-returns-0, three sprints of the same class. Board: `port/scrum/sprint-69.md` |
| 68 | 8 | **6** | 6 | ✅ **"Icons and evidence"** (autonomous, headless DoD). **S68-2: the Player Log's `?`/`✓` render** — and the cause was a whole missing subsystem: **`CDC::DrawIcon` was a no-op stub and `LoadIconA` returned NULL, so NO icon anywhere in the port had ever rendered**, silently, for the port's whole life (a stub returning *success* never gets reported). Routed by engine logic: `RDialog`'s eventsink shows `IDJ_TITLE` itself raises Cancel/OK, so the title control draws its own buttons, gated on persisted flags — and the bag carries `close=0 tick=1`. Implemented RT_GROUP_ICON→RT_ICON decoding (group is a directory naming the image by id; `biHeight` doubled = XOR bitmap + 1bpp AND mask, bottom-up, mask bit 1 = transparent). Icons live in **`Rbutton.ocx`** (828–832), not Mig.exe — third instance of "inside a control, AfxGetInstanceHandle() is that control's module". Parity **5/5 byte-identical**. Closes the last CHROME deviation on #15. **S68-1: A/B done properly** (pre-S66 built via git worktree; alternating S65↔HEAD, SUAR forced) → **S65 0/12, HEAD 0/12** ⇒ no difference between arms; **downgraded to a watch item, not attributed and not closed** (~50 runs since the single 2-report sighting is consistent with a ~1-in-50 defect). **S68-3 per-face fonts: third consecutive carry** — mis-prioritised, not unlucky. Near-miss recorded: an `ls | head` truncated `Rbutton.ocx` out of view and nearly established a false negative. Board: `port/scrum/sprint-68.md` |
| 67 | 8 | **4** | 4 | ⚠️ **"Attribute and trim" — CLOSED PARTIAL; weakest sprint of the run** (autonomous, headless DoD). **S67-1 did NOT attribute the S66 ASan finding**: a dedicated hunt (4 modes in rotation, `detect_stack_use_after_return=1` forced) found **no recurrence** — but the hunt was stopped early after ~4 runs (CPU contention), so it is a weak sample and no rate can be claimed; a clean hunt on the CURRENT build cannot distinguish "S66 didn't cause it" from "it didn't fire", and the pre-S66 A/B was not done. Carried explicitly. **S67-2 fixed the title-bar width**, cause general: **our DCs had no clip region** (Windows clips a control to its own window), and CRButtonCtrl's picture path blits its DIB at natural size — `IDJ_TITLE`'s ~550px art ran ~213px past the 336px dialog. Added `ma_gdi_set_clip`/`restore_clip` honoured by putpx/BitBlt/StretchBlt; parity **4/4 byte-identical**. `?`/`✓` narrowed: NOT template controls (IDD 276 has only 1001+1117) and NOT in the title art. **S67-3 not started.** Retro: repeated the S65 trace-cap trap one sprint after documenting it — new rule, **filter don't cap**. Board: `port/scrum/sprint-67.md` |
| 66 | 8 | **6** | 6 | ⭐ **"Face it" — CROSS-CUTTING DEVIATION #1 SOLVED** (autonomous, headless DoD). The biggest single visual gap in the parity epic since S56 is closed: colour in S63, **FACE here**. The game ships its own typeface — `drive_c/windows/Fonts/Intel.ttf`, "Copyright (c) Rowan Software, 1998" — and the port never loaded it, for two independent reasons: `ma_gdi_font_create` **ignores the requested face**, and the single global TTF load was **rejecting Intel.ttf** because `stbtt_InitFont` accepts only platform-3 cmap encodings 1/10 while Intel.ttf ships a **(3,0) SYMBOL** cmap → init failed and every run silently fell back to a system serif. Fixed by accepting symbol cmaps + routing lookups through `ma_cp()` (symbol tables address chars at 0xF000+c). **Verified against gold**: title menu in yellow small caps, identical face; Preferences matches gold's tab bar/labels/values. Residual on those rows now only the BDG tab (resource delta) + combo chrome (#2). **S66-2 (title bar width + ?/✓) displaced — the missing 2 pts.** Parity sweep = deliberate REBASE of all 7 refs (typeface changes every screen by design); byte-identical resumes S67. ⚠️ **ASan FAILED once then PASSED** — 2 intermittent `stack-use-after-return` in the packed-item proxy accessors (worldinc.h:257/565), ~1 in 20 runs, **NOT attributed** (S66's diff is font-only but the pre-S66 binary was not tested); stress 20/20. Board: `port/scrum/sprint-66.md` |
| 65 | 8 | **6** | 6 | ⚠️ **"Bags and faces" — CLOSED PARTIAL; headline target MET: the Player Log TITLE BAR RENDERS** (autonomous, headless DoD) — the target that resisted S60/S62/S64. **S64's stated blocker was a trace artefact**: `[px]` was capped at 60 lines and the boot path replays 58+ bags, so id 1001 fell off the end and absence-of-output was read as absence-of-behaviour; cap now `MA_TRACE_PX_MAX`-tunable and the replay is clean (`len=178 ok=1 consumed=175/178`). Nothing was missing — IDD 276's bag carries `IDS_PLAYERLOG` + literal `Player Log` + `FIL_TITLEB_BMP`×2, and **two narrowing filters were each withholding half** (S58's tickbox-only caption rule; S64's art-name gate). Fixed by treating `IDJ_TITLE` as the reserved engine id it is (family of `IDJ_TABCTRL`/`IDJ_PANEL0..9`). **S65-2 tested and REJECTED template membership as the general narrowing criterion** (system-box "Quit" is inTmpl=1 too) — recorded so it is not re-tried. **S65-3 font FACE NOT STARTED** (the missing 2 pts; third sprint carried). Residuals named: title bar draws wider than the dialog (`UpdateTitle` sizes from `viewsize.right`); `?`/`✓` still absent. Gates: parity 4/4 byte-identical, ASan + stress per log. Board: `port/scrum/sprint-65.md` |
| 64 | 8 | **6** | 6 | ⚠️ **"Face value" — CLOSED PARTIAL** (autonomous, headless DoD). **S64-1's result was that the reported defect does not exist**: measured gold vs native glyph band 10px vs 11px and row pitch 52px vs 51px — the SAME absolute font size. S63's "renders larger than gold" came from comparing a 1280×1003 gold shot with an 800×600 native capture. Corrected; the resolution caveat promoted to an explicit rule (the game picks its panel ART SET by resolution, so gold-vs-native density comparisons are invalid); cross-cutting #1 re-scoped to **font FACE only**. **S64-2 fixed two real bugs but missed its target**: `GetFileNum()` was a stub returning 0 (every name-resolved control silently lost art — the sound half of BoB trap 2) and `CString(LPCWSTR)` was declared but never defined (link-only, invisible until a BSTR was read back). The art-name application is implemented but **shipped OFF** after the sweep measured it resurrecting the system-box Quit/Size buttons — exactly S58's documented failure. Title bar still absent; blocker precisely located (`ma_px_replay` never fires for id 1001 — a bag-keying question). **S64-3**: `prefs_controls` embeds live joystick state → excluded from the sweep as environment-dependent. Gates: **parity 5/5 byte-identical (check RESUMED)**, ASan 4/4 modes 0 reports, stress 20/20. Board: `port/scrum/sprint-64.md` |
| 63 | 8 | **8** | 8 | ✅ **"Switch the properties on" — goal MET; reader ON by default** (autonomous, headless DoD). Cleared both S62 blockers. (1) Uninit garbage root-caused: **`WM_GETSTRING` is IN/OUT and three R* sites ignore the OUT half** — `workspace[0]=99` is the IN capacity, and with no parent routing the message SendMessage returns 0 having written nothing, leaving 'c' (0x63 = the first garbage byte) + uninit stack as the caption; latent until the reader gave m_ResourceNumber real values. Third Win32 mechanism in the same uninit family; run-to-run variance was the tell again. (2) **Recipes made font-independent** — `f,rN` (menu row from the listbox's own metric) and `f,#ID[:COL]` (control by dialog id, column via GetColFromX), because the pitch moved 16→28px and broke the parity AND ASan recipes together. `GetRowFromY` unusable (m_playerList clamp); "Back Load" is ONE horizontal listbox. (3) Reader default-on: **values yellow matching gold exactly**, tab bar yellow, labels into gold's blue family — **colour half of cross-cutting #1 SOLVED**; residual narrowed to font FACE+SIZE (native renders larger than gold). Parity sweep deliberately REBASED (not byte-identical — the reader changes fonts by design). Gold USB unmounted; local mirror used and recorded. Note 20 (owed from S62) + §8i, md5-identical. Board: `port/scrum/sprint-63.md` |
| 61 | 8 | **7** | 7 | ✅ **"The Player Log lands" — goal MET; #15 PARTIAL → CLOSE-minus** (autonomous, headless DoD). Four defects, all presenting as "the dialog draws in the wrong place": (1) ★★ **`borderwidth` was uninitialised heap** — startup never checks `RegQueryValueEx`'s return and the compat stub writes neither `type` nor `buff`, so both came off uninitialised stack; it feeds `MakeParentDialog`'s sizing ⇒ a DIFFERENT garbage dialog origin every run (the run-to-run variance is what identified it); (2) ★ **`OnGetXYOffset` built on no-op `ClientToScreen`** ⇒ every dialog reported offset 0 and the whole tree composited at the top-left — replaced with the accumulated parent-chain origin, NOT by giving ClientToScreen global semantics (S60's lesson); (3) **`IDJ_PANEL0..9` placeholders unregistered** ⇒ `AddChildren` stacked children BELOW the parent (tab box at y=396 on a 400px dialog) — they are plain NATIVE template controls, missed by S60's OCX-kind hosting; (4) **title-height double-count**, gated on `artnum==artnum` so it hit the art-less tab host but not the art-bearing page ⇒ 27px out of step and the page art painted over the tab strip. Tab bar now renders with real RTabs.ocx art, dialog centred, **tab switching capture-proven** (`MA_OOB_PLAYERLOG_TAB=N`, new ref `map_playerlog_tab1.png`). A view-rect sync needed by (1) changed the campaign map (extra tile row ⇒ canvas 644→900) and was scoped with an RAII restore — caught by the parity sweep. **Not done:** title bar + `?`/`✓` — `IDJ_TITLE` is in-template, hosted and unfiltered, but its art/caption live in the **RT_DLGINIT property stream** MA cannot yet parse ⇒ the R* property-stream reader (BoB note 17 traps 1/2) is now the TOP backlog item. Gates: parity **5/5 byte-identical** (incl. campaign_map, added this sprint), ASan **4/4 modes 0 reports**, stress **20/20**. Note 19 + §8h, both copies md5-identical. Board: `port/scrum/sprint-61.md` |
| 60 | 8 | **5** | 5 | ⚠️ **"The Player Log opens" — CLOSED PARTIAL; sprint goal NOT met** (autonomous, headless DoD). Trace-first paid off: the planning hypothesis ("RTabs unhosted → GetDlgItem NULL") was wrong — **RTabs was never CREATED**. Two engine root causes found behind three of #15's four deviations: (a) **template-declared OCX controls that no dialog class DDX_Control-binds were never instantiated** (S57's static-only hoster → kind-driven `ma_host_template_controls`: RStatic/RButton/**RTabs**; IDJ_TITLE is an RButton hosted since Phase 4, absent only because nothing created it); (b) ★ **no RDialog in a tree ever learned its own size** (ctor zeroes homesize/viewsize; the refresh line is commented out at `RDIALOG.CPP:147`) → `RDialog::OnSize` gave IDJ_TABCTRL a zero-width MoveWindow → `MaSeedTemplateSize()` + `ma_dlg_own_size()` (tab host 0x0→420x258). New `ma_oletabs.cpp` + `rtabs` build mode (both builders); tab art loaded from **RTabs.ocx's own PE** (297x31); all 3 tabs register with gold captions; **Name label + edit box now render** (unplanned). **Not met: tab bar/title bar not composited at the right offset** → S61 (suspect `OnGetXYOffset`'s `parent->artnum==artnum` walk). A `CDialog::Create`-wide version of (b) regressed the front end (canvas 644→600) and was re-scoped to the tree builders. Gates: **2D parity sweep byte-identical ×4**, ASan **4/4 modes 0 reports**, stress **20/20**. Note 18 + §8g both copies md5-identical. Board: `port/scrum/sprint-60.md` |
| 59 | 8 | 8 | 8 | **"Quick Mission settles" ✅** (autonomous; salvaged after a session-limit kill) — #9 root-caused: the "stray combo" is the dead-coded Cloud/Weather cluster parked OUTSIDE the 335-dlu dialog (Windows parent-rect clipping → `ma_dlg_never_visible`), and the phantom "I.D." label was a `!WS_VISIBLE` template control (style dword now parsed → initial show state); compat `CDC::DrawText` implements real `DT_WORDBREAK`; uninit-PX ctor audit widened to RSTATIC/RBUTTON/RCOMBO/REDTBT; the dummy==GL cmp bar caught a second environment-dependence class (DI mouse gated on the window → now unconditional). #9 → CLOSE-minus. ASan 4/4 PASS. **Stress gate deferred at close (locked display session) and CLEARED 2026-08-01: PASS 20/20 on the same commit** — environmental diagnosis confirmed. Notes 17 both directions. Board: `port/scrum/sprint-59.md` |
| 58 | 8 | 8 | 8 | **"Capture the proof" ✅ — S57 parity fixes capture-proven; 2D oracle display-independent** (autonomous, headless DoD; GLX healed) — MA_SHOT GL-free capture path (dummy==GL **byte-identical**); root-caused+fixed the salvage strip artifact = **uninit `DoPropExchange`-only members** (`RLISTBXC.CPP` ctor now inits PX defaults; also fixed title-menu doubling); **#7→CLOSE, #8→CLOSE, #1 first capture→CLOSE**; `asan_all.sh` PASS (0 reports, 4/4 paths) + stress 8/8; BoB note 16 processed + MA note 16 sent + §8f addendum (both copies md5-identical). Board: `port/scrum/sprint-58.md` |
| 57 | 8 | ~6 | — | **PE resource path adopted (BoB note 14 / §8f) — GL verification HARD-BLOCKED** (autonomous, headless DoD) — miglang.dll (BDG 0.85F, 2005-04-29) + Mig.exe confirmed as the oracle's resource modules; BoB's enumerators ported (`bob_resources.cpp`, dual-module dedup); MA found already PE-first per-IDD → applied the §8f *lessons* to MA's own files: template-static hosting (`ma_host_template_statics` — the exact 6 Others + 4 Controls missing labels are the unbound sets), IDS→BDG-string-table captions ("Input Devices:" = gold wording), membership draw/click filter, tickbox FIL_ art+glyph, **REdtBt OCX newly hosted** (Calibrate; new `oleredtbt` build mode), DI axis `tszName` fill, classic creation-data/EX parser fixes. `MA_NO_PE_RSRC=1` hatch A/B-proven byte-identical to S56. **GLX wedged machine-wide (X_GLXCreateNewContext BadValue) → re-captures + asan/stress gates skipped**; I4 not pulled (capture-gated). Board: `port/scrum/sprint-57.md` |
| 56 | 8 | 8 | — | **EPIC I parity oracle stood up ✅** (autonomous, headless DoD) — (1) inherited IMAGEMAP.CPP WIP judged: controlled A/B (2×2 runs, `MA_TRACE_LBM`) **proves the LBM bounds fix** — only delta is a 2×2 imagemap truncated at the buffer edge where unbounded reads 19 B past the heap block; instrumentation KEPT (env-gated, default bit-identical). (2) **I1 ✅**: all 14 gold shots + I4 gold #15 inventoried with per-shot verdicts (5 CLOSE / 6 PARTIAL / 2 pending) in `port/scrum/screen-parity.md`; 13 native captures committed (`port/ref/native/`); oracle provenance = **BDG 0.85F** flagged. (3) **I4 first capture**: `MA_OOB_PLAYERLOG` headless hook renders the Player Log photo art over the map. Board: `port/scrum/sprint-56.md` |
| 55 | — | done | — | **CMake+Ninja incremental build ✅** — symbol-identical to `rebuild.sh`; packaging; path cache; Wine campaign-map oracle; cross-port note 12. Commit `3d99a70` |
| 54 | 5 | 2 | 3 | **OOB render generalized (verified) + Directives root diagnosed** (autonomous) — confirmed all 7 safe OOB dialogs render (Bases/Weather/Playerlog/Squads captures); all 10 buttons no-crash. Diagnosed the Directives crash: a fnhoist shadow OOB-write in COMIT_E.CPP AddMission (`for(char i` shadows hoisted `int i`) — fix ready but REVERTED unvalidated (session SDL/GL/X11 wedge blocked the ASan gate). Deferred to S55 w/ exact fix. Board: `port/scrum/campaign-epic.md` S54 |
| 53 | 5 | 5 | — | **OOB Squads dialog RENDERS with content** (autonomous, headless DoD) — mirrored BoB S113/S114: `ma_map_paint_oob` walks the open logged-child's tree each map idle → `MaOnPaint` background art + `ma_ole_draw_toolbar` the tab's content dialogs. Clicking Squads shows the squadron photo + real data (Available Aircraft / Rotate Flights combo / Bingo Fuel edit). ASan Squads-render + campaign gate PASS. Board: `port/scrum/sprint-53.md` |
| 52 | 8 | 5 | 3 | **OOB-info dialog epic STARTED — build crash FIXED** (autonomous) — gdb pinned the toolbar-button SEGVs INSIDE `CSqdnlist::Make()` (the MakeParentDialog tree build), not a NULL fchild. Two general fixes: `CDialog::Create` discarded its parent arg → `GetParent()` NULL in every `OnInitDialog` (→`SetMaxSize`/`InDialAncestor` deref); + SetUnits FPE (`mass.gm==0` unit-division, S3 family) guarded on the map path. **Squads OOB dialog now BUILDS clean, un-blacklisted**; render is next. Authorise/Directives have deeper `OnInitDialog` crashes (still deferred, 3pt carryover). Front-end + asan_campaign + asan_flight PASS. Board: `port/scrum/sprint-52.md` |
| 50 | 5 | 5 | — | **CRToolBar Phase-3 ✅ — toolbar buttons clickable** (autonomous, headless DoD) — `ma_ole_toolbar_click` hit-tests + fires `ma_evt_fire`→`ON_EVENT` handler. Verified: click Frag2 → `OnClickedFrag2` → briefing launches. 7 safe buttons fire; 3 OOB-info crashers (Squads/Authorise/Directives deref unbuilt `fchild`) blacklisted/deferred to the OOB-render epic. ASan campaign gate + Frag2-click PASS. Board: `port/scrum/sprint-50.md` |
| 48 | 5 | 5 | — | **CRToolBar Phase-1 ✅** (autonomous) — parent-scoped `ma_ole_draw_toolbar` (no stale bleed); 40 toolbar RButtons drawn at position, verified. Board: `port/scrum/sprint-48.md` |
| 49 | 8 | 8 | — | **CRToolBar Phase-2 ✅ — main toolbar icons render** (autonomous, headless DoD) — fixed the blank buttons: `CRToolBar:CDialog` didn't inherit `RDialog::OnRowanMessage` → `WM_GETFILE` art never loaded. Added `CRToolBar::OnRowanMessage` routing + control-id→icon table (Bases/Squads/Weather/Dis/Frag/…) + fileblock cache & dir-range guard (a bad-dir icon `SayAndQuit`→exit tripped a pre-existing Curve static-teardown bug). Main toolbar renders per-button icons, default-on; `asan_campaign`+`asan_flight` PASS. Board: `port/scrum/sprint-49.md` |
| 47 | 2 | 2 | — | **Campaign map date/period readout ✅** (autonomous, headless DoD) — the `TitleBar` (`IDC_DATE`) is a `CRToolBar` not hosted yet; drew its string directly on the map from `MIGVIEW` (`GetDateName(MMC.currdate)` + period/phase). Renders "6/25/50: Morning, planning" top-left; frame-verified; ASan campaign gate PASS. Board: `port/scrum/sprint-47.md` |
| 46 | 5 | 5 | — | **Campaign map unit/airfield icons ✅** (autonomous, headless DoD) — icons never drew: `DrawIcons` took its view bounds from compat `CDC::GetBoundsRect` (garbage + `DCB_SET` → overflowed world rect excluding all 652 items). Fixed to `GetClientRect` (like `UpdateBitmaps`) + defaulted the standard map filters ON (filter toolbar not hosted yet). Map now shows airfield/squadron/supply markers + front-line + routes (33 icons); frame-verified; ASan campaign gate PASS. Board: `port/scrum/sprint-46.md` |
| 45 | 5 | 5 | — | **Map colour fidelity ✅ — was a frame-dump bug, not a render bug** — the campaign map always rendered full colour; the "grey/speckle" (STATUS S7/S14/S20) was `BOB_DUMP_FRAME`'s `glReadPixels` **`GL_PACK_ALIGNMENT`** default (4) mangling the **1021-wide** map (`1021*3` not ÷4 → 1-byte/row RGB drift → channel-shift noise); 640/800 frames were ÷4 so looked clean. Fix: `glPixelStorei(GL_PACK_ALIGNMENT,1)` before `glReadPixels`. Map now dumps clean full colour = **colour parity with BoB**; all headless captures pixel-accurate at any width. Board: `port/scrum/sprint-45.md` |
| 43 | 3 | 3 | — | **Unified ASan regression suite ✅** (autonomous, headless DoD) — `port/asan_all.sh` runs all four sweeps (flight + campaign map/fly/nextday) in one command; fails on any report or unreached path. **PASS** (all 4 paths, 0 reports). Closes the S37→S43 ASan epic (8 heap bugs fixed across the arc); the boot+flight+campaign paths are ASan-clean end to end and re-verifiable in one command. Board: `port/scrum/sprint-43.md` |
| 44 | 2 | 2 | — | **Cross-port sync (BoB S83→S93 reply) ✅** (autonomous) — processed BoB's incoming reply; answered §3 map-toolbar-art (MA's `F_GRAFIX.G` **not** skewed: 177 `FIL_ICON_*`, 0 `FIL_xICON_*` → data-drop divergence, not a shared bug; MA has `SetNormalFileNum`/`StretchDIBits` infra, no map-toolbar draw yet) + §4 map-interaction (MA's `OnLButtonDown` stock; pan/zoom via the `ma_map_nav_*` idle bridge; unit-select unwired). **Flagged 3 finds back to BoB:** `AddChildren` dangling-`Edges` (SHARED dialog framework → new §8c), `make_airgrp` negative-index, `MA_IGNORE_SAVE_DATE` save-guard trap. Reply committed + delivered to `bob/doc/`. Commit `80ce138`. |

Re-estimate the backlog and re-slice sprints after Sprint 1 establishes real velocity.

---

## 7. Risks & Mitigations (Product Owner watch-list)

| Risk | Impact | Mitigation |
|---|---|---|
| 3D-startup races deeper than expected (A1) | Blocks whole R2 train | Time-box investigation; add explicit thread-ordering barriers; mine BoB sibling port for the same fix. |
| HW display-mode/3D path diverges from Wine | Fidelity gap (B2) | Keep `MA_DUMP_BACK` A/B harness as the regression gate. |
| Miles/Smacker have no clean Linux replacement | Audio/video slip | Use libsmacker / SDL_mixer; keep stubs so gameplay (R2/R4) never depends on D/E. |
| 32-bit toolchain / packed-struct fragility | Build breakage | DoD requires 0-undef link every story; never relax `-fpack-struct=1`. |
| Scope creep into editor/multiplayer | Delays v1.0 | Out of scope for this epic; park in a future backlog. |

---

## 8. Out of Scope (parking lot for future epics)
- Multiplayer (DirectPlay→native networking)
- Mission editor tooling
- 64-bit port
- Modern renderer (Vulkan/hardware path beyond current GL present)

---

*This backlog is a living document. Update statuses at each Sprint Review; reflect material
changes in `CLAUDE.md` and the `migalley-port-state` memory note.*
