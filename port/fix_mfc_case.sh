#!/usr/bin/env bash
# The MFC unity aggregators #include fragment .cpp with mixed-case spellings that
# don't match the on-disk filenames (case-sensitive Linux). For each "No such file"
# include, find the real file case-insensitively and create a case-exact symlink.
cd "$(dirname "$0")/.."
for u in SRC/MFC/_AFX.CPP SRC/MFC/_MFC.CPP SRC/MFC/_SHEETS.CPP; do
  for i in $(seq 1 300); do
    miss=$(./port/ccx.sh "$u" /dev/null 2>&1 | grep -m1 -aoE "[A-Za-z0-9_]+\.cpp: No such file" | sed 's/: No such file//')
    [ -z "$miss" ] && break
    # find the real file (case-insensitive) in SRC/MFC, excluding an existing exact match
    real=$(find SRC/MFC -maxdepth 1 -iname "$miss" ! -name "$miss" 2>/dev/null | head -1)
    if [ -z "$real" ]; then echo "$u: NO REAL FILE for $miss"; break; fi
    ln -sf "$(basename "$real")" "SRC/MFC/$miss" && echo "linked $miss -> $(basename "$real")"
  done
done
echo "done"
