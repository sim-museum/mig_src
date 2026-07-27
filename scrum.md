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
| B7 | As a player, the F-86 radar gunsight ranges/expands with target range, so gunnery is accurate. | 8 | `DOGUNSIGHT` reticle scales with locked-target range on the software path. | ⬜ |

### EPIC C — Input

| ID | User Story | Pts | Acceptance Criteria | Status |
|---|---|---|---|---|
| C1 | As a player, I can fly with keyboard via DirectInput→SDL, so controls work in 3D. | 13 | Pitch/roll/yaw/throttle + view keys mapped SDL→engine; responsive in flight. | ✅ (S3: view-pan demo, 115 actions) |
| C2 | As a player, I can use a joystick, so flight is natural. | 8 | SDL game-controller/joystick axes→flight controls; deadzone/calibration. | ✅ (S10: live fly-validated) |
| C3 | As a player, mouse navigation works across all menus, so the UI is complete. | 5 | Click/hover hit-testing on all front-end panels (extends current listbox/button/combo). | ✅ (S2–S4 front-end; S18 in-flight `AU_UI_X/Y`) |
| C4 | As a player, SHIFT+D boxes the padlocked bogey and ALT+D shows its telemetry, so I can track targets (the Wine two-patch feature). | 13 | SHIFT+D draws a red box around the padlocked bogey (3D→2D projected); ALT+D adds text beside it: bogey kts [closure], range (ft→Nm), own kts @ relative alt. Toggleable. | 🔨 **baseline works** (engine `d`/`BOXTARGET`: red diamond + Range/Bearing/RelAlt, PO-verified). Enhancements (this sprint): C4a box-scales-with-range (enclose, don't intersect); C4b SHIFT+D=box / ALT+D=telemetry split; C4c adaptive black/white telemetry color; C4d add bogey-kts[closure] + own-kts to readout. |

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
| G2 | As a player, I can play the campaign across missions, so the game is complete. | 21 | Campaign state load/save; mission chaining; debrief. | ⬜ |

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
| I1 | As the PO, I have an inventory mapping each gold screenshot to its native screen and repro path, so parity work is scoped and diffable. | 3 | All 14 shots identified (screen name + native nav/env recipe + native capture alongside); table in `port/scrum/screen-parity.md`. | ✅ S56 (13/15 native captures in `port/ref/native/`; title+debrief pending; oracle = BDG 0.85F — provenance flagged) |
| I2 | As a player, every 2D front-end screen (title, Preferences tabs, Quick Mission, Campaign panels, map) matches its Wine gold shot. | 13 | Side-by-side native-vs-gold captures agree on layout, art, fonts, colours within stated tolerance; each deviation fixed or explicitly PO-waived in the parity table. | 🔨 S58: **verdicts flipped on real captures — #7 Controls CLOSE, #8 Others CLOSE, #1 title first-captured CLOSE** (S57 fixes capture-verified); 2D captures now GL-free (`MA_SHOT`, byte-identical to GL runs) + uninit-PX ctor fix (`RLISTBXC.CPP`) cleaned tab bar/title menu. Open: #9 stray combo (in-template, runtime-hidden on Windows — mechanism unrouted), text word-wrap, cross-cutting font/chrome (#1/#2), #12 debrief capture |
| I3 | As a player, the in-flight / 3D / campaign-map views match their gold shots. | 13 | As I2 for the 3D-view shots; reuses `MA_DUMP_BACK`/frame-dump harness with `GL_PACK_ALIGNMENT=1` (S45 lesson). | ⬜ |
| I4 | As a player, the campaign-map **Player Log** OOB dialog matches the Wine gold shot (PO-added 2026-07-26): Career tab with pilot photo, Name edit box, per-type Sorties/Combats/Kills/Losses table (F86 1 / F86 2 / F80 / F84 / F51 / All), Career / Log of Missions / Last Mission tab bar, ?/✓ title buttons — over the strategic map with toolbar + date "6/25/50: Morning, planning". | 8 | Gold: `/home/admin/Pictures/Screenshots/Screenshot From 2026-07-19 20-33-27.png` (treat as gold shot #15). Native Playerlog capture (S54 OOB path) side-by-sides it in `port/scrum/screen-parity.md`; content populated (photo art, table rows, editable Name), all three tabs render; deviations fixed or PO-waived. | ◐ S56: first native capture landed (`MA_OOB_PLAYERLOG` hook, `port/ref/native/map_playerlog.png` — photo art over map; frame/tabs/table still missing, named in parity table) |

**Backlog total (open work): ~250 pts** → ~10–12 sprints at re-baselined velocity.

---

## 5. Sprint Plan (rolling)

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
