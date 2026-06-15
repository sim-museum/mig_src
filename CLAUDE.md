# Mig Alley — native Linux (SDL2) port

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

## Status (2026-06-12)

**Phase 1 — COMPLETE: all 15/15 game module unities compile clean.**
3D, AI, AIRCRAFT, BFIELDS, COMMS, FILES, GENERAL, GRAPHICS, HARDWARE, INPUT, MATH,
MISSMAN, MODEL, MOVECODE, TEXT.

**Phase 2 — linking, in progress:**
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
