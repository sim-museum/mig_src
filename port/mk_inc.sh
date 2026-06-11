#!/usr/bin/env bash
# Build a case-folding include symlink farm so the mixed-case #include
# spellings in the (originally Windows) source resolve on case-sensitive Linux.
#
# For every distinct spelling found in `#include "X"` / `#include <X>` across the
# source tree, locate the real header file (case-insensitively) and create a
# symlink with the exact spelling the source uses.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SRC="$ROOT/SRC"
OUT="$ROOT/port/inc_ci"

rm -rf "$OUT"
mkdir -p "$OUT"

# System header basenames the farm must NEVER shadow (libc/SDL own these).
SYSHDR=" string.h strings.h time.h math.h stdio.h stdlib.h stddef.h stdint.h \
 memory.h signal.h process.h ctype.h errno.h assert.h float.h limits.h setjmp.h \
 io.h fcntl.h malloc.h types.h "

# Index candidate header files once: lowercased-basename -> real path.
# Restrict to SRC/H (the game's authoritative header dir, per DIRS.MIF) so tool/
# editor headers (e.g. MEDITOR/STRINGS.H) never satisfy game includes.
declare -A REAL
while IFS= read -r f; do
    b="$(basename "$f")"
    lb="${b,,}"
    case "$SYSHDR" in *" $lb "*) continue;; esac   # don't shadow system headers
    [[ -n "${REAL[$lb]:-}" ]] || REAL[$lb]="$f"
done < <( find "$SRC/H" -maxdepth 1 -type f )

# Collect every include spelling used in the source.
made=0; miss=0
declare -A MISSING
while IFS= read -r spell; do
    base="$(basename "$spell")"
    lb="${base,,}"
    real="${REAL[$lb]:-}"
    if [[ -n "$real" ]]; then
        ln -sf "$real" "$OUT/$base"
        made=$((made+1))
    else
        if [[ -z "${MISSING[$lb]:-}" ]]; then MISSING[$lb]=1; miss=$((miss+1)); fi
    fi
done < <(grep -rhoE '#[[:space:]]*include[[:space:]]*["<][^">]+[">]' "$SRC" 2>/dev/null \
         | sed -E 's/.*[<"]([^">]+)[">].*/\1/' | sort -u)

echo "symlinks created: $made   unresolved spellings: $miss"
if [[ $miss -gt 0 ]]; then
    printf 'UNRESOLVED (likely system/SDK or shim-provided):\n'
    for k in "${!MISSING[@]}"; do echo "  $k"; done | sort
fi
