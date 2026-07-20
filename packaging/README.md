# Mig Alley — native Linux port: packaging

The port (the engine) is what this repository builds; the **game data is not
redistributable**. So packaging ships the engine and *ingests* a user-supplied
*Mig Alley* installation — normally a Wine-style `drive_c` tree containing
`rowan/mig`, copied off an existing Windows/Wine install.

Backlog item: `scrum.md` EPIC H, **H1-pkg** ("distro package"), the residual of H1.

## Quick install (recommended)

```bash
# 1. Build the engine (either builder works)
cmake -S .. -B ../build -G Ninja && ninja -C ../build     # -> build/wmig   (incremental)
bash ../port/rebuild.sh                                    # -> /tmp/wmig   (from scratch)

# 2. Install against your data dir
packaging/install.sh --data /path/to/drive_c [--prefix ~/.local]
```

`install.sh` finds the engine (`build/wmig`, then `/tmp/wmig`, or `--bin`), asserts it is
a **32-bit i386 ELF**, validates the data tree, checks the i386 runtime libraries, then
installs:

| Path | What |
|---|---|
| `$PREFIX/lib/migalley/wmig` | the engine |
| `$PREFIX/bin/migalley` | launcher — `cd`s to `<drive_c>/rowan/mig`, exports `BOB_DRIVE_C`, execs the engine |
| `$PREFIX/share/applications/migalley.desktop` | menu entry |

`--data` accepts **either** the `drive_c` root **or** the `rowan/mig` game dir; the
script normalises to both.

### Why the launcher `cd`s

The engine has no data-path baked in. `SRC/compat/bob_main.cpp` derives `BOB_DRIVE_C`
from **the cwd's last `/drive_c` ancestor**, then `chdir`s to `$BOB_DRIVE_C/rowan/mig`
because the Rowan `FileMan` resolves `"."`-relative paths (`.\ROOTS.DIR`, …) against the
install dir. The generated launcher therefore `cd`s into the game dir *and* exports
`BOB_DRIVE_C` explicitly — the export is what makes an install work even when the data
root is not literally named `drive_c` (`install.sh` warns in that case).

Overrides honoured by the launcher: `MA_GAMEDIR`, `BOB_DRIVE_C`. Runtime toggles:
`MA_DISABLE_3D=1` (2D front-end only), `MA_NO_AUDIO=1`, `BOB_NO_RUN=1` (link-only run).

## Runtime dependencies — 32-bit (i386) multiarch

**This is the single most common install failure.** The port is deliberately i386 (it
keeps `long`/pointers at 4 bytes to match every packed struct, binary game-file layout,
inline `_asm` block and the nasm modules). Installing the 64-bit packages does **not**
satisfy it — the `:i386` suffix is required.

```bash
# Debian/Ubuntu
sudo dpkg --add-architecture i386 && sudo apt update
sudo apt install libsdl2-2.0-0:i386 libgl1:i386 libopenal1:i386 libfluidsynth3:i386 libstdc++6:i386
```

Plus a working OpenGL driver with its i386 stack (on NVIDIA that is the
`libnvidia-gl-<ver>:i386` package; on Mesa it comes with `libgl1:i386`).

To **build** you additionally need `gcc-multilib`/`g++-multilib`, `nasm`, and the
`-dev:i386` packages of the libraries above.

`install.sh` checks all of this for you: it detects a missing i386 loader
(`/lib/ld-linux.so.2`, i.e. no `libc6:i386`) up front — otherwise `ldd` just fails with
a confusing error — and otherwise reports each `not found` library with the apt recipe.

## Relocatable AppDir / AppImage

```bash
packaging/build-appdir.sh                             # -> ./MigAlley.AppDir (verified)
MA_DRIVE_C=/path/to/drive_c ./MigAlley.AppDir/AppRun  # run it
appimagetool ./MigAlley.AppDir                        # -> single-file .AppImage
```

`build-appdir.sh` walks `ldd` output and bundles the app's multimedia stack (SDL2,
OpenAL, FluidSynth and their private codec deps) into
`usr/lib/i386-linux-gnu`, with an `AppRun` that sets `LD_LIBRARY_PATH` — no `patchelf`
needed. Every candidate is re-checked with `file` so a 64-bit library can never leak
into the i386 bundle.

**Honest scope of the bundle (i386 caveat).** The standard AppImage host/bundle split
leaves the GL driver, X/Wayland and glibc to the host. For a 32-bit app that means the
host must *still* provide `libc6:i386` (and the i386 loader) and `libgl1:i386` + the
vendor GL stack. So the AppDir removes the SDL2/OpenAL/FluidSynth requirement, **not**
the multiarch requirement. `libstdc++`/`libgcc_s` *are* bundled here (unlike the 64-bit
FreeFalcon script this is adapted from) precisely because a 64-bit host commonly has no
`libstdc++6:i386` at all, and a newer bundled copy is forward-compatible.

Game data is never bundled — even the AppImage takes it at runtime via `MA_DRIVE_C` /
`MA_GAMEDIR`, or from the cwd if you launch from inside `<drive_c>/rowan/mig`.

## Status / not yet done

- `appimagetool` and `patchelf` are not installed on the dev box and there is no clean
  test VM, so the AppDir is **structurally correct and locally `ldd`-verified** but has
  not been run on a second machine, and no single-file `.AppImage` has been produced.
- No native `.deb`/`.rpm` yet. The natural next step is a `.deb` that ships
  `/usr/lib/migalley/wmig` + `/usr/bin/migalley` and `Depends:` the `:i386` list above,
  with the data dir configured post-install — `install.sh` already encodes that layout.
