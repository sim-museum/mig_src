#!/usr/bin/env bash
# Mig Alley — native Linux port: end-user install script.  (scrum.md EPIC H, H1-pkg)
#
# Installs the locally-built 32-bit `wmig` engine against a user-supplied Mig Alley
# data installation. The game data is NOT redistributable, so the user brings their
# own — normally an existing Windows/Wine install, i.e. a Wine-style `drive_c` tree
# containing `rowan/mig`.
#
# Usage:
#   packaging/install.sh --data /path/to/drive_c [--prefix ~/.local] [--bin /path/to/wmig]
#   packaging/install.sh --data /path/to/drive_c/rowan/mig        # also accepted
#
# What it does:
#   1. Locates the built engine and verifies it is a 32-bit i386 ELF.
#   2. Verifies the data dir looks like a Mig Alley install and finds its drive_c root.
#   3. Checks the i386 runtime shared-library dependencies, naming the :i386 packages.
#   4. Installs the engine + a `migalley` launcher + a .desktop entry into the prefix.
#
# The launcher `cd`s into <drive_c>/rowan/mig before exec'ing the engine, because the
# engine derives its data root from the cwd's last `/drive_c` ancestor (SRC/compat/
# bob_main.cpp) and the Rowan FileMan resolves "." paths against the install dir. It
# also exports BOB_DRIVE_C explicitly, so an install under a differently-named parent
# still works.
set -uo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DATA=""
PREFIX="$HOME/.local"
BIN=""

while [ $# -gt 0 ]; do
  case "$1" in
    --data)   DATA="${2:-}"; shift 2 ;;
    --prefix) PREFIX="${2:-}"; shift 2 ;;
    --bin)    BIN="${2:-}"; shift 2 ;;
    -h|--help) grep '^#' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
    *) echo "unknown arg: $1" >&2; exit 2 ;;
  esac
done

err()  { echo "ERROR: $*" >&2; exit 1; }
warn() { echo "!! $*" >&2; }

# --- 1. locate the engine --------------------------------------------------
# Two supported builders: CMake+Ninja (build/wmig) and port/rebuild.sh (/tmp/wmig).
if [ -z "$BIN" ]; then
  for cand in "$REPO/build/wmig" /tmp/wmig; do
    [ -x "$cand" ] && { BIN="$cand"; break; }
  done
fi
[ -n "$BIN" ] && [ -x "$BIN" ] || err "engine not built. Build it with one of:
     cmake -S $REPO -B $REPO/build -G Ninja && ninja -C $REPO/build     (-> build/wmig)
     bash $REPO/port/rebuild.sh                                          (-> /tmp/wmig)
   or pass --bin /path/to/wmig"

case "$(file -b "$BIN" 2>/dev/null)" in
  *"ELF 32-bit"*"Intel 80386"*|*"ELF 32-bit"*i386*) : ;;
  *) err "'$BIN' is not a 32-bit i386 ELF. The port is i386-only (packed struct layouts,
   4-byte long, the nasm modules). Rebuild with the -m32 toolchain." ;;
esac

# --- 2. data dir -----------------------------------------------------------
[ -n "$DATA" ] || err "pass --data /path/to/drive_c (your Mig Alley data install)"
DATA="$(cd "$DATA" 2>/dev/null && pwd)" || err "data dir not found: $DATA"

# Accept either the drive_c root or the game dir itself; normalise to both.
if [ -d "$DATA/rowan/mig" ]; then
  DRIVE_C="$DATA"; GAMEDIR="$DATA/rowan/mig"
elif [ -f "$DATA/Mig.exe" ] || [ -f "$DATA/MIG.EXE" ] || [ -d "$DATA/Artwork" ]; then
  GAMEDIR="$DATA"; DRIVE_C="$(cd "$DATA/../.." && pwd)"
else
  err "'$DATA' has neither rowan/mig nor Mig.exe/Artwork — not a Mig Alley install?"
fi
# The engine's cwd-derivation looks for a literal '/drive_c' component. Warn (don't
# fail) if the tree isn't named that way — the launcher exports BOB_DRIVE_C anyway.
case "$DRIVE_C" in
  */drive_c) : ;;
  *) warn "data root '$DRIVE_C' is not named 'drive_c'; the launcher will set
     BOB_DRIVE_C explicitly, but a bare './wmig' from the game dir will not
     auto-derive the data path." ;;
esac

echo ">> engine:  $BIN"
echo ">> drive_c: $DRIVE_C"
echo ">> gamedir: $GAMEDIR"
echo ">> prefix:  $PREFIX"

# --- 3. i386 runtime dependency check --------------------------------------
# ldd on a 32-bit binary needs the i386 loader (/lib/ld-linux.so.2, from libc6:i386).
# If that's missing, ldd fails outright — detect it and give the multiarch recipe
# rather than reporting a confusing error.
echo ">> checking i386 runtime libraries..."
APT_I386="libsdl2-2.0-0:i386 libgl1:i386 libopenal1:i386 libfluidsynth3:i386 libstdc++6:i386"
MULTIARCH_HINT="   sudo dpkg --add-architecture i386 && sudo apt update
   sudo apt install $APT_I386"

if [ ! -e /lib/ld-linux.so.2 ] && [ ! -e /lib32/ld-linux.so.2 ]; then
  warn "no i386 dynamic loader (/lib/ld-linux.so.2) — this host cannot run 32-bit binaries yet."
  echo "   Enable multiarch and install the 32-bit runtime:"
  echo "$MULTIARCH_HINT"
else
  LDD_OUT="$(ldd "$BIN" 2>&1)"
  MISSING="$(printf '%s\n' "$LDD_OUT" | awk '/not found/{print $1}')"
  if [ -n "$MISSING" ]; then
    warn "missing 32-bit shared libraries:"
    printf '%s\n' "$MISSING" | sed 's/^/     /'
    echo "   On Debian/Ubuntu:"
    echo "$MULTIARCH_HINT"
    echo "   (installing the 64-bit packages does NOT satisfy an i386 binary — the ':i386'"
    echo "    suffix is required.)"
  else
    echo ">> all 32-bit shared libraries resolve."
  fi
fi

# --- 4. install engine + launcher + desktop entry --------------------------
mkdir -p "$PREFIX/bin" "$PREFIX/lib/migalley" "$PREFIX/share/applications" || err "cannot write to $PREFIX"
install -m 0755 "$BIN" "$PREFIX/lib/migalley/wmig" || err "install of the engine failed"
echo ">> installed engine: $PREFIX/lib/migalley/wmig"

LAUNCHER="$PREFIX/bin/migalley"
cat > "$LAUNCHER" <<EOF
#!/usr/bin/env bash
# Generated by packaging/install.sh — Mig Alley (native Linux port)
# The engine expects cwd == the install dir and derives its data root from the
# cwd's last /drive_c ancestor; BOB_DRIVE_C is exported as the explicit override.
GAMEDIR="\${MA_GAMEDIR:-$GAMEDIR}"
export BOB_DRIVE_C="\${BOB_DRIVE_C:-$DRIVE_C}"
cd "\$GAMEDIR" || { echo "game dir not found: \$GAMEDIR" >&2; exit 1; }
exec "$PREFIX/lib/migalley/wmig" "\$@"
EOF
chmod +x "$LAUNCHER"
echo ">> installed launcher: $LAUNCHER"

cat > "$PREFIX/share/applications/migalley.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=Mig Alley (Linux native port)
Comment=Rowan's Mig Alley — Korean-war jet combat flight simulator
Exec=$LAUNCHER
Terminal=false
Categories=Game;Simulation;
EOF
echo ">> installed desktop entry"

echo
echo "Done. Run with:  $LAUNCHER          (ensure $PREFIX/bin is on your PATH)"
echo "Useful overrides: MA_DISABLE_3D=1 (2D only), MA_NO_AUDIO=1, BOB_NO_RUN=1 (link-only)."
