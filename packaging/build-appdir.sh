#!/usr/bin/env bash
# Mig Alley — native Linux port: assemble a relocatable AppDir (H1-pkg).
#
# Bundles the app's 32-bit multimedia stack (SDL2, OpenAL, FluidSynth + their private
# codec deps) next to the engine, with an AppRun that sets LD_LIBRARY_PATH — no
# patchelf/appimagetool needed for the AppDir itself. A single-file image is then just:
#     appimagetool MigAlley.AppDir
#
# Usage: packaging/build-appdir.sh [outdir]        (default: ./MigAlley.AppDir)
# Env:   WMIG=/path/to/wmig  to override engine discovery.
#
# i386 CAVEAT — read this before assuming the AppDir is self-contained:
# the engine is a 32-bit i386 ELF, so the host MUST still provide the i386 loader and
# base system (libc6:i386) and the i386 GL driver (libgl1:i386 + the vendor GL stack).
# Those are driver/kernel/glibc-coupled and cannot be bundled. The AppDir therefore
# removes the SDL2/OpenAL/FluidSynth requirement, not the multiarch requirement.
set -euo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
APPDIR="${1:-$REPO/MigAlley.AppDir}"

BIN="${WMIG:-}"
if [ -z "$BIN" ]; then
  for cand in "$REPO/build/wmig" /tmp/wmig; do [ -x "$cand" ] && { BIN="$cand"; break; }; done
fi
[ -n "$BIN" ] && [ -x "$BIN" ] || {
  echo "build the engine first:  ninja -C $REPO/build   (or bash $REPO/port/rebuild.sh)" >&2; exit 1; }
case "$(file -b "$BIN")" in
  *"ELF 32-bit"*) : ;;
  *) echo "'$BIN' is not a 32-bit ELF — the port is i386-only." >&2; exit 1 ;;
esac
[ -e /lib/ld-linux.so.2 ] || {
  echo "no i386 loader (/lib/ld-linux.so.2): install libc6:i386 before bundling." >&2; exit 1; }

# Host-provided libraries — MUST NOT be bundled (GL driver / display / base-system ABI).
# Note libstdc++ and libgcc_s ARE bundled here (unlike the FreeFalcon 64-bit script):
# a 64-bit host frequently has no libstdc++6:i386 at all, and a newer bundled copy is
# forward-compatible, so shipping them removes a real install failure mode.
DENY='^(libGL\.|libGLX|libGLdispatch|libEGL|libOpenGL|libglapi|libdrm|libgbm|libc\.|libm\.|libdl|libpthread|librt|ld-linux|libmvec|libwayland|libX|libxcb|libXau|libXdmcp|libxkb|libdbus|libsystemd|libapparmor|libudev|libasound|libpulse)'

rm -rf "$APPDIR"
# Keep the multiarch triplet in the bundle path: the i386 libs must not be confusable
# with a host 64-bit set if this AppDir is ever merged into another tree.
LIBDIR="$APPDIR/usr/lib/i386-linux-gnu"
mkdir -p "$APPDIR/usr/bin" "$LIBDIR"
cp "$BIN" "$APPDIR/usr/bin/wmig"

echo ">> bundling 32-bit libraries (excluding host-provided)..."
n=0
while read -r name _arrow path _addr; do
  [ -f "${path:-}" ] || continue
  if echo "$name" | grep -qE "$DENY"; then continue; fi
  # Paranoia: never let a 64-bit library into an i386 bundle. -L follows symlinks
  # (most /usr/lib/i386-linux-gnu entries are links; without -L `file` reports
  # "symbolic link to ..." and every real library would be skipped).
  case "$(file -bL "$path")" in *"ELF 32-bit"*) : ;; *) echo "   skip (not 32-bit): $name"; continue ;; esac
  cp -L "$path" "$LIBDIR/" && n=$((n+1))
done < <(ldd "$BIN" | sed 's/^[[:space:]]*//')
echo ">> bundled $n libraries into usr/lib/i386-linux-gnu"

# AppRun: relocatable via LD_LIBRARY_PATH. Game data is NOT bundled (not
# redistributable) — the user points MA_DRIVE_C/MA_GAMEDIR at their own install, and
# we cd there so the engine's cwd-based data-root derivation and the Rowan FileMan
# relative paths both work.
cat > "$APPDIR/AppRun" <<'EOF'
#!/usr/bin/env bash
HERE="$(dirname "$(readlink -f "$0")")"
export LD_LIBRARY_PATH="$HERE/usr/lib/i386-linux-gnu:${LD_LIBRARY_PATH:-}"

# Data dir: MA_GAMEDIR (the rowan/mig dir) or MA_DRIVE_C (the drive_c root), else the
# current directory (which works when you are already inside <drive_c>/rowan/mig).
GAMEDIR="${MA_GAMEDIR:-}"
if [ -z "$GAMEDIR" ] && [ -n "${MA_DRIVE_C:-}" ]; then
    GAMEDIR="$MA_DRIVE_C/rowan/mig"
    export BOB_DRIVE_C="${BOB_DRIVE_C:-$MA_DRIVE_C}"
fi
if [ -n "$GAMEDIR" ]; then
    cd "$GAMEDIR" || { echo "game dir not found: $GAMEDIR" >&2; exit 1; }
fi
exec "$HERE/usr/bin/wmig" "$@"
EOF
chmod +x "$APPDIR/AppRun"

cat > "$APPDIR/migalley.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=Mig Alley (Linux native port)
Exec=AppRun
Icon=migalley
Categories=Game;Simulation;
EOF
# appimagetool requires an icon file to exist; replace with real art when available.
: > "$APPDIR/migalley.png"

echo ">> AppDir ready: $APPDIR"
echo ">> verifying the bundle resolves against itself..."
if LD_LIBRARY_PATH="$LIBDIR" ldd "$APPDIR/usr/bin/wmig" | grep -E "not found"; then
  echo "!! unresolved libraries above."
  echo "   If they are GL/X/glibc entries this is EXPECTED on a host without the i386"
  echo "   multiarch runtime — install: libgl1:i386 libc6:i386 (see packaging/README.md)."
  echo "   Anything else should be added to the bundle (check the DENY regex)."
  exit 1
fi
echo ">> all libraries resolve."
echo
echo "Run locally:  MA_DRIVE_C=/path/to/drive_c $APPDIR/AppRun"
echo "Package:      appimagetool $APPDIR   # single-file .AppImage (needs appimagetool)"
