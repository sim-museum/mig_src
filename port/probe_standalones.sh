#!/usr/bin/env bash
# Compile-probe each standalone file; record OK / FAIL+firsterror. Output objects to obj2/.
cd "$(dirname "$0")/.."
mkdir -p port/build/obj2
ok=0; fail=0
: > /tmp/sa_ok.txt; : > /tmp/sa_fail.txt
while read -r f; do
  [ -f "$f" ] || continue
  o="port/build/obj2/$(basename "$f" | sed 's/\.[Cc][Pp][Pp]$//').o"
  e=$(./port/ccx.sh "$f" "$o" 2>&1 | grep -m1 -aE 'error:')
  if [ -z "$e" ]; then ok=$((ok+1)); echo "$f" >> /tmp/sa_ok.txt
  else fail=$((fail+1)); printf '%s :: %s\n' "$f" "$(echo "$e"|sed -E 's#.*error: ##'|cut -c1-50)" >> /tmp/sa_fail.txt; fi
done < /tmp/standalones_u.txt
echo "STANDALONE COMPILE PROBE: $ok ok, $fail fail"
