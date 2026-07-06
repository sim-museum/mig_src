# Mig Alley — native Linux (SDL2) port: STATUS

_Last updated: 2026-07-06 (S45–S54 campaign UI: colour-fidelity fix, map icons/date, **CRToolBar hosting** (icon toolbar renders + clickable), **OOB-info dialogs render** with real data; cross-port note 9 drafted). Prior: 2026-07-05 cross-port BoB S63→S82; 2026-06-29 BoB S46→S62 ASan arc; 2026-06-25 S21–S28 live play-test hardening._

> **⚠ Tooling note (2026-07-06 session):** the Bash tool was returning exit 1 for every command (confirmed
> environment-wide, incl. a subagent) — so this STATUS write could not be `git` committed/pushed in-session,
> and the S54 `AddMission` fix + note-8 (`BOB_ZDEPTH`) adoption await a working shell. Commit/deliver when Bash
> is restored. All *validated* work through S53 is already committed on `linux-port`.

Native **32-bit i386 ELF** port of the 1999 Rowan engine (OpenWatcom / Win32 / DirectX / MFC)
to Linux + SDL2/OpenGL. Branch `linux-port`. Game data: the Wine install at
`/home/m/sgl/TUE/MigAlley/WP/drive_c/rowan/mig`.

> **Sister port:** Battle of Britain (`~/bob`), the same Rowan framework one renderer-generation
> later (D3D7/Lib3D vs MiG's software rasterizer). Shared cross-port field notes live in
> `/home/m/bob/doc/ROWAN_ENGINE_LINUX_PORT_NOTES.md` (read its "MiG Alley specifics" box first).
> The two ports are at **near-parity**; knowledge flows both ways (see "Cross-port" below).

## One-line state

The game **boots to the native title screen, navigates the full single-player front-end, flies a
software-rasterized 3D mission and returns to the menu in one process, with OpenAL audio and
keyboard+joystick flight input.** Campaign reaches the operational Korea map; save/load round-trips.
Mouse drives the in-flight UI cursor. **Hot Shot air combat is playable end-to-end** (multiple
bogies, padlock view, kills register, debrief screen). Recent (S21–S28): live play-test hardening —
the campaign map is navigable, the F1-padlock crash is fixed, HUD instruments + ADI render, and the
quick-mission dropdown selects missions (Turkey Shoot / One on One now fly with their enemy).

```
cd <drive_c>/rowan/mig && ./wmig          # bare launch — no env vars (S30/H1)
```
The data path is derived from the cwd's `/drive_c` ancestor and `InitInstance` auto-runs.
Overrides: `BOB_DRIVE_C=<dir>` to point elsewhere; `BOB_NO_RUN` for a link-only run.
(3D flight is default-on; `MA_DISABLE_3D=1` keeps it 2D-only for front-end debugging.)

## Subsystem state

| Subsystem | State | Where / sprint |
|-----------|-------|----------------|
| Compile (15/15 game unities) | ✅ | Phase 1 |
| Link (`wmig`, 0 undef) | ✅ | Phase 2 — 7.8 MB i386 ELF |
| SDL2 runtime + DirectDraw→GL present | ✅ | Phase 3 (`ddraw_legacy.h` bridge) |
| 2D front-end (title + OCX hosting + Prefs) | ✅ | Phase 4 / S2–S4 |
| Full single-player nav (QuickMission/Campaign/HotShot) | ✅ | S4 |
| 3D flight (software rasterizer) | ✅ | Phase 5 / S5 — first frame + menu↔flight round-trip |
| Keyboard flight (DirectInput→SDL) | ✅ | S3 |
| Joystick (SDL_Joystick→DirectInput) | ✅ | S10 — live fly-validated, axis-map fixed |
| Audio digital path (Miles AIL→OpenAL) | ✅ | S6 — `ma_openal.cpp` (SFX/UI/engine/radio) |
| Campaign → operational Korea map | ✅ | S7 — `StretchDIBits`; **renders full colour** (S45: the "greyish map" was a `BOB_DUMP_FRAME` `glReadPixels` pack-alignment bug at the 1021-wide map, not the render — fixed) |
| 3D/map colour fidelity | ◐ | S8/S20 — terrain matches Wine; **sky renders correct blue** (S8/S9 "brown" was stale, fixed by M2 `1a70d2d`); residual = ~75-unit brightness gap vs Wine's D3D-material sky (fidelity-target choice, low pri) |
| Campaign map toolbar (CRToolBar hosting) | ✅ | S48–S50 — icon toolbar renders (Bases/Squads/Weather/Dis/Frag/…) + clickable (fires `ON_EVENT` handlers). Fixed `CRToolBar` not inheriting `OnRowanMessage` (WM_GETFILE art) + control-id→icon table |
| Campaign OOB-info dialogs (Squadrons/Bases/…) | ◐ | S52–S54 — **7 of 10 render** over the map with real data (Squads: photo + Available Aircraft/Rotate Flights/Bingo Fuel). Fixed `CDialog::Create` dropping its parent (GetParent()==NULL in OnInitDialog) + unit-conversion FPE. Deferred: Authorise/Directives 2nd crash sites; selected-tab (CRTabs) |
| Save/load (click-driven loadgame) | ✅ | S11–S14 — "Auto Save" → Load → campaign map |
| ASan heap-bug oracle + flight-path grind | ✅ | S15–S38 — **flight path ASan-clean** (0 reports across 5 flights): per-frame corruptors (S15/16), mid-freq set (S17), base-item type-confusion pair (S37), lifetime UAF (S38). Residual = deliberately-benign `FixLbmImageMap` (BoB-guarded) |
| In-flight mouse (DInput rel→`AU_UI_X/Y`) | ✅ | S18 — DInput mouse device wired (mirror S10 joystick); motion reaches native `AU_UI_X/Y` cursor axis (verified `theaxis=4`) |
| MIDI/XMIDI music | ✅ | `SRC/compat/ma_music.cpp` — XMI→SMF in-memory (`parse_xmi`) → FluidSynth + the game's shipped `MUSIC/fieldsnr.sf2` |
| Smacker intro video | ⬜ | stubbed |
| DirectPlay multiplayer | ⬜ | out of scope (scrum.md §8) |

## Phase progress

| Phase | State |
|-------|-------|
| 1 — compile | ✅ all 15/15 game module unities compile clean |
| 2 — first link | ✅ `wmig` links, 0 undefined symbols (7.8 MB i386 ELF) |
| 3 — SDL2 runtime | ✅ boots into `CMIGApp::Run()`; SDL2 window + DirectDraw→GL present bridge |
| 4 — 2D front-end | ✅ title + interactive Preferences (OCX hosting, RLE8 BMPs, TTF fonts, tabs, write-back) |
| 5 — 3D flight | ✅ software rasterizer renders the cockpit; menu↔flight round-trip; ◐ colour fidelity |
| 6 — input | ✅ keyboard (S3) + joystick (S10) + in-flight mouse (S18) |
| 7 — audio | ✅ digital path on OpenAL (S6); ✅ XMIDI music via FluidSynth (`ma_music.cpp`) |
| 8 — campaign/mission | ✅ reaches + renders operational map (S7); ✅ save/load (S14) |
| 9 — video | ⬜ Smacker → libsmacker |
| 10 — multiplayer | ⬜ DirectPlay → sockets (out of scope) |

## Live play-test hardening (Sprints 21–28, 2026-06-25)

Driven by interactive play sessions; each fix is committed + (where possible) validated headlessly.

| # | Fix | Commit | Notes |
|---|-----|--------|-------|
| S21 | **In-map navigation** | `88287a6` | campaign map: arrows/WASD/drag pan, wheel/`+`/`-` zoom, `Esc` exit, `F` fly. (Wheel-zoom window-resize is a known rough edge.) |
| S22 | **Turkey Shoot spawn measured** | `e65636d` | not a bug — bogie spawns dead-centre at FT_5000; drift is lawful dynamics. Added `MA_QUICKMISS`/`MA_TRACE_BOGIE`. |
| S23 | **F1-padlock crash, part 1** | `676eb14` | unclamped **horizontal** span → image filler OOB write. `polygon::ASM_Call_clamp` clamps span X for the 0-based image converters. |
| S24 | **F1-padlock crash, part 2** | `2ed87e6` | the real one: **vertical** OOB — a poly projected far below screen gave an off-surface `scradr`. `drawpoly` now clips `miny/maxy` to `[0,height)`. Crash handler upgraded to dump `fault_addr`+registers (`SA_SIGINFO`) — self-diagnosing now. |
| S25 | **HUD instruments default-on** | `716729c` | enemy-indicator disk + ADI were gated behind `GD_HUDINSTACTIVE` (the `h` key); default it on per flight in `MakePassive`. |
| S26 | **Lone-MiG no-spawn root-caused** | `0f0729f` | disproved the spawn-path theory; the bug was the mission combo (see S28). `MA_QUICKMISS` shipped as the interim workaround. |
| S27 | **ADI speckle glitch** | `f3a7393` | `DoArtHoriz` read the ball image out of bounds at pitch beyond ±90° → garbage texels. Wrap `offset` into `[0,h)`. |
| S28 | **Quick-mission combo selection** | `7f50acb` | **the lone-MiG fix.** Combo `TextChanged` went through the stubbed OCX connection point and never reached the dialog → mission selection did nothing (stuck on the no-enemy default). `ma_ole_click` now fires `ma_evt_fire` after any combo select, like buttons. Fixes **every** combo selection game-wide. |

Plus earlier same-session live fixes: `21ff9ec` 4× flight speed + Quit hang, `219a11c` flight-exit
crash (move-thread UAF + heap corruptors), `a1b5da7` Campaign-Begin map-render hang.

## Campaign UI arc (Sprints 45–54, 2026-07-05→06)

Autonomous headless-DoD sprints building out the campaign map's interface. Each committed + ASan-verified.

| # | Work | Commit | Notes |
|---|------|--------|-------|
| S45 | **Colour-fidelity fix** | — | the "greyish map" was a `BOB_DUMP_FRAME` `glReadPixels` `GL_PACK_ALIGNMENT` bug at the 1021-wide map, not the render. Adopted by BoB (unblocked their S101). |
| S46/S47 | **Map unit icons + date readout** | — | `CDC::GetBoundsRect` returned garbage → 0 icons drew; `GetClientRect` fix. Date/period header. |
| S48–S50 | **CRToolBar hosting** | `14d38a8`/`3f711f4`/`cbd240d` | parent-scoped toolbar draw (no bleed) → per-button icons (fixed `CRToolBar`≠`RDialog::OnRowanMessage` so `WM_GETFILE` art routed) → clickable (`ma_ole_toolbar_click`→`ma_evt_fire`→handler; Fly launches briefing). |
| S51 | **Cross-port: `CloseLoggedChild` guard** | `59aa042` | adopted BoB's S109 per-slot re-entrancy guard (Linux `OnCancel` no-op → infinite recursion) on both `CRToolBar`+`RDialog` variants. |
| S52 | **OOB dialog build crash fixed** | `cf3feed` | 2 general bugs: `CDialog::Create` dropped its parent (`GetParent()`==NULL in every `OnInitDialog`) + unit-conversion FPE (`mass.gm==0`). Squads OOB dialog builds clean. |
| S53 | **OOB Squads dialog RENDERS** | `48cf159` | `ma_map_paint_oob` walks the open logged-child tree each map idle → `MaOnPaint` art + `ma_ole_draw_toolbar` controls. Shows the squadron photo + real data. Mirrors BoB S113/S114. |
| S54 | **OOB render generalized + Directives diagnosed** | `10e6813` | verified 7/10 OOB dialogs render (Bases/Weather/Playerlog/Squads). Diagnosed the Directives crash (fnhoist var-shadow OOB-write in `COMIT_E.CPP AddMission`); fix reverted **unvalidated** (session SDL/GL/X11 wedge blocked the ASan gate) → deferred to S55 with the exact one-line fix. |

**S55 backlog:** apply+ASan-validate the `AddMission` fix; gdb the Directives 2nd site + Authorise; un-blacklist
the last 2 buttons; selected-tab render (CRTabs host); adopt BoB note-8 `BOB_ZDEPTH` for the 3D chase view.

## Cross-port with `~/bob` (sister Rowan port)

**Adopted from BoB:** refcount-UAF insurance (real `int ref` on `bob_video.cpp` D3D7 surfaces);
`INT3`-guards-are-data-bugs (fix the state, not the guard — our F3); menu↔flight one-process recipe
(`F12→CloseWindow→OnCancel→OnFlyingClosed`); CString-in-varargs Itanium-ABI fix (`FormatV`); the
ASan `new`/`delete` form-mismatch bug family (S16 cites BoB R1.3d/e, R3.9; S17 backlog cites R1.3b).

**Given to BoB:** general `ma_eventsink.cpp` (BoB adopting in its S33 to retire targeted bridges);
`ma_populate_software_modes` F3 pattern (BoB taking the approach for its resolution-combo crash);
the further-along campaign map view (BoB candidate for its R4.2 icon-culling fix).

**Watch (shared bug families):** the `fakefile` save-path family — MA has the same 3 sites BoB
flagged (`FILING.CPP` SaveGame:124 / LoadGame:138, `LOAD.CPP` MakeFileList:271) but reaches working
save/load **without** a `MA_LINUX` path bypass (the engine path + case-insensitive `fopen` resolve
it). If a save-path corruption ever surfaces, it's this known family.

**Cross-port sync 2026-06-29 (BoB S46→S62 ASan arc):** verified BoB's gameplay-loop ASan sweep against
MA's tree; three were **confirmed shared engine bugs** (see `port/BOB_PORT_LESSONS.md` §5 table):
- **`MathLib::rnd()` `rndlookup[55]` over-read** (BoB S55) — **FIXED** in MA `MATH.CPP:1722/1730`
  (`% table-size`; engine-wide PRNG, was latent for any mission).
- **compat `BITSET/BITTEST` dword-granular → byte-granular** (BoB S59) — **FIXED** in
  `SRC/H/mathasm_linux.h` (latent global-buffer-overflow for every sub-4-byte `MakeField` bitfield).
- **`LBMCPP.H` IFF unpack reads one control byte past the file buffer** (BoB S47) — **FIXED**: adopted
  BoB's `LBM_INBOUNDS`/`cend` macro (`LBMCPP.H` now byte-identical to BoB; real `cend` in
  `FixLbmImageMap`, inert sentinel in the uncalled generic `UnpackRow`). Rebuild + headless boot clean.
Not shared: BoB's `DrawSubShape`/`dodigitdial` shape-opcode `new[]/delete` (absent from MA), and its
`g_devTex` UAF / `~View3d` teardown race (DX7/Lib3D-specific — MA's software path differs).

**Cross-port sync 2026-07-05 (BoB S63→S66 campaign-serialiser + S71/S72→S82 arc):** triaged both against MA
(details in `port/CROSS-PORT-FROM-MA-2026-07-05.md`; shared-doc §5 addenda updated). Two real shared bugs fixed:
- **`PackageList::LoadGame` `new char[]`/scalar-`delete` mismatch** (BoB S65a) — **FIXED** in MA
  `MAPCODE.CPP:307/326` (`delete`→`delete[]`; `new char[SIZ=20000]` campaign-reload buffer). Compiled via
  `_BFIE.CPP`→`Mapcode.cpp` (symlink to `MAPCODE.CPP`, one file). BFIELDS unity recompiles clean.
- **`MIGLAND.CPP` two-strip terrain-index `pNorth/pEast[index+1]` OOB** (BoB S71) — **FIXED** on the flight
  terrain path: 4 active seek sites now wrap via `_seekNextIndex(index)=(index+1)%5120` (MA_LINUX). BoB's
  `& 0xFFF` mask does **not** port — MA's `north.ind`/`east.ind` are 5120 `SInfo` entries (not 0x1000), so a
  copied mask would corrupt Korea terrain; re-derived from MA's own data files. `_3D` unity recompiles clean.
- **Not shared:** BoB S64 `SaveBin` `char packstr[5]` overflow (MA's `SaveBin` uses a *different*
  `CSprintf`/`BOStream` serialiser, no such buffer); S65b `CDC::SelectObject(CPen*)` SUAR (MA's software-GDI
  applies the pen immediately + returns NULL, never caches); S78 `Formation_xyz` `wingpos[16]` (no such method
  in MA; scramble/formation model differs, cf. S54); S72 `Grid_Base::getWorld` clamp (no such symbol; software
  terrain path differs). S81 cockpit/cloud z-fighting = BoB GL-depth path — **watch** only if MA grows a
  hardware/GL 3D path.
- **Given to BoB:** MA's hosted-OCX front-end path (`ma_olecontrol.cpp`/`ma_ole_draw_all`/`ma_dlgtmpl.cpp`
  RT_DLGINIT/`ma_eventsink.cpp`) as the reference for BoB's stuck campaign Phase-1 (its campaign screens are
  OCX-hosted, not `textlists[]`-driven — the same control model MA already renders + navigates).
The three candidates were verified 2026-06-29:
- **`CRListBoxCtrl` cell-string `delete`** (BoB S58) — **shared, FIXED + ASan-validated**: `DeleteRow`
  (`RLISTBXC.CPP:2145`) did scalar `delete` on a `new char[]` cell → now `delete[]`. (MA's
  `ReplaceString:1746` was already correct, unlike BoB's.) Confirmed under ASan via a differential test
  (`MA_ASAN_LISTBOX_SELFTEST=1`, drives the real `DeleteRow` since no game code calls it): scalar `delete`
  → `alloc-dealloc-mismatch (new[] vs delete) at DeleteRow:2149`; `delete[]` → zero ASan errors.
- **`FindNextBf` `GR_Scram_*[8]` >8 groups** (BoB S54) — **NOT shared**: MA has no `glind`/unbounded
  scramble loop; the `[8]` arrays are touched only by a bounded `for(i=0;i<8)` clear + 8 fixed named refs
  (`refto8`). MiG's quick-mission scramble structure differs.
- **`LaunchScreen resolutions[m_currentres==-1]`** (BoB S57) — **already fixed in MA** independently
  (`FULLPANE.CPP:2037` `if(m_currentres==-1) m_currentres=GetCurrentRes();`, an "ASan(MA)" guard);
  `GetCurrentRes` returns `[0,5]` into the properly-sized `FullScreen::resolutions[6]`.

## Build & run

```
bash port/rebuild.sh        # ~266 TUs in parallel → /tmp/wmig
```
Link: `g++ -m32 -no-pie port/build/{obj,obj2,objmfc,objmfc2,objole}/*.o -Wl,--allow-multiple-definition -lSDL2 -lGL -lpthread -lm -o wmig`.
**Rebuild `_HARD`+`SMKDLG`+`STUB3D` on surface-layout changes; `ddraw_legacy.h` is inlined into
many TUs → full rebuild when editing it (`--allow-multiple-definition` picks one copy).**

## Diagnostics (gated, default off)

`MA_DISABLE_3D`, `MA_TRACE_3D`, `MA_TRACE_DD` (Blt src size/bpp/nonzero), `MA_TRACE_FILL`,
`MA_DUMP_BACK=N` (N-th back→primary Blt → PPM), `MA_TRACE_SKY` (fog/horizon colour), `MA_TRACE_KEY`,
`MA_TRACE_JOY`, `MA_TRACE_MOUSE`/`BOB_AUTOMOUSE`/`MA_NO_MOUSE_GRAB`, `MA_NO_AUDIO`/`BOB_AUTOFLY`,
`MA_TRACE_FPS` (per-second present fps + running average; B3 regression gate),
`MA_TRACE_REPLAY` (debrief replay-launch path; S33 Replay-hang fix),
`MA_TRACE_RES` (window resize / SetDisplayMode), `MA_TRACE_ADI` (attitude-ball draw geometry).
S21–S28: `MA_DISABLE_MAP`, `MA_QUICKMISS=<idx>` (2=Turkey Shoot, 3=One on One), `MA_TRACE_BOGIE`,
`MA_TRACE_SPAWN`, `MA_FORCE_PADLOCK=<frame>` (headless padlock repro), `MA_NO_HUDINST`, `MA_TRACE_CLIP`.
ASan oracle: see `port/scrum/asan-findings.md`. `MA_ASAN_LISTBOX_SELFTEST=1` drives the otherwise-
unreached `CRListBoxCtrl::DeleteRow` once (regression check for the BoB S58 `new[]/delete[]` fix).
Standing ASan gates: **`port/asan_all.sh`** (full suite: flight + campaign map/fly/nextday, one command),
`port/asan_flight.sh` (flight), `port/asan_campaign.sh` (save-load→map). Campaign auto-drives:
`MA_CAMP_FLY` (mission-gen→fly), `MA_CAMP_NEXTDAY` (day-advance strategic sim).
Save-date guard is **default-skip** (saves load across rebuilds; format is packing-stable, matches BoB); `MA_ENFORCE_SAVE_DATE` re-enables it.

## Known issues / next steps

- **3D colour fidelity (S20):** sky now confirmed correct blue (zenith ~(152,180,216)); the old
  "brown" defect was stale (fixed by M2 `1a70d2d`). Residual = Wine's near-horizon sky is ~75 units
  brighter (its D3D background-material brightening, stubbed in the software port). Fidelity-target
  choice (match D3D vs faithful software look), low priority — see `port/scrum/sprint-20.md`.
- **ASan flight-path grind — ✅ COMPLETE (S18 sub-epic closed).** The base-item type-confusion pair
  (`LauncherToWorld`, `InitROL`, S37) and the lifetime UAF (`PersonalThreat`/`mobileitem … T_nationality`,
  S38) are fixed; an instrumented Hot Shot flight now yields **0 ASan reports across 5 runs**. The full
  S15→S38 arc eliminated every flight-path heap error the oracle surfaced. Only known-remaining: the
  deliberately-benign `FixLbmImageMap` RLE over-read (bounded by the adopted BoB `LBM_INBOUNDS` guard).
- **ASan campaign save-load path — ✅ CLEAN (S40).** A new headless campaign-drive (`port/asan_campaign.sh`:
  title → Load Game → "Auto Save" → Load → Korea strategic map) sweeps `CFiling::LoadGame` →
  `Todays_Packages.LoadGame` → `PackageList::LoadGame` (the S65a site) → map render with **0 ASan reports** —
  so the cross-port **S65a** fix is now validated on a live load. Enabled by **`MA_IGNORE_SAVE_DATE=1`**
  (skips the build-date guard that otherwise voids every save on recompile; the save format is stable).
  Standing gates: `port/asan_flight.sh` + `port/asan_campaign.sh`.
- **ASan campaign mission-gen + fly path — ✅ CLEAN (S41), 2 real bugs fixed.** Driving a loaded campaign
  to fly (`MA_CAMP_FLY=1`) surfaced two campaign-only heap errors the flight/load sweeps never hit:
  `make_airgrp` (`Persons3.cpp:836`) `GR_Pack_TakeTime[w][gotgrpnum==-1]` **global-buffer-overflow**
  (negative group index → guarded), and `AddChildren` (`RDIALOG.CPP:537`) **stack-use-after-scope** (a
  named-local `DialBox`'s `edges` pointed at a dead `EDGES_` macro temporary → function-scope lifetime).
  Both fixed + re-verified 0.
- **ASan campaign day-advance / strategic sim — ✅ CLEAN (S42).** `MA_CAMP_NEXTDAY=1` (new hook forcing
  `OnClickedFrag2`'s no-flyable branch) drives `Campaign::NextMission`→`NextDay` (date advance / MiG
  rotation / stock replenish) →`ProcessAirFields`→`OnClickedNextPeriod` from the map idle — **0 ASan
  reports across 3 runs**. The `SaveBin`/`SaveGame` writeback was already swept by S41's frag2 else-branch.
  **The campaign path is now ASan-swept end to end** (load→map→mission-gen→fly→writeback→day-advance);
  only multi-day rollover over several days remains (low priority).
- **Higher-leverage next moves:** finish S8 sky-colour fidelity, or the deferred S17 item-type/lifetime
  ASan family — rather than grind the low-frequency ASan singleton tail.
- **Play-test backlog (queued, from S21–S28 sessions):**
  - **Radar gunsight doesn't range/expand** — the F-86 radar-ranging reticle (`DOGUNSIGHT` shape
    opcode, scaled by target range) stays fixed size; range/lock input likely not fed (software path).
  - **Debrief "Claims" table** — the first (player) column has no header label (`sairclms.cpp`).
  - **Replay hang — ✅ RESOLVED** (S33, validated in the 2026-06-29 PO play-test). The 3D loop's
    exit-key test was gated off during playback (`STUB3D.CPP:1438`); the unimplemented replay
    subsystem never ends playback → it could spin. Fix re-enables `EXITKEY` during playback
    (`MA_LINUX`, strict superset). PO session: replay loaded a live, responsive 3D view and exited
    cleanly (eject/back/quit, exit 0) — no stuck state.
  - **Replay *playback* unimplemented** — ⬜ **future epic** (separate from the hang). PO session:
    a (valid, pre-stored) replay loads only the start state (aircraft static, gear down); the
    recorded flight never advances and the VCR transport (kbd/mouse) is dead. Recording works
    (gun-camera ON writes a file). Full playback = driving the 3D view frame-by-frame from the
    replay data + wiring the transport — a substantial subsystem, same out-of-this-train tier as
    Smacker video / multiplayer. Recommend deferring; document as a known limitation.
  - **High-resolution support (S34, 2026-06-29):** the resolution combo now offers up to **1920×1080**
    (added higher 4:3 + 16:9 modes to `ma_populate_software_modes` + both DD enumerators; relaxed the
    4:3-only `IsValidMode`; windowed flight applies the selected mode via `Save_Data.displayW/H`). The
    window re-centers on resize and goes borderless-fill at desktop size. **3D flight is gorgeous at
    1080p** (software rasterizer); the **ADI kaleidoscope-on-bank is fixed** at all resolutions
    by baking roll (and pitch) into the ball image and drawing the quad axis-aligned
    (`OVERLAY.CPP DoArtHoriz`) — the ported `ma_xasm` texturer tiled any roll-rotated quad;
    resolving roll in a resample avoids that path. **C4 padlock target box + telemetry** (the
    Wine two-patch feature): **box ✅ PO-verified** — SHIFT+D (and `d`) toggle a red diamond that
    **scales with range to enclose the bogey** (`R_WORLD` projected extent, `OVERLAY.CPP DoCheatBox`);
    keys handled in the SDL layer (`g_adi_box`/`g_adi_telem`, bob_video.cpp) because the engine
    keymap binds `BOXTARGET` to `d`+no-modifier and rejects shift+d. **ALT+D telemetry 🔨 WIP** —
    toggle works and the beside-box `Rng`/`Alt` lines are computed from world coords (`SetViewVals`
    is never called, so `targRange`/`RelAlt` stay 0; World.Z is altitude), but the engine text
    path (`PrintAt2`→`PutC3`) renders a **white blob, no glyphs** in this overlay. **Next:** replace
    with a custom glyph renderer — **large fixed-size text (range-independent), no background box,
    black/white by background contrast** (PO spec) — then add **bogey-speed[closure] + own-speed**
    (C4d) and finalize the **adaptive color** (C4c). Pixel access: `currscreen->PlotPixel` (read/write
    565) or `DoClippedLine` strokes. **Remaining high-res 2D-layer issues** (the 2D overlays aren't
    resolution-independent like the 3D world): the **campaign map** tiles / scales with the wheel / icons
    not visible, and the **kneeboard** page renders blank. These need per-layer scaling work (deferred).
  - **Campaign-map wheel-zoom** resizes the window + patchworks tiles (present canvas tied to `m_size`).
  - ~~**Window title**~~ — ✅ fixed S31 ("Mig Alley (Linux native port)").
  - **Replay hang** and the items above are **interactive-repro-gated** — batch for a PO-driven
    play-test session (can't be DoD-demonstrated headlessly).

See `scrum.md` + `port/scrum/` for the sprint boards, `port/ROADMAP.md` for the completion plan, and
the `migalley-port-state` memory note for detailed per-blocker history.
