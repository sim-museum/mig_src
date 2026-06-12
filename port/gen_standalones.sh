#!/usr/bin/env bash
# Emit game-module .cpp files that are NOT actively #included by any unity aggregator.
# One per lowercase basename (collapse case-twins). Output: /tmp/standalones_u.txt
cd "$(dirname "$0")/.."
grep -h "#include" SRC/*/_*.CPP 2>/dev/null | grep -vE '^\s*//' \
  | grep -oE '[A-Za-z0-9_]+\.cpp' | tr 'A-Z' 'a-z' | sort -u > /tmp/in_unity.txt

: > /tmp/standalones_raw.txt
for d in 3D AI AIRCRAFT BFIELDS COMMS FILES GENERAL GRAPHICS HARDWARE INPUT MATH MISSMAN MODEL MOVECODE TEXT; do
  for f in SRC/$d/*.CPP SRC/$d/*.cpp; do
    [ -f "$f" ] || continue
    bn=$(basename "$f"); case "$bn" in _*) continue;; esac
    base=$(echo "$bn" | tr 'A-Z' 'a-z')
    grep -qx "$base" /tmp/in_unity.txt && continue
    printf '%s|%s\n' "$base" "$f" >> /tmp/standalones_raw.txt
  done
done
awk -F'|' '!($1 in s){s[$1]=1; print $2}' /tmp/standalones_raw.txt > /tmp/standalones_u.txt
wc -l < /tmp/standalones_u.txt
