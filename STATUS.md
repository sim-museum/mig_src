# Mig Alley — native Linux (SDL2) port: STATUS

_Last updated: 2026-06-17_

Native **32-bit i386 ELF** port of the 1999 Rowan engine (OpenWatcom / Win32 / DirectX / MFC)
to Linux + SDL2/OpenGL. Branch `linux-port`. Game data: the Wine install at
`/home/m/sgl/TUE/MigAlley/WP/drive_c/rowan/mig`.

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
bash port/rebuild.sh        # ~244 TUs in parallel → /tmp/wmig
```
Link: `g++ -m32 -no-pie port/build/{obj,obj2,objmfc,objmfc2,objole}/*.o -Wl,--allow-multiple-definition -lSDL2 -lGL -lpthread -lm -o wmig`.
**Rebuild `_HARD`+`SMKDLG`+`STUB3D` on surface-layout changes; `ddraw_legacy.h` is inlined into
many TUs → full rebuild when editing it (`--allow-multiple-definition` picks one copy).**

## Known issues / next steps

- **Flight launch is timing-sensitive in the scripted test** (`BOB_CLICKSEQ` click at frame 50
  sometimes lands before the menu is ready → no/partial launch). Not a crash; interactive use is
  unaffected. Optional: harden worker-thread quiescence until `MakePassive` completes.
- **`BOB_DUMP_FRAME`** reads the gdi-canvas (black during 3D) — use `MA_DUMP_BACK` for 3D frames.
- **Next:** 3D fidelity (A/B vs Wine), input (DirectInput→SDL), then audio/campaign/video.

See `port/ROADMAP.md` for the full completion plan and the `migalley-port-state` memory note for
the detailed per-blocker history.
