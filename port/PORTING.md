# Mig Alley — native Linux (SDL2) port

> **2026 PIVOT — adopting the Battle of Britain port foundation.** BoB (`~/bob`)
> is the *same Rowan engine* and already compiles/links/runs. We imported its
> proven `SRC/compat/` Win32+DirectX+MFC shim, `mathasm_linux.h`,
> `fix_include_case.py`, and its build recipe, and switched to the engine's
> **unity build** (`_XXX.CPP` aggregators) + MSVC code paths (`__MSVC__`).
> The hand-grown `port/` scaffold below is superseded but kept as reference.
>
> **Phase-1 status: 7/15 module unities compile clean** (MATH, GENERAL, TEXT,
> AI, AIRCRAFT, MOVECODE, MODEL). bitcount.h hard blocker solved. Remaining 8
> modules each have per-file blockers (BITABLE, Miles mssw.h, GRAFPASM asm,
> fileman access, DirectInput/DirectPlay compat) — see memory migalley-port-state. Build flags: `-m32 -fno-pie -fpermissive
> -fno-strict-aliasing -fno-delete-null-pointer-checks -fcommon -fpack-struct=1
> -DNDEBUG -DFF_LINUX -DMA_LINUX -Dstricmp=strcasecmp -ISRC/compat -ISRC/H`.
> Reconciliation lives in `SRC/H/DOSDEFS.H` (`__GNUC__` block defines
> `__MSVC__`/`WIN95`/`MA_LINUX`/`FF_LINUX`/`BOB_LINUX`). Reuse rule: copy BoB's
> file only when `diff` shows *port-only* changes (e.g. `VECTOR.H`, `MATHASM.H`,
> `HARDPASM.H`); otherwise apply BoB's `#if BOB_LINUX` technique to Mig Alley's
> own file (`MATH.CPP`, `MYMATH.H`, `MODVEC.H` done). Always use `grep -a`
> (high-byte license banners hide matches from plain grep).
>
> ---


Porting the 1999 Rowan engine (OpenWatcom / Win32 / DirectX) to a native Linux
ELF binary. This document is the source of truth for the effort; read it first
when resuming.

## Architecture decisions (locked)

- **Target: 32-bit i386 ELF** (`gcc -m32`). At 32-bit `long`/pointers are 4
  bytes, which matches every packed struct, binary game-file layout, inline
  `_asm`/`#pragma aux` block, and the 12 TASM modules. A 64-bit target would
  require rewriting all of those — not worth it.
- **Runtime backend: SDL2** (32-bit libs confirmed present). DirectDraw →
  SDL window + streaming texture with an 8/16-bit software framebuffer + palette;
  DirectInput → SDL keyboard/joystick; DirectSound/Miles/WAIL, Smacker video and
  DirectPlay → stubbed first, implemented later.
- **Game data + run target:** the existing working Wine install at
  `/home/admin/sgl/TUE/MigAlley/WP/drive_c/rowan/mig`. The Linux binary will run
  against that data directory (read-only game assets are platform-neutral).
- **Entry point:** `SRC/GENERAL/WINMAIN.CPP` (`WinMain`). Build target in the
  original makefiles is `wmig.exe` (~179 link modules / ~231 game `.CPP`).
  Editor/tool trees (MEDITOR, MFC, R* control libs, CEDITOR) are NOT part of the
  game and are excluded.

## What exists (the scaffold)

| Path | Purpose |
|------|---------|
| `port/include/` | Shim headers, FIRST on the include path (shadow vendored/SDK). `windows.h`, `windowsx.h`, `mmsystem.h`, `ole2.h`, `ma_prelude.h`. |
| `port/include/ma_prelude.h` | Force-included (`gcc -include`) before every TU. System headers (at default packing, before DOSDEFS' `#pragma pack(1)`), calling-convention macros, `GETFPCW/SETFPCW`. |
| `port/inc_ci/` | **Generated** case-folding symlink farm: one case-exact symlink per `#include` spelling → the real `SRC/H` file. Solves mixed-case includes on case-sensitive Linux. |
| `port/mk_inc.sh` | Regenerates `inc_ci/`. Indexes ONLY `SRC/H`; never shadows libc headers. Run after adding/renaming any include. |
| `port/cc.sh` | Compile one TU with the port toolchain/flags. `port/cc.sh SRC/3D/3DCODE.CPP [out.o]`. |
| `port/probe.sh` | Try-compile every game TU; prints `OK / total` and the top first-error categories. The progress dashboard. Log: `port/build/probe.log`. |
| `port/src/` | (empty) — SDL2 runtime + Win32/COM implementations go here. |

### Compile flags (port/cc.sh)
`g++ -m32 -std=gnu++98 -fpermissive -fno-strict-aliasing -fno-exceptions
-fno-operator-names -w -include ma_prelude.h -I port/include -I port/inc_ci`

## Current status

- **Compiling: 94 / 202 game TUs (47%)** via `./port/probe.sh`.
- **Build-set core: 53 / 113** — the TUs that actually map to non-DOS `wmig.exe`
  object modules in `OBJECTS.MIF` (the meaningful denominator). Measure with the
  build-set loop; the list is `port/build/buildset.txt`, current failures in
  `port/build/buildfail.txt`. Aircraft data files and some others build via
  separate makefiles and are outside this 113 core.
- Nothing links or runs yet; the SDL2 runtime layer is not written.

### Probe the build-set core
```
ok=0; while read f; do [ -z "$(./port/cc.sh "$f" 2>&1)" ] && ok=$((ok+1)); done \
  < port/build/buildset.txt; echo "$ok / $(wc -l < port/build/buildset.txt)"
```

## Source edits made so far (all in git, marked with a "Linux/GCC port" comment)

- `SRC/H/DOSDEFS.H` — GCC branches: disable debug-`new` macro, `INT3`,
  `DLLExport/__FAR/__NEAR`, use real libc `FILE`.
- `SRC/H/MYMATH.H` — `FPATan`/`FPACos` → `atan2`/`acos` (were x87 `_asm`).
- `SRC/H/MODVEC.H` — `FSqrt/FSin/FCos/FPATan/FXPowerY` → libm (were `#pragma aux [8087]`).
- `SRC/H/WORLD.H` — friend-of-private-member → friend-of-class (g++ strictness).
- `SRC/H/VIEWSEL.H`, `SRC/H/COLLIDED.H` — drop `Class::` from member-fn-type typedefs.
- `SRC/H/FILEMAN.H`, `SRC/H/KEYTEST.H`, `SRC/MFC/*`, `SRC/RBUTTON/MINFILE.H` —
  implicit-int `const NAME = …` → `const int NAME = …`.

## The porting playbook (how to make progress)

1. `./port/probe.sh` → look at the top error category.
2. Pick a representative failing file: `./port/cc.sh SRC/<dir>/<FILE>.CPP 2>&1 | head`.
3. If the error is in a widely-included `SRC/H` header, fix it there under a
   `#if defined(__GNUC__)` branch (never delete the Watcom/MSVC path) — one
   header fix typically unblocks many TUs.
4. Recurring mechanical patterns → scripted `perl -i` transform across the tree.
5. Re-probe; the OK count is the score. Commit at milestones.

## Known remaining work (roughly ordered)

1. **Finish compiling (~117 TUs).** Long tail: GCC branches for the remaining
   `#pragma aux`/`_asm` blocks, missing forward decls, member-decl syntax,
   `MSS.H` platform detection (`#error … Define __DOS__/_WINDOWS/WINN`),
   `#bollocks` guarded blocks, `<dos.h>`/`<conio.h>` users.
2. **Inline asm inventory:** 116 `#pragma aux`, 261 `_asm` blocks. Most are small
   math/bit primitives replaceable with C/libm/intrinsics (as done for FCos etc.);
   a few are real blitters/fixed-point and need careful GCC inline-asm or C ports.
3. **12 TASM `.ASM` modules** → NASM (`nasm` present) or C: blitters
   (`GRAFJIM/GRAFPASM/GRPASM25`), matrix (`MATRASM`), landscape (`LSTRASM`),
   format conv (`32TO16`), profiling (`PRO/PROLOG`), keyboard ISR (`KEYTASM` —
   replace with SDL), hardware I/O (`HARDPASM` — stub).
4. **SDL2 runtime** (`port/src/`): IDirectDraw/Surface/Palette over SDL texture +
   software framebuffer; DirectInput over SDL; Win32 funcs + window/message loop.
5. **Stub 3rd-party libs:** Miles Sound System, WAIL, Smacker (`smackw32`),
   DirectPlay — link stubs first, real impls (OpenAL / libsmacker / sockets) later.
6. **Link** the 32-bit ELF, **run** against the Wine data dir, debug to first frame.

## Hazards to remember

- **Global `#pragma pack(1)`** from DOSDEFS.H applies to everything after it,
  including the shim's win32 structs. Shim headers self-protect with
  `#pragma pack(push,4)/pop`. Any struct shared across the shim/runtime boundary
  must have matching packing on both sides — audit before trusting run-time data.
- Binary game-file structs assume 32-bit `long` + little-endian — fine on i386.
- The `_X.CPP`/`*U.CPP` files are Watcom "unity" aggregators (they `#include`
  other `.cpp`); the real build compiles the constituents. The probe skips them.

## Build system (2026-07-19): CMake + Ninja alongside `port/rebuild.sh`

`port/rebuild.sh` is the **specification** and the always-working fallback: it is unchanged
and still produces `/tmp/wmig` from scratch (~84 s, 270 TUs, `xargs -P$(nproc)`). Added
alongside it is a root `CMakeLists.txt` that reproduces its semantics exactly and gives
**incremental** builds:

- The `$COMMON` flag set is one `ma_flags` INTERFACE target. Every option is wrapped in
  `$<$<COMPILE_LANGUAGE:C,CXX>:…>` so nothing leaks onto the **nasm** command line.
  `CMAKE_ASM_NASM_OBJECT_FORMAT elf32` is set **before `project()`** (load-bearing).
  `CMAKE_BUILD_TYPE` is deliberately **empty** — rebuild.sh passes no `-O`, and a `Release`
  tree would silently change codegen (`-O3`) relative to every validated result to date.
- The five object groups map to CMake OBJECT libraries with different flags:
  `ma_obj` (unities + compat), `ma_obj2` (standalones), `ma_mfc`/`ma_mfc2`/`ma_debrief`
  (forced prelude `-include stdafx.h -include _mfc.h`), and the OCX modes
  `ma_ole`/`ma_olestatic`/`ma_olebutton`/`ma_olecombo`/`ma_oleedit`, each of which appends
  its own `-ISRC/R<xxx>` **after** the common `-I` set and force-includes `afxctl.h` first —
  that ordering is what keeps the five Rowan OCX projects' identically-named headers apart.
  The preludes use CMake's `SHELL:` prefix so the `-include <file>` pairs are neither split
  nor de-duplicated.
- `port/lists/{sa_ok,mfc_ok,mfc2_ok}.txt` are read at configure time (and are
  `CMAKE_CONFIGURE_DEPENDS`, so editing one re-configures). The documented quirk that
  `mfc2_ok.txt` holds **bare** filenames resolved under `SRC/MFC/` is handled in
  `ma_resolve()`; missing that silently drops the 22 R-control standalones and the link
  fails with `RDialog::ChildDialClosed` undefined.
- Object groups are listed to the executable in rebuild.sh's order, because
  `-Wl,--allow-multiple-definition` makes the *first* definition win.

**Why this is safe where a naive incremental build was not.** rebuild.sh's header warns that
a header edit (notably `SRC/compat/ddraw_legacy.h`, inlined into many TUs) changes weak/inline
bodies everywhere, and `--allow-multiple-definition` could keep a stale copy. Ninja consumes
GCC's `-MD` depfiles, so *every* TU that included the edited header is rebuilt: touching
`ddraw_legacy.h` rebuilds exactly `_HARD`, `SMKDLG`, `STUB3D`, `bob_video`, `bob_stubs`,
`ddraw_stubs` — i.e. it derives the hand-maintained "rebuild `_HARD`+`SMKDLG`+`STUB3D`" rule
automatically. Residual risk: `--allow-multiple-definition` still silences genuine ODR
violations at link time (identical symbol, different definition), and it cannot be dropped -
`-fcommon` tentative definitions in several TUs need it. So the linker will not warn you if two
TUs legitimately disagree; correctness rests on the depfiles being complete, which they are for
everything reached through `#include`. Anything generated outside the compiler's view (there is
nothing today) would need an explicit CMake dependency. When in doubt: `rm -rf build` or run
`port/rebuild.sh`.
