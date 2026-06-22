#!/usr/bin/env bash
# Regenerate the committed build-set lists in port/lists/ from the CURRENT known-good
# object set in port/build/{obj2,objmfc,objmfc2}/. These lists tell rebuild.sh which
# standalone + MFC-fragment TUs compile cleanly; persisting them (instead of relying on
# the /tmp probe outputs, which are cleared on reboot) makes a from-scratch rebuild
# reproducible. Run this after adding/removing a TU from the build.
set -u
cd "$(dirname "$0")/.."
mkdir -p port/lists

# objmfc/ -> full SRC/MFC paths (non-symlink twin); FULLPANE is appended by rebuild.sh.
: > /tmp/_gen_mfc
for o in port/build/objmfc/*.o; do
  bn=$(basename "$o" .o); [ "$bn" = FULLPANE ] && continue
  f=$(find SRC/MFC -maxdepth 1 -iname "$bn.CPP" ! -type l 2>/dev/null | head -1)
  [ -z "$f" ] && f="SRC/MFC/$bn.CPP"
  echo "$f" >> /tmp/_gen_mfc
done
sort -u /tmp/_gen_mfc > port/lists/mfc_ok.txt

# objmfc2/ -> bare names; LISTBX/MSCTLBR are appended by rebuild.sh.
: > /tmp/_gen_mfc2
for o in port/build/objmfc2/*.o; do
  bn=$(basename "$o" .o); case "$bn" in LISTBX|MSCTLBR) continue;; esac
  echo "$bn.CPP" >> /tmp/_gen_mfc2
done
sort -u /tmp/_gen_mfc2 > port/lists/mfc2_ok.txt

# obj2/ -> full source paths; the 9 scattered/prelude ones are hardcoded in rebuild.sh.
HARDCODED="ACMAI AUTOMOVE CDATMOS CDF94A IMAGEMAP KEYSTUB MISSINIT NODEREV DEBRIEF"
: > /tmp/_gen_sa
for o in port/build/obj2/*.o; do
  bn=$(basename "$o" .o)
  skip=0; for h in $HARDCODED; do [ "$bn" = "$h" ] && skip=1; done; [ $skip = 1 ] && continue
  matches=$(find SRC -iname "$bn.CPP" ! -type l 2>/dev/null)
  n=$(echo "$matches" | grep -c .)
  if [ "$n" -eq 1 ]; then echo "$matches" >> /tmp/_gen_sa
  else echo "WARN: ambiguous/missing source for $bn ($n matches)" >&2; fi
done
sort -u /tmp/_gen_sa > port/lists/sa_ok.txt

echo "port/lists/: sa_ok=$(wc -l < port/lists/sa_ok.txt) mfc_ok=$(wc -l < port/lists/mfc_ok.txt) mfc2_ok=$(wc -l < port/lists/mfc2_ok.txt)"
