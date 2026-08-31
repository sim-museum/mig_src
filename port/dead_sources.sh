#!/usr/bin/env bash
# port/dead_sources.sh — list source files that are NOT compiled, and the live file they duplicate.
#
# WHY THIS EXISTS (S380, 2026-08-31). The tree carries ~276 case-duplicate groups: the original
# import's UPPERCASE files (e.g. SRC/3D/VIEWSEL.CPP) sit beside the mixed-case ones the build
# actually uses (SRC/3D/Viewsel.cpp). Nothing references the uppercase copies, but every
# `grep SRC/**/*.CPP` finds them, and they are FROZEN AT THE IMPORT -- so a search lands on
# pre-port code and reports it as current.
#
# That cost a whole sprint on PO-78: I read the reverse-padlock dispatch entry as compiled out
# behind `#ifndef NDEBUG` and concluded a documented fix had never landed. The entry is present and
# un-guarded in the LIVE file, along with the MA_REVPADLOCK_AT harness I also reported missing.
#
# Authority is ninja's dependency database, not a grep: it records every file that actually reached
# a compiler. `ninja -t deps` after a build is the ground truth.
set -u
cd "$(dirname "$0")/.." || exit 1
[ -f build/.ninja_deps ] || { echo "no build/.ninja_deps -- build first (the deps db is the oracle)"; exit 2; }
LIVE=$(mktemp); ninja -C build -t deps 2>/dev/null | awk '/^ /{print $1}' | sed 's|^.*/ma/||' | sort -u > "$LIVE"
dead=0; groups=0
while read -r lower; do
    files=$(find SRC -iname "$lower" -type f | sort)
    [ "$(echo "$files" | wc -l)" -gt 1 ] || continue
    groups=$((groups+1)); livef=""; deadf=""
    for f in $files; do
        if grep -qxF "$f" "$LIVE"; then livef="$livef $f"; else deadf="$deadf $f"; fi
    done
    [ -z "$deadf" ] && continue
    dead=$((dead+$(echo $deadf | wc -w)))
    printf '  DEAD:%s\n        live:%s\n' "$deadf" "${livef:- (none compiled -- whole group unused)}"
done < <(find SRC -name "*.[cC][pP][pP]" -printf "%f\n" | tr 'A-Z' 'a-z' | sort -u)
rm -f "$LIVE"
echo "----------------------------------------"
echo "$groups case-duplicate group(s); $dead file(s) not compiled."
echo "A grep over SRC/ will find those $dead. They are import-era copies -- do not read them as current."
