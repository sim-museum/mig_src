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
# S394: report EVERY uncompiled source, not only the ones with a case-duplicate.
#
# The first cut skipped any file that was not part of a same-name group (`wc -l > 1 || continue`),
# so it reported 50 and could not see a source that is simply not built at all. Comparing it against
# BoB's corrected checker exposed that: there, 389 of 489 files are uncompiled and only ONE has a
# compiled twin -- the rest are whole subsystems the port replaced. A checker that only looks at
# duplicates cannot find those, and "50 dead files" read as the whole problem when it was a subset.
#
# The two populations are different traps and are labelled differently:
#   DEAD+twin  a compiled file of the SAME NAME exists -- a grep lands on frozen import-era code
#              that looks current (SRC/3D/VIEWSEL.CPP shadowing the built Viewsel.cpp: the PO-78
#              sprint that cost a day)
#   DEAD       not built in ANY copy -- a subsystem replaced by the compat layer. No correct file
#              of that name exists to find, so a doc citing it points at nothing live.
dead=0; twins=0
while read -r f; do
    b=$(basename "$f")
    grep -qxF "$f" "$LIVE" && continue
    t=0
    for o in $(find SRC -iname "$b" -type f); do
        [ "$o" = "$f" ] && continue
        grep -qxF "$o" "$LIVE" && t=1
    done
    if [ "$t" -eq 1 ]; then twins=$((twins+1)); mark="DEAD+twin"; else mark="DEAD     "; fi
    printf '  %s  %-44s %8d bytes\n' "$mark" "$f" "$(stat -c%s "$f")"
    dead=$((dead+1))
done < <(find SRC -name "*.[cC][pP][pP]" -type f | sort)
tot=$(find SRC -name "*.[cC][pP][pP]" -type f | wc -l)
rm -f "$LIVE"
echo "----------------------------------------"
echo "$dead of $tot source file(s) are not compiled."
echo "  $twins of them are SHADOWED by a compiled file of the same name -- the worst kind: a grep"
echo "  lands on frozen import-era code that looks current."
echo "All of them match every grep over SRC/. Do not read them as current, and do not cross-port from them."
