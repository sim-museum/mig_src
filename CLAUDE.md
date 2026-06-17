# Mig Alley — native Linux (SDL2) port

## Working mode
You are porting a Windows codebase to Linux. Work autonomously until the task is complete.
- Do NOT pause to ask for confirmation
- When choosing between approaches, pick the most idiomatic Linux/POSIX solution and proceed
- Only stop if you encounter a hard blocker with no reasonable path forward

Porting the 1999 Rowan engine (OpenWatcom / Win32 / DirectX / MFC) to a native
**32-bit i386 ELF** binary (`gcc -m32`). 32-bit keeps `long`/pointers at 4 bytes —
matching every packed struct, binary game-file layout, inline `_asm` block, and the
TASM modules. Runtime backend: **SDL2** (DirectDraw → SDL texture + software
framebuffer; DirectInput → SDL; DirectSound/Miles/Smacker/DirectPlay → stubbed first).

Game data + run target: the working Wine install at
`/home/m/sgl/TUE/MigAlley/WP/drive_c/rowan/mig`. Branch: `linux-port`
(SSH remote `git@github.com:sim-museum/mig_src.git`).

## Approach (adopted from the same-engine Battle of Britain port, `~/bob`)

- Build the engine's **`_XXX.CPP` unity aggregators** (each `#include`s its fragment
  `.cpp`s) for the 15 game modules; the MFC UI module compiles **per-fragment**
  (approach B) with the prelude force-included.
- Win32/DirectX/MFC shim in `SRC/compat/` (95+ files, gated on `FF_LINUX`/`MA_LINUX`,
  COM-vtable pattern). `SRC/H/DOSDEFS.H`'s `__GNUC__` block makes the engine
  impersonate MSVC (`__MSVC__`/`WIN95`/`MA_LINUX`/`FF_LINUX`/`WIN32`).
- Reuse rule: copy BoB's file only when `diff` is port-only; else apply BoB's
  `#if BOB_LINUX` technique to Mig Alley's own file.

## Build / probe tooling (in `port/`)

| Script | Purpose |
|--------|---------|
| `ccx.sh FILE [out.o]` | compile one game TU with the full flag set |
| `ccx_mfc.sh FILE [out.o]` | compile one MFC fragment with the prelude (`-include stdafx.h -include _mfc.h`) force-included |
| `gen_standalones.sh` | list game `.cpp` not actively in any unity |
| `probe_standalones.sh` / `probe_mfc.sh` | bulk compile-probe (writes `/tmp/{sa,mfc}_ok.txt` + `_fail.txt`) |
| `map_undef.sh` | undefined link symbol → defining file |
| `grind.sh` / `grind_mfc.sh` | per-TU auto-fix loop (for-scope hoist / FPU asm / int3) |
| `fnhoist.py` | hoist MSVC for-scope-leaked loop vars to function scope |
| `port_fpu_asm.py` / `port_xasm.py` | x87/XASM inline-asm → libm/intrinsics |

**Flags:** `-m32 -fno-pie -fpermissive -fno-strict-aliasing -fno-delete-null-pointer-checks
-fcommon -fpack-struct=1 -w -DNDEBUG -DFF_LINUX -DMA_LINUX -D_LINUX -ISRC/compat -ISRC/H -ISRC/MFC`.

## Status (2026-06-17)

**Phase 1 — COMPLETE: all 15/15 game module unities compile clean.**
3D, AI, AIRCRAFT, BFIELDS, COMMS, FILES, GENERAL, GRAPHICS, HARDWARE, INPUT, MATH,
MISSMAN, MODEL, MOVECODE, TEXT.

**Phase 2 — FIRST LINK ACHIEVED.** `wmig` links to a 7.8 MB 32-bit i386 ELF with **0
undefined symbols** (down from 220) and runs: the entry (`main` → `bob_init_instance` →
`CMIGApp::InitInstance()`) drives the real MFC boot path and SIGSEGVs at `InitInstance()+1212`
— a null-deref on the first unimplemented runtime subsystem (the **Phase 3 boundary**: SDL2
window / DirectDraw / registry not wired yet). Run it: `BOB_RUN_INIT=1 ./wmig` from the Wine
data dir. **Link line:** `g++ -m32 -no-pie port/build/{obj,obj2,objmfc,objmfc2}/*.o
-Wl,--allow-multiple-definition -lSDL2 -lGL -lpthread -lm -o wmig`.
- 131/132 MFC fragments + all needed standalones compile; only CNTRITEM (OLE) fails.
- The last 16 no-game-source symbols live in `SRC/compat/port_link_stubs.cpp` (XASM_*/
  ASM_PlotPixel asm, Smacker video, PostGameMessage) — real impls come in Phase 3.
- Entry hooks (`g_pBobApp`/`bob_init_instance`/`bob_run`) added to `MIG.CPP` under `MA_LINUX`.

**Phase 3 — SDL2 runtime, IN PROGRESS. The game now boots into its main loop.**
With `BOB_RUN_INIT=1 BOB_DRIVE_C=<wine drive_c> ./wmig` (from the data dir):
`InitInstance()` **returns 1** and the game **enters `CMIGApp::Run()`** (its custom MFC message
pump) and spins there — no crash.
- Fixed: `m_pMainWnd` was NULL (compat `ProcessShellCommand` is a no-op; the MFC doc/view
  framework never creates the SDI frame) → `MIG.CPP` now `new`s a `CMainFrame` under `MA_LINUX`.
- **Required runtime env var: `BOB_DRIVE_C`** = the Wine drive_c dir (e.g.
  `/home/m/sgl/TUE/MigAlley/WP/drive_c`) so the game's `C:\rowan\mig\…` paths resolve
  (`bob_stubs.cpp` `resolve_nocase`). Without it: file opens fail → MessageBox-error loop.
- **SDL2 window now appears on boot.** `bob_video.cpp` has a new legacy-path C bridge —
  `ma_ddraw_ensure_window(w,h)`, `ma_ddraw_setpalette(rgb,n)`, `ma_ddraw_present(bits,w,h,bpp)`
  (8-bit-indexed / 16-bit-565 software fb → GL texture → SwapWindow) — and `DirectDrawCreate`
  (ddraw_stubs.cpp) calls `ma_ddraw_ensure_window(640,480)`. Run confirms `[vid] SDL2 window
  640×480 + GL context: NVIDIA …`. The window shows the GL clear colour (no frames presented
  yet).
- **Present bridge implemented (ddraw_legacy.h):** `IDirectDrawSurface` now has a real software
  framebuffer; `Lock`→bits+pitch, `Blt(back→primary)`/`Flip`/`Unlock`→`ma_ddraw_present`;
  `IDirectDraw2::CreateSurface` allocates from the desc; `QueryInterface(IID_IDirectDraw2)`→a real
  `IDirectDraw2` (`ma_dd_query_dd2`). **But not yet exercised:** with `MA_TRACE_DD=1`, NO
  CreateSurface/Blt/Flip fire during `Run()`. **Root cause diagnosed:** the title screen
  (first screen = a 2D photo+menu, the `RFullPanelDial`/FullScreen frontend over DirectDraw 2D)
  never launches because (a) **`CMIGView` is never created** — our InitInstance fix makes only
  `CMainFrame`, and the compat `ProcessShellCommand` no-op never builds the MFC doc/view, so
  `CMIGView::OnInitialUpdate`/`OnChangeToTitle`→`LaunchFullPane(introsmack)` never runs; and
  (b) compat `CWinThread::OnIdle` is `{return FALSE;}` (the `Run()` loop *does* call it every
  iteration — `bob_msg_wait` returns `WAIT_TIMEOUT` — but it drives nothing).
- **First-frame = MFC doc/view + CMainFrame UI bring-up (core of Phase 3).** Create `CMIGDoc`+
  `CMIGView` in InitInstance, call `OnInitialUpdate`, initialise the `CMainFrame` toolbars/
  `m_wndSystemBox` (LaunchFullPane null-derefs them), trigger `OnChangeToTitle`, and drive the
  FullScreen render each idle. The 2D frontend paints via the MFC dialog/window compat (heavily
  stubbed) → DirectDraw `Blt`/`Flip` → `ma_ddraw_present` (bridge ready). Expect a grind of
  uninitialised-UI null-derefs. Then hook the Rowan D3D `SetPalette` (`HARDWIN.CPP:541`, 256×3
  RGB)→`ma_ddraw_setpalette` + DirectInput→SDL for menu nav. **Rebuild `_HARD`+`SMKDLG`+`STUB3D`
  on any surface-layout change.** *(2D campaign/menu = DirectDraw 2D = this bridge; 3D flight =
  the DX5/6 execute-buffer path + the `g_devRendered` GL path in bob_video — a later phase. Per
  DOC/CampaignGraphicsWorkarounds.pdf.)*

**Phase 4 — 2D FRONT-END FUNCTIONAL. Title screen + a complete, interactive Preferences UI render
natively.** The game boots to the **title screen** (title.bmp + menu) and the **Preferences/settings
front-end is end-to-end usable**: backgrounds, labels, values, tab navigation, click-to-change, and
persistence — all native (no Wine). Run: `BOB_RUN_INIT=1 BOB_DRIVE_C=<wine drive_c> ./wmig`.
- **GDI software canvas** (`SRC/compat/ma_gdi.cpp`, NEW): HDC/CDC over a BGRA surface — fills, blits,
  text (built-in 8x8 font), `SetDIBitsToDevice`. **RLE8 (BI_RLE8) decode** added — the dialog
  background BMPs are RLE8-compressed (only title.bmp is uncompressed); decoding them lit up every
  settings-screen background. `present_screen`→GL texture→SwapWindow.
- **OCX control hosting** (`ma_olecontrol.cpp` router + per-type glue, all NEW): the Rowan OCX
  controls are hosted by CLSID→type. Done: **RListBox** (`0x48814009`), **RStatic**
  (`0xc42bac3d`, `ma_olestatic.cpp`), **RButton** (`0x78918646`, `ma_olebutton.cpp`), **RCombo**
  (`0x737cb0c9`, `ma_olecombo.cpp`). Each reuses the real game `CRxxxCtrl::OnDraw` over the GDI
  canvas; dispatch by hand-written dispid switch (map order). `ma_ole_draw_all` renders every
  visible control each idle; `ma_ole_click` hit-tests buttons (→eventsink) and combos (→cycle).
  `ma_ole_remove_by_parent` (called from `RDialog::DestroyPanel`) drops a destroyed panel's controls
  so the registry stays bounded across transitions.
- **RT_DIALOG + RT_DLGINIT parsing** (`ma_dlgtmpl.cpp`, NEW): parses the dialog template (control
  rects, DLU→px) AND the RT_DLGINIT (240) OCX property streams to recover **static label text**
  ("Display Driver:", "Gamma Correction", …) — the labels live as ANSI strings in the per-control
  property blob (after a license string), not in the template.
- **Eventsink** (`ma_eventsink.cpp`, NEW): RTTI-based (dialog-class, control-id, dispid)→handler
  routing so a button click reaches the dialog's `ON_EVENT` handler. The redefined ON_EVENT macros
  (afxwin.h) register member thunks.
- **Mouse input** (`bob_video.cpp`): SDL clicks → canvas coords → listbox-nav (`OnSelectRlistbox`) /
  button / combo. Test injectors: `BOB_CLICK="x,y"`, `BOB_CLICKSEQ="frame,x,y;…"`.
- **Settings VALIDATED end-to-end:** Preferences renders labeled settings with combo values; the tab
  bar switches screens (3d/Flight/Game/Views/Controls/Sound/Back); clicking a combo cycles its value;
  `PreDestroyPanel`'s `OPTIONS`/`SG2C_WRITEBACK` macro writes changes to `Save_Data` (confirmed by a
  round-trip: cycle Gamma→High, switch tab, return → still "High"). 800-res layout matches Wine.
- **Build set additions** (`port/rebuild.sh`, NEW canonical builder): modes `ole`/`olestatic`/
  `olebutton`/`olecombo` (each `-ISRC/Rxxx -include afxctl.h`) compile the OCX glue + game control
  TUs into `objole/`. ~244 TUs total. Gated debug traces: `MA_TRACE_OLE/DLG/DLGINIT/STATIC/RES/
  SIZE/CLICK/DIB`, `BOB_TRACE_FOPEN/PRESENT`, `BOB_DUMP_FRAME=N BOB_EXIT_AFTER_DUMP=1`.
- **Scrum Sprint 2 (2026-06-17) — DONE; R1 functionally complete:** real combo **DROPDOWN** (F2,
  `ma_olecombo.cpp` `ma_combo_dropdown_draw` + `ma_olecontrol.cpp` open-state/hit-test — click opens a
  list in the combo's font, row-click selects, click-away closes; ≤1-item combos keep cycle);
  **RESOLUTIONS combo populated** (F3 — `Win3d.cpp ma_populate_software_modes()` pins the consistent
  software driver state + registers mode widths, called from `SDETAIL.CPP::OnInitDialog`; lists
  640/800/1024 @16, 4:3 only); `Save_Data`→disk on clean exit (A2, validated round-trip). Mouse
  coverage (C3) on all *rendering* panels confirmed; the rest (scrollbar + Campaign/QuickMission/Comms
  panels) is coupled to **F4** → Sprint 3, alongside **C1** (DirectInput→SDL flight controls, the R2
  gate). See `port/scrum/sprint-02.md` + memory `migalley-port-state`.
- **Scrum Sprint 3 (2026-06-17) — DONE; R2 input gate met:** **C1 keyboard flight controls
  (DirectInput→SDL)** validated end-to-end + demonstrated. The chain was already wired (SDL key →
  `bob_video pump_events`/`kb_push` → DI keyboard device `GetDeviceData` → `STUB3D OnKeyInput`/
  `OnKeyDown` → `commonkeymaps` → `bitflags` → `KeyHeld3d/KeyPress3d`); holding numpad-4 (ROTLEFT)
  pans the live cockpit camera (89.9% frame change; `MA_TRACE_KEY`: 115 actions, keymap loads).
  Gap closed: numpad keys (DIK 0x47–0x53) were missing from `sdl_to_dik` (`bob_video.cpp`). Bonus:
  fixed a real HUD SIGFPE — `COverlay::DrawTopText` ÷ `Save_Data.alt.mediummm==0`; `STUB3D
  MakePassive` now `Save_Data.SetUnits()` if unset (root-cause for all `mediummm` divisors). A1 8/8.
  Gated hooks `MA_TRACE_KEY`, `BOB_AUTOFLY=sweep|throttle|look`. **Sprint 4 (next):** F4
  (Campaign/QuickMission/Comms panels) + C3 remainder; begin B2 (3D fidelity vs Wine). Known S4
  hardening: full-`sweep` (all keys at once) trips a separate SEGV (unrealistic input, not a blocker).
- **Scrum Sprint 4 (2026-06-17) — IN PROGRESS; F4 Quick Mission renders:** the front-end booted to
  `demotitle` (5-item demo menu) because `MIG.CPP:506` (MA_LINUX) hard-launched `&demotitle`; the
  install is the full game, so it now launches `&title`. The full 7-item title renders and **Single
  Player → Quick Mission** navigates — the QM setup panel renders natively (labels + combos, mission
  text, Fly button), no crash. Campaign/Comms reachable next via the same path. **Nav change:** Hot
  Shot/flight is now title→Single Player→Hot Shot (two clicks); `port/stress_launch.sh` `BOB_CLICKSEQ`
  default updated. Gated diag: `MA_TRACE_DEMO`/`MA_FORCE_TITLE`/`MA_TRACE_EXIST`.
- Remaining: F4 Campaign/Comms · 3D fidelity (B2) / audio / video (later phases).

**Phase 5 — ★ FIRST NATIVE 3D FRAME (2026-06-17). The software rasterizer renders the flight view.**
Run: `BOB_RUN_INIT=1 MA_ENABLE_3D=1 BOB_CLICKSEQ="50,588,232" BOB_DRIVE_C=<wine drive_c> ./wmig`.
Validated: ~11.7M span fills/frame, ~95% of the 640×480×16 back surface non-zero, ~65 fps,
crash-free across runs; a captured frame (`MA_DUMP_BACK=N`→`/tmp/maback.ppm`) shows the cockpit
view — bright sky, hazy horizon, green terrain, cyan HUD. The chain of windowed-single-screen 3D
blockers fixed (all `MA_LINUX`): (1) compat `GetDisplayMode` filled (was no-op→0×0×0 mode→mask-loop
spin); (2) windowed 16-bit mode preference in `DDRWINIT.CPP` (was gated on `isFullScreen()`); (3)
back surface sized from the mode dims, not the zeroed `GetWindowRect` (was 0×0→black; also fixed the
`XX_SetGraphicsMode` div-by-zero SIGFPE); (4) `XX_SetGraphicsMode` wires `logicalscreenptr`/`pScreenB`
to `DD.lpDDSBack` bits (single-screen never did — the engine shipped fullscreen); (5) `STUB3D`
`MakePassive` forces `Save_Data.fSoftware=true` (only the SOFTWARE `ma_xasm` fillers exist; hardware
`DoHardPoly` is stubbed → was black, `ASM_Call` fired 0×); (6) `MATRIX.CPP` `body2screen` never takes
the hardware branch (was deref'ing a not-yet-init `mat_win`); (7) `DoMoveCycle` skips views where
`!View3d::Drawing()` (sim thread raced `MakePassive` setup → wild deref); (8) `MIG.CPP` idle loop
gates the 2D front-end present on `!in3d` (Inst3d exists) so the menu canvas stops overwriting 3D.
Diagnostics (gated, default off): `MA_ENABLE_3D` (drives `Launch3d`), `MA_TRACE_3D`, `MA_TRACE_DD`,
`MA_TRACE_FILL`, `MA_DUMP_BACK=N`. **Rebuild `_HARD`+`SMKDLG`+`STUB3D` on surface-layout changes;
`ddraw_legacy.h` is inlined into many TUs → full rebuild when editing it.** Remaining: flight launch
is INTERMITTENT (some runs crash early / window-closes — more 3D-startup races to harden); then 3D
fidelity (A/B vs Wine), input (DirectInput→SDL for flight controls), audio, campaign, video. See the
memory note `migalley-port-state` for the per-blocker detail.

**Phase 5.1 — 3D-LAUNCH RACE FIXED (Sprint 1, 2026-06-17). Validated 20/20 launches.**
Root cause of the intermittent early crash: the `View3d` ctor (`STUB3D.CPP:730`) published `this`
into `inst->viewedwin` (under `inst->mutex`) but initialised `drawing`/`View_Point`/`Whole_Screen`/
`mode` *after* releasing the mutex. `drawing` is not in the ctor init list, so between publish and
init it held garbage; the sim thread's `DoMoveCycle` guard `if (!view->Drawing()) continue;`
(`STUB3D.CPP:1652`) then tested garbage and, when it ≠ `D_NO`, fell through and dereferenced the
(also-uninitialised) `View_Point` → the wild-pointer crash in `ProcessSpot`. **Fix:** a `MA_LINUX`
block initialises those fields *before* the mutex/publish (`STUB3D.CPP:730`). Stress harness
`port/stress_launch.sh` (NEW, A4) — launches into 3D, polls for the `MA_DUMP_BACK` Blt marker,
classifies crash-vs-survive by signal exit code — confirmed **20/20** clean. Preference persistence
(A2): `ma_save_preferences()` (FULLPANE.CPP) now wired into the SDL window-close / Ctrl+ESC /
`BOB_EXIT_AFTER_DUMP` exit paths (bob_video.cpp) so settings persist on any clean exit, not just the
in-game Exit menu (load on boot, `SAVEGAME.CPP:1591`, already worked).
**Build fix:** `port/rebuild.sh` now resolves the BARE filenames in `/tmp/mfc2_ok.txt` under
`SRC/MFC/` — a from-scratch rebuild (after `rm -rf port/build/obj*`) was silently skipping the 22
R-control standalones (RDIALLOG/TITLEBAR/RSPINBUT/GETSHADW/…) and failing the link with
`RDialog::ChildDialClosed` undefined; it had been relying on pre-existing objects.
*Gotcha learned:* a wedged session GL/SDL path makes `SDL_CreateWindow` block forever on the X11
`MapNotify` (gdb: `ensure_window`→SDL2→`XIfEvent`→`poll(-1)`); induced by SIGKILL-spamming GL apps.
Also: never `pkill -f <pat>` where `<pat>` matches the test command's own line — it self-kills the
shell. Use `pkill -x wmig`.

**Earlier Phase-2 detail (for reference):**
- Link recipe confirmed: `g++ -m32 -no-pie *.o -Wl,--allow-multiple-definition -lSDL2 -lGL
  -lpthread -lm` + BoB runtime objs (`bob_main`/`bob_stubs`/`bob_threads`/`bob_video`/
  `bob_resources`/`cstring_impl`) + `matrasm.nasm` (`nasm -f elf32`).
- **Library stubs done:** `miles_ail_stub.cpp` (50 Miles `AIL_*`), `ddraw_stubs.cpp`
  (`DirectDrawCreate`/`DirectDrawEnumerateA`). Linking the 15 unities + runtime dropped
  undefined symbols 102 → 49.
- **MFC UI module: 131/132 fragments compile (only CNTRITEM/OLE fails)** (it is the game's UI layer — menus,
  dialogs, `DPlay` multiplayer — NOT an excludable editor tree; required for the link).
  Compiled per-fragment with the prelude as a PCH-equivalent. FULLPANE (the deepest hub) now
  compiles; only CNTRITEM (OLE `m_lpObject` container internals) still fails.
  - Recent root-cause fixes (one header → many fragments): `RDIALOG.H` got a
    `DialList(DialBox&&,...)` rvalue-ref overload (MSVC bound temporaries to a non-const
    ref; GCC won't) → unblocks every `DialList(DialBox(...),...)` call site;
    `INFOITEM.H` now `#include`s `bfnumber.h` so `info_airgrp` gets its full
    `BFNUMBER_Included`-gated layout (the `WorldInc.h` twin pulled it too early);
    `GLOBDEFS.H` gates the real-MFC `ON_MESSAGE`/`ON_MESSAGE_CLASS` array-entry redefs
    under `!__GNUC__` (compat empties the message map → those `{...}` entries had no array).
  - FULLPANE (the deepest hub) resolved: its `LaunchDial` trees use
    `cond?DialBox(...):constDialBox&` ternaries that copy-construct a `DialBox` in caller
    scope, so `RDIALOG.H`'s `DialBox` copy ctor had to become `const&` AND `public`
    (was non-const + protected). The "expected primary-expression before `(`" was a cascade
    from an undefined `new EmptyChildWindow`, not from `DialList`/`QuickMissionPanel`.
  - Tooling gotcha learned: include-injection scripts MUST use `grep -a` and handle BOTH
    tab- and space-style `#include` anchors, else they insert into the high-byte license
    banner (inert) and loop. `/tmp/cls2hdr.txt` (class→header index) drives auto-include.
  - Only CNTRITEM (OLE `m_lpObject`/`COleClientItem` internals) still fails.
- **Standalone game TUs** (`.cpp` not in unities): OVERLAY.CPP done; others partial.

**Remaining to a first link: 20 undefined symbols** (down from 220), now ENTIRELY
runtime-glue / 3rd-party-lib stubs — every game + MFC + standalone TU that should compile now
does (CNTRITEM is the only MFC fragment still failing — OLE internals). The 20:
- **10 `XASM_*`** + `ASM_PlotPixel` — unassembled GRAPHICS asm (`extern "C" void f(void)`,
  GRAFPRIM.CPP); stub no-op for the first link (software path wires in later).
- **5 `Smack*`** (`OpenSmack`/`CloseSmack`/`DoSmack`/`SmackSoundUseMSS`/`SmackVolumePan`) —
  Smacker video; stub.
- **4 entry-glue**: `bob_init_instance`/`bob_run`/`g_pBobApp`/`PostGameMessage` — the
  `bob_main` → Mig Alley app hooks (`PostGameMessage` is "defined in main_linux.cpp" per
  `compat/winuser.h`). This is the entry-point wiring step (mine `~/bob/SRC/compat/bob_main.cpp`).

First-link punch list: (1) a `port_link_stubs.cpp` for the XASM/ASM/Smacker no-ops; (2) wire
the entry glue (bob_main → `CMigApp::InitInstance` + message loop). Then `wmig` links → start
the SDL2 runtime (DirectDraw→texture/fb/palette, DirectInput→SDL) toward the first frame.

Build-set note: the link now also pulls `port/build/objmfc2/*.o` — 22 SRC/MFC standalones
NOT in the `_AFX/_MFC/_SHEETS` unities (R-control libs RDIALLOG/TITLEBAR/RSPINBUT/MSCTLBR/…)
plus `port/build/obj2/*.o` standalones (OVERLAY, MISSINIT, NODEREV, KEYSTUB, …).

## Key gotchas (learned the hard way)

- **Always `grep -a`** — high-byte ISO-8859 license banners hide matches from plain grep.
- **Case-colliding twins:** the repo tracks BOTH e.g. `VIEWSEL.CPP` and `Viewsel.cpp` as
  separate real files that have DIVERGED. Edits MUST use the EXACT case from the
  compiler's error path (`find -iname` is case-insensitive → edits the wrong twin →
  silently ineffective). Mixed-case `#include "./X.cpp"` may resolve to a different twin
  than expected (e.g. `MAINTBAR.H`: only the `SRC/MFC` twin defines `MAINTBAR_INCLUDED`).
- **Gated includes:** a header's full struct body is often `#ifdef X_INCLUDED`, so the
  X-header must precede it in the prelude (`maintbar→mainfrm`, `bfnumber→infoitem`).
- **MFC prelude rule:** only SELF-CONTAINED headers go in `SRC/H/_MFC.H`; dialog-style
  headers needing `resource.h`/`RowanDialog` in a specific order, or `comms.h`
  (`H2HPlayerInfo`), must be `#include`d directly in the fragments that use them.
- **Recurring fixes:** implicit-int (`const NAME =` → `const int NAME =`); MSVC
  for-scope-leak (hoist loop vars); temp-to-non-const-ref binds (make param `const&` +
  `const_cast` for legacy API, e.g. `MakeTopDialog`); `_asm{int 3}` → `{}`; x87 asm → libm.

See `port/PORTING.md` for the deeper history and the memory note
`migalley-port-state` for the live per-blocker state.
