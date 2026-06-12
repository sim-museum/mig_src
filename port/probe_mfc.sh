#!/usr/bin/env bash
# Probe-compile each MFC fragment individually (approach B). Records OK / FAIL+firsterr.
cd "$(dirname "$0")/.."
mkdir -p port/build/objmfc
ok=0; fail=0
: > /tmp/mfc_ok.txt; : > /tmp/mfc_fail.txt
while read -r bn; do
  f=$(find SRC/MFC -maxdepth 1 -iname "$bn" ! -type l 2>/dev/null | head -1)
  [ -z "$f" ] && f="SRC/MFC/$bn"
  [ -f "$f" ] || continue
  o="port/build/objmfc/$(basename "$f" | sed 's/\.[Cc][Pp][Pp]$//').o"
  e=$(./port/ccx_mfc.sh "$f" "$o" 2>&1 | grep -m1 -aE 'error:')
  if [ -z "$e" ]; then ok=$((ok+1)); echo "$f" >> /tmp/mfc_ok.txt
  else fail=$((fail+1)); printf '%s :: %s\n' "$f" "$(echo "$e"|sed -E 's#.*error: ##'|cut -c1-46)" >> /tmp/mfc_fail.txt; fi
done < /tmp/mfc_frags.txt
echo "MFC FRAGMENT PROBE (approach B): $ok ok, $fail fail"
