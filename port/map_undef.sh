#!/usr/bin/env bash
# For each remaining undefined symbol, find the standalone .cpp that defines it.
cd "$(dirname "$0")/.."
: > /tmp/grind_files.txt
for sym in $(awk '{print $2}' /tmp/undef2.txt | grep -vE '^Direct|^DPlay$|^DSound|^Smack|^XASM_|^AIL_'); do
  # search all non-MFC game .cpp for a definition: either "sym::" (method) or "TYPE sym" at col 0 (global)
  for f in $(grep -rlE "(\b${sym}::|^[A-Za-z_].*\b${sym}[ ;=\[])" SRC --include='*.CPP' --include='*.cpp' 2>/dev/null | grep -viE 'MFC/|MEDITOR'); do
    bn=$(basename "$f")
    case "$bn" in _*) continue;; esac    # skip unity aggregators
    cnt=$(grep -cE "\b${sym}::" "$f" 2>/dev/null)
    printf '%s\t%s\t%s\n' "$cnt" "$sym" "$f"
  done | sort -rn | head -1 >> /tmp/grind_files.txt
done
echo "=== files to grind (defining undefined symbols) ==="
awk -F'\t' '{print $3}' /tmp/grind_files.txt | sort | uniq -c | sort -rn
echo "--- symbol -> file ---"
awk -F'\t' '{print $2" -> "$3}' /tmp/grind_files.txt | sed 's#SRC/##' | sort -u
