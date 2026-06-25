# Mig Alley — native Linux (SDL2) port: STATUS

_Last updated: 2026-06-25 (Scrum Sprint 16 closed; cross-port sync with `~/bob`)_

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
Current work: ASan heap-bug grind on the flight path.

```
BOB_RUN_INIT=1 BOB_DRIVE_C=/home/m/sgl/TUE/MigAlley/WP/drive_c ./wmig
```
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
| 3D/map colour fidelity | ◐ | S8 — terrain matches Wine; **sky too dark** (root-caused, fix pending) |
| Save/load (click-driven loadgame) | ✅ | S11–S14 — "Auto Save" → Load → campaign map |
| ASan heap-bug oracle + flight-path grind | ◐ | S15–S16 — 5 per-frame corruptors killed; S17 — 3 more (Reg3dConv/PerspectivePoly/DoCloudLayer) fixed+verified; residual = item-type/lifetime read family (S18) |
| In-flight mouse (DInput rel→`AU_UI_X/Y`) | ⬜ | **gap** — mouse device types exist, no SDL relative-motion feed |
| MIDI/XMIDI music | ⬜ | S6 increment 2 (env-blocked: no 32-bit fluidsynth) |
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
| 6 — input | ✅ keyboard (S3) + joystick (S10); ⬜ in-flight mouse |
| 7 — audio | ✅ digital path on OpenAL (S6); ⬜ MIDI music |
| 8 — campaign/mission | ✅ reaches + renders operational map (S7); ✅ save/load (S14) |
| 9 — video | ⬜ Smacker → libsmacker |
| 10 — multiplayer | ⬜ DirectPlay → sockets (out of scope) |

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
it). If a save-path corruption ever surfaces, it's this known family. EnumObjects DIDFT filter +
in-flight mouse `AU_UI` wiring are the next liftable items from BoB.

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
`MA_TRACE_JOY`, `MA_NO_AUDIO`/`BOB_AUTOFLY`. ASan oracle: see `port/scrum/asan-findings.md`.

## Known issues / next steps

- **3D colour fidelity (S8):** sky reads too dark (`~[52,52,40]` vs Wine `~[227,232,235]`); horizon
  colour is computed correctly (142,166,200) — the gap is downstream of `DoSetHorizonColour`. Focused
  follow-up.
- **ASan tail (S17 backlog):** low-frequency singletons only (`LauncherToWorld`, `DoCloudLayer`,
  `Reg3dConv`=BoB R1.3b, `FixLbmImageMap`, …). Per-frame corruptors already gone — diminishing returns.
- **In-flight mouse:** the one clear subsystem gap vs BoB (BoB has DInput relative-motion→`AU_UI_X/Y`).
- **Higher-leverage next moves:** finish S8 sky fidelity + lift BoB's in-flight mouse, rather than
  grind the ASan singleton tail.

See `scrum.md` + `port/scrum/` for the sprint boards, `port/ROADMAP.md` for the completion plan, and
the `migalley-port-state` memory note for detailed per-blocker history.
