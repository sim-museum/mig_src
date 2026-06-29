# Mig Alley — native Linux (SDL2) port: STATUS

_Last updated: 2026-06-29 (cross-port sync with BoB S46→S62 ASan arc: rnd()/BITSET engine-wide over-reads fixed; MIDI-music de-stale). Prior: 2026-06-25 Sprints 21–28 live play-test hardening._

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
| Campaign → operational Korea map | ✅ | S7 — `StretchDIBits` impl'd |
| 3D/map colour fidelity | ◐ | S8/S20 — terrain matches Wine; **sky renders correct blue** (S8/S9 "brown" was stale, fixed by M2 `1a70d2d`); residual = ~75-unit brightness gap vs Wine's D3D-material sky (fidelity-target choice, low pri) |
| Save/load (click-driven loadgame) | ✅ | S11–S14 — "Auto Save" → Load → campaign map |
| ASan heap-bug oracle + flight-path grind | ◐ | S15–S16 — 5 per-frame corruptors killed; S17 — 3 more (Reg3dConv/PerspectivePoly/DoCloudLayer) fixed+verified; residual = item-type/lifetime read family (S18) |
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
`MA_TRACE_FPS` (per-second present fps + running average; B3 regression gate).
S21–S28: `MA_DISABLE_MAP`, `MA_QUICKMISS=<idx>` (2=Turkey Shoot, 3=One on One), `MA_TRACE_BOGIE`,
`MA_TRACE_SPAWN`, `MA_FORCE_PADLOCK=<frame>` (headless padlock repro), `MA_NO_HUDINST`, `MA_TRACE_CLIP`.
ASan oracle: see `port/scrum/asan-findings.md`. `MA_ASAN_LISTBOX_SELFTEST=1` drives the otherwise-
unreached `CRListBoxCtrl::DeleteRow` once (regression check for the BoB S58 `new[]/delete[]` fix).

## Known issues / next steps

- **3D colour fidelity (S20):** sky now confirmed correct blue (zenith ~(152,180,216)); the old
  "brown" defect was stale (fixed by M2 `1a70d2d`). Residual = Wine's near-horizon sky is ~75 units
  brighter (its D3D background-material brightening, stubbed in the software port). Fidelity-target
  choice (match D3D vs faithful software look), low priority — see `port/scrum/sprint-20.md`.
- **ASan tail (S17 backlog):** low-frequency singletons only (`LauncherToWorld`, `DoCloudLayer`,
  `Reg3dConv`=BoB R1.3b, …). Per-frame corruptors already gone; the engine-wide `rnd()`/`BITSET`
  over-reads and the `FixLbmImageMap` LBM over-read (=BoB S47) are now fixed (2026-06-29 cross-port).
- **Higher-leverage next moves:** finish S8 sky-colour fidelity, or the deferred S17 item-type/lifetime
  ASan family — rather than grind the low-frequency ASan singleton tail.
- **Play-test backlog (queued, from S21–S28 sessions):**
  - **Radar gunsight doesn't range/expand** — the F-86 radar-ranging reticle (`DOGUNSIGHT` shape
    opcode, scaled by target range) stays fixed size; range/lock input likely not fed (software path).
  - **Debrief "Claims" table** — the first (player) column has no header label (`sairclms.cpp`).
  - **Replay hang** — the debrief Replay launches the (effectively unimplemented) replay-playback
    subsystem and blocks; needs graceful-degrade like the Quit-hang fix.
  - **Campaign-map wheel-zoom** resizes the window + patchworks tiles (present canvas tied to `m_size`).
  - ~~**Window title**~~ — ✅ fixed S31 ("Mig Alley (Linux native port)").
  - **Replay hang** and the items above are **interactive-repro-gated** — batch for a PO-driven
    play-test session (can't be DoD-demonstrated headlessly).

See `scrum.md` + `port/scrum/` for the sprint boards, `port/ROADMAP.md` for the completion plan, and
the `migalley-port-state` memory note for detailed per-blocker history.
