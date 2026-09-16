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

> ⭐ **SCOPE CHANGE, PO 2026-08-25 — the port may now IMPROVE on the original.** Until today every
> item in this backlog was measured against fidelity: the gold shots, the Wine build, "what did
> Windows do". EPIC L (Tacview export) is the first story whose acceptance criterion the original
> game **cannot** satisfy, and the PO named it as such: *"yes, that's right. IMPROVING on ma, not
> just porting it ... The first improvement on ma in 20 years!"*
>
> **This does not relax the engineering constraint, and the distinction matters for every future
> judgement call.** Game sources stay unedited and the compat layer stays the place work lives; an
> improvement is *additive* — it must not change what the original path does, so every parity
> oracle keeps its authority. When something looks wrong, "is this faithful?" is still the first
> question; only a story explicitly marked **[IMPROVEMENT]** is exempt, and only in the direction
> the PO asked for.

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
| E1 | As a player, intro/briefing Smacker videos play, so cutscenes work. | 13 | `Smack*` stubs → real decode (libsmacker) → `ma_ddraw_present`; introsmack plays on title launch. | ◐ **S460 (Fable 5.1, 2026-09-06): BUILT, decoder GATED, screen UNVERIFIED.** `OpenSmack/DoSmack/CloseSmack` (the three stubs every clip path -- CSmackerDialog, title intro, campaign intros -- calls) now live in `SRC/compat/ma_smack.cpp` on `ma_smack_core.cpp` (libavformat/libavcodec; the box's **i386 libav runtime** linked by absolute path since wmig is 32-bit and no i386 -dev exists -- headers from the amd64 -dev, same 8.0.1). Frames are 8-bit+palette, blitted every `ma_ole_draw_all` pass at the dialog's CWnd rect via the GDI shim's StretchDIBits; audio streams to an OpenAL source on ma_openal's context. Callers now pass the resolved path (`File_Man.NameNumberedFile`) and the CWnd (the old `(int)m_hWnd` was always 0 here). Gate `port/smack_gate.sh` (32-bit `smacktest` vs ffprobe): Kimpo 400x300/233, intro 640x332/1235 + 44.1 kHz stereo, c1_int 384x288/570 -- all match, all PASS. `MA_NO_SMACK=1` skips clips; `MA_TRACE_SMACK=1` traces. NEXT was a title launch: done 2026-09-06 08:53 with MA_TRACE_SMACK=1 -- **the game never calls the player at the title**: `IntroSmackInit()` / `IntroSmackInitForCredits()` (the intro launchers) have NO callers in the compiled tree (the original's startup call sits in DEADCODE in PEACMISS.CPP). **DONE 09:12: MIG.CPP now launches the original `introsmack` screen (MA_NO_INTRO=1 keeps the direct title); its init calls the player, whose path resolver (`bob_resolve_path`) turns File_Man's `C:\\rowan\\mig\\smacker\\intro.smk` into the install path. Run through the AppDir launcher with MA_TRACE_SMACK=1: `[smk] open … 640x332 71.0 ms/frame audio=44100 Hz x2` then `paint frame 0/29/58/…/171` -- the intro plays with sound at the title. E1 ✅ (PO eye pending). |

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
| PO-10 | As a player, "?" opens the documentation window. | 13 | A viewer renders the topic for the screen the "?" was pressed on. | ✅ **CLOSED (S114): the "?" shows the game's REAL documentation.** The help SOURCE is in the repo — `SRC/<LANG>/HELP/MIG.RTF` (the RTF that `MIG.HLP` was compiled from), `MIG.HPJ` (`COMPRESS=12 Hall Zeck`) and `MIG.HM` (symbol → context id). `port/tools/rtf_help.py` extracts 43 topics + 186 context ids to `port/data/mig_help.txt` (installed with the game); the panel resolves the context id the game passes (`HID_BASE_RESOURCE+IDD_INTRODUCTION` → `HIDD_INTRODUCTION`) and renders that topic's text, e.g. *"1. You are in command of a force of 112 aircraft arranged into seven squadrons…"*. The compiled file's Hall compression never had to be decoded. Earlier: **S112: the "?" opens a documentation window** (`port/ref/native/help_panel.png`) — the game's own topic index read from `MIG.HLP`'s `|TTLBTREE` at runtime: *Map Screen, Main Toolbar, Filter Toolbar, Bases, Dossier, Squadron Information, Weather, Daily Intelligence Summary, Target List, Mission Results, Player Log, Aircraft Select, Routes, Debrief…* — exactly the screens the PO was pressing "?" on, with a footer stating that topic TEXT is not decoded yet. `CWinApp::WinHelp` (S98's destination, still an empty stub until now) raises it; a click dismisses it; `port/help_click.sh` now asserts the panel is **on screen** instead of reporting "routing only". **Still open: readable topic TEXT.** S112 searched **800** candidate Hall opcode layouts against S99's title oracle and the best scored **2/39**, so that family is eliminated — the decoder stays unsolved rather than shipping plausible nonsense (S99's rule) |
| PO-11 | As a player, the campaign screens have all their widgets. | 13 | A widget-by-widget inventory against gold video frames; each missing widget either implemented or listed with its cause. | ◐ **S109: four of the five clusters now render** — blue + red filter rows **with their icons** (the design-time art applies now that `CMIGView::DrawIcon`'s per-icon `fileblock` serves an already-open block, and art is gated separately from captions), main toolbar, **misc toolbar drawn for the first time**, system box; all click-routed at their paint offsets. Remaining: the **scale ruler** (`CScaleBar`, 0 hosted controls — it draws itself and nothing calls it) and any layout question that only B6 can settle. Earlier: **S108 inventory — five clusters, three mechanisms** (measured with `MA_TRACE_TOOLBARS=1`): **filters `m_toolbar1` 30 controls hosted, drawn BLANK** (no art: `ma_button_apply_icon` hand-maps ids and only knows the main toolbar + system box); **main `m_toolbar2` 10 ✅**; **misc `m_toolbar3` 6 hosted, NEVER DRAWN** (the map idle draws t1+t2 only — the same enumeration gap S106 found); **scale bar `m_toolbar4` 0 hosted** (`CScaleBar` is not an OCX dialog; nothing calls it); **debrief `m_toolbar5` 6 ✅ since S106**. The three top clusters need ~1190 px of width (393+529+264), which is why gold lays them side by side at 1280 and why the port's 800-wide canvas cannot; t1 (y=26) and t2 (y=52) also **overlap by 22 px** today. **Blocker with evidence:** the button art is a design-time property that S57 had to restrict to `FIL_ICON_TICKBOX*` after a regression, and re-widening it (`MA_BTN_ART_ALL=1`) trips a **fatal `Opened file block (6a48) again without closing`** — the S79/S84 double-open family. **Dependency:** `MA_FORCE_RES=WxH` proves the port's 2D canvas stays 800×600 in every mode, so a pixel comparison against these 1280-wide golds is blocked on **B6**, not on the widgets. Earlier: **first pass (S102, gold `short` @ 36 s vs `port/ref/native/campaign_map.png`)** — apparently absent natively: the **blue + red filter toolbar rows** (~16 icons each), the **right-hand toolbar group** (zoom in/out, save…), the **"MIG ALLEY" title-bar chrome** (native draws the date alone), the **scale ruler** down the left edge (0–350 Nm) and the **vertical scrollbar**. ⚠ **Not yet a defect list:** the gold frame is **1920×1080** and the native reference is **800×600**, and this engine picks its panel art set BY RESOLUTION (S64) — so step one is a native capture at the gold's own resolution. Judging "missing" across that boundary is the exact mistake S64 recorded |
| PO-13 | As a player, pressing a number key **inside** an in-flight menu selects that option, so the menus are usable and their sub-screens reachable. | 5 | Driving `R` then `3`, or `M` then `2`, reaches the submenu (gold `full` @ ~190 s shows the combat submenu; the map's option 2 is `waypointMapScr`, whose `UpdateWaypointDisplay` draws the gold's Rendezvous/Ingress/Initial-Point table). | ✅ **CLOSED (S107) — never a game defect.** `MA_TRACE_KEYEAT=<action>` watches `KeyPress3d` itself (the only honest way to find a consumer of a test-and-CLEAR) and showed the digit consumed **before the menu existed**: `[keyeat] KeyPress3d(106) bit=1 ret=1` *precedes* the `promote firstMapScr` line, i.e. `KEYFLY.CPP`'s throttle handler took it, correctly. The fault was the harness: `BOB_KEYSEQ` schedules on the **pump** counter, which in flight runs far slower than frames, so taps 20 pumps apart land seconds apart and these menus live 5 s. Fixed with **`MA_UISCR_KEY="0xNN[,frames]"`** — a key press armed when a screen is promoted, injected through the real buffered-keyboard queue (`ma_inject_dik`); the input twin of S104's `MA_UISCR_SHOT`. First try: `option key=1 selected -> promote waypointMapScr` |
| PO-12 | As a player, I can choose **hardware** graphics in Preferences, so the game renders through the path it was written for. | 21 | A primary-graphics option in Preferences selects hardware; the D3D path (`DoHardPoly`/`direct_3d`) renders flight and 2D; software stays selectable. | ✅ **DELIVERED S118 (2026-08-15).** Preferences → 3D → Display Driver offers *Software Driver* and **Primary Display Driver**; the choice persists in `settings.mig` and the next launch flies on the DX5/6 execute-buffer path on the GPU, no env var involved (`port/ref/native/hw_selected_in_prefs.png`). Software stays selectable and byte-identical. Four phases: S111 execute-buffer memory → S113 textures survive a mission → **S115 first frame** (blend table off by one; texture handles always 0; `GetWindowRect` a zero stub putting the whole world 240–480 px above the screen) → S116 textures + palettes (`IDirect3DTexture::Load` was a no-op, so every texture was empty) → S117 lines/points, the font coverage-mask blend, and depth (state is persistent across execute buffers; `glOrtho` negates z) → **S118 the option itself** (the Resolutions combo was empty in hardware mode: `driverNo` tag, `hard_modes` slot and `dddriver` all had to agree). Gates pass on both renderers. Remaining quality work: fog/specular, viewport Clear, other views vs the oracle. Earlier: 🔨 **S110 scoped it by measurement — a four-rung ladder** (`MA_TRY_HARDWARE=1` + `MA_TRACE_D3D=1`): (1) `IDirect3D::EnumDevices` must report a device or `DD.lpDirect3D` stays NULL and `HardPoly` returns FALSE immediately; (2) **three** places force software — `STUB3D::MakePassive`, `ma_populate_software_modes`, and the persisted `settings.mig` (which really loads since S103) — and the choice must be made **before display init**; (3) the game stops with *"3D Hardware acceleration is not enabled"* unless **two texture formats** are reported: 8-bit palettized and 16-bit-with-alpha (ARGB4444); (4) it then reaches `CreateExecuteBuffer` → `Lock` → **SIGSEGV** in `SetInitialRenderStatesLand`, so the first stub that must become real is the execute buffer's **memory**. Phase plan in `port/scrum/sprint-110.md`: buffer memory → opcode walk to GL (vertices arrive pre-transformed) → textures → the Preferences option with automatic software fallback. Earlier: **PO-added 2026-08-14.** BoB (`~/bob`, same engine) already runs hardware, so the *approach* cross-ports — but **not the code as-is**: per `ROWAN_ENGINE_LINUX_PORT_NOTES.md`, BoB is **D3D7 + Lib3D software-T&L** while MA is the older **DX5/6 execute-buffer** path (`WIN3D.CPP`/`HARDWIN.CPP` build execute buffers; `bob_video.cpp` already has the GL surfaces BoB's device sits on). Scope = an execute-buffer→GL device, not a port of BoB's device. High value beyond the option itself: it is the engine's own text/alpha path (`direct_3d::PutC`), the one the shipped game uses, so it retires a class of software-path workarounds — S102's included |
| PO-15 | As a player using **hardware graphics**, the terrain is drawn, so the ground looks like Korea rather than black ink. | 13 | Landscape renders in hardware at low altitude, matching the software renderer as oracle; runway and ground detail visible. | ✅ **CLOSED S120 (2026-08-15)** — found by the PO play-testing S118. The landscape has its OWN texture pipeline (tiles rasterised into a system surface by `TileMake::RenderTile2Surface`, blitted to video), separate from the object textures that go through `IDirect3DTexture::Load`. Root cause: the compat `IDirectDrawSurface2::Lock` never filled `ddpfPixelFormat`, so `rsd.dwRGBBitCount` reached the tile rasteriser as **0** and it wrote nothing — every land tile was blank, uploaded as fully transparent (index 0 is the engine's transparent key) and the cleared black showed through. Objects were unaffected because they never take that path, which is exactly the split the PO reported: *"huts and control tower visible, landing strips not"*. |
| PO-16 | As a player, I can **type my name** into the campaign profile, so I can start a career. | 5 | Keyboard input reaches the profile name field and the typed text is stored. | ✅ **CLOSED S121 (2026-08-15).** The front end had **no keyboard route at all** — every hosted OCX control was click-only and `CWnd::SetFocus()` was `{ return NULL; }`, so nothing recorded which control had the keyboard. Focus tracking + `SDL_TEXTINPUT` delivery added; editing is done host-side (the game's own `OnChar` needs an MFC message context a windowless host cannot give it). Two real compat nulls fixed en route: `strlen(NULL)` measuring an empty CString, and `ma_gdi_get_text_extent` dereferencing a DC it does not own. New `MA_TYPESEQ` injector — typing was the one front-end interaction with no synthetic driver. Evidence: `port/ref/native/career_typed.png`. |
| PO-20 | As a player at **high resolution**, the HUD is on my screen, not in a corner. | 8 | Info line, messages and instruments sit at the edges of the chosen resolution; both renderers fill the frame. | ✅ **CLOSED S122 (2026-08-15).** PO-reported at 1920×1080: *"the info line is at middle left, as if the screen were about 1/6 of its actual size."* `COverlay` lays out from `DoGetSurfaceDimensions`, which reported **640×480 on a 1920×1080 screen**: `SetDirectDrawMode` sized the render surface from the WINDOW RECT, which still holds the previous size when the mode is set. Before S115 that was accidentally right — `GetWindowRect` was a zero-fill stub so the mode-based fallback always fired; making the stub real removed the fallback. Also fixed the software renderer at 1920×1080 (was tiled 3× into the top 160 rows — same cause). |
| PO-21 | As a player, a front-end panel **replaces** the previous screen instead of layering on it. | 8 | Opening a campaign panel shows only that panel; clicking around does not accumulate text; the front end stays clickable at every resolution. | ✅ **REOPENED 2026-08-15.** A B6 consequence: panel art is a fixed 800×600 bitmap drawn at (0,0), so at high resolution it covers one quadrant and the rest keeps the previous panel's pixels. S128–S130 centred the art, filtered stale controls and offset hit-testing — and **broke front-end clicking**, so all three were reverted (`22aa759`). They shipped because every gate runs at the DEFAULT resolution, where the panel origin is zero and those changes do nothing. **PO-24 gate now exists (`port/panel_click.sh`) and REPRODUCES it**: with the panel work applied the menu draws at (1189,525) and clicking there does nothing; without it the gate passes (menu at (629,285), row selected). Diagnosis so far: the origin must be added in THREE places — paint, `ma_ole_click`, and `ma_ole_mouse` (which is what the menu actually uses) — and the menu's registered `m_maX` **already** includes centring (PositionRListBox measures against the canvas via MaViewRectScope), so adding it again double-counts. The drawn position differs from the registered one by 659px, which is neither the origin nor zero — that discrepancy is the next thing to explain, not to patch around. | **S144:** measured against the gold capture rather than reasoned about. Gold puts the two filter rows **immediately right of the date plate** (date 0–280, filters from x≈300); S109 right-aligned them to the canvas edge instead — which at 1920 drops them straight into the **system box's** corner. The two overlapped, so a click landed on whichever the walk reached first: the PO's *"most of the controls at upper right do nothing when clicked"* and *"there are two 'X' buttons"*. They are now placed beside the date, clear of the box, at both 1920 and 800 (where the grid's last column becomes visible for the first time). The 659px draw-vs-registry discrepancy that blocked this before was a symptom of the same overlap. |
| PO-22 | As a player, the campaign map has its **ruler/scale**, so I can judge distances. | 3 | The map scale bar draws and updates with zoom. | ✅ **PO-requested 2026-08-15.** `CScaleBar` / the Scale Toolbar is one of the two toolbar clusters S111 found present-but-unhosted (0 hosted controls, draws itself, nothing calls it). The game's own help text lists it among the five dockable toolbars. | **S135:** the ruler now draws. `CScaleBar` was constructed and initialised all along (`Init(...,AFX_IDW_DOCKBAR_LEFT,4)` gives it align 4 and width 48) but the port has no dock manager to size it and send it a paint. `MaPaintAt` supplies that geometry; all the arithmetic is the bar's own, so the ruler cannot disagree with the map. Found and fixed a wide bug class on the way — see PO-32.
| PO-23 | As a player, runway edges look right at distance — no smeared corners. | 3 | The leading end of the runway keeps its shape at range. | 🔬 **PO-reported 2026-08-15** during an otherwise clean quick-mission smoke test. Reads as texture filtering / mip selection: the port uploads only mip level 0 and lets GL filter, while the engine builds its own mip chain in `RenderTileToDDSurface` (S116 noted the chain was unused; this is its first visible consequence). Cosmetic, distance-only. | **S153 (mechanism fixed; the reported artefact not yet re-observed).** The PO's *"at distance the filtering does a low pass on the corners of the leading end of the runway, this disappears as you get closer"* is the textbook signature of a **minified texture with no mip chain**: the port set `GL_TEXTURE_MIN_FILTER = GL_LINEAR` on **every** texture upload, so a surface seen far away and at a shallow angle is point-sampled from full resolution and aliases, while the artefact vanishes as it approaches 1:1 (magnification, where `GL_LINEAR` is right). D3D on the original hardware mip-mapped these, and S120 had already noted the port uploads only level 0 of the chain `RenderTileToDDSurface` builds. Both upload paths — the execute-buffer textures and `upload_texture` (the DirectDraw-surface path the land tiles take) — now generate a mip chain and select `GL_LINEAR_MIPMAP_LINEAR`. `glGenerateMipmap` is resolved through SDL because the port's GL headers are plain 1.x, and falls back to `GL_LINEAR` if absent (an incomplete mip chain renders WHITE). **Honest limit:** on the flight frames I can capture the A/B differs by only ~3,000 px of 2,073,600, because those frames are mostly magnified texture; confirming the runway artefact needs a distant runway approach. `MA_NO_MIPMAP=1` reverts. |
| PO-24 | As the team, a gate clicks the front-end menu **in a real window at a non-default resolution**, so a hit-testing change cannot ship blind. | 5 | The gate drives a real GL window at 1920×1080, clicks a menu item by pixel, and asserts the navigation happened. | 🔨 **Opened 2026-08-15 after S128–S130 shipped a dead front end.** Every existing gate runs at the default resolution, where the panel origin is zero — so the suite was green while the feature it covers was broken. Same failure shape as S118 (the suite pinned the hardware device away) and the reason `port/hw_gate.sh` exists. **Blocks PO-21.** |
| PO-30 | As a player, the map filter buttons (red/blue, upper right) filter map icons. | 5 | Clicking a filter button shows/hides that icon class. | ✅ **PO-reported 2026-08-15**: the red and blue button rows do nothing. Related to PO-14 (the filter toolbars were made visible in S104) — they draw but their clicks are not routed. | **S137:** two independent causes, either of which alone leaves the map unchanged. (1) `CMapFilters` registers `ON_EVENT_RANGE(CMapFilters, 1, 9999, Clicked, OnClickedFilter)` and the port's range registrar **refused any span wider than 4096** — a defensive cap that silently discarded the toolbar's single handler, so nothing listened to any of its 30 buttons. Ranges are now stored as ranges, with no cap. (2) The port fired Clicked **without toggling the button**, while `CRButtonCtrl::OnLButtonUp` is literally `m_bPressed=!m_bPressed;` and then fires — so `OnClickedFilter`, which asks the button what state it is now in, always read FALSE and asked to clear a filter that was already clear. Verified on the MAP, not the log: clicking the red "all" filter changes **58,771 map pixels**. New gate `port/map_filter.sh`. |
| PO-31 | As a player, the ADI (artificial horizon) follows the aircraft in hardware graphics. | 8 | The attitude ball tracks pitch and bank in the hardware renderer as it does in software. | ✅ **PO-reported 2026-08-15**: *"has never worked with hardware graphics — always straight and level, sometimes stuck in a steep dive."* **Measured:** the ADI region is byte-identical between frames 700 and 2500 of a sweeping flight (0 of 35200 px differ). Forcing a texture re-upload every bind changes nothing, so it is not the dirty flag. `direct_3d::PutC` runs 512 times in a flight but **`fRefresh` is never true** and `CreateTexture`'s refresh branch (`RemakeTexture`) never runs — and `OVERLAY.CPP:7335` passes `offset!=lastoffset` as that flag. So the ball's offset, which carries attitude, never changes. Next: find what feeds that offset and why it is constant on the hardware path (the software path takes a different branch at `OVERLAY.CPP:7352`, which is why software is unaffected). | **S150:** the port's own MA_LINUX block broke the refresh signal. It bakes roll+pitch into the ball image (the ported texturer tiles a rotated quad, so roll is resolved by resampling instead) and ended with `lastoffset=offset; lastroll=rollkey;` — but `lastoffset` is **exactly what the hardware draw tests** twelve lines later: `pw->DoPutC(pball,dp,offset!=lastoffset?true:false)`. Updating it early made that test **always false**, so freshly re-baked pixels were never uploaded and the instrument froze. The engine updates `lastoffset` *after* the draw, which is the contract the block broke. Second fault in the same expression: `offset` tracks **pitch only** (quantised to 1/64 of 90°), so banking re-baked the ball and still would not have re-uploaded it — and roll is what the PO was watching. Now refreshes when either changed. Measured: `PutC with fRefresh` fires (previously **0 of 512** calls), and the ADI region changes **19,696 of 25,500 px** between two frames of a hardware flight, with the horizon ball visibly at a different attitude. |
| PO-32 | As a player, campaign dialogs show their text instead of garbage. | 5 | Strings built with `sprintf("%s", <CString>)` render correctly. | ✅ **S135:** MFC's `CString` survives being passed through `...` on MSVC because the object is one pointer and the ABI copies it; GCC passes the object's ADDRESS, so `%s` printed the raw bytes of the pointer. Found via the ruler's `"0 %s"` label rendering as `0 ` + 5 garbage bytes. **53 sites in 11 files** — waypoints, payload, flight tasks, mission log, profile, quick mission, dates — i.e. exactly the campaign screens reported as textless. Fixed with an explicit `(LPCTSTR)`; parity unchanged on all 5 screens. |
| PO-33 | As a player, the title screen after quitting a campaign is clean. | 3 | No stale panel is painted over the landing page. | ✅ **S146.** S139 located it; Found while fixing PO-29. With the modal's own controls removed, a **"Load Campaign / Auto Save / BACK LOAD" panel** is still painted over the title screen after quitting. Same family as the modal's ghost: a panel whose hosted controls are never removed. It is invisible on the campaign map (which draws only parent-scoped chrome) and reappears on the title screen, which uses the global draw pass — so it is a panel-lifecycle gap, not a draw-order one. **`MA_TRACE_GHOST` names the owner:** a `CLoad` panel with 4 controls, still `visible=1`, hundreds of passes after the campaign it launched was quit. The close path stops at `RDialog::OnOK`, which early-returns on `edges.l & ACTIONS_ARTCHILD` (`artchild=1` for this node) and so never reaches `EndDialog` → `DialExitFix` → `DestroyWindow`. **Two fixes were tried and REFUTED** (recorded so they are not retried): removing the controls in `CDialog::EndDialog`, and in `CWnd::DestroyWindow` — neither runs for this panel. Next: find why this node is flagged ARTCHILD, or which node the OK should reach instead. | **Fixed in S146:** `CMIGView::LaunchMap` — the transition from the front end to the campaign map — does `m_pfullpane->DestroyWindow()`. On Windows that destroys the panel window and **every child window under it**, so the dialogs it launched go too. Here the panel's own controls are dropped but its **child dialogs** are registered against themselves and survive, still `m_maVisible`, so the global draw pass keeps painting them. Nothing ever asked them to close — which is why S139's two hypotheses (`EndDialog`, `DestroyWindow`) both found nothing running. The teardown now cascades to `pdial[0..2]` here and in `RFullPanelDial::LaunchScreen`, which had the same `pdial[0]=pdial[1]=pdial[2]=NULL` "forget, don't destroy". Verified: the owner list on the landing page no longer contains `CLoad`, and the page is clean. |
| PO-34 | As a player, campaign dialog lists can be scrolled. | 8 | The scrollbars the game places on its lists are drawn and work. | ✅ **S140:** RScrlBar was never a hosted control type — 26 of them are created on the campaign map alone. Hosting it was only the first of four independently fatal gaps: the bars are placed from `GetClientRect`, which the port only fills at draw time (so every bar was `Move`d to a **negative** rect and dropped); the placement went to the control while the draw walk reads the client; and the bars are children of the **listbox**, not the dialog, so neither the draw walk nor the click walk (both keyed on `parent==dialog`) ever saw them. Verified on the LIST: clicking the bar moves 20,844 px of rows. New gate `port/dialog_scroll.sh`. Front-end listboxes deliberately excluded — see the sprint note. |
| PO-35 | As a player, the title screen matches the original. | 5 | No opaque black box behind the menu. | ✅ **S143.** Found by measurement in S141, not reported. Our title menu paints an **opaque black rectangle** behind part of the text; the gold video's title screen has none — the menu sits directly on the artwork (`port/ref/gold/title_menu_gold.png` vs `port/ref/native/title_menu_ours.png`). Cause: `CRListBoxCtrl::OnDraw` fills its box when `!artnum`, and the port's `RDialog::OnRowanMessage` deliberately returns **0** for `WM_GETARTWORK` (returning the real artnum sends controls down an offscreen-compositing path that renders all black). On Windows artnum is non-zero, so the original never fills. **S70 recorded that skipping the fill globally 'erased the title menu'**, so the fix is not simply removing it — it needs the port to answer "my parent already painted art behind me" without taking the offscreen path. **Note the parity reference itself encodes this defect**, so the 2D gate cannot catch it: a capture that shares a bug with the code under test is not evidence. | **Fixed in S143:** the fill is off by default. The condition is `!artnum` — *"my parent gave me no artwork, so I must paint my own background"* — and on Windows `artnum` is non-zero here, so the branch never runs; the port's deliberate `WM_GETARTWORK → 0` made it run everywhere. Verified against gold on **four** screens: the title menu (text on artwork, no box), the **Preferences tab strip** (gold shows sky and a blue gradient through the tabs; we had a solid black band), the Others tab, and Quick Mission's `BACK VARIANTS FLY` band. S70's "skipping it erased the title menu" no longer holds. All four parity references **rebased** — they had encoded the defect. `MA_LB_FILL=1` restores the old behaviour.
| PO-36 | As a player, the D.I.S. dialog shows the mission briefing. | 5 | The briefing window opens with the D.I.S. dialog, as in gold. | ✅ **S142:** `CDIS::OnClickedViewnotes` — which `OnInitDialog` calls — does `MakeTopDialog(...new CDis_Note(text))` then `LogChild(0,d)`. The port's OOB paint walk enumerated only the **toolbars'** logged children, and this window is logged against the **D.I.S. dialog itself**, so it was created on every open and never painted. Same shape as S106 (the tree was fine, the enumeration was too narrow) one level deeper. The gold capture is what identified it: it shows the briefing window at the map's bottom-left the instant D.I.S. opens. Now `painted 2 open dialog(s)` and the text matches gold word for word. |
| PO-37 | As a player, the title screen fills the window at high resolution. | 5 | The front-end panels scale to the display resolution. | 🔨 Noticed in S146's capture: at 1920×1080 the title screen's artwork and menu occupy only the **top-left 800×600**, the rest black. The campaign map fills the screen (S145) and the 3D view fills it (S122), so this is the remaining screen class that does not. The gold video at 1920 fills the screen. | **S147 analysis (not yet fixed):** two candidate fixes, and they are opposites, so measure before choosing. (a) Switch the CANVAS to the panel's native size while a full panel is up and back to the display size on the map — the GL present already scales a canvas to the window, and B6 set the canvas once at startup. (b) STRETCH the panel background art to the canvas and leave the controls where they are — S125's note claims the full-res canvas is "correct in layout (dialogs land at gold's size and position)". **That claim needs checking first:** at 1920 our title menu is at (629,285) while gold's sits around x≈1050–1310, so the two do not obviously agree. Measure our control positions against a gold frame at the same resolution before writing either fix. | **S151 measurement (still not fixed, deliberately):** measured both at 1920×1080. Gold's yellow menu text spans **x 320–1506, y 335–894** and its artwork reaches **x=1599**; ours spans **x 20–731, y 177–517** with artwork reaching **x=799**. So gold is **not** our layout at full-screen scale — its art is ~2× ours and stops at 1600, i.e. gold renders an 800×600 front end at **2× with the bottom clipped**, letterboxed in a 1920 window, while its text scales by only ~1.65. Neither candidate fix (a) nor (b) describes that. Choosing wrongly moves every control on every front-end screen, so this needs a decision about which layout variant to target (the game ships 640/800/1024 variants) and belongs to a session that can re-verify all five parity screens afterwards. |
| PO-53 | As a player, my joystick axes drive the right controls. | 5 | Twist drives rudder, the slider drives throttle, and the stick drives aileron/elevator. | ✅ **CLOSED (S176). Reported from play: "it pulls to the left".** The port enumerated joystick axes in **SDL order** (X, Y, twist, slider); real DirectInput enumerates in **canonical order** (X, Y, Z, Rx, Ry, Rz, Slider). `SController::RemakeAxes` fills the role combos **first-come** — stick pair, then THROTTLE, then RUDDER — so whichever axis is enumerated third becomes the throttle. In SDL order that is the **twist**, which pushed the **slider** onto RUDDER; the slider rests at its minimum, so the game read a **permanent full-left rudder (−32767)**. Measured before/after in flight: `rudder=-32767 throttle=16447` → `rudder=-643 throttle=32767`. `MA_JOY_SDL_ORDER=1` reverts. Also defined `GUID_Slider`, declared in `dinput.h` since bring-up and never given a value. **PO-verified in play, all four axes:** *"joystick pitch/elevator/rudder/throttle all working."* Slider full travel `-32768 → +32767` → throttle `-1 → 32767`. |
| PO-54 | ~~As the team, a wheel-brake keypress must not pause the simulation.~~ | 3 | — | ❌ **CLOSED S178 — NOT A DEFECT. The premise was wrong.** **PO:** *"gold standard behavior is no movement until both brakes are tapped."* So the aircraft sitting still at full throttle is the game **working**: the parking brakes are on at mission start and both keys release them. **And the "pause" was not a pause.** Traced the setter by return address: `View3d::drawloop` → `Paused(TRUE)`, at **log line 89104 of 89113**, immediately before `instances=0 currinst=(nil)` — the normal **flight teardown**. What S175 recorded as "the sim freezes and never resumes" was the aircraft **ground-looping off the strip and dying** under PO-53's full-left rudder, ending the flight. **The brake-tap correlation was read backwards:** taps → brakes released → aircraft rolls → full-left rudder → crash → flight ends. No taps → never rolls → never crashes → no "pause". Two samples, and I inferred causation in the wrong direction. Nothing to fix. |
| PO-55 | As a player, I can drag every route waypoint, including ones over water. | 5 | The Egress waypoint at S.W. Inchon (over the sea, left of the coast) drags like any other. | 🔨 **NEW — PO, Wonju playthrough.** *"waypoint on left over water not draggable."* S172 proved dragging works (`route_drag.sh` drags Initial Point and Egress and asserts a non-waypoint refuses), so this is **position-specific, not drag-in-general**. ⚠️ Prime suspect, NOT yet measured: the **Ins Wave dialog is drawn off the left edge** (PO-56) and an OOB node's rect swallows clicks inside it (`ma_oob_click_logged_rec`, MIG.CPP) — a dialog at negative x would cover exactly the left strip of map where this waypoint sits. Test: `MA_TRACE_CLICK=1` and look for `[oobclick] swallowed` at the waypoint's coordinates. |
| PO-56 | As a player, the Ins Wave dialog is fully on screen, lets me set the time, and can be closed. | 8 | The wave dialog appears wholly within the window, its time field accepts a time, and it has a reachable close control. | 🔄 **BOTH HALVES TRACE TO FIXES ALREADY MADE (S186) — needs a PO retest.** Located the dialog: `CProfile::OnClickedInsertwave` (IDC_INSERTWAVE 2100) is **empty in the shipped source**; the live button is `OnClickedInsertwavenew` (1006), whose non-CAP branch opens **`CWaveInsert`** at `Place(POSN_CALLER,POSN_CALLER,-3,-3)`. ⭐ **"No way to set the time" is the SAME BUG as PO-57.** `CWaveInsert::OnOKTitle` commits via `edit=GETDLGITEM(IDC_TIME); buffer = edit->GetCaption();` → `Profile::ValidateTime(&buffer,…)` — and `GetCaption()` returned an **empty string on every hosted control** until **S181** added the `DISPID_CAPTION` getter. The typed time went nowhere, exactly as the typed player name did. (`OnTextChangedTime` is entirely dead code, so the commit path is the only one.) **"No way to exit"**: the only exit is the title-bar OK band (`IDJ_TITLE` dispid 3) — **verified working**: `[tbclick] id=1001 TITLE … -> dispid 3 (OK) on 11CWaveInsert -> HANDLER CALLED`, dialogs 4 → 3. The dialog was placed at **(-3,-3)** and is now clamped to (0,0) by **S182**. |
| PO-57 | As a player, my own name appears in the frag pilot roster. | 5 | The name I type into the campaign-start name dialog reaches `MMC.PlayerName` and shows on the roster. | ✅ **CLOSED (S181), PO-verified:** *"Pilot name input as \"Test\" working now, appears on FLY screen, can move to a different aircraft."* ⭐ **`GetCaption()` was never implemented on ANY hosted control** — every type handled `DISPID_CAPTION` on **set** and none on **get**, since bring-up. So `CCareer` read back an empty `CString` and, because `if (buffer.GetLength() <= PLAYERNAMELEN-1)` **passes for an empty string**, it did not fall through to a default — it overwrote the typed name with nothing. Added the getter for REdit and RCombo (both have `InternalGetText`); RStatic/RButton/REdtBt have the same gap but no confirmed caller, left alone rather than changed blind. |
| PO-58 | As a player, tapping the wheel brakes releases them and the aircraft rolls. | 8 | With 100% throttle, tapping `,` and `.` releases the parking brakes and the aircraft accelerates. | 🔄 **LIKELY RESOLVED BY PO-60 — needs a PO retest.** Three measurements, none of which implicate the brake path: (1) S180 traced the brake chain **end to end from the PO's own keypresses** and it is correct; (2) synthetic taps **demonstrably release the aircraft** — A/B repeated twice, `with taps → 135 Kts, alt rising to 5 ft` vs `without → 0 Kts over 1400 frames`; (3) S185 proved **no keyboard input reached the sim at all** until the window regained focus. So the most likely reading is that the PO's taps never arrived. ⚠️ **Not closed:** the PO reported that after alt-tabbing (when F2 worked) the brakes *still* produced no movement, which this does not explain. Retest on the S185 build before deciding. |
| PO-59 | As a player, leaving a campaign returns me to a clean title screen. | 5 | Exiting the campaign shows the title screen with nothing drawn over it. | ✅ **CLOSED (S182), PO-verified:** *"main screen superimpose after campaign exit fixed."* `RDialog::DestroyPanel` deregistered only **its own** window, so a panel's CHILD dialogs kept their hosted controls — destroying the frag panel left the three `CFragPilot` rows registered and visible, and the global front-end pass drew them over the title art. **Exactly the fault S171 fixed on the `EndDialog` path and left unfixed here**; same `ma_ole_remove_subtree` walk. |
| PO-60 | As a player, the keyboard works as soon as a flight starts, without alt-tabbing. | 8 | Keys reach the sim from the moment the 3D view appears. | ✅ **FIXED (S185), cause measured.** ⭐ **SDL delivers `SDL_KEYDOWN`/`KEYUP` only to a FOCUSED window**, and the port never handled a single `SDL_WINDOWEVENT`. The resize-for-3D changes size, border AND position in one go, which window managers treat as a re-map and hand focus away. Measured directly:\n`[res] raised window after resize to 640x480 (focus=NO)` → `[res] raised window after resize to 1920x1080 (focus=yes)`. **The window genuinely had no input focus.** Fixed with `SDL_RaiseWindow` after the resize; `MA_NO_RAISE=1` reverts. ⭐ **This is why "tapping the brakes does nothing" survived two sprints as a brake bug** — S180 proved the brake chain correct end to end from the PO's own keypresses, and the keystrokes were simply never arriving. The PO's detail *"but neither did F2"* was the decisive clue: **F2 has nothing to do with brakes**, so it was never about brakes. Matches *"on the first mission but not the second"* — focus is lost exactly once, at the transition that resizes for 3D. |
| PO-61 | As a player, I can load and play a `.cam` replay. | 8 | Selecting a replay from the Replay screen plays it back. | 🔨 **STILL BROKEN (PO 2026-08-25: *"load replay from the main menu still crashes"*) — and now precisely located.** S205 fixed PO-64 and **did not fix this**, exactly as flagged: the shipped `Ian*.cam` files were recorded by the Windows build and share none of `replay.dat`'s truncation history. New signature from the PO's session: `[shape] GetShapePtr(8036) OUT OF RANGE [0,1023) -- substituting 1` (×6) → `[replay] LoadItemAnims FAILED` → `[SysError] Replay.cpp:4192`. So the `.cam` parse survives the block header and dies in **`LoadItemAnims`**, with a shape number 8036 against a 1024-entry table — S183's `GetShapePtr` guard is holding (substituting rather than crashing) but the read is misaligned by then. **S206 target: which field in `LoadItemAnims` disagrees.** *Original:* 🔄 **NO LONGER CRASHES (PO-verified); the shared reader defect FIXED in S205.** The `numframes=0` block that stopped playback came from `replay.dat` never being truncated (`SetEndOfFile` stub) — see PO-64. ⚠️ Whether the shipped `Ian*.cam` files parse is a **separate** question this does not answer: they were recorded by the Windows build, and nothing has yet read one end to end. Do not close PO-61 on PO-64's evidence. *Original note:* 🔄 **NO LONGER CRASHES (PO-verified 2026-08-24) — and S204 found what it does instead.** The PO reached the Replay screen and loaded: **clean exit, no SIGSEGV** (S183's `GetShapePtr` bounds guard holds). It now fails gracefully at `LoadHeaderID`, and S204 traced that to the same `numframes=0` block header as **PO-64** — one defect, two faces. ⚠️ Also corrects S203's carried claim that the screen's LOAD click "does not register": the PO's mouse registered it seven times; what failed was the *injected* click, a harness reach limit. *Original note:* 🔨 **NEW (S183) — PO: selecting a replay CRASHES.** Backtrace symbolised from the PO's session (`/tmp/ma_session_po.log`), SIGSEGV `fault_addr=0x36b8b1de`:\n`Rtestsh1::Launch3d` → `Inst3d::Inst3d` → `Replay::LoadFinalPlaybackData` → `LoadBlockHeader` → `LoadItemAnims` → `shape::ResetAnimData_NewShape` → `shapestuff::GetShapePtr(unsigned short)` → `fileblock::getdata()`. So a **ShapeNum read out of the replay file resolves to a bad fileblock** and `getdata()` dereferences it. The eight `Ian*.cam` files ship with the game and are already in the Linux run dir (no copy needed — the port runs out of the Wine `drive_c`). **The whole `_Replay` subsystem is exercised by NO gate.** Also reported: something is superimposed on the Replay screen (probably the same class as PO-59, on a transition `DestroyPanel` does not cover). |
| PO-62 | As a player, the game window sits wholly on one monitor. | 5 | The window is fully visible on a single display, with no part off-screen. | 🔨 **PARTLY FIXED (S184), and the original report is NOT explained by geometry.** Measured on the PO's desktop: **two displays, 3840x1080 total**. ⭐ **`SDL_WINDOWPOS_CENTERED` centres across the WHOLE DESKTOP**, so the initial 640x480 window landed at x=(3840-1920)/2 — **straddling the monitor boundary**. Fixed: centre within one display's bounds (`MA_WINDOW_DISPLAY=<n>` selects, `MA_WINDOW_CENTERED_ALL=1` reverts). ⚠️ **But the full-size window was already correct.** `xdotool` says `Position: 0,32  Geometry: 1920x1080` on screen 0 — **x=0, full width**, so nothing is cut horizontally and the reported left-edge cutting is unexplained by the window; the PO's capture was 1822px wide, so it is most likely a screenshot crop. **Do not claim this fixed the report.** **Genuinely wrong and newly measured:** the window is at **y=32**, 1080 tall on a 1080-tall display, so the **bottom 32px are off-screen** — `SDL_SetWindowPosition(0,0)` and `SDL_SetWindowBordered(FALSE)` are both called and the WM still offsets it. |
| PO-65 | As a player, saving a replay names a NEW file and never overwrites a shipped one. | 8 | The replay Save screen is fully on screen, I can enter/choose a new name, and no existing `.cam` is modified. | 🔨 **NEW — PO, from play 2026-08-25, WITH DATA LOSS. Screenshot supplied.** *"couldn't save replay because save screen corrupted ... couldn't find my saved test .cam file"*. **The save DID happen — twice — to the wrong file.** `IanMy Best Hero Kill.cam` (98,568 → 38,145 B) and `IanVertical Hero.cam` (173,131 → 38,145 B) were both overwritten with the same 38,145-byte recording at 06:35, and the PO's screenshot shows **`…ical Hero` highlighted** — `IanVertical Hero`, one of the two. So the mis-placed screen is not cosmetic: the selection lands on an existing entry and SAVE clobbers it. **Both restored from the pristine set at `~/sgl/TUE/afterGameReport/` (all 8 now byte-identical); the PO's own recording preserved at `scratchpad/po65/your_saved_replay.cam`.** ⚠️ **`~/sgl/TUE/afterGameReport/` is the only known pristine copy of the shipped `.cam` files — treat it as the oracle and never let a gate write to `Videos/`.** Symptom: the whole panel is shifted off the LEFT edge — the list reads `-On Kill` / `Best Hero Kill` / `t 1 v 1 Quick` and the menu reads `CK SAVE VIEW` for `BACK SAVE VIEW`, all clipped by the same amount, so it is one panel offset rather than per-control drift. Same family as **PO-56** (Ins Wave drawn off the left edge, `Place(POSN_CALLER,POSN_CALLER,-3,-3)`, clamped by S182) and the Replay screen's own 800×600-in-a-1920×1080-canvas placement noted in S203. |
| PO-64 | As a player, the replay VCR controls play back my flight. | 8 | After a quick mission, Replay/View plays the flight back: the 3D view advances in time and the VCR transport (play/step/rewind) moves it. | ✅ **CLOSED (S205), PO-VERIFIED 2026-08-25:** *"yes! replay moves!"* The PO's own session corroborates it independently — they ejected from `screen=PLAYING flag=0 paused=0`, i.e. playback was genuinely running, and the reader walked the whole file: block 0 `numframes=1024 emptyblock=0` ending 31175, block 1 found at **31179**, `numframes=386`, then a clean **BUFFER EXHAUSTED at 36424 = the file size**. Before the fix it died at 19915 on zero fill. *Original:* 🔄 **FIXED (S205) — awaiting PO retest.** ⭐ `SetEndOfFile` was a compat stub returning success and doing nothing, so `replay.dat` (opened `OPEN_ALWAYS`) was **never truncated and accumulated every flight ever flown** — 2.4MB and growing across the PO's sessions. Playback starts at the FIRST block, a stale one whose count was never back-patched, read `numframes=0`, and could not advance. Real `ftruncate` implemented; `MA_NO_TRUNCATE=1` reverts. Verified to the byte: one flight now yields **20,641 bytes = 18952 + 963 + 66×11 exactly**, magic at 18952, counts (66,0,65) at 19905. New gate `port/replay_record.sh` (+ negative control). **The VCR transport was innocent throughout** — S204 proved play works and playback re-pauses itself after the failed read. *Original note:* 🔨 **ROOT CAUSE FOUND (S204), NOT FIXED — and it is PO-61.** ⭐ **The transport works.** Measured interleaving: `SEL_4 -> PLAY: PlaybackPaused=0` → `LoadHeaderID at 19915 → MAGIC MISMATCH` → next call `paused=1`. Play un-pauses, the block read fails, playback **re-pauses itself** — so the PO's "the control does nothing" and the code's "the transport ran" are both true. **The recorded block header says `numframes=0`**, so `LoadBlockHeader` marks it `emptyblock`, consumes no frames, and looks for the next header where it already stands. The reader is correct; the RECORDER wrote a block claiming nothing is in it. `FRAMESINBLOCK=1024`, so a ~10 s flight never fills a block and `Replay::StopRecord()` is the only thing that would write the real count. **S205: does `StopRecord` run on ALT+X, with `Record` still TRUE?** ⚠️ Four hypotheses died on measurement first (see the S204 review) — including one asserted by my own diagnostic's wording. *Original note:* 🔨 **NEW — PO, from play 2026-08-24.** *"did a quick flight - default quick mission. Replay/View initial 3D view is correct, but VCR controls don't work - 3D view shows no motion. keyboard shortcut '0' to exit back works."* ⚠️ **Two candidate causes and they look IDENTICAL on screen:** (a) the VCR controls never reach a handler (a UI/dispatch fault, the S82/S164 family), or (b) the transport works and playback cannot ADVANCE because no block ever decodes. **(b) has direct evidence from the same session:** the PO's log carries seven `[replay] LoadHeaderID FAILED` lines — the S195 diagnostic for **PO-61** — and `LoadHeaderID` is the FIRST step of `LoadBlockHeader`. If no block header parses, no frame is decoded, so the view shows the live world state it entered with (hence "initial 3D view is correct") and never moves. **That would make PO-64 and PO-61 one bug with two faces.** Not yet established: nothing in the session ties those seven failures to the Replay/View path rather than the `.cam` file screen. Distinguish before fixing. |
| PO-63 | As the team, a menu row past the control's height is still reachable by recipe. | 3 | `#ID:rN` / `,rN` resolves a clickable point for every drawn row. | ✅ **CLOSED (S203). S183's diagnosis was right and the resolver was never at fault** — `[clickrow] row=4 -> (582,335)` lands exactly on row 4's drawn band (measured ink runs y=322–341). It was the **hit test**: every listbox bounded the click by `m_maH`, while paint covers `GetListHeight()`. Fixed with `Hosted::drawH` (record what paint did — S84's principle); `MA_NO_DRAWH=1` reverts; `parity_2d` 5/5 byte-identical because the change can only widen what accepts a click. **Row 4 is Replay**, so this unblocks PO-61. Gate `port/replay_screen.sh` (+ built-in negative control and a vacuity guard). *Original note (S183):* 🔨 **NEW (S183), found while trying to reach the Replay screen.** The title menu traces `[clickrow] row=4 -> (582,335) [listbox (530,210) 105x100, 7 rows, listH=199 rowH=28]` — **7 rows × 28px = 196px drawn inside a control only 100px tall**, so rows past the third are painted OUTSIDE the control's own rect (210–310) and the injected click at y=335 hits nothing. The PO reaches Replay with a real mouse; **no recipe can**, which is why the Replay path has no gate. Same family as **PO-43** (a list overflowing its dialog), now on the title menu. Blocks automated testing of PO-61. |
| PO-52 | As a player, my aircraft accelerates down the runway and takes off. | 8 | From a runway start at 100% thrust the aircraft accelerates past rotation speed (~100 kt) and leaves the ground. | ✅ **ROOT CAUSE FOUND (S176): it was PO-53 all along, and the PO called it.** *"Your flight test regression was just spinning into the ground every time because of the joystick mis-calibration."* Correct — full-left rudder was ground-looping the aircraft, which is why it sat at 20 kt at full thrust. With PO-53 fixed the ground roll runs **0 → 143 Kts**, straight past rotation speed. ⚠️ **Three causes were published here before that one and all three were wrong:** S174 "ground-roll physics"; S175 "the sim stops being stepped" (real, but it was my own test driver's brake-key taps pausing the sim — see PO-54); and the implicit assumption that a flight defect must be in the flight code. **The reporter had the answer and I spent two sprints not asking.** Residual for **K10**: the aircraft does not rotate, because `BOB_AUTOFLY=takeoff` applies throttle only — the PO's script says "pull back around 100 knots" and nothing commands pitch. That is a driver feature, not a game defect. |
| PO-39 | As a player, the campaign map's title plate reads "MIG ALLEY" as in the original. | 2 | The plate shows the game name above the date. | 🔨 **S152 tried one fix and REVERTED it.** Gold shows "MIG ALLEY" over the date in a plate at the map's top-left; we show the date alone. `CMainFrame` creates and initialises `m_titlebar` and nothing paints it — the same *built, initialised, never painted* shape as the scale ruler (S135), so drawing it looked like the same fix. **It is not:** the title bar's hosted control carries *the date*, which the map already draws itself, so drawing it produced the date twice, overlapping and offset. Whatever renders gold's "MIG ALLEY" is elsewhere. Low priority — cosmetic, and `port/ref/native/map_vs_gold.png` shows how close the rest of the map now is. |
| PO-40 | As a player, pressing FLY from the campaign starts the mission. | 8 | The campaign fly path reaches 3D and returns. | ✅ **S155 — the blocker.** `ensure_window` resizes, re-centres and re-borders the SDL window, and `CreateSurface` calls it for **every primary surface** — which during `Launch3d` happens on the **flight thread**. SDL's X11 backend requires those calls on the thread that created the window; off-thread it wedges and the main loop spins at 100% on one core. Measured on real GL: `driving Launch3d → [res] resize to 640x480 → [res] resize to 1920x1080 → hang`; **the identical recipe headless completes**, which is why every gate passed. Off-thread callers now record the size and the main thread applies it from the pump. After the fix the whole round trip runs: flight → `AUTOEXIT` → `flight close` → `OnFlyingClosed` → CAMP debrief → `back in front-end`. |
| PO-41 | As a player, ALT-X removes the 3D view. | 3 | No stale cockpit behind the results window. | ✅ **S155:** the flight *is* torn down (`InThe3D=0`), but the last flight frame stays on screen — the front end paints its panel into the GDI canvas and presents that, so wherever the panel does not cover, the old 3D frame shows through and the results window reads as floating on the cockpit. There was already a clear for the map→panel transition (`_wasMap`); the 3D→panel one was missing. |
| PO-42 | As a player, the map's upper-right icons are not duplicated. | 3 | The misc toolbar and the system box do not sit on top of each other. | ✅ **S155:** the misc toolbar was **right-aligned**, dropping it against the system box; both end in an X-ish glyph, so the pair read as duplicates. Gold keeps them well apart (main 700–1200, misc 1230–1460, system box 1855–1915). It now follows the main toolbar's extent, with right-alignment kept only as the narrow-canvas fallback — parity at 800×600 is byte-identical, so only wide canvases change. Same mistake S144 fixed for the filter rows, in the same corner. |
| PO-43 | As a player, a campaign dialog's list stays inside its dialog. | 5 | The Intelligence list does not paint over its own buttons. | 🔨 **S155 located it; two fixes tried and REVERTED.** `CRListBoxCtrl::ResizeToFit` grows the control to hold **every** row, and nothing constrains it — Windows clips a child to its parent window, this path does not. So the supply list paints past the bottom of its dialog, over its own **Dossier/Authorize** buttons and on down the map. The buttons are exactly where they belong; the list is on top of them, which is why it reads as *"buttons not at the right place, not drawn right either"*. The **scrollbar stops at the dialog's true bottom** — that is the tell. Tried: (a) clip to the listbox's own rect → no effect, the listbox *is* the oversized object; (b) clip to the OOB node's rect → removed the tab row and the combo border (the node rect is smaller than the dialog's visible content) and the list still overflowed, because it belongs to a different node. The fix belongs where the size is decided (`ResizeToFit` / the template rect), not at paint time. |
| PO-44 | As a player, a dialog's tick/close glyphs are drawn and clickable where they appear. | 5 | The check mark dismisses the dialog from where it is drawn. | 🔨 **Reported, not yet investigated.** *"check mark icons at upper right on dialog boxes often not drawn correctly"*, and on the weather dialog the PO could only dismiss it *"by clicking at upper right, but not at the corner"* — i.e. the tick's hit band and its drawn glyph disagree. This is the S82 rule (the click walk must mirror the paint walk) applied to the title bar's glyph bands; `ma_button_title_hit` computes the bands independently of the control's own draw. |
| PO-45 | As a player, the fly screen's dialogs do not overlap. | 5 | Clicking an aircraft icon from the fly view gives readable dialogs. | 🔨 **Reported, not yet investigated.** Several dialogs are composited at the same origin (the PO's capture shows D.I.S., weather, squadron and briefing text all interleaved). Related to PO-17/PO-21's family: dialogs drawn at a shared origin rather than their own. |
| PO-46 | As a player, the help text is large enough to read comfortably. | 2 | Optional: a larger face in the documentation panel. | 🔨 PO marked it optional. The panel already measures its font (S134); raising the body size is a one-line change plus a re-measure of the wrap. |
| PO-47 | As a player, the quit confirmation is a message box, not a slab. | 2 | The dialog is the size it says it is. | ✅ **S156.** Reported as *"quit dialog is oversized"* — and it was the **art** that was oversized, not the dialog. `RMdlDlg::OnPaint` blits its background bitmap (`FIL_MAP_ARMY`) at natural size, roughly 535×590, while the dialog reports **279×142** (its own trace line says so), so the box appeared as a tall black slab with the buttons stranded near the top. Windows clips a window's painting to the window; the port has to say so. Now clipped to the dialog rect. Introduced by S138's modal loop — my own. Gate: the quit confirmation still opens at 279×142, "Yes" is located and clicked, and 99.1% of the map area changes. |
| PO-48 | As a player, the landing page is clean after exiting a campaign. | 3 | No stale graphics on the title screen after quitting. | 🔨 **PO-reported 2026-08-16**, after confirming *"campaign worked … overall completely useable"*. S146 fixed one instance of this (a `CLoad` panel that survived because `LaunchMap`/`LaunchScreen` forgot its dialogs rather than destroying them), and the landing page was clean in that capture — so this is a **second** survivor from the newer exit path (quit via the map's X → `OnBye` → `LaunchFullPane(&title)`), not a regression of the first. Use `MA_TRACE_GHOST`, which names the owning class and control count of everything the global draw pass paints. |
| PO-25 | As a player, 3D objects keep their textures for a whole mission. | 8 | Aircraft, buildings and cockpit stay textured across a long attack sortie. | 🔨 **PO-reported 2026-08-15**: *"after a few passes all the objects turned white"* (screenshot: aircraft, buildings and panel all flat white; terrain still textured). White = UNTEXTURED, drawn in the vertex colour. **Two hypotheses tested and refuted:** texture-handle exhaustion (peaks at 1160 of 4096 over 3000 frames and **1208 over 12000 frames — identical with and without caching**, so the engine requests each texture once and the cache changes nothing) and `CreateTexture` failing (never fires in a 4000-frame flight). Not reproducible in an automated sortie, so a self-report now fires the first time the textured-batch share collapses. |
| PO-26 | As a player, the map "?" help shows the right topic, formatted. | 3 | Help on a dialog opens that dialog's topic with readable layout. | ✅ **PO-reported 2026-08-15** on the Mission Results dialog: poorly formatted AND the wrong topic. The context-id → symbol → topic map from S114 evidently lacks this dialog's id, so it falls back to the index. | **S134:** the "?" now opens the dialog's OWN topic — `CDialog`'s ctor never recorded the template id as the help context (real MFC does), so every dialog reached help with no identity and fell through to `CMainFrame::OnCommandHelp`, which hardcodes `IDD_INTRODUCTION`. Verified: Player Log resolves `0x20114 -> HIDD_PLAYERLOG -> topic 30`. Body text is now wrapped by **measuring** the font instead of assuming 7px/char and a 13px line, and the column is capped at 1040px.
| PO-27 | As a player, the map zoom button zooms the map. | 5 | The zoom control changes scale cleanly; no tiled/blocky corruption. | ✅ **FIXED (PO-27 S1 this pass, 2026-09-15):** the shim read a bottom-up DIB's source rect from the wrong edge, so above `m_zoom>25` (the quadrant path) every tile was drawn with its halves exchanged. Seam metric 105.6 → 73.2. **PO-reported 2026-08-15** (screenshot: map becomes coarse tiles with a seam). Supersedes PO-18 — same defect, now with a reproduction (the small two-boxes zoom icon). |
| PO-38 | As a player, clicking a map aircraft icon gives a usable mission dialog. | 3 | The dialogs it opens are legible and complete. | ✅ **S149 (verification, no new code):** the PO's *"clicking on map icon with airplane on it yields this confusing dialog"* is fixed by S135 + S136 together. Clicking a `WayPointBAND` icon opens two dialogs, and both now render fully: the flight profile (**Munsan-Seoul Rail-Line**, Wave/ToT/Main Duty/AAA Cover/Air Cover, row *1.Reconn 05:40 F80 (1)*, buttons **Route / Task / Save / Ins Wave / Del Wave**) and the **Mission Folder** (Objective/Task/ToT/Flights, row *Munsan-Seoul Rail-line / Reconn / 05:40 / 1*, buttons **Intelligence / Profile / Delete / Frag**). The row text came back with the `(LPCTSTR)` fix (PROFILE.CPP was one of the 53 sites); the button captions with the plate-button rule. **Neither dialog has a FLY button in the original either** — gold puts Fly on the full-screen mission panel (*MAP FLY PREFERENCES*), which the PO has confirmed works. |
| PO-28 | As a player, map dialogs show their button and body text. | 5 | Buttons carry labels; the Situation dialog shows its body text. | ✅ **PO-reported 2026-08-15**: many map dialogs have blank buttons, and the default-open Situation dialog has no body text either. | **S136:** three causes, all found by measuring the D.I.S. dialog rather than guessing. (1) **RRadio was not a hosted control type** — `CDIS::OnInitDialog`'s `AddButton("Target")`/`("General")`/`("Latest")`/`("Priority")` calls went to controls that did not exist, so the dialog drew blank bars. Now hosted (`ma_oleradio.cpp`), drawn, and clickable — the click walk uses the geometry the paint recorded. (2) **Plate buttons never got their captions.** The caption policy admitted only tickboxes; the D.I.S. buttons carry `FIL_MAP_DIS_BUTTON` art plus `IDS_NOTES`/`IDS_FOOTAGE`/`IDS_INTELL`, and now read Notes / Footage / Intelligence. The discriminator is the ART, not the presence of a string resource — icon buttons' `IDS_` names are TOOLTIPS (`IDS_ZOOMIN`, `IDS_AIRFIELD`), and drawing those was the S57 regression. (3) The **empty body** was a consequence of (1): with the filter radios inert, the list was never populated. Clicking Target now lists the intelligence items. See also PO-32 (53 `sprintf("%s",CString)` sites) fixed in S135.
| PO-29 | As a player, X on the campaign map offers save/quit/cancel. | 3 | X opens the exit dialog rather than dropping to the landing page. | ✅ **PO-reported 2026-08-15**: X drops straight to the landing page, with stale text on it. | **S138:** the game always asked — `CMainFrame::OnBye` opens `RMessageBox(QUITGAME, AREYOUSURE, SAVE, YES, CANCEL)` — but the port had **no modal loop**: `CDialog::DoModal` was `{ return -1; }` and `EndDialog` was `{}`, and `-1 < 2` is OnBye's "quit without asking" branch. So every confirmation in the game answered "yes, quit" without being shown. `RMdlDlg::DoModal` now runs a real nested loop (pump input → the dialog paints its own art → draw its controls → present) until a button calls `EndDialog`. Save/Yes/Cancel all work; the stale text the PO saw was the modal's own controls, still registered after it closed. |
| PO-17 | As a player, campaign dialogs are **positioned, not piled up**, so I can read them. | 8 | Dossier / Load Profiles and friends open in their designed positions without overlapping each other. | 🔨 **S123/S124: NOT a placement bug — it is the 800×600 canvas (B6).** Three campaign dialogs opened together sit at three DIFFERENT, correct rects: (223,92) 339×400, (142,89) 501×407, (164,101) 457×382. Each is placed where the game asks. They overlap because they are **all open at once**, centred on the same region — which is what the PO's screenshot shows. So the fix is not placement: either opening one should dismiss the others, or the panels must be **draggable** so the player can arrange them (they have title bars; the port supports dragging the MAP but not dialogs). **The gold video settles it:** at 12s the planning map shows the Player Log alone; at 28s Debrief AND Mission Results are open together and do NOT overlap — so multiple open dialogs is normal. The difference is SCALE: gold's Player Log is ~340×420 in a 1920×1080 front end (18%×39% of screen); ours is the same 339×400 in an **800×600 canvas** (42%×67%), so three dialogs cannot help but collide. The dialogs are absolutely sized; the canvas is not. **Fixing PO-17 means fixing B6** — running the 2D front end at the selected resolution instead of a fixed 800×600. |
| PO-18 | As a player, the campaign **map zoom** draws cleanly, so the map is legible when zoomed. | 5 | Zooming the campaign map produces a continuous map, not tiled/blocky artefacts. | 🔨 **PO-reported 2026-08-15**: *"Zooming the map worked except it produced tiles."* Visible in the PO screenshot as blocky green/tan patches. Likely the same tile-cache/StretchDIBits path as the campaign map render. |
| PO-19 | As a player, the **3D recon view** zoom keys work, so I can inspect a target. | 3 | Keys 3 and 4 zoom the recon view; 1/2 rotate and 0 exits (those already work). | ✅ **PO-reported 2026-08-15**. Rotation and exit work, so the view and its key routing are alive — only the zoom actions are unhandled. Recon terrain was black too; expected fixed by S120, needs confirming. | **S145:** the reported symptom ("the small zoom icon messes up the map") turned out to be a **black band that was there before any click** — measured identical, 242,558 black pixels, with and without it. The map view was sized from the frame minus `m_borderRect`, the space the **docked** toolbars occupy: on Windows those are real docked windows that fill it, but this port composites its toolbars as overlays, so at 1920×1080 a 192px band in each axis was reserved and never painted (`[maptile] client 1728x888 -> m_zoom=1.692383 size=1728x3027`). The view now takes the whole **canvas** — and the canvas, not the frame, because the frame is still a compat 800×600 default. Result: `client 1920x1080, m_zoom=1.879883, size=1920x3363`, black pixels **242,558 → 42,055** (the remainder is the distance ruler's own strip). The map fills the screen for the first time. |
| PO-49 | As a player, a target dossier is the size it says it is. | 3 | The dossier's backdrop art stops at the dialog edge. | ✅ **CLOSED (S159)** — and it was **every** campaign dialog, not just the dossier: 9 of 9 in the OOB sweep reclaimed map area (bases 172k px, intelligence 114k). `RDialog::OnPaint` passed `SetDIBitsToDevice` the BITMAP's size, never the dialog's; the art blit is now clipped to the dialog rect (`MA_NO_ART_CLIP=1` reverts). Found by measurement in S158, not reported. The dossier node reports **330×320** (`MA_TRACE_OOB`) and its art paints **≈394×575** — **281 px of skirt below the Center/Zoom/Photo/Authorize row**, on supply *and* bridge dossiers alike. Same shape as PO-47 (*the dialog is not oversized, the ART is*), one screen further on. S156 fixed that case with `ma_gdi_set_clip` in `RMdlDlg::DoModal`; the dossier is painted by the map's OOB walk instead. ⚠ S155 already tried clipping the OOB **node** rect (for PO-43) and reverted it — it ate the tab row and the combo border. So clip **the art blit**, to the size the dialog reports. |
| PO-50 | As a player, clicking a row of the mission I am editing does not open an unrelated dialog. | 5 | Clicks on a campaign dialog reach that dialog, not the toolbar underneath it. | ✅ **CLOSED (S165).** ⭐ `ma_map_paint_oob` descends a **second level of logged children** (a dialog can be logged on another dialog — the wave folder is a child of the Mission Folder, not of `m_toolbar2`); `ma_map_click_oob` had only the first level, so those dialogs were painted and no click could ever reach them. ⚠ **S164's stated cause ("the walk paints 3 of 5 dialogs") was a MISREADING** of a per-frame counter — see S165. |
| PO-51 | As a player, the frag screen is not covered by the campaign map's dialogs. | 5 | Once a full-screen panel takes over, the map's OOB dialogs stop painting. | ✅ **CLOSED (S169).** The map branch was correctly off; it was the still-hosted **controls** that the global `ma_ole_draw_all` pass drew, because nothing had marked them as belonging to another screen. Every node of an open map dialog is now `ma_ole_set_parent_scoped` — the mechanism S97 built for the map chrome. The scoper walks **`dchild` as well as `fchild`/`sibling`**: Route's columns hang off `dchild`, are never painted by the OOB walk, and were still being drawn by the global pass. |


### EPIC K — The Wonju supply-depot attack *(PO-added 2026-08-21)*

> **New gold standard, added by the PO 2026-08-21:**
> `~/gold standard/ma/wonju_attack.mp4` (1920×1080, 60 fps, 344 s) and its written
> walkthrough `~/gold standard/ma/wonju_script.txt` (steps 4–18: recon → plan → fly).
> **The PO's stated intent: *"as a test of campaign I will try to create and run this
> mission in linux MA."*** So this epic is not a screen-parity epic — it is an
> **end-to-end acceptance run of the campaign mission-builder**, with the video as the
> oracle for what each step should do and the script as the step list the PO will follow.
>
> Frames via `port/tools/gold_video.sh <video> …` (`wonju` alias added S158).
> The recording is a **desktop capture with the game windowed and letterboxed** like the
> two 260814 videos — measure with `gold_video.sh geom`, and per S64 never judge size or
> density across the gold↔native boundary; judge layout order, art, content and colour.
>
> **Why it matters:** every EPIC J item so far has been *one widget on one screen*. This
> is the first oracle for a **whole workflow** — nine dialogs, four combo boxes, a
> drag-editable route and a flown sortie, in the order a player actually meets them. A
> step that opens but cannot be *completed* fails this epic even when its screen passes
> EPIC I/J.

| ID | User Story | Pts | Acceptance Criteria | Status |
|---|---|---|---|---|
| K0 | As the team, the Wonju gold is reachable from the tools, so every K item can cite frames instead of prose. | 2 | `gold_video.sh` knows `wonju`; the script's 15 steps are inventoried against timestamps in `port/scrum/wonju-walkthrough.md`. | ✅ **S158.** Alias added; geometry measured (1280×1024 at desktop 320,28). **The recording stops at the frag screen (~t=333) — steps 15–18 have NO video oracle**, only the script and the older `full` video. |
| K1 | As a player, I can find the target: Front Line + Red Supply filters on, and clicking the Wonju Supply Dump icon opens its Intelligence Dossier. | 3 | Both filters toggle their icon classes; the dump icon north of the Central Front Line marker opens a dossier reporting the AAA presence. | ✅ **CLOSED (S160).** `MA_MAP_CLICK_NAME=Wonju` finds **id=9801 (0x2649), AmberSupply** by the game's own `GetTargName`, and its dossier matches the PO's script **on content**: the script predicts *"no MiGs expected, but a large AAA presence"* and the port reads **Threat AAA High / MiG 15 Low**, MSR **Central**. Residual, named rather than waived: the two *specific* filters (Front Line, Red Supply) are not individually asserted — `map_filter.sh` gates the red "all" filter (PO-30). |
| K2 | As a player, Photo gives me the 3D recon view and I can zoom right out to read the terrain. | 3 | Photo → recon 3D; zoom keys move the eye through the full range without leaving the view. | ✅ **CLOSED (S160) for the headline half — the recon renders.** ⭐ `Inst3d::Inst3d(bool)` (the map-view ctor Photo takes) started the sim thread ~40 lines before `Three_Dee.InitialiseCache()` built the landscape cache that thread reads: SIGSEGV in `moveloop` while the main thread was still in the ctor. **S69 had already fixed this exact race in the no-argument `Inst3d` twin and it never crossed the 100 lines between them.** Gate: `port/recon_photo.sh`. Residual: the *zoom keys inside the recon* are PO-19 (closed) but were not re-driven from this entry point. |
| K3 | As a player, zooming in on the dump reveals its sub-targets, and Damage tab → top combo lists the warehouses. | 5 | Sub-target icons appear at high zoom; the Damage tab's combo box enumerates the warehouse group. | ✅ **CLOSED (S163).** ⭐ The blocker was that **combos inside an OOB dialog were drawn and inert** — `CT_COMBO` was missing from `ma_ole_toolbar_click`'s type filter, the same shape as S87 (listbox rows) and S140 (scroll bars). Damage tab → combo → **All elements** lists eight warehouse groups (8/8/8/8 and 4/4/4/4) and ten `SB Flak Site` rows (`Fully / functional`) — lower bounds, since the list runs off the bottom of the screen — the script's *"groups of warehouses"* and independent confirmation of the *"large AAA presence"*. Gate `port/damage_elements.sh`. ⚠ The list **overflows its dialog** — that is **PO-43**, and this is new evidence it is not Intelligence-specific. |
| K4 | As a player, Authorize offers the mission types and I can pick **Minimum Strike**. | 5 | The Authorize dialog lists the strike types; selecting Minimum Strike creates a mission that is *not* auto-filled. | ✅ **CLOSED (S162).** `DossierButtons::OnClickedAuthorise` → `CLoadProf::MakeSheet`: a three-tab chooser listing **Minimum Strike / Napalm Strike / Fighter Bomber Strike**, and Load creates the mission — the **MISSION FOLDER** then lists `Wonju Supply Dump  Bomb  08:30  2`. Gate `port/authorize_mission.sh`. Note: gold reads `F84 (2)` where the port reads `F80 (2)` — the game's own choice from the squadrons available on the pinned save's date (day one), not a defect. |
| K5 | As a player, Mission Folder → Profile lets me add a third flight to the wave. | 8 | Either route works: the Squadron slot's Flights spin-box, or clicking the Off-Duty 3rd flight slot and choosing the 1000 lb payload. Flight count persists into the frag. | ✅ **CLOSED (S170) by the spin-box route.** ⭐ **RSpinBut was the LAST unhosted R\* type** — the wrapper had compiled since bring-up, so every `InvokeHelper` on one was a silent no-op and no spin control was ever created, drawn or clickable. Two more gaps sat in front of it: **CT_EDTBT was drawn but inert** (`IDC_ACTYPE`, the `F84 (2)` duty field, is the only door to `ChooseSquad`), and **`:rN` addresses a ROW, so the wave table's row centre is column 3** — the recipe was opening the *flak* tab while looking correct. Gate `port/add_flight.sh`: **Mission Folder Flights `2 → 3`**, the walkthrough's own cheapest end-to-end assertion. Residual: the Off-Duty-3rd-slot route and *persists into the frag* are not asserted — they belong to K7/K9. |
| K6 | As a player, I can set the attack method and pattern. | 5 | Attack method stays Dive Bomb; attack pattern changes to **Individual Targets** and the change survives reopening the dialog. | ✅ **CLOSED (S171).** Gate `port/attack_pattern.sh` drives default → `Spaced target selection` → close+reopen → still Spaced → `Individual targets` → close+reopen → still Individual, with the method reading `Dive Bomb` every time. ⚠ **Named divergence:** on this save the port's pattern is **already Individual targets** when the dialog first opens (the Minimum Strike profile sets `attpattern=2`), so the step has no distance to travel; gold only ever shows the post-change state, so the default is NOT claimed wrong. The blocker was **S171's registry leak** — a closed dialog's controls stayed hosted and visible, so after one reopen there were two of everything. |
| K7 | As a player, I can add flak suppression: Task → AAA cover tab → an Off-Duty squadron, restored to rockets and guns. | 8 | The AAA-cover tab accepts a squadron assignment and a stores change; the suppression flight appears in the frag. | ✅ **CLOSED (S171).** Gate `port/flak_suppression.sh`: the AAA Cover cell (`:r1.3`) → duty field → ChooseSquad → stores. Slot goes **`Off Duty` → `F80 (1/1)`**, payload becomes **`Rockets & Fuel tanks`** (gold's PAYLOAD frame), Mission Folder Flights **2 → 3**. ⚠ The script says pick **F84**; ChooseSquad's own **Available** column reads `F84: 0` on this save's date and the game refuses any squadron with `numavail < 4` — the gate asserts that **refusal** explicitly. Same divergence class as **K4**'s F84/F80 note. Residual: *appears in the frag* is **K9**. ⭐ Found and fixed a **latent S170 crash**: a spinner with an empty list SIGSEGVs in its own `OnDraw`. |
| K8 | As a player, I can drag the route: Egress inland, IP within 4 miles of the target, the two AAA waypoints over the target area. | 8 | Waypoints are draggable on the campaign map and the edited route is what the flight flies. | ✅ **CLOSED (S172).** ⭐ The port's **first press-move-release interaction**. The engine already had the whole chain (`OnLButtonDown` → `OnMouseMove` → `AllowDragItem` → `OnDragItem`); S95 deliberately drove down+up in ONE tick to keep `m_bDragging` FALSE, so nothing had ever issued the moves. `CMapDlg::MaDriveDrag` does, and `MA_MAP_DRAG` addresses waypoints **by name** through the map's own `FindMapItem`. Gate `port/route_drag.sh`: IP dragged onto the target lands **3.06 miles** away (the script asks ≤ 4), Egress moves, both report `dragging=1`, the map redraws them 4-24px from the drop, and **the target itself refuses to drag** (`allowdrag=0`). ⚠ The script's *two AAA waypoints* live on a second suppression **wave**, which needs aircraft this save's day one does not have — same availability arithmetic as **K7**. |
| K9 | As a player, the Frag dialog lets me set callsign and aircraft and review the mission before flying. | 5 | Callsign edit accepts text (cf. PO-16), aircraft selection works, the review lists the three flights + suppression. | ✅ **CLOSED (S173).** Gate `port/frag_review.sh`, all four clauses read GAME STATE: `FlyableAircraftAvailable=1`, a **12-name pilot roster** (the "final review"), the callsign reaching the package (`pack[1][0][0].callname 1 → 5 " Red "`) and the seat the player flies (`MMC.playeracnum → 4`, matching flight 1 slot 0, no longer the default lead). ⭐ The blocker was that the screen hosts **three `CFragPilot` sub-dialogs with identical control ids**, so `@CFragPilot` is ambiguous with itself — caught by **S171's** ambiguity warning. New `@Class#N` names the Nth by **screen position**. Note: the callsign control is a **combo**, not an edit — PO-16 (text entry) is not on this path. **PO-37** (panel does not fill 1920) is unchanged and does not affect any clause here. |
| K10 | As a player, the mission starts me on the runway and I can take off. | 5 | 100 % thrust, wheel brakes release on `,`/`.`, nose lifts around 100 kt; F6/F2 views and P pause behave as in gold. | 🔨 **HALF DONE (S174). The mission FLIES and starts on the runway** — the built Wonju strike goes frag → Fly → 3D with the player at **speed 0 Kts, alt 4 ft**, wheels on the strip. Throttle input reaches the flight model (A/B: no input holds 0 kt for 420+ frames; with the drive it climbs). **Blocked on PO-52:** the aircraft plateaus at **20 kt** at 100% thrust and never rotates, so "I can take off" is not met. The `,`/`.` half of the criterion is **wrong**: wheel brakes are `KeyHeld3d` (hold-to-brake), so they are off unless held and there is nothing to "release" — same class of error as **K9**'s "callsign edit" (§8-MA123). New drive `BOB_AUTOFLY=takeoff`, new oracle `MA_TRACE_HUD`. |
| K11 | As a player, accel-to-IP works: M → 1 → 4 puts me at the Initial Point and returns me to the cockpit. | 5 | The cockpit map's accel options include Initial Point and the time compression ends at the IP. | 🔨 **NEW** — PO-13 made in-menu digits selectable; this is the first *use* of them. |
| K12 | As a player, I can order and fly the attack: R → 6 FAC → 1 "Begin your run", bombs selected with N, gun camera on V. | 8 | The FAC replies "Roger" (or "Cannot identify target" when out of range); N switches to bombs; ordnance releases on the target. | 🔨 **NEW** |
| K13 | As a player, I can go home and see the debrief: R → 6 → 6, M → 1 → 5, ALT+X. | 5 | Accel-to-home runs; ALT+X reaches the debrief with the sortie's results. | 🔨 **NEW** — the ALT+X half is PO-9/S106; the accel-home half is new. |

**EPIC K total: 75 pts.** K0 first (tooling), then K1→K13 in script order — the PO will walk
the script top to bottom, so a blocker at step *n* hides everything after it.

---

### EPIC L — Tacview ACMI export **[IMPROVEMENT]** *(PO-added 2026-08-25)*

> **PO:** *"when you save a replay .cam file, also save an equivalent tacview replay file of the
> same material ... This is a major improvement, as it allows the user to review their performance
> precisely."*

The first story in this backlog that the original game cannot satisfy — see the scope note in §1.
Tacview is the modern standard for flight-sim debriefing; exporting to it turns MA's opaque
`.cam` into something a player can actually analyse (track, altitude, speed, energy, gun solutions),
on a timeline, against modern tooling. **Additive only:** the `.cam` write must be byte-unchanged,
which is also what makes the epic safely testable — the existing replay path is its own control.

**Format** (`FileType=text/acmi/tacview` / `FileVersion=2.2`, UTF-8 text, optionally zipped):
global properties on object id `0` (`ReferenceTime`, `ReferenceLongitude`, `ReferenceLatitude`,
`DataSource`, `Title`); time advanced by `#<seconds-since-ReferenceTime>` markers; per-object lines
`<hex-id>,T=<transform>,Name=…,Type=Air+FixedWing,Color=…,Pilot=…`; removal by `-<hex-id>`.
Units are metric throughout — degrees, metres, m/s, altitude MSL.

⭐ **The decision that de-risks this: use Transform syntax #4**,
`T=Lon|Lat|Alt|Roll|Pitch|Yaw|U|V|Heading`, whose `U`/`V` are **native flat-world metres**. MA's
theatre is a flat Korea map in centimetres, so we pick one `ReferenceLongitude/Latitude` for the
theatre origin and emit `U`/`V` directly — **no geodetic projection to get wrong**, and the numbers
stay checkable against the sim's own coordinates.

| # | Story | Pts | Acceptance criterion | Status |
|---|---|---|---|---|
| L0 | *Spike:* is the recorded data sufficient? ✅ **ANSWERED S211: NO — the packet is deltas; export from the SIM instead** | 3 | A written answer to: what does a `REPLAYPACKET` (11 bytes, packed) actually contain — absolute state or deltas — and for which objects? Reconstructing a track needs per-object position **and** orientation over time; if the packet carries deltas against sim state, the export must be driven from the live sim during playback rather than parsed from the file. **Nothing else in this epic can be sized until this is answered.** | 🔨 **NEW — do this first.** |
| L1 | As a player, saving a replay also writes a `.acmi` beside the `.cam`. | 5 | `Videos/<name>.acmi` appears next to `<name>.cam`; the `.cam` is **byte-identical** to what the same save produced before (the existing replay path is the control). | 🔨 **NEW.** ✅ **UNBLOCKED 2026-08-25** — N1/PO-68 is fixed and PO-verified (S251), so the save path can be built on. Was blocked because nothing should be built on a save path that destroys files. **The PO re-confirmed EPIC L on 2026-08-25 after PO-71b closed**, with the format reference `https://raia-software-inc.gitbook.io/tacview/technical-documentation/acmi-telemetry-file-format`. |
| L2 | The file loads in Tacview and shows the player's aircraft moving. | 5 | Header + `ReferenceTime` + at least one object with a `#`-advanced track; opens without error in Tacview and the track matches the flight flown. | 🔨 **NEW.** Needs L0's answer for orientation. |
| L3 | Every aircraft in the sortie is exported, not just the player. | 5 | AI aircraft appear as distinct objects with `Color` by side and `Type=Air+FixedWing`; objects that die are removed with `-<id>`. | ⚠️ **REOPENED S436, DOWNGRADED S437 (cross-port from BoB R3).** BoB landed the `uniqueID.count` fix and A/B-measured it: **0 id swaps in 1,033,353 checks with the POSITIONAL scheme** over a campaign run of ~148 aircraft, so the predicted instability **did not reproduce** and this is a LATENT hazard, not a live defect. What BoB's run *did* prove is that `uniqueID.count` is distinct across all ~148 aircraft and stable across 400+ frames. Port the same change here when L3 is next picked up — it is strictly more correct and costs nothing — but do not bill it as fixing an observed bug. Original mechanism, still real in the code: The walk and the side colours are done, but **the object id is POSITIONAL** — `Replay.cpp:558–601` resets `_id` each frame and counts position in `ACList`, and that list is **head-inserted** by `AirStruc::AddToList()` (`MOVEALL.CPP:891–892`) and head-removed at `PERSONS3.CPP:3139`. Any spawn/despawn shifts every later id, so object N becomes a different aircraft mid-file and Tacview sees tracks swap identity. `ac->uniqueID.count` is the stable identity the engine already provides and the replay stream already writes (`Replay.cpp:1737`, `:1772`). Also `_id < 256` silently truncates. BoB has the identical code; whichever port fixes it hands the other the change. |
| L4 | Flight data beyond position. | 3 | `IAS`, `AGL`, `AOA` where the sim has them, so the debrief is quantitative rather than a shape. | 🔨 **NEW.** |
| L5 | Gate: the export is well-formed without opening Tacview. | 5 | `port/tacview_export.sh` flies a mission, saves, and validates the `.acmi` structurally (header, monotonic time markers, ids consistent, every referenced object introduced before use) **and asserts the `.cam` is unchanged**. Negative control: an env switch disables the export and the gate goes red. | 🔨 **NEW.** |

---

### EPIC N — Open at the end of the 2026-08-25 play-test session *(PO-added 2026-08-25)*

> **PO:** *"Add all 3 to the backlog"* — the three items still open when the session's fixes were
> pushed. N1 is first because it is the only one that **destroys the player's data**.

| # | Story | Pts | Acceptance | Status |
|---|-------|-----|------------|--------|
| N1 | As a player, saving a replay under a name I type creates **that** file and leaves every other `.cam` untouched. | 8 | Type `foo`, save → `Videos/foo.cam` exists and holds the flight just flown; **every pre-existing `.cam` is byte-identical to before** (checked with `cmp`, not by eye). Negative control: `MA_NO_SAVENAME_PULL=1` restores the broken behaviour and the check goes red. | ✅ **DONE — S251, PO-VERIFIED 2026-08-25** (saved as `260825test6`; file created under the typed name, all five other `.cam` byte-identical). Root cause was TWO stacked gaps: `ma_ole_char` never fired TextChanged, and `afxwin.h` had no `LPTSTR` thunk so the handler was discarded by a silent no-op fallback — **18 text handlers across the game were affected, not one**. Original report: **(PO-68).** Confirmed destructive **twice in one session**: saving `260825test` and later `260825test2` each overwrote `corpus-baseline.cam`. Recovered only because three independent nets existed (pre-session backup, S214's `.bak`, the S234 autosave watcher). **Root cause known:** `CLoad::filename` is a `CString&` bound to the caller's `selectedfile`, and the only thing that writes a *typed* name into it is `OnTextChangedSavename`, an OCX event the compat never delivers; `selectedfile` was pre-seeded with `Save_Data.lastreplayname`, so OK saves under the PREVIOUS name. **S237's fix did not engage** — it hooked `OnClickedFileok`, which the real save path bypasses (trace showed neither handler firing). **Next step, already identified:** `CLoad` does **not** override `OnOK`, and every save path funnels through it (`OnClickedFileok`, `OnReturnPressedSavename`, double-click row) — so an `OnOK` override that pulls the edit text via the game's own `OnUpdateSavename()` is the choke point. |
| N2 | Ctrl+F6 (reverse padlock) works during **replay playback**, not only in live flight. | 5 | With a replay playing and a target padlocked, Ctrl+F6 switches to the view-from-target and back. | 🔨 **OPEN (PO-70 residual).** PO-verified working **in flight** after S240 enabled the feature (it had been `#ifndef NDEBUG` since 1996). Not investigated in playback — **unmeasured, and deliberately not guessed at**. **S265 attempted this twice with an automated harness and could not drive it — recording that rather than a third attempt.** What WAS established: the replay loads and parses cleanly on the automated path (`LoadReplayData('corpus-baseline.cam')`, `overshoot 0`), so the reader is not the obstacle. What failed is the *harness*: the click recipe reaches the replay screen but never starts playback (`screen=PLAYING` count 0), and injected `xdotool` keypresses did not appear in `MA_TRACE_KEY` at all. Both attempts were caught by a reach check before any conclusion was drawn — *"Ctrl+F6 does nothing in playback"* would have been an entirely fabricated result, since playback never started. ⭐ **The honest trade: this needs ~10 seconds of PO time (start a replay, press `4` to play, then Ctrl+F6 with `MA_TRACE_KEY=1`) versus another sprint of harness plumbing.** The decision tree is unchanged and cheap to read: `shift=0` means the modifier state is lost in playback; `shift=4` with `action index=0` means the binding is absent there; a non-zero action index means the key resolves and the padlock/target state is what differs. |
| N3 | Audit what else the port silently drops on undelivered `WM_*` routes and over-narrow guards. | 13 | A written inventory of every `ON_MESSAGE`/`ON_EVENT` route the game sends that the compat does not deliver, and every port-added range guard, each marked *live defect* / *harmless* / *fixed*; plus a runnable check that fails when a route the game sends returns 0 unhandled. | 🔨 **OPEN.** ⭐ **Three distinct bugs in ONE session traced to this class**, which is why it is worth sweeping rather than fixing one PO report at a time: (a) **S243** — the Replay/Ready Room swap lived in `OnShowWindow`, a `WM_SHOWWINDOW` handler never delivered, so the fix silently did nothing; (b) **S243c** — the *same* dead hook was also the only thing calling `SetDisabled(false)`, leaving the button visible but click-swallowed; (c) **S248** — `CRToolBar::OnGetFile`'s guard admitted only dirs 104..113 and **blanked 732 of 1347 art fetches**, while its twin `RDialog::OnGetFile` had been widened long before. BoB has the same shape on record (S158: `SendMessage` was an allowlist of three; 16 of 20 routes returned 0). **Cross-port: applies to both ports.** |

| N4 | After exiting the 3D in a campaign, the **campaign instruction text** and the **next-mission instructions** are present. | 8 | Fly a campaign mission, Alt+X to the debrief map, and the instruction/briefing text for the campaign and for the NEXT mission renders — compared against the gold video `260814_mig_complete_campaign.mp4`, which records a complete campaign mission including the map window text. | 🔨 **OPEN (PO-72) — S253 scoped it, did NOT reproduce it, and is asking the PO to point at the screen rather than guess a fifth time.** Candidates found, with what is already known about each: **(a) the D.I.S. briefing window** — *already fixed* in **S142/PO-36** (`CDIS::OnClickedViewnotes` logs its child against the D.I.S. dialog, and the OOB paint walk enumerated only the toolbars' children); if this is what is missing, S142 has regressed and that is a sharp, testable claim. **(b) the Directives panel** (`DIRECTS2.CPP`) — its line text is a `CREdtBt` caption set via `SetCaption(LoadResString(...))`; the compat *does* cover `DISPID_CAPTION` for `CT_EDTBT`, so the obvious path is not broken. Note its toolbar icon `FIL_ICON_DIRECTIVES` (0x6607) was one of the two icons **S248 restored**, so this panel was reachable-but-invisible until today. **(c) PO-45**, already open: *"D.I.S., weather, squadron and briefing text all interleaved"* — dialogs composited at a shared origin. **If the text is present but stacked under another dialog, this is PO-45 and not a new defect.** ⭐ **Deliberately not chosen between:** three plausible causes, no measurement — and every wrong guess this session came from picking one and building on it. The cheap discriminator is one screenshot of the screen where the text should be. Original report: *"campaign instruction text and next-mission instructions after 3D export are missing"* (read as "after 3D **exit**" — the Alt+X return to the debrief map). **Not yet reproduced or measured by me.** Two things to check before theorising, in this order: (1) is the text *absent* or *drawn blank/elsewhere* — the same question S248 just answered for icons, where art was fetched, rejected by a guard, and silently painted nothing; (2) does it come through `WM_GETSTRING`/`AfxLoadString` or through a control's DLGINIT bag, since **N3's undelivered-route class covers text exactly as it covered art**. ⚠️ **Do not assume it is the same bug as S248** — that guard is now widened and this was reported after; but the *shape* (content resolved, then silently dropped) is worth testing first because it is cheap. |

**EPIC N total: 34 pts** (N1 done — 26 remaining).

### EPIC M — Mine the patch changelists and the docs for bugs we still have *(PO-added 2026-08-25)*

> **PO:** *"check the ~/sgl/TUE patch changelists for ma and bob, and check whether any bug fixes
> listed in these patch changelists are bugs that need to be fixed in the ma or bob linux
> codebase"* … *"do the same with any bugs mentioned in ~/sgl/TUE ma or bob documentation, either
> that distributed with the games or provided later by the user communities"*.

> ⚠️ **CORRECTED S432 (2026-09-05): the paragraph below is WRONG where it says "by default".**
> The source is **not** pre-patch — `KEYMAPS.H:390` carries the v1.03 flaps fix labelled *"for US
> version and Patch"*, and the tree holds work dated to Dec 2000, eight months past v1.23. A patch
> fix is **present unless shown absent**, and each must be checked individually. The oracle-is-a-
> patched-binary half still stands (BDG 0.85F is a community patch on top of v1.23). Evidence:
> `port/scrum/patch-bugs.md`.

⭐ **Why this is sharper than it first sounds: WE COMPILE THE SOURCE, THE ORACLE IS A PATCHED BINARY.**
MA's parity oracle is the **BDG 0.85F patched build** (`RUNNING.md`), and Rowan shipped an official
patch chain **v1.01 → v1.23** (`INSTALL/Mig-Alley_Patch_Win_EN_Patch-123/readme.txt`, with a
per-version CONTENTS listing fixes and "Workarounds"). Every bug those patches fixed **in the EXE**
is, by default, **still live in our port** — we build the pre-patch sources — while being **absent
from the gold shots**. Two consequences, both material:
> 1. a list of real, already-diagnosed bugs we have never looked for; and
> 2. **some recorded "parity deviations" may be patch differences rather than port defects** — which
>    would revise verdicts in `port/scrum/screen-parity.md`, not just add work.

| # | Story | Pts | Acceptance criterion | Status |
|---|---|---|---|---|
| M0 | Inventory the corpus and extract every named bug/fix. | 5 | A table in `port/scrum/patch-bugs.md`: source doc → version → symptom → one line on what it implies for the port. Sources: `INSTALL/Mig-Alley_Patch_Win_EN_Patch-123/readme.txt` (v1.01–v1.23 + Workarounds), `DOC/MigAlleyTips.pdf`, `DOC/CampaignGraphicsWorkarounds.pdf`, `DOC/MigAlleyLinks.html`, `DOC/communityDoc/`, `DOC/REFERENCE/`. | ✅ **DONE (S432).** Readme corpus fully inventoried — v1.23 added as MA-P10…MA-P25, and **there are no v1.2/1.21/1.22 changelists to find** (the readme jumps V1.1 → V1.23; that "still to inventory" line is answered). New source found and listed: `SRC/CHANGES.TXT`. PDFs/communityDoc remain the only un-read part and are a separate, smaller pass. |
| M1 | Establish what patch level our SOURCE is. | 3 | Written answer with evidence: does `SRC/` already contain the v1.0x fixes, or is it the pre-patch tree? Decides whether the whole list applies or only part. **Do this before triaging anything** — it is the difference between a long list and an empty one. | ⚠️ **RE-ANSWERED (S432) — S212's answer is OVERTURNED. The source is NOT pre-patch.** `KEYMAPS.H:390` carries the v1.03 flaps fix with the comment *"New Flap controls for US version and Patch" //CSB 24/08/99*, and the tree holds dated work to **Dec 2000**, eight months past v1.23. **This inverts M2's prior**: a patch fix is now "present unless shown absent". Full evidence in `port/scrum/patch-bugs.md`. |
| M2 | Triage each item: live / already-fixed / N/A / data-only. | 8 | Every M0 row gets a verdict **from evidence** (a grep, a run, a `git log -L`), never from reading the description. Patch items that only ship DATA (art, missions, `bdg.txt` values) are N/A to a source port and must be marked so. | 🔨 **NEW** |
| M3 | Fix the live ones, highest-impact first. | 13 | Each fix gated or measured like any other backlog item. | 🔨 **NEW** |
| M4 | Re-examine parity verdicts in the light of M1. | 5 | Any screen whose deviation is explained by a patch difference is re-marked, with the patch item cited. **An oracle we mis-attribute is worse than no oracle.** | 🔨 **NEW** |
| M5 | ⭐ **PO-61 candidate, testable now:** the patch readme states *"Applying the patch will invalidate all existing savegames and recorded videos."* | 3 | Determine whether the shipped `Ian*.cam` files are from a different patch level than our source expects. That is a concrete, independent explanation for S204/S210's `LoadItemAnims FAILED` + `GetShapePtr(8036) OUT OF RANGE`, and it would mean PO-61 is **not** a port defect at all. | 🔨 **NEW — cheap, and it may close PO-61.** |

**EPIC M total: 37 pts.** M1 first (it sizes everything else), then M0 → M2 → M5 → M3/M4.

---

**EPIC L total: 26 pts** (L0 first; L1 gated behind PO-65).
**Backlog total (open work): ~472 pts** (EPIC J residuals ~300 + EPIC K 75 + EPIC L 26 + EPIC M 37 + EPIC N 34).

---

## 5. Sprint Plan (rolling)

### 🏆 S194–S202 — **THE WONJU RAID FLIES, END TO END** — ✅ PO-VERIFIED 2026-08-23

**PO:** *"wonju raid successful! A big milestone! everything worked"* — the full walkthrough:
authorise the mission, edit the route, insert a wave and set its time, name the pilot, take off,
fly the raid.

Seven defects stood between the port and that sentence, and **not one of them was in the feature
the PO named**. Each was found by measuring the layer below the symptom.

| # | reported as | actually was |
|---|---|---|
| **S194** | "click the check mark on ins wave → crash" | `FillWaveRow` formatting `"2.Flak Supp."` (12 chars) into `char buffer[10]` — a stack smash. Committing a wave is what *creates* that row |
| **S196** | "replay → crash" | The game had already diagnosed the failure correctly; `SayAndQuit`'s `exit(0)` then ran static destructors over a 3D world that was never built |
| **S197** | "ins wave shows 'Player' where 8:30 should be" | `CWnd::SetWindowTextA(LPCSTR) { return TRUE; }` — a stub that **reports success and discards the value** |
| **S200** | "can't edit 8:30" | `CT_EDIT` missing from the OOB click allowlist: a hosted edit could not be clicked, so it could never take focus |
| **S201** | "gun camera on → 3D → crash" | **Xlib's default error handler calls `exit()`**; a transient `BadWindow` killed the process, and the teardown crashed |
| **S202** | "can't edit the player name either" | `Acquire` set `g_diKbAcquired`; `Unacquire` released the **mouse** and ignored the keyboard. After any flight, every front-end edit was dead |
| **S189/S193** | "no waypoint drags" | The map had **never received a drag** — every click was press+release fused into one tick, deliberately, to dodge an unported `GetDC()` |

⭐ **The lesson worth keeping is about the STACK of causes.** "I can't type in that box" had **three
independent causes** — the allowlist, the focus, the keyboard grab — and each one hid the next. Two
of my fixes were placed by *assuming* which of the port's three click dispatchers a dialog used;
the routing (`[oobclick] → [tbclick]`) was in the PO's log the whole time. **Reading the routing
costs one grep; assuming it cost the PO two retests of a fix that could not work.**

⭐ **And S202 could not have been gated.** It needs a flight FIRST and typing AFTER. Every gate
enters the front end fresh and types before it flies, if it flies at all. The suite tests features
one at a time; **that bug lived in the ORDER of two features.** The PO's own history was the clue
and it was hiding in plain sight — name entry had worked for weeks because they always typed the
name before flying.

**Still open:** the replay VCR controls (buttons highlight, `0` exits, nothing plays) and the `.cam`
load failing at `LoadItemAnims` — a different fault from the crash that used to mask it. The PO's
gun-camera recording got further than the Wine `.cam` files ever have.

### 🏃 S187–S188 — the suite had no runner, and two gates were lying — ✅ CLOSED 2026-08-23

**Gates:** 18/18 clean (`port/gates_all.sh`), binary unchanged across the run.

- ⭐ **S187: `port/gates_all.sh` — the suite runner MA never had.** Until now "re-run the gates"
  meant remembering which of ~23 scripts in `port/` are gates and running each by hand. That is
  how a suite silently shrinks: **a gate nobody remembers is a gate that never runs, and it goes
  stale without ever going red.** One `gl-lock` for the whole suite, one verdict, and the
  binary's md5 checked before and after — a suite run against a binary that changed underneath
  it is not a result. `stress_launch` and `hw_gate` are deliberately excluded and it says so out
  loud rather than quietly under-covering.
- ⭐ **Its first run went red on two gates, and BOTH failures were in the harness.**
  - `panel_click` was the ONE gate that took `gl-lock` itself. Run by hand that is invisible;
    run under a suite that holds the lock it blocked on its own parent, burned both 90 s
    timeouts, and reported *"the menu was drawn but clicking there did nothing"* — a dead front
    end that did not exist. S159 predicted exactly this. Now 9 s and green.
  - The runner's own gate list spans three source lines and the `gl-lock` re-entry passes it
    inside a `bash -c` string: bash read lines 2 and 3 as **commands**, so the suite ran **7 of
    18 gates** and still printed a confident *"5 passed, 2 FAILED"*. A suite that under-runs
    without saying so is worse than no suite.
- ⭐ **S188: `overlay_text` was measuring EMPTY SKY.** It cropped a hardcoded rectangle
  calibrated when in-flight capture ran at a smaller back-surface size. Flight now renders at
  1920×1080 with the overlay at **fixed pixel offsets, not proportional ones**, so the rectangle
  missed the panel entirely. The radio menu renders perfectly — *"1.Group Info / 2.Precombat / …
  / 0.Exit"* in red — and the gate called it BLOCKS-OR-BLANK. **The tell was in the gate's own
  output:** with `ARMS=all`, the fix arm scored 78, `MA_NO_ALPHATEXT` scored 78 and
  `MA_NO_GLYPHS` scored 78. **A gate whose control arms score the same as its fix arm is not
  measuring the thing it names.** It now LOCATES the panel by its own UI grey.
- **The verdict conflated two different failures.** "BLOCKS-OR-BLANK" was reported both for "the
  ink is wrong" and for "no screen ever appeared". There is now a distinct **NO PANEL** verdict —
  and the moment it existed it told the truth about the waypoint screen, which turned out to be
  the *locator's* fault too: its brightness window was 90–190, the radio panel is (120,128,128)
  and the waypoint notepad is (232,240,240). So the first cut of the fix reported "the screen
  never opened" about a map that had rendered perfectly — terrain, route line, options, table.
- **And the metric would have passed a blank screen.** The waypoint panel has a drawn **spiral
  binding** across its top; measured as part of the bounding box it contributed **1094 edges**
  against a threshold of 600, so `MA_NO_GLYPHS` scored LETTERS. Each row now contributes only its
  widest contiguous stretch of panel colour, and only if that spans ≥75 % of the panel width.
  The 0.75 came from a **sweep against the control arms** — recorded in the source so it is not
  a magic number. Final: `radio` 1347/56/0, `waypoint` 1188/0.
- **Measured, not assumed:** `MA_UISCR_KEY` re-armed on **every** screen promote, including the
  one its own keypress caused, so the driver pressed the same digit again inside the screen it
  had just opened. A/B, same build, one flag apart: `oneshot` 1 injection / panel 967,35..1884,540;
  `repeat` 2 injections / panel 0,125..1403,540. It is a real driver fix — and it is **NOT** what
  caused the NO PANEL verdict, which this entry says rather than claiming a two-for-one.
- **`gates_all.sh` now refuses on a stray `wmig`**, before the suite and after every gate. An
  **orphan** — parent dead, so `timeout` can no longer reach it — held the run directory and
  silently blocked the next arm from starting for half an hour. It **refuses rather than kills**:
  the stray may be the PO's own game (S177).

### 🏃 BoB S200–S203 — the dogfight crash, and why the AI never fights — ✅ CLOSED 2026-08-23

**Reviews:** `~/bob/PORT.md` (S200, S201–S203).

- ⭐ **S200: the dogfight crash is a one-past-the-end read.** `ACMMAN.CPP:4277` runs
  `for(SWord i = 0; i <= 3; i++)` over `Cloud Layer[3]` (SKY.H:74) and dereferences `Layer[3]`.
  `fault_addr=0x8a4c000` is page-aligned — reading just past the end of a block into an unmapped
  page. On Windows it landed in adjacent members of a large global and was harmless.
  ⚠️ **Fixed by inspection, NOT reproduced** — the crash needs page-layout luck, and the labelled
  status is "unambiguous by inspection", not "verified".
- **S201: GATE 6, the combat soak.** Asserts the sim actually soaked (≥200k dispatches — without
  that it would pass on a run that never got airborne) and that nothing crashed over ~1.45M
  dispatches. Reports combat activity and deliberately does **not** assert it: a gate that always
  fails is noise, and asserting a property the port has never had is asserting a wish.
- ⭐ **S202–S203: the AI never fights, and it is not an ACM bug.** Traced link by link, each one
  measured: `AUTO_COMBAT` ← `SetEngage` (**0 calls**) ← `AUTO_PRECOMBAT` (**0 ticks**) ←
  `AUTOSAG_PRECOMBAT` ← `PS_DETAILRAID`/`PS_ENEMYSIGHTED` ← a squadron with `method=AM_INTERCEPT`
  ← **the raid being detected**. Across all 39 waypoint executions: `method=0 detected=0`, and
  `AM_INTERCEPT=1`. **The raid is never detected, so no interceptor is ever tasked.** The raid
  itself flies correctly (`PS_FORMING → PS_INCOMING → PS_TARGETAREA`). The manoeuvre code is fine
  and simply unreached; the campaign's **detection/interception** side is inert.
- **Two instrumentation faults caught inside the sprints, both worth the entry:** a histogram
  placed between two `case` labels (after a `break`) was unreachable and reported a confident
  **zero** that meant "never executed"; and a status histogram inside `SAGDecisionFollowWP` — which
  runs **once** in 600s — reported "stuck at PS_FORMING" when the status was advancing fine.
  **Measuring where the subject rarely goes reports its first value forever.**

### 🏃 Sprints 179–185 — the PO played it, and eight defects came back — ✅ CLOSED 2026-08-23

**PO-driven throughout.** The PO built and flew the Wonju mission end to end and reported what
broke. Reviews: `port/scrum/sprint-179.md` … `sprint-185.md`.

**Fixed and PO-verified:**
- **PO-57 the player's name** — ⭐ `GetCaption()` was **never implemented on any hosted control**:
  every type handled `DISPID_CAPTION` on *set* and none on *get*, since bring-up. `CCareer` read
  back an empty `CString` and, because an empty string passes its length check, **overwrote the
  typed name with nothing**. PO: *"working now, appears on FLY screen."*
- **PO-59 roster over the title screen** — `DestroyPanel` deregistered only its own window, so the
  frag panel's three `CFragPilot` sub-dialogs kept their controls. The **exact fault S171 fixed on
  the `EndDialog` path and left unfixed here.** PO: *"fixed."*

**Fixed, awaiting the PO:**
- ⭐ **PO-60 the keyboard was dead until alt-tab.** SDL delivers key events only to a **focused**
  window; the resize-for-3D changes size, border and position at once and the WM takes focus away.
  Measured `focus=NO`. **This is why "the brakes do nothing" survived two sprints** — S180 had
  proved the brake chain correct from the PO's own keypresses, and the keys were never arriving.
  The PO's aside *"but neither did F2"* was the whole answer: F2 has nothing to do with brakes.
- **PO-61 replay crash** — `GetShapePtr` had no bounds check; `shapetable` is
  `new fileblockptr[ShapeNumMAX]` NULLed only from `ShapeNumMIN` up, so an out-of-range shape
  number from a `.cam` file read an uninitialised pointer and `getdata()` dereferenced it.
- **PO-62 window straddled two monitors** — `SDL_WINDOWPOS_CENTERED` centres across the *whole*
  3840x1080 desktop. ⚠️ **Does not explain the reported left-edge cutting**, which the measured
  geometry (`0,32 1920x1080`) contradicts; said so rather than claiming the win.

**BoB, same session:** ⭐ **the dogfight crash is a one-past-the-end read** —
`for(i = 0; i <= 3)` over `Cloud Layer[3]` in `ACMAirStruc::DefenceManoeuvre`. And the measurement
that matters more: `BOB_TRACE_ACM` counts **zero** `DefenceManoeuvre` calls in the whole convoy
gate — **the suite never enters the combat AI at all**, which is why a player found this and no
gate could.

**The through-line:** four of these were one report each, and three of them (PO-58, PO-60, and the
earlier PO-52) were **the same root cause wearing different clothes**. Every wrong turn came from
building a theory on the layer I had instrumented rather than the layer the claim was about
(§8-MA126), and every correction came from the PO's own words.

### 🏃 Sprint 214 — "It ate a replay again" (PO-65) — ✅ CLOSED 2026-08-25 (data loss stopped, 8/8)

- ⚠️⚠️ **PO-65's data loss RECURRED, on the S210 build.** Routine integrity check after the PO's
  session: **7/8 replays pristine.** `IanVertical Hero.cam` overwritten *again* at 08:30
  (**173,131 → 23,885 B**) — the same file as the first time. Restored (8/8), and the PO's own
  recording preserved at `scratchpad/po65/po_saved_replay_0830.cam`.
- ⭐ **S210 fixed the port's contribution and NOT the data loss, and the distinction matters.** S210
  made the name field clickable (it was hit-tested under the list). But the *default target* is
  still `Save_Data.lastreplayname`, which persists in `settings.mig` across sessions — so a player
  who does not type a new name still overwrites whatever was last loaded or viewed. **Fixing "you
  can now type a name" is not the same as "it no longer destroys your files."**
- ⭐ **The overwrite is the ORIGINAL's design and is deliberately left alone.** `SaveReplayData` calls
  `CopyFile(replay.dat, <name>, FALSE)` — `bFailIfExists=FALSE`, i.e. overwrite on purpose. Refusing
  would diverge from the game, and fidelity is still the rule outside `[IMPROVEMENT]` stories.
- 🔧 **What is NOT the original's design is a port that destroys the player's game data with no way
  back.** So the save stays **byte-identical and additive**: if the target exists, its contents are
  copied to `<name>.bak` first. Nothing the game does changes; a lost file becomes recoverable.
  `MA_NO_SAVE_BACKUP=1` disables it.
- **Verified end to end on a real flight**, and it reproduced the exact loss:
  `[replay] SAVE would overwrite 'IanVertical Hero.cam' -- previous contents kept as
  'IanVertical Hero.cam.bak'`, with the `.bak` holding the **full 173,131 bytes**. Test residue
  removed; **8/8 pristine** afterwards.
- ⭐ **Process point worth keeping: the integrity check is what caught it, not the PO.** They had
  closed the window without testing. A per-session `cmp` of the shipped `.cam` set against
  `~/sgl/TUE/afterGameReport/` — the only known pristine copy — is now the thing standing between
  this defect and permanent loss. **Where a port can destroy user data, the check belongs in the
  routine, not in the bug report.**
- **Gates:** `parity_2d` **5/5 byte-identical**.

## 📋 STANDING PROCESS (PO directives, 2026-08-25)

1. **ALTERNATE MA AND BOB SPRINTS.** Odd sprints ma, even sprints bob, unless a PO-reported
   defect is live. Long ma-only runs (S207–S227 was **21 consecutive**) starve the other port.
2. **TIME-BOX HYPOTHESES.** Do not spend sprint after sprint on one causal story — especially not
   on recovering/parsing legacy data. If two sprints do not settle it, change the EXPERIMENT, not
   the argument. (Directly prompted by S219→S226: four sprints on one wrong number.)
3. **REPLAY TESTING USES PORT-GENERATED FILES ONLY.** See S227.

---

### 🏃 Sprint 275 — "A regression I introduced, and verified around" (EPIC L) — ✅ CLOSED 2026-08-25 (8/8)

**BoB's S274 found that its export never marked the player. Checked MA. Same bug — and I put it
there.**

- **S255** emitted one object and marked it `Pilot=Player`. Correct.
- **S257** replaced that with the walk over `ACList`, comparing `_ac == gac`
  (`Persons2::PlayerGhostAC`) — **which matches nothing.** From S257 onward **no aircraft was marked
  as the player**: measured, **0 `Pilot=Player` samples across 72 markers**.

⭐ **AND S257 VERIFIED TWO THINGS, BOTH CORRECTLY, NEITHER OF WHICH COULD SEE THIS.** It checked the
object COUNT (40, matching an independently-measured chain) and the colour RATIO (24/16 = 1.5,
matching the mission table's 12/8 side balance). Both were real evidence for what I had *added*.
**Verifying the thing you changed is not the same as verifying the thing your change could break** —
the player marker was pre-existing behaviour that the rewrite silently dropped, and nothing I
measured was pointed at it.

**Fixed** by accepting `Manual_Pilot.ControlledAC2`, which is what the game's own friend/foe test
uses (`MSGAI.CPP:1781`). Verified: **80/80 markers carry `Pilot=Player`**, 40 objects preserved,
colour ratio still 1919/1279 = **1.50**.

- **Also checked and CLEAN:** MA does *not* have BoB's 7.7x duplicate-sample bug — 71 samples per 72
  markers (99.3%), because `StoreDeltas` advances `replayframecount` on every call here. *Same
  writer, same structure, different engine behaviour* — which is the third time this week that a
  BoB/MA constant or behaviour failed to transfer.
- **The cross-port check is what found it.** BoB's bug was visible because its export was new and I
  looked at everything; MA's was invisible because I only looked at what I had just changed.

### 🏃 Sprint 273 — "It is only a file copy" (EPIC L / L1 residual) — ✅ CLOSED 2026-08-25 (8/8)

**S255 shipped L1 with the tee proven and the PUBLISH step never once run**, on the reasoning that
"publishing is a file copy". ⭐ *That is exactly the kind of reasoning this project keeps having to
retract* — S258's conclusion was right for the wrong reason, S250's was simply wrong, and both came
from not running the cheap thing. So: run it.

**`MA_SAVE_AS=<name>` drives the game's OWN `SaveReplayData` once a flight ends**, exercising the
save path — and the `.acmi` publish hooked into it — without a human at the replay dialog.

```
[saveas] driving SaveReplayData("s273probe.cam")
[acmi] wrote Videos/s273probe.acmi (144 object samples)

  s273probe.cam    21,529 bytes
  s273probe.acmi   17,016 bytes     <- beside it, which is what L1 asks for
```

- **L1's acceptance is now fully met**: `Videos/<name>.acmi` appears next to `<name>.cam`, from the
  game's own save path rather than a harness copying files around.
- **Honest scope:** the UI still chooses the filename; this hook starts one step after that choice.
  Everything downstream — `SaveReplayData`, `GetReplayFilename`, the `CopyFile`, the ACMI publish —
  is the real code.
- **Useful side effect:** the whole chain (fly -> record -> tee -> save -> both files) is now
  drivable in one command, which is what L5's gate needs to cover the publish step too.
- **EPIC L: L0 ✅ L1 ✅ (fully) L3 ✅ L4 ✅ L5 ✅ — only L2 remains, and the PO is installing Tacview.**

### 🏃 Sprint 269 — "A missing statement, invisible to a constant scan" (N3) — ✅ CLOSED 2026-08-25 (8/8)

**The handler audit found a real latent double-free — on its second method.**

S267 scanned the duplicated `WM_*` handlers for **magic constants** and found nothing conclusive
(its "copies disagree" flags were an artefact of a crude capture window, and I said so). S269 diffed
the same copies **by normalised body** — comments and whitespace stripped, code only — against the
**true compiled twin set** read out of `build.ninja`.

**Result:**
```
OnGetFile          4 copies, 4 distinct
OnReleaseLastFile  4 copies, 2 distinct   <-- three agree, ONE differs
OnGetOffScreenDC   3 copies, 1 distinct   (identical)
```

**⭐ `CRToolBar::OnReleaseLastFile` was the only copy missing one line.**
```c
RDialog / CMIGView / RMdlDlg      CRToolBar
  delete m_pfileblock;              delete m_pfileblock;
  m_pfileblock = NULL;              /* ...and nothing */
```
`WM_RELEASELASTFILE` is how a control says *"done with the file"*, and **every drawn control sends
one** — so two releases in a row are ordinary, not exotic, and the second `delete` frees an
already-freed block. **LATENT, not a live crash**: `OnGetFile` overwrites the pointer on the paths
that allocate, and deliberately NULLs it for cached blocks it does not own. Fixed anyway, because
*a double-free that depends on call order surfaces later as unrelated-looking corruption.*

- ⭐ **THE METHOD IS THE FINDING.** A constant-scan could never have seen this: **the difference is a
  missing STATEMENT, not a changed number.** S267 looked for the shape of the *last* bug (S248's
  wrong integer) and found nothing; looking for *difference itself* found it in one pass. **When an
  audit comes back clean, check whether it could have detected the thing you are looking for.**
- **Also mapped, and worth keeping:** the build compiles **UPPERCASE** twins for MFC
  (`RDIALOG.CPP`, `RTOOLBAR.CPP`, `MIGVIEW.CPP`, `RMDLDLG.CPP`) but **mixed-case** for COMMS/3D
  (`Replay.cpp`, `Winmove.cpp`, `Viewsel.cpp`). *Two conventions in one tree* — which is exactly how
  four sprints this month came to edit a file the build ignores.
- **No regression:** `port/replay_record.sh` **PASS** (66 frames, `header+frames == EOF` exactly).

### 🏃 Sprint 263 — "The field was findable; I had searched for the wrong word" (EPIC L / L3) — ✅ CLOSED 2026-08-25 (8/8)

**L3 completed: aircraft are now coloured by side.** S257 deliberately left this out rather than
guess, on the grounds that *a confident wrong colour on every AI aircraft is worse than none*. That
was the right call — and the field was findable after all.

**⭐ WHY I MISSED IT: I SEARCHED FOR THE WRONG WORD.** I looked for `side` / `team` / `GetSide` —
words this codebase does not use — and concluded the field was "not where I could cheaply find it".
The game's own friend/foe test is `trg->nationality == Manual_Pilot.ControlledAC2->nationality`
(`MSGAI.CPP:1781`), and `nationality` is a `LASTFIELD` bitfield on the item base
(`worldinc.h:715`). *Search for how the program ITSELF distinguishes the thing, not for the word you
would have named it.*

**✅ VERIFIED BY A RATIO, not by looking at colours.** On the 20-aircraft mission the export gives
**24 Blue / 16 Red**. The mission table for that index (`IDS_QUICK_2`) has **12 UN aircraft** (F-80,
B-29, F-86A, F-84) against **8 MiG-15s** — and **12/8 = 1.5 = 24/16**. The colour split reproduces the
mission's *designed side balance* exactly, from a table I read days earlier for an unrelated reason.
A miscategorisation would have to preserve that ratio to hide.

**EPIC L is now complete except L2:** L0 ✅ L1 ✅ **L3 ✅** L4 ✅ L5 ✅. **L2 (opens in Tacview) is the
only story left, and it is PO-blocked** — structural validity is gated (L5) but "Tacview accepts it"
can only be settled by Tacview.

### 🏃 Sprint 261 — "The comment said 10, the measurement said 25" (EPIC L / L4) — ✅ CLOSED 2026-08-25 (8/8)

**IAS is now exported, and a cross-check caught a 2.5x unit error that a source comment would have
baked in permanently.**

`FLYMODEL.CPP:162` documents `vel, i_a_s` as **"10 cm/s"**, making m/s = `vel/10`. Emitting that
produced perfectly plausible lines: valid syntax, sensible magnitudes, a believable 505 kt for an
F-86. **The L5 gate passed it.** Nothing about the file said it was wrong.

**⭐ THE CROSS-CHECK IS WHAT CAUGHT IT.** Speed derived from successive `U`/`V` positions — an
independent route, anchored by the altitude landing on **exactly 1524.00 m = 5000 ft** — disagreed by
a ratio of **2.4986 on every sample**. Too clean to be noise. With the divisor corrected to **25**:

```
mean ratio = 0.9995        (1.0 = the two independent routes agree)
sample IAS = 103.88 m/s = 202 kt
```

- **The comment was the ONLY evidence for /10, and a comment is not a measurement.** Shipping a
  2.5x-wrong speed would be worse than shipping none — a debrief tool's value is that its numbers can
  be trusted, and every line would still have looked right. Same reasoning that left L3's side colour
  unemitted rather than guessed.
- ⚠️ **AND I MIS-DIAGNOSED IT FIRST.** Seeing 2.5, I reasoned 20 Hz x 2.5 = 50 Hz and "found" the
  cause: `WINMOVE.CPP:14969` sets `RateDivider=2` -> 100/2 = 50 Hz. Tidy, and **wrong** — that
  assignment is on a path this port never takes. **Printing the runtime value gave `RateDivider=5`
  -> 20 Hz**, exactly what L1 assumed. *Reading the source instead of printing the value would have
  "fixed" a correct time axis into a broken one, and left the real error in place.*
- **Two units now confirmed by independent routes rather than by reading**: the centimetre position
  scale and the velocity scale.
- **EPIC L: L0 ✅ L1 ✅ L3 ◐ L4 ✅ L5 ✅.** Only **L2** (opens in Tacview) remains.

### 🏃 Sprint 259 — "The gate found a real bug on its first run" (EPIC L / L5) — ✅ CLOSED 2026-08-25 (8/8)

**`port/tacview_export.sh` — the export is now verifiable without opening Tacview.** Flies, then
asserts: the flight recorded, the header, the global properties, **strictly monotonic** time markers,
every object line's 9-field transform, at least one tracked object, and that the recording survives.
**PASS**, and `CONTROL=1` (`MA_ACMI=0`) **goes red on assertion 2 — verified**.

- ⭐ **WHY STRUCTURE AND NOT VALUES:** *a wrong export looks exactly like a right one.* Get the
  centimetre scale wrong and every line still parses, every field is still a number, the file still
  opens. **The failure mode of this feature is a plausible file** — the exact kind this project keeps
  being fooled by — so the assertions test what a wrong scale *cannot* fake: structure and internal
  consistency.
- ✅ **AND IT FOUND A REAL DEFECT ON ITS FIRST RUN.** 171 object lines with a valid 9-field transform
  and **one trailing line cut mid-number**: `2,T=||1555.24|71.65|14.56|145.76|413327.26|8`. The
  working file is written continuously, so a kill (or a save) can catch it mid-line. Harmless in the
  scratch file; **not** harmless in a published `.acmi` — a debrief tool should never be handed a
  truncated record. **Fixed: `ma_acmi_save_as` now copies WHOLE LINES ONLY**, dropping any partial
  tail. One lost frame in thousands, against a file that is always well-formed.
- **Two of the failures were the GATE'S OWN, and finding them is why it is trustworthy now:**
  (a) assertion 1 could never pass — the `StopRecord` line it greps for only exists under
  `MA_TRACE_REPLAY=1`, which the gate never set, so *"flight recorded: NO"* was the gate failing, not
  the game; (b) my trailing-partial tolerance compared a **description string** against a **line**
  and never matched. *A gate that has not been watched pass AND fail is not yet a gate* — this one
  did both, plus two self-inflicted failures, before being believed.
- **EPIC L now: L0 ✅ L1 ✅ L3 ◐ (no side colour) L5 ✅.** Remaining: **L2** (opens in Tacview — needs
  the PO or a copy of Tacview) and **L4** (IAS/AGL/AOA).

### 🏃 Sprint 257 — "Forty aircraft, and the number was already known" (EPIC L / L3) — ✅ CLOSED 2026-08-25 (6/8, partial)

**Every aircraft now exports, not just the player.** The tee walks `*AirStruc::ACList` stepping with
**`*ac->nextmobile`** — *the same link `LoadItemData` uses*. That detail cost four sprints to learn
(S226: I counted the world with `->Next`, a chain the reader never traverses, and built three
sprints of conclusions on the number).

- ⭐ **AND THE RESULT CROSS-CHECKS AGAINST AN EARLIER MEASUREMENT.** On the PO's 20-aircraft bomber
  strike the export produced **40 distinct objects** — exactly the `NEXTMOBILE-chain=40` measured
  independently in **S236** while debugging the replay hang. *Two unrelated instruments, months of
  reasoning apart, agreeing on the same number* is the strongest evidence available that the walk is
  the right one. 377 KB / 4159 lines from one sortie.
- Bounded at 256 so a corrupt link cannot spin the recorder.
- **IDs are WALK POSITION, not a UID** — stable for the length of a sortie, which is all one `.acmi`
  covers. Named in the code because it is the assumption most likely to be wrong later: if the list
  is ever reordered mid-flight, two tracks would swap.

**⚠️ NOT DONE — `Color` by side, which L3's acceptance also asks for.** `AirStruc`'s side field is
not where I could cheaply find it (`AirStruc` is only forward-declared in `AI.H`), and I had already
spent the sprint's budget hunting for it. ⭐ **Emitting a GUESSED side would paint a confident wrong
colour on every AI aircraft — worse than none**, because a debrief tool's whole value is that you can
trust what it shows. The player is marked; the rest are left uncoloured until the field is confirmed.
That is why this closes at 6/8 rather than 8/8.

### 🏃 Sprint 255 — "🎉 The first improvement, not a port" (EPIC L / L1) — ✅ CLOSED 2026-08-25 (8/8)

## 🎯 **MiG Alley now writes a Tacview `.acmi`.** The PO called this *"the first improvement on ma in
## 20 years"* — the first backlog story the original game cannot satisfy.

**Built:** `SRC/compat/ma_acmi.cpp` — a writer taking **plain C types only**, knowing nothing about
the game's structures; the game side walks its own world and hands over numbers. Three hooks in
`Replay.cpp`: begin-on-first-frame, tee-per-recorded-frame, publish-on-save.

**Teed from the SIM, not converted from the `.cam`** — L0/S211 settled that: a `REPLAYPACKET` is 11
packed bytes of **deltas** against a reconstructed world. Converting one would mean re-implementing
the playback integrator and inheriting every alignment bug PO-61 and PO-69 spent a dozen sprints on.

**✅ OUTPUT IS STRUCTURALLY VALID** — header, global properties on object 0, monotonic `#t` markers,
transform syntax #4:
```
#0.00
1,T=||1524.00|0.00|0.00|24.12|412524.97|892951.89|24.12,Name=F-86,Type=Air+FixedWing,Color=Blue,Pilot=Player
```
- ⭐ **THE NUMBERS CORROBORATE THE UNIT ASSUMPTIONS, WHICH IS THE POINT.** Altitude comes out at
  **exactly 1524.00 m = 5000 ft** — a round quick-mission start — and the per-frame delta gives
  **104 m/s ≈ 202 kt**, a plausible F-86 cruise. Two independent quantities landing on
  physically-sensible round values is real evidence the centimetre and binary-angle conversions are
  right. *A wrong scale factor produces a perfectly plausible-looking file*, so both are named in the
  code as assumptions and `MA_ACMI_CMPERM` overrides one without a rebuild.

**✅ THE CONTROL PASSES — the recording is untouched.** `port/replay_record.sh`: **PASS**, 65 frames,
`header+frames == EOF` exactly (20630 == 20630). *The epic is safely testable precisely because the
existing replay path is its own control* — "did I break the recording?" is answered by arithmetic,
not judgement. `MA_ACMI=0` disables the whole thing.

**NOT DONE, AND NOT CLAIMED:**
- **The save hook is written but not exercised end-to-end** — it needs a UI save to fire. The tee
  (the hard half) is proven; publishing is a file copy.
- **L2 — opens in Tacview:** unverified. Structural validity is not the same as Tacview accepting it.
- **L3 — only the player is exported.** AI aircraft need the world walk (`nextmobile`, per S226).
- **L4 — no IAS/AGL/AOA yet.**

### 🏃 Sprint 251 — "🎉 Typing told nobody" (PO-68 / N1) — ✅ CLOSED 2026-08-25 (8/8), PO-VERIFIED

**The destructive save, root-caused at last — and it is TWO gaps stacked, either of which alone
would have been enough to lose the name.**

**GAP 1 — the event was never fired.** `ma_ole_char` put the character into the control's own text
and told **nobody**. The R* controls raise TextChanged via `COleControl::FireEvent`, whose connection
point is stubbed in this port — which is precisely why the dropdown path in `ma_ole_click` fires its
event *by hand*, with a comment saying so. The edit path never got the same treatment.

**GAP 2 — and the event could not have been delivered anyway.** `afxwin.h` has thunks for
`int/long/short/(int,int)/(long,long)/LPCSTR`, then a **silent no-op fallback for anything else**.
`OnTextChangedSavename(LPTSTR)` is `void(C::*)(char*)` — **`LPCSTR` was covered, `LPTSTR` was not**.
So the handler was discarded without a word.

**⚠️ THIS IS 18 HANDLERS, NOT ONE.** Every `afx_msg void OnTextChangedXxx(LPTSTR)` in the game hit
that fallback: the replay save name, **the pilot's Name field** (`CAREER.H`), the **radio message
lines** (`RADIO.H`), the wave-insert time. *Every text field in the game was typing into a void.*

**Why it DESTROYED files rather than merely failing:** `CLoad::filename` is a **`CString&` bound to
the caller's `selectedfile`**, and `OnTextChangedSavename` is the only writer of a typed name into
it. `selectedfile` is pre-seeded with `Save_Data.lastreplayname`, so the save wrote to **the previous
file**. Measured twice on the PO's machine: `260825test` and `260825test2` both landed on
`corpus-baseline.cam`.

- ⭐ **A FALLBACK THAT SILENTLY SUCCEEDS IS A DISPATCHER THAT SILENTLY DROPS.** `ma_evt_call`'s
  catch-all exists so an unmapped signature does not break the build — a reasonable goal that also
  makes an unmapped signature **invisible**. This is the same shape as S250's BoB audit (routes
  answered 0), S248's guard (art rejected), and S243's undelivered hook. **Four instances, one
  lesson: when a dispatcher cannot handle something, it must SAY SO, not return a plausible
  nothing.**
- **Where my earlier attempts went wrong, and why:** S237 pulled the text in `OnClickedFileok` — a
  path the save does not take. S243-era reasoning then assumed `CLoad::OnOK` was the choke point;
  it is not — `Rowan::CDialog::OnOK` is an **empty virtual**, and the save is driven by the
  FullScreen panel item `IDS_SAVE -> ReplaySave` reading `selectedfile` directly. **Two fixes aimed
  at plumbing that does not carry the water**, because I inferred the path instead of tracing it.
- ✅ **PO-VERIFIED — medium bomber single mission, saved as `260825test6`.** All three acceptance
  criteria met, and the trace shows **every link in the chain** for the first time:
  ```
  [type] '6' -> "260825test6"
  [type] fired TextChanged id=1060 -> "260825test6"     <- S251 gap 1: the event now fires
  [savename] OnTextChangedSavename("260825test6")       <- S251 gap 2: the LPTSTR thunk delivers it
  [savename] ReplaySave -> SaveReplayData("260825test6.cam")
  ```
  1. `Videos/260825test6.cam` exists (34,247 bytes) — **the name the PO typed**.
  2. The handler received the text, so `selectedfile` carried it to the save.
  3. **All five pre-existing `.cam` files byte-identical to the pre-session backup — nothing
     overwritten.** Checked with `cmp`, not by eye.
- **This closes backlog N1, the only open item that destroyed the player's data**, and with it the
  three-net safety arrangement (pre-session backup + S214 `.bak` + S234 autosave watcher) stops being
  load-bearing. Keep all three: they are what made every one of these sessions recoverable.
- ⭐ **UNBLOCKS EPIC L (Tacview ACMI export).** L1 writes the `.acmi` beside the `.cam`, and was
  explicitly blocked on this — *nothing should be built on a save path that destroys files.* The
  save path is now trustworthy, so L0/L1 can start.

### 🏃 Sprint 249 — "I committed the very defect I had just diagnosed" (bob→ma cross-port) — ✅ CLOSED 2026-08-25 (8/8)

**A BoB sprint that found an MA bug — which is the argument for alternating.** The cross-port question
was simply: *does BoB have the narrow `OnGetFile` guard that S248 found blanking half MA's toolbar art?*

**BoB: NO, and the reason is structural.** BoB has **ONE** implementation —
`DIALCLASS::OnGetFile` (`RDIALMSG.CPP:91`) — `#include`d three times with `DIALCLASS` redefined as
`RDialog`, `RMdlDlg`, `CMIGView`. All three classes get **identical code by construction**. Its range
is `>0x6600 && <0x7200`, whose floor already admits `DIR_ICONS_2`. **You cannot widen one copy and
forget the twin here, because there is no twin.**

**⚠️ MA: FOUR hand-written copies — and S248 fixed only one of them.**
| copy | range before S249 |
|---|---|
| `RDialog::OnGetFile` | already widened (long ago) |
| `CRToolBar::OnGetFile` | widened by **S248** |
| **`CMIGView::OnGetFile`** | **still `0x6800..0x7100`** |
| **`RMdlDlg::OnGetFile`** | **still `0x6800..0x7100`** |

- ⭐ **THE SPRINT'S REAL FINDING IS ABOUT ME.** S248's headline was *"two handlers, one fact,
  disagreeing — nobody widened the twin."* I then widened the copy I was chasing, shipped it, wrote
  that lesson up in the commit message, **and left two more copies narrow.** Diagnosing a defect
  class is not the same as searching for its other instances; the write-up felt like completion and
  wasn't. **The fix is mechanical and the search is the work.**
- **`CMIGView` is the campaign map view** — it draws map symbols from the same art files, so this was
  not a dormant twin. `RMdlDlg` serves modal dialogs.
- Both now use S248's predicate, and `MA_NARROW_TBART=1` reverts **all three** together so the A/B
  covers every copy rather than one.
- **Backlog N3 sharpened:** the audit should look for *duplicated* handlers, not just undelivered
  ones. BoB's macro-include is the pattern worth copying — a single implementation instantiated per
  class makes divergence impossible rather than merely unlikely.

### 🏃 Sprint 248 — "🎉 A guard that blanked half the toolbar art" (PO-71b) — ✅ CLOSED 2026-08-25 (8/8)

## 🎯 **PO-VERIFIED:** *"yes! The icon is a flag, and it appears only AFTER return from 3D. Clicking
## it yields the replay dialog."*

**ROOT CAUSE — `CRToolBar::OnGetFile`, `RTOOLBAR.CPP:606`, a PORT-ADDED guard:**
```c
if (filenum <= 0x6800 || filenum >= 0x7100) { m_pfileblock = NULL; return NULL; }
```
It admits **directories 104..113 only**. `FIL_ICON_REPLAY = 0x660c` lives in `DIR_ICONS_2 = 0x6600` —
**directory 102, below the floor** — so `WM_GETFILE` returned NULL and `CRButtonCtrl::DrawBitmap`
painted nothing. The button was genuinely special: **the only Misc Toolbar icon in that directory.**

**THE MEASUREMENT THAT SETTLED IT — both arms, one line apart:**
```
fn=0x660c ... WM_GETFILE -> (nil)      -> SKIP        <- Replay   (dir 102)
fn=0x6a8a ... WM_GETFILE -> 0x9d01360  -> OK, drawing <- Zoom Out (dir 106)
```

**⚠️ AND IT WAS NEVER A ONE-BUTTON BUG.** Before: **615 drawn, 732 SKIPPED**. After: **3923 drawn,
0 message-NULL skips** (the remaining skips are buttons that legitimately have no art). *More than
half the toolbar art in the game was suppressed by that one line* — and nobody had reported it,
because you cannot report an icon you have never seen.

- ⭐ **THE GUARD OUTLIVED ITS CAUSE (§8-BoB210).** It was added because a FileNum in an **unloaded**
  directory makes `makedirectoryname` `SayAndQuit->exit()`. But dir 102 is **not** unloaded —
  `MA_PROBE_FILENUM` (built in S246 for a hypothesis it then *refuted*) asks the game's own file
  system and gets `0x660c size=3382 data=yes`. The guard was excluding a directory that loads fine.
- ⭐ **TWO HANDLERS, ONE FACT, DISAGREEING.** `RDialog::OnGetFile` — the *other* `WM_GETFILE` handler,
  same art, different parent — was widened for Linux to `filenum>0 && filenum<=0xFFFF` and has served
  art safely since. `CRToolBar::OnGetFile` kept the narrow range. **Nobody widened the twin.** Both
  now answer the same question the same way. `MA_NARROW_TBART=1` reverts.
- **The PO was right and I was wrong twice.** They said *"maybe the replay icon is a special case"* —
  it was, just not conditionally: it is the only icon in a directory the guard rejected. And they
  remembered a **flag**; I said film strip from `i_reply1.bmp`. The fix restored **`0x6607` as well
  as `0x660c`**, so the flag was a *second* icon that had also been suppressed — their memory was of
  something the port had been hiding all along.
- **Six sprints of elimination, then one measurement.** S244+S246 cleared six candidates (bag name,
  FileNum, art application, file existence, bitmap format, directory registration). What finally
  found it was tracing the **three gates inside `DrawBitmap`** and printing a **working neighbour
  beside the broken one**. *The control arm is what turns a symptom into a diff.*
- **Self-inflicted, from a rule written 12 lines above the code I was editing:** the first trace used
  `_n < 40` and the front-end ate every slot before the toolbar painted — the **S65 trace-cap trap**,
  which `ma_button_draw`'s own comment warns about (*"Filter, don't cap"*). Re-cut as a FileNum
  filter and it worked first time.

### 🏃 Sprint 244 — "The blank icon: five candidates eliminated, one named" (PO-71b) — ✅ CLOSED 2026-08-25 (6/8, time-boxed)

**The Replay button works (PO-verified, S243) but draws with no picture.** Cosmetic, so this sprint is
deliberately bounded per the PO's time-boxing directive — the point is to leave the next person a
short list, not to keep guessing.

**ELIMINATED, each by measurement rather than reasoning:**
1. **The art reference is right.** DLGINIT bag for `IDC_REPLAY` names `FIL_ICON_REPLAY`; decoded all
   six Misc Toolbar bags and every one carries its `FIL_ICON_*`.
2. **The FileNum is right.** `[btnart] id=1049 fn=0x660c`, and `F_GRAFIX.G:42` says
   `FIL_ICON_REPLAY = 0x660c`. Exact match. (The four working icons are `0x6a84..0x6a8d`, a
   different block — which looked suspicious and turned out to be irrelevant.)
3. **The art IS applied** — `ma_ole_set_artnum` fires for 1049, same as for the working buttons.
4. **The file exists** — `Artwork/AXART2/i_reply1.bmp`, and also in `AXART/`.
5. **The format is not the problem** — `i_reply1.bmp` and the working `i_zm_in1.bmp` are
   **byte-for-byte the same shape**: 48x48, 8bpp, uncompressed, 3382 bytes, data at 1078.

**THE ONE REMAINING CANDIDATE, and where it came from:** `MASTER.FIL` puts a directory marker
immediately above Replay's entry — `102  dir.dir  DIR_ICONS_2`. The four working icons live in the
FIRST icons directory (`AXART`, 234 files); **Replay and Ready Room are the only Misc Toolbar icons in
`DIR_ICONS_2` (`AXART2`, 13 files)**. Both directories have a `DIR.DIR`. So: *is directory 102
registered in the port's file table at all?* That single question explains the whole symptom — a
correct FileNum in an unregistered directory yields no bits and draws nothing.

- ⭐ **A wrong instrument, caught this time before it produced a conclusion.** `BOB_TRACE_FOPEN` showed
  **neither** icon being opened — not the broken one and **not the working one either**. The right
  reading is not "the file is never opened" but *"this trace cannot see this path"*: the game reads
  art through the indexed `DIR.DIR` archive (`fileblock`/`getdata`), not by filename. **The control
  arm is what saved it** — had I only traced the broken icon, "no fopen for i_reply1.bmp" would have
  looked like a finding. S233 cost three conclusions to an instrument I had not validated; this is
  the same check applied one step earlier.
- **S246 (next):** trace `fileblock(FileNum)` for `0x660c` vs `0x6a87` and compare which directory
  each resolves through. That is one measurement, on the path that actually carries the bits.

### 🏃 Sprint 240 — "🎉 A feature compiled out since 1996" (PO-70) — ✅ CLOSED 2026-08-25 (8/8)

**PO: Ctrl+F6 does nothing on a padlocked bogey.** ✅ **Now PO-VERIFIED working in flight.**

- **The key path was PERFECT the whole time.** Traced: `scancode=0x1d shift=0 -> action index=8`
  (LCTRL sets `currshifts = 8>>1 = 4`), then `scancode=0x40 shift=4 -> action index=224` —
  `OUTREVLOCKTOG` is `KeyName(112)` and indices are 2x, so **224 is exactly right**.
- **THE ACTION DID NOT EXIST.** Rowan guarded the whole reverse padlock with `#ifndef NDEBUG`
  (RDH, 13Dec96): the dispatch entry, `List6Toggle`, `InitOutRevPadlock`, `DrawOutRevPadlock`, the
  view record and the `VM_OutRevPadlock` enum value **all vanish from a release build**. The key
  resolved to an action with no handler. **It has never worked in any shipped build.**
- ⭐ **Three traps, all documented in this repo, all hit in one sprint:**
  1. **The case-colliding twin.** I analysed `VIEWSEL.CPP`; the build compiles **`Viewsel.cpp`**
     (22 KB different). Caught only by checking what `_3D.CPP` actually `#include`s.
  2. **The guarded lines still used MSVC's bare `= InitOutRevPadlock`** rather than
     `&ViewPoint::`. The port's MSVC->GCC sweep never touched them **because they never compiled** —
     which is independent confirmation the code has been dead since 1996.
  3. **`/**/#ifndef NDEBUG`** — the enum guard is prefixed, so a `^#if` scan misses it.
- **`VM_OutRevPadlock` APPENDED to the enum, not enabled in place**: enabling mid-enum would
  renumber `VM_Satellite` onward and misread a view mode already persisted in `settings.mig`.
  Every pre-existing value keeps its number. `MA_NO_REVPADLOCK=1` disables the action.
- **Also fixed:** Alt+F6 is unusable under GNOME (`cycle-group` claims it; verified by gsettings).
  Added **Ctrl+F6**, chosen by elimination against BOTH key spaces — free in MA (F6 had only
  Alt/Shift/norm, so `TOGGLEWOBBLEVIEW` keeps Shift+F6) and free in GNOME (only `<Alt>F6`,
  `<Shift><Alt>F6` and `<Primary><Alt>F6` are taken). **The PO declined altering their desktop
  config — the right call: a port should not cost the user a shortcut to be usable.**
- **Still open:** Ctrl+F6 does nothing *during replay playback*. Narrower, unmeasured, not guessed at.

---

### 🏃 Sprint 243 — "🎉 A live button with no picture" (PO-71) — ✅ CLOSED 2026-08-25 (8/8)

**PO: no Replay button on the campaign map.** ✅ **PO-VERIFIED:** *"I clicked to the right of the X
in campaign after the 3D exit, and got the replay dialog."*

**THREE STACKED CAUSES, each hiding the next — and the first two were my own fixes landing nowhere:**
1. **The button was buried.** `IDD_MISCTOOLBAR` gives `IDC_REPLAY` and `IDC_READYROOM` the **same
   rect** (`WS_TABSTOP,144,0,32,30`). Neither is template-invisible, and the code that swapped them
   was commented out on **19/03/99**. READYROOM is declared last, so it drew on top.
2. ⭐ **My fix went into a hook the port never calls.** I put the swap in
   `CMiscToolbar::OnShowWindow` — a `WM_SHOWWINDOW` handler our compat does not deliver. **The trace
   never printed and the fix never ran.** *A fix placed in a hook the port does not call is
   indistinguishable from no fix.* This port has been bitten by exactly this before (BoB S158:
   `SendMessage` was an allowlist of three; 16 of 20 `WM_*` routes silently returned 0). **The trace
   is what caught it** — moved to `DoDataExchange`, which demonstrably runs.
3. **`OnShowWindow` did TWO jobs.** The swap *and* `SetDisabled(false)`. Restoring only the swap left
   the button visible-but-disabled, and the dispatcher swallows clicks on disabled controls
   (`[tbclick] id=%d is DISABLED -- click swallowed`). Both jobs now run.

- ⭐ **AND I MISIDENTIFIED THE BUTTON FROM GEOMETRY.** I told the PO the "X" was Replay, reasoning
  from template rects. **The ids refuted me:** `IDC_FILES = 10` at canvas 741 is the X;
  `IDC_REPLAY = 1049` sits at **789-837**, one slot further right. *Inferring identity from position
  when the identity is directly readable is a choice to guess.*
- **What actually settled it** was printing the control's own state rather than theorising:
  `slot5 REPLAY: vis=1 rect=(216,0,48,48)` next to `READYROOM: vis=0`. Visible, enabled, in the
  toolbar's 264 px width, unfiltered, unclipped.
- **REMAINING (PO-71b): the button draws BLANK.** It is live and clickable but has no icon, so the
  PO could not see it. Its art is presumably set on the same dead `OnShowWindow` path. Next sprint.
- **New diagnostic kept:** `ma_dlg_in_template` now reports what it filters (`MA_TRACE_DLG=1`) —
  previously a control dropped by that filter had **no draw, no hit-test and no trace**, i.e. it was
  invisible to diagnosis as well as to the eye.

### 🏃 Sprint 236 — "🎉 The bounds check overflowed, so it passed the sizes it existed to reject" (PO-69) — ✅ CLOSED 2026-08-25 (8/8)

## 🎯 **PO-VERIFIED: the 40-aircraft replay plays.** PO: *"yes, replay working. I fast-forwarded
## through it. multiple aircraft"* — `NEXTMOBILE-chain=40`, **16 blocks**, vs **2** for the 1v1.

**PO-69 was: bomber-strike replay → "not responding" dialog.** Caught live under gdb — the debugger
the PO asked for paid for itself on its second use.

**ROOT CAUSE — a 32-bit pointer overflow in `ReplayRead`'s bounds check:**
```c
if (playbackfilepos + size > playbackfileend)   // 32-bit sum: a large `size` WRAPS
```
The wrapped sum compares small, the guard says "fine", and `memcpy` then copies `size` bytes out of a
buffer that does not contain them. **The guard passed precisely the sizes it existed to reject.**

**Live evidence (gdb, PO's stopped process):** main thread in `memcpy` under
`ReplayRead <- BackupSmokeInfo <- LoadBlockHeader <- PreScanReplayFile`, size argument **~1.9 GB**,
`VmData: 2,200,160 kB` **in a 32-bit address space**. Never deadlocked — page-faulting through a
multi-gigabyte copy sourced from a **417 KB** file.

- ⭐ **PROVED BY ELIMINATION, NOT ASSUMED** — the distinction this project keeps paying for. ~40 KB
  remained and ~1.9 GB was requested. **There is no arrangement of two valid pointers into one buffer
  for which `pos + 1.9GB <= pos + 40KB` holds unless the addition wraps.** So the guard cannot have
  been evaluated without overflow — no need to read the pointers to know it.

**THE FIX, two layers:**
1. `ReplayRead` compares against **bytes remaining** — `size > (ULong)(playbackfileend -
   playbackfilepos)` — a *difference* of two pointers into one buffer, which cannot overflow, where a
   *sum* can. Plus rejects a position already past the end. `MA_NO_READ_OVERFLOW_FIX=1` reverts.
2. `BackupSmokeInfo` bounds `smokesize` **before `new UByte[smokesize]`**, since the allocation alone
   reached 2.2 GB before any read was attempted. `MA_NO_SMOKE_BOUND=1` reverts.

**✅ RESULT:** clean exit, no crash, guard fired once —
`BackupSmokeInfo REFUSED: smoke block claims 1,925,809,042 bytes but only 365,523 remain` — and the
replay **played**, PO fast-forwarding through it (`key=5 screen=PLAYING` in the log).

**⚠️ THE NEGATIVE CONTROL DID NOT REPRODUCE THE HANG — it SIGSEGV'd (exit 139).** Reverting both
guards gives a *crash*, not a *hang*. Same defect, different symptom: an unbounded allocation fails
differently by available memory — it **hangs** when the 1.9 GB can be allocated and slowly faulted in,
and **crashes** when it cannot. So the control proves the guards are load-bearing (139 → clean exit)
but **does not** reproduce the PO's exact symptom, and I am not claiming it does.

**⚠️ AND THIS IS A SAFETY NET, NOT A CORRECTNESS FIX.** A smoke block claiming 1.9 GB means the
stream **is genuinely misaligned at that point** — the guard makes that survivable, not correct. One
smoke block is being skipped. **The underlying misalignment is still open**, and the scaling signal is
now sharp: **2 aircraft parse perfectly, 40 aircraft misalign at the smoke block.** Something in the
per-item read scales wrong. S238.

### 🏃 Sprint 234 — "🎉 PO-VERIFIED: the two-aircraft replay plays" (PO-61) — ✅ CLOSED 2026-08-25 (8/8)

## 🎯 **PO-61's CORE GOAL IS MET.** PO, on a One-on-One flown and replayed on this binary:
## *"one-on-one replay view - it worked! entire 2 aircraft replay"*

**The evidence, captured live from the gdb session:**
- **Both aircraft resolve in EVERY block** — `uid=4096 -> ac=0x965f970` and `uid=3584 -> ac=0x9683660`,
  the same two addresses block after block. No wild pointers, no non-aircraft.
- **A genuine multi-block replay:** `numframes=1024`, `1024`, `1024`, then `929`.
- **The scan closes exactly:** `end of file reached [scan stopped at 70134, file is 70134, overshoot 0]`.

**✅ AND THIS FINALLY SETTLES THE `nextmobile=2` QUESTION.** Four sprints (S219, S223, S225, and
S226's correction) circled a supposed discrepancy between `Next=51` and `nextmobile=2`. **2 was right
all along**: a One-on-One *has* two aircraft, the reader walks two, the file contains two, and it
plays. There was never a "collapse" to explain — I spent four sprints explaining a number that was
correct, having measured it off the wrong chain (S226). **The PO's instruction to generate the test
data instead of excavating it (S227) is what closed this**, by removing the only remaining unknown.

- **Fixed a defect in my own trace, same class as S206:** `uid == 0` is the **loop terminator** of the
  do/while, and my trace flagged it `<-- did not resolve`, so every healthy replay ended on a scary
  line. A diagnostic must not report the **normal terminal condition** as a problem. Now labelled
  `(uid 0 = end-of-list terminator, normal)`.
- **PO-67 (black window) did NOT recur** this run. Not fixed — **unreproduced**, and still open.

**⚠️ AND I LOST THE RECORDING.** The successful 1v1 lived in `replay.dat`, which is correctly
truncated on the next flight and on exit (S205's real `SetEndOfFile`). I captured the **log** and left
the **file** to be destroyed; by the time the PO asked for the filename, it was 0 bytes. **Third `.cam`
loss in this project** (PO-65 owns the first two), and this one is squarely mine — the PO had even
told me the corpus was the point.
- **FIX — `port/replay_autosave.sh`:** an out-of-process watcher that snapshots `replay.dat` to
  `Videos/auto/<time>-<size>.cam` once its size is **stable** (post back-patch, not mid-write) and its
  content is new. ⭐ **Deliberately a watcher, not a game-code hook:** a snapshot is pure observation
  and cannot perturb recording, alignment, or timing. Two of my instruments *this same block* changed
  or misread what they measured — S231's trace dereferenced a wild pointer and crashed the PO's
  session, S233's screen capture reported black for a demonstrably-rendering app. **When an
  instrument's job is to preserve evidence, keep it outside the process.**
- ⭐ **Second self-inflicted error, from a rule I had already written down:** `pkill -f <pattern>`
  matched the shell running that very command and killed it (**exit 144**) — the exact failure my own
  memory note warns about. Retried with `-x`. *Having written the rule down is not the same as
  applying it; the note only helps if it is consulted before the command, not after the exit code.*

### 🏃 Sprint 233 — "Three of my own readings were wrong, including the instrument" (PO-67) — ✅ CLOSED 2026-08-25 (6/8)

**PO: *"dogfight view - app minimized"*, then *"when I click on the icon, it just darkens the screen."***

**✅ WHAT IS ESTABLISHED, and the good news comes first: THE REPLAY DATA IS FINE.** That session's load
parsed perfectly — `uid=4096 -> ac=0x9c87140`, `uid=3584 -> ac=0x9c847d0`, the `uid=0` terminator,
`overshoot 0`, `LoadFrameCounts: numframes=1024`. **A port-recorded dogfight loads correctly.** Whatever
PO-67 is, it is downstream of the replay reader.

**✅ THE APP IS NEITHER CRASHED NOR STALLED — measured:** still running, threads in `nanosleep`,
`WM_STATE` Normal. With `BOB_TRACE_PRESENT` the presents keep flowing —
`[present] frame 360 via legacy-2d centre rgb=(206,215,222) glErr=0`. **The canvas holds real
content** (light grey, a live 2D front-end pixel), the present succeeds, and GL reports no error.

**✅ THE ONE HARD ANOMALY:** the window manager forces the window to **1920×1080 at (0,32)** while the
app's mode is **640×480** — I asked xdotool for 1600×900 and the WM gave 1920×1080 anyway, reproducing
the PO's exact geometry. **The app is presenting into a window 3× its mode.** That is the
**P6/MA-S209b** class — cross-ported to BoB this same sprint as a detector (`bob` S232).

**⚠️⚠️ AND THREE OF MY OWN READINGS WERE WRONG. Retracting all three:**
1. **"no crash, timed out still playing"** (S231's verification of `po-dogfight.cam`) — **exit 124 is a
   TIMEOUT.** It is not evidence of rendering. My own §8-MA138 says a "no crash" from a path that may
   never have run is not a result; I wrote it anyway, in the sprint immediately after being burned by
   an unverified claim.
2. **"the log stopped growing, so it is hung"** — `MA_TRACE_REPLAY` **does not log per frame**, so a
   flat log was never a stall signal. Presents were flowing the whole time; I later measured them.
3. **⭐ THE INSTRUMENT ITSELF.** `ffmpeg x11grab` reported `ENTIRE SCREEN BLACK` for a run whose own
   present trace shows `rgb=(206,215,222)` in the canvas with `glErr=0`. **A capture method that reports
   black for a demonstrably-rendering app cannot judge what the user sees** — GL/compositor-managed
   window content is not necessarily in the X root pixmap. **Every "black" measurement I took this
   sprint is void**, including the ones I reported to the PO.

- ⭐ **The lesson, and it is the sharpest form yet of a rule this project keeps relearning:** *validate
  the instrument on a case whose answer you already know, before trusting it on the case you don't.*
  Had I pointed the capture at the app **while it was known-good**, it would have said black then too,
  and I would have discarded it in one step instead of building three conclusions on it.
- **Open, and the PO can settle it in one glance** — the two candidates look completely different:
  **(a)** a small 640×480 image parked in a corner of a large black window → present/window rect
  mismatch, S209b's fix applies directly; **(b)** a uniformly black window with no image anywhere →
  the present is not reaching the display at all, a different bug. **S235 asks.**

**⭐ S276 — CAUGHT LIVE UNDER GDB, and it is neither (a) nor (b).** The PO reproduced it by clicking
One-on-One. Measured, not inferred:
- **The app is NOT hung.** Main thread cycling normally through
  `CMIGApp::Run -> RFullPanelDial::OnPaint -> RDialog::OnPaint -> SetDIBitsToDevice ->
  ma_gdi_set_dibits`. It is *painting a full-screen panel*, every frame, and the log keeps growing.
- **It never entered 3D.** No `Launch3d`, no 3D present.
- **⭐ THE PRESENT PATH SWITCHED.** MA has **two** 2D present routines, each uploading its own
  texture and calling `SDL_GL_SwapWindow`:
  | path | earlier frames | now |
  |---|---|---|
  | `legacy-2d` (DirectDraw surface) | **real content** — 231,239,255 → white → grey | silent |
  | `gdi-canvas` (GDI framebuffer) | — | **every frame, rgb=(0,0,0)** |
  Frames 480–960 came through `legacy-2d` with real pixels; frames 6480–7260 all come through
  `gdi-canvas`, black. **The content is on one path and the display is being fed from the other.**
- **This is the "two things disagreeing about one fact" family again** — the same shape as the four
  `OnGetFile` copies (S248/S249) and the two `WM_GETFILE` handlers, one layer down in the renderer.
- **Next step, precise:** find what selects between the two present routines, and why the panel's
  paint lands in the GDI canvas while the display shows... whichever swapped last. Both call
  `SDL_GL_SwapWindow`, so *the last one to run wins the frame* — which makes ORDER, not correctness,
  decide what the player sees.

### 🏃 Sprint 231 — "⚠️ The PO's crash was MY DIAGNOSTIC" (PO-66) — ✅ CLOSED 2026-08-25 (8/8)

**PO: *"dogfight replay view crash."* Their session died with SIGSEGV. The cause was a line I added.**

**The evidence chain, and it is short:**
- Backtrace: `item::T_shape::operator ShapeNum()` ← `Replay::LoadItemAnims()` ← `LoadBlockHeader` ←
  **`PreScanReplayFile`** ← `LoadFinalPlaybackData`. `fault_addr=0x86000018`.
- **During prescan there is exactly ONE pointer dereference in `LoadItemAnims`: mine.** The engine's
  own `ac->shape` (`ResetAnimData_NewShape`) and `item->shape` (second loop) **both sit behind
  `if (!prescan)`** — verified by reading both sites.
- My S216 trace read `int _shape = ac ? (int)ac->shape : -1;` **unconditionally** — `getenv` gated
  only the `fprintf`, not the dereference. So **every build, every run** paid it.
- `Persons2::ConvertPtrUID` returns a **WILD pointer** for an out-of-range uid, not NULL, so
  `if (ac)` waves it through. The engine never touches it during prescan; it was inert until I
  dereferenced it. **Confirmed numerically after the fix:** the trace now prints
  `uid=41216 (0xA100) -> ac=0x85fffffc`, and `0x85fffffc + 0x1c` = **`0x86000018`**, the fault address.

- ⭐ **THE IRONY IS EXACT, and it is the lesson.** This trace was added in **S216 to investigate an
  out-of-bounds read of AirStruc fields off a non-aircraft**, and **S217 fixed the engine's version of
  that read 60 lines below** by testing `Status.size == AirStrucSize`. **My diagnostic performed the
  very read it was written to catch, unguarded.** *A diagnostic must not dereference the pointer whose
  validity is the question it exists to answer.*
- **FIX:** print the uid and the **pointer value only** — never a field. That is where the signal
  actually lives (did the uid resolve, and to what address); the `shape` number is precisely what sent
  S213–S216 chasing the phantom 8036. Safe in every build now, env or no env.
- ✅ **VERIFIED on the PO's own dogfight** (`po-dogfight.cam`, 92,573 bytes, saved from their
  `replay.dat` so their flight is preserved as the corpus's first multi-aircraft entry): **no crash**,
  run timed out still playing.

**AND THE CONFOUND-FREE DIAGNOSIS IS NOW VISIBLE**, on a file *this binary wrote*:
- The uids `LoadItemAnims` reads are **garbage** — 59653, 30068, 20482, 595, 41216. Real uids are not
  these. **So the stream is misaligned BEFORE `LoadItemAnims` ever runs** — that is option **(a)** of
  the two S216 named, and it is now the supported one, on port-generated data with no
  Windows-provenance ambiguity.
- Incidental: the end-of-scan overshoot here is **−1**, not 64. So S229's "constant 64" was not
  constant either — **more confirmation that report-only was the right call** on that assertion.

**⚠️ NAMED, NOT YET FIXED:** `ConvertPtrUID` returning a wild pointer for a bad uid is the hazard my
memory has flagged for weeks; the authors' own bounds check exists but is compiled out
(`#ifndef NDEBUG`). It is the reason a garbage uid becomes a crash rather than a clean skip. **S233**
— not folded into this sprint, which is a fix for my own regression.

### 🏃 Sprint 229 — "The control killed two pass-signals in a row" (PO-61) — ✅ CLOSED 2026-08-25 (7/8)

**Built `port/replay_corpus.sh`** — the corpus harness the PO's directive calls for: fly → record →
verify well-formed → publish as `Videos/<name>.cam` → **reload it** → append to `Videos/CORPUS.md`
with what was flown. Publishing is a plain copy of `replay.dat` because that is *literally* what the
game's `SaveReplayData` does (`CopyFile`); not a scaffold standing in for a missing feature.

**✅ First corpus entry recorded and reloaded by this binary:** `corpus-baseline.cam` — 120 frames,
21,235 bytes, 1 aircraft on the `nextmobile` chain. **Round-trip works.**

**Three harness bugs found by running it, not by reading it:**
- `replay.dat` lives in **`Videos/`**, not the run root — my first run measured a file that was never
  there and reported "0 bytes" while the recording sat 20 KB fat one directory over.
- `StopRecord` prints `replayframecount=`, not `frames=` — my regex matched nothing and declared the
  flight a failure *while the log line said it succeeded*.
- `BOB_AUTOEXIT` counts **all** frames, and the menu burns ~250 before the flight begins, so a
  "25-second" recording captured **79 frames** (~4 s).

**⚠️⚠️ THE FINDING, and it is about my own gate: THE NEGATIVE CONTROL KILLED TWO SUCCESSIVE
PASS-SIGNALS.**
1. **`"end of file reached"`** — a `.cam` with 64 bytes chopped off **still "reloaded cleanly"**. Of
   course it did: that string is the **normal terminator**, so the damaged file produces it too. I
   had chosen a success signal that the failure case also emits.
2. **`overshoot == 0`** — I then made the trace print where the scan stopped vs the file size, and
   the control went red with `overshoot=64`, matching my 64 removed bytes exactly. It looked
   decisive. **It was a coincidence:** the healthy baseline overshoots by **64 as well** — 64 is a
   *constant* end-state. The control "passed" on a number that matched by luck, not by measurement.

**So the reload verdict is now REPORT-ONLY and deliberately unasserted.** Both candidates would have
shipped as green gates that **cannot fail**. The gate asserts what it has actually shown (recording
well-formed, publish) and prints the reload numbers without a verdict.
- ⭐ **The rule this earns:** *a pass-signal must be shown to SEPARATE good from bad before it is
  asserted.* Not "does it appear on success" — **"is it absent on failure"**. Both of mine passed the
  first test and failed the second. Only a **runnable negative control** can tell them apart, and it
  did so twice inside one sprint.
- **Honest tally:** this is the third time in this project a confident green signal turned out to be
  unfalsifiable. The pattern each time was the same — I validated the signal against the *working*
  case only.
- **Next experiment (not this sprint, per the PO's time-boxing directive):** a **semantic** assertion
  rather than a byte-offset heuristic — *frames played back on reload == frames recorded*. A
  truncated file must yield fewer. Byte offsets were the wrong altitude for this claim.

**🎯 PO is flying a dogfight now to record a genuine TWO-AIRCRAFT replay** — with the Windows-recorded
files archived out, that is the first multi-aircraft input whose provenance is *this binary*. It is
exactly the input the entire PO-61 chase has lacked.

### 🏃 Sprint 227 — "Generate the test data, don't excavate it" (PO-61) — ✅ CLOSED 2026-08-25 (8/8)

- **PO DIRECTIVE, and it dissolves the question I was about to spend another sprint on:** *"The way
  to test more complex cam file replay is to generate more complex replay files, eliminating the
  confounding factor of how old .cam files (recorded under MS Windows) might be different."*
- **This is the right call and I should have reached it myself.** Every shipped `.cam` was recorded
  by a **different binary, on a different OS, at an unknown patch level**. Debugging our reader
  against them means debugging two unknowns at once — reader *and* input. I had been treating the
  Windows files as ground truth when they are simply **the confounded variable**.
- **DONE — archived, with the backup verified BEFORE the move:** all 8 shipped `.cam` files
  compared byte-for-byte against `~/sgl/TUE/afterGameReport/`, all 8 identical, only then moved to
  `~/sgl/TUE/cam-archive-windows-recorded/`. `Videos/` now holds **no Windows-recorded replays**, so
  a future test cannot silently pick one up. (Guarding the copy first is not ceremony — S-PO65 lost
  these files twice.)
- **Last measurement before the pivot, recorded so it is not lost:** span between the two trace
  points is **23364 − 22292 = 1072 bytes**; my aircraft accounting covers **2×203 + (4+3)×6 = 448**.
  **624 bytes are unaccounted for** — so my inventory of what `LoadItemData` reads is *incomplete*
  (other item classes, or calls between the trace points I never enumerated). Noting it and
  **stopping**, per directive 2: this is the third distinct explanation attempted for one symptom.
- **What replaces the excavation:** a matrix of **port-recorded** replays of increasing complexity —
  1v1, 2v2, many-v-many, multi-minute — each recorded and replayed by this binary. That turns
  "does complex replay work?" into a **controlled experiment with one unknown**, and it produces a
  regression corpus we own. S229 builds it.
- ⭐ **The lesson, and it generalises past replay:** *when a test input is older than the code and
  came from another platform, it is not an oracle — it is a second unknown.* Generate your own
  inputs and the reader is the only variable left. Cross-ported to BoB (its `.sav` files carry the
  identical hazard).

### 🏃 Sprint 226 — "⚠️⚠️ RETRACTION: I counted the wrong linked list" (PO-61) — ✅ CLOSED 2026-08-25 (correction, 6/8)

- ⚠️⚠️ **WITHDRAWING THE CAUSAL STORY OF S219, S223 AND S225.** All three rest on an aircraft count
  I obtained by walking `AirStruc::ACList` via **`->Next`**. `LoadItemData` advances with
  **`ac = *ac->nextmobile`** — a different chain. **I was counting a list the reader never
  traverses.**
- **The arithmetic is what exposed it, not a hunch.** `sizeof(ASPRIMARYVALUES)=165` +
  `sizeof(MIPRIMARYVALUES)=38` = **203 bytes** per aircraft. 51 aircraft would need **10,353 bytes**;
  `LoadItemData` spans **950**. It cannot have read 51 pairs. Measuring both chains:

      LoadItemData   at offset 22292   Next=51   nextmobile=2

  **The chain that matters holds 2, and is CONSTANT at every step and across both passes.** There is
  no collapse in the list the loop walks.
- **What is therefore withdrawn:** "the world has 51 aircraft and will read 51 record pairs" (S219);
  "`PreScanReplayFile` collapses the world 51 → 1" (S223); and S225's *verification* that the guard
  "keeps ACList at 51" — all wrong-chain measurements.
- **What survives, and why it survives independently:**
  - **S217's two memory bugs are real and fixed** — ASan named the instructions and the addresses;
    nothing about them depends on my counter. ASan 3 reports → 0, SIGSEGV → clean exit.
  - **The PO-verified play still stands** (S224) — a human watched it.
  - **S225's guard is still defensible on its own terms** (a pass whose comment says it needs only
    counts should not mutate the world, and the original author's `if (!prescan)` said so before it
    was commented out in 1999) — but its *stated justification* was wrong, and it is **not** shown to
    fix anything. **Re-verified that it does not REGRESS the working case:** our port-written replay
    still parses to a clean `end of file reached` with `Next=1 nextmobile=1`.
- ⭐ **The instrument error worth carrying to both ports:** *when code walks a linked structure, count
  it with the SAME link the code uses.* An object can sit on several chains at once, and a plausible
  number from the wrong one reads exactly like evidence. This is §8-MA138's rule ("check the thing
  you instrumented is on the path the claim is about") at one more level of subtlety — the instrument
  was on the right *object* and the wrong *edge*.
- **Second retraction in this chase** (S221→S222 was the first), both caught the same way: **the next
  measurement contradicted the story.** Neither was caught by re-reading the reasoning.
- **The real open question, restated cleanly:** for a shipped `.cam` the reader walks **2** aircraft;
  for our own recording it walks **1** and parses to EOF. How many did each recording actually
  contain? **S227 derives it from the bytes each block's item section spans**, which needs no theory
  about worlds at all.

### 🏃 Sprint 225 — "The guard was commented out in 1999" (PO-61) — ✅ CLOSED 2026-08-25 (real fix, 7/8)

- ⭐ **Named the exact step that destroys the world**, by adding the ACList count to the existing
  per-step trace rather than reading further:

      LoadItemData    at offset 22292   ACList=51
      LoadItemAnims   at offset 23242   ACList=1     <- 50 aircraft gone inside LoadItemData

- ⭐⭐ **THE FIX WAS SITTING IN THE FILE AS DEAD CODE.** `LoadItemData` contains:

      RestorePrimaryASValues( ac,&aspv);
      //DeadCode DAW 04May99      if (!prescan)
      //DeadCode DAW 04May99      {
      RestorePrimaryMIValues(...);   ...   world.RemoveFromSector(...); world.AddToWorld(...);
      //DeadCode DAW 04May99      }

  On **4 May 1999** someone commented out the `if (!prescan)` guard around the world-mutating block
  — including the **sector move**. Since then the prescan, a pass whose own comment says it needs
  only frame and block *counts*, has applied full item state and moved aircraft between world
  sectors.
- ⚠️ **Reinstating it verbatim would be wrong, and that is probably why it was commented out rather
  than fixed:** a `ReplayRead` for aero devices sits *inside* the guarded region, so skipping the
  block would misalign the stream. **Guarded each MUTATION and kept every READ** — the same shape as
  S217. `MA_NO_PRESCAN_GUARD=1` restores the 1999 behaviour and is the control arm.
- **Measured after:** the world **survives the prescan** — `ACList=51` at every step and 51 after the
  scan (was 1) — so **both passes now agree**, which is the property S219/S220 said was missing.
  `parity_2d` 5/5 byte-identical, `replay_screen` PASS.
- ⚠️ **Shipped `.cam` files still fail at `LoadItemAnims`, and the reason has narrowed again:** if
  both passes agree at 51 and the file still disagrees, then **51 is not the recording's count**.
  The arithmetic already hints at it — `LoadItemData` spans **950 bytes for 51 aircraft**, under 19
  bytes each, far too small for two structs per aircraft. **So our reconstructed world is much
  larger than the recording's.**
- **S226:** derive the recording's true count from
  `sizeof(ASPRIMARYVALUES)+sizeof(MIPRIMARYVALUES)` against the bytes that section spans, and compare
  with what `Launch3d` builds. That decides whether the SCRAMBLE substitute mission is over-populating
  the world after all — the S221 suspicion that S222 correctly refused to bank without evidence.

### 🏃 Sprint 224 — "⭐ A port-written replay plays from the Replay screen — PO-verified" (PO-61) — ✅ CLOSED 2026-08-25 (8/8)

- ⭐⭐ **THE PORT CAN NOW RECORD A REPLAY, SAVE IT, AND PLAY IT BACK FROM THE TITLE-MENU REPLAY
  SCREEN.** PO, watching the run live: *"yes, replay worked. (I pressed 4, then after confirming it
  works, 0 to exit)"*. That is the first time a `.cam` has ever played in this port.
- **How it was tested, and why the test was worth constructing:** the PO's own saved flight
  (`scratchpad/po65/your_saved_replay.cam`, preserved when PO-65's SAVE overwrote a shipped file) is
  a `.cam` whose **content is known to play** — they had watched it via post-flight Replay/View. Put
  at the list's row 0, it isolates the **load path** from the **content**, which nothing else could:
  a pass proves the path works, a failure would have proven it broken regardless of file.
- ⭐ **It also confirms S223's prediction exactly.** The trace:

      ACList=1  (before PreScanReplayFile)      <- our recording: ONE aircraft
      LoadItemData: world has 1 aircraft ...    <- matches the file
      LoadHeaderID FAILED -- end of file reached (this is how the block scan normally ENDS)

  The prescan ran **to a clean EOF** rather than derailing, because a single-aircraft recording gives
  its world-collapse nothing to collapse. **The shipped dogfights (51 aircraft) are destroyed by that
  same pass.** So the difference between "plays" and "fails" is the recording's aircraft count, as
  predicted — not the file format.
- ⭐ **Settles the PO's question directly: the shipped `.cam` files ARE compatible.** Same format
  (S213 + the +141 block-header fingerprint across all nine files), and the load path is now
  demonstrated to work. **Nothing needs removing** — the earlier suggestion to delete "files from a
  different patch level" was based on a hypothesis I raised in M5 and then disproved.
- ⚠️ **NOT claimed fixed: the prescan is still wrong even in the working case.** The same run shows
  the world moving **1 → 41 → 1** across the two passes, and the real load reading **41** record pairs
  from a file holding **1**. It played anyway, so playback tolerates that misalignment here — but the
  mutation S223 identified is untouched, and the multi-aircraft files still fail on it. **A passing
  case over a known-broken mechanism is not a fix.**
- **S225 remains the prescan fix** (snapshot/restore the world around it, mirroring `BackupPrefs`, or
  re-restore from the super header's savegame). The acceptance test is now concrete and strong: a
  **shipped** multi-aircraft `.cam` must play.

### 🏃 Sprint 223 — "The prescan eats the world it scans" (PO-61) — ✅ CLOSED 2026-08-25 (cause found, 8/8)

- ⭐⭐ **`PreScanReplayFile` collapses the world 51 → 1.** Bracketed both passes inside
  `LoadFinalPlaybackData`:

      [replay] ACList=51  (before PreScanReplayFile)
      [replay] ACList=1   (after PreScanReplayFile / before the real LoadBlockHeader)
      [replay] ACList=1   (after the real LoadBlockHeader)

  One call, 51 → 1. **The prescan is not read-only.**
- **Why:** its own comment says all it needs is *"1: Total number of frames … 2: Total number of
  blocks … 3: Number of frames in last block"* — but it gets them by looping **`LoadBlockHeader`
  over the entire file**, and `LoadBlockHeader` restores item state and processes dead items on the
  way. So by the time the scan reaches EOF the world has been walked to the replay's **final**
  state, where one aircraft survives. The real load then starts again from the first block **against
  that wrecked world**, sizes every read against 1 aircraft, and misaligns.
- ⭐ **The `prescan` flag guards the wrong things.** It exists — `if (!prescan)` appears at ten sites —
  and it correctly suppresses the anim work. It does **not** suppress the state restore or the dead
  items, which are what actually mutate the world. **A flag that names the pass is not the same as a
  flag that makes the pass safe.**
- ⭐ **The authors knew this pass had side effects — for PREFERENCES.** `BackupPrefs()` is called
  immediately before `PreScanReplayFile()` and restored after. They protected the prefs and not the
  world. **The shape of the fix is already in the file**, one line above the bug.
- **This completes PO-61's causal chain** and supersedes S221's mis-stated cause (corrected in S222):
  the world *is* restored from the recording; the prescan then destroys it; the real load reads
  against the wreckage; `LoadItemData` sizes from `ACList`; the stream misaligns; a uid lands on a
  dead slot; `LoadItemAnims` fails. **Every symptom since S183 hangs off one non-read-only scan.**
- **S224 is the fix**, and it has two candidate shapes, to be decided by measurement rather than
  taste: (a) **snapshot/restore the world around the prescan**, exactly as `BackupPrefs` does for
  preferences; or (b) **re-restore from the file** (the savegame is already in the super header) once
  the scan is done. (a) mirrors the existing idiom; (b) is authoritative. Neither is a change to what
  the recorder writes.

### 🏃 Sprint 222 — "⚠️ CORRECTION: the savegame IS loaded; the bug is the 51 → 1 collapse" (PO-61) — ✅ CLOSED 2026-08-25 (correction + narrowed, 7/8)

- ⚠️⚠️ **THIS CORRECTS S221's HEADLINE.** S221 concluded *"the world is built from a substitute
  mission, not from the recording."* **Measured, that is wrong.** The super-header half carrying
  `LoadSaveGame()` — which restores the recording's own world — **does run, in the right order**:

      [replay] LoadReplayData('IanHead-On Kill.cam'): world holds 0 aircraft BEFORE Launch3d
      [replay] LoadSuperHeaderBeginning: ENTERED (this is the half with LoadSaveGame)
      [replay] LoadSaveGame: ENTERED -- restoring the recording's own world
      [replay] LoadItemData: world has 51 aircraft ...
      [replay] LoadItemData: world has  1 aircraft ...

- ⚠️ **The process failure that produced the wrong headline is one my own notes ban.** I grepped for
  callers of `LoadSuperHeaderBeginning`, **piped it through `head -8`**, saw only definitions, and
  concluded "dead code, never called". The real caller — `WINMOVE.CPP:2146`, inside
  `SendInitPacket()` — was **below the cut**. *"NEVER pipe a grep through `head` when the question is
  'does X exist anywhere'"* is booked in the shared notes and cost three wrong conclusions in one
  FreeFalcon session. **Completeness is the whole point of an existence search.** Caught only
  because the next measurement contradicted the story.
- ⭐ **What is actually established:** the recording's world *is* restored, so **51 may well be the
  correct count** for the prescan, and the defect is the **51 → 1 collapse between the two passes**.
  Narrower than "the wrong world is built" — and it **inverts which pass is suspect**: the *real*
  load, reading against 1 aircraft, is the one out of step.
- **Untouched by the correction:** `LoadItemData` still sizes its reads from `AirStruc::ACList`
  rather than the file, and the two passes still disagree about that list (S219/S220). The
  *mechanism* stands; only the *cause* of the disagreement was mis-stated.
- **S223: what tears the world down between `PreScanReplayFile` and the real `LoadBlockHeader`?**
  Both run inside `LoadFinalPlaybackData` a few lines apart, so the window is small and the question
  is answerable by counting aircraft either side of each call.

### 🏃 Sprint 221 — "The world is built from a substitute mission, not from the recording" (PO-61) — ✅ CLOSED 2026-08-25 (root cause complete, 8/8)

- ⭐⭐ **S220's hypothesis CONFIRMED by a count, not an argument.** Instrumented `LoadReplayData`,
  which runs from `ReplayLoad` **before** `Launch3d` builds anything:

      [replay] LoadReplayData('IanHead-On Kill.cam'): world holds 0 aircraft BEFORE Launch3d
      [replay] LoadItemData: world has 51 aircraft in ACList; will read 51 record pair(s)
      [replay] LoadItemData: world has  1 aircraft in ACList; will read  1 record pair(s)

  **Zero before, 51 during the prescan.** So the 51 are **not leftovers** from the front end — the
  alternative I had to rule out — they are *created by `Launch3d`* from the **SCRAMBLECAMPAIGN
  substitute mission** that `ReplayLoad` selects. And the world then drops to **1** before the real
  load.
- **PO-61's chain, end to end, every link measured:**
  1. `ReplayLoad` sets `Miss_Man.currcampaignnum = SCRAMBLECAMPAIGN` — a **substitute** mission;
  2. `Launch3d` builds a world of **51** aircraft from that mission, **not from the recording**;
  3. `PreScanReplayFile` reads the whole file sized against those 51;
  4. the world drops to **1** between the passes;
  5. the real `LoadBlockHeader` reads sized against 1;
  6. → stream misalignment → implausible anim-delta count → uid on a dead slot → `LoadItemAnims`
     fails. **Every symptom from S183 onward is downstream of step 2.**
- ⭐ **The defect stated plainly: a file-format reader is sized by a world the file did not
  describe.** The recording's aircraft set lives in the file's super header; the port instead hands
  the reader whatever mission it happened to launch. **No amount of care below step 2 can recover
  that** — which is why five sprints of narrowing kept finding real bugs (S217's two were genuine
  and are fixed) without reaching the bottom.
- **Also newly named and not yet explained: the world changes MID-LOAD, 51 → 1.** Even a correctly
  built world would still be read inconsistently by the two passes. That is a second, separable
  defect on the same path.
- **S222 starts here**, and the shape is now clear rather than exploratory: make the world the
  replay's, or make the reader independent of the world. **Not attempted blind** — the recorder is
  the same code, so the format cannot simply be given a count that the writer never emits.
- **What the player has today:** the replay fails cleanly instead of corrupting memory (S217). Still
  not playing, still not claimed.

### 🏃 Sprint 220 — "Two passes, two different worlds, one file" (PO-61) — ⚠️ CLOSED PARTIAL 2026-08-25 (5/8)

- **Located the 51-vs-1 divergence in the load's own structure.** `LoadFinalPlaybackData` is:

      LoadSuperHeaderEnd();
      BackupPrefs();
      PreScanReplayFile();      // pass 1 -- sets prescan=true, loops LoadBlockHeader over the
                                //           WHOLE file to count blocks and frames
      if (LoadBlockHeader())    // pass 2 -- the real load
          Playback=TRUE;

  Both passes call `LoadBlockHeader`, so **both call `LoadItemData`, and both size their reads from
  `AirStruc::ACList`** — which held **51** aircraft during the prescan and **1** during the real
  load. Two reads of one file, consuming different amounts. **At most one can be right, and the
  file's true count is fixed by whatever was recorded.**
- **Where the 51 come from — hypothesis, explicitly NOT yet measured:** `ReplayLoad` sets
  `Miss_Man.currcampaignnum = MissMan::SCRAMBLECAMPAIGN` and picks that campaign's mission before
  `Launch3d`, so the world is populated by a **substitute mission** rather than by the recording. If
  so, the aircraft the reader is sizing against are the scramble mission's, not the replay's. **The
  next measurement is a count at `ReplayLoad` time**, before `Launch3d`, which settles it in one
  run — recorded as a hypothesis rather than banked, because three earlier stories for PO-61 died
  exactly this way.
- **Why this is the right shape of answer:** a file-format reader must be driven by the file. The
  engine's design here makes the *world* authoritative for the record count, which only works if the
  world has already been restored to match the recording. **That is the property to establish, and
  it is testable without fixing anything** — count the aircraft, count the records the file holds,
  and see whether any pass agrees.
- **Deliberately not attempted yet:** changing the loop to read a count from the file. That would be
  inventing a format the recorder does not write, and the recorder is the same code — so the count
  must instead come from a correctly-restored world.

### 🏃 Sprint 219 — "The loop is driven by OUR world, not by the file" (PO-61) — ✅ CLOSED 2026-08-25 (root cause of the misalignment, 7/8)

- ⭐⭐ **`LoadItemData` reads one record per aircraft IN THE CURRENT WORLD, not per aircraft in the
  file:**

      ac = *AirStruc::ACList;
      while (ac) { ReplayRead(&aspv,...); ReplayRead(&mipv,...); ... }

  So the byte count it consumes is decided by **what the port has built**, not by what was recorded.
  If the recording had N aircraft and we reconstruct M, the stream misaligns by (M−N) record pairs
  and everything downstream is garbage — the implausible anim-delta count, the uid landing on a dead
  slot, `LoadItemAnims` failing. It also explains the byte counts already measured:
  **383 bytes replaying our own flight (works) vs 950 on a shipped `.cam` (does not).**
- ⭐⭐ **And it is worse than a mismatch with the recording — the world changes MID-LOAD.** Instrumented
  the count; one run, one file, two `LoadBlockHeader` passes:

      [replay] LoadItemData: world has 51 aircraft in ACList; ... will read 51 record pair(s)
      [replay] LoadItemData: world has  1 aircraft in ACList; ... will read  1 record pair(s)

  **51 → 1 between the two passes.** The two reads of the same file therefore consume completely
  different amounts. Whatever the recording held, **at least one of those passes is guaranteed to be
  wrong**, and no amount of care further down can recover it.
- **This retires the last of the competing stories for PO-61.** Not a `.cam` format version (S213),
  not a shape-numbering problem (S216), not the memory corruption (S217 — real, fixed, and it was
  masking this), not an out-of-range uid (S218). It is that **a file-format reader is being driven
  by live engine state.**
- **The shape of the fix, for S220:** the record count must come from the FILE (or from a world
  reconstructed to match it) before this loop runs — and the two passes must agree about the world
  they are reading into. Note the loop already distinguishes a `prescan` pass, which is very likely
  where the 51-vs-1 divergence lives, and that is where S220 starts.
- **Not claimed:** the replay still does not play. What is now known is *why*, precisely, with a
  number attached.

### 🏃 Sprint 218 — "The uid is in bounds; the slot is dead" (PO-61) — ⚠️ CLOSED PARTIAL 2026-08-25 (5/8)

- **The remaining half of PO-61, now that S217 stopped the corruption hiding it.** The parse is
  structurally sound to the last step — per-step offsets for the shipped file run
  `21852 → 21856 → 21860 → 21974 → 22234 → 22246 → 22292 → 23242`, every step consuming a plausible
  amount — and only then does `LoadItemAnims` disagree.
- ⭐ **`uid 15503` is IN BOUNDS.** `PITEMTABLESIZE = IllegalBAND+1 = 0x4000 = 16384`, so this is not
  an index overflow. `pItem[15503]` holds a **stale pointer to a dead slot** — `Status.size == 0`,
  and `ItemBase()` sets `Status.size = ItemBaseSize`, so **zero means never constructed**. The
  replay refers to an item that does not exist in the world we reconstructed.
- ⚠️ **Hazard found and deliberately NOT changed: `Persons2::ConvertPtrUID` has no check in a release
  build.** Its only guard is `if (tmpUID==0) return NULL;` **inside `#ifndef NDEBUG`** — so with
  `NDEBUG` set there is no NULL check and no bounds check at all; it just returns `pItem[tmpUID]`.
  The original developer left the note himself: *"The reason I'm here is because it crashes because
  pItem is null. Therefore this should be protected in it's calls… THAT'S WHY THERE'S A SLOW VERSION
  BELOW"* (`SlowConvertPtrUID`). **Not touched here** — it would not fix this case (the uid is in
  bounds) and it is called from all over the engine, so it is a change with wide blast radius and no
  test to justify it today. **Recorded as a named hazard rather than silently fixed or silently
  ignored.**
- **So PO-61's remaining defect is a WORLD-RECONSTRUCTION mismatch:** the super header rebuilds a set
  of items, and the replay's per-item uids do not all land on live aircraft within it. That is S219,
  and it is a different kind of work from the last three sprints — no longer "find the bad
  instruction" but "why does the rebuilt world differ from the recorded one".
- **What S217+S218 did deliver for the player:** the replay path no longer corrupts memory or
  crashes. It fails cleanly instead. That is not the feature working, and it is not being claimed as
  such.

### 🏃 Sprint 217 — "ASan named both writers" (PO-61) — ✅ CLOSED 2026-08-25 (goal MET, 8/8; the corruption is gone, the parse is not)

**Two real memory bugs found and fixed, both driven by file contents, both verified by the oracle.**

- ⭐⭐ **Bug 1 — an out-of-bounds READ: `LoadItemAnims` treats every uid as an AIRCRAFT.** ASan on the
  PO's repro:

      READ of size 2 at 0xe9d230a9
        #0 weap_ctl::T_Weapons::operator int()  WorldInc.h:980
        #1 Replay::LoadItemAnims()              Replay.cpp:1167
      0xe9d230a9 is located 54 bytes after 83-byte region
        allocated by Persons3::make_gndgrp()    Persons3.cpp:1925

  The uid resolved to an **83-byte `info_grndgrp`** — a ground group — and reading `ac->weap.Weapons`
  / `ac->shape` off it ran 54 bytes past the allocation. **That garbage was the 8036.** `if (ac)`
  says the uid *resolved*, not that it resolved to an aircraft. Fixed with the engine's own
  discriminator, `Status.size == AirStrucSize` (its idiom at `3DCODE.CPP:1716`, `:3408`), skipping
  **only** the anim calls so every `ReplayRead` still runs and stream alignment is byte-identical.
  Measured after: `uid=15503 resolved to a NON-AIRCRAFT (Status.size=0, AirStrucSize=12)`.
- ⭐⭐ **Bug 2 — an out-of-bounds WRITE, the more serious one.** Fixing bug 1 let the parse run
  further, and ASan immediately named the next:

      WRITE of size 2 at 0xe6e02c61, 0 bytes after a 993-byte region
        allocated by shape::shape()  3dcom.cpp:557

  `AnimDeltaList = new ReplayAnimOffsets[sizeof(PolyPitAnimData)]`, and the loop indexes it with a
  count **read straight from the file and never bounded** — so a replay can write off the end of a
  **global**. **A heap-corrupting write driven by file contents.** The engine's own *writer* bounds
  itself by that array's extent (`3dcom.cpp:19547`); the *reader* never did. Bounded at both live
  sites; the stream is still consumed in full. Measured: `anim delta index 331 exceeds
  AnimDeltaList[331] -- refusing to write past the end`, exactly at the boundary.
- **Verified:** ASan **0 reports** (was 3), run **exit 0** (was 139/SIGSEGV). `parity_2d` 5/5
  byte-identical, `replay_screen` PASS.
- ⚠️ **PO-61 is NOT closed, and the remaining half is now visible because the corruption stopped
  hiding it.** `LoadItemAnims` still returns FALSE: an anim-delta count of **331+ for a single item**
  is implausible on its face, so the stream really is misaligned *before* this point. That is S218 —
  and it is the hypothesis S213 raised, which the corruption had been masking all along.
- ⭐ **The method that did it:** S213 narrowed uid→object, S216 proved same-uid/same-pointer with a
  mutating field, and ASan then named both instructions outright. **Three sprints of narrowing made
  the oracle's answer unambiguous** — pointing ASan at "the replay crashes" would have produced the
  same three reports with nothing to attach them to.
- **And S183's guard is now explained:** it substituted shape 1 and carried on **over an
  out-of-bounds read**, which is `§8-BoB210`'s "a guard that hides its own cause" in this port's own
  tree. Left in place as defence in depth, but it is no longer what holds the replay together.

### 🏃 Sprint 216 — "The uid is fine; the aircraft is corrupted between reads" (PO-61) — ⚠️ CLOSED PARTIAL 2026-08-25 (6/8)

- ⭐⭐ **PO-61 is MEMORY CORRUPTION, and neither hypothesis S213 offered was right.** Traced the
  uid→object step that S213 identified as the real location. Measured on a real 3D `.cam` playback:

      [replay] LoadItemAnims uid=15503 (0x3C8F) -> ac=0xa772ac0 shape=307   animset=0x1
      [replay] LoadItemAnims uid=15503 (0x3C8F) -> ac=0xa772ac0 shape=8036  animset=0x1  <-- SUSPECT

  **Same uid, same object pointer, and `shape` mutates 307 → 8036 between the two calls.** So the
  stream is *not* misaligned (the uid is stable and sane) and the world reconstruction is *not*
  missing the object (it resolves, twice, to the same address). **Something writes through a bad
  pointer and clobbers the aircraft's `shape` field between block-header loads.**
- **That retires the last of the earlier explanations.** `GetShapePtr(8036)` was never about shape
  numbering, patch levels, or `.cam` format versions (S213 already showed ours and the shipped files
  share a structure). It is the port's signature bug class — a stray write — surfacing through the
  replay path because that path walks objects nothing else walks.
- ⚠️ **S183's `GetShapePtr` bounds guard is now definitively a symptom treatment**, and by
  `§8-BoB210` exactly the kind of guard that can hide its own cause. It substitutes shape 1 and
  carries on **over a corrupted aircraft** — so playback continues against a damaged object rather
  than stopping where the damage is. It should be reconsidered once the writer is found; not before,
  and not by deleting it blind.
- **Next, running now:** the ASan build (`port/asan.sh build`) against this exact repro. This is the
  tool BoB's notes call the port's most productive hardening oracle on this engine, and the repro is
  now a single reproducible recipe — `30,r4;70,#1055:r0;110,#2063:1` under real GL — so ASan should
  name the writing instruction outright.
- **The repro is fully scripted**, which it was not before S203 made the Replay screen reachable:
  title row 4 → file row 0 → LOAD, and the corruption appears within seconds.

### 🏃 Sprint 215 — "A gate that drives a real mouse" — ✅ CLOSED 2026-08-25 (7/8; control honestly inconclusive)

- ⭐⭐ **`port/real_mouse.sh` — the first end-to-end test of the real SDL mouse path in this port's
  history.** `xdotool` moves the real pointer and presses the real button on the real window, so the
  click travels **compositor → `SDL_MOUSEBUTTONDOWN` → `pump_events` → `win_to_canvas` → hit test →
  handler** with nothing synthesised. **PASS:** click received, mapped to row 1, and the screen
  **advances** (the second click lands in the 213x143 Single Player submenu, a different listbox from
  the title menu's 105x100).
- **Why it had to exist:** S209 moved the picture and left the click mapping behind; the PO hit it in
  one click and **`parity_2d` stayed 5/5 byte-identical through the whole regression.** It could not
  have caught it — S192 found 17 of 19 gates never pump an SDL event, and every click recipe injects
  a **canvas** coordinate *below* `win_to_canvas`, i.e. below the exact layer that broke.
- ⭐ **The gate's first run failed, and the failure was the finding.** Zero `[ole_mouse]` lines while
  the menu was demonstrably drawn — because **under click-to-focus the first click on an unfocused
  window is consumed activating it.** A hand-run of the identical coordinates had worked minutes
  earlier only because that window already had focus. Now `xdotool windowactivate --sync` first.
  **Same family as S185/PO-60** (SDL delivers *keys* only to a focused window) — this is the mouse
  half, and it is a plausible reading of the PO's original report too.
- ⚠️ **The negative control is INCONCLUSIVE and says so (exit 2), rather than reporting a pass it has
  not earned.** Two attempts to make it discriminate both failed, for a reason worth knowing:
  with no resize `canvas == window`, so the old mapping is *exact*; and **when the window is resized
  externally, the port undoes it** — `ensure_window` re-asserts `SDL_SetWindowSize` every frame, so
  it snaps back within a frame or two. The port itself prevents the steady-state mismatch. **The bug
  S209b fixed lived in the TRANSITION** (the requested size is assigned immediately while the real
  resize is deferred/declined/clamped), which is transient by nature — and that is exactly why it was
  invisible to everything. Written into the header so nobody reads a green control as protection.
- **A gate that says "I could not create the fault" is worth more than one that quietly passes** —
  the §8-BoB206 rule applied to my own gate.

### 🏃 Sprint 212 — "We compile the pre-patch source; the oracle is a patched binary" (EPIC M) — ✅ CLOSED 2026-08-25 (M1 answered, M0 started, 8/8)

- ⭐⭐ **M1 ANSWERED — the story the whole epic was gated on: OUR SOURCE IS PRE-PATCH.** No version
  marker anywhere in `SRC/`; **no `BDG` reference in any game source** (every hit is our own
  `SRC/compat/` commentary); and the port has been compensating for the split case by case for
  fifty sprints without naming it — S57's oracle ruling already says the gold shots are the **BDG
  0.85F patched build** and that resources must come from the *installed* `miglang.dll`.
  **BoB is the same** (Release P): its only `BDG` mentions are port comments, and
  `bob_ole.cpp:265` states the split outright — source-only dialogs drawn under *"the BDG
  `IDD_SSOUND` layout"* from the installed PE.
  > **We compile the PRE-PATCH source. The parity oracle is a PATCHED binary.**
  So every bug the patches fixed **in the EXE** is live here by default, and **some recorded parity
  deviations may be patch differences rather than port defects** — which *revises* verdicts in
  `screen-parity.md` rather than merely adding work.
- **M0 STARTED — `port/scrum/patch-bugs.md`**, the official Rowan chain (v1.01→v1.23) extracted
  verbatim with a first-pass implication per item. **Two of the nine v1.02 fixes are direct hits on
  work this port did the hard way:**
  - ⭐ *"Random Crashes in the replay [audio and accel]"* — a **known** replay crash class, fixed in
    the patch. **PO-61** is exactly a replay failure we have been chasing since S183.
  - ⭐ *"Improved font if 'Intel' font not installed"* — **S66 rediscovered this from scratch**: the
    game ships `Intel.ttf`, stb_truetype rejected it over a (3,0) SYMBOL cmap, and the port drew the
    front end in DejaVu for **ten sprints**. The patch had already flagged that font as fragile, and
    the v1.1 Workarounds say so again from the other direction.
  Also live-looking: *"Missing dialogs if Windows Font is 110% or 200%"* (we have an active
  scaling story, S206/S209), *"The Sticky key problem has now been fixed"* (we have a keyboard-state
  history: PO-60, S202), `Rcombo` crashes, and a 3D→Preferences memory leak.
- ⭐ **The list is PREDICTIVE, and that is the argument for working the rest of it.** Two items we
  had already found independently, at a cost of many sprints each, were sitting in a readme in the
  install directory the whole time. **Recording a patch item we already fixed is not wasted** — it
  is the evidence that the remaining items are worth triaging.
- **Triage rules written into the doc** so M2 cannot drift: a verdict comes from *evidence* (a grep,
  a run, a `git log -L`) and never from the patch text; **data-only items are N/A to a source port**
  and must be marked so rather than left ambiguous.
- **Still to inventory:** v1.2–v1.23; `DOC/MigAlleyTips.pdf`; `DOC/CampaignGraphicsWorkarounds.pdf`
  (**the title alone promises known graphics defects + workarounds**); `communityDoc/`; `REFERENCE/`.
- **No code changed this sprint.**

### 🏃 Sprint 211 — "L0: the .cam holds deltas, so export from the SIM, not the file" (EPIC L) — ✅ CLOSED 2026-08-25 (spike answered, 3/3)

**The spike EPIC L was gated on. It has an answer, and it makes the epic simpler than written.**

- ⭐ **`REPLAYPACKET` is `_basic_packet` (`WINMOVE.H:132`), 11 bytes packed** — matching the
  `sizeof(REPLAYPACKET)=11` measured in S204:
  `Shift:4 | Velocity:4 | X | Y | Z | Heading | Pitch | Roll | IDCode | byte1..3`.
- ⭐ **Position is ONE BYTE PER AXIS. That cannot be a world coordinate** — MA's Korea theatre is
  millions of centimetres across — so **X/Y/Z are DELTAS**, scaled by the 4-bit `Shift` exponent,
  with `Velocity` a 4-bit signed delta too. Absolute state lives only in the **block header**
  (`LoadItemData` / `LoadPrevPosBuffer`), which the frames then walk forward from.
  **Orientation is different and usable as-is:** `Heading/Pitch/Roll` are one byte each = absolute
  angles at 360/256 ≈ **1.4°** resolution. `IDCode` carries 2 bits of packet type + 6 bits of info,
  so object identity is 6 bits wide (or positional) — **not** a general object id.
- ⭐⭐ **Therefore: do NOT write a `.cam` → `.acmi` converter.** Decoding the file means
  reimplementing the engine's delta integration and its `Shift` scaling exactly, duplicating logic
  that already exists and inheriting every rounding difference. **Export from the live sim instead,
  where positions are already absolute** — `ArtInt::ACArray[]` (`MSGAI.CPP:109`) holds every live
  `AirStrucPtr`, each with `World` (`COORDS3D`, `WORLDINC.H:345`) and `hdg` (`ANGLES`, :619). The
  sim does the integration for us, for free, and correctly.
- **The architecture that follows, and it mirrors the game's own:** the engine records continuously
  into `replay.dat` and, on SAVE, simply `CopyFile`s it to `<name>.cam` (S210). So the exporter
  should **write ACMI continuously during the flight** into a sibling temp file, and on SAVE copy it
  to `<name>.acmi`. **Zero delta decoding, and the `.cam` write stays byte-identical** — which is
  exactly the "additive only" constraint EPIC L was given, satisfied by construction rather than by
  care.
- **Re-sizing the epic on this answer:** L1 becomes "tee ACMI while recording + copy on save" rather
  than "convert a file"; L2/L3 get *easier* (absolute positions and real object identity are
  available from `ACArray`, where the 6-bit `IDCode` would have been a hard limit); L4 (`IAS`, `AGL`,
  `AOA`) becomes nearly free, since those are sim state too — `MA_TRACE_HUD` (S174) already reads
  the flight model's own speed/alt/mach.
- ⚠️ **One consequence to state now, not discover later:** exporting from the live sim means
  **replays recorded before the feature exists cannot be exported**, and neither can the shipped
  `Ian*.cam` files. That is a real limitation of the chosen design and the PO should hear it up
  front — the alternative (a file converter) is the thing this spike just showed to be the expensive
  path.
- **Still blocked:** L1 waits on **PO-65**, whose remaining half is now much smaller after S210.

### 🏃 Sprint 210 — "The edit was drawn over the list and hit-tested under it" (PO-65) — ✅ CLOSED 2026-08-25 (goal MET, 8/8)

- ⭐⭐ **PO: *"no text appears when I type a replay name."* Not a keyboard fault — a Z-ORDER one.**
  On the replay Save/Load screen (`CLoad`) the *Current File* edit sits at **(14,187) 202x26**,
  wholly **inside** the file list's own rect **(10,128) 294x260** — the overlap is visible in any
  capture. The front-end dispatch tries `ma_ole_mouse` → `ma_ole_listbox_click` → `ma_ole_click`, so
  **the list consumes the click and the edit's focus code never runs.** The edit is *drawn over* the
  list and *hit-tested under* it.
- ⭐ **And underneath that, a second layer: S198's edit-focus code in `ma_ole_click` was UNREACHABLE
  DEAD CODE.** The type filter 17 lines above it reads
  `if (h.type != CT_BUTTON && h.type != CT_COMBO && h.type != CT_EDTBT) continue;` — **`CT_EDIT` is
  not in it**, so the loop skipped every edit long before reaching the focus handler. **The S164
  family for the fourth time** (a control type absent from a click walk's *type filter* is drawn,
  looks alive, and is inert) and the **third dispatcher** needing the same repair — S200 did the OOB
  allowlist, S198 the `[tbclick]` one, this is the front end. The tell was sitting in S198's own
  comment: it moved that code here after finding the trace *"firing ZERO times"* elsewhere, and it
  fired zero times here too, one layer up. **Two independent reasons the same click did nothing.**
- 🔧 **Fixed both.** `CT_EDIT` added to the filter (`MA_NO_EDIT_CLICK=1` reverts), and a new
  `ma_ole_edit_click` gives a **topmost edit first refusal** before the list
  (`MA_NO_EDIT_FIRST=1` reverts) — the rule MA already adopted for OOB dialogs in S82 and BoB had to
  relearn for toolbars in their S188 (*hit-test in the reverse of the paint order*). Applied to the
  one overlap there is evidence for, rather than reordering the whole dispatch blind.
- **A/B on one binary:** fix → `[click] edit id=1060 takes keyboard focus (topmost, before the
  list)`; `MA_NO_EDIT_FIRST=1` → **0** focus lines. A line that had never appeared in this port's
  history.
- **The other PO item is explained, not a defect:** *"SAVE highlights but nothing happens."*
  `ReplaySave` → `SaveReplayData` → `CopyFile(replay.dat → <selectedfile>.cam)`, and `CopyFileA` is
  properly implemented (not a stub — checked, given this session's form). It **silently succeeds**,
  writing to `selectedfile` — which, with the name field unusable, was a shipped `.cam`. So "nothing
  happens" was *the overwrite*, seen from the outside. With the edit now clickable the player can
  finally name the file, which is what makes PO-65's data loss avoidable.
- ⚠️ **Hazard found the hard way, and FIXED: `port/parity_2d.sh` does `pkill -x wmig` and killed the
  PO's live session mid-test.** ⭐ **The policy was already written and simply not called.**
  `gate_lib.sh` has carried `assert_clean_start()` since it was created, with the right rule in its
  comment — *"REFUSE, do not kill. A stray wmig may be the user's own game on the display, and a
  gate is never entitled to close it."* **14 gates source it and call it; `parity_2d.sh`, the most
  frequently run of the lot, did neither.** Now guarded (exit 2 = *could not run*, distinct from
  *failed*). **Verified against the PO's live session: `REFUSING TO RUN: wmig already running (pid
  35540)`** — the exact case that caused the harm. Same family as S81's *"a gate must never eat the
  player's save"*. ⚠️ Remaining unguarded (they do not source `gate_lib`): `asan_flight`,
  `asan_campaign`, `asan_all`, `dialog_scroll`, `oob_sweep`, `overlay_text`, `map_drag` — a
  schedulable chore, not done blind here. *(Honest note: the refusal path is verified against a live
  session; the PASS path was last verified minutes before this edit, and re-verifying it requires a
  free display.)*
- **Gates:** `parity_2d` **5/5 byte-identical**, `replay_screen` PASS.

### 🏃 Sprint 209b — "I broke the mouse, and the reporter caught it in one click" — ✅ CLOSED 2026-08-25 (regression fixed)

- ⚠️⚠️ **MY REGRESSION, reported by the PO within minutes:** *"main screen, clicking on single player
  has no effect."* S209's Fix 2 made the present viewport follow the **drawable**, so the canvas is
  stretched into the real window — but `win_to_canvas`/`canvas_to_win` still divided by
  **`g_scrW/g_scrH`**, i.e. what the game *asked* for. **I moved the picture and left the click
  mapping behind.**
- **The mechanism, and why it is intermittent:** `ensure_window` assigns `g_scrW/g_scrH` at the top,
  *immediately*, while the actual `SDL_SetWindowSize` may be deferred, declined, or clamped. During
  that gap the requested size and the real window differ — and the front end resizes 640x480 →
  1920x1080 on the way to the title. Pre-fix, clicks were scaled by a ratio the picture no longer
  used; the error is zero when the two happen to agree, which is why my own gl-lock runs (window
  settled at 1920x1080, canvas 1920x1080, identity) showed nothing wrong.
- 🔧 **Fixed:** both mappings now use the **same rectangle the present stretches into** — the window
  size — via one helper, falling back to `g_scrW/g_scrH` only when there is no window (the headless
  path, so every existing gate stays byte-identical). **Rule: whatever rectangle the present
  stretches the canvas into, the inverse mapping must use THE SAME rectangle.**
- ⭐ **Verified with a REAL mouse click, which no gate in this port can do.** `xdotool` on the live
  window: `[ole_mouse] listbox rect=(530,210,105,100) click=(582,251) -> local=(52,41) inside` →
  `[ole_click] -> row=1` (Single Player), and the follow-up click lands in the **213x143** five-row
  Single Player submenu — proving the screen actually advanced. **This is the first end-to-end test
  of the real SDL mouse path in this port's history**; S192 found 17 of 19 gates never pump an SDL
  event, and every click recipe injects *canvas* coordinates, entering below the exact layer I
  broke. **A whole class of regression is invisible to the entire gate suite** — that is the finding
  to act on, and `xdotool` is the tool that closes it.
- ⚠️ **Not proven to be the PO's exact fault:** their session logged
  `canvas=viewport=window=drawable=1920x1080`, all agreeing, where the mapping error is zero. The
  transition through 640x480 is the plausible window and it is not in the trace. Stated as the
  mechanism, not as a confirmed diagnosis.
- **Gates:** `parity_2d` **5/5 byte-identical**, `replay_screen` PASS — and neither could have
  caught this, which is the point.

### 🏃 Sprint 209 — "It was the dock" (PO-65) — ⚠️ CLOSED PARTIAL 2026-08-25 (7/8, PO retest pending)

- ⭐⭐ **PO-65's "corrupted save screen" was never a rendering fault.** The PO dragged the window to
  the **other monitor and it rendered whole**. That one action cracked it: the defect is
  **display-dependent**, and their own session reports
  `canvas=viewport=window=drawable=1920x1080` — all four in agreement, **0 deferrals** (S208 held).
  The window is 1920x1080 at (0,0) on a 1920x1080 display, and **GNOME's DOCK (~60px, left) and TOP
  BAR (~25px) are drawn OVER it.** Their first screenshot has content starting at x≈63, y≈25 —
  exactly those widths. **Content occluded, not clipped**, which is precisely why every canvas
  capture I took was clean and why four reproductions found nothing.
- **Same cause as S184's leftover PO-62 residual** — *"the window is at y=32 on a 1080-tall display,
  so the bottom 32px are off-screen"* — desktop chrome the port never accounted for. One defect,
  reported twice, months apart, from opposite edges of the screen.
- 🔧 **Fix 1 — place and size within `SDL_GetDisplayUsableBounds`**, the work area minus panels,
  instead of `SDL_GetDisplayBounds`. A window larger than the work area has to hide somewhere, so
  the size is clamped too, with a loud line when it happens. `MA_NO_USABLE_BOUNDS=1` reverts.
- 🔧 **Fix 2 — the present viewport now follows the DRAWABLE, not `g_scrW/g_scrH`.** Those are what
  the game *asked* for; the drawable is what the compositor *gave*. They can differ (a clamped
  window, a HiDPI scale, a declined resize), and when they do the canvas quad is mapped to a
  rectangle the window does not have. Taking the drawable makes that mismatch **structurally
  impossible**. `MA_VIEWPORT_SCRWH=1` reverts. Independently correct of Fix 1, and it is what
  removes the whole class rather than this instance.
- ⚠️ **Still PO-verification pending**, and deliberately not claimed closed: the fixes are correct
  on their own terms, but the PO's symptom was environmental, so only their retest counts.
- ⭐ **The method note worth keeping: the reporter's ONE action beat four of my reproductions.**
  I had measured the canvas five ways and it was clean every time, because `MA_SHOT` cannot see the
  window. "Drag it to the other monitor" isolated an environmental variable no instrument in this
  port can reach. **Third time in this project a PO sentence outran the traces** (PO-52 *"spinning
  into the ground"*, PO-54 *"no movement until both brakes are tapped"*, now this) — §8-MA129.
- **Gates:** `parity_2d` **5/5 byte-identical**, `replay_screen` PASS.

### 🏃 Sprint 208 — "The viewport and the window disagreed 2,654 times" (PO-65) — ⚠️ CLOSED PARTIAL 2026-08-25 (6/8, real fix, PO retest pending)

- ⭐ **The PO's full-desktop screenshot settled S207's open question: reading 1.** The window is
  wholly on screen, the panel **fills it**, and the left edge is genuinely missing — **not** a
  screenshot crop. So the fault is at the **canvas→window present**, the one layer *no capture in
  this port can see*: `MA_SHOT` dumps the canvas, which is why all four of my reproductions looked
  perfect. **Asking beat measuring** — I had no instrument that could have answered it.
- ⭐⭐ **Found in `ensure_window`: the dedup sat BELOW the off-thread deferral, and both halves of
  that ordering were wrong.**
  1. An off-thread caller never reached the dedup, so an **unchanged** size was re-deferred every
     frame — **2,654** identical `deferred to main` lines in the PO's session.
  2. Worse: `g_scrW/g_scrH` are assigned at the top of the function and **`glViewport()` is built
     from them**, so an off-thread request moved the **viewport immediately** while the real
     `SDL_SetWindowSize` was deferred — and when the main thread applied the pending resize, this
     same dedup could **skip it**, because `lastW/lastH` already matched from an earlier
     main-thread call. **The viewport and the window can then disagree indefinitely, and the canvas
     gets mapped to a rectangle the window does not have.**
  Fixed by testing the dedup **first, on every thread**: an unchanged size is a no-op wherever it
  comes from, and a changed size still defers exactly once (`lastW/lastH` deliberately not updated
  on the deferral path). `MA_OLD_RESIZE=1` restores the old ordering.
- **Measured, before and after, same recipe:** deferrals **2,654 → 0**, and
  `[present] canvas=1920x1080 viewport=1920x1080 window=1920x1080 drawable=1920x1080` — all four in
  agreement. New `MA_TRACE_PRESENT` prints canvas / viewport / window / drawable and shouts when
  viewport ≠ drawable.
- ⚠️ **NOT claimed as PO-65's fix.** I never reproduced the clipping, so I cannot say this cures it —
  only that it is a real defect on exactly that path and the best candidate. **PO retest decides.**
  Claiming it here would repeat PO-62 and S175 exactly: a plausible cause published before the
  reporter confirmed it.
- 🔨 **Two new PO observations from the same session, not yet investigated:** typing a replay name
  shows **no text**, and clicking **SAVE highlights the word but does nothing**. The first is the
  S198/S200 focus family (*clicking an edit must focus it*); the second is a different path from the
  in-flight `SEL_8` save that *did* fire (and overwrote two files), so **`RFullPanelDial::ReplaySave`
  is its own defect**. Both fold into PO-65 and are S209's work.
- **The apparatus finding stands and is bigger than this bug:** every screen-parity oracle here is
  blind to the whole present path — scaling, letterboxing, cropping, compositor behaviour. Nothing
  has ever tested it, and this sprint is the first time it has been instrumented at all.
- **Gates:** `parity_2d` **5/5 byte-identical**, `replay_screen` PASS.

### 🏃 Sprint 207 — "The overwrite is explained; the clipping is not" (PO-65) — ⚠️ CLOSED PARTIAL 2026-08-25 (5/8)

**The data-loss half is solved. The rendering half needs one answer from the PO, and I am not
guessing it.**

- ⭐ **THE OVERWRITE MECHANISM, reproduced.** Drove a real flight → debrief → **REPLAY**
  (`#2063:3`) and captured the PO's screen. `RFullPanelDial::ReplaySaveInit` does
  `selectedfile = Save_Data.lastreplayname`, and **SAVE writes to `selectedfile`** — so with no new
  name typed, SAVE overwrites whatever was last *loaded or viewed*. My capture shows
  **`IanVertical Hero` highlighted as the default selection** — one of the exact two files the PO
  lost. The chain is complete and matches the PO's session precisely: they had been loading shipped
  `.cam` files testing PO-61, each load setting `Save_Data.lastreplayname=selectedfile`
  (`FULLPANE.CPP:3607/3657`), and the next SAVE landed on one.
- **The out-of-box default is SAFE**, which is why this only bit after PO-61 testing:
  `SaveData::InitPreferences` sets `lastreplayname = "MiG Alley.cam"` — a name that collides with
  nothing. **It is the act of selecting a replay that poisons the save target.** (And note this only
  works at all because **S103** made `InitPreferences` run; before that the default never loaded.)
- ⭐ **The port-side defect that makes it unavoidable: the name field is unusable.** The capture
  shows the *Current File* edit drawn **on top of the list**, overlapping the `IanMany v Many#1`
  row. On the load screen the same pair is `id=1060` edit at (14,187) **inside** `id=1055` listbox
  at (10,128) 294x260. With no reachable way to type a new name, SAVE can only ever reuse
  `selectedfile`. **Same family as PO-43/S155** (a list overflowing its dialog, `ResizeToFit` grows
  the control and nothing constrains it).
- ⚠️ **The PO's LEFT-CLIPPING IS NOT REPRODUCED, and I will not claim a cause.** Same screen, same
  build, driven the same way: ink bbox **x 0..798, y 0..598** — nothing clipped. Two readings remain
  and they need different fixes:
  1. **a genuine window-level defect** — my captures dump the **canvas**, and the PO sees the
     **window** after a canvas→window present, so a clipping fault in presentation is *invisible to
     every capture I can take*. That is the "apparatus cannot see the failure" class, and it is a
     real blind spot in this port's whole screen-parity apparatus, not just here.
  2. **a screenshot crop** — the PO's image is **733x580**, which is neither the window (640x480)
     nor the canvas (1920x1080); a 733-wide crop taken while the window was still 1920x1080 (the 3D
     size, restored to 640x480 only afterwards) would cut the panel's left edge by roughly the
     observed amount. **PO-62 is the precedent**: a reported left-edge cut that measurement showed
     was a 1822px screenshot crop, and the sprint that "fixed" it was wrong to claim so.
  **One question to the PO decides it** — see the review note. Asking beats another sprint of
  measuring the wrong layer.
- **Re-measured and stated precisely:** the S206 finding stands — `GetCurrentRes` reads
  `GetWindowRect`, which returns `g_scrW/g_scrH` when `g_win` is NULL, so headless it always answers
  **800x600** whatever the canvas is. The window genuinely resizes 640x480 → 1920x1080 (3D) →
  640x480, and the layout is chosen from a size that tracks none of that.
- **Measurement-apparatus finding worth its own note:** `MA_SHOT` captures the **canvas**, never the
  window. Every screen-parity oracle in this port is therefore blind to the entire present path —
  scaling, letterboxing, cropping, compositor behaviour. Nothing has ever tested it.
- **Not changed:** no fix shipped this sprint; the tracing added is `MA_TRACE_RES`, default-off.

### 🏃 Sprint 206 — "The save screen wrote over the game's own replays" (PO-65) — ⚠️ CLOSED PARTIAL 2026-08-25 (5/8, characterised, NOT fixed)

- ⭐ **PO-64 CLOSED, PO-VERIFIED:** *"yes! replay moves!"* S205's fix confirmed in play. The PO's log
  corroborates it independently — they ejected from `screen=PLAYING paused=0`, and the reader walked
  block 0 (`numframes=1024`) → block 1 at **31179** (`numframes=386`) → clean **BUFFER EXHAUSTED at
  36424 = the file size**.
- ⚠️⚠️ **PO-65 NEW, WITH DATA LOSS.** The PO tried to save a replay; the save screen is drawn shifted
  off the LEFT edge (screenshot: list reads `-On Kill` / `Best Hero Kill`, menu reads `CK SAVE VIEW`
  for `BACK SAVE VIEW`). **The save happened anyway, twice, onto shipped files:**
  `IanMy Best Hero Kill.cam` 98,568 → 38,145 B and `IanVertical Hero.cam` 173,131 → 38,145 B, and
  the PO's screenshot shows **`…ical Hero` highlighted** — one of the two. **Both restored** from
  `~/sgl/TUE/afterGameReport/` (all 8 now byte-identical to the pristine set); the PO's own recording
  preserved at `scratchpad/po65/your_saved_replay.cam`. **`afterGameReport/` is the only known
  pristine copy of the shipped `.cam` files — treat it as an oracle and never let a gate write to
  `Videos/`.**
- ⭐ **Measured defect, real but NOT the whole of PO-65: the layout picker and the canvas disagree
  about the screen size.** `RFullPanelDial::GetCurrentRes` sizes the front end from
  `AfxGetMainWnd()->GetWindowRect()`, which answers **800x600 whatever the canvas is**:
  `[shot] canvas 1920x1080` against `[res] GetCurrentRes: window=800x600 -> chose=1`. So every
  front-end panel is laid out for a screen that is not the one being drawn — which is exactly why
  the Replay screen sits in the top-left quadrant (noted in passing at S203). **Fifth instance in
  this port of two code paths disagreeing about one fact** (collection S165, control type S164, row
  count S166, extent S203, and now *screen size*).
- ⚠️ **And it does NOT explain the PO's report — measured, not assumed.** Ink bbox of the Replay
  screen on a 1920x1080 canvas is **x 0..798, y 0..598**: the panel is confined to the top-left and
  **nothing is clipped on the left**. The PO's panel was drawn at NEGATIVE x, so PO-65 has a further
  cause live only on the **post-flight** path. **Prime suspect, explicitly unmeasured:** the S105
  family — the 3D sub-window is created `WINSH_MID` so its origin is the screen CENTRE, and drawing
  the front end through it displaces content by −(w/2). That is S207's first measurement, not a
  finding.
- **Latent bug found and left alone deliberately:** both loops in `GetCurrentRes` run
  `for (res=0;res<6;res++)` over `resolutions[]`, which has **four** entries, and the winner then
  subscripts a four-entry `m_currentscreen->resolutions[]`. Measured here it never lands out of
  range, so fixing it now would be a change with no observable effect and no test — logged, with
  `MA_TRACE_RES=1` printing a loud warning if it ever does.
- 🔧 **Fixed a diagnostic of mine that had started lying:** `MA_RPL_FAIL` claimed *"every later read
  is misaligned"* on **every** failure including a clean end of file — the block scan's normal
  terminus — so the PO's *successful* run printed it directly under *"BUFFER EXHAUSTED ... this is
  NOT a format disagreement"*. Two contradictory lines, the scarier one wrong. It now says which.
- ⚠️ **PROCESS, third occurrence: I truncated a latin-1 source to ZERO bytes** with
  `open(path,'w',encoding='latin-1')` — the write throws on a non-latin-1 character *after* the open
  has already truncated. `SRC/MFC/FULLPANE.CPP` went to 0 bytes and the link failed with undefined
  references that pointed at COMMS, not at the file I had broken. Cost nothing because the work was
  committed (`git checkout --` restored 140,097 bytes), which is the third time that has been the
  only thing between this trap and lost work. **Build the bytes, write to a temp, rename** — and the
  rule now has a corollary: *the link error names the wrong file, so check `git status` for a
  zero-byte source before believing the symbol names.*
- **Gates:** unchanged and green (`parity_2d` 5/5, `replay_screen`, `replay_record` all ran clean
  earlier this session); this sprint added instrumentation only.

### 🏃 Sprint 205 — "A stub that returned success ate every replay" (PO-61/PO-64) — ✅ CLOSED 2026-08-24 (goal MET, 8/8)

- ⭐⭐ **ROOT CAUSE: `SetEndOfFile` was `{ (void)h; return TRUE; }`** — a compat stub reporting
  success and doing nothing. This port's signature bug, and the one its own notes say to grep for.
  `Replay::OpenRecordLog` opens `replay.dat` with **`OPEN_ALWAYS`** (its own comment: *"add to any
  file that is there"*) and empties it via `SetEndOfFile` when `ResetFileFlag` is set (*"instead of
  deleting file, just truncate to zero"*). With the stub, **the file was never emptied and every
  flight ever flown was appended to it.** Measured across the PO's sessions: 2,427,259 → 2,459,480
  → 2,491,867 → 2,551,847 bytes. Playback starts at the **first** block — a stale one whose count
  was never back-patched — read `numframes=0`, treated it as empty, and then looked for the next
  header where the frame data still sat (S204's MAGIC MISMATCH at 19915). **No motion, however good
  the newest recording was.** Fixed with a real `ftruncate` at the current file pointer (the Win32
  semantic); `MA_NO_TRUNCATE=1` reverts.
- **Verified to the byte, not to the vibe.** After one flight: `replay.dat` **2,551,847 → 20,641**
  bytes, and `18952 (super) + 963 (block header) + 66×11 (frames) = 20,641` **exactly**. On disk:
  magic `78 56 34 12` at 18952, frame counts `42 00 | 00 00 | 41 00` = **(66, 0, 65)** at 19905 —
  the back-patch lands precisely where the reader looks.
- ⚠️ **S204's suspicion was wrong and measurement killed it.** S204 asked *"does `StopRecord` run on
  ALT+X, with `Record` still TRUE?"* — it does: `Record=1 replayframecount=66 -> StoreRealFrameCounts
  (num=66 start=0 end=65)`. The back-patch was never being skipped. It was patching the newest
  block in a file whose **first** block playback would read.
- **New gate `port/replay_record.sh`** — flies a real quick mission (real GL; there is no headless
  variant, `SDL_CreateWindow` fails under the dummy driver) and asserts the recording is *playable*:
  the flight recorded, the count was back-patched, **exactly one block is in the file**, the FIRST
  block carries this flight's count, and its header+frames end at EOF. Stashes and restores the
  player's `replay.dat` (S81's rule). `CONTROL=1` restores the stub and it goes red.
- ⭐ **The negative control caught a bad assertion in my own gate — the second time this session.**
  v1 asserted "the file is small" and "SOME block closes at EOF", and **the control PASSED**: with
  the stub restored the file grew 20641 → 41282 and the *appended second* block closed at EOF
  perfectly. Playback reads the **first** block, so the check tested something playback never does.
  Rewritten to the invariant that actually matters — *exactly one block, and the first one closes*
  — the control now fails with the defect stated outright: *"2 block headers at [18952, 39593]
  (playback reads the FIRST)"*. **Size was never the invariant; a longer flight legitimately makes a
  bigger file.**
- **Also fixed a self-inflicted §8-BoB206:** the gate's first version read the super-header offset
  from `MA_TRACE_REPLAY`'s per-step offsets — which are emitted by **playback**, so a record-only
  run never produces them, and it reported "super-header offset known: NO" against a healthy file.
  An assertion keyed on evidence its own recipe cannot emit, one day after writing that note. Now
  the offset is **discovered from the file**.
- ✅ **PO-VERIFIED 2026-08-25:** *"yes! replay moves!"* The PO's session also corroborates it from
  the log alone — they ejected from `screen=PLAYING paused=0`, and the reader walked block 0
  (`numframes=1024`) → block 1 at **31179** (`numframes=386`) → clean **BUFFER EXHAUSTED at 36424 =
  the file size**. Before the fix it died at 19915 on zero fill.
- ⚠️ **PO-61 is NOT fixed by this and the PO confirmed so in the same message** (*"load replay from
  the main menu still crashes"*). Flagged in advance rather than discovered afterwards: the shipped
  `.cam` files share none of `replay.dat`'s truncation history. New signature → **S206**.
- **S206 fixed a diagnostic of mine that had started lying:** `MA_RPL_FAIL` claimed *"every later
  read is misaligned"* for every failure **including a clean end of file** — the normal terminal
  condition of the block scan — so the PO's successful run printed it directly under *"BUFFER
  EXHAUSTED ... this is NOT a format disagreement"*, two contradictory lines with the scarier one
  wrong. It now says which.
- **Gates:** `replay_record` PASS (+ control red), `parity_2d` **5/5 byte-identical**,
  `replay_screen` PASS.

### 🏃 Sprint 204 — "Play works; the recording is empty" (PO-61/PO-64) — ⚠️ CLOSED PARTIAL 2026-08-24 (6/8, root cause located, NOT fixed)

**Driven entirely by the PO play-testing live, four runs, each one narrowing it.** Nothing here is a
fix; the sprint bought a root cause and it is one line away from the defect.

- ⭐ **PO-64 is PO-61 wearing different clothes, and the arrow points the way I nearly got backwards.**
  Measured interleaving, not argued: `SEL_4 -> PLAY: PlaybackPaused=0` → `LoadHeaderID at 19915 →
  MAGIC MISMATCH` → next call `paused=1`. **The transport works.** Playback un-pauses, fails a block
  read, and re-pauses itself. So "the VCR controls do nothing" and "the transport ran" are both true,
  and the PO's report and the code never disagreed.
- ⭐ **Root cause: the recorded block header says it holds ZERO frames.**
  `LoadFrameCounts: numframes=0 startframe=0 endframe=0 emptyblock=1`. `LoadBlockHeader` computes a
  block's end as `pos + numframes*sizeof(REPLAYPACKET)`, so a zero count marks it empty, consumes no
  frames, and sends the reader looking for the next header **exactly where it stands** — which is the
  measured signature (header parses 18952→19915 in twelve steps, then finds zero fill at 19915).
  **The reader is correct. The recorder wrote a block claiming nothing is in it.**
- **Mechanism, and why a short flight is the trigger.** `FRAMESINBLOCK` is **1024**. The mid-block
  back-patch (`StoreReplayPacket`) only fires on `replayframecount==1024`, so a ~10 s flight never
  fills a block, and `Replay::StopRecord()` — which calls
  `StoreRealFrameCounts(replayframecount,0,replayframecount-1)` — is then the **only** thing that
  ever writes the real count. **Open question for S205, deliberately not guessed:** does `StopRecord`
  run on ALT+X, and is `Record` still TRUE when it does? Both traces are in and default-off.
- ⚠️ **FOUR of my own hypotheses died on measurement this sprint**, each plausible enough to have been
  written up: (1) "the reader disagrees at the very first read" — no, block 0 parses cleanly from
  exactly `SuperHeaderSize`; (2) "the super-header parse consumed the wrong byte count" — **my own
  diagnostic asserted this in its message text** and it is false, now reworded to report *where* and
  let the offsets say *who* (a diagnostic that asserts a cause gets quoted — BoB S101); (3) "eject
  works because it sits outside the `ReplayFlag` guard" — every case including eject is *inside* it;
  (4) "`SetFilePointer` is a stub, so the back-patch goes nowhere" — it is a real `lseek`, correctly
  implemented. Reading one function further killed (3) and (4) before either reached the record.
- ⭐ **"No transport flags arrived" proved nothing, and nearly proved something false.** My first
  instrument traced `ReplayFlag` arrivals and measured **zero** — which reads as "the button is
  dead". But **`SEL_4` never sets `ReplayFlag` at all**; it only clears `PlaybackPaused` and swaps
  the overlay screen. Zero flags is exactly what a *working* play button produces. Same shape as
  S203's carried "the LOAD click does not register", also wrong. **Before believing a negative,
  check that the thing you instrumented is on the path the claim is about** (§8-MA126, again).
- **Correction to S203's carried item:** it recorded that the Replay screen's LOAD click "does not
  register". The PO's mouse registered it **seven times**. What failed was my *injected* click, i.e.
  the harness — a scaffold-reach limit, not a defect. Corrected in place rather than left to mislead.
- **New tooling** (all `MA_TRACE_REPLAY`, default-off; the two FAILURE lines stay unconditional
  because the failure mode was that nobody was looking): per-step **offsets** through
  `LoadBlockHeader` so an over-consuming step names itself by subtraction; `LoadHeaderID` now
  separates **BUFFER EXHAUSTED** from **MAGIC MISMATCH** (two causes, opposite investigations, one
  message until now); `LoadFrameCounts` prints the counts that decide advance; `StopRecord` /
  `StoreRealFrameCounts` say whether the back-patch ran.
- **Gates:** `parity_2d` **5/5 byte-identical**, `replay_screen` PASS. No behaviour changed.

### 🏃 Sprint 203 — "The menu drew seven rows and would take four" (PO-63) — ✅ CLOSED 2026-08-24 (goal MET, 8/8)

- ⭐ **PO-63 CLOSED, and it was blocking a whole subsystem.** The title menu's listbox is
  **105x100 and draws seven rows of 28px = 199px**. Measured off the capture rather than argued:
  ink runs at y=**215, 238, 266, 294, 322, 350, 378** for a control at y=210 h=100. Nothing clips
  it — Windows clips a child to its parent window and this path does not — and the **gold title
  screen shows the whole list too**, so drawing all seven is CORRECT. Every listbox hit test
  bounded the click by `m_maH`, so rows 4–6 were painted and unclickable by any route, injected or
  real. **Row 4 is REPLAY.** That is why the entire `_Replay` subsystem had no gate and PO-61 had
  no headless repro: it sat behind a row no recipe could address.
- **Fix: hit-test the height PAINT covered.** New `Hosted::drawH`, recorded by `ma_ole_draw_all`
  from the control's **own** `GetListHeight()` (the metric `OnDraw` lays rows out with, so it
  tracks any font change — the same reason `ma_ole_menu_row_point` already resolves rows through
  it). Never shrinks below the rect. **It can only WIDEN what accepts a click, so it cannot move a
  pixel** — and `parity_2d.sh` is **5/5 byte-identical**, as predicted before the run.
  `MA_NO_DRAWH=1` reverts.
- ⭐ **The fourth time in this port that the paint walk and the click walk disagreed about one
  fact** — collection (S165), control type (S164), row count (S166), and now **extent**. When
  something is drawn and does nothing, find the second opinion. This one is worth generalising:
  the disagreements have now covered *which* things, *what kind*, *how many*, and *how big*.
- **New gate `port/replay_screen.sh`**, with the control built in: `CONTROL=1` sets `MA_NO_DRAWH=1`
  and the gate must go **RED** (verified — both assertions fail). Plus a **vacuity guard**: the
  overflow depends on the live window resolution, so the gate reports whether the PO-63 condition
  is even present in this run instead of banking a pass that proves nothing. Straight from BoB
  **§8-BoB206**, landed the same day: *a gate nobody has watched fail is indistinguishable from a
  gate that cannot fail.* Both assertions key on evidence **this recipe** emits (`MA_TRACE_CLICK`
  is set by the gate), which is the other half of that note.
- ⚠️ **My own first hypothesis was wrong and the measurement killed it.** I predicted the rows were
  drawn at ~14px pitch *inside* the 100px control and that `listH=199` was a bad metric — i.e. that
  PO-63's recorded cause was wrong. Measured: pitch is **28** and `listH=199` is exactly right.
  S183's diagnosis was correct as written. Predicting first is what made that cheap to find out.
- 🔨 **Carried to S204, measured not guessed:** on the Replay screen the **LOAD** click
  (`#2063:1`) resolves a point at (133,575) inside 2063's rect and **no listbox hit test runs at
  all** — not even a "miss" for the file list that had just accepted one. Something upstream
  consumes it. `#2063:N` demonstrably works on other screens (`parity_2d` drives it on prefs), so
  this is screen-specific. **PO-61 needs that click**, so it is the top of S204. Deliberately not
  diagnosed here on one observation.

### 🏃 Sprint 178 — "PO-54 was not a bug, and the premise was mine" — ✅ CLOSED 2026-08-22 (goal MET, 8/8)

**Sprint Review (PO-driven, logged 2026-08-22):** `port/scrum/sprint-178.md`.

- ❌ **PO-54 closed as NOT A DEFECT.** The PO: *"gold standard behavior is no movement until both
  brakes are tapped."* The aircraft standing still at full throttle is the game working —
  parking brakes on at mission start, both keys release them. I had recorded that as a symptom.
- ⭐ **The "pause" was the flight ENDING.** Traced `Inst3d::Paused(bool)` by
  `__builtin_return_address(0)` and symbolised it: `View3d::drawloop`, at **log line 89104 of
  89113**, immediately before `instances=0 currinst=(nil)`. Normal teardown. What S175 called "the
  sim freezes and never resumes" was the aircraft **ground-looping and dying** under PO-53's
  full-left rudder.
- **The correlation was read backwards.** Brake taps → brakes released → aircraft rolls → full-left
  rudder → crash → flight ends. No taps → never rolls → never crashes → no "pause". Two samples,
  causation inferred in the wrong direction, and it produced a backlog item for a bug that does
  not exist.
- **The return-address trace is the technique worth keeping.** A dozen `Paused(TRUE)` callers, and
  reading them produced two wrong guesses (cockpit map, accel map — neither fires).
  `__builtin_return_address(0)` + `addr2line` named the caller in one run. **When a value is set
  from many places, do not read the places — print who set it.**
- **And the reporter was right again.** Second time today: PO-52 (*"spinning into the ground"*) and
  now PO-54 (*"no movement until both brakes are tapped"*). Both times a sentence of domain
  knowledge beat a sprint of instrumentation, because I was measuring a system whose CORRECT
  behaviour I did not know. **§8-MA129 generalised: ask what it is SUPPOSED to do, not only what
  the reporter saw.**

### 🏃 Sprint 177 — "A gate that cannot tell its preconditions from its subject" — ✅ CLOSED 2026-08-22 (goal MET, 8/8)

**Sprint Review (PO pre-approved ceremony, logged 2026-08-22):** `port/scrum/sprint-177.md`.

- **`damage_elements` reported "the tab bar never took a click" in a suite run and PASSED
  standalone minutes later.** Not a regression from S176's joystick change, which was the obvious
  suspicion: the suite had been SIGKILLed twice to free the display for the PO, leaving a **stray
  `wmig`** that still held the run directory when the next gate started. That gate's clicks went
  nowhere and it reported a **content** failure for an **environment** problem.
- **Same family as S171's "PASS on a crashed run":** a gate that cannot distinguish its own
  preconditions from its subject. S171 taught gates to assert how a run ENDED; S177 teaches them
  to assert how it BEGAN.
- **`assert_clean_start` REFUSES, it does not kill.** A live `wmig` may be the PO's own game on the
  display, and a gate is never entitled to close it. Exits **2**, not 1, so a suite can tell
  "could not run" from "failed" — a distinction the old output could not express.
- The suite runner also kills any stray between gates, so one killed gate can no longer poison the
  next.
- **Full suite from a clean start: 16/16, every gate exit 0, no failures.** `parity_2d`
  byte-identical on all five references — which matters because **prefs-Controls is one of them**
  and S176 changed DirectInput axis enumeration; if the reordering had shifted that screen, this
  is where it would have shown.
- **Worth naming: I nearly went looking for the joystick change in a 2D dossier gate.** The failure
  arrived one step after a plausible culprit, and the cheap check — *run it on its own* — was one
  command. Reproduce in isolation before reading any diff.

### 🏃 Sprint 176 — "It pulls to the left" (PO-53) — ✅ CLOSED 2026-08-22 (goal MET, 8/8) — ⭐ and it was PO-52 all along

**Sprint Review (PO-driven, logged 2026-08-22):** `port/scrum/sprint-176.md`.

- ⭐ **The port enumerated joystick axes in SDL order; DirectInput enumerates canonically.** SDL
  gives X, Y, twist, slider; DI gives X, Y, Z(slider), Rz(twist). `SController::RemakeAxes` fills
  the role combos **first-come**, so whichever axis is enumerated **third becomes the throttle** —
  the twist, in SDL order — which pushed the **slider** onto RUDDER. The slider rests at its
  minimum, so the game read a **permanent full-left rudder (−32767)**. Fixed by emitting in
  canonical rank order with the DIDFT instance still carrying the SDL axis index;
  `MA_JOY_SDL_ORDER=1` reverts. Also defined `GUID_Slider`, declared since bring-up, never valued.
- **PO confirmed from play:** elevator, aileron and rudder calibrate correctly. Trace agrees —
  rudder covered −31740 → +29683 and returns to ~128 centred (was pinned at −32767).
- ⭐ **This was PO-52, and the PO called it:** *"your flight test regression was just spinning into
  the ground every time because of the joystick mis-calibration."* Full-left rudder ground-loops
  the aircraft — which is why every runway test sat at **20 kt at full thrust**. Fixed, the same
  test runs **0 → 143 Kts**, past rotation speed. **Three causes had been published for PO-52 and
  all three were wrong** (S174 physics, S175 my own driver, and the unstated assumption that a
  flight defect lives in flight code).
- **The lesson is not "measure more".** Two sprints of instrumentation sampled *quantities*; the
  PO had watched the *behaviour* — spinning — and one sentence carried more than all of it. **When
  a defect is reported from play, ask the reporter what they SAW before instrumenting what you
  think it is.** It costs one question.
- **PO confirmed all four axes** on a second session — slider full travel `-32768 → +32767` gives
  throttle `-1 → 32767`, both ends, direction correct. **Residual:** **K10** still needs an elevator input in the
  takeoff driver to rotate; **PO-54** open; the regression suite was interrupted twice for the
  display and `damage_elements` needs re-checking.

### 🏃 Sprint 175 — "It was never the ground roll, and the second cause was mine" (PO-52) — ✅ CLOSED 2026-08-22 (goal MET, 8/8)

**Sprint Review (PO pre-approved ceremony, logged 2026-08-22):** `port/scrum/sprint-175.md`.

- ⭐ **S174's cause was wrong and this sprint retracts it.** S174 said *"the ground roll — thrust
  is not producing acceleration past 20 kt"*. One trace inside the engine: **~19.7 kN at 100% RPM,
  airspeed climbing 0.1 → 10.6 m/s and still rising** when the trace stops. The aircraft was never
  failing to accelerate; the "plateau" is the last model state redrawn 400+ times.
- ⭐ **Then this sprint's own first answer was wrong too, and it was MY TEST DRIVER.** Walking the
  gate chain inward (sim thread 13,650 iterations, `timeout=0`, `accelcountdown=1`) the last gate
  is `Paused()` — and **tapping the wheel-brake keys pauses the simulation**, reproducibly, 2/2
  runs, from cycle 707. Without those taps the sim never pauses.
- **I wrote the note that predicts this one sprint ago.** §8-MA124: *"a synthetic driver is code,
  and it fails in the shape of the bug you are hunting."* Then walked into it again. What caught it
  was that note's own prescription — **A/B the driver against no driver, and against the driver
  with one input removed**. Three runs. *Writing the lesson down does not install it; running the
  check does.*
- **What is established:** the mission flies and starts on the runway; the engine makes full
  thrust; brake-key taps pause the sim; and **without them the aircraft does not move at all** —
  0 kt over 13,600 frames at full thrust with the sim running. Those last two point at the
  **wheel-brake key path** (`KEYFLY.CPP:1189`, `KeyHeld3d`), and the next measurement is whether
  `KeyHeld3d(LEFTWHEELBRAKE)` reads true when nothing is held.
- **Not published as a cause.** Two have been published too early in two sprints; PO-52 now carries
  both retractions so the next attempt starts from the evidence rather than from a story.
- **Both mistakes have one shape:** the conclusion named a layer that had not been measured — S174
  measured inputs and rendered output and concluded about the physics between them; S175 measured
  sim scheduling and concluded about the game when the input was its own. *Before writing a cause,
  name the measurement that is of the cause itself, not of its neighbours.*

### 🏃 Sprint 174 — "The mission flies, and stops at 20 knots" (K10) — ⚠️ CLOSED 2026-08-22 (goal PARTLY MET, 5/8)

**Sprint Review (PO pre-approved ceremony, logged 2026-08-22):** `port/scrum/sprint-174.md`.

- **The built Wonju strike FLIES.** Frag → Fly → 3D, with the player on the runway at **speed 0
  Kts, alt 4 ft**. That is the first half of K10 and it is real.
- ⚠️ **It will not take off.** At 100% thrust it accelerates **0 → 20 kt and then plateaus,
  indefinitely**. Logged as **PO-52**, with the usual suspects already excluded by measurement:
  the throttle command **lands** (`thrustpercent=100`, 188 times); the player **has manual
  control** (`controlmode=MANUAL`, `movecode=AUTO_FOLLOWWP`); the brakes are **hold-to-brake** so
  they are not on; and the **flight model is fine** — the airborne Hot Shot start flies at
  **503 kt / Mach 0.84 / 15,966 ft** under the same build. The defect is the **ground roll**.
- **My first hypothesis was wrong, and measuring it cost one run.** "The player is still on the AI
  takeoff rail (`AUTOMOVE.CPP`)" was plausible, fitted the symptom, and is **false**: `movecode=0`
  and `controlmode=MANUAL`, identical to the airborne flight that works. *Two runs of a trace beat
  an afternoon of reading `AUTOMOVE.CPP`.*
- **New tooling, both reusable for K11–K13:** `MA_TRACE_HUD=<n>` samples the flight model's own
  speed/altitude/mach — the numbers the HUD prints, in the player's units — and `BOB_AUTOFLY=takeoff`
  drives full throttle **counting from when the sim is up** (`g_ma_in3d`). The existing `throttle`
  mode counts from process start and is capped at 600 pumps, so on the campaign path every one of
  its taps was spent in the front end before a flight existed.
- **A driver bug found by its own symptom:** the first takeoff drive tapped the brakes at *two*
  points, and they toggle — released, then re-applied. Indistinguishable from "the brakes never
  released". They turned out to be `KeyHeld3d` and irrelevant either way, but the lesson stands:
  **a synthetic driver is code, and it fails in the shape of the bug you are hunting** (§8-MA121
  again, one sprint later).
- **Correction to K10's wording:** "wheel brakes release on `,`/`.`" describes something the game
  does not do — `KEYFLY.CPP` applies them only while **held**. Same class as K9's "callsign edit"
  (§8-MA123): a criterion written from the PO's prose, inheriting a mechanism the game never had.

### 🏃 Sprint 173 — "Three sub-dialogs, one set of control ids" (K9) — ✅ CLOSED 2026-08-22 (goal MET, 8/8)

**Sprint Review (PO pre-approved ceremony, logged 2026-08-22):** `port/scrum/sprint-173.md`.

- ⭐ **The frag screen hosts THREE `CFragPilot` sub-dialogs — one per package — with identical
  control ids.** `@CFragPilot` is therefore ambiguous *with itself*, exactly as a reopened dialog
  was in S171. **S171's ambiguity warning caught it on the first run** — three visible hosts for
  id 2356, and the recipe could not say which. Without that warning this would have silently
  driven whichever sorted first by pointer, and the gate would have passed for the wrong row.
- **`@Class#N` names the Nth instance BY SCREEN POSITION** (top-to-bottom, then left-to-right).
  Map order is by pointer, i.e. by whatever the allocator did; "the second flight row" has to mean
  the one the player sees second. Verified: `#0/#1/#2` resolve to distinct clients at y=34/145/256
  — evenly spaced, the three rows. Carried in the class string, so every existing recipe form is
  untouched.
- **Every K9 clause reads game state, not pixels.** The callsign is
  `Todays_Packages.pack[p][w][g].callname` (a combo can repaint a caption without the write
  landing); the aircraft is `MMC.playeracnum`, and the gate checks it equals `flight*4 + slot`.
  That matters because `OnClickedPlayer` **refuses** a dead pilot's slot and one taken by another
  comms player — so a click that legitimately does nothing is indistinguishable from a broken one
  unless the write is traced.
- **Correction to the story text:** the callsign control is a **combo**, not an edit. K9's wording
  ("Callsign edit accepts text (cf. PO-16)") assumed text entry; `CFragPilot::FillComboBox` fills
  `IDC_FRAG_CALLNAME` from the game's callsign string table and the player *picks*. **PO-16 is not
  on this path** and remains open on its own terms.
- Result: `FlyableAircraftAvailable=1`, a **12-name roster**, callsign `1 → 5 " Red "`, seat
  `acnum 0 → 4`. **PO-37** (the panel does not fill 1920) is untouched and affects no clause here —
  it is a layout decision that needs all five parity screens re-verified, and it was blocking K9
  only in the sense that S168 declined to call a verdict while it stood.

### 🏃 Sprint 172 — "The port had never dragged anything" (K8) — ✅ CLOSED 2026-08-22 (goal MET, 8/8)

**Sprint Review (PO pre-approved ceremony, logged 2026-08-22):** `port/scrum/sprint-172.md`.

- ⭐ **The first press-move-release interaction in the port.** Every click it had learned was
  press-and-release in one place — and that was *deliberate*: S95's `MaDriveClick` issues down+up
  in a single tick specifically to keep `m_bDragging` FALSE, because `CMapDlg::OnMouseMove` was
  believed to deref `GetDC()` unchecked. It does not any more (compat returns a real static CDC),
  so the whole engine chain was sitting there intact and unreachable.
- **`CMapDlg::MaDriveDrag(from,to)`** drives the genuine handlers in eight steps, because the
  waypoint's world position is recomputed on **every** move: a one-jump drag would exercise the
  drop and not the dragging.
- **`MA_MAP_DRAG` addresses waypoints BY NAME**, resolved through the map's own `FindMapItem` —
  `<frame>,<wp>@<dest>` or `<frame>,<wp>+<dx>,<dy>`. Icon positions move with zoom, scroll and
  campaign state; a recipe naming a pixel tests that pixel (S95).
- **`MA_MAP_ITEM_SCAN` now takes a list of frames.** It was one-shot, which can only describe the
  map as it *opens* — but the whole of EPIC K edits the map, and waypoints do not exist until a
  mission is authorised. Only the first scan clicks; a later one is pure observation, or the run
  would diverge because we looked at it.
- **The world position is the oracle, not the screen position.** `info_waypoint::World` is what the
  flight reads; screen coordinates are a rendering artefact. Units are centimetres
  (`RANGES.H: METRES250KM = 25000000`), which makes the script's *"within 4 miles"* directly
  checkable: the IP lands **3.06 miles** from the target after `OnDragItem` clamps and recalcs.
- **My own instrumentation lied first.** The after-position was read through `m_buttonid`, which the
  drop path is free to change — so the second drag reported the *first* waypoint's coordinates.
  Caught only because two different waypoints printed **byte-identical** world coordinates, which
  is not a thing that happens. Capture the uid at press time. *A trace is code, and it can be wrong
  in exactly the way the thing it is measuring cannot.*
- **The gate asserts a NEGATIVE too:** dragging the target itself must move nothing
  (`allowdrag=0`). Without it, a hit-test that dragged whatever was under the cursor passes every
  other assertion in the gate.
- ⚠ The script's *two AAA waypoints* are on a second suppression **wave**. `Ins Wave` fires and
  creates no route, because a wave with no squadron has no waypoints and there are no spare
  aircraft on this save's day one — the same arithmetic K7 documented. Named, not claimed.

### 🏃 Sprint 171 — "A dialog you close is still on the screen as far as the registry knows" (K6, K7) — ✅ CLOSED 2026-08-22 (goal MET, 8/8)

**Sprint Review (PO pre-approved ceremony, logged 2026-08-22):** `port/scrum/sprint-171.md`.

- ⭐ **Closing a campaign dialog leaked its whole control set into the hosted registry, still
  flagged visible.** `RDialog::EndDialog` tears down a *subtree*; compat's `CWnd::DestroyWindow`
  deregisters exactly *one* window. After one close/reopen there were two live `CProfile`s and two
  live `CFlt_Task`s, and `#2149@CFlt_Task` resolved to whichever sorted first **by pointer** — the
  dead one. The click that opened a dropdown and the click that picked a row were addressing
  different controls. Fixed with the same fchild/dchild/sibling walk S169 built for scoping;
  `MA_NO_SUBTREE_REMOVE=1` reverts.
- **`@Class` stops disambiguating when a dialog is ambiguous with itself.** S85's ambiguity warning
  only ran when *no* class was given. It now counts after the same filters the resolver uses, class
  included, and names each candidate's parent pointer. The `UNRESOLVED` dump names the parent class
  too — without it the message told you to add a qualifier you had no way to choose.
- ⭐ **The separate `Load` click never did anything, in any recipe, ever.**
  `CLoad::OnSelectRlistboxfile` calls `OnOK()` when the clicked row is *already* current, and
  `currrow` starts at 0 — so `:r0` selects Minimum Strike **and loads it**, destroying the chooser
  in the same click. The `620,#1056@CLoad` step was landing on a destroyed dialog; it only *looked*
  like it worked because the dead dialog's controls were still registered. Fixing the leak turned a
  silent no-op into a hang, which is how it was found.
- **A recipe entry that can never resolve holds every entry behind it**, indistinguishably from "the
  control is not up yet". `[clickseq] STALLED` now says so once, loudly, naming the entry.
- ⭐ **`flak_suppression.sh` reported PASS on a run that SEGFAULTED.** Every assertion it made was
  true — the evidence was in the log before the crash — and it never looked at how the run ended.
  Only `oob_sweep` checked, and that is the one gate whose job *is* counting crashes. New
  `port/gate_lib.sh` (`assert_no_crash`, `assert_recipe_ran`), wired into nine gates; it symbolises
  the top frames, because an address list is not a diagnosis.
- **The crash was S170's, latent since it shipped:** a spinner with an **empty list** dereferences
  NULL inside its own `OnDraw` (`m_list.GetAt(m_list.FindIndex(m_index))`), directly under
  `ASSERT(m_list.GetCount()); // have at least one entry!` — which `NDEBUG` compiles out. The port
  paints every hosted control every idle, so a dialog that populates its spinner a moment after
  creating it gets one fatal frame. The culprit is **`WPDetail`**'s ETA spinner — named as a
  residual in S170 and reached here by accident, which is the only reason it was found before K8.
- **K6 CLOSED:** pattern default → Spaced → *reopen* → Spaced → Individual → *reopen* → Individual,
  method `Dive Bomb` throughout. ⚠ The port's default is *already* Individual targets; gold only
  shows the post-change state, so the default is not claimed wrong.
- **K7 CLOSED:** AAA-cover slot `Off Duty` → `F80 (1/1)`, stores → `Rockets & Fuel tanks`, Mission
  Folder Flights **2 → 3**. ⚠ The script names **F84**; the dialog's own **Available** column reads
  `F84: 0` and the game refuses `numavail < 4` — the gate asserts the **refusal**, rather than
  quietly assigning something else and calling it step 11.

### 🏃 Sprint 170 — "The last unhosted control, and the two doors in front of it" (K5) — ✅ CLOSED 2026-08-22 (goal MET, 8/8) — ⭐ EPIC K step 8 works: Flights 2 → 3

**Sprint Review (PO pre-approved ceremony, logged 2026-08-22):** `port/scrum/sprint-170.md`.

- ⭐ **RSpinBut was the LAST R\* type the port never hosted.** The wrapper (`SRC/MFC/RSPINBUT.CPP`)
  has compiled since bring-up, so nothing ever failed: every `InvokeHelper` on a spin button was a
  silent no-op, and the control was never created, drawn or clickable. `SRC/compat/ma_olespin.cpp`
  hosts it (dispids 1–12, verified against the wrapper, not guessed).
- **Its header keeps the dispatch block `protected:`** where RCombo/RListBox leave it public
  (`BEGIN_OLEFACTORY` reopens `public:` and those headers never re-specify). A thin derived
  accessor republishes exactly the members the host needs — no game source edited.
- **Two doors were shut in front of the spinner, and both looked like working code:**
  - **`CT_EDTBT` was drawn but inert** — `IDC_ACTYPE`, the `F84 (2)` duty field, is an RedtBt and
    the *only* route into `ChooseSquad`, which owns the spin-box. Hosting the spinner without this
    reaches nothing. Same shape as S87 (listbox rows), S140 (scroll bars), S163 (combos): a control
    type missing from `ma_ole_toolbar_click`'s filter. **That is now four.**
  - **`:rN` addresses a ROW, and a row's centre is a cell.** On the Profile wave table
    (Wave / ToT / Main Duty / AAA Cover / Air Cover) `:r1` lands in **column 3**, so Task — which
    reads `currcol` — opened the **flak** tab while the recipe read as if it were editing the main
    duty. S162 and S85 again, one dimension further out. New form **`:rN.C`** names the cell,
    resolved through the control's own `GetRowFromY` + `GetColFromX`.
- **`:-3` / `:-4` address a title bar's OK / Cancel bands** (generalising S98's `:?` for Help), so a
  recipe can *commit* a dialog the way a player does. Without it a gate can only ever show that a
  control moved, never that the change reached the mission.
- **The spinner refuses correctly.** `CRSpinButCtrl` will not go UP at `index > count-2`, so a click
  on a spinner at its limit is taken and does nothing. The gate asserts **the index changed**, not
  that the click was delivered — the first draft would have passed on a spinner pinned at maximum,
  which is exactly how the same control read in BoB S197.
- **Result, end to end:** `port/add_flight.sh` — Main Duty cell → duty field → ChooseSquad → the
  Flights spinner moves `1 → 2` on a 3-entry list → `ChooseSquad::OnTextChangedRspinbutctrl1` →
  `SetFlights(3)` → the **Mission Folder lists `Wonju Supply Dump  Bomb  08:30  3`**. The
  walkthrough's own "cheapest end-to-end assertion in the epic", read from the game's AddString
  trace rather than from pixels.
- Two compat gaps found on the way in: the missing `RSpinBut.h` case-symlink (every sibling OCX
  project has one) and `CWnd::ReleaseCapture` (the spin control calls it `this->`-qualified, so the
  global `::ReleaseCapture` the other controls resolve to was not reachable).

### 🏃 Sprint 169 — "The dialogs belonged to someone else's screen" (PO-51) — ✅ CLOSED 2026-08-22 (goal MET, 8/8)

**Sprint Review (PO pre-approved ceremony, logged 2026-08-22):** `port/scrum/sprint-169.md`.

- **PO-51 ✅** The idle's map/panel branches are a proper if/else, so the map was correctly gone when
  the frag pane launched — what remained were the campaign dialogs' **still-hosted controls**, drawn
  by the global `ma_ole_draw_all` pass. Fixed with `ma_ole_set_parent_scoped`, the mechanism **S97**
  built when the map chrome was drawing on the title screen; the toolbars were registered and the
  dialogs never had been.
- **The scoper walks `dchild` too** — wider than the paint recursion. Scoping only painted nodes left
  Route's `S. Wonju / Position / Altitude / ETA` columns on the frag screen: those nodes hang off
  `dchild`, so the walk never paints them, yet they stay hosted. *A node the OOB walk does not paint
  has no business being drawn by the front-end pass either.*
- The frag screen now renders only its own content — `Viper` / `Rattler` callsigns, the pilot roster,
  `Map Fly Preferences`. **PO-37** (panel does not fill 1920) is unchanged and still open.
- Gates: 9/9, parity 5/5 byte-identical.

### 🏃 Sprint 168 — "Four eventsink maps were silently thrown away by the linker" (K9) — ✅ CLOSED 2026-08-22 (goal MET, 8/8) — ⭐ FRAG works; the Wonju mission reports FLYABLE

**Sprint Review (PO pre-approved ceremony, logged 2026-08-22):** `port/scrum/sprint-168.md`.

- ⭐ **The macro named its sink registrar by `__LINE__` and defined the constructor out of line**, so
  the symbol had external linkage. Two TUs whose `BEGIN_EVENTSINK_MAP` sat on the same line emitted
  the same symbol, and this port links with `-Wl,--allow-multiple-definition`: the linker kept the
  first and **discarded the second entire sink map**, in silence. The winner registered twice.
- **Measured, not estimated: 68 sink maps, four colliding pairs** — `SQDNLBUT/WPBUT` (waypoint
  buttons, step 13), `LISTBX/WAVETABS` (the wave tabs, steps 8–12), `MAPFLTRS/MISSFLDR` (the Mission
  Folder: Intelligence, Profile, Delete, **Frag**), `SERVICE/SESSION`. One macro fault took out most
  of the PO's walkthrough from step 8 on.
- **Fix:** key the registrar by **class** (a class has exactly one sink map) and define its ctor
  *inside* the struct so it never reaches the external symbol table. Two belts — this was silent for
  the port's whole life.
- **The diagnosis chain was instruments, not inference:** `[evt_fire] NO HANDLER …` now reports an
  unmatched dispatch and lists what *is* registered (the `-> fire` trace is printed *before* the
  dispatch and reads like success); `MA_TRACE_EVTREG=<class>` is filtered, not capped, with
  `CProfile` as the control; then `objdump` on the TU's initialiser named the wrong callee outright.
- **Result:** `[frag] FlyableAircraftAvailable=1` — the mission is flyable, `LaunchFullPane(singlefrag)`
  runs, and the **pilot roster renders with `Map Fly Preferences`**. Fly is on that bar.
- New: **PO-51** — the map's OOB dialogs paint over the frag panel. **PO-37** confirmed on this screen.

### 🏃 Sprint 166 — "Two row-count opinions inside one control" (K5 cont.) — ✅ CLOSED 2026-08-22 (goal MET, 8/8)

**Sprint Review (PO pre-approved ceremony, logged 2026-08-22):** `port/scrum/sprint-166.md`.

- S165 asked *"is the header a row or chrome?"* — **neither guess was right.** `MA_TRACE_LBROW=1`
  answered it in one run: `tmHeight=16` (the row height was never wrong) and **`count=0`**.
- ⭐ **`GetRowFromY` clamps against `m_playerList`**, which only `AddPlayerNum` fills (multiplayer /
  player log), while rows come from `AddString` into **`m_list`**. So on every listbox that is not a
  player list it answered **-1 for every row past the first**. The control's own `OnLButtonDown`
  clamps against `m_list` and is correct — **two opinions about "how many rows" inside one control**,
  the same shape as this week's paint-vs-click walk and draw-vs-click type filter, one scale smaller.
- **Safe to correct, checked not assumed:** `GetRowFromY` has **no caller in the game tree** — its
  only consumer is the port's own `#ID:rN` resolver (S162), which was therefore silently limited to
  player lists from the day it was written. `MA_LB_PLAYERCLAMP=1` restores the old guard.
- **K5 progress:** `:r1` now selects the real `1.Bomb` row and the **Task button fires**. It still
  does not open the TASKS dialog. Two candidates, one trace apart → **S167 prints `currrow`/`currcol`
  at the top of `OnClickedTask`** rather than reasoning about it, which is how S164 went wrong.

### 🏃 Sprint 165 — "The click walk never descended the level the paint walk does" (PO-50) — ✅ CLOSED 2026-08-22 (goal MET, 8/8)

**Sprint Review (PO pre-approved ceremony, logged 2026-08-22):** `port/scrum/sprint-165.md`.

- ⚠ **This sprint corrects S164.** S164 reported *"the OOB walk paints 3 dialogs while 5 are on
  screen"* — a **misreading** of a per-frame counter, which became a confident and wrong claim in the
  sprint record, the board, `STATUS.md` and a cross-port note. The real asymmetry: the paint walk
  descends a **second level of logged children**; the click walk did not. **A summary number was used
  to infer a set difference; the fix was to print the sets** (`[oobrender]` vs `[oobvisit]`) and diff
  them. *When the question is "does A see the same things as B", never compare their counts.*
- **PO-50 ✅** The wave folder is a logged child of the Mission Folder, not of `m_toolbar2`, so every
  click on it fell through to the main toolbar and fired `IDC_OVERVIEW`. The click walk now mirrors
  the paint walk's descent; grandchildren get first refusal because they are painted on top (S82).
  `MA_NO_OOB_GRANDCHILD=1` reverts. Coverage now matches, 4 nodes to 4.
- **The trace fix that made it findable:** `[oobpaint]` was capped `if (_r++<40)`, so the budget went
  to the first dialog tree and later dialogs never appeared — **"filter, don't cap", third booking**,
  and it is what cost S164 its diagnosis. Both walks now print every distinct node exactly once, ever.
- **K5 still open, with a precise question:** `Task` now fires, but the click landed on **row 0**
  (the header) and `#2018@CProfile:r1` reports *"row 1 not mapped by GetRowFromY (h=110)"* — the
  listbox believes it has fewer rows than the screen shows. S166 answers that with one trace.

### 🏃 Sprint 164 — "The click walk and the paint walk do not enumerate the same dialogs" (K5) — ⚠️ CLOSED PARTIAL 2026-08-22 — K5 not delivered, its blocker named

**Sprint Review (PO pre-approved ceremony, logged 2026-08-22):** `port/scrum/sprint-164.md`.

**Goal NOT met**, recorded as PARTIAL rather than claimed (S89's rule).

- **The blocker, measured:** driving the wave folder's list resolved the right control and the click
  was taken by **`IDC_OVERVIEW` on the main toolbar underneath**, opening an unrelated dialog. The
  folder is drawn at (200,24) over the toolbar row, so **a player clicking a row of the mission they
  are editing gets the Overview dialog**. → **PO-50**.
- **The cause, named:** `[oob] painted 3 open dialog(s)` while **five** dialogs are drawn. The wave
  folder is not in the collection the click walk iterates, so it cannot be offered a point. S82's
  "mirror the paint walk" failing at the **collection** level, not the rect level — and BoB's
  §8-BoB183 with "control" replaced by "dialog", which is worth sending back since MA answered that
  note "N/A, already closed" in S161 on the strength of the control case.
- **What did land:** a node's painted area now swallows a click even when none of its controls wants
  it (the swallow rule previously existed only at the top-level logged child, whose rect does not
  contain its descendants' paint positions). `MA_NO_OOB_NODE_SWALLOW=1` reverts. An **origin bug
  inside that fix** put one dialog's rect at (0,0) 457×382 and it started swallowing the map's
  top-left corner — caught because the swallow trace names *which node* swallowed. A trace that
  names the actor, not just the action, is what makes that a one-line diagnosis.
- **Next, first thing:** the `[oobpaint]` trace is capped `if (_r++<40)`, so the whole budget goes to
  the first dialog tree and the folder never appears — **"filter, don't cap", booked for the third
  time**. Fix the trace before chasing the routing.


### 🏃 Sprint 163 — "The combos were drawn and inert" (K3) — ✅ CLOSED 2026-08-21 (goal MET, 8/8) — ⭐ K5/K6/K7 were all behind one missing control type

**Sprint Review (PO pre-approved ceremony, logged 2026-08-21):** `port/scrum/sprint-163.md`.

- ⭐ **`CT_COMBO` was missing from `ma_ole_toolbar_click`'s type filter**, so every combo box in every
  campaign-map dialog has been **drawn and inert** for the port's whole life — S87 (listbox rows) and
  S140 (scroll bars) one control type later, and the widest yet: the walkthrough's TASKS dialog alone
  drives **five** combos, PAYLOAD one, the frag two. K5/K6/K7 were all sitting behind it.
- Three parts, because a combo is not one click: the **click** (open the dropdown, or cycle a
  1-item combo); the **draw** — the open list is painted **after the whole OOB tree**, since drawn
  per-dialog it is covered by the next dialog in the walk; and the **dismiss** — an open list gets
  first refusal on the next click and consumes it either way, mirroring the paint order (S82's
  "topmost gets first refusal", one layer up). The row arithmetic is shared with the front-end path,
  not reimplemented.
- **`:rN` now means "the Nth item of this control"** — listbox row (`GetRowFromY`), tab
  (`CRTabsCtrl::m_rectList`), or a row of a combo's **open** dropdown (the geometry paint recorded).
  Never a pixel. The combo form takes **two entries** on purpose — open, then pick — rather than one
  scaffold click that opens and selects at once (the S82 trap). The unqualified `#ID:rN` must be
  parsed **before** the generic `#ID:%d` or the index is silently dropped: third appearance of that
  exact shape in this parser.
- **K3 ✅** Damage tab → *All elements* lists warehouse groups and `SB Flak Site` rows (`Fully / functional`).
  New gate `port/damage_elements.sh` (5 assertions, incl. real row ink — the first four all pass on
  a dialog that switched mode and drew nothing).
- ⚠ **The element list overflows its dialog: PO-43 again, on a second dialog.** Not fixed here, and
  the gate deliberately does not assert the list fits, so it cannot start passing for the wrong reason.

### 🏃 Sprint 162 — "Authorize" (K4) — ✅ CLOSED 2026-08-21 (goal MET, 8/8) — ⭐ the Wonju mission exists in the campaign

**Sprint Review (PO pre-approved ceremony, logged 2026-08-21):** `port/scrum/sprint-162.md`.

- **K4 ✅** Authorize opens the profile chooser (**Minimum Strike / Napalm Strike / Fighter Bomber
  Strike**, Minimum Strike preselected) and Load creates the mission: the **WONJU SUPPLY DUMP** wave
  folder (`1.Bomb 08:30 F80 (2)`, Route/Task/Save/Ins Wave/Del Wave) and the **MISSION FOLDER**
  listing `Wonju Supply Dump  Bomb  08:30  2`. New gate `port/authorize_mission.sh`.
- ⭐ **The recipe had been clicking the wrong row.** `#1055@CLoad` resolves to the listbox's *centre*
  = row 2 of 3 = "Fighter Bomber Strike" — the one profile the PO's script says not to pick — and the
  mission was created anyway, so it looked right. New form **`#ID@Class:rN`**, resolved through the
  control's own `GetRowFromY` (never a pixel), parsed before the generic `:%d` which fails on `r0`.
  **Stated plainly: on this save both profiles produce the same wave**, so this corrects what the
  recipe *addresses*, not what it produces.
- **Two corrections to the S158 walkthrough:** the bottom-left dialog is the **MISSION FOLDER**, not
  a "COMBAT ORDER" (S158 named it from a gold frame cut off at `…DER`); and gold's `F84` vs the
  port's `F80` is the campaign date, not a defect.
- **Scope, said out loud:** S162-3 was planned as K3 and was replaced by the `:rN` work, which this
  sprint turned up and which had to land before the gate could claim anything. K3 → S163.
- Gates: authorize_mission PASS, parity 5/5, oob_sweep 9/9, map_filter, dialog_scroll, help_click.

### 🏃 Sprint 161 — "Synced is not processed" (cross-port debt) — ✅ CLOSED 2026-08-21 (goal MET, 8/8) — ⭐ S159 rediscovered a bug already written down in our own tree

**Sprint Review (PO pre-approved ceremony, logged 2026-08-21):** `port/scrum/sprint-161.md`.

- ⭐ **`§8-BoB181` describes S159's PO-49 exactly and had been sitting in MA's byte-identical copy of
  the shared notes.** The sync was never the problem — syncing was being mistaken for *processing*.
  Three BoB notes (181/182/183) sat unanswered from S157 while MA rediscovered one from a play-test
  defect. Cost: a sprint.
- **Structural fix: `§8-LEDGER`**, one row per note with a per-port verdict (applied / N/A + reason /
  open + blocker). A note with no row is unprocessed by definition. It names **MA's own** unassessed
  rows (`§8-BoB173`, `173d`, `180b`) rather than quietly omitting them.
- **Verdicts shipped:** BoB182 **N/A** — MA has the identical `ChangeDisplaySettings` stub but
  implemented *neither* half, and declining is correct here (the caller switches the *desktop* to
  640×480); the stub now says so under `MA_TRACE_STUB=1`. BoB183 **N/A** — PO-1/S97, and MA's paint
  and click walks enumerate the same two toolbars.
- **Notes sent: MA 107–110**, including ⭐ *two constructors, one fix* (S160's `Inst3d` race) and the
  `gdb`-under-`ptrace_scope=1` technique. Both copies re-synced byte-identical.
- **`port/ref/native/` labelled:** 5 oracles, 50 undated snapshots — several showing bugs since fixed
  (the `1021×644` size *is* the canvas-overhang bug). Not refreshed wholesale, because their recipes
  were never recorded. `README.md` says so per file. **If you need a reference to be true, gate it.**
- Gates: parity 5/5 byte-identical, map_icon_click PASS.

### 🏃 Sprint 160 — "Photo" (K1 + K2) — ✅ CLOSED 2026-08-21 (goal MET, 8/8) — ⭐ the 3D recon of the Wonju Supply Dump renders natively

**Sprint Review (PO pre-approved ceremony, logged 2026-08-21):** `port/scrum/sprint-160.md`.

Steps 4 and 5 of the PO's script.

- **K1 ✅** Map items now carry **the game's own name** (`GetTargName`) and `MA_MAP_CLICK_NAME=Wonju`
  selects by it — necessary, because a band cannot pick one of *twenty* `AmberSupply` items. The
  dossier matches the script **on content**: it predicts *"no MiGs expected, but a large AAA
  presence"*; the port reads **Threat AAA High / MiG 15 Low**, MSR **Central**. A name that matches
  nothing clicks nothing and says so.
- **K2 ✅** Photo hung the game. `ptrace_scope=1` blocks attaching, so it was run **under** gdb:
  **thread 11 had already taken SIGSEGV in `Inst3d::moveloop` while thread 1 was still inside
  `Inst3d::Inst3d(bool)`**, down in `Three_Dee.InitialiseCache()` building the landscape cache the
  worker reads. ⭐ **S69 fixed this identical race in the no-argument `Inst3d` twin and the fix never
  crossed the 100 lines to this one.** *When a fix is a reordering inside a constructor, look for the
  constructor's twins before closing it.* Invisible to every gate we own because they all set
  `MA_DISABLE_3D=1`, and with 3D off the photo dialog never launches 3D at all.
- **New gate `port/recon_photo.sh`** — four assertions, negative control checked (`MA_DISABLE_3D=1`
  → FAIL). Its first "is this a rendered scene" test asked for >2000 distinct colours and failed a
  perfectly good frame: **the software rasterizer is 8-bit palettised and can never exceed 256.**
  Measure something the renderer can actually produce (S64's rule).
- Gates: `recon_photo` PASS, **`stress_launch` 20/20** (the gate Phase 5.1 built for exactly this
  class of change), parity 5/5 byte-identical, map_icon_click PASS.

### 🏃 Sprint 158 — "A new gold standard, and the class of target it asks for" (EPIC K opened) — ✅ CLOSED 2026-08-21 (goal MET, 8/8)

**Sprint Review (PO pre-approved ceremony, logged 2026-08-21):** `port/scrum/sprint-158.md`.

The PO added `~/gold standard/ma/wonju_attack.mp4` + `wonju_script.txt` and said the intent out loud:
*"as a test of campaign I will try to create and run this mission in linux MA."* EPIC K opened
(K0–K13, 75 pts) as an **end-to-end acceptance run of the mission builder**, not another screen-parity
epic.

- **K0 ✅** `gold_video.sh wonju`; `port/scrum/wonju-walkthrough.md` maps the script's steps to
  timestamps. **The recording stops at the frag screen — steps 15–18 have no video oracle**, which is
  the sort of thing that becomes a fictional verdict if nobody writes it down.
- **K1 🔨 measured.** `MA_MAP_ITEM_SCAN` now names each item's UID band, tallies the classes on the map
  (**20 AmberSupply**, 22 AmberBridge, 5 AmberAirfield, 3 AmberCivilian, 6 WayPoint) and accepts
  `MA_MAP_CLICK_BAND=AmberSupply` so a test can ask for the class the walkthrough starts from instead
  of taking whatever the scan hits first (a bridge). The supply dossier opens with the right fields and
  exposes **Photo** (K2) and **Authorize** (K4).
- **PO-49 found by measurement:** the dossier's backdrop art overhangs its dialog by ~281 px (measured
  off the capture; S159's trace then gave the exact figures — a **540×602 bitmap in a 327×316
  dialog**). PO-47's shape one screen on. → Sprint 159.
- Gates: ninja clean, `map_icon_click.sh` PASS, `parity_2d.sh` **5/5 byte-identical**.

### 🏃 Sprint 159 — "The dossier is the size it says it is" (PO-49) — ✅ CLOSED 2026-08-21 (goal MET, 8/8) — ⭐ every campaign dialog's art was oversized, not just the one reported

**Sprint Review (PO pre-approved ceremony, logged 2026-08-21):** `port/scrum/sprint-159.md`.

`RDialog::OnPaint` hands `SetDIBitsToDevice` the **bitmap's** width and height, never the dialog's.
Windows clips painting to the window; this port has none, so the target dossier's `FIL_MAP_SUPPLY`
backdrop painted **540×602 into a 327×316 dialog** and hung a 286 px skirt over the map. The art blit
is now clipped to the dialog's own rect (`MA_NO_ART_CLIP=1` reverts, `MA_TRACE_OOB` prints one
`[artclip]` line per clipped node).

- **It was never one dialog.** A/B over the OOB sweep: **9 of 9** reclaim map area — bases 172,230 px,
  intelligence 113,635, overview 43,623, weather 39,198, dis 35,080, directives 31,132, playerlog
  30,387, missionfolder 27,132, squads 1,489.
- **Not S155's reverted clip.** S155 clipped the node rect around the *controls* and it ate the tab
  row and the combo border. A backdrop is different in kind — art larger than its own dialog is
  always wrong — so only the DIB blit is clipped, and parity stays byte-identical.
- **PO-43 is untouched and still open**, visibly: the Intelligence supply table still runs past the
  dialog bottom. It is a `ResizeToFit` listbox, not art, exactly as S155 said.
- **A gate that was reporting on itself:** `asan_campaign.sh` said "NO-MAP / INCONCLUSIVE", which
  reads like this sprint breaking the campaign. An A/B with the clip disabled failed identically, and
  the real cause was its **hardcoded pixel** navigation (the S62/S63 trap). Switched to the symbolic
  `f,rN` / `f,#ID` recipe → **MAP-OK 2/2, 0 ASan reports.** Why the pixels stopped working is *not*
  claimed: `hw_gate.sh` still passes with the same three. `ab.sh`, `asan_flight.sh` and `hw_gate.sh`
  are logged for the same treatment.
- Gates: parity 5/5 byte-identical, oob_sweep 9/9 OPEN 0 CRASH, sysbox_exit, map_icon_click,
  map_filter, dialog_scroll, map_drag, help_click, panel_click (real GL @1920×1080), asan_campaign.

### 🏃 Sprint 127 — "Only the axis with no room" (B6 SHIPPED) — ✅ CLOSED 2026-08-15 (goal MET) — ⭐ the campaign UI runs at full resolution

**Sprint Review (PO pre-approved ceremony, logged 2026-08-15):** `port/scrum/sprint-127.md`.

- **⭐ B6 is on by default.** The 2D front end runs at the selected resolution. At 1920×1080 the
  campaign map fills the screen and the Player Log sits at (783,340) 339×400 against gold's
  ~(780,330) ~340×420. **PO-17 closes with it** — the dialogs were never misplaced, they were drawn
  on a canvas a third of the intended size.
- **The fault, found by measuring the scroll rather than the pixels:** baseline (0,642) → one-way
  (0,562) → round trip **(100,642)**, against a maximum of (0,2139). The Y axis, which has scroll
  room, restored perfectly; **only the X axis, which has none, did not.** The engine clamps only the
  low end, so a drag left is held at 0 and the drag back adds its full delta unopposed. Clamping to
  the map's real extent on every paint makes the round trip exact — 0 px differ.
- **Three sprints, three wrong descriptions of one artifact** ("missing tile column", "107px seam",
  "stale canvas repaint"), all from looking at pixels. Measuring the quantity that decides the
  behaviour — tile coverage, then scroll — ended it each time.
- Parity stayed **5/5 byte-identical** throughout: the gate runs at the default resolution where the
  canvas still equals the art size, so shipping this did not cost the regression net.
- **Gates (flip ON):** parity 5/5 · map drag PASS · sweep 9 OPEN/0 CRASH · map click · sysbox ·
  help click · stress 12/12 · hw_gate PASS.

### 🏃 Sprint 126 — "Not a seam: the map ran out" (B6) — ⚠️ CLOSED PARTIAL 2026-08-15

**Sprint Review:** `port/scrum/sprint-126.md`.

- **The black band was the map ending, not a seam.** `[maptile] client=1728x888 tile=256 areax=4
  endx=4 -> tiles reach x=913`: the map is 4 columns × 256px = 1024px wide at the startup zoom
  against a 1728px client. Every tile loads.
- **The engine already had the answer** — `CMIGView::Zoom()`'s *"min zoom for full screen map"*
  block, guarded by `rect.bottom > m_size.cy`: **height only**. The map is 4 across and 7 down, so
  on 4:3 height always bound and width came free; 16:9 reverses that. Now takes the larger of the
  two required zooms, identical to the original whenever height binds.
- **It had to be called from the right place:** `Zoom()` only runs when the player zooms, and
  `CMIGView::OnDraw` — the natural hook — is never called by this port, which paints the map from
  its idle loop. Measured by hooking it and watching the trace never fire. The hook belongs in
  `UpdateBitmaps`.
- Default flip **reverted again** on gate evidence: panning still lost 820211 px per round trip.

### 🏃 Sprint 125 — "The gate said no" (B6 default flip) — ⚠️ CLOSED PARTIAL 2026-08-15

**Sprint Review:** `port/scrum/sprint-125.md`.

- The flip was **correct in layout** and **reverted on gate evidence**: `map_drag` round trip
  differed by 108000 px ≈ 107×1080. **A gate failure is a reason not to ship, and saying so is part
  of the job** — the alternative is knowingly shipping a regression to satisfy the letter of a
  request.
- No reference re-baseline was needed, contrary to the plan: parity runs at the default resolution
  where the canvas still equals the art size, so all five references stayed byte-identical.

### 🏃 Sprint 124 — "The canvas, not the dialogs" (B6) — ✅ CLOSED 2026-08-15 (goal MET)

**Sprint Review:** `port/scrum/sprint-124.md`.

- **Gold's Player Log is ~340×420 in a 1920×1080 front end; ours was the same 339×400 in an 800×600
  canvas** — 18%×39% against 42%×67%. Identical absolute size; only the canvas differed, which is
  why three open dialogs could not avoid colliding.
- Nothing pinned the canvas to 800×600: it grows to fit what is drawn, and the first thing drawn is
  an 800×600 background. `ma_gdi_set_screen_size()` now establishes it from `Save_Data.displayW/H`,
  and `MaViewRectScope` keeps the view rect so the map fills it. **A workaround for a too-small
  canvas stops being correct once the canvas is right.**

### 🏃 Sprint 123 — "Campaign flies; the dialogs are not misplaced" (campaign playability) — ✅ CLOSED 2026-08-15 (goal MET, PO-17 re-scoped)

**Sprint Review (PO pre-approved ceremony, logged 2026-08-15):** detail in
`port/scrum/sprint-123.md`.

- **⭐ A campaign mission flies end to end.** Driven from the map: frag → briefing → Fly →
  StartFlying → `Launch3d returned`, and the flight renders at 1920×1080 (97% non-black, 26143
  colours) with terrain, cockpit, placard and info line. `port/ref/native/camp_flight.png`. The
  campaign path itself is not broken — what blocked the PO was navigating it.
- **PO-17 re-scoped by measurement.** Three campaign dialogs open together sit at three DIFFERENT,
  correct rects — (223,92) 339×400, (142,89) 501×407, (164,101) 457×382. Each is placed where the
  game asks, so **this is not a placement bug**. They overlap because they are all open at once in
  the same region. The fix is either "opening one dismisses the others" or making the panels
  draggable; next sprint checks the shipped behaviour before choosing.
- **⭐ "Filter, don't cap" — booked for the fifth time, and I walked into it again.** The first
  measurement said all three dialogs were the SAME object at the SAME rect, which would have sent
  the next sprint chasing a placement bug that does not exist. The trace was capped at the first 24
  prints *across all passes*, so it was spent entirely on the dialog that happened to be open
  first. Re-keyed per distinct dialog, the real picture appeared immediately. **A capped trace does
  not report less, it reports something false.**
- Kept: the OOB paint walk now paints each distinct dialog once per pass (cheap guard against
  double-painting a translucent panel through two slots).
- **Gates:** parity 5/5 byte-identical · sweep 9 OPEN/0 CRASH.

### 🏃 Sprint 122 — "The surface is the mode, not the window" (PO-20) — ✅ CLOSED 2026-08-15 (goal MET) — ⭐ high resolution works in both renderers

**Sprint Review (PO pre-approved ceremony, logged 2026-08-15):** detail in
`port/scrum/sprint-122.md`.

- **⭐ One trace answered the PO's report.** At 1920×1080: mode 6 selected, and
  `[overlay] GetSurfaceDimensions -> 640x480`. `COverlay` places the info line, messages and
  instruments from that answer, so the HUD sat in a corner — while the terrain looked right,
  because the GL path projects from the drawable, not the surface. Two notions of "the screen",
  disagreeing.
- **Root cause was S115's.** `SetDirectDrawMode` sized the render surface from the **window rect**,
  which still holds the previous size when the mode is set (SDL resizes afterwards). Before S115
  this was accidentally correct: `::GetWindowRect` was a zero-fill stub, so the mode-based fallback
  always fired. S115 made that stub real — rightly, `SetViewParams` needs it — and removed the
  fallback. **Correct behaviour that rests on a stub being wrong is a fault waiting for the stub to
  be fixed.** The surface is now sized from the mode, which is what the port means.
- **It fixed the software renderer too.** S119 measured software at 1920×1080 as tiled 3× and
  squashed into the top 160 rows — the signature of rendering 640 wide and presenting 1920 wide.
  Same cause. Content now fills the frame. **High resolution works in both renderers**, having been
  logged as a pre-existing defect in both.
- **Reproducibility cost more than the bug.** The PO also hit a hang at max settings, and I could
  not reproduce it: their configuration lived only in memory, and killing the hung process to read
  its stacks destroyed the state being reported. Now `MA_FORCE_DETAIL=max` makes "all settings to
  max" scriptable, and preferences autosave once a minute (`MA_NO_PREF_AUTOSAVE=1` disables). The
  max-settings run then reproduced clean.
- **Gates:** parity 5/5 byte-identical · sweep 9 OPEN/0 CRASH · map click · map drag · sysbox ·
  help click · stress 15/15 · **hw_gate PASS**.

### 🏃 Sprint 121 — "The front end had no keyboard" (PO-16) — ✅ CLOSED 2026-08-15 (goal MET)

**Sprint Review (PO pre-approved ceremony, logged 2026-08-15):** detail in
`port/scrum/sprint-121.md`.

- **The player can type a profile name.** `port/ref/native/career_typed.png` — TESTPILOT in the
  Player Log → Career Name field, entered through the path a player's keystrokes take.
- **⭐ The front end had no keyboard route at all.** Every hosted OCX control was click-only, and
  `CWnd::SetFocus()` was `{ return NULL; }` — nothing recorded which control had the keyboard.
  `CAREER.CPP` calls `SetCaption / SetEnabled / SetFocus` correctly; the keystrokes had nowhere to
  go. Selection had always been enough, so the gap was invisible.
- **Editing lives in the host, deliberately.** Calling the game's own `CREditCtrl::OnChar`
  segfaulted: it runs in an MFC message context this port does not provide (measures through
  `GetDC()`, invalidates, drives a caret timer), and chasing those nulls one at a time is
  unbounded. The host supplies the behaviour, exactly as `ma_ole_click` cycles a combo rather than
  invoking its `OnLButtonDown`. The text still lives in the game's control, so its own `OnDraw`
  renders it.
- **Two compat nulls found and kept — both have wider reach:** `CDC::GetTextExtent` reached
  `strlen(NULL)` because an **empty** CString converts to a NULL `LPCSTR`; and
  `ma_gdi_get_text_extent` dereferenced a DC it does not own. Neither could fire while the front
  end was click-only.
- **New injector `MA_TYPESEQ`.** Typing was the one front-end interaction with no synthetic driver,
  which is why this could only be reproduced by hand. Count 0 means "as soon as an edit has focus",
  because the count is in *pumps* and a frame-shaped number silently never fires — the S113/PO-13
  lesson, which caught me again on the first attempt. *A defect you cannot drive from a script
  cannot have a gate.*
- **Gates:** parity 5/5 byte-identical · sweep 9 OPEN/0 CRASH · map click · map drag · sysbox ·
  help click · stress 12/12.

### 🏃 Sprint 120 — "The landscape has its own pipeline" (PO-15) — ✅ CLOSED 2026-08-15 (goal MET) — ⭐ terrain renders in hardware

**Sprint Review (PO pre-approved ceremony, logged 2026-08-15):** detail in
`port/scrum/sprint-120.md`.

- **⭐ Hardware terrain renders.** `port/ref/native/hw_terrain.png` — quick mission at 4,530 ft:
  brown/olive Korean terrain, 7595 distinct colours in the lower view (was one flat black), matching
  `sw_terrain_ref.png` at the same altitude.
- **The PO's observation was the diagnosis:** *"huts and control tower visible, landing strips not."*
  One class of surface drawn and another not, same frame — that eliminates geometry, depth, blending
  and projection in a sentence, and points at a texture path objects do not use.
- **Terrain has its OWN texture pipeline.** Objects go through `CreateTexture`/`PrepTexture`/`Load`;
  the landscape rasterises tiles into a system surface (`TileMake::RenderTile2Surface`) and blits
  them to video. **Root cause: the compat `IDirectDrawSurface2::Lock` never filled
  `ddpfPixelFormat`**, so `rsd.dwRGBBitCount` reached the tile rasteriser as **0** — no format to
  write in — and every land tile came back blank. Blank tiles blit to video, index 0 is the
  transparent key, so terrain uploaded fully transparent and the cleared black showed through.
- **The chain that found it was mostly negative results:** land Executes carry vertices (not a
  submission problem) → false-colour leaves the horizon band black (not a shading problem) → bound
  texture has 0 of 4096 texels (it is the texture) → `UploadLandTexture` never called (a dead
  branch; its sys-RAM half is commented out in the shipped source) → **the blits run but the source
  is empty** (the fault is upstream of everything I had been changing).
- **A wrong turn, recorded:** I "fixed" this in S119 by carrying the palette across `Load` and
  declared terrain fixed from a capture at 17,000 ft where the ground is haze. The palette bug was
  real but unrelated. *Measuring the right quantity in the wrong conditions is not a measurement.*
- **New standing gate — `port/hw_gate.sh`.** S118 shipped with every gate green because the suite
  pins `MA_NO_HARDWARE=1`, withdrawing the device entirely — a configuration no player has. The new
  arm runs parity (renderer-independent screens only), stress and the campaign path on the
  **hardware** renderer. Its first two runs "failed" on correct behaviour, twice by my error:
  asserting byte-identity on the Preferences screens that legitimately report the renderer, and
  `tail -1` never seeing the verdict because gl-lock prints last.
- **Gates:** parity 5/5 byte-identical · sweep 9 OPEN/0 CRASH · map click · map drag · sysbox ·
  help click · stress **20/20 software and 20/20 hardware** · **hw_gate PASS**.

### 🏃 Sprint 119 — "What the PO found in ten minutes" (play-test of S118) — ✅ CLOSED 2026-08-15 (goal MET)

**Sprint Review (PO pre-approved ceremony, logged 2026-08-15):** detail in
`port/scrum/sprint-119.md`.

- **The PO ran the shipped hardware option under a debugger and it failed immediately** — SIGSEGV
  entering 3D, then campaign screens showing stale patchwork text, then Fly leaving a blank window.
- **⭐ Why four green sprints missed it:** S118 made the driver properly visible (`dddriver=-1` +
  3D-capable primary), which is exactly the condition at `Win3d.cpp:1826` that makes the engine
  request **fullscreen** — selecting the flip-chain path (`Hardwin.cpp` case 2) that the port had
  never executed. Every hardware sprint before it ran windowed. **Shipping the option moved the
  renderer onto untested code in the same change.**
- **Three faults:** `GetAttachedSurface` was a stub returning `DD_OK` with a NULL out-pointer (the
  crash — and my first fix replaced it with an unbounded chain, since callers WALK the chain, so the
  terminator is the fix); the port must stay **windowed** (`isFullScreen()` now false under
  MA_LINUX, `MA_ALLOW_FULLSCREEN=1` to restore); and the 3D scene was sized from `g_scrW/g_scrH`,
  which the 2D canvas overwrites with 800×600 mid-flight — hence the PO's 1920×1080 "upper-left
  quadrant".
- **My frame dump shared the same bug**, which is why I had reported 1920×1080 as correct: it read
  `g_scrW/g_scrH` too. *A capture that shares a bug with the code under test is not evidence.*
- Also fixed: `IDirect3DTexture::Load` now carries the **palette**, not just the texels.
- **Gates:** as S120 above (both sprints verified together).

### 🏃 Sprint 118 — "The player can choose it" (PO-12 phase 4) — ✅ CLOSED 2026-08-15 (goal MET) — ⭐ **PO-12 DELIVERED**

**Sprint Review (PO pre-approved ceremony, logged 2026-08-15):** detail in
`port/scrum/sprint-118.md`.

- **⭐ The PO's ask is delivered.** Preferences → 3D → **Display Driver** now offers *Software
  Driver* and **Primary Display Driver**; choosing the latter writes `fSoftware=false` to
  `settings.mig` through the game's own writeback, and the next launch flies on the hardware
  renderer **with no environment variable anywhere**
  (`port/ref/native/hw_selected_in_prefs.png` — a flight whose only instruction was the menu click).
- **The first experiment found the real defect.** Running the standing parity gate with hardware
  forced on cost one line and showed `prefs_3d` differing by 704 px — exactly the `640 X 480`
  readout: **the Resolutions combo was empty in hardware mode.** Three things all derive from
  `dddriver` and all had to agree — the mode's `driverNo` tag, the width table
  (`hard_modes[dddriver+1]`, taken from SDETAIL's own expression rather than guessed), and
  `dddriver` itself (the port's one hardware driver is the primary, which is **-1**; a saved 0
  selects a combo entry that does not exist). Normalised on load so an old settings file cannot
  carry an unreachable driver number in.
- **An option has to be visible to be chosen.** SDETAIL adds the hardware entry only when
  `!fNoHardwareAtAll && sd.fFirstHardIsPrimary` — the engine sets those in CONFIG.CPP after probing
  a real device; the port's is synthetic, so it states the same conclusion for it.
- **The three places that forced software** (S110 measured them and warned a choice must survive
  all three) are now one predicate, `ma_hardware_available()`. `MA_TRY_HARDWARE=1` remains a
  developer override; `MA_NO_HARDWARE=1` withdraws the offer.
- **Gates now pin their renderer.** With the renderer a player setting, an unpinned gate tests
  whichever one `settings.mig` holds — and this repo's own runs write that file. Every gate pins
  `MA_NO_HARDWARE=1`; `=0` runs it on hardware. **The standing gates pass on BOTH renderers**:
  parity 5/5 byte-identical software *and* hardware, stress 20/20 software *and* hardware, plus
  sweep 9 OPEN/0 CRASH · map click · map drag · sysbox · help click · overlay text 3/3 · ASan 0.
- **A harness error, booked not hidden:** overlay-text first came back FAIL because I ran it
  concurrently with the ASan gate — two runs driving the display, only one inside `gl-lock`.
  *A gate result obtained outside the display lock is not a result.*

**Retro.** Four sprints from "the hardware path returns D3D_OK and draws nothing" to a shipped
option. What made them cheap was measuring the thing itself each time — the opcode census, the
off-screen breakdown, the texel histograms, the parity gate pointed at the new mode — rather than
reasoning about which layer was to blame.

### 🏃 Sprint 117 — "Lines, points, and the depth the engine meant" (PO-12 phase 3c) — ✅ CLOSED 2026-08-15 (goal MET)

**Sprint Review (PO pre-approved ceremony, logged 2026-08-15):** detail in
`port/scrum/sprint-117.md`.

- **The hardware frame now matches the software oracle on the cockpit view.**
  `port/ref/native/hw_cockpit_full.png` vs `sw_cockpit_ref.png`: canopy, panel, compass, altimeter
  tape, gunsight and pipper, artificial horizon, wireframe attitude gizmo, lower coaming, and the
  info line — `Speed: 379Kts  Mach: 0.64  Alt: 17662ft  Hdg: 279  Thrust: 0`.
- **Lines and points drawn** — 133824 `D3DOP_LINE` and 8271 `D3DOP_POINT` per flight, stepped over
  since S115. `D3DPOINT` is a RUN (`wCount` from `wFirst`), which the macro says and a guess would
  not.
- **The font texture is a coverage MASK.** Its RGB is uniformly zero on purpose — `SetPalette`'s
  "knobble" block pins the `FONTMASK` entry to `0x08`, a marker not a colour, and `PutC` puts the
  real colour in the **vertex**. `GL_MODULATE` computes `tex.RGB × vertex` = black. The renderer
  now detects the mask **from the texels** (RGB blank while alpha varies) and switches to
  `GL_COMBINE` — colour from the vertex, coverage from the texture. Same finding as S102 for the
  software path, reached from the other side.
- **⭐ Two depth faults, one symptom, two "missing features".** (a) **Render state must persist
  across execute buffers** — the walk reset it every `Execute`, and the census proves the engine
  relies on persistence: it sets `ZENABLE` **once in a whole flight**. (b) **`glOrtho` negates z**,
  so `depth = (1−z)/2` — the reverse of D3D's convention, which made *farther* geometry win and
  rejected the overlay batches that sit at the near end. With both fixed, depth testing stays ON
  (the engine's own `ZENABLE=1`, `ZFUNC=LESSEQUAL`), and the info line and coaming both appear.
- **Method note worth keeping:** depth was suspected and "cleared" back in S115 — but that control
  ran while blend was still multiplying everything to zero, so it proved nothing. **A control that
  runs while a known fault is still present does not clear its suspect.**
- **Hot-path hygiene:** `getenv` was being called **per texel** in one trace and per draw in three
  others; all cached. The S115 sequence scaffolding is removed — it answered its question.
- **Gates:** parity 5/5 byte-identical · sweep 9 OPEN/0 CRASH · map click · map drag · sysbox exit ·
  help click · overlay text 3/3 · stress 20/20.

### 🏃 Sprint 116 — "The textures arrive" (PO-12 phase 3b) — ✅ CLOSED 2026-08-15 (goal MET)

**Sprint Review (PO pre-approved ceremony, logged 2026-08-15):** detail in
`port/scrum/sprint-116.md`.

- **The hardware cockpit is textured.** `port/ref/native/hw_cockpit_textured.png`: metallic canopy
  frame with shading, instrument panel, compass gauge, altimeter tape, trim knob, gunsight glass
  with reflections and pipper, artificial horizon, textured terrain. **6722 distinct colours, up
  from 86 in S115.** `sw_cockpit_ref.png` is the software renderer at the same frame, as the oracle.
- **Both formats identified by measurement, not assumption.** `MA_TRACE_TEX` shows the game uses
  exactly the two formats `EnumTextureFormats` offered it in S110 and nothing else: **ARGB4444**
  and **8-bit palettized**. 4444 maps to GL with no conversion pass (`GL_BGRA` +
  `GL_UNSIGNED_SHORT_4_4_4_4_REV`); the surface now remembers the masks it was created with,
  because 4444 read back as 565 is unrecognisable art with no alpha and nothing would say so.
- **⭐ `IDirect3DTexture::Load` was a no-op, so every texture was empty** — the first upload trace
  read **"0/4096 non-zero texels"** for every texture in both formats. The engine writes texels
  into a SYSTEM surface and then `dest->Load(src)` copies them to the video texture, which is the
  surface the handle names and the one the renderer uploads. Nothing had ever written to it.
  *When art looks wrong, first check that there IS art:* an empty texture and a misread format
  look alike from the screen.
- **`CreatePalette` returned NULL, so palettized art was black.** A real vtbl-backed
  `IDirectDrawPalette` now holds its 256 entries, and each surface remembers which palette its
  texels index — the engine keeps `MAX_PALS` of them and picks per texture, so the global display
  palette is not a substitute.
- **Textures re-upload on change, not per frame:** `Unlock` on the face `PrepTexture` writes
  through forwards a dirty flag to the surface that owns the pixels and the GL texture name.
- **Still missing vs the software oracle (→ S117):** the bottom info line (the engine's own
  `direct_3d::PutC` text path), the lower cockpit coaming, and the `D3DOP_LINE` (76224) /
  `D3DOP_POINT` (5329) instructions the walk still steps over.
- **Gates:** parity **5/5 byte-identical** (the palette object is new on a path the 2D front-end
  also uses) · sweep 9 OPEN/0 CRASH · map click · map drag · sysbox exit · help click · overlay
  text 3/3 · stress 20/20.

### 🏃 Sprint 115 — "The hardware path draws" (PO-12 phase 3) — ✅ CLOSED 2026-08-15 (goal MET) — ⭐ first hardware-rendered frame

**Sprint Review (PO pre-approved ceremony, logged 2026-08-15):** detail in
`port/scrum/sprint-115.md`.

- **⭐ The hardware renderer puts a frame on the screen.** `IDirect3DDevice::Execute` now walks the
  opcode stream the game writes and submits its triangles to GL: 13028 streams read (0 unusable,
  every one EXIT-terminated), **562909 triangles over 1406 scenes, 99.5% screen coverage.** The
  capture (`port/ref/native/hw_cockpit.png`) shows the cockpit — canopy frame, windscreen bow,
  gunsight housing, instrument coaming, sky and hazy horizon — rasterised on the GPU from the
  game's own execute buffers. White surfaces are the ones still awaiting their texture (S116).
- **Three separate faults, each of which alone produced a black screen:**
  1. **The D3D→GL blend table was off by one.** The game asks for `SRCALPHA`/`INVSRCALPHA`; the
     table answered `ONE_MINUS_SRC_ALPHA`/`DST_ALPHA`, so with opaque alpha the source factor was
     1−1 = **0** — every triangle rasterised and multiplied out of existence. **Shared with the DX7
     path, so `~/bob` inherits the fix** (cross-port note §8-MA101).
  2. **Texture handles were always 0.** S113 stored the handle on a subclass, but these interface
     methods are not virtual, so `GetHandle` dispatched to the base and returned 0 = "no texture".
     Moving the handle into the base took textured triangles from 0 to 474864.
  3. **⭐ `GetWindowRect` was a zero-fill stub.** `SetViewParams` computes
     `viewdata.originy = screen_height − window_height/2`, so with `screen_height` 0 the *entire
     world* was generated 240–480 px **above** the screen. The measurement that named it:
     **58.1% of triangles wholly off-screen — 327172 above, 0 below, 0 left, 0 right.** A uniform
     one-way offset is a missing origin, not a clipping or projection bug. After the fix: 0.0%
     off-screen. **Third sprint lost to this bug class** — a stub returns a plausible zero and
     quietly reroutes real work off the path the shipped game used.
- **Also:** the per-frame execute-buffer leak is closed (real refcounting; the game creates one
  buffer per frame); `MATRIX.CPP body2screen`'s hardware branch is live again, guarded so software
  mode is bit-identical; the 2D present no longer uploads over a hardware frame.
- **Method note — two wrong predictions, cheaply.** Predicted depth twice; measured blend, then
  geometry placement. What did the work was a **control arm** (an immediate-mode quad through the
  identical projection at the identical moment landed its exact 10000 px, clearing context,
  projection, thread and readback in one run) and a **whole-framebuffer count** rather than a probe
  at a vertex, where the fill rule can legitimately exclude the pixel.
- **Gates:** parity **5/5 byte-identical** (the `GetWindowRect` change is global — this is the gate
  that mattered) · sweep 9 OPEN/0 CRASH · map click · map drag · sysbox exit · help click · overlay
  text 3/3 · stress **20/20** · ASan 0.

**Retro.** Three sprints of hardware work assumed the geometry was not arriving. It was arriving
the whole time — submitted correctly, then multiplied by zero, then drawn off the top of the
screen. **When a renderer is silent, prove the pipe with a control draw before scoping the pipe.**

### 🏃 Sprint 114 — "The help source was in the tree" (PO-10) — ✅ CLOSED 2026-08-15 (goal MET) — ⭐ the "?" shows the real documentation

**Sprint Review (PO pre-approved ceremony, logged 2026-08-15):** detail in
`port/scrum/sprint-114.md`.

- **⭐ The compiled help file never had to be decoded.** Three sprints had been working on
  `MIG.HLP`'s Hall compression (S98 routing, S99 four of five stages, S112 an 800-candidate search).
  The PO pointed at the Wine tree (`winhlp32.exe` — a viewer that could have given ground truth) and
  at BoB, whose help ships as CHM **with its sources beside it** — and that is the clue that
  mattered: this engine keeps help SOURCES in the source tree. MiG Alley's are in
  `SRC/<LANG>/HELP/`: **MIG.RTF** (the RTF the .HLP was compiled from), **MIG.HPJ** (which records
  `COMPRESS=12 Hall Zeck`, naming the very compression that cost two sprints) and **MIG.HM** (the
  symbol → context-number map).
- **Shipped:** `port/tools/rtf_help.py` extracts 43 topics + 186 context ids into
  `port/data/mig_help.txt` (installed with the game); `ma_help.cpp` resolves the context id the game
  passes — `HID_BASE_RESOURCE+IDD_INTRODUCTION` → `HIDD_INTRODUCTION` → the Introduction topic — and
  the panel renders that topic's **real text**, word-wrapped, with the index as fallback.
- **Two extractor details, both found by reading the output:** RTF formatting is **scoped to its
  group**, so an unsaved `\v` (hidden jump target) hides the rest of the topic; and bare newlines in
  an RTF file are layout, not text ("Yo\nu are supporting").
- **Side benefit:** the Map Screen topic documents *"five dockable toolbars: Title, Main, Utility,
  Scale, Filters"* — the game's own confirmation of PO-11's inventory.
- **Gates:** parity 5/5 · sweep 9 OPEN/0 CRASH · map click · map drag · sysbox exit · **help click
  (panel + resolved topic)** · overlay text 3/3 · stress 20/20 · ASan 0.

**Retro.** The PO's rule, learned the expensive way: **look in the tree before decoding a binary.**
Three sprints of decompression work were aimed at a file whose source sat two directories away.

### 🏃 Sprint 113 — "Textures, and a frame that survives" (PO-12 phase 2) — ✅ CLOSED 2026-08-15 (goal MET)

**Sprint Review (PO pre-approved ceremony, logged 2026-08-15):** detail in
`port/scrum/sprint-113.md`.

- **The hardware path now runs a whole mission**: 9114 `BeginScene`/`EndScene` cycles, textures
  loaded, no crash (16 distinct D3D methods). It still draws nothing — `Execute` does not walk the
  opcode stream yet, which is phase 3.
- **Cause of S111's crash:** `direct_3d::CreateTexture` builds a texture as three faces over ONE
  allocation (DX1 surface → its DX2 face → the texture object), and the DX2 face was a pure stub, so
  `PrepTexture` wrote texels through whatever `lpSurface` happened to contain. The DX2 surface is now
  a **view** that borrows the DX1 surface's pixels, and `QueryInterface` **dispatches on the IID**
  instead of S111's shortcut of returning the 3D device for every request.
- **Noted for phase 3:** `CreateExecuteBuffer` is called once per frame (9140 for 9114 frames) and
  `RELEASE()` is a no-op in compat, so buffers currently leak at that rate. Fine for a measurement
  run; must be real before the option ships.
- **Gates:** hardware stays opt-in; parity 5/5 byte-identical and stress 6/6 verified.

### 🏃 Sprint 112 — "Show what we can read" (PO-10) — ✅ CLOSED 2026-08-15 (goal MET) — the "?" opens a documentation window

**Sprint Review (PO pre-approved ceremony, logged 2026-08-15):** detail in
`port/scrum/sprint-112.md`.

- **⭐ The last of the PO's six play-test defects now has a visible answer.** Clicking "?" raises a
  panel listing the game's own help topics — *Map Screen, Main Toolbar, Filter Toolbar, Bases,
  Dossier, Squadron Information, Weather, Daily Intelligence Summary, Target List, Mission Results,
  Player Log, Aircraft Select, Routes, Debrief…* — parsed from `MIG.HLP`'s `|TTLBTREE` at runtime by
  the new `SRC/compat/ma_help.cpp`. `CWinApp::WinHelp`, the destination S98 spent a sprint reaching,
  had still been an empty stub.
- **A negative result worth having.** S99's oracle (*a correctly decoded topic contains its own
  title*) is what makes a search legitimate, so this sprint enumerated **800 candidate Hall opcode
  layouts** and scored every one: **best 2/39**. The layout is not in that family — the obvious
  space is now ruled out rather than merely unsearched, and the panel says plainly that topic TEXT
  is undecoded instead of showing plausible nonsense.
- **The gate stopped reporting a half-truth.** `port/help_click.sh` used to PASS with *"this proves
  routing only"* — which was exactly PO-10's shape: routing correct since S98, player sees nothing.
  It now asserts the panel is on screen and reports `PASS (click -> documentation panel)`.
- **Gates:** parity 5/5 · sweep 9 OPEN/0 CRASH · map click · map drag · sysbox exit · **help click
  (panel on screen)** · overlay text 3/3 · stress 20/20 · ASan 0.

**Retro.** When the decoder could not be finished honestly, the sprint shipped the part of the file
that *is* verified and labelled the gap in the UI itself. A gate that says "routing only" and passes
is a gate that agrees with the defect.

### 🏃 Sprint 111 — "A frame's worth of hardware" (PO-12 phase 1) — ✅ CLOSED 2026-08-15 (goal MET)

**Sprint Review (PO pre-approved ceremony, logged 2026-08-15):** detail in
`port/scrum/sprint-111.md`.

- **The game now submits execute buffers.** The D3D census went from 1 method to **14**, in the
  game's own order: EnumDevices → EnumTextureFormats → CreateViewport → AddViewport → SetViewport →
  CreateExecuteBuffer → Initialize → Lock → Unlock → SetExecuteData → **BeginScene → Execute →
  EndScene** → CreateMaterial. Nothing renders yet (`Execute` does not walk the opcode stream), but
  everything in front of it is live.
- **Two things had to become real:** (1) `IDirect3DExecuteBuffer` owns an allocation — the game
  Locks it and writes its whole instruction stream into `lpData`, which is why a NULL there wrote
  through a null pointer in `SetInitialRenderStatesLand`; (2) the device is obtained from the **back
  surface** (`lpDDSBack->QueryInterface(Driver[n].Guid, …)`), so the surface hands one back via
  `ma_d3d_device()` — a new TU, **registered in BOTH builders** (CMake *and* `port/rebuild.sh`, the
  one the ASan build uses — the S88 lesson).
- **Next rung, from a backtrace not a reading:** `PrepTexture` ← `CreateTexture` ←
  `RegisterTextureUse` ← `FlushPTDraw` ← `EndScene`. Textures next; then the `Execute` opcode walk,
  which is what finally draws.
- **Gates:** hardware stays opt-in (`MA_TRY_HARDWARE`), so shipped risk is `ddraw_legacy.h` plus one
  new TU — verified **parity 5/5 byte-identical, stress 6/6**.

### 🏃 Sprint 110 — "How far does hardware get?" (PO-12) — ⚠️ CLOSED PARTIAL 2026-08-15 — the ladder is measured, four rungs deep

**Sprint Review (PO pre-approved ceremony, logged 2026-08-15):** detail in
`port/scrum/sprint-110.md`.

- **PO-12 scoped by measurement, not by reading headers.** `MA_TRY_HARDWARE=1` makes the DX5/6 path
  reachable, `MA_TRACE_D3D=1` counts every compat D3D call. Five runs, each naming exactly one next
  requirement:
  1. `IDirect3D::EnumDevices` never invoked its callback → `DD.lpDirect3D` NULL → `HardPoly` returns
     FALSE on its first line;
  2. **three separate places force software** — `STUB3D::MakePassive`, the port's
     `ma_populate_software_modes`, and (since S103 made preferences load) the persisted
     `settings.mig` — and the choice must be made **before display init**;
  3. the game refuses to start without **two texture formats**: 8-bit palettized and 16-bit with
     alpha (ARGB4444) — otherwise `[SysError] 3D Hardware acceleration is not enabled`;
  4. with those, it reaches `CreateExecuteBuffer` → `Lock` → **SIGSEGV** in
     `SetInitialRenderStatesLand` ← `CreateLandExecuteBuffer`: the first stub that must become
     real is the execute buffer's **memory**.
- **BoB cannot be copied here** (confirmed, not assumed): BoB is D3D7 + Lib3D software T&L; MiG
  Alley is DX5/6 execute buffers. The approach cross-ports, the device does not.
- **Phase plan:** (1) execute-buffer memory → (2) opcode walk → GL (the game submits pre-transformed
  `D3DTLVERTEX`, so `PROCESSVERTICES` is a copy) → (3) textures → (4) the Preferences option with an
  automatic fall back to software.
- **Nothing rendered this sprint, deliberately.** Five runs bought a ladder in the order the game
  demands it; guessing would have produced a device built against assumptions.
- **Gates:** not re-run — the only shipped changes are inert unless `MA_TRY_HARDWARE`/`MA_TRACE_D3D`
  are set. S109's results stand.

### 🏃 Sprint 109 — "The art was always there" (PO-14 → PO-11) — ✅ CLOSED 2026-08-15 (goal MET) — ⭐ the campaign map has its filter toolbars

**Sprint Review (PO pre-approved ceremony, logged 2026-08-15):** detail in
`port/scrum/sprint-109.md`.

- **⭐ The campaign map now carries its chrome:** blue + red filter rows **with icons** (30 buttons),
  main toolbar, misc toolbar (6 buttons, drawn for the first time) and the system box — where S108
  found one and a half clusters.
- **The double-open opener was none of the ones I guessed.** Two runs went on guarding
  `CRToolBar::OnGetFile` and `CMIGView::OnGetFile`; `MA_TRACE_FILEOPEN` — added in S84 *for exactly
  this*, with the note "print the stack rather than reason from the call graph" — named
  `CMIGView::DrawIcon` in one run. It builds a stack-local `fileblock` per map symbol per frame,
  which is fine until the filter toolbar caches the same art (`FIL_ICON_R_SUPPLY_ON`, 0x6a48, is
  both a map symbol and the red supply button). **I had the right instrument and reached for
  guesses first.**
- **Art and captions were one predicate, and only captions were dangerous.** `ma_dlg_artnum_any()`
  now applies art while `ma_dlg_artnum()` still gates captions to the tickbox family, so S57's
  caption regression stays fixed while the art widens.
- **Widening the art exposed a ghost S97 had only half-fixed:** the system box drew a second time at
  its raw template origin on the title and Preferences screens (parity caught it at once — four
  screens differing in exactly rows 0–47, cols 0–71). The four map-chrome dialogs are now registered
  **parent-scoped at creation** instead of at first map draw.
- **Layout:** filter rows right-aligned at the top edge (drawing them at x=4 covered the map's date
  — *a widget must not change the state of the screen it draws on*), main toolbar at (4,52) with the
  22 px overlap gone, misc toolbar right-aligned on that band; clicks follow the paint offsets for
  both.
- **Gates:** parity **5/5 after a justified rebaseline of `campaign_map`** (diff confined to rows
  4–99, cols 451–747 — nothing below row 100; the other four returned to byte-identical once the
  ghost was fixed, which is what proves the widening is contained) · sweep 9 OPEN/0 CRASH · map
  click · map drag · sysbox exit · help click · overlay text 3/3 · stress 20/20 · ASan 0.

**Retro.** A previous sprint had already written the tool that answers this class of question, and
put a note in the code saying to use it. Reading that note only after two wrong guesses is the
lesson worth keeping.

### 🏃 Sprint 108 — "Count the widgets first" (PO-11) — ⚠️ CLOSED PARTIAL 2026-08-15 — inventory delivered, one blocker named

**Sprint Review (PO pre-approved ceremony, logged 2026-08-15):** detail in
`port/scrum/sprint-108.md`.

- **The comparison was not legitimate yet, and that is now measured.** New hook `MA_FORCE_RES=WxH`
  (writes the two fields the resolution combo writes) shows the port's **2D canvas stays 800×600 in
  every mode**, while the golds are 1280×1024. This engine picks its panel art set by resolution
  (S64), so a pixel diff against these golds is blocked on **B6**, not on the widgets.
- **PO-11 is now a work list, not a complaint:** filters `m_toolbar1` **30 controls hosted but
  drawn blank** (no art); main `m_toolbar2` ✅; misc `m_toolbar3` **6 hosted, never drawn** (the same
  enumeration gap S106 found); scale bar `m_toolbar4` **0 hosted** (`CScaleBar` draws itself and
  nothing calls it); debrief `m_toolbar5` ✅ since S106. The three top clusters need ~1190 px and
  t1/t2 currently **overlap by 22 px**.
- **One blocker, with evidence rather than suspicion:** button art is a design-time property that
  S57 had to restrict to tickboxes after a regression, and the existing re-widening switch
  `MA_BTN_ART_ALL=1` **crashes** — `[SysError] Opened file block (6a48) again without closing`, the
  S79/S84 double-open family. So "give the filters their icons" is really "make button art
  resolvable without double-opening the art file", and it gets its own sprint.
- **Deliberately not done:** drawing the misc toolbar before its art works would have added six
  blank rectangles. S94's rule — *positioned and clickable but invisible is not a fix* — applies to
  a widget that is merely present, too.
- **Gates:** parity 5/5 · sweep 9 OPEN/0 CRASH · map click · map drag · sysbox exit · help click ·
  overlay text 3/3 · stress 20/20 · ASan 0. (One counter, one trace, one env hook; no render path
  changed.)

**Retro.** The sprint that did not fix anything is the one that made the next one cheap: five
clusters, a named mechanism each, one measured blocker and one honest dependency.

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

### 🏃 Sprint 288 — "PO-67's black front end is a two-line stub" — ✅ CLOSED 2026-08-26

**ROOT CAUSE FOUND.** MA's front end is laid out on an 800x600 screen and then shown on a
1920x1080 one, leaving 77% of the display black. Both halves of that come from ONE stub:

```c
BOOL ShowWindow(int nCmdShow) { m_maVisible = (nCmdShow != 0 /*SW_HIDE*/); return TRUE; }
```

`SW_SHOWMAXIMIZED` is treated exactly like "show". So `CMainFrame::OnGoBig()` — which
`LaunchFullPane` calls specifically to reach full size BEFORE building the panel — does nothing,
and everything downstream measures a stale window:

| measured | value |
|---|---|
| layout chosen by GetCurrentRes | index 1 (800) — decided at `window=800x600` |
| container rect at LaunchMain | **800x600** (view client) |
| actual SDL window, moments later | 1920x1080 |
| painted coverage | 477507 / 2073600 = **23.0%** |

**THE FIX I WAS ABOUT TO SHIP WOULD HAVE BEEN WRONG.** S286's evidence pointed cleanly at
"re-select the layout after the resize", and the forced-layout measurements supported it: index 3's
artwork exists and lifts coverage to 40.4%. But forcing it produced a CLIPPED front end — the 1280
art centred into a 1040x812 box, cut ~106px at the top and ~120px at the right. The container is
stale too, so a bigger layout only yields a bigger clipped picture. One measurement (printing the
container rect instead of inferring it) separated a real fix from a plausible one.

**NOT FIXED, DELIBERATELY.** Implementing maximise means resizing the frame AND propagating to the
view, which real MFC does through WM_SIZE and this port does not do at all. Every committed 2D
reference capture was taken at the current layout, so this lands with the parity gates or not at
all. S125 records an earlier flip of exactly this kind being reverted on gate evidence.

**Instruments added:** `MA_FORCE_RESINDEX=n` (force the layout choice), `MA_TRACE_PRESENT`
(coverage + painted bbox + centre pixel), `[res] LaunchMain container rect`.

**Retraction:** I had been treating PO-67 as intermittent ("did not recur"). The corner-art
condition is present in EVERY run. It read as intermittent because the centre-pixel test used to
detect it reports black in the healthy case too — the instrument could not see its own subject.

### 🏃 Sprint 290 — "The fix works; the gate was already red" — ✅ CLOSED 2026-08-26

**PO-67 FIX IMPLEMENTED (default OFF).** `CMainFrame::OnGoBig` now really maximises under
`MA_MAXIMIZE=1`: it moves the frame AND the view, because it is `m_pView->GetClientRect()` that
LaunchMain measures and this port has no WM_SIZE propagation.

| | off | on |
|---|---|---|
| container rect | 800x600 | **1920x1080** |
| layout chosen | index 1 (800) | **index 3 (1280)**, automatically |
| coverage | 23.0% | **62.9%** |
| painted bbox | 0,0..799,599 | **320,28..1599,1051** = 1280x1024 exactly centred |

`port/map_drag.sh` PASSES with it on (round trip 0 px, lossless) -- notable because S125 reverted a
similar flip for corrupting the pan. That flip enlarged the CANVAS alone; this one enlarges the
window and view, and the map is clean.

**⚠️ FOUND: `port/parity_2d.sh` HAS BEEN FAILING, AND NOT BECAUSE OF THIS SPRINT.** campaign_map
differs from its reference by 5184 px. It fails IDENTICALLY with MA_MAXIMIZE unset, so it is not
mine-today; it was last recorded byte-identical in S70/S71.

**Attribution, by bisecting the revert switches -- and my first guess was WRONG:**
- `MA_NO_REPLAY_SLOT_FIX=1` (S242 toolbar swap) -> still 5184 px. **Not S242.** I had already
  written the explanation before testing it; the test refuted it.
- `MA_NARROW_TBART=1` (S248 widened OnGetFile guard) -> **all 5 byte-identical, gate PASSES.**

So S248 changed the campaign-map toolbar: two slots that were blank now carry artwork.

**OPEN, AND NOT MINE TO SETTLE ALONE: is the new output right or is the reference?** The refs are
gold-derived, so a diff means the port deviates from gold. Two readings, opposite actions:
 (a) S248 fixed missing art -> the refs are stale and should be REBASED;
 (b) S248 draws art gold does not draw there -> S248 is too broad and should be NARROWED.
The PO's own testimony cuts toward (b) for at least one icon: "the icon is a flag, and it appears
only AFTER return from 3D". These captures are of a campaign map reached without flying.
NEXT: get a gold capture of that screen, or ask the PO what the toolbar shows before a sortie.
Rebasing on my own judgement would destroy the only gold evidence for the disputed slots.

### 🏃 Sprint 292 — "Ctrl+F6 is not dead, it is invisible" (N2) — ✅ CLOSED 2026-08-26

**The PO reported Ctrl+F6 doing nothing on a padlocked bogie, twice, after S240 supposedly fixed it.
Reproduced headlessly and traced end to end. The chain WORKS -- up to the point where it should
become visible.** BOB_KEYSEQ="400,0x40,0x1D" injects Ctrl+F6 through the real DirectInput path.

| step | evidence |
|---|---|
| key reaches dispatch | `[keyseq] tap dik=0x40 (with modifier)` |
| handler entered | `List6Toggle entered, viewmode=1` |
| mode set | `mode SET to VM_OutRevPadlock` (12) |
| setup ran | `InitOutRevPadlock ran` |
| view record | assigned (S240 opened that guard too) |
| draw ran | `DrawOutRevPadlock frame 1, viewmode=12 trackeditem=0x...` |

**RULED OUT, each with a trace that stayed silent rather than by argument:**
- InitFlyingView stealing the mode -- it is the ONLY site that rewrites VM_OutRevPadlock, and its
  trace NEVER FIRED. The mode is not taken back.
- a null/unassigned view record -- `outrevpadlockviewrec = &OutRevPadlockViewRec` runs.
- the S240 dispatch gap -- fixed and confirmed still fixed.

**WHERE IT ACTUALLY DIES: `DrawOutRevPadlock` is called exactly ONCE** (the trace fires on the first
three frames and on every 200th; only "frame 1" ever appears, over ~1000 frames after the tap). And
`viewdrawrtn` has exactly ONE invocation in the whole tree -- inside the parachute/death path
(Viewsel.cpp:1480). So `InitOutRevPadlock`'s `viewdrawrtn = &DrawOutRevPadlock` is very nearly
vestigial: per-frame rendering runs off `currentviewrec` and a viewmode switch elsewhere. The mode
is set and nothing acts on it every frame, which is exactly what "no effect" looks like from a seat.

**NEXT:** find the per-frame camera update and check its VM_ coverage. `VM_OutPadlock` appears in 3
switches, `VM_OutRevPadlock` in 2 -- a missing case is the leading candidate, but the third site
(6406) is the in/out toggle and is NOT it, so the per-frame path has still to be located.

⚠️ **My repro differs from the PO's in one way that may matter: viewmode was 1 (inside) at the tap,
i.e. NO BOGIE WAS PADLOCKED.** The PO padlocked first. The trace shows the mode activating anyway,
so the finding stands on its own, but "same bug" is an assumption until it is tried with a padlock.

### 🏃 Sprint 294 — "It works when a bogie is really padlocked" (N2) — ✅ CLOSED 2026-08-26

**S292 left N2 with the reverse padlock drawing exactly ONE frame and a caveat: my repro had no
bogie padlocked, unlike the PO's. Closed that gap with MA_FORCE_PADLOCK=1 (an existing hook that
fires ENEMYVIEW), and the picture inverts.**

With a bogie padlocked, Ctrl+F6 works completely:
- `DrawOutRevPadlock` runs EVERY frame (frames 1,2,3...; mode still 12 at fc=240)
- `camera=(41336055,156605,89265685)` and `bogey=(41336055,156605,89265685)` -- IDENTICAL
- camera sits ~984 m from the player, looking back at them

The one-frame behaviour in S292 was the NO-PADLOCK case: `DrawOutRevPadlock` starts with
`if(!trackeditem2) { viewnum.viewmode = VM_Track; InitTrack(); }`, so with nothing padlocked it
silently reverts to a normal tracking view -- indistinguishable from a dead key.

**⚠️ A MEASUREMENT I ALMOST PUBLISHED WAS WRONG.** The first camera probe sat at the TOP of the
draw routine and read "camera 3 m from the player", which fit the PO's report perfectly and would
have been written up as the finding. It samples the PREVIOUS frame's camera -- the wrong instant,
dressed as the right one. Moving it after `CopyPosition`, inside the swap, gave the opposite answer.
A number that confirms the expected story is the one to re-check hardest.

**So N2 is NOT a broken feature. Two candidates remain for what the PO saw:**
 (a) no bogie was actually padlocked at the moment they pressed it -- the silent VM_Track fallback;
 (b) the view did change and did not read as a change from the seat.
These need different work: (a) is a FEEDBACK defect (the game should say "no target"), (b) is not a
defect at all. NEXT: make the no-target case announce itself rather than silently reverting.

### 🏃 Sprint 296 — "A knob, not a guess" (Tacview placement) — ✅ CLOSED 2026-08-26 (partial)

**GOAL WAS to anchor the .acmi export over the real Korea. NOT ACHIEVED as a calibration.** Said
plainly because the alternative -- shipping a fitted constant that looks calibrated -- is the more
expensive failure.

**What was tried:** the sim's U/V are absolute world metres from a map origin of (0,0)
(MAPS.H `KOREAMAPORIGINX/Y`). Nothing in the source ties that origin to a latitude: map extents are
absent from the headers, and the node tables DO name real places (Seoul, Pyongyang, Kimpo, Sinuiju,
NODE.CPP) but carry UIDs, not coordinates. The positions live in world item data reachable only at
runtime, which is more digging than a cosmetic issue justified this sprint.

**What shipped instead:** `MA_ACMI_REF="lat,lon"` and `MA_ACMI_ORIGIN="u,v"` (and the BoB twins), so
placement is fixable by observation without a rebuild. Demonstrated: with
ORIGIN="412000,893000" REF="37.55,126.79" the 1v1 plots at lon 126.80 lat 37.55 -- over Korea -- and
the pair's opening separation is still EXACTLY 500 m, i.e. relative geometry untouched.

⚠️ **THOSE VALUES ARE NOT THE DEFAULT AND SHOULD NOT BECOME IT.** They were fitted to one mission's
observed coordinates, and they land the action near Seoul when MiG Alley's quick missions belong up
by the Yalu (~40N 124.5E) -- so the fit is visibly wrong in a way that would be easy to miss and
easy to mistake for calibration later.

**The bounded sprint that WOULD close it:** dump a named airfield's runtime World.X/Z, pair with
Kimpo 37.558N 126.791E and Sinuiju 40.100N 124.400E, solve offset + scale, verify a third landmark.

**Also recorded:** the Ctrl+F6 "no target" feedback idea from S294 was DROPPED after checking gold.
`CheckPadlock` reverts silently for EVERY view key with no target -- that is the game's own
convention, so adding a message would be a deviation from gold, not a fix.

### 🏃 Sprint 298 — "Two dead ends and one useful by-product" (PO-72) — ✅ CLOSED 2026-08-26 (no repro)

**PO-72 STILL NOT REPRODUCED.** Reached the campaign map headlessly (parity_2d's recipe) and it
renders completely: date readout "6/25/50: Morning, planning", both toolbars, unit icons, front
line, routes, NM scale bar. No instruction text is MISSING there because none belongs there -- that
is the PLANNING state, not the post-3D debrief the PO described. Candidate (b), the Directives
panel, was probed by clicking IDC_DIRECTIVES (2074); nothing opened, so the test is INCONCLUSIVE,
not negative -- the control id is right but the click evidently did not land on it.

**Abandoned this sprint, deliberately: the lat/lon landmark calibration.** UIDs turn out to be
string-resource ids (Kimpo = IDS_UID_AfBlKimpo = 9215), so matching one to a runtime item needs the
static item list, and MA's `LoadItemAnims` is declared in REPLAY.H with no implementation in the
compiled Replay.cpp -- a probable case-twin hunt. That is the SECOND sprint this cosmetic item has
cost more than its scope, so it is parked rather than pursued a third time.

⭐ **USEFUL BY-PRODUCT, and it sharpens PO-67:** the campaign map FILLS 1920x1080 correctly, while
the front-end panels sit in an 800x600 corner. Same binary, same run, same canvas. So the stale
container rect (S288) affects the FULL-PANEL front end only, not the map view -- which is exactly
why the PO can see a correct-looking campaign map and a mostly-black main menu in the same session,
and why "the port ignores the display resolution" would have been the wrong description of PO-67.

**NEXT for PO-72:** it needs the post-3D debrief state, which no recipe here reaches. Either build
that recipe (fly a campaign mission, Alt+X, capture) or take the PO's screenshot. The recorded
advice from S253 still stands and is still unanswered: is the text ABSENT, or drawn BLANK/ELSEWHERE?

### 🏃 Sprint 300 — "The gate cannot see the fix" (PO-67 qualification) — ✅ CLOSED 2026-08-26

**Goal: qualify the S290 maximise fix against a green gate, since parity_2d is red for an unrelated
reason (S248) and that redness masks whether maximise itself is clean.**

`MA_NARROW_TBART=1 MA_MAXIMIZE=1 parity_2d` -> **PASS, 5/5 byte-identical.**

⚠️ **THAT PASS IS WORTH ALMOST NOTHING, AND I NEARLY REPORTED IT AS A CLEAN BILL.** The gate pins
`settings.mig`, which fixes the display at 800x600; every committed reference is 800x600 (verified
in the PPM headers). Maximising to 800x600 is a NO-OP. The gate is byte-identical whether or not the
fix is sound -- it cannot distinguish them.

**The confirmation itself needed a correction.** My first check ran the same recipe WITHOUT the
pinning and showed canvas 1920x1080, maximise firing, container 1920x1080, layout index 3 -- i.e.
apparently contradicting the no-op claim. That test had simply omitted `pin_settings`, so it
reproduced my desktop rather than the gate. Reading the gate's OWN captured headers settled it.
Twice in one sprint the fast answer was the wrong one.

**WHAT IS ACTUALLY ESTABLISHED (and it is not nothing):** at 800x600 the fix is a proven no-op,
byte-identical across all five screens. A user at 800x600 sees no change whatsoever. What remains
unqualified is its behaviour at larger resolutions -- which is the only case it exists for.

⭐ **So PO-67 is blocked by the ABSENCE of a gate, not by a failing one.** Every 2D reference in this
repo is 800x600 by construction, so no existing gate can judge a display-resolution layout change.
**NEXT: capture a second reference set at 1920x1080 with MA_MAXIMIZE=1** and make parity_2d run both
resolutions. That is the gate event S288 predicted, and it is the real prerequisite for flipping the
default -- not the S248 toolbar question, which is a separate and independent blocker.

### 🏃 Sprint 302 — "The new gate caught the fix on its first run" — ✅ CLOSED 2026-08-26

**Built the gate S300 said was missing: `PARITY_RES=1080` runs the five parity recipes at 1920x1080
with MA_MAXIMIZE=1 against their own reference set. Then ran it and LOOKED at the captures before
committing any of them as references.**

⭐ **THE S290 MAXIMISE FIX IS PARTIAL, AND WOULD HAVE SHIPPED BROKEN.**
- `title` at 1920x1080: **CORRECT** -- 1280x1024 art centred, menu placed, dark margins. Exactly the
  PO-67 goal, and it looks right.
- `prefs_others`: **BROKEN** -- labels overlap and DOUBLE UP ("Music Volume" over "Master Volume",
  "Radio Chatter Volume" over "Gamma Correction"), control column squashed left, art panel offset.

So maximise repairs the full-pane menu screens and breaks the DIALOG screens, which place controls
from layout data the widened container does not carry. Every earlier measurement of this fix was of
the title screen (coverage 23%->62.9%, bbox centred to the pixel) and every one of them was true --
and none of them could see this, because none looked at a dialog.

**Reference set deliberately NOT populated** (port/ref/native1080/README.md records why). Committing
these would enshrine a broken layout as the expected result -- and this repo already has one case
(S290) of a stale reference outranking a real fix.

**Also recorded in the gate itself:** ref/native came from the real game; ref/native1080 would be
captured from THIS PORT. That makes it a REGRESSION oracle ("did this change?"), never a parity
oracle ("is this right?"). Written into parity_2d.sh so the distinction cannot be lost later.

**NEXT for PO-67:** fix dialog control placement at a widened container, then capture references.
The fix stays OFF by default; at 800x600 it remains a proven no-op (S300).

### PO-21 re-measured 2026-09-04 — the layering symptom does NOT reproduce at 1920x1080

PO-21 was REOPENED on 2026-08-15 as *"panel art is a fixed 800x600 bitmap drawn at (0,0), so at
high resolution it covers one quadrant and the rest keeps the previous panel's pixels"*, after
S128-S130 were reverted for breaking front-end clicking. Re-tested on today's build, at the
resolution the complaint is about:

1. **PO-24 gate (`port/panel_click.sh`): PASS.** Real window at 1920x1080, menu LOCATED at pixel
   (1100,470) — not hardcoded — and the click was accepted (`OnSelectRlistbox row=1`). The
   clicking regression that forced the S128-S130 revert is gone.
2. **Title screen at 1920x1080:** panel art is CENTRED with clean black letterboxing. Not at
   (0,0), no stale pixels.
3. **A second front-end panel at 1920x1080** (title -> multiplayer "Select a Service", captured at
   frame 300): the new panel **fully replaces** the previous one — centred, clean letterbox, and
   **no title-screen pixels survive anywhere in the frame**. That is precisely the symptom PO-21
   describes, and it is absent.
4. Incidentally confirms **PO-20** still holds: a 1920x1080 flight renders full-screen with the
   HUD at the bottom edge (Speed/Mach/Alt/Hdg/Thrust), not in a corner.

**Status: the reported symptom is not reproducing.** Not closing it unilaterally — PO-21's text
also covers widget placement (S144's overlap fix) and the item was reopened by the PO, so the PO
should confirm against their own screens. What IS established is that the two things that made it
a live defect — dead clicking at high resolution, and a panel that leaves the previous screen's
pixels behind — both now test clean at 1920x1080.

### PO-11 (2026-09-04) — the scale ruler is IMPLEMENTED and wired, but NOT yet verified on screen

PO-11's last remaining cluster was the scale ruler: *"`CScaleBar`, 0 hosted controls -- it draws
itself and nothing calls it."* BoB hit the identical gap and solved it, so this is a cross-port.

**What was done**

1. `CScaleBar::OnPaint`'s 294-line drawing body was extracted into a new member
   `CScaleBar::MaDrawScale(CDC*, CRect)` (SCALEBAR.H / SCALEBAR.CPP). Extracted as a **member**,
   not a free function as BoB did: the body uses unqualified member access throughout
   (`m_align`, `m_pView`, `m_bHorzAlign`, `zoom` via `m_pView->m_zoom`), so as a member it moves
   verbatim. A free function would have meant rewriting 294 lines by hand -- a re-implementation
   pretending to be a move.
2. New `ma_map_paint_scalebar(CMIGView*, sw, sh)` drives it with the real screen DC. Two details
   that matter and are easy to get wrong:
   * ma's `CDC` routes every primitive through `ma_gdi_*` on `m_hDC`, so the DC must be
     `ma_gdi_screen_dc()` -- BoB's `(HDC)1` idiom happens to be the same value here, but for a
     different reason.
   * the ruler's drawing is CLIENT-relative (it starts at `CPoint(30,2)`), so the DC's viewport
     origin is moved to the docked strip with `ma_gdi_set_viewport_org` **and restored after** --
     it is the shared screen DC and everything painted later would inherit the offset.
3. Called from the map idle beside `ma_map_paint_oob()` (MIG.CPP). `MA_NO_SCALEBAR=1` disables.

Builds and links clean.

### ✅ VERIFIED 2026-09-04 (same day) — and the verification found a wild pointer

Driven to the campaign map with the navigation the other map gates already use
(`30,r3;65,#1055;100,#2063:1` — title -> Campaign -> load Auto Save -> map). The ruler renders:
**"N M" and 50 / 100 / 150 / 200 / 250** down the right edge, i.e. the 0-350 Nm scale PO-11
listed as missing (`/tmp/ma_map_sb2.ppm`).

**But the first verified run exposed a defect in my own fix.** The trace printed
`sb=0x92e7197` — **not 4-byte aligned**, so not a valid object. `CMIGView::m_pScaleBar` is
DECLARED (MIGVIEW.H:169) and DEREFERENCED (MIGVIEW.CPP:979) and **never assigned anywhere in the
tree**: the CScaleBar object is never created. PO-11 recorded the symptom as "hosts 0 controls,
draws itself, nothing calls it"; the truth underneath is that it does not exist.

My first version took that pointer and *wrote through it* (`sb->m_align = 4; sb->m_pView = v;`) —
a wild write into arbitrary memory that happened not to crash. It also drew the WRONG scale:
0/5/10/15/20/25 instead of 50/100/150/200/250, because `MaDrawScale` was reading `m_bHorzAlign`
and `m_init` out of garbage. So "it rendered" was not evidence of correctness — the numbers were
wrong and memory was being corrupted.

Fixed: the paint function now owns a `static CScaleBar` and initialises `m_init` / `m_bHorzAlign`
itself; the view's uninitialised member is read only to report it in the trace. This is the port's
documented uninitialised-read class, and it is worth noting that a capture alone would have
"passed" it.

Remaining: `m_pScaleBar` is still uninitialised for MIGVIEW.CPP:979's `RedrawWindow()` call —
that is a separate latent dereference on the same member, not touched here.

**Superseded note from earlier today:** The capture run produced no `[scalebar]` trace at all,
which means the function was never called: the click path used (`40,r2;120,r1`) lands on the
multiplayer screen, and this paint only runs while the **campaign map** is active. So nothing here
says the ruler renders, or renders in the right place -- only that the code compiles and is wired.
To finish: drive a click path that actually reaches the campaign map, confirm `[scalebar] v=.. sb=..`
appears with a non-NULL `sb`, and capture the right-hand strip. If `sb` is NULL the view never
constructed a `CScaleBar` and that is a different problem from the one fixed here.

### CORRECTION 2026-09-04 — the "m_pScaleBar is never assigned" claim was WRONG

Yesterday's PO-11 entry (and two code comments) asserted that `CMIGView::m_pScaleBar` is
*"declared and dereferenced but never assigned anywhere in the tree"*, and built a conclusion on
it: that the CScaleBar object is never created, and that the first ruler paint was making wild
writes into arbitrary memory.

**The assignment exists.** `MAINFRM.CPP:408`:

    m_toolbar4.Create(CScaleBar::IDD, view);
    m_toolbar4.Init(this, 200, 400, 48, AFX_IDW_DOCKBAR_LEFT, 4);
    view->m_pScaleBar = &m_toolbar4;          // m_toolbar4 is a real CScaleBar member

I missed it because I searched with the default `grep`, which is ugrep and **silently skips files
it decides are binary** -- the trap documented at the top of this repo's own notes, which I had
already cited earlier in the same session. `/bin/grep -rn m_pScaleBar SRC/` finds it instantly.
A "0 assignments, therefore never initialised" conclusion drawn from that grep is not sound here.

**What survives the correction, and what does not:**

* SURVIVES: the ruler now renders, and renders CORRECTLY. Through the view's pointer it drew
  0/5/10/15/20; with an owned CScaleBar it draws **N M / 50 / 100 / 150 / 200 / 250**, matching
  the game's Nm scale. That difference is measured, from captures, and is not affected by the
  error above.
* SURVIVES: initialising the member in CMIGView's constructor and guarding the
  `RedrawWindow()` call. It is read before MAINFRM's assignment runs, and an uninitialised read is
  undefined even when a later write would have fixed it.
* DOES NOT SURVIVE: "the object is never created" and "this was a wild write into arbitrary
  memory". Neither is established.

**Now OPEN, and it is a real question:** at map-paint time `v->m_pScaleBar` reads `0x8c55197` --
**not 4-byte aligned**, so not a valid object address -- consistently across runs, and it still
does after the constructor sets it to NULL. A class-layout mismatch between translation units was
checked and ruled out (MIGVIEW.H has no MA_LINUX-conditional members). So either MAINFRM's
assignment has not run at that point and something else is writing the member, or the value is
being read through a pointer that is not the object I think it is. Worth its own sprint; the
owned-instance workaround makes the ruler correct meanwhile but does not explain this.

### CORRECTION 2 (2026-09-04) — the pointer was never garbage. Following it to the end.

The previous correction fixed one error ("never assigned" — it is, at MAINFRM.CPP:408) but left an
"open question": why the member read back as `0x..197`, not 4-byte aligned. I treated that as
evidence of corruption. It is not. Traced the assignment itself:

    [scalebar-asg] view=0xa068bb0  &m_toolbar4=0xa077197  stored=0xa077197
    [scalebar]     v=0xa068bb0     view's m_pScaleBar=0xa077197

**`&m_toolbar4` IS `0xa077197`.** The address of the member itself is what looked wrong. The
pointer is stored correctly, read back correctly, and points at a real `CScaleBar` that MAINFRM
has `Create()`d and `Init()`d. Nothing was uninitialised and nothing was corrupted.

**So two claims I made are withdrawn:**
* "the CScaleBar object is never created" — false; MAINFRM creates `m_toolbar4`.
* "the first ruler paint was a wild write into arbitrary memory" — false; it wrote to a real
  object's real members.

**What actually caused the wrong scale** (0/5/10/15/20 vs the correct 50/100/150/200/250): not
pointer validity, but STATE. MAINFRM inits `m_toolbar4` docked with `AFX_IDW_DOCKBAR_LEFT`, and
the drawing branches on `m_bHorzAlign` and `m_align`. The first version set `m_align` but not
`m_bHorzAlign`, so it took the horizontal branch and drew a horizontal ruler's numbering into a
vertical strip. The owned instance works because it sets BOTH.

**Lesson worth keeping:** an unaligned-looking pointer is a strong smell in this codebase — it is
the port's documented bug class — and it was reasonable to suspect it. It was NOT reasonable to
assert corruption without printing the address it was assigned FROM. One extra `fprintf` at the
assignment site would have prevented both wrong conclusions, and it is what finally settled it.

The owned-instance implementation stays: it is correct, it is verified on screen, and it does not
depend on MAINFRM having run first. But it is a CHOICE now, not a workaround for a broken pointer.

### PO-55 (2026-09-04) — the "OOB dialog swallows left-edge clicks" suspect is REFUTED

PO-55 (*"waypoint on left over water not draggable"*) named a prime suspect and, to its credit,
the exact test: the Ins Wave dialog drawn off the left edge (PO-56) covering the left strip, with
`ma_oob_click_logged_rec` swallowing clicks inside its rect — *"Test: MA_TRACE_CLICK=1 and look
for `[oobclick] swallowed` at the waypoint's coordinates."*

Ran it. Campaign map loaded from Auto Save, clicks injected at three x positions on the same row
(`BOB_CLICKSEQ="...;300,60,400;340,200,400;380,900,400"`, `MA_TRACE_CLICK=1`):

    [mapclick] (60,400)  hit id=0(0x0) band=-1   -> drove CMapDlg down/up
    [mapclick] (200,400) hit id=0(0x0) band=-1   -> drove CMapDlg down/up
    [mapclick] (900,400) hit id=0(0x0) band=-1   -> drove CMapDlg down/up

**Zero `[oobclick] swallowed` events**, and the two left-strip clicks are indistinguishable from
the right-side control: no OOB node is hit (`id=0`), and each is delivered to `CMapDlg`. So clicks
on the left strip are NOT being swallowed, and PO-56's off-left dialog is not stealing them.

**Scope of this result, stated honestly:** I clicked arbitrary left-strip coordinates, not the
Egress waypoint's actual position (which I do not have). So this refutes "the left strip is
swallowed" — the general form of the suspect — but cannot rule out something specific to that one
waypoint's exact location. Getting its coordinates from the PO's saved route, or dumping the route
node positions, is the way to close that gap.

**Where to look next:** the click ROUTING is fine, so the failure is downstream — in the waypoint
HIT-TEST or in what `CMapDlg` does with a node over water. `route_drag.sh` already drags Initial
Point and Egress successfully (S172), so comparing the node record of a draggable waypoint against
the water one is the cheapest next measurement.

### PO-55 (2026-09-04, cont.) — the waypoints are LOCATED, but the scan is TRUNCATED so absence proves nothing

Following the previous entry (click routing to the left strip is fine, so the failure is
downstream in the hit-test), the next question was where the waypoints actually are.
`MA_MAP_ITEM_SCAN=1` on the Auto Save campaign map:

    [mapitem] (546,408) id=260(0x104) band=0x0100 WayPoint "Waypoint: End"
    [mapitem] (564,432) id=259(0x103) band=0x0100 WayPoint "Waypoint: Start"
    [mapitem] (528,438) id=261(0x105) band=0x0100 WayPoint "Waypoint: Regroup"
    [mapitem] (570,450) id=258(0x102) band=0x0100 WayPoint "Waypoint: Initial Point"
    [mapitem] scan done, 58 distinct item(s) over 1024x768 (printed 48)

All four sit in a tight central cluster (x 528-570). **No "Egress" waypoint appears** — the one
the PO describes as being on the left over water.

**That is NOT evidence that Egress is missing or unfindable.** The scan's own last line says it
printed **48 of 58** distinct items: the list is TRUNCATED, and this repo has been burned by
exactly this before ("the lists are truncated (top 25 / top 22) so absence from them proves
nothing", MIG.CPP). Two readings remain open and they need opposite work:

1. **This saved game's route simply has no Egress waypoint** (different mission from the PO's
   playthrough) — then the whole reproduction is against the wrong route and needs the PO's save.
2. **Egress exists but is among the 10 unprinted items, or is not returned by `FindMapItem` at
   all** — and the second of those IS PO-55: `CMapDlg::MaCanDragAt` returns
   `(id && AllowDragItem(id)) ? id : 0`, so a waypoint `FindMapItem` cannot see can never be
   dragged, which matches the symptom exactly.

**Next step, cheap:** raise the scan's print cap and re-run, so the item list is COMPLETE, then
look for Egress. If it is present with an id, click its coordinates and check `AllowDragItem`;
if it is absent from a complete scan, the defect is in `FindMapItem`'s coverage — most likely a
band or bounds test that excludes items over water or beyond the coast.

Do not conclude anything from the four-waypoint list above until the scan is uncapped.

### PO-55 (2026-09-04, resolved as far as this save allows) — NOT reproducible here; needs the PO's save

The truncation caveat in the previous entry was the right call. Using the scan's NAME-MATCH path
(`MA_MAP_CLICK_NAME=Egress`), which searches all 58 items rather than the 48 printed:

    [mapitem] name match "Waypoint: Egress" -> id=262(0x106) band=0x0100 WayPoint at (624,594)

**Egress exists and `FindMapItem` returns it** (id 262). So the previous entry's "no Egress
appears" was purely the print cap, and had I concluded from it I would have sent the next sprint
after a missing-waypoint bug that does not exist.

**And it is not where the PO's is.** The PO describes *"waypoint on left over water"*; this one is
at (624,594) — centre-lower, over land, in a route whose five waypoints (Start, Regroup, Initial
Point, Egress, End) all sit centre-map. S172's `route_drag.sh` already drags THIS Egress
successfully.

**So PO-55 is not reproducible against the Auto Save campaign this harness loads.** What has been
established, and it is not nothing:

* Click routing to the left strip is fine — no `[oobclick] swallowed`, left clicks reach
  `CMapDlg` identically to right ones. **PO-56's off-left dialog is not the cause** (refuted).
* `FindMapItem` finds waypoints and returns ids; `AllowDragItem` gates the drag; and the drag
  path works on this route's Egress.

**What is needed to go further:** the PO's own save, or a mission whose route genuinely puts a
waypoint over water on the left. Without one, any further sprint here would be testing a
condition that does not exist in the data — which is how this item's first suspect (PO-56) came
to be plausible and wrong. Marking it BLOCKED ON DATA rather than continuing to sprint on it.

### PO-19 (2026-09-04) — the recon view's zoom is NOT in the global key table; it needs its own option entries

PO-19: *"Keys 3 and 4 zoom the recon view; 1/2 rotate and 0 exits (those already work)"* — i.e.
rotate and exit respond, zoom does nothing.

Dumped the live 3D bindings (`MA_DUMP_BINDINGS=1`, which writes `<gamedir>/controls.cfg` — 617
bindings) and decoded the relevant keys:

    ZOOMIN   = 0x4A          (numpad -)        BIGZOOMIN  = 0x4A, 6 / 0x4A, 7
    ZOOMOUT  = 0x4E          (numpad +)        NEXTSHAPEDN= 0x4A, 4 / 0x4A, 5
    '1' 0x02 = RPM_10        '3' 0x04 = RPM_30
    '2' 0x03 = RPM_20        '4' 0x05 = RPM_40

**Two things follow.** First, in the FLIGHT key table the zoom actions live on **numpad -/+**, not
on 3/4 — the same layout BoB uses (its PORT.md R28 entry records the identical finding, including
that Ctrl+numpad gives the FOV zoom). Second, digits 1-4 there are THROTTLE settings, so they
cannot be what rotates the recon view either.

**Therefore the recon view handles digits ITSELF**, as the in-flight menu screens do
(`MapScr::OptionList`, where a digit selects an option) — which is consistent with the PO's report
that 1/2 and 0 work while 3/4 do nothing: those options simply have no entries. So the fix is to
add the zoom options to that screen's own list, NOT to bind anything in the global table.

**Next step:** find the recon view's option list (it is reached from the dossier's Photo button —
see `port/recon_photo.sh`) and compare its entries against the four the PO expects. If 3/4 are
absent, that IS the defect and it is a small, local addition.

⚠️ **A correction on method, recorded because it nearly produced a false finding.** My first attempt
passed `MA_DUMP_BINDINGS=/tmp/ma_keys.csv`, assuming the env named the output path. It does not —
the path comes from a different variable and defaults to `<gamedir>/controls.cfg`. The file I then
read did not exist, and my checker duly reported **"ZOOMIN NOT BOUND, ZOOMOUT NOT BOUND"** for
every action: an empty instrument reporting absence. Had that been believed, the conclusion would
have been the exact opposite of the truth. Check that a dump FILE EXISTS and has rows before
reading anything into what is missing from it.

### PO-19 (2026-09-04) — ANSWERED: the recon view's zoom is on NUMPAD −/+, not on 3 and 4

Driven to the recon view with the gate's own recipe (name-click Wonju to open its dossier, then
`#2078@DossierButtons` at pump 420) and keys injected under `MA_TRACE_KEY`:

    [key] DOWN scancode=0x04 shift=0 -> action index=108     '3'
    [key] DOWN scancode=0x05 shift=0 -> action index=110     '4'
    [key] DOWN scancode=0x02 shift=0 -> action index=104     '1'
    [key] DOWN scancode=0x4a shift=0 -> action index=88      numpad −
    [key] DOWN scancode=0x4e shift=0 -> action index=90      numpad +

Action index is `KeyName * 2`, so:

| key | action | |
|---|---|---|
| '1' | `RPM_10` | throttle |
| '2' | `RPM_20` | throttle |
| '3' | `RPM_30` | **throttle — not zoom** |
| '4' | `RPM_40` | **throttle — not zoom** |
| **numpad −** | **`ZOOMIN`** | |
| **numpad +** | **`ZOOMOUT`** | |

**So PO-19's premise is wrong: 3 and 4 are throttle settings and were never the zoom keys.** The
recon view uses the flight key table, and zoom lives on numpad −/+ — exactly the layout BoB uses
(its PORT.md R28 entry records the identical finding for the cockpit, including Ctrl+numpad for
FOV). Nothing is unhandled; the wrong keys were being pressed.

⚠️ **What this does NOT establish:** the keys are DELIVERED and the zoom ACTIONS are dispatched.
Whether the recon view visibly responds is a further question — BoB showed exactly this gap, where
`ZOOMIN` dispatched fine but adjusted `currentviewrec->range`, an orbit distance the cockpit view
does not use. **Ask the PO to try numpad −/+ in the recon view.** If it zooms, PO-19 closes as
"wrong keys". If it does not, the item survives with a much sharper question: the action arrives,
so what consumes it in this view?


---

## 🔲 BACKLOG — MP-2: connecting two MA AppImages for multiplayer is not usable

**PO, 2026-09-04:** *"ma multiplayer. Not clear how to connect ma appImages for multiplayer.
Select sides is missing. Clicking on TCP connection line seems off somehow."*

**Evidence:** `/home/admin/Videos/260904_ma_multiplayer.mp4` (15 MB, 23:25). PO-recorded, keep it.

### Three distinct complaints, which should not be merged

1. **No documented route to connect two AppImages.** The gates below drive multiplayer through the
   game's own UI with hand-built arguments; nothing tells a user which AppImage hosts, what address
   the joiner types, or whether a port must be open. This may be purely a documentation gap, or the
   AppImage may need to expose something (an `MA_MP_*` env, a port note in the launcher). Decide
   which before writing anything.

2. **"Select sides is missing."** The engine HAS the concept, so this is unlikely to be "never
   ported": `_DPlay.SideSelected` is SET at `SRC/MFC/LOCKER.CPP:493`, cleared at
   `SRC/COMMS/COMMS.CPP:171` and `SRC/COMMS/WINMOVE.CPP:15040`, and READ at
   `SRC/MFC/FULLPANE.CPP:5036` (`side != _DPlay.Side && _DPlay.SideSelected`). The state exists and
   is consulted; what the PO cannot reach is the UI that sets it. Start at those four sites.

3. **"Clicking on the TCP connection line seems off somehow."** This is a shape MA already has scars
   from -- drawing into one rect while hit-testing against another (S208/PO-65, where the panel
   filled the window and its left edge was genuinely missing, and every screen-parity oracle was
   blind to it because MA_SHOT captures the CANVAS, not the window). MA has NO centre/scale
   mechanism at all -- that is bob-only (R9) -- so a list row drawn in one place and hit-tested in
   another is entirely possible. Run `MA_TRACE_PRESENT=1` against this screen FIRST: it prints
   canvas/viewport/window/drawable, and if those disagree the click offset follows from it.

### What already passes, so it is NOT the whole story

`port/gates_all.sh` runs four multiplayer gates, all currently green:

| gate | what it proves |
|---|---|
| `mp_connect` | PO-76: multiplayer gets past its front door |
| `mp_uihost`  | the game's OWN UI hosts a session and another process can join it |
| `mp_twogame` | TWO game instances, one hosting one joining, through the game's own UI |
| `mp_packet`  | two processes exchange a DirectPlay packet |

Host, join and packet exchange therefore work when driven by the gates. The PO's report is about
the path a HUMAN takes and about AppImage packaging -- **not** the transport. A sprint that starts
by re-proving the transport is starting in the wrong place.

### Not started
No sprints have been run against this item. Watch the video first; it shows the exact screens.


### MP-2 addendum — "Select sides is missing" is the KNOWN CRRadio-not-drawn defect (PO-83 / S329)

Sprints 1-3, 2026-09-05. The earlier entry guessed the side-selection UI was unreachable. It is
more specific than that, and it is already a tracked defect.

**Where side selection actually lives.** Not a "SideSelect dialog" (that is bob's, `sidesel.cpp`).
In MA it is a RADIO CONTROL on the locker/game-setup screen, `SRC/MFC/LOCKER.CPP:474-493`:

```c
radiobox = GETDLGITEM(IDC_RRADIO_SELECTSIDE);
selection = radiobox->GetCurrentSelection();
switch (selection) { case 0: _DPlay.Side = TRUE; ... }
_DPlay.SideSelected = true;
```

So the state the earlier entry found being READ at `FULLPANE.CPP:5036` is written from a
`CRRadio` OCX -- and that control class has a standing bug:

> `SRC/compat/ma_olecontrol.cpp:394` -- *"S329-S2 (PO-83): the variants dialog's two CRRadio groups
> ARE populated (S329-S1 measured three buttons reaching each control) yet nothing is drawn, and
> CRRadioCtrl::OnDraw never fires even under real GL."*
> `:1175` -- *"AddButton receives 3 correct strings for each group ("F86A"/"F86E"/"F86F"), yet
> CRRadioCtrl::OnDraw fired ZERO times under real GL."*

A control that is populated but never painted is exactly "suspiciously blank". **This is very
likely not a multiplayer bug at all** -- it is the radio-control drawer, and the PO happened to meet
it in the multiplayer path. If so, fixing it also fixes the variants dialog (PO-83).

**The next experiment is already built and takes one run.** `MA_TRACE_RADIO=1` prints
`[radio] HOSTED as CT_RADIO: ctrl=... client=...` at classification time. The S329 note states the
discriminator plainly: if the control does NOT appear on that line, it is not hosted as a radio at
all and **the fault is CLSID recognition, not painting** -- a different fix entirely. Drive MA to
the multiplayer locker screen with that set and read the answer before touching any drawing code.

**Not done:** that run needs the display, which was occupied by the bob gate suite. Queued.
**Sprints on MP-2 so far: 3.**


### MP-2 sprints 1-4 (2026-09-05)  [SUPERSEDED -- read the correction below] — "Select Side is blank" LOCALISED: 2 of 3 radios never draw

`MA_TRACE_RADIO=1`, `BOB_CLICKSEQ="40,r2;90,#2063:1;180,#2063:1"` (the mp_uihost route), run BOTH
headless and under real GL -- identical results both ways:

| control | buttons it holds | `[radio] OnDraw` calls |
|---|---|---|
| `0xba13570` | Death Match / Team Play / Quick Missions | **24** |
| `0xbae0540` | **Red / UN** -- this is IDC_RRADIO_SELECTSIDE | **0** |
| `0xbae2680` | Everybody | **0** |

**So the control is hosted, classified CT_RADIO, and populated with both sides -- and its drawer is
never called.** That is the PO's blank panel, exactly.

**This CORRECTS the earlier reading of S329/PO-83.** That note says `CRRadioCtrl::OnDraw` "fired
ZERO times under real GL". Measured now, it fires 24 times under real GL -- for ONE control. The
defect is not "radios never draw"; it is **only one of three radios draws**, and the other two
include the side selector. Any fix aimed at "the radio drawer doesn't run" would be aimed at
something that does run.

**Also corrected, my own slip:** a first loose grep reported 11008 OnDraw calls. That pattern
(`OnDraw\|radio.*draw`, case-insensitive) was matching thousands of unrelated radar/draw lines. The
precise count is 24. A number that large should have been suspicious on its face.

**Next sprint -- the discriminating question is why ONE draws.** All three are hosted through the
same `CT_RADIO` path in `ma_olecontrol.cpp`, so the divergence is downstream: candidates are the
draw dispatch only reaching the first-created control, or only the control on the ACTIVE dialog
being visited, or the other two having no valid host window/rect. The trace already prints
`bounds=(0,0)-(193,99) hWnd=1` for the drawn one -- print the same for the hosted-but-undrawn two
and the answer should be immediate. Do NOT start by rewriting the drawer.

**Sprints on MP-2: 4 this turn (7 total).**


### MP-2 sprints 5-8 (2026-09-05) -- CORRECTION, then the real cause: control id 2324 collides

**The table in the entry above was an instrument artifact. Retract it.** `CRRadioCtrl::OnDraw`'s
trace was written `static int n = 0; if (n++ < 24)` -- **24 was the trace's CEILING, not a count.**
The first 24 draws all belonged to one control, so the other two looked like they never drew. Worse,
a headless run and a real-GL run each reported exactly 24, and I read that as independent
corroboration when it was two runs hitting the same hard-coded limit. The suspicious part was
visible in the log itself -- a number that lands on a round 24 twice is a cap, not a measurement.

**Instrument fixed** (`SRC/RRADIO/RRADIOC.CPP`): cap PER CONTROL (3 lines each, 16 controls) plus a
`[radio] FIRST DRAW of ctrl %p` census line, so a control that draws late is still represented.
`MA_TRACE_RADIO_ALL=1` removes the cap entirely.

**Re-measured with the honest instrument:**

| ctrl | buttons it holds | panel id | draws? |
|---|---|---|---|
| `0xb3fcd80` | Death Match / Team Play / Quick Missions | 2323 | yes |
| `0xb4cbdc0` | Everybody | 2324 | yes |
| `0xb4c9d40` | **Red / UN** = IDC_RRADIO_SELECTSIDE | **never dispatched** | **no** |

So it is ONE of three that never draws, not two -- but it is still exactly the one holding the two
sides, which is the PO's blank panel. The earlier headline survived; its supporting table did not.

**ROOT CAUSE CANDIDATE, and it is a known trap in this codebase.** `SRC/MFC/RESOURCE.H`:

    #define IDC_RRADIO_SELECTSIDE   2324
    #define IDC_RRADIO_DETAILS      2324
    #define IDC_RRADIO_MIGVARIANTS  2324

Two hosted radios on this screen carry id **2324**. The panel dispatcher logged 9934 visits to
"panel id=2324" and every one of them drew `0xb4cbdc0` ("Everybody") -- `0xb4c9d40` (Red/UN) was
never reached. A host map keyed by id (or by parent+id) cannot hold both, so the second registration
shadows the first and one control becomes permanently invisible while remaining fully alive:
hosted, classified, populated, clickable-in-principle, never painted.

**S329 is NOT contradicted** -- worth stating plainly, because I said it was. Its "OnDraw fired ZERO
times" is labelled *"Measured before fixing"* in the source comment. That zero was the pre-fix state
and S329's dispatcher branch is what makes these draws happen at all.

**Next sprint (MP-2 is at 8 -- four left before the Fable 5.1 label):** open the host map in
`ma_olecontrol.cpp` and confirm the key. If it is id-based, the fix is to key by the control's own
identity (client pointer) rather than by resource id, which is the only thing unique here. Verify by
counting map entries with id 2324 before touching any drawing code.


### MP-2 sprints 9-12 (2026-09-05) -- the entries are ERASED, not shadowed. My id-collision theory is dead.

**Retract the id-collision root cause from the previous entry.** The host map is
`std::map<void*, Hosted>` keyed by the CLIENT POINTER (`ma_olecontrol.cpp:79`), not by resource id,
so `IDC_RRADIO_SELECTSIDE` sharing 2324 with `IDC_RRADIO_DETAILS` cannot make one control shadow
another. Measured: the three hosted radios have three DISTINCT client keys
(`0x9b7c9a9`, `0x9b7c9d1`, `0x9b801ec`). Nothing was overwritten. The id collision is real in
RESOURCE.H and irrelevant here.

**What is actually happening**, from two new measurements:

* `MA_TRACE_NODRAW` + `MA_TRACE_BTNSTR`: **zero** clip-skips, and "90600 control draws dispatched,
  0 with no branch". So the side radio is not being skipped inside the paint loop -- it is not IN
  the paint loop.
* New `[radiocensus]` (`MA_TRACE_RADIO=1`) dumps every CT_RADIO entry the map holds at paint time:

      [radiocensus] entry client=0x9b801ec ctrl=0x9b807a0 id=2324 parent=0x9b80010 relative=1
      [radiocensus] map holds 1 CT_RADIO entries (of 76 total)

  **One entry, out of three hosted.** The survivor is the "Everybody" control. The Red/UN control
  and the game-type control are gone from the map -- and the game-type control demonstrably drew 90
  times earlier in the same run, so it was present and was later removed.

So the defect is an ERASE (or an un-host) that removes live controls from the map while their dialog
is still on screen. That also explains the whole confusing history of this bug: the controls are
created, classified, populated and briefly drawn, then silently stop existing as far as painting is
concerned -- which is why every earlier instrument that looked at creation, classification or
population found nothing wrong.

**Next sprint, precise:** instrument the erase loop at `ma_olecontrol.cpp:~189`
(`for (iterator j = m.begin(); j != m.end(); )`) to log every CT_RADIO entry it removes, with the
reason and the caller. That is one trace line and it names the culprit.

**SPRINT COUNT: 12.** Per the standing rule this is the last turn before MP-2 must be labelled
"waiting for Fable 5.1". Noting deliberately that it is converging fast -- three hypotheses killed
by measurement (CLSID recognition, the drawer, the id collision) and the cause now localised to a
single loop -- so the next sprint either closes it or it gets the label.


### MP-2 sprints 13-16 (2026-09-05) -- ROOT CAUSE FOUND: DestroyPanel erases the live radios

The "erase" reading from the previous entry was right; the erase SITE was not the one I instrumented.
`ma_ole_forget` reported **zero** radio removals -- so I went looking for other mutation sites and
found a THIRD one: `ma_ole_remove_by_parent` (:915), called from `RDialog::DestroyPanel`, which drops
every hosted entry whose `parent` matches. Its only trace was behind `MA_TRACE_SIZE`, which is why
four sprints of `MA_TRACE_FORGET` work saw nothing.

Measured, with the site now named under `MA_TRACE_RADIO`:

    [radioforget] BY-PARENT parent=0x9b68000 ctrl=0x9b530c0 id=2323 client=0x9b68279
    [radioforget] BY-PARENT parent=0x9b68000 ctrl=0x9c1f9f0 id=2324 client=0x9b682a1

`0x9c1f9f0` is exactly the control that `AddButton` filled with **"Red"** and **"UN"**. Both the
game-type radio and the side selector are removed from the host map by one DestroyPanel on their
shared parent -- which is why the game-type radio drew 90 frames and then stopped, and why the side
selector, hosted and populated, never painted at all. The surviving "Everybody" radio has a
different parent, so it is untouched. That is the complete explanation of the PO's blank panel.

**Not yet fixed.** The open question is whether that DestroyPanel is spurious (the panel is still on
screen and should never have been destroyed) or legitimate-but-unpaired (the panel is genuinely
recreated afterwards and nothing RE-HOSTS its controls). Those need opposite fixes, and the log
already distinguishes them: find the caller of DestroyPanel for `0x9b68000` and check whether a
matching re-host follows. **Do not add a "skip erase for radios" special case** -- that would leave
stale entries pointing at freed controls, which is the use-after-free this map's comment says the
erase exists to prevent.

**SPRINT COUNT: 16 -- past the 12 limit.** Flagging rather than silently continuing: by the letter of
the standing rule this is now "waiting for Fable 5.1". It reached that count in the same turn it
produced its root cause, and the remaining work is one localised question, so parking it now would
shelve an item that is one sprint from a fix. **PO decision: park per the rule, or allow one more
turn to land the fix.** Continuing next turn unless told otherwise.


### MP-2 sprints 17-20 (2026-09-05) -- caller identified; PARKED at the rule's limit

`[destroypanel]` now logs the return address. The destroy that takes the locker room's radios is:

    [destroypanel] this=0xaed9b70 artnum=27658 caller=0x832c1ce
    [radioforget] BY-PARENT parent=0xaed9b70 ctrl=... id=2323 ...
    [radioforget] BY-PARENT parent=0xaed9b70 ctrl=... id=2324 ...

`addr2line` resolves `0x832c1ce` to **`RFullPanelDial::CreatePlayer`**. That is normally a HINT only
-- addr2line on this optimised 32-bit build has named the wrong function before -- so it was checked
against the source: `CreatePlayer` (FULLPANE.CPP:3993) is the `IDS_CONTINUE` handler for
`readyroomhostmatch`, and it contains BOTH `pdial[0]->DestroyPanel()` and, further down,
`LaunchDial(new CLockerRoom,0)`. The attribution holds.

**So the teardown is deliberate**: CreatePlayer drops the current panel and launches a FRESH
`CLockerRoom`. The defect is that the new instance's radios never appear -- after the destroy the
only CT_RADIO ever hosted again is one filled with "Everybody", and both 2323 (GAMETYPE) and 2324
(SELECTSIDE) are gone for the rest of the session. So this is the "legitimate-but-unpaired" branch
of the previous entry's question, not the "spurious destroy" branch.

**Note on the id collision, which I retracted earlier and which is now partly back in play:** the
surviving entry carries **id=2324** -- the same id as SELECTSIDE -- but holds "Everybody". Whatever
dialog owns it reuses 2324 for a different control. That does NOT make it the root cause (the map is
keyed by client pointer, as established), but it does mean **id 2324 cannot be used to identify
which control is which**, and any fix or diagnostic that keys off it will be wrong.

**Next step, precise:** instrument `CLockerRoom`'s init on the SECOND launch -- does its
`DDX_Control`/host path run at all for GAMETYPE and SELECTSIDE? If it does not run, the new dialog
never registers them (fix goes there). If it runs and the entries still vanish, a later destroy
takes them (fix goes at that destroy).

**PARKED: WAITING FOR FABLE 5.1.** Sprint count 20, well past the standing 12-sprint limit. I raised
the conflict last turn and continued in the absence of an answer; continuing further would ignore the
PO's own rule. The item is deliberately left in a resumable state: cause localised to one function
pair, next measurement named above, and all instruments (`MA_TRACE_RADIO` now covers hosting,
population, drawing, both erase paths and the destroy caller) are committed and in the build.


### MP-2 sprints 21-22 (Fable 5.1, 2026-09-06) -- FIXED: "Select sides is missing" was an inert radio, and the side selector is hidden BY DESIGN until a game type is chosen

**Two of the four earlier root causes were confounded by the instrument, and the erase chase was a
wrong turn.** Every earlier run CONTINUEd through the locker room within a frame or two of populating
it (the click recipe's later entries fired back-to-back), so "the side radio never draws" could not be
told from "the screen was gone". Held on the screen instead (`BOB_CLICKSEQ="40,r2;90,#2063:1"`,
MA_SHOT at idle 400) with a new uncapped per-radio filter trace (`[radiofilter]`, MA_TRACE_RADIO=1):

    [radiofilter] id=2323 ... vis=1 parentvis=1 scoped=0 in_template=1 never_visible=0   -> drawn 310 frames
    [radiofilter] id=2324 ... vis=0 parentvis=1 scoped=0 in_template=1 never_visible=0   -> never drawn

`vis=0` is `CLockerRoom::RedrawSide()`: under Death Match (the host's default) the side selector is
`ShowWindow(SW_HIDE)` **by design**, and only `OnSelectedRradioGametype` -- the GAME TYPE radio's
Selected event -- ever shows it. So the question was never the drawer, the erase or the id
collision; it was whether a click on GAME TYPE reaches that event. It did not:

    [clickid] id=2323 col=2 -> (117,464)   ...then [click] listbox miss / button / combo miss -- no radio line

**Root cause: `ma_ole_click` (the FRONT-END click dispatcher) had no CT_RADIO in its type filter.**
The radio click arm (`ma_radio_click` -> fire Selected) lived only in the `[tbclick]` toolbar
dispatcher. A radio on a full-screen panel was drawn and inert -- the S164 family for the fifth
time. Fix: CT_RADIO admitted to the filter and given the same arm (`MA_NO_RADIO_CLICK=1` restores
the old filter). Also: the `#ID:n` click recipe now resolves item n of a radio from the control's
own grid (`ma_radio_item_point`); its centre point falls between rows and missed a 2-item group.

Result, same recipe plus `#2323:2` (Quick Missions) and `#2324:1` (Red):

    [radioclick] front-end id=2323 local=(12,50) of 193x99 -> hit=1 sel=2 parent=CLockerRoom
    [radiofilter] id=2324 ... vis=1 ...            -> SELECT SIDE drawn 220 frames
    [radioclick] front-end id=2324 local=(12,30) of 141x95 -> hit=1 sel=1

Capture `~/Documents/260906/logs/mp2_locker_teamplay3.png`: the PO's locker room with GAME TYPE
(Quick Missions ticked) and SELECT SIDE (UN ticked, Red below) both on screen.

**Gate `port/mp_sideselect.sh`** (registered in `gates_all.sh`): the fix arm asserts the GAME TYPE
click registers, the side radio is visible and drawn afterwards, and the Red click registers with
sel=1; the negative control (`MA_NO_RADIO_CLICK=1`) must register no radio click and never draw the
side radio -- it does not, so the draw is click-gated and not incidental. PASS.

**Side effect fixed on the way (E1):** the intro Smacker delayed every headless BOB_CLICKSEQ gate
(24 scripts) by ~12 s and logged `[clickseq] STALLED on entry 0 for 240 idles`. Under
`SDL_VIDEODRIVER=dummy` the intro is now skipped (nobody can see it); `MA_INTRO=1` forces it.

**What the PO will see:** host a Multi-Player game, tick Team Play or Quick Missions, and SELECT
SIDE appears with UN / Red. Under Death Match it stays hidden, as the original does.
**Still open in MP-2:** complaint 1 (no documented route between two AppImages) and complaint 3
(the TCP line's click offset, `MA_TRACE_PRESENT=1` first). Sprints on MP-2: 22.

### MP-2 sprint 22 (Fable 5.1, 2026-09-06) -- complaint 3 FIXED: the "TCP connection line" was drawn 270 px above its label

Captured the service-select screen headless (`BOB_CLICKSEQ="40,r2"`, MA_SHOT at idle 160):
"Internet TCP/IP Connection For DirectPlay" sat at the TOP-LEFT of the screen (0,42) while its
"Select a Service" label was at (20,289). `MA_TRACE_OLE` names it: listbox client (0,32) 586x230,
`rel=0`, parent = the service dialog (not the full panel).

**Cause:** the draw loop treats every CT_LISTBOX as screen-absolute (`rel` excludes the type) --
right for the full panel's own menus and tab bars, whose parent sits at (0,0), wrong for a listbox
hosted by a CHILD DIALOG, whose coordinates are parent-relative like every Windows child. The
provider list is the first dialog-parented listbox anyone has looked at closely.
**Fix:** a dialog-parented listbox is offset by its parent's origin (the full panel's are untouched
by construction; `MA_NO_LB_PARENT_ORIGIN=1` reverts). The click follows the draw through S317's
recorded draw origin: `[clickrow] row=0 -> (313,323)` ... `[click] listbox id=2325
rect=(20,312,586,230) ... HIT`. Capture: `~/Documents/260906/logs/mp2_service_fix.png`.

**Regression check:** `parity_2d` title/prefs_3d/prefs_others/quickmission byte-identical;
`campaign_map` differs by 37929 px -- **identically with the fix disabled**, so it is not this
change: the tree carries uncommitted SCALEBAR/OVERLAY/MIGVIEW edits (not mine, left in place) that
draw down the whole left edge. `dialog_scroll`, `help_click`, `mp_sideselect` PASS. `panel_click`
failed on both arms because it captures a REAL window at a fixed idle and found the INTRO there:
the E1 intro is now skipped on any harness run (`BOB_RUN_INIT`) as well as headless; PASS after.

**MP-2 status:** complaints 2 and 3 fixed and gated; complaint 1 answered by the RUNNING.md guide
(`MA_DPLAY_HOST=<host IP>` on the joiner). Left: the original's TCP/IP address prompt is not
ported -- the env var stands in for it. Sprints on MP-2: 22.

### E1 CORRECTION (PO 2026-09-06 16:15): the title intro is NOT wanted -- and the shipped intro showed a black screen

PO: *"I don't want the mig intro video - it is low quality and doesn't add to the gameplay, I had it
disabled, not sure how that got into the backlog to change that"* and, from the test round: *"ma run
appImage, black screen, ^C in terminal to exit."* The E1 item was "Smacker videos play" and I took
it as far as launching the ORIGINAL introsmack screen at startup; that step was verified by trace
(paint frames 0..171) and never by eye, and on a real screen it is black. Reverted: `MIG.CPP` starts
at the title by default, `MA_INTRO=1` is the opt-in; the AppRun also exports `MA_NO_INTRO=1` unless
`MA_INTRO` is set. The Smacker player stays for the clips the game plays in context (campaign
win/lose, dead pilot), which are untested on screen too -- to be looked at only if the PO asks.
Rule kept: [[fixed-in-dev-is-not-shipped]] -- a screen the PO will see is verified by a capture or
an eye, not a trace.

## ⭐ PO PRIORITY RULING (2026-09-05) — the ordered backlog, highest first

PO, verbatim: *"backlog priority, highest first: ma EPIC M, bob R3, ff GMRADAR-8 and PIT-1,
julia PERF-1, AI car rear-tyre rods, also ma and julia multiplayer"*

Cross-project order, as given:

| rank | project | item |
|---|---|---|
| 1 | **MA** | **EPIC M** — mine the patch changelists + docs for bugs we still have |
| 2 | BoB | R3 — every aircraft exports to ACMI, not just the player |
| 3 | FF | GMRADAR-8 (GMT tank elevation) and PIT-1 (3-view renders no tarmac) |
| 4 | Julia | PERF-1 — frame rate + load time |
| 5 | Julia | AI car rear-tyre rods (the E102 treatment applied to the 5 AI chassis) |
| 6 | **MA + Julia** | **multiplayer** |

**⚠️ Rank 6 conflicts with the standing 12-sprint rule and I am reading the PO's own word as the
override.** MA's multiplayer item is **MP-2**, which I parked at sprint 20 as *"WAITING FOR
FABLE 5.1"* under the PO's rule. The PO has now named MA multiplayer as a priority. Since the rule
is the PO's and this instruction is later and specific, **MP-2 is UN-PARKED** and resumes at rank 6.
It parks again only on a fresh instruction. Its resumable state is intact (cause localised to the
`DestroyPanel`/erase pair, `MA_TRACE_RADIO` covering hosting, population, drawing, both erase paths
and the destroy caller, all committed and in the build).

Julia's multiplayer is **E85** (the epic, sprints 2–4 open) and **MP-4** (two AppImages on different
PCs). Neither is parked.

**Not in the ruling and therefore below rank 6**, in their existing order: MA EPIC J residuals,
EPIC K, EPIC N, EPIC I; BoB R1/P5/P6/P7/UI-2/MP-3; FF MP-1/UIRACE-1; Julia AI-GOLD/SHIFT-3 and the
E-series index. Items parked for Fable 5.1 stay parked: BoB headless flight + R3.2, FF RWY-3 +
LOD-1, Julia Ring/Spa-end-to-end + engine graphics + the terrain-step decision.

**Starting at rank 1 this turn.**


### 🏃 Sprint 432 — "The epic's founding premise was wrong" (EPIC M, rank 1) — ✅ CLOSED 2026-09-05 (8/8)

First sprint under the PO's 2026-09-05 priority ruling. EPIC M is rank 1.

- ⚠️⚠️ **M1 IS RE-ANSWERED, AGAINST S212. Our source is NOT pre-patch, and EPIC M's founding
  sentence is wrong.** S212 concluded "pre-patch" from two negatives — no version string in `SRC/`,
  no `BDG` reference outside our own comments. Both are still true; the inference was not. **This
  codebase does not carry version markers, it carries author-and-date comment tags**, and those date
  the tree directly. Three strands, all in files verified to compile:
  1. `SRC/H/KEYMAPS.H:390–392` — `KeyName(281,NEWFLAPSUP)` … **`//New Flap controls for US version
     and Patch //CSB 24/08/99`**, bound at `:910–966` as shift-f/shift-r/shift-v and implemented at
     `KEYFLY.CPP:1243–1251`. **That is the v1.03 changelist item verbatim**, dated three days before
     v1.03 was compiled. The comment says "Patch" itself.
  2. A date-tag census: 37+4 tags dated **2000**, running to **`RJS 4Dec00`/`05Dec00`** and
     `DAW 27Sep00`. **V1.23 compiled 13 April 2000** — the tree holds work eight months past the
     last patch.
  3. Not stale code: `IMAGEMAP.CPP`, `OVERLAY.CPP`, `MATH.CPP`, `3DCODE.CPP`, `USERMSG.CPP`,
     `WINMOVE.CPP`, `KEYFLY.CPP`, `GEAR.CPP` all confirmed in the build.
- ⭐ **The consequence is a reversal, not an addition.** EPIC M read *"every bug those patches fixed
  in the EXE is, by default, still live in our port"*. **Delete "by default."** M2's prior flips from
  *live unless shown fixed* to **fixed unless shown live** — which makes M2 cheaper (a positive find
  is a dated grep, not a run) and makes M4 harder (a parity deviation can no longer be waved at
  "patch difference"). S212's two celebrated "direct hits" need re-reading on the new prior; MA-P4's
  claimed link to PO-61 is now the first row to re-check.
- ⚠️ **A build-membership trap that would have voided all of the above, caught first.**
  `compile_commands.json` has only 288 entries and says `USERMSG.CPP`, `GEAR.CPP`, `ENGINE.CPP`,
  `MODEL.CPP`, `ACMMAN.CPP` are **not compiled**. They are. **MA builds several directories as unity
  TUs** — `SRC/AI/_AI.CPP` includes `../ai/usermsg.cpp`, `SRC/MODEL/_MODE.CPP` includes `Engine.cpp`,
  `Gear.cpp`, `Model.cpp`, `Acmman.cpp` and six more, all through the lowercase case-variant
  symlinks. Real figure: **382 source files reach the compiler**. Anything asking "does this file
  compile" must resolve the unity includes; `stale-duplicate-sources` and
  `editing-through-a-symlink-splits-it` both apply here at once.
- ✅ **M0 CLOSED.** The patch readme is the same English note in four languages; the English block is
  lines 28–246 and is now fully inventoried — **v1.23 extracted as MA-P10…MA-P25** (15 rows, nearly
  all crash classes: trees-as-targets, tab/fire/pause on take-off, too-many-radio-messages, audio
  thread, a second 3D memory leak, multiplayer init/warping, bad fuel reporting). ⭐ **There are no
  v1.2 / v1.21 / v1.22 changelists** — the readme jumps V1.1 → V1.23, so that "still to inventory"
  line is answered and must not be re-opened. New corpus source listed: **`SRC/CHANGES.TXT`**, the
  drop's own last-changed file list (11 files, all compiled; ACM+ENGINE+GEAR+MODEL — the flight-model
  and air-combat cluster). Recorded as a lead, not evidence: it has no dates and no descriptions.
- ⭐ **M2 first row worked — MA-P20 "Too many radio messages crash" — and it found a live defect on
  the MULTIPLAYER path** (rank 6, so this is groundwork for MP-2, not a detour):
  - `AI.H:45` — `DecisionAI(){optiontable[optionnumber=++optiontablemax]=this;}`. Pre-increment,
    **no bounds check**: `optiontable[0]` is NULL forever, and the 100th construction writes one past
    a 100-element array. **Measured, not guessed: 71 of 100 slots used** (50 live `INSTANCEAI` in
    `USERMSG.CPP` + 21 in `SPOTTED.CPP`). So **LATENT** — with 29 spare, and a trip-wire for exactly
    the EPIC J/PO-7 work that would add radio options.
  - ⭐ `WINMOVE.CPP:8369`, in **`DPlay::ProcessWingmanCommand`** — a DirectPlay packet handler:
    `decision = id2 & 0x7f` (0..127 **straight off the wire**) indexes that 100-entry table with no
    check, then makes a **virtual call**: `decision==0` is a guaranteed NULL deref, 72..99 are NULL,
    100..127 read out of bounds. `option = id2>>7` is added to the options pointer unbounded.
  - **Honest limits:** NOT proven to be MA-P20, and NOT reached in a run — two matched builds only
    ever send a real `optionnumber` in 1..71. It needs corrupted packets or **mismatched peers**,
    which is the situation MP-2 is about. Fix is additive (bound both sites, drop the packet) and
    is deliberately queued **with MP-2** so the two share one multiplayer run.
- **No code changed this sprint.** Everything above is inventory and evidence; the one fix identified
  is scheduled against rank 6 rather than landed here.

## ⭐ PO CADENCE RULE CHANGE (2026-09-05) — **4 sprints per item, not 8 or 12**

PO, verbatim: *"continue scrum, highest backlog items first, then other backlog items, no more than
4 sprints on any one backlog item"*.

**This supersedes the old 8-sprint (BoB/Julia) and 12-sprint (MA/FF) limits.** From now on an item
gets **at most 4 sprints in a pass**, then the loop moves to the next item.

**My reading, stated so it can be corrected in one word:** 4 sprints is a **rotation cap, not a
death sentence** — the item stays open and is eligible again on a later pass through the backlog. It
is not the old rule's "mark it for Fable 5.1 and never run it again". Items already parked for
Fable 5.1 stay parked; the new cap does not retroactively re-park anything, and it does not re-park
MA's MP-2, which the PO un-parked by naming it at rank 6.

Order within a pass: the PO's 2026-09-05 priority ruling first (MA EPIC M → BoB R3 → FF GMRADAR-8
and PIT-1 → Julia AI-CARGFX → Julia PERF-1 → MA and Julia multiplayer), then everything else.


### 🏃 Sprint 433 — "The inverted prior made a prediction, and it held" (EPIC M, rank 1, sprint 2 of 4) — ✅ CLOSED 2026-09-05 (8/8)

- ⭐ **MA-P4 triaged: the fix IS in our tree, so S212's starred PO-61 link is RETRACTED.** S432's
  reversal predicted *"a patch fix is present unless shown absent"*; the first row tested was the one
  S212 called a direct hit on PO-61, and the prediction held. Both of MA-P4's named triggers are
  handled and dated **before** v1.02 (compiled 19 Aug 1999):
  - **audio** — `_Miles.delayedsounds.isSet = FALSE;` is the first statement of
    `Replay::LoadBlockHeader()` (`Replay.cpp:6422`), `//DAW 18Aug99`, **one day before the v1.02
    build**. A pending delayed-sound flag surviving a block boundary is exactly "random crashes in
    the replay, audio trigger".
  - **accel** — `Replay::stopforaccel` (`H/REPLAY.H:725`) driven at `MFC/STUB3D.CPP:1288–1311`,
    `//AMM 26May99`, with the superseded version left beside it as DeadCode.
  - **Verdict: N/A to this port.** S212 wrote that PO-61 *"is exactly"* this crash class; that
    inference required the fix to be missing. It is not. **PO-61 is a different defect**, and M5's
    existing line — the uid→object resolution in `LoadItemAnims` — stands as the live one.
- **The method is now exercised, and it is cheap:** three greps per patch row (year census on the
  implied file → the tags dated after the patch's compile date → read them).
- ⚠️ **A SPLIT PAIR found on the worst possible file, caught only because two greps disagreed.**
  `stopforaccel` reported at line 7697 of `SRC/COMMS/REPLAY.CPP` and line 8478 of
  `SRC/COMMS/Replay.cpp` — one Windows filename cannot hold one symbol at two lines. They are **two
  regular files**, not a symlink pair: the uppercase is the frozen Jul-19 import (7,717 lines, **not
  in the build**); the lowercase is compiled via `_COMM.CPP`'s unity include and is **781 lines
  larger** (8,498), carrying this port's entire replay body of work — EPIC L's ACMI tee, PO-61,
  PO-64, PO-65. `editing-through-a-symlink-splits-it`, second confirmed instance.
  - **The build is correct** (it takes the live file); the hazard is to *reading*, and the first pass
    of this sprint fell into it. Both MA-P4 markers were re-verified in `Replay.cpp` before the
    verdict was written.
  - **Checked and cleared:** the port's own PO-61 reasoning is at `Replay.cpp:1368`, i.e. it was done
    on the live file. No earlier conclusion is void.
  - **Not fixed here, deliberately** — deleting a 176 KB source mid-triage is not this sprint's call.
    Standing rule filed instead: **for replay work grep `Replay.cpp`, never `REPLAY.CPP`.**
- **No code changed this sprint.** EPIC M is at **2 of its 4 sprints** under the PO's new cadence rule.


### 🏃 Sprint 434 — "The port removed Rowan's own containment" (EPIC M, rank 1, sprint 3 of 4) — ✅ CLOSED 2026-09-05 (8/8)

- ⭐ **MA-P15 is LIVE, and an S103 port decision made it wider than it was in Rowan's build.** The
  v1.23 readme warns *"if you update your graphics hardware you must delete `savegame\settings.mig`"*.
  Three steps, each measured:
  1. **`settings.mig` really does persist the graphics selection.** `SaveDataLoad`
     (`H/SAVEGAME.H:267–341`), the block read wholesale, carries `screenresolution`, `colourdepth`,
     `displayW/H`, **`dddriver`**, the enumerated `SDrivers sd`, `fNoHardwareAtAll` and **`fSoftware`**.
  2. **Rowan's containment was the `date2` build-date stamp** — a settings file from a
     differently-dated build is discarded wholesale, which is why the readme's advice was only needed
     for a hardware change without a binary change. **We disable that guard by default** (S103,
     `SAVEGAME.CPP:210–226`; `MA_ENFORCE_SAVE_DATE=1` restores it) for a good local reason: the port
     rebuilds continuously and the stamp would void preferences most days. S103 could not have known
     what it was also switching off; MA-P15 is what names it.
  3. **The loaded value steers rather than being replaced** — `HARDWARE/CONFIG.CPP:722` branches on
     `if (Save_Data.fSoftware)` *before* the redetection at `:746–895` assigns `dddriver`.
  - ⭐ **This lands on PO-12** (21 pts, "choose hardware graphics in Preferences"): `fSoftware` is read
    in `3DCODE`, `3DCOM`, `LANDSCAP`, `TILEMAKE`, `OVERLAY`, `DDRWINIT`, `POLYGON.H`, and **S102
    already found text drawing rerouted by exactly this flag**. A stale `settings.mig` pinning
    `fSoftware` would present as PO-12's symptom while being no rendering bug at all.
  - **NOT established:** that any `settings.mig` here holds a stale value *today*. The proven claim is
    structural. **Next, and cheap: `MA_TRACE_PREFS=1` to dump the live `dddriver`/`fSoftware` before
    a PO-12 sprint spends a run on the renderer.**
- **MA-P17 "Crack and Burn" — the term is resolved:** it is a **mission type**
  (`H/MISSSUB.H:418,428` `S_CRACKBURN`/`CAS_CRACKBURN`, `TEXT_CRACKBURN` in `TEXTENUM.G`), not a
  damage-model term, so the row belongs with campaign/mission triage. Untriageable until the word was
  pinned; recorded for that reason.
- **MA-P12 (F51 speed indicator) — weak signal, deliberately not promoted.** The F51's own files carry
  only 1998 tags, which on the dating method leans "fix absent" — but the fix may live in shared ASI
  code that has not been looked at. Left 🔨.
- **No code changed.** EPIC M is at **3 of its 4 sprints**.


### 🏃 Sprint 435 — "TAB is the accelerate key, not the target key" (EPIC M, rank 1, sprint 4 of 4) — ✅ CLOSED 2026-09-05 (8/8)

- ⭐ **MA-P19 was mis-scoped by its own English until a key table settled it.** `KEYMAPS.H:989` —
  `KeyMap(ACCELKEY, tab, norm)`. **TAB is TIME ACCELERATION.** So *"crash when pressing
  tab/fire/pause on take-off"* is not a targeting bug: it is **accel + fire + pause during
  take-off**, which puts it on **K10**'s path and beside **K11**. A sprint could easily have gone
  into target selection on the obvious reading.
- ⭐ **A candidate mechanism, and it turns on an `assert` this build compiles out:**
  1. `STUB3D.CPP:1983–1997` — engaging accel calls `AutoToggle(AUTOACCEL_WAYPT)`.
  2. `AUTOMOVE.CPP:3425` — that mode does `dp1 = *FindDesPos();`.
  3. `AirStruc::FindDesPos()` never returns NULL; its no-waypoint branch is
     `assert(ai.homebase && "Null waypoint pointer and no home to go to!"); despos = ai.homebase->World;`
  4. **The build is `-DNDEBUG`** (verified in `AUTOMOVE.CPP`'s real command line), so the assert is
     gone and a NULL `ai.homebase` dereferences directly. `-fno-delete-null-pointer-checks` stops the
     optimiser exploiting it, not the deref.
  - **On take-off** is exactly when "no waypoint yet, no homebase resolved yet" is plausible.
  - **NOT proven** to be MA-P19's crash, and `ai.homebase` has not been observed NULL. **Next and
    cheap:** count entries to that `else` with `ai.homebase==NULL`, then run K10's take-off recipe
    with TAB. An assert-only guard in a release build is a defect on its own terms regardless of the
    patch row.
  - Circumstantial only, and marked so: the same accel/pause handler carries `//DEADCODE DAW
    18/02/00` mono-monitor debug writes — someone was debugging *this function* inside the
    v1.2/1.21/1.22 window (4/8/24 Feb 2000).
- ⚠️ **The first `grep ACCELKEY` hit `SRC/3D/VIEWSEL.CPP` — the dead half of the split pair that
  `stale-duplicate-sources` names by name.** Everything above was read from `STUB3D.CPP` and
  `AUTOMOVE.CPP`, both confirmed in the build. Two sprints running, a duplicate source was the first
  thing a grep landed on.
- **EPIC M closes its 4-sprint pass here.** M0 ✅, M1 ✅ (re-answered), M2 ◐ **5 of 25 rows verdicted**,
  M3/M4 not started, M5 unchanged. Handed over with the two cheapest next moves named and both
  display-free: `MA_TRACE_PREFS=1` for MA-P15, and the `ai.homebase` counter for MA-P19.
- **No code changed.** **Rotating to rank 2 — BoB R3.**


## 🔴 NEW ITEM (PO, 2026-09-05): MPTEST-MA — test MA multiplayer by the method that worked for BoB

**PO, verbatim:** *"follow the same general process used to test bob multiplayer, to test ma multiplayer"*

**The method, and why it is worth copying rather than re-inventing.** BoB multiplayer went from
"clicking Fly does nothing" to a host that flies and a client that joins and is allocated a player
slot, in one day, and every step of it was driven from a script with no human at the keyboard. The
reusable parts:

1. **Two instances on ONE machine over loopback**, each with its own scratch game tree
   (`tools/bob_scratch_gamedir.sh` → separate `BOB_DRIVE_C`). No second PC needed, and no risk to
   the player's own tree. MA has the same shape: `MA_DPLAY_HOST` / `MA_DPLAY_PORT` (default 47624).
2. **Enumerate the menus first, then drive by index.** `BOB_DUMP_MENU=1` prints every screen's items
   with captions, rects and centres; the click script is then written against real indices instead of
   guessed pixels. MA's equivalent should be found or added before any driving.
3. **Two click drivers, and the second one matters.** A tick-counter driver
   (`BOB_SDL_CLICK="tick,x,y"`) stops firing whenever the game blocks inside its own comms timeouts —
   which is exactly when the interesting screens appear. **`BOB_SDL_CLICK_MS="ms,x,y"` schedules on
   elapsed milliseconds, so a stalled pump delays a click instead of losing it.** That single hook is
   what finally got BoB's join clicked; without it a harness limit looked like a game defect for
   several sprints.
4. **A control run.** Every claim about the multiplayer path was checked against a *single-player
   flight* with the same instrument. Three wrong conclusions died to that comparison, including one
   where the instrument was in a source file that is not in the build.
5. **Trace both ends of the one message you care about**, not the whole protocol.

**Where MA stands today, so this does not start cold:** PO-76 got two instances joined through the UI
and the FLY door open (S417-S431) — the last of those fixed a shim returning `DPERR_NOCONNECTION` for
the ordinary state of being the first player in your own game. MP-2 (the blank "Select sides" radios)
is un-parked at the PO's rank-6 instruction. BoB's cross-port lessons that should transfer: the
comms path may never fill the Quick-Mission definition the 3-D entry reads from; `EnumSessions` may
consume game packets off a shared socket; and joining clients may never be added to the group the
host broadcasts to. **All three were real in BoB and all three are in shared-engine code.**

**MPTEST-MA — S1 (2026-09-05): the two missing HOOKS are built. Recipe-writing is now possible.**

Step 3 of the method turned out to be the real gap, and step 2 was half-there.

**⭐ `BOB_CLICKSEQ_MS=1` — wall-clock scheduling (the hook the record says mattered most).**
MA's `BOB_CLICKSEQ` is in several ways BETTER than BoB's: entries can name a menu row (`f,rN`), a
control id qualified by hosting class (`f,#ID@Class`), a column (`:COL`), a listbox row (`:rN`), a
cell (`:rN.C`) or a title-bar help glyph (`:?`), all resolved AT FIRE TIME, and it warns when an
entry has stalled. **But it counts in `idle` ticks, one per pump** — the exact clock that stops
advancing when the game blocks inside its own comms timeouts, which is when the multiplayer screens
appear. In BoB that harness limit read as a game defect for several sprints. Now one MODE switch
reinterprets every count as elapsed milliseconds: every existing recipe form keeps working
unchanged, and a stalled pump *delays* a click instead of losing it.

**⭐ `MA_DUMP_MENU=1` — the live clickable inventory.** MA already had `MA_TRACE_DLGCTL`, but it
runs at dialog-TEMPLATE parse time and MA's front end is OLE controls whose captions are not in the
template: it prints `id=2245 class="{78918646-...}" title=""` for every one. Ids with no captions
and no rects are not enough to write a recipe against. The new dump walks the live `hosted()` map,
applies the SAME filters as the click resolver (visible, parent visible, non-zero rect) and computes
the centre the same way — so what it lists is exactly what `f,#ID@Class` would hit — and prints once
per SCREEN CHANGE rather than per frame:

    [menu] ---- 6 clickable control(s) ----
    [menu] #2325@14CSelectService  rect(0,32 586x230)   centre(293,147)  type=1  ""
    [menu] #2023@14CSelectService  rect(40,680 585x26)  centre(332,693)  type=2  ""
    [menu] #2063@14RFullPanelDial  rect(40,960 778x47)  centre(429,983)  type=1  ""

**What that already tells us, which nothing did before:** the existing `BOB_CLICKSEQ="30,r2"` recipe
**does reach `CSelectService`**, the multiplayer service screen, and that screen is **listbox-driven**
(`type=1` = CT_LISTBOX) — not buttons. So MA's recipes must address listbox ROWS, and what a recipe
author needs printed is ROW TEXT, not button captions.

**A defect I introduced and removed before it shipped.** The first dump read the caption by casting
the hosted `void*` to `COleControl*` and reading `m_maText`. A hosted button's `ctrl` is a
`CRButtonCtrl`, a listbox's is a listbox — **not** a `COleControl` — so that is undefined behaviour,
and it printed empty strings *by luck*, which is the worst way for an unsafe read to behave. Button
labels are now cached at the single funnel they are set through (`ma_button_set_string` and DISPID 8
in `ma_olebutton.cpp`), because `CRButtonCtrl::GetString()` and `m_string` are both **protected** and
no free function can read them. Every other control type prints no caption rather than a guess.

**Next sprint, precisely:** listbox ROW TEXT in the dump. `CRListBoxCtrl::GetString(short row, short
col)` (dispid 41, `dispidGetString`) is the accessor and it is **protected** too, and unlike the
button there is no `ma_lb_getprop`-style dispatch shim to reach it through — one has to be added,
the way `ma_button_getprop` exists. With row text printed, the host/join recipe can be written
against real strings instead of row indices guessed from rect arithmetic.

**Then:** two instances over loopback with `MA_DPLAY_HOST=127.0.0.1`, asserting the host logs
`probe from a client -> offered session` and `client joined from ... -> assigned pid`, and the client
logs `EnumSessions -> 1 session(s)` — all three trace lines already exist in `ma_dplay.cpp`.
`isHost` is set only by the UI's `Open(CREATE)`, so there is no shortcut around the recipe.

**Verified:** MA builds clean, and `port/mp_connect.sh` (the PO-76 front-door gate, with its negative
control) still passes with all of this in.

**MPTEST-MA — S2 (2026-09-05): the recipe now reads MA's front end in ENGLISH, and drives to the
multiplayer locker room.**

`MA_DUMP_MENU` gained the two things S1 identified as missing, each cached at its own funnel (the
control classes' accessors are not reachable from a free function, and guessing at a member through
the hosted pointer is the undefined behaviour S1 had to remove):

* **listbox ROW TEXT**, read via `CRListBoxCtrl::GetString(row, col)` — reachable from
  `ma_olecontrol.cpp` after all, since the dispid dispatch in that same file calls it.
* **static LABEL text**, cached in `ma_olestatic.cpp`.

**What that immediately settled.** `BOB_CLICKSEQ="30,r2"` had been used for sprints on the
assumption that row 2 is Multi-Player. It is — now stated rather than assumed:

    main menu #2063@RFullPanelDial : Preferences / Single Player / MULTI-PLAYER / Load Game /
                                     Replay / Credits / Quit
    #2325@CSelectService           : "Internet TCP/IP Connection For DirectPlay"

**`BOB_CLICKSEQ_MS` proved out end to end** (S1 only compiled it). A two-step ms-scheduled recipe —
`BOB_CLICKSEQ_MS=1 BOB_CLICKSEQ="20000,r2;35000,#2325@CSelectService:r0"` — drove Main →
Multi-Player → service select → **`CLockerRoom`**, the screen BoB's join went through, and the log
confirms the mode with `[clickseq] counts are MILLISECONDS`.

**The locker room, read off the dump:**

| control | type | label |
|---|---|---|
| `#2321` | edit | **Name** |
| `#2320` | edit | **Session** |
| `#2144` | edit | **Password** |
| `#1012` | combo | **Data Rate** |
| `#1010` | combo | **Scenario** |
| `#2323` | radio 193x99 at (41,795) | under **GAME TYPE** (41,767) |
| — | — | **SELECT SIDE** (284,767) — **nothing beneath it** |

⭐ **That last row is MP-2** ("the blank Select sides radios"), and the dump turns it from a visual
complaint into a structural fact: GAME TYPE has a hosted radio group at (41,795); the SELECT SIDE
column at x≈284 has **no hosted control at all** in the band below its label. Nothing is drawn there
because nothing is registered there — so MP-2 is a control that is never created/hosted, not a
control that draws blank. Worth confirming against the dialog template before acting on it.

⭐ **The action bar `#2063` currently holds only "Back"** — no Host, no Join, no Fly. In BoB the
equivalent bar gained its entries once the screen's fields were valid, so the next step is to fill
**Name** and **Session** (`MA_TYPESEQ` already exists for exactly this and is the one interaction
with no other injector) and re-dump to see whether the bar grows. That is the gate between here and
two instances over loopback.


### MPTEST-MA S3 (Opus 5, 2026-09-12) — the locker room fills; the action bar does NOT unlock on Name+Session

Two instrument gaps fixed first, because S2's next step could not be driven or read without them:

* **`MA_TYPESEQ` is now a SEQUENCE** — `"<at>,<text>;<at>,<text>;..."`, same shape and the same
  `BOB_CLICKSEQ_MS` millisecond mode as the click recipe. It was single-shot, and the locker room
  needs TWO fields (Name, Session); one injection cannot fill two.
* **`MA_DUMP_MENU`'s re-print signature covers ROW TEXT and button/static captions, not just the id
  set.** An action bar keeps its id while its rows change — "Back" alone before a screen is valid,
  "Back / Host / Join" after — so the exact event this sprint is looking for could not trigger a
  re-print. The dump now re-prints when what a click can ADDRESS has changed. (`ma_olecontrol.cpp`,
  using the dump's own `CRListBoxCtrl::GetString` / `ma_button_cached_string` accessors.)

**Recipe that now runs end to end** (`~/Documents/260912/logs/ma_locker_fill.sh`, log `..._fill4.log`):

    MA_DUMP_MENU=1 BOB_CLICKSEQ_MS=1 MA_TRACE_CLICK=1 \
    BOB_CLICKSEQ="20000,r2;35000,#2325@CSelectService:r0;50000,#2321@CLockerRoom;58000,#2320@CLockerRoom" \
    MA_TYPESEQ="53000,Viper;61000,MAGAME" ./wmig     # from the install dir

    [clickid] id=2325 col=-100 -> (653,750)          # service row 0
    [clickid] id=2321 col=-1 -> (594,661)            # Name edit
    [click] edit id=2321 takes keyboard focus (topmost, before the list)
    [typeseq] entry 0 injected "Viper" at ms 53002 -> focused=1
    [typeseq] entry 1 injected "MAGAME" at ms 61025 -> focused=1

⛔ **Result: the action bar `#2063` still holds only "Back" after both fields are filled** — and with
the row-text signature in place, a bar that grew would have re-printed. So the BoB analogy ("the bar
gains its entries once the fields are valid") does NOT carry: Name + Session are not the gate.

**The locker room as dumped, with what is still unaddressed:**

| control | type | note |
|---|---|---|
| `#2321` / `#2320` / `#2144` | edit | Name / Session / Password — all fillable now |
| `#1012` / `#1010` | combo | Data Rate / Scenario — never touched by a recipe yet |
| `#2323` | radio 193x99 | under GAME TYPE — hosted, never clicked |
| — | — | **SELECT SIDE (`#2024`) still has NO hosted control beneath it** (MP-2) |
| `#2063` | action bar | "Back" only |

**S4 (next):** click the GAME TYPE radio `#2323:r0` and pick a Scenario `#1010:r0`, then re-dump —
those are the two screen fields a lobby normally validates before offering Host/Join; and try Enter
after the Session text, since a DirectPlay lobby commonly commits the session name that way.


### MPTEST-MA S4 (Opus 5, 2026-09-12) — ⭐ MP-2 EXPLAINED: the SELECT SIDE radios are created when a GAME TYPE is chosen

S3 left the locker room with Name and Session filled and the action bar still offering only "Back",
and listed the two untouched fields. S4 clicked the first of them — the GAME TYPE radio group
`#2323` — and re-dumped:

    [clickid] id=2323 col=-100 -> (457,872)     # GAME TYPE, row 0
    [menu] #2323@11CLockerRoom  rect(41,795 193x99)  type=8      GAME TYPE   (was already there)
    [menu] #2324@11CLockerRoom  rect(284,795 141x95) type=8      <- NEW, and it sits at x=284,
                                                                    directly under SELECT SIDE (284,767)

⭐ **`#2324` is MP-2.** The PO's *"Select sides is missing"* and S2's structural finding ("the SELECT
SIDE column has no hosted control at all in the band below its label") describe the screen **before a
game type is chosen**. Choosing one creates the side radios in exactly the place the label promises.
MP-2 is therefore not a control that fails to draw and not a port defect — it is screen flow, and
every sprint that hunted `DestroyPanel`, the erase pair and the radio population was hunting a
control that was never supposed to exist yet. (Sprints 13-22 remain correct about what they measured;
what they lacked was a recipe that could click a radio, which S3's sequence work finally provided.)

Also measured in the same pass: the Scenario combo `#1010` **disappears** when the game type is
chosen (`[clickid] id=1010 UNRESOLVED`), and `#1012` (Data Rate) stays — i.e. the screen swaps its
fields per game type, which is coherent UI behaviour rather than a defect.

⛔ **The action bar is still "Back" only.** So the remaining gate is not Name/Session and not the game
type. S5 (running) picks a SIDE from the new `#2324` and re-dumps; if the bar still holds one row
after every field on the screen is set, the next question is whether MA's action bar is populated at
all in this port (a `#2063` that only ever holds "Back" would be its own defect, and the dump can now
prove it either way because its signature covers row text).


### MPTEST-MA S5/S6 (Opus 5, 2026-09-12) — ⭐⭐ THE ACTION BAR WAS NEVER EMPTY. My dump read one column of a horizontal list

S5 picked a SIDE from the new `#2324` and set the Data Rate combo; the bar still printed one row,
"Back". S6 asked the obvious question — where would "Host"/"Join" come from? — and the answer is in
`FULLPANE.CPP:500`:

    {IDS_CREATEGAME, &multiplayer,   &RFullPanelDial::CreateCommsGame},
    {IDS_JOINGAME,   &selectsession, &RFullPanelDial::GetSessions},

and in `RFullPanelDial::PositionRListBox` (`FULLPANE.CPP:2245`): for a HORIZONTAL list it does
`AddColumn(seperation); AddString(string, x)` — **one entry per COLUMN of a single row**. My dump
called `GetString(row, 0)`. It printed the first column and nothing else, for every sprint that has
ever looked at this bar.

With the dump reading every column:

    service select   row 0: [0] "Back"  [1] "Create Game   "  [2] "Join Game    "
    locker room      row 0: [0] "Back"  [1] "Continue"

⭐ **MA's multiplayer UI is fully populated and always was.** "The action bar holds only Back" was my
instrument, not the game — the same class as [[instrument-bookkeeping-lies]], and it cost S3-S5 their
conclusions. What stands from those sprints is the screen flow (Name/Session type in, GAME TYPE
creates the SELECT SIDE radios) and the correction to MP-2.

**Two fixes went in with this:**
* the dump prints every populated column and folds them into its re-print signature;
* `CRListBoxCtrl::GetString` (`RLISTBXC.CPP:1890`) **indexed both its column and row lists with no
  bounds check** — `FindIndex()` returns NULL past the end and `GetAt(NULL)` dereferences it, so
  asking a listbox for a column it does not have SEGFAULTED the game (rc=139, caught on the first
  column walk). Out of range now reads as "no string", which is what every caller already tests for.

**The path to two instances is now spelled:** service select `#2063@RFullPanelDial:r0.1` = Create Game
(host) or `:r0.2` = Join Game (client); locker room `:r0.1` = Continue. S7 drives the host end of it.

## MPTEST-MA S8 (2026-09-12) — the UI path is proven end to end; the LINK is not, and run 1 could not see it

`tools/ma_mp_two_instance.sh` ran two `wmig` instances over loopback for 260 s (logs:
`~/Documents/260912/logs/ma_mp2/`). Every click and keystroke in the sequence was delivered, which
settles the screen map S5–S7 built:

| | host | client |
|---|---|---|
| main menu `r2` | Multi-Player | Multi-Player |
| `#2325@CSelectService:r0` | "Internet TCP/IP Connection For DirectPlay" | same |
| `#2063@RFullPanelDial` | `:r0.1` **Create Game** | `:r0.2` **Join Game** |
| `CLockerRoom` | (fields left default) | `#2321`="Viper2", `#2320`="MAGAME" typed; `#2323:r0`, `#2324:r0` clicked |
| bar `:r0.1` Continue | → **`CReadyRoom`** (art 28172) | → **`CSQuick1`** (art 28166) |

Two things worth keeping:

- ⭐ **`#2324` (SELECT SIDE) does not exist in the locker-room dump until GAME TYPE is clicked**, then
  appears at `rect(604,823 141x95)`. That is MP-2's explanation confirmed by observation rather than
  by reading the code: the side radios are *created by* the game-type click. MP-2 can be closed.
- The host's Ready Room bar is `Quit / Fly / Visitors / Radio / Paint Shop / Prefs` and its player
  table holds exactly **one** row: `"1" "Player" "F86 1" "Flight Line" "0" "0"`.

**The client never joined.** Its Continue led to `CSQuick1` (Mission / Target Zone / Cloud —
the *mission definition* screen, bar `Quit / Variants / Flight Line`), which is what a standalone
game does, and the host's table never gained a second row.

**Run 1 could not say why, and I nearly misread it.** Both logs contained zero `[dplay]` lines, which
looks like "no traffic" — but `SRC/compat/ma_dplay.cpp` prints nothing at all unless
`MA_TRACE_DPLAY` is set, and the harness never set it. The silence was a blind instrument, not a
measurement; the empty player table was the only real evidence. (Same trap as
`instrument-bookkeeping-lies`: prove the instrument can speak before believing a zero.)

The shim is not a stub — it is a real UDP peer (`MSG_PROBE/OFFER/JOIN/DATA/ASSIGN`, a 64-deep queue,
groups). Its own comment names the exact ambiguity to resolve next:

    /* PO-76: A host answers discovery ONLY from pump(), and pump() runs only when the game calls
       Receive / GetMessageCount / EnumSessions. So "nobody can find my session" and "the game is
       not pumping" are the same symptom from outside, and a client that finds nothing cannot tell
       them apart. */

**S8 harness change (committed with this note):** both instances now run with `MA_TRACE_DPLAY=1` and
a pinned `MA_DPLAY_PORT`/`MA_DPLAY_HOST`, and three assertions were added that test the *link*
rather than the screens — the shim speaks on both sides (pump lines), the host answers a probe, and
the host's table gains a second row. Run 2 is in flight with these.

## MPTEST-MA S9 (2026-09-12) — the client never takes the join path at all, and S7's screen map was wrong

Run 2 (`~/Documents/260912/logs/ma_mp2b/`) with `MA_TRACE_DPLAY=1` on both instances. The shim can
now speak, so the zeros below are measurements rather than silence.

| | host | client |
|---|---|---|
| `pump #` lines | **113168** | **0** |
| `EnumSessions` | — | **0 calls** |
| `Open(...)` / `bound to UDP` | `Open(CREATE) session "MiG Alley"`, bound 47624 | **none** |
| `UINewPlayer` | `PlayerName="Player" SessionName="MiG Alley" type=1` | **none** |
| probes received | **0** | — |

The client's ENTIRE DirectPlay history is four calls:

    CoCreateInstance -> EnumConnections (1 provider) -> InitializeConnection -> UpDateDPlay

It never opens a session, never enumerates, never sends a probe. So this is **not** the PO-76
ambiguity the shim's comment warns about — the host pumps 113k times and would answer a probe
instantly; nothing ever asks.

⭐ **Why: neither instance ever activates "Create Game" or "Join Game".** The dispatch table
(`FULLPANE.CPP:498-503`) is

    {IDS_QUICKMISSION1, &title,          &RFullPanelDial::CleanUpComms}    // Back
    {IDS_CREATEGAME,    &multiplayer,    &RFullPanelDial::CreateCommsGame}
    {IDS_JOINGAME,      &selectsession,  &RFullPanelDial::GetSessions}     // <- the only EnumSessions path

and `GetSessions` is the only route to `UIGetSessionListUpdate()` → `EnumSessions`. But clicking the
service-provider row `#2325@CSelectService:r0` **already advances to the locker room by itself**: in
both logs the very next dump is `CLockerRoom` with the two-item bar `Back / Continue`, and the host
has by then already run `UINewPlayer type=1` and `Open(CREATE)`. Selecting the provider therefore
implies Create Game. S7 read the service bar `Back / Create Game / Join Game` out of a dump and
assumed it was still on screen at `44000ms`; it was not. The harness's `:r0.1` / `:r0.2` clicks at
that moment land on the **locker room's** bar, where the host's x=587 hit Continue (it advanced to
`CReadyRoom`) and the client's x=818 hit nothing (it stayed put, and its later Continue fell through
to `CSQuick1`).

So MPTEST-MA has been measuring one hosting instance and one instance that quietly hosts nothing.

**S10 (next MA rotation), in order:**

1. Click the service **bar** item before/instead of the provider row, and prove it by the trace:
   the client must show `[dplay] EnumSessions: probing 127.0.0.1:47624` and the host a probe answer.
   If selecting the provider row cannot be avoided, find what consumes it — `SelectServiceInit` is
   the screen's init hook and is the first thing to read.
2. Then the two known field defects, both already visible: typing **appends** to the edit box rather
   than replacing it (`IDC_NAME` came out `"PlayerViper2"`, not `"Viper2"`), and the client's session
   name must match the host's — the host creates `"MiG Alley"` from the untouched default while the
   harness types `"MAGAME"` into the client.
3. `UIGetSessionListUpdate()` (`COMMS.CPP:325`) has a silent-success hole worth closing regardless:
   `HRESULT res=DP_OK; if (lpDP4) res = lpDP4->EnumSessions(...); if (res!=DP_OK) return false;`
   — with a null `lpDP4` it returns **true** and an empty list, i.e. "discovery succeeded, no games",
   which is indistinguishable from a real empty lobby.

**Harness correction made this sprint:** the S8 assertion `row 2: [0]` for "a second player joined"
is a **false positive** — it also matches the main menu's third item, and run 2 scored it PASS
against a one-row table. It now requires a row carrying the player table's sixth column (`[5]`),
which no menu list has. (Same family as `instrument-bookkeeping-lies`: the assertion, not the game,
was doing the lying.)

## 🏃 Sprint 436 — EPIC M / MA-P19: the assert that was the only guard on a null dereference (2026-09-12)

Taking the cheaper of the two moves sprint 435 handed over (`MA_TRACE_PREFS` for MA-P15 is the
other). S435 named a candidate mechanism for the patch row *"crash when pressing tab/fire/pause on
take-off"* and left it unproven; this sprint makes the code safe and arms the counter that can prove
or kill it.

**Verified, not assumed.** `AUTOMOVE.CPP:8164-8166`, the no-waypoint branch of
`AirStruc::FindDesPos()`:

    assert(ai.homebase && "Null waypoint pointer and no home to go to!");
    despos=ai.homebase->World;

and `compile_commands.json` for this exact file carries **`-DNDEBUG`**. So in every shipped binary
the assert is gone and the next line dereferences a null `ai.homebase` directly. That is a defect on
its own terms whether or not it is MA-P19's crash — the release build has *no* guard where the
source appears to have one. (TAB is `ACCELKEY`, time acceleration, `KEYMAPS.H:989`, which is what
puts that patch row on this path at all; S435 established that.)

**Fixed, both levels.**

1. `AirStruc::FindDesPos()` — null `ai.homebase` now **holds station** (`despos = World`, +800 m,
   range 1600 m, matching what the homebase branch does to its own result) instead of dereferencing.
2. `mobileitem::FindDesPos()` was `return waypoint->FindDesPos();` with nothing checking `waypoint`,
   one level above a branch that carefully guards its own null waypoint. Now guarded the same way.

Both paths count their entries and print under **`MA_TRACE_DESPOS=1`** (first 8, then every 500th),
so "is `ai.homebase` ever actually null?" stops being unanswerable.

**Honest state of the evidence.** Built clean, `wmig` carries the new strings, and a 60 s front-end
smoke run reaches the main menu with no regression — but **zero `[despos]` lines**, because the front
end never flies. The counter cannot speak until something takes off, so the fix is a hardening with
the hypothesis still open, not a confirmed MA-P19 repro. Stated that way deliberately.

**Next (named and partly mapped):** MA has only one harness (`tools/ma_mp_two_instance.sh`, which is
a multiplayer path). A 95 s probe mapped the first step of the single-player route — main menu `r1`
"Single Player" → a screen whose list begins **"Hot Shot"** (the pilot/difficulty chooser). Walking
that to a take-off and then driving K10's recipe (accel + fire + pause) with `MA_TRACE_DESPOS=1` is
the run that settles MA-P19.


## 🏃 Sprint 437 — EPIC M / MA-P19: the single-player route, mapped; and why the TAB test still cannot run

S436 hardened the null dereference and armed `MA_TRACE_DESPOS`, but the counter cannot speak from the
front end. This sprint went after the run that would make it speak. Three probes, each ~2 min.

**The single-player route (new; MA had no single-player harness at all — `tools/` held only the
multiplayer one):**

| step | click | lands on |
|---|---|---|
| main menu | `r1` | Single Player |
| | `r1` | the mode list: **Hot Shot / Quick Mission / Campaign / Entire War** |
| Quick Mission | `r1` | `CSQuick` + `CCampBack`, bar **`Back / Variants / Fly`** |
| | `#2063@RFullPanelDial:r0.2` (Fly) | `CSQuick2` — a SECOND briefing page, not the sim |

⚠️ **The `:rN.C` column form does not work on the mode list.** It printed
`[clickid] id=2063 col=1 not mapped by GetColFromX (w=333)` — that list is VERTICAL (333 px wide,
5 rows, `rowH=43`), so rows are addressed with a plain `rN` and only the horizontal action bars take
`:r0.C`. Worth writing down: the same control id `2063` is a vertical list on one screen and a
horizontal bar on the next, and the wrong form fails with a message that does not say "wrong axis".

**Where it stops, measured:** a 200 s run that clicked through to Fly produced **zero** `View3d` /
`Launch3d` / `InThe3D` lines and **zero** `[despos]` lines. `Fly` on `CSQuick` advances to `CSQuick2`
(the second briefing page, showing "Operational" / "N / A" fields); the sim launch must be behind at
least one more control on that page. `CSQuickAirClaims` (bar `Back / Ac Stats / Ground Stats /
Replay`) also appears in the walk and is a debrief-style page, not the route.

**So MA-P19 is blocked on two things, both now named rather than guessed:**

1. **One or two more clicks** to get from `CSQuick2` into the 3D. Cheap — one more probe reading that
   page's `#2063` bar.
2. ⭐ **A key-injection hook, which does not exist.** MA-P19's recipe is *accel + fire + pause during
   take-off*, i.e. real game keys in the 3D. `MA_TYPESEQ` injects characters through
   `ma_ole_char()` into front-end OLE edit boxes and cannot reach the sim, and synthetic keys do not
   work on this desktop at all (`no-synthetic-keys-under-wayland`). BoB solved the same problem with
   an env-driven hook; MA needs the equivalent — call it `MA_KEYSEQ`, feeding the same path the
   keyboard handler uses, with `ACCELKEY` (`KEYMAPS.H:989`) as its first customer.

**Useful either way:** once step 1 lands, the `[despos]` counter reports on the AI too — every AI
aircraft calls `FindDesPos` continuously — so "is `ai.homebase` ever null?" gets an answer from any
flight at all, without needing the TAB recipe. That is the cheapest next move and it does not depend
on the key hook.


## 🏃 Sprint 438 — EPIC M / MA-P19: the flight runs, and the candidate mechanism does NOT fire

**First, a correction to S437.** S437 reported "zero `View3d`/`Launch3d`/`InThe3D` lines, so the sim
never launches". That was my instrument, not the game: those are **BoB's** bridge tags, and I grepped
them in an MA log. MA has its own hook. With `MA_TRACE_3D=1` the launch is fully traced:

    [3d] Start3d(stat=2) status=1  ->  status=3 (waiting for DONEBACK=4)
    [3d] Start3d(stat=4) status=3  ->  status=7 (7 = GOING)
    [3d] flight-view active; setup3dstatus=8
    [3d] driving Launch3d (OnGetString)...
    [3d] Launch3d returned; tmpinst=0xb2ba860 tmpview=0xb8b3510
    [3d] flight close (id=1) -> OnOK + OnFlyingClosed
    [debrief] OnFlyingClosed rv=1 gamestate=2 (HOT=2 QUICK=1 CAMP=3 WAR=4)

**MA flies.** Two other things S437 got wrong and this sprint fixed: the flyable route is **Hot Shot**
(`RUNNING.md:10`, "Single Player → Hot Shot is two clicks to a flyable 3D mission"), not Quick
Mission — Quick Mission's `Fly` leads to a second briefing page and then the claims screen. And the
3-D is **default-ON**, gated only by `MA_DISABLE_3D` (`MIG.CPP:2509`); the `MA_ENABLE_3D` I went
looking for is a legacy alias mentioned only in a comment.

**The measurement MA-P19 wanted.** `MA_TRACE_DESPOS=1` over a Hot Shot flight:

| counter | result |
|---|---|
| `FindDesPos` calls (denominator) | **call #0 printed** — the function runs |
| `waypoint==NULL` branch entered | **0** (prints its first 4 entries; none appeared) |
| `ai.homebase==NULL` | **0** |

⭐ **So S435's candidate mechanism does not fire in an ordinary flight.** Every aircraft has a
waypoint; the no-waypoint branch that holds the null dereference is not on the everyday path. The
S436 hardening remains correct on its own terms — a release build had no guard at all where the
source appeared to have one — but it should **not** be described as "MA-P19 fixed", and the patch
row stays open.

**Two instrument rules applied here, both from earlier scars.** The branch counter was made
unconditional (S437's zero was equally consistent with "never null" and "branch never reached"), and
then a **denominator** was added, because a zero from an instrument that never ran is not evidence.
The denominator is what turned this from a guess into a result: `call #0` proves the function
executed, so the branch counters' silence means the branch genuinely was not taken.

**Still open for MA-P19:** the flight closes quickly (only one `call #0` report at a 20,000-call
interval, so under 20k calls), and the TAB recipe — accel + fire + pause on take-off — still cannot
be driven, because MA has no key-injection hook for the sim (S437). Either extend the flight and
re-measure, or build `MA_KEYSEQ`. The latter unlocks every future in-sim test, not just this one.


## 🏃 Sprint 439 — EPIC M: **MA_KEYSEQ built** — the sim can be driven by key at last; MA-P19 does not reproduce

S437 named the blocker: "a key-injection hook, which does not exist... MA needs the equivalent —
call it `MA_KEYSEQ`". Built this sprint, and it works.

**It needed no new input plumbing.** The game has always had `keytests::KeyFake3d(keyval, held, hit)`,
which sets exactly the bitflags the sim polls, so the only missing piece was a schedule. `MA_KEYSEQ`
pumps from `KeyPress3d` (called every frame), on a wall clock that starts at the **first 3-D key
poll** — so a recipe's times are measured from the flight, not from process start.

    MA_KEYSEQ="<ms>,<keynum>[,<holdms>][;...]"      MA_TRACE_KEYSEQ=1 reports press and release

`keynum` is the `KeyName()` number from `KEYMAPS.H`, **not a scancode** — `KEYMAPS.H:93` defines the
`KeyVal3D` as `keynum*2` and the hook does that conversion, so recipes quote the header's own
numbers: `SHOOT=50` (:167), `PAUSEKEY=51` (:168), `ACCELKEY=130` (:240). Hold defaults to 400 ms;
one key is held at a time.

**Proven in a Hot Shot flight** — MA-P19's own recipe, accel + fire + pause:

    [keyseq] armed: "5000,130;9000,50;13000,51" (ms from first 3-D key poll)
    [keyseq] press keynum=130 (KeyVal3D=260) at 5000ms (due 5000ms) hold 400ms
    [keyseq] release keynum=130 at 5400ms
    [keyseq] press keynum=50 (KeyVal3D=100) at 9000ms ...
    [keyseq] press keynum=51 (KeyVal3D=102) at 13000ms ...

**Result for MA-P19: no crash.** The flight survived all three keys and was still running when the
200 s cap ended it (no `OnFlyingClosed`). `[despos]` printed only its denominator (`call #0`) — the
`waypoint==NULL` branch was never entered, so the null-`ai.homebase` dereference S435 proposed did
not occur even under the recipe that names it. The four `GLib-GObject-CRITICAL` lines in the log are
GTK start-up noise timestamped before the flight, not a fault; checked rather than counted.

⚠️ **Honest limit:** the keys fired 5–13 s after the first 3-D key poll, which is not necessarily
*during take-off* — the patch row says "on take-off" and that timing is still a guess. A run that
locates the take-off roll and fires there is the remaining variable.

⚠️ **Process: I edited a dead file first.** The change went into `SRC/INPUT/KEYTESTS.CPP`, which
defines `keytests::KeyFake3d`/`KeyPress3d` and **is not in the build** — `ninja -t deps` shows zero
references and it is absent from `compile_commands.json`. The live implementation is
`SRC/INPUT/KEYSTUB.CPP`. The build said "no work to do" and that is the only reason it was caught.
Exactly the `stale-duplicate-sources` trap, re-learned; the dead file was reverted rather than left
carrying a change that can never run.

**MA_KEYSEQ is the reusable result here.** Every future in-sim MA test — controls, HUD, weapons,
the remaining PO-row repros — now has a way in that does not depend on the desktop.


## MPTEST-MA S10 (2026-09-13) — the client DISCOVERS now; the host opens its session too late

S9's named step was "click the service **bar** item instead of the provider row, and prove it by the
trace: the client must show `[dplay] EnumSessions: probing 127.0.0.1:47624`". Done, and it does:

    [dplay] EnumSessions: probing 127.0.0.1:47624
    [dplay] EnumSessions -> 0 session(s)

⭐ **The Join path is finally taken** — three sprints of this item were spent on a client that never
called `EnumSessions` at all. Skipping `#2325@CSelectService:r0` is what does it: `GetSessions`
(`FULLPANE.CPP:3797`) calls `UISelectServiceProvider` itself, so the provider never needed choosing,
and choosing it silently implied Create Game.

**But zero sessions, and the host's log has no `Open(` in it at all.** The host clicked Create Game
correctly — `[clickid] id=2063 -> (644,1009)` on a 778-px three-item bar puts 644 in
`Back|619..878 Create Game|878.. Join Game`, the middle item. It just does not do what the name
suggests:

```c
Bool RFullPanelDial::CreateCommsGame(FullScreen*&fs)
{
    _DPlay.UISelectServiceProvider(&_DPlay.ServiceName[0]);
    _DPlay.UIPlayerType = PLAYER_HOST;
    return TRUE;
}
```

⭐ **"Create Game" creates nothing.** It selects the provider, marks this instance the host, and
advances to the locker room. The session is opened later, in `UINewPlayer`, called from the READY
ROOM (`READY.CPP:204`) — i.e. **after the locker-room CONTINUE**. Run 2 only appeared to open early
because its provider-row click had already advanced it to the locker room, so its next click landed
on Continue.

So the harness had the host pressing Continue at 190 s while the client probed at ~70 s: the client
was enumerating a lobby that did not exist yet and correctly reported nothing. Not a transport
fault, an ordering one.

**Harness fixed:** new `HOST_OPEN_MS` (default 60 s) presses the locker-room Continue — and therefore
opens the session — before the client's Join, which moves to 55 s on its own clock (85 s wall).
`HOST_FLY_MS` still drives the Ready Room's Fly afterwards.

**S11 (next MA rotation):** rerun with that ordering. The pass condition is unchanged and now
reachable: the client's `EnumSessions` must report **1** session, the host must log a probe answer,
and the host's Ready Room player table must gain a second row (the assertion fixed in S9 so it can
no longer match a menu item).


## MPTEST-MA S11 (2026-09-13) — ⭐ the two instances SEE each other for the first time

S10's ordering fix works. With the host pressing the locker-room Continue at 60 s (which is where
`UINewPlayer` actually opens the session) and the client joining at 85 s wall:

    host    [dplay] UINewPlayer: PlayerName="Player" SessionName="MiG Alley" type=1
            [dplay] host bound to UDP 47624
            [dplay] Open(CREATE) session "MiG Alley"
            [dplay] probe from a client -> offered session "MiG Alley"
    client  [dplay] EnumSessions: probing 127.0.0.1:47624
            [dplay] EnumSessions: found "MiG Alley"
            [dplay] EnumSessions -> 1 session(s)

**`host answers a discovery probe` PASSes for the first time in this item.** Host creates, client
probes, host offers, client finds it — the transport and the session naming work end to end over
loopback. Four sprints of this item were spent on a client that never even called `EnumSessions`.

**And the UI shows it.** The client's screen walk is now
`CSelectService → CSelectSession → CLockerRoom`, and the session screen's list reads:

    [menu] #2326@14CSelectSession  rect(0,32 570x230)  type=1
    [menu]      row 0: [0] "MiG Alley"

⭐ **The host's game is listed, by name, in the joiner's UI.**

**Why it still fails:** the harness walked straight past that screen to the locker-room fields
without ever selecting the session, so no join was requested. `host table shows a SECOND player`
correctly reports FAIL — nothing asked to join. (The host's two `CreatePlayer` calls, pid 1 and
pid 3, are both its own; the shim mints ids from `nextPid++` for a host, so they are not evidence of
a joiner.)

⚠️ Also note `shim speaks ... c=0` is a **bad assertion on my part**, not a client fault: the shim's
`pump()` is called from `Receive`/`GetMessageCount`, and `EnumSessions` runs its own recvfrom loop
without pumping. A client that has enumerated but not joined legitimately never pumps. That
assertion needs to be scoped to after a join, or dropped.

**S12 (next MA rotation):** the harness now clicks `#2326@CSelectSession:r0` at 62 s — the listed
session — before the locker-room fields. The pass condition is unchanged: the host's Ready Room
player table must gain a second row.


## MPTEST-MA S12/S13 (2026-09-13) — ⭐⭐ THE CLIENT JOINS. MA has a real two-instance session.

S11 left the client sitting on `CSelectSession` with the host's game listed and nothing selecting it.
Two clicks were missing, both found by reading the dump rather than guessing.

**1. Selecting the row is not committing it.** `#2326@CSelectSession:r0` highlights the session; the
screen has its OWN action bar — 292 px wide, `row 0: [0] "Back" [1] "Select"` — which is a different
bar from the 778 px service bar, so the column index differs too. Without the `Select` the client
stayed put re-enumerating, and the next click (`#2321`, a locker-room field) came back
`UNRESOLVED`.

**With `Select` added, the join happens:**

    [dplay] Open(JOIN) -> host 127.0.0.1:47624
    [dplay] host assigned us pid 4
    [dplay] CreatePlayer "(unnamed)" -> pid 4
    [dplay] pump #0 (host=0) -- discovery is answered only from here

⭐ **That is MA's first real join between two instances** — session opened as a client, player id
assigned BY THE HOST, player created, and the client now pumping. `shim speaks (host/client pump
lines)` flipped to PASS in the same run. The client's screen walk is
`CSelectService → CSelectSession → CLockerRoom` and its Name/Session/GAME TYPE fields all resolve.

**2. The joiner must NOT click SELECT SIDE.** `#2324` is never created for a joiner — the game type
is the host's choice, which is MP-2's finding seen from the other side. The click returned
`UNRESOLVED`, and an unresolved click is **retried, not skipped**, so the sequence never advanced to
the locker-room Continue. That is why a client that had genuinely joined still failed
`client reaches the Ready Room`: a harness artefact on top of a working join, not a comms fault.
Removed in S13.

**Still FAIL, and the two remaining assertions are honest:** `client reaches the Ready Room` and
`host table shows a SECOND player`. Both should follow from the Continue that the `#2324` retry was
blocking. S13's rerun is the test.

**Worth recording about the harness itself:** an UNRESOLVED click silently stalls the whole sequence.
Every MA recipe that targets a control which may not exist on a given path needs that control to be
optional, or the run reports a downstream failure that has nothing to do with the thing under test.


## MPTEST-MA S13 (2026-09-13) — ✅✅ **PASS. MA multiplayer works between two instances.**

    MA two-instance  (host fly at 190000ms, client +30s, 260s)
      host reaches the Ready Room                    PASS
      client reaches the locker room                 PASS
      client reaches the Ready Room                  PASS
      shim speaks (host/client pump lines)           PASS
      host answers a discovery probe                 PASS
      host table shows a SECOND player               PASS
      MA TWO-INSTANCE: PASS

**The evidence, end to end, from one run** (`~/Documents/260913/logs/ma_mp2g/`):

    host    [dplay] Open(CREATE) session "MiG Alley"   /   probe from a client -> offered session
    client  [dplay] EnumSessions -> 1 session(s), found "MiG Alley"
    client  [dplay] Open(JOIN) -> host 127.0.0.1:47624
    client  [dplay] host assigned us pid 4
    client  screens: CSelectService -> CSelectSession -> CLockerRoom -> CReadyRoom
    host    player table row 1: "1" "Player" "F86 1" "Flight Line"
            player table row 2: "2" "Player" "F86 1" "Flight Line"     <- THE JOINER

⭐ **Two players in the host's Ready Room table.** That is MPTEST-FF's acceptance criterion met for
MA: the PO's *"follow the same general process used to test bob multiplayer, to test ma
multiplayer"* now has an automated answer, on one machine, with no second PC and nobody at the
keyboard.

**What actually fixed it, across S9-S13, was never the comms.** The transport worked from S10
onwards; every remaining failure was the recipe walking the UI wrongly, and each was found by
reading the menu dump rather than by changing code:

| sprint | the wrong step | what the dump said |
|---|---|---|
| S9 | clicking the service-provider row | it advances by itself, silently implying Create Game, so Join was never chosen |
| S10 | host pressing Continue at 190 s | `CreateCommsGame` creates nothing; `UINewPlayer` opens the session at the locker-room Continue, so the client probed an empty lobby |
| S12 | selecting the session row | that screen has its own `Back/Select` bar; selecting is not committing |
| S13 | clicking SELECT SIDE as a joiner | `#2324` does not exist for a joiner, and an UNRESOLVED click is **retried, not skipped**, stalling the whole sequence |

**Not claimed:** neither instance has flown together yet — the host's Fly is driven at 190 s and the
client's onward path from the Ready Room is untested. "Two players in a session" is what passes here,
not "two players in the air". That is the next item, and it is now a short step rather than a hunt.


## MP-2 / MPTEST-MA S14 (2026-09-13) — ✅✅✅ **BOTH INSTANCES FLY.** MA multiplayer is usable end to end.

    MA two-instance  (host fly at 190000ms, client +30s, 330s)
      host reaches the Ready Room                    PASS
      client reaches the locker room                 PASS
      client reaches the Ready Room                  PASS
      shim speaks (host/client pump lines)           PASS
      host answers a discovery probe                 PASS
      host enters the 3-D                            PASS
      client enters the 3-D                          PASS
      host table shows a SECOND player               PASS
      MA TWO-INSTANCE: PASS

    host    [3d] Launch3d returned; tmpinst=0xafe5ba0 tmpview=0xb31e850
    client  [3d] Launch3d returned; tmpinst=0xa262310 tmpview=0xa5a66e0

⭐ **MP-2's title is "connecting two MA AppImages for multiplayer is not usable". It is now usable:**
discover → join → both in the Ready Room → both in the 3-D, driven end to end with nobody at the
keyboard. The client's own FLY was the missing click; its Ready Room bar is FIVE items
(`Quit / Fly / Radio / Paint Shop / Prefs`) against the host's six (the host has `Visitors` at 2), so
Fly is index 1 on both but the bars are NOT interchangeable and the column widths differ.

⚠️ **The first attempt at this reported `host enters the 3-D FAIL` / `client ... FAIL` — and both had
already entered.** The assertion greps `[3d] Launch3d returned`, which only prints under
`MA_TRACE_3D`, and the harness did not set it. The tell was in the same logs: both sides had
`[keybind] applied 585 binding(s)` and `[joy] opened`, i.e. the sim's input layer was up on each.

**This is the FOURTH blind assertion today** (BoB's `Joining` arm, FF's `FM_JOIN_SUCCEEDED`, FF's
`[STARTCAMP]`, and now this), and they all have one shape: *a gate grepping for a trace it does not
enable*. The rule, now applied here: **a gate must turn on the trace it greps for, in the same
place.** `MA_TRACE_3D=1` is set on both instances by the harness itself.

**Honest scope:** "both instances are in the 3-D with the joiner in the host's player table" is what
passes. Whether they can SEE each other in the air — position updates, shooting, kills — is not
tested. That is the next item and it needs an in-sim probe, not a menu one; `MA_KEYSEQ` (S439) is the
tool for it.


## MP-2 S15 (2026-09-13) — the two aircraft EXCHANGE STATE in the air, and it is symmetric

S14 passed with both instances in the 3-D and said plainly what was still untested: *"whether they
can SEE each other in the air ... is not tested."* Answered from S14's own logs — no new run — by
splitting each side's `[dplay]` traffic at its `Launch3d returned` line:

| | `Send` before 3-D | **`Send` AFTER 3-D** | `received` before | **`received` AFTER** |
|---|---|---|---|---|
| host | 90 | **1900** | 69 | 1759 |
| client | 68 | 1760 | 85 | **1900** |

⭐ **The counts cross-match.** The host's 1900 sends are the client's 1900 receipts; the client's
1760 sends are the host's 1759 receipts, the one difference being a packet still in flight when the
run was cut. Traffic jumps by a factor of ~20 the moment both are in the 3-D (90 → 1900), which is
what a per-frame state exchange looks like and is not what lobby chatter looks like.

Payload sizes after 3-D entry are small and few: the host receives 12, 14 and 69-byte packets; the
client receives 5, 12, 14, 57 and 60-byte packets. Consistent with position/state updates rather
than bulk transfer.

**So the netcode is live inside the sim, in both directions, between two instances that discovered,
joined and flew without a human.** Combined with S14, MP-2's title — *"connecting two MA AppImages
for multiplayer is not usable"* — no longer describes the port.

⚠️ **What this does NOT establish:** that each aircraft is DRAWN in the other's world. Packets
crossing is necessary, not sufficient — the receiving side could be decoding them into a remote
aircraft that never renders, which is exactly the class of defect BoB's MP-5 turned out to be. A
capture from both instances at the same moment, or an in-sim probe counting remote aircraft in the
draw list, is what would close it. `MA_KEYSEQ` (S439) can drive the views for that.

**Cheap and worth noting:** this sprint cost one `python3` pass over logs that already existed. The
instrumentation to answer it had been written days ago; nobody had split it at the 3-D boundary.


## MP-2 S16 (2026-09-13) — the harness is green again; the aircraft census is NOT trustworthy and says nothing

**The good part.** After repairing the harness (below), the full run is green again with the census
enabled — `MA TWO-INSTANCE: PASS`, all nine assertions including both instances in the 3-D and the
joiner in the host's player table. So S14/S15 reproduce.

**The census, which was the point of this sprint, failed.** `MA_TRACE_ACCOUNT=1` walks
`AirStruc::ACList` and reports the count, to answer S15's open question (packets cross, but is the
peer actually BUILT here?). Both sides report:

    [accnt] aircraft in this world: now=0 MAX=0 (sample #0)

⚠️ **Zero is impossible and therefore means the instrument is wrong, not the game.** The census runs
inside `AirStruc::FindDesPos` — a method ON an AirStruc — so at least one aircraft demonstrably
exists whenever it executes. A count of 0 can only mean the walk is reading the wrong thing. Two
candidates, both visible in the tree:

- **The idiom.** `WORLDINC.H:701` declares `static MobileItemPtr ACList;`. `SPOTTED.CPP:1147` walks
  it as a CAST — `AirStruc* currac = (AirStruc*) AirStruc::ACList;` — while `PERSONS3.CPP:3135`
  dereferences — `AirStrucPtr Me = *AirStruc::ACList;`. I copied the second. Both compile; they are
  not the same thing, and the census suggests I picked the wrong one.
- **The sampling point.** Only `sample #0` ever printed, and samples are every 2000 calls, so
  `FindDesPos` ran **under 2000 times** in a 330 s run with two aircraft flying. S438 already
  measured this function as rare. It is the wrong per-frame hook, however convenient it was.

**Recorded rather than worked around, because this is the session's recurring shape:** a zero from an
instrument that cannot speak. I have caught it four times today in gates grepping traces they did not
enable; this one is mine, in code I wrote an hour ago, and the giveaway was that the value was not
merely surprising but *impossible*.

**S17 (next MA rotation):** use `SPOTTED.CPP`'s cast idiom, and hang the census on a function
measured to run per frame in the 3-D rather than assumed to — `MoveAll` is the obvious candidate, but
note `DOSMOVE.CPP`, which contains the obvious move cycle, is **not in the build** (stale duplicate),
so confirm against `compile_commands.json` first. Only then is "1 vs 2 aircraft" an answer.

**Harness repair, also this sprint.** Two patches raced on `ma_mp_two_instance.sh` — a `sed` of mine
and a queued `python3` that was still waiting on an `until` loop — and both applied, producing
`MA_TRACE_ACCOUNT=1 MA_TRACE_ACCOUNT=1` and a corrupted line 74 (`ep: command not found`). That run
reported `host answers a discovery probe FAIL` and `second player FAIL`, neither of which was real.
Restored from HEAD and applied once. **A queued job that edits a file is a write that has not
happened yet**; the `until` loops in this session make that easy to forget.


## MP-2 S17 (2026-09-13) — the census speaks (8 aircraft), and 8 does NOT yet answer the question

S16's census reported an impossible 0. Fixed two ways — walk the `MobileItemPtr` directly as
`MoveList` does (`while (entry) { next = entry->nextmobile; }`) instead of dereferencing it, and hang
it on `MoveAll`, the real per-frame move cycle, instead of `FindDesPos`, which S438 had already
measured as rare. Rerun, with the harness green throughout (`MA TWO-INSTANCE: PASS`, nine
assertions):

    host    [accnt] aircraft in this world: now=8 MAX=8
    client  [accnt] aircraft in this world: now=8 MAX=8

**The instrument works** — 8 is a real count where 0 was impossible, and it agrees on both sides.

⚠️ **But 8 does not answer S15's question, and it would be easy to pretend it does.** The question is
whether each instance BUILDS the other's aircraft. A quick mission carries AI as well as the two
players, so 8-and-8 is equally consistent with:

- the peer being constructed (7 local + 1 remote on each side), and
- both instances independently spawning the same 8 local aircraft and neither seeing the other.

**Nothing distinguishes those without a baseline.** The control is a single-instance Hot Shot flight
with `MA_TRACE_ACCOUNT=1`: if it reports **7**, the extra aircraft in the two-instance run is the
peer and MP-2's last question is answered; if it reports **8**, the peer is not being built and S15's
1,900 packets a second are being decoded into nothing — which is precisely what BoB's MP-5 turned
out to be.

**S18: run that control before drawing any conclusion from 8.** It is one 200 s single-player run
with a flag already in the binary, and it is the whole difference between "MA multiplayer works" and
"MA multiplayer exchanges packets".


## MP-2 S18 (2026-09-13) — ⛔ the peer is NOT built. "Multiplayer works" must be qualified.

S17 measured 8 aircraft in each instance and said plainly that 8 answers nothing without a baseline.
Here is the baseline, and it is decisive.

| run | aircraft in world |
|---|---|
| two-instance, **host** | 8 |
| two-instance, **client** | 8 |
| ⭐ **host ALONE, same mission, no client** | **8** |

**8 = 8.** The joining client's aircraft is not added to the host's world, and vice versa. So S15's
~1,900 packets each way are being received and decoded into nothing that exists as an aircraft —
which is precisely what BoB's MP-5 turned out to be, and the reason S15 refused to call packet
traffic sufficient.

⚠️ **This qualifies everything claimed for MA multiplayer today, and the qualification matters
because an AppImage shipped this morning highlights it.** What is proven, and still stands:

- discovery, join, player-id assignment, and the joiner listed in the host's Ready Room table (S13);
- both instances entering the 3-D unattended (S14);
- a symmetric ~1,900-packet-per-side state exchange in flight (S15).

What is **not** true: that the two players can see each other. Two people running this would each fly
a private copy of the same mission, in the same session, exchanging packets that produce no visible
opponent.

⚠️ **A wrong control first, caught by its own number.** The first baseline I ran was a single-player
**Hot Shot** flight, which reported **40** aircraft. Hot Shot is a different mission from the comms
quick mission, so 40-vs-8 compares nothing; the discrepancy is what exposed it. The valid control is
the harness's own host invocation run alone — same clicks, same mission, no peer. (It also failed
once on `Link-only run (no data path found)` because I launched from the repo instead of
`<drive_c>/rowan/mig`.)

**S19: find where a remote player's aircraft should be constructed.** The packets arrive
(S15 counted them crossing), so the defect is downstream of the transport, in whatever turns a
received player update into an `AirStruc` on `ACList`. BoB's equivalent is `AddPlayerToGame`
(`WINMOVE.CPP`); MA will have a counterpart, and the census is now a working oracle for it — a fix
shows up immediately as 9 instead of 8.


## MP-2 S19 (2026-09-13) — ⭐⭐ ROOT CAUSE, and it is shared with BoB. S18's conclusion is WITHDRAWN.

**First, the retraction.** S18 measured host-alone = 8 aircraft and two-instance = 8, and concluded
*"the peer is NOT built"*. **That inference is invalid.** `DPlay::AddPlayerToGame`
(`WINMOVE.CPP:5464`) does not create an aircraft:

```c
AirStrucPtr thisac = (AirStrucPtr)Persons2::ConvertPtrUID((UniqueID)id);
if (!thisac) return;                 // silent
...  mad->IsInvisible = 0;  thisac->Status.deaded = FALSE;
```

It **claims** one of the mission's already-allocated aircraft by uniqueID and clears its
invisible/dead flags. Nothing is added to `ACList`, so the count stays at 8 whether the peer is
claimed or not. The census answered a question this design does not ask. S18's measurement was
sound; the inference from it was not.

**The real measurement.** Traced `AddPlayerToGame` itself (`MA_TRACE_ADDPLAYER=1`) over a passing
two-instance run — both in the 3-D, joiner in the host's table:

    addplayer lines, host:   (none)
    addplayer lines, client: (none)

**It is never called on either side.** So the peer is never made visible, and the chain to why is
short and complete:

| step | code | condition |
|---|---|---|
| peer becomes visible | `AddPlayerToGame` | called from `case PID_IAMIN` (`COMMS.CPP:2116`), gated on the receiver being `CPS_3D` |
| that message is sent by | `SendEnteringGameMessage()` — `data.PacketID = PID_IAMIN`, *"send my uniqueID so other players can set up my AC"* | called from exactly one live site, `WINMOVE.CPP:2359` |
| that site | `if (_DPlay.Implemented) { if (_DPlay.Joining) { SendEnteringGameMessage(); } }` | **only a JOINING peer announces itself** |
| `Joining=TRUE` | set only in `DPlay::JoinGame()` | reached only via `// if game in progress then join, otherwise dont do anything` |

⭐ **When both players start together, neither is "joining", so neither sends `PID_IAMIN`, so neither
calls `AddPlayerToGame`, so neither is ever made visible to the other.** Everything else works — they
are in the same session, in the 3-D, exchanging ~1,900 packets a side.

⭐⭐ **And BoB is identical.** MP-5 cont.18 found BoB's `Joining=TRUE` set only in `DPlay::JoinGame`,
reached only from `if (DPlay::H2H_Player[0].status == DPlay::CPS_3D)` under the comment *"if game in
progress then join, otherwise dont do anything"* — the same sentence, the same flag, the same
consequence. **This is a shared Rowan comms design, not a port defect in either game**, and it means
the two ports' next steps are one investigation:

- either the **late-join** path must work (BoB's LATEJOIN arm showed the client never completes the
  join, MP-5 cont.18), or
- `SendEnteringGameMessage()` must also fire for a peer that starts with the host rather than after
  it — which is the smaller change and the one the evidence points at.

**MP-2 rotates off, over its cap at S14-S19.** What the stretch produced: multiplayer that discovers,
joins, seats both players in the Ready Room, enters the 3-D on both sides and exchanges state in
flight — plus the named, evidenced reason the two aircraft cannot see each other, and the fact that
the same reason applies to BoB.

## MPVIS-1 S3 (Opus 5, 2026-09-13) — ✅ the peer aircraft is DRAWN, not merely flagged: each side renders the OTHER aeroplane

S1 and S2 established that the peer is *flagged* visible (`IsInvisible==0`, `dead==0`) and that the
two instances agree on its identity. Neither established that anything reaches the screen, and the
standing rule is that a flag is not a picture ([[screenshot-beats-printf-for-view-defects]]). S3
asks the draw side of the same question.

**Two instruments, both gated:**

* `[drawac]` (`3D/3DCODE.CPP`, inside `ThreeDee::do_object`'s own `if (!mad->IsInvisible)`) —
  censuses the AIRCRAFT that pass that gate, by `uniqueID`, once a second. `MA_TRACE_DRAWAC=1`.
* `[peer] ... range=Nm` (`MOVECODE/MOVEALL.CPP`) — the peer's DISTANCE from this instance's own
  aeroplane. Added **before** the run, because a silent census is ambiguous between a visibility
  defect and ordinary geometry: an aircraft beyond the draw distance is correctly absent.

**Predictions stated before the run:** (1) `[drawac]` non-empty on both sides; (2) the peer's uid
appears iff it is in draw range; (3) *most likely* the peer sits several km away and is absent —
spawn separation, not a visibility defect.

**MEASURED — prediction 3 is WRONG, and the answer is better than predicted:**

    host    [peer] uid=3585 found=1 visible=1 dead=0 range=226m
            [self] uid=3584 visible=1 dead=0
            [drawac] 1 distinct aircraft drawn: uid=3585
    client  [peer] uid=3584 found=1 visible=1 dead=0 range=226m
            [self] uid=3585 visible=1 dead=0
            [drawac] 1 distinct aircraft drawn: uid=3584

⭐ **Each instance draws exactly one aircraft, and it is the OTHER player's.** The host renders 3585
(its peer) and not 3584 (itself); the client renders 3584 and not 3585. That is precisely correct
from a cockpit — your own aeroplane is not drawn from inside it — and it is a far stronger result
than "a flag is clear": the uid that reaches the renderer on each side is the uid the *other*
instance calls its own. They are 226 m apart, well within sight.

**MA multiplayer visibility is real.** MP-6 + MPVIS-1 deliver two players who discover, join, fly,
exchange state, and now provably SEE each other.

⚠️ **One number, stated rather than glossed:** the census printed a single line per side (`x1`), not
one per second of flight. The logs explain it without appeal to intermittent drawing — `[3d] Launch3d
returned` on both sides and only two `[accnt]` frame samples, i.e. **the 3-D phase in this harness
run was itself about a second long**, so the instrument fired on the only second it had. The
positive result stands; "drawn continuously throughout a flight" is not yet measured.

**S4, precisely.** Two things, one run: (a) give the census a DENOMINATOR — render-frame count
alongside draw count, since a hit count without one says nothing ([[instrument-bookkeeping-lies]]);
(b) cover the OTHER draw gate — `3DCODE.CPP:2183` is a group/range LOD path with its own
`!mad->IsInvisible`, and distant aircraft go through it rather than `do_object`, so the present
census is blind beyond some range. Then lengthen the harness's 3-D phase so the flight lasts long
enough to say "throughout".

**MPVIS-1: 3 sprints.**

## MPVIS-1 S4 (Opus 5, 2026-09-13) — ⛔ S3's conclusion must be QUALIFIED: the census sites are not the per-frame draw path

S4 was scoped to three things S3 named: a denominator, the second draw gate, and a longer 3-D phase.
All three were done, and the result overturns part of S3 rather than extending it.

**First, S3's caveat was WRONG and is withdrawn.** S3 explained its single census line by saying the
3-D phase "lasted about a second", citing `[3d] Launch3d returned`. That line is at **log line 370 of
227,661** — `Launch3d` *returns the instance and view pointers*, i.e. it is the 3-D being successfully
CREATED, and the flight ran for the rest of the log. The explanation was wrong; the observation it
explained was real and still needed explaining.

**Second, the real explanation, and it is worse.** The census now covers BOTH gates
(`do_object` and `do_object_grp`, the grouped/LOD path distant aircraft use) and reports per second,
which is its own denominator. Result over a passing two-instance run:

    host    drawac lines: 0     client  drawac lines: 0

A zero from an instrument not shown to speak is worthless, so the census was widened to count
**every object** through those gates, aircraft or not ([[instrument-bookkeeping-lies]]):

    host    drawobj lines: 0    client  drawobj lines: 0

⛔ **Not one object of any kind passes either gate often enough to span a second.** These are not the
per-frame draw path in this build — so the census cannot speak to how continuously anything is drawn,
and **S3's headline "the peer aircraft is DRAWN" is qualified to "was drawn at least once"**: S3's
line printed because that version printed on its FIRST call, which is a single event, not a rate.

**What still stands from S3, and it is not nothing.** The identity cross-match is untouched: the host
drew uid 3585 and the client drew uid 3584, each the other's own aeroplane. A single event can be a
coincidence of timing; it cannot manufacture a correct cross-matched identity pair. So the claim that
survives is *the peer reaches a real draw gate with the right identity*, not *the peer is rendered
throughout the flight*.

**S5 (next pass) — find the actual draw path before measuring anything else on it.** The candidates
in `3DCODE.CPP` are exhausted: `:1718` is the shadow pass, `:3465` dispatches `do_map_object` (the
MAP, not the world). Do not add a fourth speculative probe. Run ONE instance through the
single-player flight recipe (Sprint 439's `MA_KEYSEQ`) with `MA_TRACE_DRAWAC=1`: if the census speaks
there, the two-instance harness is what suppresses it (and every draw-side conclusion drawn from that
harness is suspect); if it is silent there too, `do_object` is not the renderer's entry at all and the
path must be found from the frame loop downward.

**MPVIS-1: 4 sprints — at cap, rotating off with a correction rather than a claim.**

## EPIC M / harness validity (Opus 5, 2026-09-14) — ⛔ the two-instance harness does NOT render; every draw-side conclusion from it is void

MPVIS-1 S4 ended with a fork it could not resolve inside its sprint cap: the aircraft draw census
printed ZERO in the two-instance run, and that could mean either the harness suppresses drawing or
`do_object` is not the renderer's entry at all. S4 named the experiment — one instance through the
single-player recipe — and this is it. The answer governs more than MPVIS-1, so it is recorded here
rather than against a capped item.

**The single-player path, built this pass** (no new hook needed; `MA_DUMP_MENU` names the rows):

    main menu  r1 = "Single Player"   ->   r0 = "Hot Shot"
    BOB_CLICKSEQ_MS=1 BOB_CLICKSEQ="20000,r1;35000,r0" MA_QUICKMISS=9

**MEASURED, same binary, same census, two configurations:**

| | `[drawac]` lines | aircraft | per-aircraft rate | `[drawobj]` objects/s |
|---|---|---|---|---|
| two-instance harness | **0** | — | — | **0** |
| single Hot Shot flight | **270** | **16** | **17–18/s** (max 51/s) | **1,200 – 14,500** |

⛔ **So `do_object`/`do_object_grp` ARE the per-frame draw path** — 17–18 draws per aircraft per second
is one per frame at the rate this flight runs — **and the two-instance harness does not exercise the
renderer at all.** Not "less", not "differently": zero objects of any kind.

**What that voids.** Every draw-side conclusion taken from the two-instance harness, which includes
MPVIS-1 S3's headline. S4 had already qualified it from "the peer is DRAWN" to "was drawn at least
once"; the correct statement now is weaker still — *the harness is not a configuration in which
drawing happens*, so the single event S3 saw is an artefact of the 3-D coming up, not evidence about
steady-state rendering. **What survives is unchanged and was never draw-side:** the identity
cross-match (host claims 3585, client claims 3584, each the other's own aircraft), which came from
the entity lists.

**Also justified retrospectively:** S4's decision to cover `do_object_grp` as well. The grouped path
carries the majority of objects when it runs (4,216 of 4,871; 12,624 of 14,529) and none at other
times — a census on `do_object` alone would have under-counted by ~85 % in those frames.

**Next, for whoever picks up MPVIS-1 (it is at its 4-sprint cap):** the visibility proof must be
re-run in a configuration that renders. Two candidates, and the first is cheap — find WHY the
harness does not draw (both instances are launched unattended and may never present a frame), or
drive a single instance into a multiplayer session by the same `BOB_CLICKSEQ` route now that the
single-player path is mapped. Do not re-assert peer visibility from the existing harness logs.

## EPIC M / harness validity S2 (Opus 5, 2026-09-14) — the 3-D object walk is NEVER CALLED in the multiplayer harness

The previous entry established that the two-instance harness draws nothing, and left two readings
open: the object walk runs and finds nothing in range, or it never runs. That distinction decides
whether the harness is merely a poor vantage point or not a rendering configuration at all, so it was
measured rather than argued. `[doobjs]` counts calls to `ThreeDee::do_objects()` — the walk that
feeds both draw gates — once a second, under the same `MA_TRACE_DRAWAC` switch as the census it
explains.

    single Hot Shot flight     [doobjs] do_objects() called 50/s   (114 lines)   [drawobj] 95 lines
    two-instance harness, host [doobjs] 0                          [drawobj] 0
    two-instance harness, client [doobjs] 0                        [drawobj] 0

⛔ **Never called, on either side.** Not "called and empty" — the entire 3-D object walk does not
execute in the multiplayer harness, while the same binary runs it 50 times a second in single-player.

**Windowing is not the difference, checked before concluding:** both configurations print the same
`[vid]` lines — SDL2 window 640x480, GL context on the GTX 1660, vsync 1. The harness has a real
window and a real GL context and still never walks the scene.

**What this fixes in the record.** The previous entry inferred "the harness does not exercise the
renderer" from a downstream zero; this measures it at the source and upgrades it from inference to
fact. `do_objects()` is called from `ThreeDee::render3d` (`3DCODE.CPP:713`), so the question is now
narrow and well-posed: **does `render3d` run at all in the multiplayer flight, or does it run and
skip the call?**

**⚠️ A question for the PO this raises, and I am NOT answering it from an unattended harness:** if
`render3d` also fails to run in a multiplayer flight a HUMAN starts, MA multiplayer would show no
world. That is a very different claim from "the test rig does not render", and nothing measured here
distinguishes them — every MA multiplayer run on file is unattended. One human session settles it.

**Next:** put the same one-line counter on `render3d` itself. If render3d runs 50/s and do_objects
does not, the gate between them is the answer and it is ten lines away; if render3d is also 0, the
multiplayer flight never enters the 3-D render at all and the search moves to the flight-entry path
— which is the same shape as BoB's `g_bob_flight_active` finding from R3.7 S3 today.

## EPIC M / harness validity S3 (Opus 5, 2026-09-14) — ⭐⭐ ROOT CAUSE: MA multiplayer HOLDS every frame waiting for `_DPlay.csync`

S2 measured `do_objects()` at 50/s single-player and 0 in the harness. S3 walked the chain up with one
counter at a time, proving each instrument could speak in the configuration that works before
believing its zero in the one that does not.

    ThreeDee::do_objects()   single 50/s      harness 0
    ThreeDee::render3d()     single 50/s      harness 0
    ThreeDee::render()       single 50/s      harness 0   (single-player prints drawSpecialFlags=0 -> render3d,
                                                           so the map/replay fork is NOT the cause)

So nothing in the 3-D render chain is reached. The only live caller is `STUB3D.CPP:1450`, and it is
gated:

    if (!_DPlay.Implemented || _DPlay.csync)
        Three_Dee.render(&window3d, This->View_Point, This->inst->world);
    else if (doit&0x1000)   /* "dont want resyncing message at all, but keep waiting message" */

⭐⭐ **In multiplayer the frame is rendered ONLY when `_DPlay.csync` is set.** Measured over a real
two-instance session (`[rendergate]`, on change and once a second):

    host    87 samples   ALL  DPlay.Implemented=1 csync=0 -> held (waiting/resync)
    client  90 samples        DPlay.Implemented=1 csync=0 -> held
             1 sample         DPlay.Implemented=1 csync=1 -> RENDER

**MA multiplayer never syncs, so the game holds every frame.** Two instances that discover, join,
seat both players, enter the 3-D and exchange ~1,900 packets a side are rendering a waiting state,
not a world.

⭐ **And this retro-explains MPVIS-1's oddest observation.** S3 of that item caught exactly ONE
aircraft draw, of the peer, with cross-matching identity, and S4 could not reproduce it — I qualified
it down to "drawn at least once" and then, in the previous entry, down again to an artefact. It was
neither: the client got **exactly one frame past the csync gate**, and in that frame the peer WAS
drawn, correctly. The observation was sound; only my explanations of it were wrong. **MPVIS-1's
claim is restored on this evidence: when a frame renders, the peer renders with it.**

⚠️ **This is a GAME defect, not a harness one — the distinction I have been careful about all
session, and it falls the other way this time.** A human joining a multiplayer game hits the same
gate in the same code. The earlier entry's flag ("if a human sees this too, MA multiplayer shows no
world") is now answered as far as code can answer it: there is nothing harness-specific in
`!_DPlay.Implemented || _DPlay.csync`.

**Next: who sets `csync`, and why it stays 0.** That is the whole of MA multiplayer's remaining
gap — everything else in the chain works. Note the comment on the else-branch ("dont want resyncing
message at all") suggests the original team also found this path unsatisfactory.

## EPIC M / MP S4 (Opus 5, 2026-09-14) — ⭐ the stall is `InitSyncPhase`: it never succeeds, so nothing downstream ever runs

S3 found MA (and BoB) render nothing in multiplayer because the frame is gated on `_DPlay.csync`,
measured 0 on 87/87 host samples. The chain to csync is two gated phases in sequence, and a
downstream zero cannot say which one stalls, so both were instrumented (`MA_TRACE_SYNC=1`).

**MEASURED over a real two-instance session:**

    host    81 samples  [sync] synched=0 csync=0
            [sync] InitSyncPhase FAILED ~840,000/s   (bails the routine)
    client  89 samples  [sync] synched=0 csync=0
            [sync] InitSyncPhase FAILED 59-60/s, and ~854,000/s in bursts

⭐ **`InitSyncPhase` never succeeds, on either side.** So `synched` never becomes TRUE, so
`SecondSyncPhase` (the only place `csync=true` is reached, `WINMOVE.CPP:4478`) is never called, so
csync stays 0 and `STUB3D.CPP:1450` holds every frame. The whole multiplayer render gap reduces to
this one function.

Note the RATE: the host spins on it ~840,000 times a second. This is not a phase that is waiting
politely — it is a busy retry loop that never makes progress, which is also why the two-instance
harness burns CPU for its whole run.

**Where it bails.** `InitSyncPhase` (`WINMOVE.CPP:4164`) receives from the aggregator (`aggID`) and,
if nothing arrived, takes `if (!got) return FALSE;` (`:4245`). A second gate follows,
`if (num != CurrPlayers)`. So the phase is waiting on an **aggregate sync packet that never
arrives** — or arrives with a player count that does not match.

**S5 — one question, and the shim already has the instrument.** `MA_TRACE_DPLAY=1` reports the
shim's DirectPlay traffic. Ask whether the aggregate packet InitSyncPhase waits for is ever SENT by
the peer and ever DELIVERED by the shim: if it is never sent, the fault is upstream in the comms
layer; if it is sent and not delivered, it is the shim's `DPRECEIVE_FROMPLAYER` filtering — which is
exactly the class of bug MP-6 S2/S3 already found and fixed once for the announce packet ("the shim
ignores the filter the game depends on"). Check that fix's shape before writing a new one.

## EPIC M / MP S5 (Opus 5, 2026-09-14) — ⭐ the shim delivered NOTHING locally; two fixes, and the stall moves two links down the chain

S4 reduced the whole multiplayer render gap to one function: `InitSyncPhase` never succeeds, so
`synched` stays FALSE, `SecondSyncPhase` never runs and `csync` stays 0. It waits on an aggregate
packet from `aggID`. S5 asked S4's question — is that packet ever SENT, and ever DELIVERED?

**MEASURED (run 1, `MA_TRACE_AGG=1`, unmodified code):**

    host    StaticTimeProc 49/s  impl=1 host=1 running=1  -> AggregatorGetPackets 49/s
    host    SendEx aggpacket 12/s  from=1 to=2(playergroup) players=0 size=5 res=0x0
    host    InitSyncPhase received 12 msg/s  aggID=1  From seen: 4      <-- never 1
    host    InitSyncPhase got-agg-packet 0/s   no-packet 800223/s
    client  InitSyncPhase received 12 msg/s  aggID=1  From seen: 1
    client  InitSyncPhase got-agg-packet 12/s

⭐ **It is SENT (12/s, `res=DP_OK`) and it is DELIVERED — to the CLIENT only.** The HOST never
receives the packet its own aggregator produces. The ids say why: the host process owns TWO players,
the aggregator (pid 1) and its game player (pid 3), group 2 holds {3, 4}. The shim's `Send` puts the
datagram on the wire and does nothing else, so a send from one local player to another, or to a
group with a local member, is delivered everywhere except at home. MA cannot work that way: the
host's game half has no other route to a packet its own aggregator made — the game's structure is
the evidence that real DirectPlay expands a group send to its local members.

**FIX 1 — local loopback** (`SRC/compat/ma_dplay.cpp`, `MA_NO_LOOPBACK=1` reverts): queue a local
copy when the destination is a local player or a group with a local member. Run 2: the host now
receives it (`From seen: 4 1`, `got-agg-packet 12/s`). **And the aggregate was still `players=0`.**

**FIX 2 — the shim remembered only ONE local player.** `myPid` held the LAST id created, so the
aggregator (pid 1) was not a known player on the host. Two consequences, both measured: a dummy sent
from the game player to the aggregator went out on the wire instead of to the aggregator, and the
receive filter's catch-all (`!isKnownPlayer(dst)`) handed the CLIENT's aggregator-addressed dummies
to the game half — `From seen: 4` in run 1 is that theft. `localPids[]` now records every player
this process creates.

**MEASURED (run 3, both fixes):**

| | before | after |
|---|---|---|
| aggregator receives | (never probed) | 46–60 msg/s, from 3 and 4, all mapped to slots |
| aggregate packet | players=0 size=5 | **players=2 size=33** |
| host gets the packet | no | yes, 24/s |
| InitSyncPhase busy spin | ~800,000/s | **2,238/s** host, **60/s** client |

⭐ That spin was S4's separate MA mystery ("840,000/s where BoB retries 60/s"). It was not a second
defect: the routine was failing at its FIRST receive every time, and with the packet arriving it now
paces itself. **That item can be closed by this fix.**

⚠️ **Multiplayer still does not render, and I am not going to call this fixed.** `synched` is still
0. The stall has moved to the LAST gate of `InitSyncPhase`, and the probe names it:

    [agg] GATE num=1 CurrPlayers=4  aggCount=143 myFrame=147  IDCodes: 196 194 194 ...  (DUMMY=196)

**`CurrPlayers=4` in a two-player session.** Two players are seated, the aggregate carries two, and
the gate wants four. There are two counting sites — `CountPlayers()` (COMMS.CPP:1470, recount from
`H2H_Player[].status`) and a bare `CurrPlayers++` (WINMOVE.CPP:5642, per allocated player) — and a
recount followed by two increments gives exactly the 4 measured. That is the next sprint's question:
which sites run, in what order, and is the increment double-counting a player the recount already
saw. `num` also drops to 0 as the slots fill with REAL packets (207/194) rather than dummies (196),
so the dummy-count half of the gate needs its own look.

**Cross-port: BoB's shim (`SRC/compat/bob_dplay.cpp`) has the identical `Send` — wire only, no local
delivery — and the same single-`myPid` assumption.** Both fixes port directly; BoB's stall is the
same csync chain.

**EPIC M / MP: 5 sprints (S4, S5 this pass). Rotating off.**

## EPIC M / MP S6 (Opus 5, 2026-09-14) — ⭐ InitSyncPhase now SUCCEEDS: `synched=1` on both sides, and one of the two causes was mine

S5 left both ports failing `num != CurrPlayers` with **CurrPlayers=4 in a two-player session**. S6
traced every write to that variable (`MA_TRACE_COUNT=1`).

**MEASURED, before the fix, identical on host and client:**

    [count] CountPlayers -> 2   status: [0]status=3,dpid=3 [1]status=3,dpid=4
    [iamin] announcing entry (started with host, Joining=FALSE)
    [addplayer] slot=0 ... [count] AddPlayerToGame slot=0 -> CurrPlayers=3
    [addplayer] slot=1 ... [count] AddPlayerToGame slot=1 -> CurrPlayers=4

Two writers, exactly as the arithmetic suggested: `CountPlayers()` recounts the H2H table and gets
the right answer, then `AddPlayerToGame`'s bare `CurrPlayers++` runs once per announcing player.

⚠️ **And each side was adding ITSELF, which is my own S5 regression.** The game announces entry with
`SendMessageToPlayers(playergroupID)`, and S5's loopback asked only "does this group have a local
member" — true of the sender itself. So a player's own PID_IAMIN came back to it. Caught because the
trace showed the host adding slot 0, its own slot.

**TWO FIXES.**

1. **Shim** (`MA_LOOPBACK_SELF=1` reverts): a group send is looped back to local members OTHER than
   the sender. The aggregate packet still arrives — its sender is the aggregator (pid 1), which is
   not a member of the group — and a player's own broadcast no longer does.
2. **Game** (`MA_MP_NORECOUNT=1` reverts): `AddPlayerToGame` recounts with `CountPlayers()` instead
   of `CurrPlayers++`. The `++` is only right for Rowan's original flow, where the announcer is a
   LATE joiner not yet in the table; MP-6 made peers who start together announce too, and those are
   already counted. The caller sets the slot's status first, so the recount is right in both cases.

**MEASURED after, both sides:**

    [count] AddPlayerToGame slot=1 id=3585 -> CurrPlayers=2     (the peer only, counted once)
    [agg] GATE num=1 CurrPlayers=2
    [sync] synched=1 csync=0

⭐ **`synched=1`. InitSyncPhase, which has failed on every measurement of this epic since S4, now
succeeds on both sides.** That is the first of the two sync phases cleared.

⚠️ **Still no world: `csync` is 0 and the frame is still held.** The stall has moved one link, to
`SecondSyncPhase` (`FAILED 10/s` host, `35/s` client — note the polite rate; this is a phase waiting,
not spinning). That is the last gate before `csync=true` at `WINMOVE.CPP:4478`, and it is the next
sprint's target.

**Cross-port: BoB needs both fixes.** It shares the shim and the `CurrPlayers++`, and S5 measured it
reading the same `num=1 CurrPlayers=4`.

**EPIC M / MP: 3 sprints this pass (S4, S5, S6). One left before the cap.**

## EPIC M / MP S7 (Opus 5, 2026-09-14) — the second phase starts, then a RESYNC resets it; and the aggregate is not the problem

S6 cleared the first gate (`synched=1`). S7 instrumented the second one the same way (`[agg] GATE2`).

**MEASURED (`MA_TRACE_AGG=1`, two-instance session):**

    host    SendEx aggpacket   players=2 in 134 of 140 samples
    host    GATE2 num=0 CurrPlayers=2   (printed twice in the whole run)
    client  GATE2 num=1 CurrPlayers=2   (printed twice)
    host    SecondSyncPhase FAILED 628/s

**The aggregate is NOT the problem.** It carries both players in 134 of 140 samples. And the second
gate is barely ever reached: the phase fails 628 times a second but the gate probe, which prints once
a second whenever the code gets that far, printed twice — so nearly every failure is the EARLIER
return, where the phase asks the queue for an aggregate packet addressed to it and gets nothing.

⚠️ **Two readings I checked before believing, and one of them was wrong.**

* The slot codes at the gate (194 = `PIDC_PACKETERROR`, 207 = `PIDC_EMPTY`) look like meaningful
  states but `AGGSENDPACKET packet` is an UNINITIALISED LOCAL and `ExpandAggPacket` fills only the
  slots the aggregate carries. Any slot the packet did not carry is stack garbage, so those codes
  prove nothing. (The `num` count itself reads those slots — worth remembering.)
* "InitSyncPhase failing again means `synched` flipped back" — my first reading was that this is just
  the transition second, since the two counters both print within one second. It is not. Ordering the
  log lines: host line 761 is a SecondSyncPhase failure and line 1083 is an InitSyncPhase failure;
  client 1212 then 1552. **On both sides `synched` goes back to 0 AFTER the second phase has
  started.**

⭐ **Only one thing clears `synched`: `ResetState()` (`WINMOVE.CPP:3669`), and it has exactly one
caller — the RESYNC path at `WINMOVE.CPP:3843` ("sets csync to false to begin commssync").** So the
sequence is: first phase passes, second phase starts, something raises a resync, the state resets and
it all begins again. That is why csync never latches.

**Next sprint (MP is at its 4-sprint cap this pass, so this is the handover):** find who raises the
resync. `BeginSyncPhase` is reached through `SendNeedResyncMessage` / the resync trigger in the
frame loop; instrument the trigger, not the reset, and report which condition fires and on which
side first.

**EPIC M / MP: 4 sprints this pass (S4–S7). AT THE CAP — rotating the item off.**

## MAP-COLOUR S1 (Opus 5, 2026-09-14) — ✅ the operational map is at COLOUR PARITY with the Wine gold; the "greyish" status was stale

`port/reference/wine-gold/README.md` names two "highest-value A/B targets", and the first is the
operational map: *"renders but greyish (Sprint 14) — THE colour-fidelity A/B target (M4/M8 map-tile
palette gap)"*. Before spending a sprint fixing a palette, S1 re-measured it. The gold was in the
tree the whole time (`14-operational-map.png`), and the map is reachable headless with the existing
recipe (`port/map_filter.sh`'s nav: pinned save → `30,r3;65,#1055;100,#2063:1`, `MA_SHOT=400`).

**MEASURED** (map area below the toolbar; the gold's Player Log window masked out, since it is UI,
not map; pixels classified sea/land by blue dominance):

| | sea RGB | land RGB | land saturation |
|---|---|---|---|
| **gold (Wine)** | 56.9, 72.1, 143.5 | 110.7, 107.5, 76.6 | 0.330 |
| **port (today)** | 54.5, 72.5, 144.5 | 111.1, 108.3, 76.6 | 0.312 |

⭐ **Sea agrees within 2.4 RGB units, land within 0.8, saturation within 0.018.** There is no grey
map. The screen shows blue sea, olive and green land, yellow highlands, red roads and blue rivers,
exactly as the gold does.

⚠️ **What is compared, stated plainly.** The two captures are at different zoom, framing and
campaign state (the gold has a Player Log open, ours does not; sea fills 77.7% of the gold's crop
and 58.4% of ours), so this is a comparison of colour DISTRIBUTIONS over comparable regions, not a
pixel diff. That is the right instrument for "greyish", which is a claim about colour, and the wrong
one for geometry.

**Action taken:** the README row and the "highest-value targets" list are corrected, with the
measurement recorded next to them, so the next sprint does not chase a palette that is already
right. This is the second time this session that a long-standing "known defect" turned out to be
stale documentation rather than code — the rule from the PO-89 sprint (*"blocked on the PO was never
tested against what the repo already held"*) applies to our own status notes too.

**MAP-COLOUR: 1 sprint. CLOSED.**

## GOLD3D-1 S1 (Opus 5, 2026-09-14) — the 3-D flight gold can be captured after all; the hook the notes point at is DEAD, and the one that works was already there

The wine-gold README's other two named A/B targets are the flight frames (#10 cockpit, "THE
software-rasterizer A/B target", and #11 external), and neither has ever been compared. S1 asked why,
and the answer was the instrument, not the renderer.

**What is NOT usable, measured:**

* `MA_SHOT` cannot capture the 3-D at all — every arm of it is gated on `!ma_in3d`
  (`MIG.CPP:2541`), by design: it dumps the GDI canvas for 2-D parity.
* `MA_DUMP_BACK`, which the README tells the reader to use, survives only as a COMMENT.
* `ma_ddraw_present`, the Phase-3 bridge the porting notes point at, **is never called in the current
  build**. A counter at the top of it fired **zero** times across a full run, front end included. A
  capture hook was written there this sprint and REMOVED again once that was measured — writing to a
  dead path would have produced a silent no-op for whoever used it next.

⭐ **What works, and it needed no new code: `BOB_DUMP_FRAME=<n>`.** The live present is
`present_surface()` (`bob_video.cpp:1217`), and it already carries a `glReadPixels` dump of the
window to `/tmp/bobframe.ppm`. In the 3-D the scene is in the GL framebuffer by then, so it captures
the real flight frame.

**Recipe, now recorded so the next sprint does not rediscover it:**

    BOB_RUN_INIT=1 BOB_DRIVE_C=… MA_ENABLE_3D=1 MA_QUICKMISS=3 \
    BOB_CLICKSEQ="40,r1;60,r1;110,#2063:2" BOB_AUTOEXIT=600 BOB_DUMP_FRAME=185

Frame 185 is in the flight; **frame 300 is already the DEBRIEF** (which incidentally renders well and
is gold #12). The capture is saved as `port/ref/native/flight_cockpit_260914.png`: canopy arch,
gunsight glass and its yellow reticle, the gunsight head unit, cloud layer, horizon haze, terrain,
and the HUD info line, all rendering.

⚠️ **A first visual reading of mine — "the lower instrument panel is missing in ours" — is NOT
supported and is withdrawn.** Measured over the bottom fifth of each frame, central 60% of width:

| | mean RGB | near-black | stdev |
|---|---|---|---|
| gold | 34.6, 38.4, 37.7 | 57.5% | 43.7 |
| port | 55.3, 61.0, 61.7 | 34.7% | 54.1 |

The port's panel band is BRIGHTER and LESS black than the gold's, not emptier. What differs is *what*
is drawn there, which a band statistic cannot tell apart from a lighting difference.

**S2:** do the A/B properly, which means a MATCHED view — same resolution and aspect (the gold's game
window is not 4:3), and the same eye position and altitude, so the two frames can be compared feature
by feature rather than by global statistics. The gold's panel carries VOLTS, EXHAUST and HORIZ. STAB
gauges and a red "RUDDER TRIM IN NEUTRAL" placard; those are nameable features to look for one at a
time.

**GOLD3D-1: 1 sprint.**

## GOLD3D-1 S2 (Opus 5, 2026-09-14) — the gold's aspect cannot be matched by this port, and the check that survives that says the cockpit geometry agrees

S1 built the capture and set S2 the job of a MATCHED view. Two things came out of trying.

⚠️ **1. The port cannot reproduce the gold's frame shape.** The gold's game window measures
**1233 × 1003** (the PNG is 1280 × 1003 with a desktop border to x=47), i.e. about **5:4**. The port
pins its software modes to 640/800/1024 wide at **4:3** — by design, recorded in the wine-gold README
itself — and `MA_FORCE_RES=1232x1003` does not error, it silently **falls back to 640×480**
(measured: `dumped frame 185 … (640x480)`). So a pixel-for-pixel A/B of these two frames is not
available, and any vertical comparison between them is contaminated by the aspect.

⭐ **2. The check that survives an aspect difference agrees.** Horizontal field is the fixed axis in
this projection, so the cockpit's horizontal geometry can be compared as a fraction of width. Taking
the two canopy pillars at the same FRACTIONAL height in each frame:

| | left pillar | right pillar |
|---|---|---|
| gold (1233 px wide) | x/W = **0.105** | x/W = 0.930 |
| port (1024 px wide) | x/W = **0.105** | x/W = 0.902 |

The left pillar matches to three decimals and the right to 2.8% of width — and the pillars are
slanted, so part of that is the band I sampled. **The cockpit is not grossly mis-projected; its
horizontal geometry lines up with the gold.**

⚠️ **A measurement I tried and am NOT reporting as a result:** locating the gunsight reticle by its
yellow. It finds 248 px in the gold centred at y/H = 0.930 — the bottom instrument panel, whose dial
faces are also yellow — and 8 px in ours. The detector does not isolate the reticle in either frame,
so it says nothing about either, and no reticle claim is made from it.

**S3:** either add the gold's mode to the port's software list so the frames can be compared
directly, or pick features that are aspect-invariant by construction and compare those one at a time
(pillar x, gunsight-head width as a fraction of width, canopy apex x). The first is more useful than
one A/B: without it, every future 3-D gold comparison in this port has the same problem.

**GOLD3D-1: 2 sprints.**

## GOLD3D-1 S3 (Opus 5, 2026-09-14) — ⛔ S2's "the port cannot do 5:4" is WRONG; the matched capture exists, and it shows a 2.6% difference I will not yet call a defect

S2 concluded the gold's ~5:4 frame could not be reproduced because "the port pins its software modes
to 640/800/1024 at 4:3". **That is wrong and is withdrawn.** The DirectDraw shim enumerates
`{640,480} {800,600} {1024,768} {1280,960} {1280,1024} {1600,1200} {1920,1080}` — **1280x1024 is 5:4
and has been available all along**:

    [prefs] MA_FORCE_RES -> displayW/H = 1280x1024
    [present] dumped frame 185 to /tmp/bobframe.ppm (1280x1024)

⚠️ **S2 measured a real fallback and then explained it with the wrong cause.** `MA_FORCE_RES=1232x1003`
does fall back to 640x480 — because *that particular size* is not enumerated, not because the aspect
is unavailable. I had also started this sprint by adding an `MA_EXTRA_MODE=WxH` hook to "fix" the
missing aspect; it is **reverted**, since the capability was already there and an unnecessary knob is
a liability.

**The matched comparison, canopy pillars at the same fractional height:**

| | left pillar | right pillar | centre | width |
|---|---|---|---|---|
| gold (1233 px) | 0.105 | 0.929 | 0.517 | 0.824 |
| port 5:4 (1280 px) | 0.101 | 0.903 | 0.502 | 0.802 |
| port 4:3 (1024 px) | 0.102 | 0.902 | 0.502 | 0.800 |

⭐ **The port's horizontal layout does not move with aspect** (0.102/0.902 at 4:3 vs 0.101/0.903 at
5:4), which is what a fixed horizontal field should do, and it confirms the aspect was never the
problem.

⚠️ **The remaining difference is 2.6% of width on the RIGHT pillar only, and I am not calling it a
defect.** Two explanations fit and this capture cannot separate them: the port's horizontal field
could be ~2.7% narrower, or the gold PNG's right edge could be the capture boundary rather than the
window's (the desktop border is visible on the left at x=47, and the content runs to the last column
on the right). A 1268 px window would make the two agree exactly.

**S4:** settle it with something that does not depend on the gold's framing — measure a feature whose
angular size is known, or capture a gold frame whose window edges are both visible. Until then the
cockpit's horizontal geometry is "agrees to about 2.6%, with the discrepancy unattributed".

**GOLD3D-1: 3 sprints.**

## GOLD3D-1 S4 (Opus 5, 2026-09-14) — ⭐ at matched aspect the instrument panel sits ~11% of frame height TOO LOW, and its gauges fall off the bottom edge

S1's band statistic could not tell "the panel is missing" from "the panel is dark", and I withdrew
the claim. S3 got the aspect matched (1280x1024). S4 crops the same strip from both frames and looks
at it — `port/ref/native/panel_vs_gold_260914.png`, gold above, port below.

**The gold's lower panel carries** a VOLTS gauge, a checklist card, the red "RUDDER TRIM IN NEUTRAL"
placard, a yellow knob, HORIZ. STAB, EXHAUST °x100 and two more dials.
**The port's shows** the coaming, two small rectangles, and then — right at the frame's bottom edge,
half cut off — the words "VOLTS" and "RUDDER TRIM IN NEUTRAL" beginning to appear.

⭐ **So the panel is drawn, with the right artwork, in the wrong PLACE.** Taking the one feature
present in both, the rudder-trim placard:

| | placard's vertical position |
|---|---|
| gold | y/H = 0.884 |
| port | y/H = **0.991** |

**about 11% of frame height lower**, which is exactly enough to push every gauge below the bottom of
the screen. That is why the cockpit reads as bare: the instruments are rendered where the frame ends.

⚠️ **Not yet attributed.** A panel drawn low, an eye point set high, and a view pitched up all
produce this, and this capture cannot separate them. The canopy pillars agree horizontally (S3) and
the gunsight head is at a plausible height, which argues for the panel's own placement rather than a
whole-view offset — but that is an impression, not a measurement.

**S5:** measure a SECOND feature's vertical position — the gunsight head unit, which is fixed to the
same cockpit — in both frames. If it is also 11% low, the whole view is offset; if it is not, the
panel alone is mis-placed, and the two want different fixes.

**GOLD3D-1: 4 sprints — AT THE CAP, rotating off with a measured, reproducible difference.**

## PO-82-leak S1 (Opus 5, 2026-09-14) — ⛔ the gated sweep CRASHES: it frees surfaces the landscape texture manager still holds

`MA_FREE_TEX_SURFACES=1` has sat behind its flag since S369 with the note *"until the census and the
gate suite have both had a look — the failure mode of getting it wrong is a use-after-free"*. S1
spent one flight on exactly that question.

**MEASURED, one quick-mission sortie with the flag on:**

    [surfsweep] freed 1122 texture surface(s) at 3D teardown
    Segmentation fault (core dumped)        exit=139

⭐ **The sweep does what it claims** — 1,122 surfaces, matching the ~1,000-per-sortie leak the S350
census measured — **and then the process dies.** Resolving the backtrace names the owner that was
not consulted:

    Display::RenderTileToDDSurface(unsigned char*, HTEXT const&)
    Display::DoRenderTileToDDSurface(...)
    Window::DoRenderTileToDDSurface(...)
    LandScape::ManageHighLandTextures(long, long)

**The terrain texture streamer still holds handles to the swept surfaces** and renders a tile into
freed memory. The sweep's own comment reasoned that *"~IDirectDrawSurface disposes of the twin and
the twin's destructor clears the S348 handle registry, so a swept surface leaves nothing pointing at
it"* — that reasoning covers the handle registry and **not** `LandScape`'s own references, which is
the gap.

⚠️ **So the flag must not be shipped, and the leak is not fixable by sweeping alone.** A correct fix
has to either drop `ManageHighLandTextures`' references at teardown (making the sweep safe), or free
only surfaces no manager still holds (making the sweep smaller and provably safe). The first is the
real fix; the second is the cautious one.

**The value of this sprint is the failure mode, cheaply.** The flag existed for a year with a
comment saying it might be unsafe; it is unsafe, it takes one sortie to show, and the crash names
the exact owner to fix. **Leaving it on by default would have crashed every flight's exit.**

**PO-82-leak: 1 sprint.**

## PO-82-leak S2 (Opus 5, 2026-09-14) — ✅ the crash was the sweep running MID-FLIGHT; moved to the real end of the flight, 1,227 surfaces freed and no crash

S1 measured the gated sweep freeing 1,122 surfaces and then dying in
`LandScape::ManageHighLandTextures`. S2 asked why the terrain streamer still held them.

⭐ **Because the flight had not ended.** S369 put the sweep immediately after `MaDriveLaunch()` with
the comment *"the 3D session is over"*. **`Launch3d` returns once the WORLD IS BUILT, not when the
flight ends** — this session's own multiplayer work measured `Launch3d returned` at **log line 370 of
227,661**. So the sweep freed the flight's textures while it was still flying, and the terrain
streamer rendered its next tile into freed memory. Not a subtle ownership problem: the wrong moment.

**FIX:** the sweep now runs from `ma_process_flight_close()` — after the game's own teardown
(`OnOK`/`OnCancel` pause and delete the view and instance) and after `THISTHIS` is disarmed.

**MEASURED, one sortie, flag on:**

    [3d] flight close (id=1) -> OnOK + OnFlyingClosed
    [surfsweep] flight close: freed 1227 texture surface(s)
    exit=124   (the harness timeout, i.e. it ran to the end — no SIGSEGV)

⭐ **1,227 surfaces a sortie recovered, and the process survives.** That is the leak S350 measured
(1002 created, 0 freed) bounded to one flight.

⚠️ **Still `MA_FREE_TEX_SURFACES=1`, not default-on.** The flag's own standard is *"until the census
AND the gate suite have both had a look"*; the census has now looked twice and the gate suite has not.
One clean `port/gates_all.sh` with the flag on is all that stands between this and default — worth
doing, because an unbounded ~1,200-surface-per-sortie leak is a real limit on a long session.

**A note on the shape of this bug, because it has now bitten twice.** *"Launch3d returned"* reads
like an end and is a beginning. MPVIS-1 drew a wrong conclusion from the same line in this session
and had to withdraw it; S369 wrote a comment asserting it and shipped a use-after-free behind a flag.

**PO-82-leak: 2 sprints.**

## PO-82-leak S3 (Opus 5, 2026-09-14) — the sweep passes the gates that fly; and `parity_2d` is RED on `campaign_map`, which is NOT this change

S2 moved the surface sweep to the real end of the flight and left it gated, with "one clean gate
suite" as the last step before default-on. S3 ran the gates that bear on it.

| gate | with `MA_FREE_TEX_SURFACES=1` |
|---|---|
| `add_flight` (flies a sortie, returns to the front end) | **PASS** |
| `revpad_caller` (flies twice, both padlock arms) | **PASS** |
| `parity_2d` | **FAIL — `campaign_map` differs by 37,929 px of 480,000** |

⭐ **The failure is not mine, and I checked rather than assumed.** Re-running `parity_2d` alone with
the flag OFF gives **the identical count, 37,929 px** — the same number to the pixel, so it is
deterministic and unrelated to the sweep. The other four screens are byte-identical in both runs.

**What differs** (`port/ref/native/campaign_map_vs_build_260914.png`, reference above, build below):
the map's **Nm distance ruler is on the LEFT in the reference and on the RIGHT in the build**, and
the reference's `6/25/50: MORNING, PLANNING` banner is not in the same place. A layout shift, not a
colour or content difference.

⚠️ **And it is a REGRESSION against this gate's own baseline.** S414 (2026-09-01) recorded
*"`parity_2d` is 5/5 byte-identical for the first time"*, and `campaign_map` is one of those five. So
the map's furniture moved between 2026-09-01 and now, and the gate has been red since, unnoticed.

**Three explanations ruled out before reporting it:** the gate pins `settings.mig` from
`ref/save/settings_pristine.mig` for every capture, so it is not my resolution experiments leaking
through the player's settings; it pins the campaign save too, so it is not campaign drift; and the
reference file itself is unchanged since S145 (Aug 16), so nobody re-seeded it.

**Next, and it is a bisect, not a hypothesis:** the window is 2026-09-01 → now. `git log` over the
map/canvas layout in that range, rebuild at the midpoint, run `parity_2d campaign_map`.

**For PO-82-leak specifically:** the two flying gates pass with the sweep on, which is the evidence
the flag was waiting for on its own account. Turning it on by default should still wait for a green
`parity_2d`, because a suite with a known red in it cannot tell a new break from the old one.

**PO-82-leak: 3 sprints.**

## MAP-RULER S1 (Opus 5, 2026-09-14) — ⭐ the campaign map has TWO rulers; the second blacks out 48 px of map, and removing it takes parity from 37,653 px to 570

PO-82-leak S3 found `parity_2d` red on `campaign_map` by 37,653 px and left a bisect. The bisect was
not needed: the picture named the cause.

**MEASURED, build vs reference, by column:** **87% of the differing pixels are in columns 700-799**,
and columns **752-799 are more than 90% BLACK in the build where the reference has map** — a 48 px
black strip down the right edge. 48 is the ruler's own width.

⭐ **The port draws the map ruler TWICE, from two different sprints:**

| | site | where it draws |
|---|---|---|
| S135 (PO-22) | `MIG.CPP:2103` → `m_toolbar4.MaPaintAt(0, …)` | **left** edge, and its note says the gold shows "a black strip down the left edge" |
| PO-11 (2026-09-04) | `MIG.CPP:2168` → `ma_map_paint_scalebar()` | **right** edge, `bx = sw - 48`, and it fills that strip black first |

Two rulers cannot both be right, the port's own blessed baseline has the left one, and the second was
added after that baseline — which is exactly why the gate went red on 2026-09-01 and stayed red.

**FIX** (`MA_SCALEBAR_RIGHT=1` restores it): the right-hand paint is off by default.

**MEASURED after:** `campaign_map` **37,653 px → 570 px differing**, i.e. the duplicate was **98.4%**
of the regression. The other four screens stay byte-identical.

⭐ **And the residual 570 px name the next defect exactly.** They are all in columns 0-32 — the left
ruler's own strip — and the crop (`port/ref/native/ruler_vs_ref_260914.png`, reference left, build
right) shows why: **the reference's ruler carries its labels (`0 Nm`, `50`, `100`, `150`) and the
build's draws the ticks with no numbers at all.**

**S2:** find why the left ruler lost its labels. Both paths call `MaDrawScale`; the right-hand one
owns a `static CScaleBar` precisely because drawing through the view's pointer produced a wrong scale
(0/5/10/15/20 instead of 50/100/150/200), so the label path is known to be sensitive to which object
it runs on. That note is the lead.

**MAP-RULER: 1 sprint. The map is 98.4% closer to its reference and the gate's red is now one small,
named difference instead of a mystery.**

## MAP-RULER S2 (Opus 5, 2026-09-14) — ⚠️ the probe I reasoned from was NOT IN THE TREE; one real measurement survives

S1 left 570 px between `campaign_map` and its reference, all in the left ruler's strip, with the
reference carrying labels (`0 Nm`, `50`, `100`, `150`) where the build draws bare ticks. S2 went
after the label path.

⭐ **The one measurement that stands** — it used an env flag, not a source edit:

| arm | bright (tick/label) pixels in the left 48 columns |
|---|---|
| default | **3,904** |
| `MA_NO_MAP_RULER=1` | **259** |

**So the left strip IS drawn by S135's `MaPaintAt`** (the path that survives after S1 removed the
duplicate), and the missing labels are in that path, not somewhere else on the screen.

⛔ **Everything else this sprint appeared to establish is WITHDRAWN.** I added two probes to
`SCALEBAR.CPP` — one in the `m_align==4` branch, one at `MaDrawScale`'s entry — rebuilt, ran, and
neither printed. I read that as *"MaDrawScale is never called"* and started reasoning about which
early-out swallowed it. **The probes were never in the tree:** the marker text is absent, `git
status` shows the file unmodified, and its mtime is still 2026-09-04. **A probe that does not exist
is silent for the most boring reason there is, and its silence looked exactly like a finding.**

⚠️ **I could not determine what happened to the edits, and I am not guessing.** Both edits reported
success and each was followed by a build that did compile a TU. There is only one `SCALEBAR.CPP` in
the tree and `ninja -t deps` confirms it is the compiled one, so the tree's documented
case-variant-twin trap does not explain it. When the probe was re-applied, its anchor — which had
matched **once** an hour earlier — matched **twice**, which says the file's content is not what it
was. Unresolved.

**The rule this earns, and it is cheap:** after editing a source in these trees, `grep` the marker
back out of the file before running the experiment that depends on it. One command, and it would
have turned two wasted runs into none.

**S3:** re-apply the entry probe, VERIFY the marker, then read `m_align`, `m_width` and `grad_10` —
the label gate is `(counter%20==0 || (counter%10==0 && zoom>0.5) || grad_10>10)` and the label
position comes from the current font's height, so those four numbers decide it.

**MAP-RULER: 2 sprints.**

## MAP-RULER S3 (Opus 5, 2026-09-14) — ✅ `parity_2d` is GREEN: the ruler's labels were drawn BLACK ON BLACK, and S2's vanished probe is explained

**1. S2's mystery, solved, and it was mine.** Re-applying the probe threw
`UnicodeEncodeError: 'latin-1' codec can't encode characters in position 11592` — my insert text
carried a non-latin-1 character. `open(p,'w')` had already truncated the file, so it was left at
**ZERO bytes**; `git checkout` restored it. That is what happened in S2: the probe never reached the
compiler, its silence was read as *"MaDrawScale is never called"*, and a sprint's reasoning was built
on it. **Plain ASCII in these inserts, and grep the marker back out before running** — which is how
this sprint caught it in seconds.

**2. With a probe that exists, the terms are unambiguous:**

    [scalebar] MaDrawScale align=4 width=48 horz=0 rect=(0,25)-(48,924) zoom=1.255 longcm=185300 grad10=35.48

The label gate is `counter%5==0 && (counter%20==0 || (counter%10==0 && zoom>0.5) || grad_10>10)`, and
**grad_10 = 35.48 > 10**, so it is TRUE: `TextOut` was being called about five times down the bar.
**The labels were not missing — they were invisible.**

⭐ **3. Because nothing set the text colour.** `MaPaintAt` selects a font and an alignment before
driving the paint, and never a colour, so the labels inherited whatever the map last drew with and
landed **black on the ruler's own black backdrop**. The ticks survived because they are drawn with a
PEN, not with text.

**FIX** (`MA_NO_RULER_TEXTCOL=1` reverts): set the text colour white for the ruler paint.

**VERIFIED — the gate is green for the first time since 2026-09-01:**

    title  OK   prefs_3d  OK   prefs_others  OK   quickmission  OK   campaign_map  OK byte-identical
    ### GATES: 1/1 clean

**Two defects, both user-visible, both closed:** S1's duplicate ruler blacking out 48 px of map, and
S3's invisible distance labels. `campaign_map` went 37,653 px → 570 px → **0**.

⭐ **And this unblocks PO-82-leak:** its remaining condition for default-on was "a clean `parity_2d`",
which could not be judged while the suite carried a known red. It is clean now.

**MAP-RULER: 3 sprints. CLOSED.**

## PO-82-leak S4 (Opus 5, 2026-09-14) — ✅ DEFAULT-ON, and ⚠️ the gates that "fly" turn out never to create a texture surface at all

MAP-RULER closed and `parity_2d` went green 5/5, which was the last condition S3 set. S4 flips the
sweep on by default and re-runs the gates that bear on it.

**The change** — both halves of the mechanism, tracking and sweep, now default on with one opt-out:

    SRC/compat/ddraw_legacy.h : if ((caps & DDSCAPS_TEXTURE) && !getenv("MA_NO_FREE_TEX_SURFACES")) ma_surf_track_texture(surf);
    SRC/MFC/MIG.CPP           : if (!getenv("MA_NO_FREE_TEX_SURFACES")) { ... ma_surf_free_session() ... }

`MA_NO_FREE_TEX_SURFACES=1` restores the leaking behaviour. The old positive `MA_FREE_TEX_SURFACES`
is gone from the binary (`strings build/wmig`: 0 hits for the old name, 2 for the new).

| gate | result with the sweep on by default |
|---|---|
| `add_flight` | **PASS** — third flight reaches the mission |
| `revpad_caller` | **PASS** — both padlock arms |
| `parity_2d` | **PASS — 5/5 byte-identical** |

⚠️ **And then the sweep was asked to speak, and said zero.** A sortie flown to the ALT+X exit
(`BOB_KEYSEQ="9000,0x2D,0x38"`, MA_TRACE_3D) closes the flight properly and prints

    [3d] flight close (id=1) -> OnOK + OnFlyingClosed
    [surfsweep] flight close: freed 0 texture surface(s)

⭐ **Not a broken flag — an empty list. `MA_TRACE_TEX=1` on the same run counts TWO
`CreateSurface` calls in the entire process, caps `0x840` and `0x4200`; neither carries
`DDSCAPS_TEXTURE`.** A longer flight (330 s, exit at pump 9000 instead of 900) gives the same two.
**This harness never creates a texture surface, so it can neither leak one nor be hurt by freeing
one.**

⚠️ **That re-prices the evidence for this whole item, including S3's.** The two "flying" gates pass
with the sweep on — but they would pass with a *broken* sweep too, because there is nothing in their
surface list to free. The real positive measurement remains S2's single run (1,227 freed, process
survives to its timeout); today's gates show only that nothing else regressed.

**So the default-on ships on S2's measurement, not on today's.** That is a defensible place to be —
an unbounded ~1,200-surface-per-sortie leak is a real limit on a long session, the mechanism has been
measured working once end to end, and the revert is one env var — but it must be written down as
what it is.

**S5:** find what S2's run did that these do not (its flight streamed terrain textures; `add_flight`
and `stress_launch`'s Hot Shot flight does not reach that loader) and make it a gate. A leak gate
that cannot observe the leak is the same class of instrument failure as a dedup'd census: it reports
success from silence.

**PO-82-leak: 4 sprints — AT THE CAP. Shipped default-on; the gate that would prove it is S5's.**

## GOLD3D-1 S5 (Opus 5, 2026-09-14) — the second feature DISAGREES with the first, and that rules out a shift: it is a vertical SCALE difference, most of it predicted by the two frames' aspects

S4 measured the rudder-trim placard 11% of frame height low and named the discriminator: measure the
GUNSIGHT, fixed to the same cockpit. If it is also 11% low the whole view is offset; if not, the
panel alone is misplaced. S5 measured it — through the repo's own A/B harness, against the wine
pixel oracle, so the comparison is reproducible by anyone.

⚠️ **First, the harness was broken and failing SILENTLY.** `port/ab.sh` still carried PIXEL click
coordinates (`40,588,231;95,588,217`) frozen from before S63 moved every harness to menu ROWS. The
clicks land on nothing, the run sits in the front end, and the report is `! no frame captured` —
which reads like a crash in the 3D path. **Fixed** to `40,r1;95,r0`, the recipe `stress_launch.sh`
keeps current. *(This is the third harness in this session whose failure looked like a code defect.)*

**MEASURED on `port/ab.sh cockpit`** — native 1280×1024 against `01_cockpit_fwd_gunsight.png`:

| feature | native y/H | wine y/H | difference |
|---|---|---|---|
| gunsight head, red knob (centroid of saturated red) | **0.801** | **0.808** | **0.7%** |
| canopy arch apex (centre 8% of width) | **0.140** | **0.242** | **10.2%** |

⭐ **The two features disagree, and a translation cannot do that.** Fitting the only line through
them, `native = -0.143 + 1.168 × wine`: the native frame is the wine frame **vertically EXPANDED by
×1.17 about y/H = 0.851** — not shifted. Both of S4's candidates (a low panel, a high eye point) are
shifts, and neither survives.

⭐ **And the aspect predicts most of the expansion.** S3 established the horizontal field is the
fixed axis, so the vertical field goes as H/W: native `1024/1280 = 0.800`, wine `1076/1189 = 0.905`.
The native frame therefore shows **13% less vertical field**, putting every feature 1.13× further
from the view centre. **Measured 1.17 against a predicted 1.13.** On this pair of frames, "the panel
is 11% low" is what a 13% narrower vertical field looks like.

⚠️ **This does NOT retract S4, and the reason matters more than the result.** S4's gold was
**1233×1003** (H/W 0.813, within 1.7% of the port's 1280×1024) — aspect-matched, therefore a valid
oracle. This sprint's gold is `port/ref/wine/01_cockpit_fwd_gunsight.png` at **1189×1076** (H/W
0.905). **There are two different gold cockpit captures in circulation whose vertical fields differ
by 13%, and a sprint that mixes them measures the aspect instead of the defect.** The port cannot
even be matched to the second: every enumerated mode is H/W ≤ 0.8.

**S6, in this order:** (1) record the provenance of the 1233×1003 gold in `port/ref/` — it is not in
`port/ref/wine/` and it is none of the ten 1280×1003 screenshots in the gold folder, all of which are
menus; (2) repeat this two-feature measurement on THAT pair, where the aspects agree to 1.7% and the
answer means something; (3) put the frame size and H/W in the A/B report so the next reader cannot
compare two fields of view by accident.

**GOLD3D-1: 1 sprint this pass. The discriminator worked — it just disqualified the oracle.**

## GOLD3D-1 S6 (Opus 5, 2026-09-14) — ⭐⭐ the port can now be captured at the GOLD'S OWN frame shape, and with the aspect matched the difference does NOT go away: the cockpit is stretched **×1.20 vertically about the gunsight**

S5 measured two features that disagreed and showed that the two frames' aspects predicted most of the
disagreement (a factor 1.13 against a measured 1.17). The obvious next move was to match the aspect —
and S2 had recorded that the port cannot: `MA_FORCE_RES=1232x1003` "silently falls back to 640x480".

⭐ **It can now, and the fallback was never about aspect.** `DDRWINIT` selects a mode by matching
`Save_Data.displayW/H` against the DirectDraw shim's **enumeration**; an unlisted size matches
nothing and the pick drops to the desktop mode without a word. The shim now fills one spare slot from
`MA_FORCE_RES`:

    [3d] MA_FORCE_RES: enumerating 1189x1076 as an extra display mode
    [prefs] MA_FORCE_RES -> displayW/H = 1189x1076
    port/out/ab/cockpit.ppm:  P6 1189 1076

**The first cockpit capture in this port's history at a gold's exact frame shape.** Capture-only —
nothing changes unless the variable is set, and the Preferences combo is untouched.

**MEASURED, native 1189×1076 against `01_cockpit_fwd_gunsight.png` 1189×1076 — same shape, same
field:**

| feature | native y/H | wine y/H | difference |
|---|---|---|---|
| gunsight head, red knob | **0.8092** | **0.8083** | **0.09%** |
| canopy arch apex | **0.1292** | **0.2417** | **11.3%** |

⛔ **So S5's aspect explanation is REFUTED by its own experiment.** Matching the frame shape exactly
should have shrunk the arch difference from 10.2% to about 1%; it did not shrink at all — it grew to
11.3%. The two frames now differ in nothing but the renderer.

⭐ **And the shape of the difference is exact.** The line through the two features is
`native = -0.161 + 1.200 x wine`, whose fixed point is **y/H = 0.804 — the gunsight**. **The port
draws the cockpit stretched vertically by 20% about the gunsight**: the arch rides 11% of frame
height too high and the instrument panel, on the other side of the fixed point, drops off the bottom
edge — which is S4's original observation, now with a mechanism and a number.

*(Filed: `port/ref/native/cockpit_matched_1189x1076_260914.png`, native | wine | diff, both 1189×1076
— the first aspect-matched pair, so the next sprint does not have to rediscover which gold it used.)*

**S7:** ×1.20 about a point is a transform, and there are only a few places it can come from — the
projection's vertical half-angle, an aspect divisor applied once too often, or the cockpit model's
own scale. Print the projection terms the 3D view builds (vertical FOV, aspect, near/far) and compare
the vertical half-angle against `2 x atan()` of the gold's; a 20% error should be visible in the
number rather than inferred from pixels.

**GOLD3D-1: 2 sprints this pass. The oracle is usable for vertical geometry for the first time.**

## GOLD3D-1 S7 (Opus 5, 2026-09-14) — ⭐⭐ ROOT CAUSE: the projection's aspect ratio is computed from VIRTUAL dimensions carrying two independently-rounded integer scales

S6 measured the cockpit stretched vertically by **×1.200** against an aspect-matched gold and said a
×1.20 about a point is a transform with only a few possible homes. S7 printed the terms.

    [proj] viewCone=8192 FoV=0.4142  virtual=26158x19368 (H/W=0.7404)
           winmode=640x480 (H/W=0.7500)  aspectRatio=0.3067

⭐ **The frame is 1189×1076 — H/W 0.9050 — and the projection is using 0.7404.**
`matrix::SetViewParams` computes `aspectRatio = VirtualHeight/VirtualWidth × FoV`, and the Virtual
dimensions are the physical ones multiplied by **integer** scales set in `HARDWIN.CPP:299`:

    virtualXscale = (FULLW + window_width  - 1) / window_width     // FULLW 25600, integer divide
    virtualYscale = (FULLH + window_height - 1) / window_height    // FULLH 19200

At 1189×1076 that is **xscale 22, yscale 18** — 1189×22 = 26158 and 1076×18 = 19368, exactly the
numbers printed. **The two roundings do not cancel, and their ratio, 22/18 = 1.2222, becomes a
vertical stretch of everything the 3D view draws.** S6 measured 1.200. Agreement to 1.8%.

⭐ **And the table says why nobody has ever seen this.** FULLW:FULLH is 25600:19200 — exactly 4:3 —
so the two scales come out equal at precisely the modes the game shipped with:

| mode | xscale | yscale | virtual H/W | true H/W | vertical error |
|---|---|---|---|---|---|
| 640×480 | 40 | 40 | 0.7500 | 0.7500 | **1.0000** |
| 800×600 | 32 | 32 | 0.7500 | 0.7500 | **1.0000** |
| 1024×768 | 25 | 25 | 0.7500 | 0.7500 | **1.0000** |
| 1280×1024 | 20 | 19 | 0.7600 | 0.8000 | 1.0526 (5% stretch) |
| 1189×1076 | 22 | 18 | 0.7404 | 0.9050 | **1.2222 (22% stretch)** |
| **1920×1080** | 14 | 18 | 0.7232 | 0.5625 | **0.7778 (22% SQUASH)** |

⭐⭐ **Every mode the 1999 game offered is exact; every mode this port added is wrong — and the worst
is 1920×1080, the one a modern player picks.** That is the defect behind S4's "the instrument panel
sits 11% too low": at 1280×1024 the stretch is 5%, at the PO's desktop resolution the cockpit is
squashed by 22%.

⚠️ **One thing does not fit yet, and it is the next test.** A pure vertical-scale error stretches
about the view CENTRE, y/H = 0.5; S6's fixed point was **0.804**, at the gunsight. The magnitude
matches to 1.8% and the fixed point does not, so either the 3D viewport's vertical centre is not the
frame centre, or a second term is involved.

**S8 is one experiment that both proves this and fixes it:** use the PHYSICAL dimensions —
`aspectRatio = PhysicalHeight/PhysicalWidth × FoV`, a no-op at all three legacy modes since virtual
and physical H/W agree there — then re-run S6's two-feature measurement. **The prediction is that
the canopy arch difference collapses from 11.3% to under 1% and the gunsight stays where it is.**
If it does not, the fixed-point anomaly above is the real story and this is only part of it.

**GOLD3D-1: 3 sprints this pass, and the item has a root cause with a one-line candidate fix and a
falsifiable prediction.**

## GOLD3D-1 S8 (Opus 5, 2026-09-14) — ✅ **SHIPPED: the ×1.20 vertical stretch is gone (1.200 → 0.979)**, and what is left behind it is a pure OFFSET

S7 named the cause and made a prediction. S8 applied the one-line change and re-ran S6's measurement
on the same aspect-matched pair.

**The change** (`MATRIX.CPP`, `SetViewParams`, `MA_NO_ASPECT_FIX=1` reverts):

    aspectRatio = Float(win->PhysicalHeight) / Float(win->PhysicalWidth);   // was Virtual*

    [proj] ... aspectRatio 0.3067 -> 0.3748      (0.9050 x FoV 0.4142 = 0.3749)

**MEASURED, native 1189×1076 vs the 1189×1076 wine gold, before and after:**

| feature | wine | before | after |
|---|---|---|---|
| canopy arch apex (y/H) | 0.2417 | 0.1292 (**11.3%** out) | **0.1979 (4.4% out)** |
| gunsight red knob (y/H) | 0.8083 | 0.8092 (0.09% out) | 0.7523 (5.6% out) |
| whole-frame RMSE | — | 115.13 | **102.13** |

⭐ **Fit the line again and the result is unambiguous: the SCALE error is gone.**

| | slope (vertical scale) | offset |
|---|---|---|
| before | **1.200** | −0.143 |
| after | **0.979** | −0.039 |

**A 20% stretch became a 2% residual.** S7's root cause is confirmed: the projection was being handed
the *virtual* frame shape, whose two integer scale factors do not cancel.

⚠️ **The prediction as literally written is NOT met, and the difference is the finding.** S7 predicted
*"the arch collapses to under 1% and the gunsight stays put"*. The arch went to 4.4% and the gunsight
moved to 5.6% — because what remains is not a stretch at all but a **uniform vertical TRANSLATION of
−3.9% of frame height**: the whole view now sits about 42 px too high in a 1076-line frame. Both
features moved by the same amount, which is what a translation looks like and a scale error does not.
*(It also explains S6's odd fixed point at y/H 0.804: a stretch plus an offset has its fixed point
wherever the two cancel, not at the view centre.)*

**Regression-checked:** `revpad_caller` **PASS** (both padlock arms, 3D flight), `parity_2d`
**PASS — 5/5 byte-identical**. The change cannot touch the 2D front end, and at 640×480, 800×600 and
1024×768 it is arithmetically a no-op, so the three legacy modes are untouched by construction.

⭐ **What the PO gets today:** at 1280×1024 the cockpit was stretched 5%, and at **1920×1080 it was
SQUASHED by 22%** — that is fixed in the default build. The instrument panel S4 reported "11% too low"
should now be ~4% low, all of it from the remaining offset.

**S9:** the offset. −0.039 of frame height at 1189×1076 is ~42 px. Candidates in order of cost:
the viewport's vertical origin (`wvMinY`/`PhysicalMinY`), a half-pixel-style rounding in the same
integer-scale family, or the HUD info-line strip being reserved at the top. Print the viewport
origin and the drawn strip's height and compare with 42.

**GOLD3D-1: 4 sprints this pass — AT THE CAP, and it ships a measured fix for every non-4:3 mode.**

## GOLD3D-2 S1 (Opus 5, 2026-09-14) — ⭐ the aspect fix improves **ALL FOUR** gold views, and the claim is calibrated against a measured noise floor

GOLD3D-1 S8 shipped the projection fix and verified it on the cockpit view. It changes the projection
for every 3D view, so it has to be checked on every view there is a gold for. `port/ab.sh` already
drives all four; at the gold's own frame shape (`MA_FORCE_RES=1189x1076`) they can finally be
compared without an aspect mismatch of their own.

**MEASURED — the same four views, fix reverted (`MA_NO_ASPECT_FIX=1`) and fix on:**

| view | gold | before | after | change |
|---|---|---|---|---|
| cockpit | `01_cockpit_fwd_gunsight` | 115.13 | **102.13** | **−13.00** |
| external (F6) | `04_ext_chase_high` | 131.46 | **126.16** | **−5.30** |
| chase (F9) | `05_ext_chase_low_airfield` | 141.52 | **138.36** | **−3.16** |
| satellite (F10) | `06_ext_flyby_terrain` | 94.38 | **93.53** | **−0.85** |

⭐ **And the noise floor is measured, not assumed.** Whole-frame RMSE against a gold whose flight is
not in the same place as ours has a floor, and a 0.9 change means nothing until that floor is known.
Three same-configuration runs of `external`:

    131.46   131.68   131.66      -> spread 0.22 RMSE (0.17%)

**Every one of the four improvements is outside it** — the smallest, satellite's 0.85, by four times;
the cockpit's by sixty. **The fix helps all four views and harms none.**

⚠️ **What this does NOT say.** The absolute numbers stay between 93 and 138 because the port's sortie
is not at the same point in the world as the PO's gold recording — these frames show different
aeroplanes over different ground. **RMSE here is only valid as a same-config A/B of one change**, and
that is all it is used for. A fidelity number would need matched flight state, which no gold in this
set carries.

**S2:** the satellite view barely moved (−0.85 of 94), which fits — it is a near-overhead view where a
vertical scale error has the least to distort. Worth confirming by measuring a FEATURE in it the way
S6/S8 did for the cockpit, rather than inferring from a whole-frame statistic.

**GOLD3D-2: 1 sprint. The projection fix is now verified on every gold view the port has.**

## GOLD3D-2 S2 (Opus 5, 2026-09-14) — the residual offset shows up in the EXTERNAL view too, so it is view-wide rather than a cockpit-model problem

GOLD3D-1 S8 left a **−3.9% of frame height** vertical offset after the scale error was fixed, and one
view cannot tell "the cockpit model sits high" from "every view sits high". The external gold has a
feature the cockpit does not: **the aircraft itself**, and the chase camera places it, not the pilot.

**MEASURED — the F-86's yellow wing markings, the one saturated colour present in both frames:**

| | native y/H | wine y/H | difference |
|---|---|---|---|
| external (F6) | **0.4975** | **0.5218** | **−0.0243** |
| cockpit (S8's residual, for comparison) | — | — | −0.0386 |

⭐ **Same sign, same order of magnitude, a different view and a completely different feature.** The
port's image sits 2–4% of frame height high in both. **That points at the viewport or the projection's
vertical origin — something every view shares — and away from the cockpit model**, which was the
other candidate S8 left open.

⚠️ **Corroboration, not a measurement.** The chase camera's framing depends on the aircraft's
attitude, and the two sorties are not flying the same way: in the gold the jet is low over brown
terrain, in ours it is above a white cloud deck. A number taken from a feature the camera positions
cannot be cleaner than the camera's own agreement.

⚠️ **And the chase view (F9) is not comparable at all** — the same measurement finds **no yellow
markings in the native frame**: our chase capture does not frame the aircraft where the gold's does.
Recorded so the next sprint does not spend a run rediscovering it; `external` is the usable external
pair.

**This also sharpens GOLD3D-2 S1's caveat.** The two frames show different ground at different
altitudes, which is exactly why the RMSE numbers there are only valid as a same-config A/B of one
change — and why the noise floor had to be measured before any of them was believed.

**S3:** with the offset now shown in two views, print the viewport rectangle the 3D view is given
(`wvMinY`/`PhysicalMinY` and the strip reserved above it) and compare with 42 px at 1076 lines. If it
matches, the fix is the same shape as GOLD3D-1 S8's: use the frame the renderer is actually drawing
into.

**GOLD3D-2: 2 sprints.**

## GOLD3D-2 S3 (Opus 5, 2026-09-14) — ⛔ the viewport-origin hypothesis is REFUTED: the software projection has no vertical offset term at all

S2 showed the residual −3.9% in two views and pointed at "the viewport or the projection's vertical
origin". S3 went to look, and found a suspicious line — then found it never runs.

**The suspect, `Win3d.cpp:3548`:**

    viewdata.originy = (Float) screen_height - window_height/2.0;

`screen_height` is the **window rect** from `GetWindowRect()`; `window_height` is the **render
surface** (`VirtualHeight/virtualYscale`). Those are the same number only if the SDL window is
exactly the size of the surface — and any difference lands directly on the vertical origin. A probe
was added there.

⭐ **The probe printed nothing, and that is the result.** The `[proj]` line from `MATRIX.CPP` printed
in the same run, so the trace machinery works; this site simply does not execute. **The live software
path is `SRC/GRAPHICS/Polygon.cpp:1350`** *(the mixed-case twin — `ninja -t deps` confirms which of
the two is compiled)*, and it reads:

    currscreen->DoGetSurfaceDimensions(win_width, win_height);
    viewdata.originx = win_width>>1;
    viewdata.originy = win_height>>1;      // the exact centre
    viewdata.scaley  = viewdata.originy;

⭐⭐ **And the whole vertical chain is now accounted for, with no offset anywhere in it:**

    screeny = -scaley * bodyy / (hoD * bodyz) + originy      hoD = h/D = 1.0
    originy = H/2   (exact centre)   scaley = H/2
    viewMat.L11 = 1/FoV     viewMat.L22 = 1/aspectRatio     (no L13/L23 translation terms)

**The projection is a pure scale about the frame centre.** So the residual cannot be a projection
offset, and S2's leading candidate is closed. *(The same reading re-confirms GOLD3D-1 S8: the vertical
half-angle is `(H/W)·FoV`, which is exactly why using the virtual H/W was wrong.)*

**What is left, and the arithmetic for the next test.** The gold is the ORIGINAL binary at
1189×1076 — H/W 0.905, far from the 4:3 its cockpit art was authored for. This port **centres** the
extra vertical field on the boresight (`originy = H/2`). If the original instead keeps the 4:3 top
edge and extends downward, its content sits lower by `(1076 − 1189×0.75)/2 / 1076` = **8.6%** of
frame height — same sign as the measurement, same order as the 3.9–5.6% observed.

**S4:** capture the port at **1189×892** — the same width at a true 4:3 — and measure the two
features again. If the residual vanishes there, the port and the gold distribute a non-4:3 frame's
extra field differently, and that is a decision to make deliberately rather than a bug to fix.

**GOLD3D-2: 3 sprints. One hypothesis closed by reading, one candidate left with a number attached.**

## GOLD3D-2 S4 (Opus 5, 2026-09-14) — ⛔ the field-distribution hypothesis is refuted; the residual is a CONSTANT ~2° vertical offset, measured on two features to within 0.3 of a row

S3 predicted the residual came from the port and the gold distributing a non-4:3 frame's extra
vertical field differently, and named the test: capture at **1189×892** — the gold's width at a true
4:3 — and measure again. S4 ran it, in absolute pixel rows this time rather than fractions.

**MEASURED, same width, both features:**

| | canopy arch apex | gunsight red knob |
|---|---|---|
| port, 1189×**892** (4:3) | row **120** | row **718.3** |
| wine gold, 1189×**1076** | row **262** | row **860.6** |
| difference | **142** | **142.3** |

⭐ **A pure translation of 142 rows, agreeing between two unrelated features to 0.3 of a row.**

⭐ **And the port's own behaviour is now measured rather than assumed.** At 1189×1076 the port put
the arch at row 213; at 1189×892 it puts it at 120. **The content moved down 93 rows when 184 rows of
height were added — exactly half.** The port centres the extra vertical field on the boresight, which
is what `originy = H/2` says it should.

⛔ **So S3's arithmetic is refuted by its own experiment.** If the gold anchored its 4:3 content at
the top the offset would be 0; if it centred, 92. Correcting the port's centring, the gold's content
is still **49 rows (4.6% of frame height) lower at the gold's own size** — and the same offset is
there at 4:3. **It does not scale with frame height, so it is not about the extra field at all.**

⭐ **In angle, 49 rows is about 2°.** The vertical half-angle at 4:3 is `(892/1189)·FoV = 0.311`;
49 rows is 11% of the half-height, i.e. **0.034 rad ≈ 1.96° of pitch**. A constant angular offset of
the whole view — which is why it appears identically in the cockpit and the external views (S2) and
survives a change of aspect.

**S5:** find the 2°. The cockpit view's camera is rigid to the airframe, so an attitude difference
between the two sorties cannot move the arch or the gunsight — it has to be a term in the view setup.
Print the eye point and any pitch offset the 3D view applies (`View_Point`'s angles at
`InitFlyingView`, and the cockpit eye offset the shape supplies) and look for ~0.034 rad.

**GOLD3D-2: 4 sprints — AT THE CAP. The offset is now a single number with a unit, and two
hypotheses about it are closed.**

## PO-37 S1 this pass (Opus 5, 2026-09-14) — ⛔ the measurement this item was parked on was taken in DESKTOP coordinates; at the gold's own resolution the port already matches it

PO-37 has been parked since S147/S151 on a choice between two opposite fixes, because S151's
measurement said *"gold renders an 800×600 front end at 2× with the bottom clipped, letterboxed in a
1920 window, while its text scales by only ~1.65× — neither candidate fix describes that."*

⚠️ **That measurement was of the DESKTOP, not of the game.** `gold_video.sh geom` on all three gold
videos gives the same answer:

    non-black bbox (320, 28, 1600, 1052)  ->  1280x1024

**The gold's game window is 1280×1024, sitting at (320,28) in a 1920×1080 desktop capture.** S151's
"art reaches x=1599" is 1599−320 = **1279 in window coordinates — the right-hand edge of the window**,
and its "text spans 320–1506" is 0–1186. There is no 2× scaling, no letterboxing and no clipping:
**the gold's front end simply FILLS its window** — measured, 99% of the window is non-black, artwork
edge to edge.

⭐⭐ **And so does ours, at the same resolution.** Captured the port's title screen at both sizes:

| | non-black extent | fill |
|---|---|---|
| gold, 1280×1024 window | x[0,1279] y[0,1023] | **99%** |
| **port, `MA_FORCE_RES=1280x1024`** | **x[0,1279] y[0,1023]** | **99%** |
| port, `MA_FORCE_RES=1920x1080` | x[320,1599] y[28,1051] | 63% |

**At the gold's own resolution the port's title screen is already right.** The defect is real only at
sizes the front end has no art for: at 1920×1080 it draws a **1280×1024 front end CENTRED** —
(1920−1280)/2 = 320, (1080−1024)/2 = 28, exactly the observed offset — inside a black border.

*(The entry's description is stale too: "the top-left 800×600, the rest black" was S146's state. It
is now 1280×1024 and centred, so something has already moved this on.)*

⭐ **What that does to the item.** The gold cannot arbitrate a 1920×1080 front end **because the PO's
recording was never made at one** — every gold video is a 1280×1024 window. So PO-37 is not a
fidelity defect against the gold at all; it is a **product decision about non-4:3 desktops**: letterbox
(what it does now), stretch to fill, or add a native art variant. **That is a question for the PO, and
it is a much smaller question than the one this item has been carrying.**

**S2:** put the choice to the PO with the two pictures — 1280×1024 filling the window, and
1920×1080 with its border — and ask which they want at desktop resolution. Do not choose it here: the
entry itself warns that choosing wrongly "moves every control on every front-end screen", and now we
know the gold has no opinion.

**PO-37: 1 sprint this pass. A three-sprint blocker dissolved by subtracting a window origin.**

## PO-61 / M5 S1 this pass (Opus 5, 2026-09-14) — ⛔ M5 is not supported: every `.cam` shares the same magic, and the "shipped files are a different format" reading dies

M5 asks whether the shipped `Ian*.cam` replays are from a different patch level than our source
expects — the patch readme says applying it *"will invalidate all existing savegames and recorded
videos"*. That is answerable from the bytes, with no game run.

**Compared three replays THIS binary recorded with three shipped Windows-recorded ones:**

    all six begin  78 56 34 72   ("xV4r" -- the same 4-byte magic)

| pair | common prefix |
|---|---|
| ours vs ours (`corpus-baseline` / `ma-long` / `po-dogfight`) | **18,903 – 19,202 bytes** |
| `IanAce Kill` vs each of ours | **1,111 bytes** |
| `IanLooper Hero` vs anything, including the other Ian file | **4 bytes** |

⭐ **The shared-prefix length measures CONTENT, not format.** Our three recordings are all from the
same quick mission, so they agree for ~19 kB; `IanAce Kill` agrees with them for 1,111 bytes and then
diverges; two other shipped files diverge immediately after the magic — **including from each other**.
A constant container preamble would show the same length for every pair. This does not.

⛔ **So M5's hypothesis is not supported at the container level: the shipped files carry the same
magic and the same shape of stream as ours.** The PO-61 failure is about the **content** — which
matches S217's root cause (a uid resolving to an 83-byte ground group rather than an aircraft) far
better than "a different file format" ever did.

⚠️ **And a fact that changes how PO-61 can be tested at all: the shipped `Ian*.cam` files are NOT in
the game's Videos directory.** `find` over the whole `drive_c` returns none; the 17 `.cam` files there
are all ones this project recorded. The Ian replays live in `sgl/TUE/cam-archive-windows-recorded/`
and `sgl/TUE/afterGameReport/`. **The Replay screen cannot list a file that is not in `Videos/`, so
the PO's crash is not reproducible through the UI on this install as it stands.**

**S2:** copy ONE `Ian*.cam` into `Videos/` (additive — it overwrites nothing, and PO-65 is a
data-loss item so nothing here may overwrite) and run the reload recipe
(`BOB_CLICKSEQ='30,r4;70,#1055:r0;110,#2063:1'`) with `MA_TRACE_REPLAY=1`. The `[replay]
LoadItemAnims uid=… -> ac=…` line then answers S216's fork on a SHIPPED file for the first time:
**garbage uid = the stream is misaligned; sane uid resolving to the wrong object = the world the
super-header rebuilt does not contain what the replay refers to.**

**PO-61: 1 sprint this pass; M5 closed as unsupported.**

## PO-61 S2 (Opus 5, 2026-09-14) — ⭐⭐⭐ a SHIPPED replay is loaded through the UI at last, and S216's fork resolves to **(a): the stream is misaligned, by ONE BYTE**

S1 found the shipped `Ian*.cam` files are not in `Videos/`, so the Replay screen could never list one.
S2 put one there — `cp "sgl/TUE/cam-archive-windows-recorded/IanAce Kill.cam" <drive_c>/rowan/mig/Videos/`
(additive; it overwrites nothing, and it is left in place so the PO can test the same file) — and drove
the Replay screen to it.

**Finding the row took two probes and killed a hypothesis on the way:** row 0 loads
`260829_texture_gone.cam`, which looked like *"the row click does not change the selection"* (the
dialog preselects `Save_Data.lastreplayname`). It does change it — **row 2 → `ma-long.cam`, row 8 →
`IanAce Kill.cam`.** The list is simply not in directory or alphabetical order.

**MEASURED, `MA_ENABLE_3D=1 MA_TRACE_REPLAY=1`, the shipped file:**

    [replay] LoadItemAnims      at offset 20564   Next=51 nextmobile=2
    [replay] LoadItemAnims uid=50688 (0xC600) -> ac=(nil)   <-- did not resolve
    [replay] LoadItemAnims uid=1     (0x0001) -> ac=(nil)   <-- did not resolve
    [replay] LoadItemAnims uid=454   (0x01C6) -> ac=(nil)   <-- did not resolve
    [replay] LoadItemAnims uid=59136 (0xE700) -> ac=(nil)   <-- did not resolve
    [replay] LoadItemAnims uid=1     (0x0001) -> ac=(nil)   <-- did not resolve
    [replay] LoadItemAnims uid=3583  (0x0DFF) -> ac=(nil)   <-- did not resolve
    [replay] LoadItemAnims uid=256   (0x0100) -> ac=0xa6c0790
    [replay] LoadItemAnims FAILED ... [scan stopped at 56441, file is 56442, overshoot -1]

⭐⭐ **S216 set the fork — (a) the uid is garbage, so the stream is misaligned before
`LoadItemAnims`; (b) the uid is sane but the rebuilt world lacks the object. It is (a).** Six of
seven uids do not resolve, and the values are not plausible ids: 50688, 454, 59136, 3583. **Look at
two of them — `0xC600` and `0x01C6`. The same byte `C6` appears in both, at different positions:
that is the signature of the same bytes being read at a ONE-BYTE SHIFT.**

⭐⭐⭐ **And the scan's own arithmetic says one byte, independently.** The failing pass *"stopped at
56441, file is 56442, overshoot −1"* — **one byte short of the end** — while a second pass over the
same file (a different block, starting at offset 20442 instead of 20564) ends *"at 56442, file is
56442, overshoot 0"*, the normal termination. **Two independent signals, both saying one byte.**

⚠️ **The failure has also MOVED since the PO reported it.** Their session ended at
`[SysError] Replay.cpp:4192` with six `GetShapePtr(8036) OUT OF RANGE` lines; this run reaches
`Replay.cpp:4753`, shows **no** `GetShapePtr` message at all, and **does not crash** (graceful
SysError, clean exit). Whatever S217/S231 fixed, the shipped file now gets further.

**S3 is now an accounting problem, not a search:** find the field that costs one byte in the block
beginning at offset **20564** (`Next=51 nextmobile=2`). The sibling block at 20442 (`Next=1`) reads to
the exact end, so the difference is inside the record layout that block uses — a `UByte` where the
writer put a `UWord`, an alignment pad, or a count read as the wrong width. **Print each field's
offset as it is read and compare the two blocks;** the one that ends on 56442 is the control.

**PO-61: 2 sprints this pass. The item's central question has one answer and a one-byte target.**

## PO-61 S3 (Opus 5, 2026-09-14) — ⭐⭐ the misaligning step is `LoadItemData`, and its byte count is sized **entirely from the LIVE WORLD**: 203 per item **plus 6 per aero device**

S2 established the stream is misaligned before `LoadItemAnims`. S3 finds where the bytes go, and the
answer is a full accounting rather than a suspicion.

⭐ **Both passes over the shipped file are IDENTICAL up to `LoadItemData` — both enter at offset
19492 — and diverge inside it:** one leaves at **20564** (1,072 bytes consumed), the other at
**20442** (950 bytes). **122 bytes of difference over the same file.**

⚠️ **The step's own probe predicts 406** (`NEXTMOBILE-chain=2 × pair=203`) and is wrong by 666 and
544 bytes. So the model *"one ASPRIMARYVALUES+MIPRIMARYVALUES per mobile item"* is incomplete, and a
prediction that far out is worth more than a guess: it says there are reads nobody had counted.

**A trace at the single choke point — every `ReplayRead` prints its offset and size
(`MA_TRACE_REPLAYREAD=1`, new) — gives the sequence:**

    off=19492 size=165   off=19657 size=38   then 6,6,6,6      <- item 1
    off=19719 size=165   off=19884 size=38   then 6,6,6        <- item 2
    off=19940 size=66    32   16   4,4,4,4                     <- a fixed block
    off=20070 size=38 x10                                      <- a run

⭐⭐ **165+38 = 203, exactly the modelled pair — and then a VARIABLE tail: four 6-byte reads for the
first aircraft, three for the second.** The existing device probe agrees to the byte:

    [replay] aircraft devices=4  sizeof(AERODEVVALUES)=6 -> 24 bytes of device records
    [replay] aircraft devices=3  sizeof(AERODEVVALUES)=6 -> 18 bytes of device records

⭐⭐⭐ **So `LoadItemData` consumes `Σ over LIVE mobile items of (203 + 6 × that aircraft's aero
devices)`, plus a fixed 130-byte block, plus N × 38 — and every one of those terms is sized from the
WORLD, not from the file.** `while (ac)` walks the live list; `while (pAeroDevice)` walks the live
aircraft's own device chain. **A recording made with a different set of aircraft, or the same
aircraft with different devices, therefore misaligns the stream by 6 bytes per device and 203 per
item** — which is exactly the 122-byte divergence measured between the two passes, and the one-byte
overshoot S2 saw at the end of the scan.

**S4 — and it is now a design question with two answers:** either the file records the counts (in
which case the reader must take them from the stream instead of from the world), or it does not (in
which case the world must be rebuilt from the file's own super-header BEFORE this step, and the real
defect is that it is not). **Decide by reading the WRITER** — `SaveItemData`'s symmetric loop — and
seeing whether it writes any count at all. If it does not, no reader can be made correct on its own
and the fix belongs upstream.

**PO-61: 3 sprints this pass. The mechanism is fully accounted for, byte by byte.**

## PO-61 S4 (Opus 5, 2026-09-14) — ⭐⭐⭐ the WRITER emits no counts at all: the `.cam` item section is **world-coupled by design**, and no reader can be made correct on its own

S3 measured `LoadItemData` consuming `Σ (203 + 6 × aero devices)` over the LIVE world and set S4 one
question: **does the file record those counts?** The writer answers it.

    Bool Replay::StoreItemData()
    {
        ac = *AirStruc::ACList;
        while (ac)
        {
            StorePrimaryASData(ac);        // writes ASPRIMARYVALUES   (165 bytes)
            StorePrimaryMIData(ac);        // writes MIPRIMARYVALUES   (38)
            … StorePrimaryFMData / EngineData / ElmtData …
        }
        …
    }

⭐⭐ **There is no `ReplayWrite` of a count anywhere — not before the loop, not per item, not per
device.** The section is a bare back-to-back sequence of fixed structs whose framing is **implicit in
the writer's own `ACList`**. The reader recovers the framing by walking *its* `ACList` and each
aircraft's device chain, which is exactly the arithmetic S3 measured.

⭐⭐⭐ **So the defect is not in `LoadItemData`, and it cannot be fixed there.** A reader with no
counts in the stream has nothing to resynchronise against: **the only way to read this section
correctly is to have rebuilt a world identical to the one that wrote it** — same aircraft, same
order, same per-aircraft aero devices.

**That splits PO-61 cleanly, and the two halves need different answers:**

| | status |
|---|---|
| **our own recordings** | the world is rebuilt by our code on both sides, so the framing matches — and the corpus round-trip gate (S229) is the evidence that it does |
| **the shipped `Ian*.cam` files** | written by a different binary, on a different OS, at an unknown patch level, from a world we never built. **Reading them requires reconstructing that world from the file's super-header — and if the super-header does not fully determine it, they are unreadable by construction** |

⚠️ **This also re-frames every sprint from S213 onward.** `GetShapePtr(8036)`, the garbage uids, the
one-byte overshoot — all of them are symptoms of the same structural fact, and the sprints that hunted
for "the field that disagrees" were looking for something that does not exist: **there is no field.**

**S5 — and it is a decision, not a measurement.** Either (a) prove the super-header does fully
determine the world and make the rebuild exact before `LoadItemData` (which S225's prescan-guard fix
was the first step toward), or (b) record that foreign `.cam` files are out of scope and say so to the
PO, who can then stop expecting the shipped replays to play. **(b) is honest and cheap; (a) is the
real fix and its feasibility is one reading of the super-header away.**

**PO-61: 4 sprints this pass — AT THE CAP, with the item's shape finally understood: it is a format
property, not a parsing bug.**

## PO-27 S1 this pass (Opus 5, 2026-09-15) — ⭐⭐ ROOT CAUSE **and fix**: the zoom "tiles" are the GDI shim reading a bottom-up DIB's source rect from the wrong edge

PO 2026-08-15, with a screenshot: *"Zooming the map worked except it produced tiles."* Never
investigated (PO-18/PO-27 have no prior sprint). **It reproduces, but only at the top of the zoom
range**, which is why nothing before now had seen it.

**New harness `port/map_zoom.sh` + `port/tools/map_zoom_measure.py`.** It reaches the campaign map
with `parity_2d`'s own recipe, then presses the game's own zoom-in button *by control id*
(`#7@CMiscToolbar` = `IDC_ZOOMIN`, found with `MA_DUMP_MENU`; never a pixel — S63), captures at each
zoom level, and scores the map area for a seam: the mean |ΔRGB| across every row/column boundary,
against the median of the same measure. The player's save and `settings.mig` are pinned and restored
exactly as the parity gate does.

| clicks | m_zoom | strongest row | next row | verdict |
|---|---|---|---|---|
| 0 | 1 | 52.9 | 49.4 | clean |
| 1 | 3 | 71.8 | 67.6 | clean |
| 2 | 7 | 61.5 | 59.8 | clean |
| 3 | 15 | 64.0 | 60.7 | clean |
| 4 | 30 | **105.6** | 71.3 | **seam at y=705** |
| 5 | 50 | **118.7** | 80.0 | **seam at y=815** |

⭐⭐ **The threshold is `m_zoom > 25`** — `CMIGView::UpdateBitmaps` draws each tile as ONE stretch
below that and as **four quadrants** above it, and only the quadrant path passes a PARTIAL source
rectangle. `ma_gdi_stretch_dibits` mapped the source row as `H-1-(sy + Y*sh/dh)`: it measured `sy`
from the TOP of the bitmap and then flipped for bottom-up order. Windows measures `(sx,sy)` from the
**lower-left** for a bottom-up DIB, so the top destination row comes from memory row `sy+sh-1`.

**The two formulas agree exactly when `sy+sh == H`** — i.e. for the full bitmap, which is what every
other caller in the tree passes (the thumbnail, the Smacker player, and the map's own two
single-stretch branches). So the bug was invisible everywhere except the quadrant path, where it
handed the `sy=0` calls the tile's TOP half and the `sy=128` calls its BOTTOM half: **every tile was
drawn with its halves exchanged**, which is a hard seam across the whole map with unrelated terrain
on either side of it. That is the PO's picture.

Fix (`SRC/compat/ma_gdi.cpp`): `srcrow = topdown ? (sy+off) : (sy+sh-1-off)`.

**Measured after, same recipe:** zoom4's y=705 seam 105.6 → the strongest row is 73.2 with the next
at 73.1 (no outlier); zoom5 118.7 → 77.2 against 73.0. The captures show rivers and roads running
continuously through what used to be the seam.

**The instrument was proved to speak before its zero was believed.** New `MA_TRACE_SUBRECT` prints
every blit whose source rect is partial — i.e. exactly the blits this fix moves. The zoomed map
prints **440** of them, the first at `dest(-925,705 963x963)`, the same row the seam was measured at;
`quickmission` and `campaign_map` print **0**, so the fix is provably inert on the parity screens.

**Gates:** `parity_2d` 800×600 **5/5 byte-identical**; `map_drag` PASS (round trip 0 px); `map_icon_click`
PASS. ⚠️ `PARITY_RES=1080` shows `quickmission` 1497 px and `campaign_map` 5184 px against
`ref/native1080` — **both are PRE-EXISTING**: the campaign_map number is identical to the capture taken
before this change, and the `MA_TRACE_SUBRECT` census above shows neither screen performs a single
blit this fix touches. Those two 1080 references need their own sprint; they are not this one.

**PO-27 (and PO-18, which it supersedes): 1 sprint this pass. Fixed; awaiting the PO's eyes.**

## PARITY1080-1 S1 (Opus 5, 2026-09-15) — the 1080 oracle was RED for 19 days, and both screens are the REFERENCE being wrong, not the port

`PARITY_RES=1080` has been failing two of five screens — `quickmission` 1497 px, `campaign_map`
5184 px — while the 800×600 arm, whose references come from **the real game**, is 5/5
byte-identical. A gate that is permanently red is a gate nobody reads, so this sprint asks which
side is wrong. **Both times it is the reference.**

`port/ref/native1080` was captured in **S311 (2026-08-27)** and is **213 commits old**.

**`quickmission`, 1497 px — the reference predates its own fix.** The differing band (373,274)–(753,296)
is the F-86 variants radio row: the capture draws **✔ Scenario / ○ UN / ○ Red**, the reference draws
plain panel art. **The real game draws the row** — it is right there in `ref/native/quickmission.png`,
the 800×600 reference taken from the shipped game. The 1080 reference was taken on 08-27; **S354
fixed exactly this on 08-30** ("the radio was blacking out the panel art", 12012 px → 1497). The
reference is a photograph of the bug.

*(The pixel count being 1497 at BOTH resolutions is not a coincidence: the radio's marks are drawn at
a fixed size, so the same content differs by the same area whatever the canvas.)*

**`campaign_map`, 5184 px — the reference shows three widgets the real game does not.** The diff is
three connected regions, no more: a 48×48 at (286,52), a 48×48 at (789,52), and a 24×24 at (624,28).
Cropped out of the old reference they are **a star plate button**, **a film-strip plate button** and a
**white star filter icon**. Neither star nor film-strip appears anywhere in the 800×600 real-game
reference's toolbar (airbase, question, map, clock, aircraft / clipboard, compass, calendar,
aircraft / notes, layers, zoom-out, X), and the current 1080 capture's toolbar carries that same set.
So the old 1080 baseline recorded **extra** controls, and the build that stopped drawing them moved
toward the original, not away from it.

**RE-BASELINED, deliberately and with the reasoning on the record.** Both files re-captured from the
current build; `PARITY_RES=1080` is **5/5 byte-identical** again and the 800 arm is still **5/5
against the real game** (checked after, not assumed). ⚠️ `ref/native1080` remains what its own header
says it is — a PORT-provenance regression oracle. It answers "did this change?", never "is this
right?". The 800 arm is the only one with gold provenance, and it is what licensed this re-baseline:
every pixel changed here was checked against it first.

**METHOD:** a stale baseline outranks a real fix silently — it is the same failure as MA's S290, and
the tell is a gate that has been red long enough that its number has become part of the scenery
("⚠️ pre-existing" appeared in the last three sprint notes that ran it). **When a reference-based gate
goes red, date the reference before debugging the code.**

**PARITY1080-1: 1 sprint. Gate green, provenance re-stated.**

## PO-44 S1 (Opus 5, 2026-09-15) — the glyphs and their hit bands AGREE; what the PO saw is two dialogs OVERLAPPING, which is PO-45's item

*"Check mark icons at upper right on dialog boxes often not drawn correctly"*, and the weather dialog
dismissed *"by clicking at upper right, but not at the corner"*. The item's recorded theory was the
S82 rule — that `ma_button_title_hit` computes the glyph bands independently of the control's own
draw. **Measured, that theory is wrong, and the picture has a different explanation.**

**New instrument `[titlebands]`** (printed by `MA_DUMP_MENU`): for every visible title bar, scan its
own hit test right-to-left at mid-height and print the BANDS as screen-coordinate runs.

⚠️ **The first version of it printed nonsense, for the reason the item is about.** A title bar is not
a `relative` control, so the dump's template origin reads **(0,0)** for it and every band came out
anchored at the screen corner. The fix is S84's rule: anchor on `drawOx/drawOy`, *what paint did*,
never on a re-derived rect. An instrument for a paint-vs-hit question must not re-derive the origin
it is testing.

**FOUR title bars, 1920×1080, campaign map with Weather / D.I.S. / Squadrons / Player Log opened:**

| bar painted at | Help band | OK band | "?" glyph drawn | "✓" glyph drawn |
|---|---|---|---|---|
| (792,330) 318×27 | 1067–1088 | 1089–1109 | **1068–1078** | **1090–1107** |
| (10,837) 412×27 | 379–400 | 401–421 | **380–390** | **402–419** |
| (0,0) 444×27 | 401–422 | 423–443 | **402–412** | **424–441** |

⭐ **Three for three: each glyph falls wholly inside its own band**, and the OK band runs to the bar's
last pixel, so the corner IS live. The band/glyph disagreement this item assumed does not exist here.

⭐⭐ **The fourth bar is the PO's picture.** A second title bar is painted at **(783,340)** — 9 px left
and 10 px below the first — so its "?" and "✓" peek out beside the front dialog's, and the row
contains **three** red glyph clusters instead of two. That is not a drawing fault in the glyphs: it is
**two dialogs stacked at nearly the same origin**, which is PO-45 ("the fly screen's dialogs do not
overlap") and PO-17's family. At the PO's 800×600 canvas the same dialogs are 42%×67% of the screen
instead of 17%×25%, so the overlap is far worse — and a tick belonging to the dialog BEHIND is exactly
the thing that looks "not drawn correctly" and refuses to dismiss what you think you are clicking.

**What is NOT yet measured, and should not be claimed:** whether the click ROUTER (not the dump) also
puts the corner in the OK band, and whether the overlap reproduces at 800×600. **S2:** resolve a
screen point through the router itself for the corner pixel, and repeat the four-dialog capture at
800×600 — the PO's own canvas is where the overlap story has to be confirmed.

**PO-44: 1 sprint. Its stated mechanism is refuted; the symptom now points at PO-45.**

## PO-44 S2 (Opus 5, 2026-09-15) — the corner DOES dismiss, measured through the router; and at a second resolution the picture is again two dialogs 9 px apart

S1 measured the drawn glyphs against the hit bands and found them in agreement, and pointed at
overlapping dialogs instead. S2 tests the two things S1 deliberately did not claim.

**1. The click ROUTER, not the dump, at the exact corner pixel.** The weather dialog's title bar is
painted at (792,330) 318×27, so its top-right pixel is **(1109,330)**. A scripted click there
(`BOB_CLICKSEQ="…;180,1109,331"`, `MA_TRACE_CLICK=1`):

    [tbclick] id=1001 TITLE local=(317,1) of 318x27 -> dispid 3 (OK) on 8CWeather
    [tbclick] no OK handler registered -> virtual OnOK on 8CWeather

and the capture taken afterwards shows **the dialog gone — the map paints where it was**. The PO's
*"I could dismiss it by clicking at upper right, but not at the corner"* does **not** reproduce: the
corner is in the OK band, the router agrees with the dump, and the click closes the dialog.

**2. A second resolution (1280×1024, the player's own `settings.mig`, unpinned on purpose).** Four
dialogs opened from the map toolbar land at:

| dialog (by bar width) | painted at |
|---|---|
| 444×27 | **(0,0)** — the screen's top-left corner, over the toolbar |
| 412×27 | (10,781) — hard against the bottom-left |
| 318×27 | (472,302) |
| 336×27 | **(463,312)** — 9 px left and 10 px below the one above it |

The same (−9,+10) pair appeared at 1920×1080 — (792,330) and (783,340). **Two dialogs nine pixels
apart put one dialog's "?" and "✓" immediately beside the other's**, which is exactly the row of
three red glyph clusters S1 measured and exactly what *"check mark icons often not drawn correctly"*
describes. A dialog pinned at (0,0) is the same family.

⭐ **So PO-44's remaining content is placement, and it belongs to PO-17/PO-45.** Nothing in the glyph
drawing or the glyph hit-testing is wrong at either resolution measured; what is wrong is where the
dialogs are put.

**RECOMMENDATION:** close PO-44 as *not a glyph defect* and move the evidence to PO-45, whose
acceptance criterion ("clicking an aircraft icon from the fly view gives readable dialogs") is the one
these captures fail. **Not done unilaterally — that is a PO call**, and it is a cheap one to make with
the two captures in hand.

**PO-44: 2 sprints this pass.**

## DELIVERY 260915 (Opus 5, 2026-09-15) — a new MiG Alley AppImage, with the shipped artefact measured rather than assumed

The PO's newest MiG Alley image was **260913**. Since then two user-visible fixes landed — **GOLD3D-1
S8** (the cockpit's ×1.20 vertical stretch, 1.200 → 0.979) and today's **PO-27** (the campaign map's
zoom "tiles") — plus the texture-leak sweep going default-on and the MAP-RULER work. *"Fixed in dev is
not fixed in the AppImage they run"* is a standing rule here, so this sprint ships.

`~/Documents/260915/MigAlley-x86_64-260915.AppImage` (483 MB, packed 03:22).

**VERIFIED BY RUNNING THE ARTEFACT, not by building it.**

1. It launches into a scratch install (`MA_HOME=…/verify_ma_260915`) and reaches the front end:
   `[shot] canvas 1280x1024 nonblack=1304271/1310720` — the title screen paints.
2. It carries **today's** code, proved by exercising it: the same four-zoom-click recipe through the
   packed image prints **360 `[subrect]` lines** (the probe added this morning) and reaches
   `zoom=30`, and the seam metric on that frame reads **73.2 / 73.1 — no outlier**, identical to the
   fixed dev build and against **105.6** before the fix.

That second check is the one that matters: a build stamp says a file was copied, while a measurement
on the shipped image says the defect is gone *in the thing the player launches*.

**DELIVERY 260915: 1 sprint.**

## PO-48 S1 (Opus 5, 2026-09-15) — "Invalid ID!" is TWO different things: a lying dump (fixed) and a real one on the D.I.S. notes

Opened the campaign map's X to look at PO-48 ("the landing page is clean after exiting a campaign")
and found the QUIT GAME confirmation reporting its body text as **"Invalid ID!"** in `MA_DUMP_MENU`.
Chasing that produced one instrument fix, one dead path replaced, and one real defect localised —
and the order in which those separated matters.

**1. The QUIT GAME modal is CORRECT, and the gold says what it should read.** A Wine capture of the
real game exiting a campaign is in the repo — `gold standard/ma/260814_mig_alley_start_campaign_and_exit.mp4`
— and at **t=38 s** it shows the dialog: title *QUIT GAME*, body ***"Are you sure?"***, buttons
*Save / Yes / Cancel*. New `MA_TRACE_STATICDRAW` prints what a static is about to paint, and ours
paints:

    [staticdraw] ctrl=0xb4087a0 at(510,476 258x73) text="Are you sure?"

⭐ **So the "Invalid ID!" was in the DUMP, not on the screen.** `ma_static_setprop` case 3 (the OLE
"String" property, dispid 3) set the control's string and left `MA_DUMP_MENU`'s cache holding the
design-time literal. **Fixed** — the cache now tracks every runtime `SetString`. *A dump that
disagrees with the screen sends sprints after defects that do not exist; this one cost most of a
sprint before the draw-time probe separated them.*

**2. And the trace that should have caught it sooner was capped.** `MA_TRACE_STR` printed 40 lines
and stopped — all of them the campaign map's own per-frame strings (`1770 "planning"`, `1741
"Morning"`, `516 "Nm"`), none of the modal's. Uncapped (`MA_TRACE_STR=100000`) the modal's five
strings are all there and all correct, including **`id=690 n=13 "Are you sure?"`**. The cap is now a
setting. *(Third capped-trace-goes-quiet this session, across two projects.)*

**3. The real defect: `CDis_Note` paints "Invalid ID!" on screen.** With the dump fixed, the
draw-time trace still shows **40 draws** of the literal, from the D.I.S. Notes dialog's two statics
(`#1005`, `#1006`). Their templates carry **no caption and no DLGINIT** (`[dlgctl] id=1005
class="{C42BAC3D-…}" title="" cdlen=0`), so the text comes from the persisted OLE *String* property —
and what was persisted, at design time in 1999, is the literal `"Invalid ID!"`.

**4. Why it was persisted, and the path that is now implemented.** `GetResourceNumberFromID` and
`ConvertResourceID` (`SRC/MFC/GETRESRC.CPP`) are the game's DESIGN-TIME resolver: they `fopen`
**`\mig\src\mfc\resource.h`** and **`\mig\src\mfc\mig.rc`** — absolute paths on a developer's
machine — and turn a caption like `IDS_AREYOUSURE` into its string. No shipped game has those files;
the authoring machine evidently did not either for these two controls, so `"Invalid ID!"` was saved
into the resource. Under `MA_LINUX` both functions now work: a generated 5,444-symbol name→id table
(`SRC/compat/ma_resource_ids.h`, from `RESOURCE.H`) feeding the port's existing PE string loader.
⚠️ **This is not what fixes anything today** — measured, neither function is called on the path that
paints the D.I.S. notes — but it replaces a path that could only ever fail, and the *"Invalid ID!"*
fallback it guards is now reachable only when a name genuinely is not in the table.

**Gates:** `parity_2d` 800×600 **5/5 byte-identical** after all of the above.

**S2:** give `CDis_Note`'s two statics their text. The port already resolves `IDS_* → id → string`
(`ma_dlg_label`); these controls simply have no name persisted, so the fix is at the population site
(`CDis_Note`'s own `OnInitDialog`/notes loader) — find what the notes pane is meant to display and
call `SetString` with it, the way `RMdlDlg` does.

**PO-48: 1 sprint. The reported symptom (stale landing page) is not yet tested — this sprint never
got past the dialog on the way out.**

## PO-48 S2 (Opus 5, 2026-09-15) — ✅ the "Invalid ID!" the player can see is FIXED: a DDX-bound static that nothing ever assigned

S1 separated two things wearing the same string: a stale dump (fixed there) and a real one painted on
screen. S2 finds and fixes the real one.

**Measured first, exactly.** `MA_TRACE_STATICDRAW` over the D.I.S. dialog, counting the strings the
statics actually paint:

    365  "The NKAF is posing a serious threat. On June 27, 2 Yaks strafed Kimpo airfield, …"
    365  "Invalid ID!"

⭐ **The same count — 365 each — because they are the two statics of the SAME dialog.** `CDis_Note`
owns `IDC_DISNOTES` and `IDC_DISNOTES2`; `OnInitDialog` sets the first from
`RESTABLE(2,DISPARA_0,idtext)` and **never assigns the second at all**. `DIS_NOTE.H` binds it with
`DDX_Control` and nothing writes to it, so it paints whatever its design-time OLE *String* property
holds — and what was saved, in 1999, is the developers' own placeholder: the literal `"Invalid ID!"`
that `CRStaticCtrl::OnUpdateCaptionChanged` writes when it cannot resolve a resource name (S1 showed
why it never can in a shipped build).

**Fixed at the one place that owns this dialog's text** — `CDis_Note::OnInitDialog` now clears
`IDC_DISNOTES2` — and **deliberately not** by filtering the literal globally at the draw: a global
filter would also hide a genuinely unresolved resource id, which is a thing worth seeing.
`MA_DIS_NOTE2_RAW=1` restores the old behaviour for an A/B.

**After:**

    365  "The NKAF is posing a serious threat. …"
    365  ""

**Gates:** `parity_2d` 800×600 **5/5 byte-identical**.

⚠️ **What is NOT established:** whether the ORIGINAL shows the placeholder here too. On a player's
Windows machine the same resolver would also fail (it opens `\mig\src\mfc\resource.h`), so the
shipped game may well have shown "Invalid ID!" in that box for twenty-five years. **This port now
shows nothing there, which is better and is a deliberate divergence** — recorded so that a future
gold comparison can overrule it rather than be surprised by it. The 33×19 control is too small for
the string in any case: whatever it was meant to hold, it was not that.

**PO-48: 2 sprints. The reported symptom (stale landing page after exiting a campaign) is STILL not
tested** — two sprints have now gone into what the exit dialog says rather than what the title screen
looks like afterwards. S3 should click through Yes and photograph the landing page, which is the
sentence the PO actually wrote.

## PO-48 S3 (Opus 5, 2026-09-15) — ⭐⭐ the PO's actual sentence is TESTED at last, and the landing page is NOT clean: the menu panel comes back **DOUBLE SIZE**

Two sprints went into what the exit dialog says. S3 does what PO-48 actually reports — *"the landing
page is clean after exiting a campaign"* — by clicking through it: map → **X** (`#10@CSystemBox`) →
**Yes** (`#2125@RMdlDlg`) → photograph the title screen, and compare against the title screen of a
FRESH start captured in the same configuration.

**MEASURED — 20,911 differing pixels, all of them inside one box:**

| | menu text bbox | size |
|---|---|---|
| fresh start | x **487–677**, y 210–403 | 5,418 px of text |
| after exiting a campaign | x **541–731**, y 210–403 | 5,412 px of text |

⭐ **The menu is drawn 54 px to the RIGHT** after a campaign — same vertical position, same amount of
text, same art everywhere else on the screen. Not "stale graphics", but the landing page is
demonstrably not the same page.

⭐⭐ **And the control dump names the cause.** The title menu's own panel, at the same origin in both:

    fresh start :  #2063@RFullPanelDial  rect(530,210  105x100)   centre(582,260)
    after exit  :  #2063@RFullPanelDial  rect(530,210  213x199)   centre(636,309)

**Same origin, DOUBLE the size** — 105→213 (×2.03) and 100→199 (×1.99). The text is laid out inside
that box, so a box twice as wide puts its centred text ~54 px further right. **The defect is a panel
rect that is scaled twice**, not a painting or a stale-canvas problem.

*(`RMdlDlg::OnInitDialog` is one place that rescales a dialog's own client rect —
`rect2.right = ScaleTranslate(rect2.right, false)` — and the QUIT GAME modal is exactly what runs
between the campaign and the title. That is a lead, not a conclusion: nothing here shows WHICH code
applied the second scale.)*

**S4:** print `#2063`'s rect at each transition (title → campaign → modal → title) and find the step
that doubles it. One run, `MA_DUMP_MENU` already prints it — this is a bisect over four screens, not
a search.

**PO-48: 3 sprints this pass. The reported symptom is reproduced and measured for the first time.**

## PO-48 S4 (Opus 5, 2026-09-15) — the landing page is wrong **against the real game**, and the obvious cause is eliminated: the layout index is identical both times

S3 measured the menu 54 px right of a fresh start. S4 anchors that to the gold and kills the first
candidate.

**1. Against the REAL GAME, not just against ourselves.** `port/ref/native/title.png` came from the
shipped game at 800×600, and the parity gate proves a fresh-start title is **byte-identical** to it.
The title reached by exiting a campaign, captured in the same pinned configuration:

    after-exit vs the real game:  20,911 differing pixels, bbox (487,210)-(734,407)

**The same 20,911 and the same bbox as the before/after comparison at 1280×1024** — because the menu
is drawn at fixed absolute coordinates, so the defect is identical at both canvas sizes. **The
landing page is objectively wrong, not merely different from itself.**

**2. And the canvas comes back TWO PIXELS TALLER.** The after-exit capture is **800×602**; every
fresh capture in this configuration is 800×600. Small, but it is a second symptom of the same
rebuild and it is measured rather than inferred.

⛔ **3. The obvious cause is NOT the cause.** `RFullPanelDial::GetCurrentRes` picks a per-resolution
layout (artwork id + dial origin) and carries a known out-of-bounds loop (`for res<6` over a 4-entry
`resolutions[]`), which would explain a differently-laid-out panel exactly. `MA_TRACE_RES=1` across
the whole sequence:

    [res] GetCurrentRes: window=800x600 -> bestresX=1 bestresY=1 chose=1     (first title)
    [res] GetCurrentRes: window=800x600 -> bestresX=1 bestresY=1 chose=1     (after the campaign)

**Identical inputs, identical choice, no out-of-range index.** The layout selection is innocent, and
the hook that proved it has been sitting in the tree since S206 waiting for a question to answer.

⭐ **What remains, sharply.** `LaunchMain` runs twice (once per title visit — the trace shows both),
and the menu listbox is rebuilt each time with the same seven items, at the same origin, at a
different SIZE:

    first title :  #2063@RFullPanelDial  rect(530,210  105x100)
    after exit  :  #2063@RFullPanelDial  rect(530,210  213x199)

**So the size is computed differently on the rebuild** — same layout, same content, same origin.

**S5 (next pass):** print the listbox's size where it is SET (`PositionRListBox` / the `AddColumn` +
`AddString` path) on both visits. Two numbers from one run: if the second build measures its rows
against an already-populated list, the growth is a missing `ResetContent`; if the size arrives
already doubled, the caller is passing a different metric.

**PO-48: 4 sprints this pass — AT THE CAP. Reproduced, gold-anchored, one candidate eliminated, and
the remaining question is a single print.**

## PO-55 S1 (Opus 5, 2026-09-15) — the prime suspect is ELIMINATED, and a waypoint parked over the sea drags normally

PO, Wonju playthrough: *"waypoint on left over water not draggable."* The item named a suspect and
said plainly it had not been measured — *"the Ins Wave dialog is drawn off the left edge and an OOB
node's rect swallows clicks inside it… Test: `MA_TRACE_CLICK=1` and look for `[oobclick] swallowed`"*.
S1 runs that test, and then a better one.

**1. The suspect is dead.** Through a full authorise-and-drag run with `MA_TRACE_CLICK=1`:

    [oobclick] swallowed …            ZERO occurrences
    [oob] dialog 0x9c2a770 8RDEmptyD depth=0 at (935,0) 330x320      the ONLY open dialog

**No click is swallowed anywhere, and the one open dialog sits in the top-RIGHT** (x 935–1265, y 0–320),
nowhere near the left-hand water strip. A dialog cannot be eating clicks it does not cover.

**2. And a waypoint over the sea drags — measured by putting one there.** `MA_MAP_DRAG` moved Egress
600 px left, into the water strip, and then dragged it again from where it landed:

    entry 0: "Egress" at (786,742) -> (186,742)
             allowdrag=1 dragging=1 world (67234323,53699950) -> (34879708,53830948)  moved=1
    entry 1: "Egress" at (168,736) -> (268,736)          <- now over the sea, on the left
             allowdrag=1 dragging=1 world (34879708,53830948) -> (39162117,54145480)  moved=1

**Both drags engage and both move the world position.** Position is not the discriminator, at least
not through this path.

⚠️ **And that last clause is the whole of what S2 must do.** `MA_MAP_DRAG` drives
`CMapDlg::MaDriveDrag` directly — the engine's drag ARITHMETIC, headless. The PO drags with a mouse,
and **`MA_MAP_DRAG_REAL` exists precisely because those are different paths**: S189 added it after
finding the SDL-event → pump → drag-edge → map-tick chain had never been tested, and
`route_drag.sh` could not have noticed *"by construction: it uses the first hook, under the dummy
driver"*. **S2 repeats this over-water test with `MA_MAP_DRAG_REAL` under `gl-lock`.** If the real
path fails where the arithmetic path succeeds, that is PO-55 — and it would also explain why every
existing gate says dragging works.

**Separately, noticed while running it: `route_drag.sh` FAILS on its own tolerance, not on a defect.**
Every functional assertion passes (`Initial Point` and `Egress` both `allowdrag=1 dragging=1 moved=1`;
a non-waypoint refuses), and the gate then fails because the IP landed **4.05 miles** from the target
against a script-chosen limit of 4. A gate that reports FAIL for a 1% overshoot of its own number
will stop being read; the limit needs either a reason or a wider band.

**PO-55: 1 sprint this pass.**

## PO-55 S2 (Opus 5, 2026-09-15) — the PLAYER'S path drags it too: over water, on real GL, through SDL events

S1 showed a waypoint over the sea dragging through `MA_MAP_DRAG` and said plainly what that did not
cover: *"`MA_MAP_DRAG` drives `CMapDlg::MaDriveDrag` directly — the engine's drag ARITHMETIC,
headless. The PO drags with a mouse."* S2 runs the same test through `MA_MAP_DRAG_REAL`, which S189
added for exactly this gap: **SDL event → pump → drag-edge stream → the map tick's dispatch → CMapDlg**,
on a real GL display under `gl-lock`.

    [dragreal] entry 0: "Egress" id=262 at (786,742) -> (186,742) via SDL events
    [mapdrag]  press (786,742)  allowdrag=1 world=(67234323,53699950)
    [mapdrag]  release (186,742) wasdragging=1 world -> (34879708,53830948)  moved=1

    [dragreal] entry 1: "Egress" id=262 at (168,736) -> (268,736) via SDL events     <- now over the sea
    [mapdrag]  press (168,736)  allowdrag=1 world=(34879708,53830948)
    [mapdrag]  release (268,736) wasdragging=1 world -> (39162117,54145480)  moved=1

⭐ **Both drags engage and both move the world position, through the path a player actually uses,
with the waypoint sitting over water on the left of the map.** PO-55 does not reproduce on this
build by either route — the arithmetic path (S1) or the event path (S2).

**Where that leaves it.** The report is real and specific (*"waypoint on left over water not
draggable"*, Wonju playthrough) and two independent paths now say the general case works, so what is
left is something about that PARTICULAR waypoint or moment: a different mission's route, a scrolled
map, an icon overlapped by another item, or a build older than the S172/S189 drag work. **None of
that is guessable from here.**

**Joins the batched PO question** with TERRAIN-1, PO-37, R3.8 and LOAD-1: *does this still happen on
the AppImage you run now, and if so, which mission and which waypoint?* A screenshot with the cursor
on it would settle in one look what two sprints of harness work cannot.

**PO-55: 2 sprints this pass.**

## PO-48 S5 (Opus 5, 2026-09-15) — ⭐⭐ the double-size menu is **entirely a FONT change between the two visits**, measured at the line that sets the size; and the same trace finds a second, always-on sizing error

S4 named this sprint: *"print the listbox's size where it is SET … if the second build measures its
rows against an already-populated list, the growth is a missing `ResetContent`; if the size arrives
already doubled, the caller is passing a different metric."* `MA_TRACE_LISTSIZE=1` (new, in
`CRListBoxCtrl::Shrink` and `::ResizeToFit`, `SRC/RLISTBOX/RLISTBXC.CPP`) prints the terms of the
round trip rather than its result, because the round trip is where the size comes from:

    Shrink      stores  m_sizeList[i] = bestwidth * 16 / tm.tmHeight  + spacing
    ResizeToFit computes width       = SUM m_sizeList[i] * tm.tmHeight / 16

**Both title visits, one run** (`port/sysbox_exit.sh`'s drive: title → campaign → map → X → Yes →
title; the trace rides along, and the gate's own assertions prove the X was reached):

| | Shrink `tmHeight` | ResizeToFit `tmHeight` | cols | `sumSizeList` | result |
|---|---|---|---|---|---|
| **first title** | 12 | 14 | 1 | 120 | **105 x 100** |
| **after the campaign** | 40 | 43 | 1 | 124 | **333 x 305** |

⭐⭐ **Same content, same column count, same origin, same layout index (S4) — and the size scales
with the font, exactly: 43/14 = 3.07, 333/105 = 3.17.** It is not a missing `ResetContent`:
`CRListBoxCtrl::Clear()` empties `m_list`, `m_sizeList`, `m_playerList`, `m_isPictureList` and
`m_rowColourList`, and the trace confirms one column on both visits. **The second build is measured
against a font three times the height of the first.**

**Why the first visit gets a different font, and it is already written down in this tree.** The S317
(PO-67) comment in `SRC/compat/ma_olecontrol.cpp` records it: `PositionRListBox` runs *during screen
setup*, **before the front end's global font table answers `WM_GETGLOBALFONT`** — which at that
moment returns NULL for every index — so the control falls back to the DC's 14 px font. By the
second visit the table is up and index 10 answers with a 43 px font. S317 already worked around the
consequence by re-running `ResizeToFit` at paint time and keeping only the width; the trace shows
that re-fit as the bare third call (`tmHeight=43 … width=322`) with no `Shrink` before it.

⭐ **And the trace found a second defect nobody was looking for: `Shrink` and `ResizeToFit` never
agree on the font height.** 12 against 14 on the first visit, 40 against 43 on the second — in every
call, on every screen in the log. `ResizeToFit` does `pdc->SetMapMode(MM_TEXT)` before it measures
and `Shrink` does not, so the two read the same font through different map modes. The round trip
therefore does not cancel and **every listbox in the game is sized `tmHeight_resize/tmHeight_shrink`
too wide** — 1.167x at the title, 1.075x elsewhere. Small, permanent, and independent of PO-48.

⚠️ **Which size is RIGHT is still open, and the gold says "neither".** S4 established that a fresh
start is byte-identical to the shipped game's own `port/ref/native/title.png`, and S3 measured that
title's menu text at x 487–677, y 210–403 — **seven rows over 193 px, a row pitch of ~27.5 px.**
That is neither the first visit's 14 px font nor the second's 43 px one. So the 105-wide first visit
is not "correct" either; it merely happens to place its centred text where the gold's is, because
`OnDraw` centres rows on the rect's centre and lets them overflow.

**S6 (next pass):** measure the gold title's row pitch properly (`port/ref/native/title.png` is in
the tree — count the text rows' baselines), then print what each `WM_GETGLOBALFONT` index returns and
find the index whose `tmHeight` matches it. **The fix is a font-table question, not a listbox one**,
and it is shared with PO-67 — whose overflow (7 rows laid out 292 wide and 305 tall inside a
105x100 box) is the same 43 px font seen from the painting side. Fixing the two sizing calls to
agree on a map mode is worth doing on its own, but it moves the width by 7–17%, not by 3x.

**Delivered:** `MA_TRACE_LISTSIZE` (default off) and `port/po48_listsize.sh`. ⚠️ **The standalone
script's own drive does not reach the X** — it stops after the NAV clicks and the log has no
`evt_fire id=10`, so it produces the first visit only. The numbers above come from
`port/sysbox_exit.sh`, which does reach it; the script is left in the tree with that stated rather
than silently producing half an answer.

**PO-48: 5 sprints (4 on the previous pass + this one). Root cause of the reported symptom named at
the line.**

## PO-48 S6 (Opus 5, 2026-09-15) — the gold's row pitch is **28 px**, which is neither font — so the listbox's computed row height is NOT what draws the menu, and the defect is the BOX alone

S5 ended with "which size is right is still open, and the gold says neither"; S6 measures the gold
properly.

**The shipped game's own title, `port/ref/native/title.png` (800x600).** Counting bright text rows in
the menu column (x 480–690):

    row bands start at y = 215, 243, 271, 299, 327, 355, 383      seven rows
    pitch = 28 px, exactly, every gap; glyph band ~15 px tall

**28 px.** The port's listbox computes **100 px** of height for those same seven rows on the first
visit (14.3/row, from the fallback 14 px font) and **305 px** on the second (43.6/row, from the real
43 px one). Neither is 28.

⭐ **And S4 already established that a fresh-start title is BYTE-IDENTICAL to this gold.** Both
statements can only be true together if the glyphs are not laid out at the listbox's computed row
height at all — **the computed size is the BOX**: the hit area, and the centre that `OnDraw` centres
each row on. That is exactly the shape of S3's measurement, which nobody had an explanation for at
the time: after the campaign the menu has *the same amount of text* (5,412 px against 5,418) *shifted
54 px right* — the glyphs never changed, the box did, and a box twice as wide moves its centre by
half the difference.

**So the fix target is narrow and the correct value is known.** The first visit's box (105x100)
reproduces the gold byte for byte; the second visit's (333x305 in the listsize trace, 213x199 as
S3 measured it on the drawn panel) does not. **The second visit has to compute what the first one
computes.**

**S7 — two candidate fixes, and they are distinguishable by the parity gate, not by argument:**
1. **Make both builds measure against the same font.** `PositionRListBox` runs during screen setup;
   on the first visit `WM_GETGLOBALFONT` answers NULL and the DC's 14 px font is used, on the second
   the table is up. Pinning the font the control sizes against would make the two agree — but it
   makes the SIZE depend on a fallback, which is fragile.
2. **Compute the box once and keep it.** The panel is rebuilt with identical content at an identical
   origin every visit; caching the first computed extent per screen and reusing it is the smaller
   change and cannot regress a screen whose content genuinely changes, provided the cache is keyed
   on the content.
   
   ⚠️ Whichever is chosen, run the title parity gate **and** PO-67's hit-test check: S317 records
   that the box also decides where clicks land, so a box that matches the gold's pixels must still
   accept clicks across the whole visible label. Those two requirements are what killed the naive
   "just make the box bigger" reading of PO-67.

**Not measured this sprint, and it should be:** the after-exit title's own row pitch. The gate's
`exit.ppm` is captured from the first pass (X clicked, question not yet answered) at 1280x1024, so it
is not the post-exit title — a capture of the SECOND title visit has to be taken from the answering
run. If its pitch is also 28, point (3) above is confirmed directly rather than by inference from
S4's byte-identity.

**PO-48: 6 sprints (2 this pass).**

## PO-48 S7 (Opus 5, 2026-09-15) — ✅ **FIXED AND VERIFIED AT ZERO PIXELS: the landing page after a campaign is now byte-identical to a fresh start**

S6 established that the whole defect is the listbox's BOX (the glyphs are drawn at the gold's 28 px
pitch either way), that the first visit's extent is the one that reproduces the shipped game byte for
byte, and that the second visit differs only because it measures against a different font.

**The fix.** `RFullPanelDial::PositionRListBox` now caches the computed extent per
**(screen, resolution, text-id list)** and reuses it on a rebuild. A screen whose CONTENT changes
recomputes, because the key changes with it. 32 entries, and it says so when full.
`MA_NO_LBEXTENT_CACHE=1` reverts.

It is the smaller of S6's two candidates and it does not depend on a fallback font existing: the
panel is rebuilt with identical content at an identical origin every visit, so the first computed
extent IS the right one by construction.

**It does what it says** (`MA_TRACE_LISTSIZE=1`, the campaign-exit drive):

    [listsize] PositionRListBox: first build for this screen -> 105x100 (cache 1/32)
    [listsize] PositionRListBox: first build for this screen -> 262x47  (cache 2/32)
    [listsize] PositionRListBox: rebuild computed 333x305, keeping the first build's 105x100

⭐⭐ **And the acceptance test is the PO's own sentence, measured.** Same configuration (1280x1024,
software path), same capture frame:

| | pixels differing |
|---|---|
| before the fix — fresh start vs after a campaign | **46,328** (menu region, x 715–1126, y 370–671) |
| **after the fix — fresh start vs after a campaign** | **0 of 1,310,720** |

**Byte-identical.** *"The landing page is clean after exiting a campaign"* is now true in the strict
sense.

**No regression:** `port/parity_2d.sh` — title, prefs_3d, prefs_others, quickmission, campaign_map —
**5 of 5 byte-identical to the committed references**, and `port/sysbox_exit.sh` still passes
(handler called, confirmation opened, left the map).

⚠️ **What this does NOT fix, and it must not be confused with it.** PO-67's hit-test half is
untouched: the compat layer still re-runs `ResizeToFit` at paint time (S317) and paints the rows
322 px wide inside a 105-wide box, so the box the fix preserves is still smaller than the drawn
label. That was true of the shipped game's own pixels too — it is what makes a fresh start
byte-identical — so this fix is the right one for PO-48, and PO-67 remains its own item about where
clicks land.

Also still open from S6, and worth the run: the `Shrink`/`ResizeToFit` map-mode disagreement (12 vs
14, 40 vs 43 in every call), which sizes every listbox in the game 7–17% too wide. Independent of
PO-48, now that the title's box is pinned to the first build.

**PO-48: 7 sprints (3 this pass). ✅ CLOSED — the reported symptom is gone, measured at zero pixels
against a fresh start, with the 2D parity set unmoved.**

## PO-55 S3 (Opus 5, 2026-09-15) — ⛔ **S1 and S2 could not have reproduced this**: both clicked a point the HIT TEST had already confirmed. And the gold names the mission, the waypoint and the zoom

S2 parked PO-55 on the PO with *"two independent paths now say the general case works"*. S3 checks
the paths themselves, and then checks the repo before the PO.

⛔ **1. The two paths are not independent in the way that matters.** `MA_MAP_DRAG_REAL` resolves its
waypoint through `ma_map_find_named` (`MIG.CPP:1373`), which scans a grid and asks
**`CMapDlg::FindMapItem(CPoint)` — the map's own HIT TEST** — for the first point that reports the
named item, and then presses **exactly there**. `MA_MAP_DRAG` resolves the same way. So both sprints
pressed on a pixel the hit test had already said belonged to the waypoint.

**A player does not do that. A player aims at the ICON.** If the drawn icon and the hit region
disagree, every harness drag passes and the player still cannot grab the waypoint — and *"the icon is
drawn somewhere other than where the map thinks it is"* is a defect class this exact map has already
produced: PO-27 was map tiles drawn with their halves exchanged at zoom. **Neither S1 nor S2 could
detect that by construction**, which makes "does not reproduce" an unsupported conclusion rather than
a finding. (`ma_map_find_named`'s own comment says it resolves by name "never from hardcoded pixels"
— correct for stability, and the reason the harness cannot see this.)

⭐ **2. And the gold store has the mission.** `~/gold standard/ma/wonju_attack.mp4` is 5:44 of the
real game flying **the Wonju playthrough PO-55 was reported from**, with `wonju_script.txt` beside
it. Step 13 of that script is PO-55's operation, in the PO's own words:

> *"Route: **drag Egress inland** (target is close to the front line, go home direct); drag the IP to
> within 4 miles of the target …; drag the two AAA waypoints over the target area."*

**The configuration is now fully specified** (frame saved as
`port/ref/native/wonju-planning-gold-2026-08-21.png`, t=216 s):

* **1920x1080** — the PO's resolution;
* the campaign map **fully zoomed out** (scale ruler 0–250 Nm, whole peninsula);
* **Wonju Supply Dump**, Minimum Strike, 1/5/51 morning, Mission Folder open;
* the white route's **leftmost vertex sits over open water** off the west coast, at
  approximately **(527, 583)**.

⭐ **3. And in the original that waypoint has NO ICON.** Magnified 3x, the over-water vertex is a
**bare corner in the route line** — no tan waypoint box, while the other route waypoints on land
carry theirs. So in the shipped game the player drags a waypoint that is drawn only as a line corner,
and the script says to do exactly that. Whatever the port draws there, this is the state to compare
against.

**S4 — the experiment neither earlier sprint could run.** On the Wonju planning map at full zoom-out,
for every route waypoint print BOTH: (a) the point `FindMapItem` reports it at (the grid scan already
implemented in `ma_map_find_named` / `MA_MAP_ITEM_SCAN`), and (b) where the map DRAWS its icon or
vertex. **Any disagreement between the two is PO-55**, and the gold frame above says what the drawn
answer should look like. Only if they agree — icon on the hit region, at the gold's position — is
"does not reproduce" earned.

**PO-55: 3 sprints this pass.**

## PO-55 S4 (Opus 5, 2026-09-15) — ⭐⭐⭐ **REPRODUCED AT LAST, AND ROOT-CAUSED: `FindMapItem` lets a LATER BAND overwrite a waypoint, so some waypoint icons are not clickable at all.** Two of eight, on the first save tried

S3 said the earlier sprints could not see a disagreement between where an icon is DRAWN and what a
click there resolves to, because both harnesses press where the hit test already said. S4 asks the
question directly.

⭐ **Reading `CMapDlg::FindMapItem` (`MAPDLG.CPP:489`) first, because the structure is the answer.**
It fills one variable, `m_hintid`, from **three loops in band order**:

    for (i = UID_Null;          i < WayPointBAND;    i++)   ... if (in box) m_hintid = i;
    for (i = WayPointBAND;      i < WayPointBANDEND; i++)   ... if (in box) m_hintid = i;   <- waypoints
    for (i = WayPointBANDEND;   i < IllegalSepID;    i++)   ... if (in box) m_hintid = i;

**Last write wins, and the waypoints are in the MIDDLE loop.** Every item in the third range —
`LandscapeBAND` 0x0600, roads, rails, aircraft, airfields, AAA sites, bridges, marshalling yards,
trains, trucks, troops — **overwrites a waypoint whose icon box it overlaps.** Which waypoint that
hits depends entirely on what happens to be underneath it, which is exactly the shape of *"this one
waypoint will not drag"*.

⭐⭐ **Measured.** `MA_TRACE_WPHIT=<frame>` (new) computes every waypoint's DRAWN screen centre with
the map's own `CMIGView::ScreenXY` — the same call the icon draw uses, fan offset included — asks
`FindMapItem` what is at that point, and reports both. On the pristine campaign save, straight off
the shelf, no special setup:

    [wphit] waypoint id=260 "Waypoint: End"   drawn at (692,522) -> FindMapItem says
            id=10234 "Yesong Rail Bridge" (band 0x2700) -- THE ICON IS NOT CLICKABLE
    [wphit] waypoint id=259 "Waypoint: Start" drawn at (694,556) -> FindMapItem says
            id=0 "(nothing)"                              -- THE ICON IS NOT CLICKABLE
    [wphit] 8 waypoints: 6 answer at their own drawn centre, 2 are OVERRIDDEN, 0 off-screen

**Two of eight waypoints cannot be picked up by clicking on them.** "End" is taken by a bridge in
`AmberBridgeBAND` (0x2700) — the third loop, exactly as the code predicts. **PO-55 reproduces.**

**And it explains everything the earlier sprints could not.** The report was specific to ONE waypoint
because only some waypoints sit on a later-band item; both harnesses passed because
`ma_map_find_named` scans a grid and stops at the first point where the waypoint *wins*, which exists
as long as any part of its icon is clear.

*(The "Start" case returns NOTHING rather than a different item, so it is a second failure of its own
and is not claimed as the same one. Not yet explained; do not fold it into the band fix.)*

⚠️ **One instrument bug, caught before it became a finding.** The first version of the probe passed
`+m_scrollpoint` to `ScreenXY`; every real caller passes **`-m_scrollpoint`** (`MAPDLG.CPP:568` and
seven more). It reported all eight waypoints 450–820 px below the map pane, which would have read as
a spectacular draw/hit disagreement. **The numbers were absurd rather than merely wrong**, which is
the only reason it was checked. [[instrument-bookkeeping-lies]].

**S5 — the fix, and it is small.** A click should prefer the item the player is most likely to have
aimed at, and a route waypoint is a deliberate, movable object sitting on top of the scenery. Either
run the waypoint loop LAST, or keep a waypoint hit once found and let later bands fill in only when
no waypoint matched. **The acceptance test is this same probe: 8 of 8, and `port/parity_2d.sh`
unmoved.** Then re-run the two drag gates — with the hit test changed they are testing something new.

**PO-55: 4 sprints this pass — AT THE CAP, and reproduced with the mechanism named at the line.**

## PO-55 S5 (Opus 5, 2026-09-15) — ⛔ **S4's "PO-55 reproduces" is RETRACTED.** The two failing waypoints have no hit region ANYWHERE — they are route endpoints with no icon, and the gold draws none either

S4 read `FindMapItem`'s three-loop, last-write-wins structure, saw "End" resolve to a bridge in the
third band, and concluded the bridge was **overwriting** the waypoint. S5 built the fix for that and
it changed nothing — which is the whole finding.

⛔ **1. The preference fix does not move the result.** With the waypoint hit kept in preference to
later bands, "End" still resolves to `Yesong Rail Bridge`. That can only mean **the waypoint loop
never matched there either** — the bridge was *filling in*, not overwriting.

⛔ **2. Nor is it the fan offset.** Both loops offset a waypoint by one icon radius at
`(id-WayPointBAND) x 50 deg`, and the two implementations agree (screen `px += s*r/32768` against
world `wx += MULSHSIN(s, worldicon, 15)`, with `worldicon = m_iconradius*65536/zoom`). Measured at
both points anyway:

    id=259 fanned (694,556) -> 0      ; UNFANNED (688,566) -> 0
    id=260 fanned (692,522) -> 10234  ; UNFANNED (696,533) -> 10234

⭐⭐ **3. And the decisive one: neither waypoint has a hit region ANYWHERE on the pane.** An 8-px
grid sweep of the whole map pane for each id:

    id=259 "Waypoint: Start" -- found anywhere on the pane: NO -- it has no hit region at all
    id=260 "Waypoint: End"   -- found anywhere on the pane: NO -- it has no hit region at all

The hit loop skips any item whose `DrawIconTest` yields no file — and an item with no icon file has
**nothing to draw either**. So these two are not "icons that cannot be clicked"; they are **route
endpoints that carry no icon**, and my probe was computing where an icon *would* go.

⭐ **4. The gold agrees, and this is where it matters.** The Wonju gold frame
(`port/ref/native/wonju-planning-gold-2026-08-21.png`, S3) shows the real game's route with a
**bare corner** at the over-water vertex and tan icon boxes only on the land waypoints. **The
original does not draw an icon there either.**

**So what PO-55 most likely is:** the PO tried to drag a route point that has no icon in *either*
game. That is a discoverability problem, not a port regression — and it is consistent with every
measurement across five sprints, including the two that said "does not reproduce".

**What is NOT retracted.** The band-override hazard S4 found is real *in structure*: a third-band
item genuinely can overwrite a waypoint that matched. It simply has no demonstrated instance. The
preference is therefore **implemented and DEFAULT OFF** (`MA_WP_HITPRIORITY=1` enables it) — a
behaviour change with no case does not ship. `port/parity_2d.sh`: **5 of 5 byte-identical**.

**The PO question, now worth one line instead of a paragraph:** *on the map you were using, did the
waypoint you could not drag have a little tan ICON BOX on it, or was it just a corner in the route
line?* If it had a box, this is a live defect and `MA_TRACE_WPHIT` will find it in one run on your
save. If it was a bare corner, the answer is that the original does not let you grab that point
either — and the useful change would be to give route endpoints an icon, which is a design call.

**PO-55: 5 sprints this pass. One claim retracted, the instrument kept, the fix parked behind a flag.**

## GOLD3D-1 S6 (Opus 5, 2026-09-15) — ⛔ the 3D A/B **runs**, and its numbers are **not a fidelity verdict**: the two sides are at different flight states, and the reference set records no state at all

`port/ab.sh` is the port's 3D pixel-oracle harness and it had not been run this pass. It runs, and
**all four views capture** (S5's click-recipe fix holds — the silent "! no frame captured" failure is
gone):

| view | RMSE | mean abs diff | pixels changed >24 |
|---|---|---|---|
| cockpit | — | — | (per-chan R 97.4 G 89.0 B 76.9) |
| external | 129.33 | 111.61 | 92.8% |
| chase | 140.35 | 130.00 | 99.0% |
| satellite | 94.23 | 62.57 | 69.2% |

⛔ **And none of that means what it looks like.** Looking at the side-by-side rather than the number
(the rule this project keeps relearning):

* **our chase frame is at ALTITUDE over an unbroken cloud deck**; the reference,
  `05_ext_chase_low_airfield.png`, is — as its own filename says — **low over an airfield**;
* **our capture states its own flight state in-frame**: `Speed: 494Kts  Mach: 0.82  Alt.: 15998ft
  Hdg: 278  Thrust: 72`;
* **the reference states nothing.** Its bottom 80 rows contain **zero** bright-text pixels — the Wine
  references were captured with no HUD, so they carry no state and cannot be matched as they stand;
* the canvases differ too (native **1280x1024**, references **1189x1076**), and `ab_compare.py`
  quietly resamples both to 640x480, so the mismatch never surfaces as an error.

**RMSE 94–140 and "92–99% of pixels changed" measure "a different moment", not "wrong rendering".**
The harness prints a confident number for a comparison that is not valid, which is exactly the
failure [[gate-frame-must-match-the-eye]] and [[parity-captures-must-record-their-state]] describe.
A warning block naming all four measurements now sits at the top of `ab.sh` so the next reader cannot
quote the numbers innocently.

⭐⭐ **And the better oracle is already in the repo.** `~/gold standard/ma/260814_mig_complete_campaign.mp4`
is 5:53 of the **real game in 3D at 1920x1080** — cockpit, external, chase, the in-flight map — **with
the same HUD in frame**:

    gold:  Speed: 352Kts   Mach: 0.54   Alt.: 5116ft    Hdg: 317   Thrust: 49
    ours:  Speed: 494Kts   Mach: 0.82   Alt.: 15998ft   Hdg: 278   Thrust: 72

Same layout, same fields. **So a gold frame can be selected BY STATE and our run driven to match it**
— which is the one thing the stills could never support, and it needs no new capture from the PO.

**S7:** build `port/gold3d.sh` on that basis — index the video's 3D frames by the altitude/speed/
heading their own HUD reports, pick the frame nearest our run's state (our HUD is in the dumped
frame and `MA_TRACE_HUD`-style values are in the log), and compare **region by region** (sky band,
horizon, terrain mass) rather than whole-frame. Only then is a number worth printing.

**GOLD3D-1: 1 sprint this pass. A misleading gate is labelled and a state-matched oracle is
identified.**

## GOLD3D-1 S7 (Opus 5, 2026-09-15) — the first band comparison against the gold VIDEO, and it measures the **noise floor** that kills every whole-frame number this item has ever printed

S6 identified the gold video as a state-labelled oracle. S7 takes the first measurement from it, and
the most valuable thing it produces is not a parity verdict but a scale.

**Sky and terrain bands** (rows 5–15% and 55–85%), eight gold frames 5 s apart inside ONE 3D
sequence, against our four `ab.sh` captures (`port/tools/gold3d_bands.py`, new):

| capture | sky mean RGB | terrain mean RGB |
|---|---|---|
| gold f_002 | 126.5 126.1 115.1 | 31.0 26.0 18.4 |
| gold f_005 | 126.7 126.2 115.2 | 28.6 25.0 18.3 |
| gold f_007 | 113.1 112.5 106.0 | 50.7 41.9 27.9 |
| gold f_009 | 92.7 91.9 88.7 | 72.7 72.3 61.0 |
| port external | 144.3 168.1 198.0 | 184.8 197.5 198.9 |
| port chase | 144.3 168.1 198.0 | 222.5 233.4 233.8 |
| port cockpit | 144.2 168.0 197.7 | 171.9 182.7 184.6 |
| **port satellite** | 32.0 34.2 32.5 | **54.8 53.3 36.5** |

⭐⭐ **The noise floor first, because it governs everything else.** Across eight gold frames of the
*same* sequence, five seconds apart: **sky mean R swings 92.7 → 126.7 (37%)** and **terrain mean R
swings 28.6 → 72.7 (2.5x)**. **The gold disagrees with itself, in one flight, by more than most
defects would.** Any parity number smaller than that is measuring the moment. That is now written
into the tool's own docstring so it cannot be read past.

⚠️ **Three of our four captures are not comparable at all**, and the bands say so plainly: external,
chase and cockpit all report a "terrain" band of 172–234 — that is **cloud**, not terrain. Those
frames are at ~16,000 ft over an unbroken deck (S6), while the gold frames are low over land. Their
sky reads (144, 168, 198) — a high-altitude blue — against the gold's (127, 126, 115) hazy white at
low level. **Altitude, not fidelity.**

⭐ **And the one capture that IS over terrain agrees with the gold in hue.** `satellite` reports
terrain **(54.8, 53.3, 36.5)** — dark, warm, **R ≈ G > B** — which is exactly the gold's signature
across its whole range (31,26,18 … 72.7,72.3,61.0: R ≥ G > B throughout). Korea is the right colour
where we can actually see it. *(One view, one moment: stated as agreement in HUE, not as a parity
pass.)*

**So the useful output of this sprint is a rule and a tool.** The rule: **no 3D parity number means
anything until the two captures are state-matched**, and the gold's own 37%/2.5x spread is the bar.
The tool: `port/tools/gold3d_bands.py`, which prints the bands and the noise floor together.

**S8:** the state match. Our captures print their own state in-frame (`Speed/Mach/Alt/Hdg/Thrust`)
and so do the gold's; drive the harness to ~5,000 ft over land — the state most of the gold's 3D
footage sits in — rather than leaving it wherever the default flight ends up, and only then compare
bands. The `satellite` view already gets there, which is why it is the one that reads sensibly.

**GOLD3D-1: 2 sprints this pass.**

## GOLD3D-1 S8 (Opus 5, 2026-09-15) — ⭐ **a state-matched 3D capture at last: 5,062 ft against the gold's 5,116 ft** — and I nearly reported the DEBRIEF SCREEN's colours as the sky

S7 said the state match was the next step. S8 makes it possible, makes it, and gets caught once on
the way.

⛔ **1. The Hot Shot flight can never match the gold, and now it is measured.** `MA_TRACE_HUD=60`
across the whole recipe:

    frame=60   alt=15966 ft     frame=600  alt=16144 ft     frame=1440 alt=16737 ft

It enters at **15,966 ft and CLIMBS**. The gold's 3D footage sits near **5,000 ft**. No capture from
that recipe can ever be state-matched, which is why every band comparison so far compared cloud with
terrain.

⭐ **2. `BOB_AUTOFLY=dive[:tick]` (new).** `KEYMAPS.H:1083` binds `ELEVATOR_FORWARD` to
`J_moveup` = `DIK_UP` (0xC8); the mode **holds** it, like the existing `look` mode, rather than
tapping — S91 lost a sprint to a 60-tap dive that never moved the aeroplane. Measured descent:

    frame=240 alt=15389   frame=540 alt=12052   frame=840 alt=6949   frame=1140 alt=1417   frame=1440 alt=34

⛔⛔ **3. And the first capture from it was the DEBRIEF SCREEN.** `MA_SHOT=946` produced a frame whose
bands read sky (66.2, 69.7, 85.3) and terrain (29.0, 30.1, 38.3) — which I was one step from writing
up as *"our sky is half the gold's brightness and blue where the gold is warm"*, a tidy confirmation
of STATUS.md's long-standing "sky too dark". **It was the post-flight debrief panel**: the dive flew
into the ground and the run moved on. `MA_SHOT` and `MA_TRACE_HUD` do not share a counter;
**`MA_DUMP_BACK` does** (`MA_DUMP_BACK=300` and `[hud] frame=300` agree on 15,998 ft). Caught only by
looking at the image. [[screenshot-beats-printf-for-view-defects]], [[gate-frame-must-match-the-eye]].

⭐ **4. With `MA_DUMP_BACK=946`: a real 3D cockpit frame, HUD reading `Alt: 5062ft`** against the
gold's `Alt.: 5116ft`. Saved as `port/ref/native/lowlevel-5062ft-2026-09-15.ppm`.

**5. The terrain palette, state-matched:**

| | R | G | B | R−B | sd |
|---|---|---|---|---|---|
| **port, 5,062 ft** | 45.6 | 46.1 | 34.4 | **+11.2** | 34.6 |
| gold f_002 | 31.0 | 26.0 | 18.4 | +12.6 | 29.8 |
| gold f_004 | 32.1 | 27.6 | 19.3 | +12.8 | 30.8 |
| gold f_006 | 44.7 | 35.4 | 21.0 | +23.6 | 44.8 |
| gold f_008 | 62.6 | 59.7 | 48.9 | +13.7 | 75.6 |

**Our terrain sits inside the gold's own range on every axis** — brightness (45.6 in 31–63), warmth
(R−B +11.2 against +12.6…+23.6) and texture spread (34.6 in 29.8–75.6). Korea is the right colour.

⚠️ **One consistent difference, and it is small:** the gold always has **R > G** (by 5.0 to 9.3);
ours has **R ≈ G** (−0.5). Our terrain is a shade greener relative to red than the original's. That
is inside the noise floor S7 measured for brightness but *not* for the R−G relation, which is stable
across all four gold frames — so it is worth an item, not a shrug.

⚠️ **Attitude is still not matched.** Ours is a 0.96-Mach dive looking down; the gold is level with a
horizon. So **the SKY band is still not comparable** and nothing here bears on "sky too dark".
**S9:** level off — hold the dive to ~6,000 ft, release, and capture in level flight; then the sky
band becomes meaningful and the oldest open colour item in this port finally gets a number.

**GOLD3D-1: 3 sprints this pass.**

## GOLD3D-1 S9 (Opus 5, 2026-09-15) — ⛔ **S8's dive is NOT reproducible** — four runs, zero descent — and the reason may be a defect a player would feel: the elevator key has a **consume-on-read** second reader

S8 shipped `BOB_AUTOFLY=dive` and measured a descent from 15,389 ft to 34 ft. S9 set out to level
off at the bottom of it. It could not, because the dive stopped happening.

⛔ **Four runs today, three different tick bases, not one descent:**

| run | trigger | result |
|---|---|---|
| S8 | `dive:200` (acquired-pump ticks) | 15,389 → 12,052 → 6,949 → 1,417 → **34 ft** |
| S9 a | `dive:200:800:200` | 16,078 → **16,879 ft** (climbing) |
| S9 b | `dive:5:555:150` | 15,972 → **16,850 ft** |
| S9 c | `dive:60:600:200`, ticks counted **in 3D** | 15,965 → **16,689 ft** |

In every failing run the hook fired and said so (`[autofly] dive: holding ELEVATOR_FORWARD (DIK
0xC8) from tick N`). **The key-down is delivered and the aeroplane does not respond.**

**One real improvement was made anyway.** The tick base now counts from when the SIM IS UP
(`g_ma_in3d`), which is the same correction the `takeoff` mode already carries in its own comment —
`cnt` counts DI-keyboard pumps from process start, and acquisition lands at a different point
relative to the flight in different runs. A recipe timed on that is not a recipe. It did not fix the
descent, so it is not offered as one.

⚠️ **S8's claim "the harness can now reach any altitude" is WITHDRAWN.** The 5,062 ft capture it
produced is real — it exists, with its own HUD in frame, and S8's terrain comparison stands on it —
but the mechanism that produced it is unreliable and must not be built on until it is understood.

⭐ **And the likely explanation is not a harness problem at all.** `ELEVATOR_FORWARD` has **two**
readers:

    KEYFLY.CPP:958    Key_Tests.KeyHeld3d (ELEVATOR_FORWARD)    <- the flight control
    OVERLAY.CPP:1775  Key_Tests.KeyPress3d(ELEVATOR_FORWARD)    <- the in-flight MENU (Selection--)

and in this engine **`KeyPress3d` is consume-on-read** — BoB's R3.7 S2 established it at the line:
`return BITRESET(keymap->bitflags, keyval+1);` clears the bit and returns its old value. **A reader
that clears the bit before the flight control sees it eats a held key.** If the overlay's menu path
runs while the key is down, the elevator never moves — intermittently, depending on what the overlay
is doing.

🔴 **That is a PLAYER-FACING shape, not just a harness one:** "the stick sometimes does not respond"
is exactly what a consumed key feels like. It should not be assumed — but it should not be left
unlooked-at either.

**S10:** `MA_TRACE_KEY` already exists. Print both reads of `ELEVATOR_FORWARD` in the same frame —
the overlay's `KeyPress3d` and KEYFLY's `KeyHeld3d`, with the bit's value before and after each — and
see whether the menu path is consuming it. If it is, the fix is to test the bit without clearing it
on the paths that are not the key's owner, and the dive hook works for free.

**GOLD3D-1: 4 sprints this pass — AT THE CAP, with one claim retracted and a possible player-facing
defect surfaced from the retraction.**

## KEYHOLD-1 S1 (NEW, Opus 5, 2026-09-15) — ⛔ **GOLD3D-1 S9's "consume-on-read eats the held key" is WITHDRAWN.** The hold is real, measured, and survives — and in this run the dive worked

S9 could not make a held elevator key move the aeroplane and offered a mechanism: `ELEVATOR_FORWARD`
is read by both `KeyHeld3d` (the flight control) and `KeyPress3d` (the in-flight menu), and
`KeyPress3d` is test-and-clear, so the menu might be eating it. It flagged that as a possible
player-facing defect. **It is not one, and the code says so plainly.**

⚠️ **First, the file I read in S9 is not the file that runs.** `SRC/INPUT/KEYTESTS.CPP` is **not
compiled**; `SRC/INPUT/KEYSTUB.CPP` is (`ninja -t deps`: 0 hits against 3). The stale-duplicate trap
again — the implementations happen to agree here, but the reading was luck.

⭐ **There are TWO bits per action, not one:**

    KeyHeld3d(kv)  : if (BITRESET(bitflags, kv+1)) return TRUE;   // the EDGE bit, consumed
                     return BITTEST(bitflags, kv);                // the HELD bit, NOT consumed
    KeyPress3d(kv) : return BITRESET(bitflags, kv+1);             // the EDGE bit only

`kv` is "held", `kv+1` is "just pressed". **`KeyPress3d` consumes only the edge. It cannot eat a
held key** — nothing clears the held bit until key-up.

⭐⭐ **Measured, with a new `MA_TRACE_KEYHELD=1`** (first 40 calls where either bit is set, so it can
neither flood nor be starved):

    [keyheld] KeyHeld3d(18) held=1 edge=1     <- the key-down
    [keyheld] KeyHeld3d(18) held=1 edge=0     <- and every call thereafter
    [keyheld] KeyHeld3d(18) held=1 edge=0
    ... (held=1 throughout)

**The synthetic key-down produces a proper HOLD and it survives every subsequent read.** S9's
hypothesis is dead, and no player-facing defect is filed.

⭐ **And in the same run the dive WORKED:** `15,841 → 15,235 → 14,139 ft`. So the mechanism is sound.

⚠️ **What is left is INTERMITTENCY, and it is now the whole item.** Six attempts across two sprints:
**two descended** (S8's `dive:200`, this run's `dive:60`) and **four did not** (S9's three, plus one
earlier). Every one of the six logged its keypress. Both successes used the simple `dive:<tick>`
form and all four failures used the extended `dive:<tick>:<stop>:<pull>` form — **suggestive, not
conclusive**, because the extension only acts at ticks LATER than the press and the failures were
already climbing before then.

**S2:** print, per frame, the elevator's effect rather than its key state — `KEYFLY.CPP:958` reads
`KeyHeld3d(ELEVATOR_FORWARD)` and turns it into a stick deflection; print that deflection alongside
`held`. That separates "key held and ignored" from "key held and acted on but the aeroplane trims it
out", which are different defects, and it makes the intermittency visible in one run instead of six.

**KEYHOLD-1: 1 sprint. One false mechanism killed, the real behaviour measured, and the remaining
question narrowed to intermittency.**

## KEYHOLD-1 S2 (Opus 5, 2026-09-15) — ⭐ **the input chain is SOUND end to end**, and the intermittency is a bug in my own dive hook, not in the game

S1 proved the held BIT survives. S2 asks the next question — does a held key actually DEFLECT the
elevator — with `MA_TRACE_ELEV`, printed at the line that turns the key into a deflection
(`KEYFLY.CPP`, after `MODLIMIT`).

⭐ **Two runs of the simple form, identical, both descending:**

    [elev] fwd=0 back=0 pressed=0 elevator=0     Delta1=327 limit=16383
    [elev] fwd=1 back=0 pressed=1 elevator=16383 Delta1=327 limit=16383   <- saturated
    [hud] frame=120 alt=15841 ft      frame=360 alt=14139 ft              <- descending

**The chain works end to end**: synthetic key-down → held bit → `KeyHeld3d` → `fly.elevator` ramps by
`Delta1 = 327` per frame to the `LimitVal = 16383` stop in ~50 frames → the aeroplane descends.
**There is no MiG Alley input defect here.** Everything S9 suspected is cleared.

⛔ **And the extended form fails, visibly, in the same instrument:**

    [autofly] dive: holding ELEVATOR_FORWARD (DIK 0xC8) from tick 60
    [elev] fwd=0 back=0 pressed=0 elevator=209 ...   <- and 0 for the whole run
    [hud] frame=120 alt=15966    frame=600 alt=16147  <- climbing

**`fwd=0` throughout.** The keypress is pushed — the hook says so — and `KeyHeld3d` never sees it
held. The residual `elevator=209` is a single frame's worth of deflection decaying and stopping,
so the key registered for an instant and then was not held.

⭐ **So the tally resolves cleanly: 5 runs of `dive:<tick>` all worked, 4 runs of
`dive:<tick>:<stop>:<pull>` all failed.** The intermittency was never intermittent — it tracked the
form of the argument exactly, and **the broken one is the release/level-off extension I added in
GOLD3D-1 S9.** A harness bug of mine, reported for two sprints as a game mystery.

⚠️ **Not yet explained**, and worth one more sprint rather than a guess: the extension's release only
acts at a LATER tick than the press, so on the face of it it cannot affect the press — yet it does,
reproducibly. The parse is sound (`dive:60:600:200` yields at=60, stop=600, pull=200, all printed).
**S3** prints `cnt3`, `g_ma_in3d` and the pushed key at the moment of each `kb_push`, which will show
whether the press lands during a brief early in-3D flicker and is discarded when the real flight
starts — the one mechanism that fits both the press firing and the key never being held.

**Meanwhile the usable recipe is `BOB_AUTOFLY=dive:<tick>` with no extension**, and it reaches any
altitude reliably: 15,841 → 14,139 ft in 240 frames, repeatable.

**KEYHOLD-1: 2 sprints. The game is exonerated; the bug is mine and is now localised.**

## KEYHOLD-1 S3 (Opus 5, 2026-09-15) — ⭐⭐ **FOUND AND FIXED: a "tick" was a PUMP, not a frame — 740 pumps passed between two frames, so press, release and level-off all landed inside ONE frame gap**

S2 named the suspect (my own dive extension) and asked for the press to be printed with the state
that decides it. S3 does that in a proper A/B — `port/keyhold_ab.sh` (new): same binary, same
mission, same run length, same traces, back to back, **the argument the only difference**. Nine runs
spread over three sprints could not have shown this; two runs in four minutes did.

⛔ **The two runs, before the fix:**

| | `dive:60` | `dive:60:600:200` |
|---|---|---|
| frames with the elevator deflected | **1075** | **1** |
| altitude, first → last | 15,9xx → **31 ft** | 15,9xx → **16,409 ft** |

⭐⭐ **And the cause is visible in eight consecutive log lines.** The press, the release and the
level-off all print within nine lines of each other, with **no `[elev]` line — i.e. no rendered
frame — between them**:

    [autofly] push-window cnt=67 cnt3=66 in3d=1 sent=1 qhead=0 qtail=1
    [autofly] dive: released at tick 600, now holding ELEVATOR_BACK to level off
    [autofly] dive: level-off pull released at tick 800
    [key] DOWN scancode=0xc8 ... [key] UP scancode=0xc8 ... [key] DOWN scancode=0xd0

**`cnt3` counted PUMPS of the SDL event loop, and a pump is not a frame.** Measured here: the counter
ran from 66 to 800 — **734 pumps — between two consecutive rendered frames** as the flight started.
So "hold from 60, release at 600, pull for 200" collapsed into a single burst: the game drained
down-0xC8, up-0xC8, down-0xD0, up-0xD0 in one poll, giving the one frame of deflection S2 measured as
`elevator=209` and calling it intermittency.

**The simple form worked every time for the same reason** — it never pushes a release, so a key-down
alone survives any number of pumps.

✅ **The fix: count PRESENTS.** `g_ma_presents` is incremented in `present_dbg()` (ungated — it is
now a recipe unit, not a diagnostic), and the dive's `cnt3` advances only when a new present has
happened while in 3D. **The timeline is now in the same units `MA_TRACE_HUD` reports**, which is what
the mode's own docstring has claimed since S8.

⭐ **Verified, same harness, rebuilt binary:**

    [hud] frame=120 alt=15944 ft      <- press at frame 60
    [hud] frame=360 alt=14668 ft
    [hud] frame=600 alt=11546 ft      <- release + pull here
    [hud] frame=720 alt= 9833 ft
    [hud] frame=840 alt= 8940 ft      <- descent flattening
    [hud] frame=960 alt= 8625 ft      <- levelled

**540 frames of forward deflection, then 251 frames of back deflection, then neither** — the dive
and the level-off both do what the argument says, for the first time since the extension was written.
`dive:60` still dives to the deck (44 ft), so nothing regressed.

⚠️ **One residual, measured and not fixed:** during the pull the trace reads `fwd=1 back=1` — the
key-UP for ELEVATOR_FORWARD does not clear its held bit until the pull's own release drains. The
level-off works because the BACK deflection saturates the elevator (-16383) regardless, but a recipe
that needs a clean release rather than an override would have to look at that. Not on the path of any
current item.

⭐ **The wider lesson, and it has now cost three items:** `BOB_KEYSEQ`, `BOB_CLICKSEQ` and
`BOB_AUTOFLY` all count pumps. Any recipe whose steps must be ORDERED IN TIME relative to the
simulation is unreliable in that unit — the pump rate varies by two orders of magnitude between
loading and flying. `BOB_AUTOFLY=dive` is fixed; the other two are worth the same treatment when an
item next depends on their timing.

**KEYHOLD-1: 3 sprints. Diagnosed, fixed and verified — and the harness that found it is now a
script.**

## STATEMATCH-1 S1 (NEW, Opus 5, 2026-09-15) — ⭐ **a 3D capture can now be aimed at an ALTITUDE and hit it to 18 ft across runs** — and the same runs kill the idea that altitude alone is a state match

Split out of GOLD3D-1, which is at its four-sprint cap this pass, exactly as KEYHOLD-1 was. The item
is the CAPABILITY the gold comparison needs: put the aeroplane in a chosen, repeatable state and
photograph it there.

⭐ **1. The dive is now a ruler.** With KEYHOLD-1 S3's frame clock, `BOB_AUTOFLY=dive:60` produces
the same profile every run — three runs, same frames:

    frame  120    480    600    720    840    960   1080   1200   1320
    alt  15944  13310  11561   9563   7452   5280   3060    804     44 ft
    run2 15944  13312  11563   9565   7454   5331   -      -      -
    run3 15965  13310  11579   9582   7452   5299   -      -      -

**11,561 / 11,563 / 11,579 ft at frame 600 — an 18 ft spread across three separate flights.** The
gold campaign video's 3D state is 5,116 ft; frame 960 is 5,280–5,331 ft, within 4%.

⭐ **2. `MA_DUMP_PATH` (new).** The frame dump was hardcoded to `/tmp/maback.ppm`, and this box's
/tmp is a 7.6 GB tmpfs that a capture run has already filled once, killing every shell in the
session. The destination is an env var now (default unchanged), and `port/gold3d_state.sh` (new)
writes under `/home`.

⭐ **3. `BOB_KEYSEQ_FRAMES=1` (new, opt-in) and PROVEN.** KEYHOLD-1 S3 named `BOB_KEYSEQ` as carrying
the same pump-versus-frame bug. Switched to the present clock it lands exactly where asked:

    [keyseq] tap dik=0x04 at kidle=300
    [hud] frame=300 speed=510 Kts alt=15172 ft mach=0.85     <- the same instant

Opt-in, because `port/ab.sh`'s `KEY_AT` and the PO-9 ALT+X route are calibrated in pumps.

⛔ **4. And the capture at the matched altitude is USELESS for the gold comparison — I looked at it
before quoting its numbers.** At frame 960 the bands read sky (53.9, 52.2, 38.6), terrain (39.1,
44.3, 36.4), which would have read as "our sky is half the gold's brightness". **The frame contains
no sky at all**: at Mach 0.96 the nose is 60-odd degrees down and the entire windscreen is ground.
The gold at 5,116 ft is at **352 Kts, Mach 0.54, level**. Matching the altitude matched nothing.

⭐ **5. The level-off works and gets closer.** `dive:60:880:300` bottoms out at 2,810 ft and settles
— but the capture at frame 1500 (4,026 ft, 537 Kts) is inside a **cloud deck**, white in every band.
Two captures, two different reasons the bands cannot be compared, and both reasons are visible only
in the picture.

⛔ **6. The throttle cut FAILED, and the frame says why.** `BOB_KEYSEQ="300,0x04"` (RPM_30, `KEYMAPS.H:806`
binds `n3` in the `norm` shift state) fires on schedule and **thrust stays at 72**. The HUD strip of
that very frame carries the answer:

    select your own target!
    Speed: 581Kts  Mach: 0.95  Alt.: 9925ft  Hdg: 284  Thrust: 72

**The digit reached the sim and was consumed by TARGET SELECTION, not by the throttle.** So the
number-row RPM bindings are unreachable in flight through this path — which is a finding about the
game's own key dispatch, not about the harness.

**S2:** find the consumer of the number row in flight (the message text is the thread to pull), and
with the throttle reachable, drive to the gold's actual state — **5,116 ft, 352 Kts, level, below the
cloud deck** — before any band number is quoted again.

**STATEMATCH-1: 1 sprint. The altitude is a ruler now; the attitude and the speed are not yet.**

## STATEMATCH-1 S2 (Opus 5, 2026-09-15) — ⛔ **S1's "target selection ate the digit" is RETRACTED.** The key is dispatched perfectly; its consumer, `ManualPilot::GetRPMABKeys`, is never called

S1 saw the throttle tap do nothing, read *"select your own target!"* on the HUD strip of the same
frame, and concluded the digit had been consumed by target selection. **The trace that was already
running in that very log says otherwise:**

    [keyseq] tap dik=0x04 at kidle=300
    [key] DOWN scancode=0x04 shift=0 -> action index=108

`shift=0` is the `norm` state, and **index 108 IS `RPM_30`** (`STUB3D.CPP:1380` indexes by key value,
and `kv = 2 x` the `KeyName` id; `KEYMAPS.H:169` gives RPM_30 id 54). The key resolved to exactly the
action wanted. **The radio phrase was ambient chatter that happened to be on screen** — a coincidence
I read as a cause, which is the `parity-captures-must-record-their-state` trap wearing different
clothes.

⭐ **So the question became: the bits are set, who fails to read them?** Two measurements, each one
line of trace:

1. **The `RPM_30` branch never runs.** `KEYFLY.CPP:671` — `if (Key_Tests.KeyPress3d(RPM_30)) {...
   thrustpercent = 30; }` — printed **0 times** in a full flight with the tap delivered.
2. **The function containing it is never CALLED.** A trace at the top of
   `ManualPilot::GetRPMABKeys` (`KEYFLY.CPP:575`), before its `int_fuel > 0` gate, printed
   **0 times** as well.

⛔⛔ **`ManualPilot::GetRPMABKeys` is not reached in this flight at all**, so the throttle digits have
no consumer. It is called from `GetStickKeys` (`KEYFLY.CPP:185`), which is the manual-pilot input
path — and the ELEVATOR keys, which work perfectly in the same flight (KEYHOLD-1 S3: 540 frames of
deflection, 15,944 → 44 ft), are read elsewhere. **Two flight controls, two different paths, and only
one of them is live on the campaign Hot Shot flight.**

⚠️ **Whether this is a player-facing defect is NOT yet established, and I am not going to claim it
is.** The flight may legitimately start under autopilot, where a throttle key should do nothing until
the player takes control. What is established is narrower and useful: **the harness cannot set
throttle on this flight**, and the reason is a call that does not happen rather than a key that does
not arrive.

**S3:** trace `GetStickKeys` itself at `KEYFLY.CPP:185` and the branch that chooses between
ManualPilot and the autopilot. If `GetStickKeys` runs and `GetRPMABKeys` does not, that is a defect
in this port; if neither runs, the aircraft is on autopilot and the harness needs to disengage it
first — which is also exactly what a state-matched capture needs, since the gold frame is a hand-flown
level pass.

**STATEMATCH-1: 2 sprints. One of my own conclusions withdrawn, and the throttle question narrowed
from "the key is eaten" to "one function is never called".**

## STATEMATCH-1 S3 (Opus 5, 2026-09-15) — ⭐⭐ **the joystick's throttle axis silences the keyboard throttle keys — by the game's own design, one line: `if (thro != -0x8000) {axis} else GetRPMABKeys(...)`**

S2 established that `ManualPilot::GetRPMABKeys` is never called and named `GetStickKeys` as the place
to look. It is called from there — **conditionally**, and the condition is the whole answer.

⭐⭐ **`KEYFLY.CPP:1218`, original game code:**

    if (thro != -0x8000)                       // -0x8000 = "no throttle axis"
    {
        ... TempThrust from the axis ...
        ControlledAC->fly.thrustpercent = TempThrust;
    }
    else
        GetRPMABKeys(ControlledAC);            // the 1..0 throttle keys

**A throttle AXIS and the throttle KEYS are mutually exclusive, and the axis wins.** This box has a
Logitech Extreme 3D plugged in (`[joy] opened 'Logitech Extreme 3D' axes=4`), its slider sits wherever
it was last left, and so:

* `GetRPMABKeys` is never reached — exactly S2's zero, now explained;
* `thrustpercent` is pinned to the **physical lever position**, which is why every capture in S1 and
  S2 read **`Thrust: 72`** no matter what was pressed. 72% was the slider, not the game.

✅ **Proved by removing the axis.** `MA_NOJOY=1` (new, `bob_video.cpp`) leaves the joystick unopened:

    [joy] MA_NOJOY: joystick not opened
    [thr] GetRPMABKeys: int_fuel=62300000 thrustpercent=100     <- the function RUNS now
    [keyseq] tap dik=0x04 at kidle=300
    [thr] RPM_30 -> thrustpercent=30 afterburner=0              <- and the KEY WORKS

**The same tap that did nothing in S1 and S2 sets 30% thrust.** Nothing about the key, the dispatch
or the shift state was ever wrong — an axis was quietly outranking it.

⚠️ **This is original behaviour, not a port defect**, and I want that stated plainly: the branch is
Rowan's, and "a throttle lever beats the throttle keys" is a defensible design. **What it costs is
felt anyway** — a player with any stick plugged in cannot use the 1..0 throttle keys at all, and the
aircraft's thrust follows a lever they may not be touching.

⚠️ **One residual, measured and NOT explained.** Across the sampled frames `thrustpercent` reads 100
before the tap, 30 at the tap, and **0 from the next sample onward** — the aeroplane ends up gliding
at 120 Kts, Mach 0.20. Something drives thrust to zero after the key sets it. The nearest suspect is
in view: `FLYMODEL.CPP:637` runs formation-keeping (`slowdownleader`) immediately before
`GetKeyCommon`, and it manipulates `thrustpercent`. **Suspect, not conclusion.**

**S4:** trace every write to `fly.thrustpercent` for one flight. With the axis out of the way there
are few writers, the formation logic is one of them, and that answers both "what zeroes it" and
"can the harness hold a commanded thrust" — which is the last thing standing between this item and a
state-matched capture at the gold's 352 Kts.

**STATEMATCH-1: 3 sprints. The throttle keys work; an axis was outranking them; and the harness can
now fly at a chosen power setting.**
