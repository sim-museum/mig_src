# Mig Alley — native Linux (SDL2) port

A native **32-bit i386 ELF** port of Rowan's *Mig Alley* (1999, the OpenWatcom / Win32 /
DirectX / MFC engine) to Linux + SDL2 / OpenGL / OpenAL — **no Wine, no DirectX**. Branch:
`linux-port`. Sister project: the same-engine [Battle of Britain port](../bob).

## Status

The game **boots to the native title screen, navigates the full single-player front-end,
flies a software-rasterized 3D mission, and returns to the menu in one process** — with
OpenAL sound effects, XMIDI music (FluidSynth), keyboard + joystick + in-flight mouse, a
navigable campaign map, and click-driven save/load. See **[STATUS.md](STATUS.md)** for the
live subsystem matrix and **[scrum.md](scrum.md)** / `port/scrum/` for the sprint history.

Out of scope (this release train): DirectPlay multiplayer, Smacker intro video, the mission editor.

## Run

The build is 32-bit i386. You need the **32-bit** runtime libraries:

```
# Debian/Ubuntu (multiarch)
sudo dpkg --add-architecture i386 && sudo apt update
sudo apt install libsdl2-2.0-0:i386 libgl1:i386 libopenal1:i386 libfluidsynth3:i386 libstdc++6:i386
```

Then, from the game's install directory (the `mig` folder under a Wine-style `drive_c`):

```
cd <drive_c>/rowan/mig
./wmig
```

That's it — **no environment variables**. `wmig` derives the data path from the cwd's
`/drive_c` ancestor and auto-runs (S30). Overrides:

| Variable | Effect |
|---|---|
| `BOB_DRIVE_C=<dir>` | point at a `drive_c` elsewhere (skips cwd derivation) |
| `BOB_GAME_DIR=<dir>` | use this exact install dir |
| `BOB_NO_RUN=1` | link-only run (no game), the safe default for a non-install dir |
| `MA_DISABLE_3D=1` | 2D-only (front-end debugging) |
| `MA_NO_AUDIO=1` | silence the audio backend |

Game data is not included — supply your own *Mig Alley* installation.

## Build

Two equivalent builders — both produce the same 270-TU, zero-undefined-symbol binary:

```
cmake -S . -B build -G Ninja && ninja -C build   # incremental  -> build/wmig
bash port/rebuild.sh                             # from scratch -> /tmp/wmig
```

**CMake + Ninja** (`CMakeLists.txt`) is the day-to-day builder: it tracks header
dependencies, so editing one `.cpp` recompiles one TU (~1 s) and editing
`SRC/compat/ddraw_legacy.h` recompiles exactly the 6 TUs that include it (~5 s) instead
of all 270 (~84 s). `ninja -C build install-wmig` copies the result to `/tmp/wmig`, where
the `port/*.sh` harnesses look for it. ASan variant:
`cmake -S . -B build-asan -G Ninja -DMA_ASAN=ON && ninja -C build-asan`.

**`port/rebuild.sh`** remains the canonical from-scratch builder and the fallback if the
CMake build ever misbehaves; it is unchanged and always safe. Its flag set is the
specification the CMake build reproduces (five object groups with different
include/prelude flags, six OCX build modes, `port/lists/*` build sets, two nasm modules).

Link line (also in `port/rebuild.sh`):

```
g++ -m32 -no-pie port/build/{obj,obj2,objmfc,objmfc2,objole}/*.o \
  -Wl,--allow-multiple-definition -lSDL2 -lGL -lopenal -lfluidsynth -lpthread -lm -o wmig
```

A 32-bit GCC toolchain (`gcc-multilib`/`g++-multilib`), `nasm`, and the 32-bit `-dev` packages
of the libraries above are required to build. `ASAN=1 bash port/rebuild.sh` produces the
AddressSanitizer diagnostic build (`/tmp/wmig-asan`); see `port/asan.sh`.

## Install / package

`packaging/install.sh --data <drive_c>` installs the engine, a `migalley` launcher and a
`.desktop` entry against your own data install; `packaging/build-appdir.sh` assembles a
relocatable AppDir with the 32-bit multimedia stack bundled. See
[`packaging/README.md`](packaging/README.md) — including the i386 multiarch caveat.

## Layout

- `SRC/` — the game engine + MFC UI (unedited Rowan source; ports live behind `MA_LINUX`/`BOB_LINUX`).
- `SRC/compat/` — the Win32 / DirectX / MFC → SDL2 shim (95+ files).
- `port/` — build/probe tooling, the rebuild script, and the scrum boards.
- `CLAUDE.md` — the deep porting log; `port/BOB_PORT_LESSONS.md` — shared cross-port field notes.
