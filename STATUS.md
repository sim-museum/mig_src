# Mig Alley — native Linux (SDL2) port: STATUS

_Last updated: 2026-06-17 (Scrum Sprint 1 closed)_

Native **32-bit i386 ELF** port of the 1999 Rowan engine (OpenWatcom / Win32 / DirectX / MFC)
to Linux + SDL2/OpenGL. Branch `linux-port`. Game data: the Wine install at
`/home/m/sgl/TUE/MigAlley/WP/drive_c/rowan/mig`.

## Project management — Scrum (see `scrum.md`, `port/scrum/`)

Run as Scrum. Epic: *complete the port of Mig Alley to Linux*. PO-accepted first-release gate =
**R2 (Flyable 3D)**.

**Sprint 2 "Front-end finished" — CLOSED (2026-06-17); R1 functionally complete.** Machinery
restarted after reboot (PO grants standing pre-approval for every sprint). The reboot cleared the
S1 SDL display wedge. Restart-resume closed the S1 carry: **A1 re-validated 20/20**, **A2.4 round-trip
PASS** (clean-exit rewrites `settings.mig`). Then:
- **F2 — combo dropdown:** settings combos open a real list panel (combo's own font, current item
  highlighted), row-click selects + fires the change, click-away closes; ≤1-item combos keep the cycle
  fallback. `ma_olecombo.cpp` (`ma_combo_dropdown_draw` + select/itemcount/curindex) +
  `ma_olecontrol.cpp` (open-state, drawn on top after the control loop, hit-tested in `ma_ole_click`).
- **F3 — RESOLUTIONS combo populated:** lists 640×480 / 800×600 / 1024×768 (4:3 only). Root cause was
  an inconsistent driver state (fSoftware=0/dddriver=0 with software-only modes) failing SDETAIL's
  filter, not missing modes. `Win3d.cpp ma_populate_software_modes()` pins the software state +
  registers mode widths; `SDETAIL.CPP::OnInitDialog` calls it before the fill.
- **C3 — mouse coverage (partial):** audit — every interactive control on the rendering panels
  (listbox/button/tab-bar/combo+dropdown) is hit-tested; only the listbox scrollbar (RScrlBar,
  `CT_OTHER`) isn't, and short lists don't need it. "All panels" is coupled to F4 → re-sliced to S3.
- **Carried to Sprint 3:** C1 (DirectInput→SDL, R2 headline) + F4 (Campaign/QuickMission/Comms panels)
  + the C3 remainder. Board: `port/scrum/sprint-02.md`. **Build note:** `rebuild.sh` won't recompile
  an MFC fragment when `/tmp/*_ok.txt` is absent (post-reboot) — recompile the fragment `.o` manually.

**Sprint 1 "Dependable launch" — CLOSED / ACCEPTED (2026-06-17):**
- **A1 — intermittent 3D-launch crash: FIXED, validated 20/20.** Real bug: `View3d` ctor
  (`STUB3D.CPP:730`) published the view into the sim thread's `viewedwin` (under the mutex) *before*
  initialising `drawing`/`View_Point`; the `DoMoveCycle` guard then tested garbage and could deref a
  wild `View_Point`. Fix inits those fields before publishing.
- **A4 — `port/stress_launch.sh`** (launch→poll `MA_DUMP_BACK` marker→classify by signal).
- **A2 — preference persistence: code-complete.** `ma_save_preferences()` (FULLPANE.CPP) wired into
  the SDL window-close / Ctrl+ESC / `BOB_EXIT_AFTER_DUMP` exits (bob_video.cpp); boot-load already
  worked (`SAVEGAME.CPP:1591`). **Carry to Sprint 2:** live round-trip re-demo (was blocked by a
  session-level SDL window-map wedge — clears on reboot; resume steps in `port/scrum/sprint-01.md`).
- **Build fix:** `rebuild.sh` resolves bare `mfc2_ok.txt` names under `SRC/MFC/` (from-scratch
  rebuild was skipping 22 R-control TUs → link fail). Link now = **266 TUs**.

**Next PO touchpoint:** Sprint 2 Planning (after reboot + A2.4 re-demo). Planned: F2/F3/C3/F4.

## Current milestone — ★ FIRST NATIVE 3D FRAME

The 3D cockpit **flight view renders natively** via the software rasterizer (no Wine, no
hardware Direct3D). Run from the data dir:

```
BOB_RUN_INIT=1 MA_ENABLE_3D=1 BOB_CLICKSEQ="50,588,232" \
  BOB_DRIVE_C=/home/m/sgl/TUE/MigAlley/WP/drive_c ./wmig
```

**Validated:** ~11.7M span fills/frame, ~95% of the 640×480×16 back surface non-zero, ~65 fps,
**crash-free across 8+ runs**. A captured frame (`MA_DUMP_BACK=N` → `/tmp/maback.ppm`) shows the
expected structure — bright sky on top, hazy horizon, green terrain below, cyan cockpit/HUD.

## Phase progress

| Phase | State |
|-------|-------|
| 1 — compile | ✅ all 15/15 game module unities compile clean |
| 2 — first link | ✅ `wmig` links, 0 undefined symbols (7.8 MB i386 ELF) |
| 3 — SDL2 runtime | ✅ boots into `CMIGApp::Run()`; SDL2 window + DirectDraw→GL present bridge |
| 4 — 2D front-end | ✅ title screen + interactive Preferences UI (OCX hosting, RLE8 BMPs, TTF fonts, tabs, write-back) |
| 5 — 3D flight | ✅ **first frame** — software rasterizer renders the cockpit view; ⏳ fidelity/input next |
| 6 — input | ⬜ DirectInput → SDL for flight controls |
| 7 — audio | ⬜ Miles/WAIL → OpenAL/SDL_mixer |
| 8 — campaign/mission | ⬜ flow + binary-compatible file IO |
| 9 — video | ⬜ Smacker → libsmacker |
| 10 — multiplayer | ⬜ DirectPlay → sockets |

## Phase 5 — blocker chain fixed this session (all gated `MA_LINUX`)

Starting from "3D window opens but spins / crashes / renders black":

1. **Mode-init spin** — compat `GetDisplayMode` was a no-op → mode selection collapsed onto a
   0×0×0 mode → infinite RGB-mask-derivation loop. Filled the desc (window dims @ 16-bit 565);
   added a windowed 16-bit mode preference in `DDRWINIT.CPP` (was gated on `isFullScreen()`).
2. **SIGFPE** — zeroed compat `GetWindowRect` → div-by-zero in `XX_SetGraphicsMode`'s
   `virtualXscale`. Guarded `window_width/height` to the selected mode dims.
3. **Black (0×0 back surface)** — `SetDirectDrawMode` sized the offscreen surface from the same
   zeroed rect → bits never allocated. Same guard.
4. **Render target unwired** — single-screen (forced-windowed) mode never connected
   `logicalscreenptr`/`pScreenB` to `DD.lpDDSBack` (the engine shipped fullscreen,
   `NumberOfScreens>=2`). `XX_SetGraphicsMode` now Locks the back surface for its bits.
5. **`fills=0` (root of "black")** — `Save_Data.fSoftware` was false → every polygon routed to
   the stubbed hardware D3D path (`polygon::hardpoly`→`DoHardPoly`), so `ASM_Call` fired 0×.
   `STUB3D` `MakePassive` forces software mode (the port implements only the `ma_xasm` fillers).
6. **`body2screen` wild-pointer crash** — it unconditionally dereferenced `mat_win` for the
   hardware check (valid only after `ThreeDee::Init3D`; the draw thread reached it a frame
   early). Software-only port: never take the hardware branch.
7. **Sim-thread startup race** — the timer fired `DoMoveCycle` while `MakePassive` was still
   constructing a view → wild deref in `ProcessSpot`→`FrameTime`→`LastFrameTime`. `DoMoveCycle`
   now skips views where `!View3d::Drawing()` (set last in `MakePassive`).
8. **Present arbitration** — `MIG.CPP` idle loop gates the 2D front-end present on `!in3d`
   (once `Inst3d` exists) so the menu canvas stops overwriting the slower 3D frames.

## Diagnostics (gated, default off)

`MA_ENABLE_3D` (drives `Launch3d`), `MA_TRACE_3D`, `MA_TRACE_DD` (Blt src size/bpp/nonzero count),
`MA_TRACE_FILL` (span-filler invocation counter), `MA_DUMP_BACK=N` (writes the N-th back→primary
Blt as PPM via raw POSIX `open`/`write` — compat `#define fopen fopen_nocase` can't create `/tmp`).

## Build & run

```
bash port/rebuild.sh        # ~266 TUs in parallel → /tmp/wmig
```
Link: `g++ -m32 -no-pie port/build/{obj,obj2,objmfc,objmfc2,objole}/*.o -Wl,--allow-multiple-definition -lSDL2 -lGL -lpthread -lm -o wmig`.
**Rebuild `_HARD`+`SMKDLG`+`STUB3D` on surface-layout changes; `ddraw_legacy.h` is inlined into
many TUs → full rebuild when editing it (`--allow-multiple-definition` picks one copy).**

## Known issues / next steps

- **Flight-launch crash race: FIXED in Sprint 1** (A1, `STUB3D.CPP:730` — see Scrum section).
  Stress-validated 20/20 via `port/stress_launch.sh`. (`BOB_CLICKSEQ` click timing can still cause
  a no-op/partial launch in the scripted test — that's menu-readiness timing, not a crash.)
- **`BOB_DUMP_FRAME`** reads the gdi-canvas (black during 3D) — use `MA_DUMP_BACK` for 3D frames.
- **Next:** 3D fidelity (A/B vs Wine), input (DirectInput→SDL), then audio/campaign/video.

See `port/ROADMAP.md` for the full completion plan and the `migalley-port-state` memory note for
the detailed per-blocker history.
