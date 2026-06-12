#!/usr/bin/env bash
# Auto-grind one MFC fragment (approach B): for-scope hoist + FPU asm + int3, using the
# force-included prelude. Usage: grind_mfc.sh SRC/MFC/FILE.cpp [maxiter]
cd "$(dirname "$0")/.."
f="$1"; max="${2:-40}"
# resolve to the real file for editing (follow symlinks; case-exact)
if [ -f "$f" ]; then real=$(readlink -f "$f"); else
  real=$(find SRC/MFC -maxdepth 1 -iname "$(basename "$f")" 2>/dev/null | head -1); real=$(readlink -f "$real" 2>/dev/null)
fi
[ -z "$real" ] && real="$f"
for n in $(seq 1 "$max"); do
  err=$(./port/ccx_mfc.sh "$real" /dev/null 2>&1 | grep -m1 -aE 'error:')
  [ -z "$err" ] && { echo "OK $real"; exit 0; }
  ecpath=$(echo "$err"|grep -aoiE '[A-Za-z_0-9]+\.cpp:[0-9]+'|head -1)
  fnbase=$(echo "$ecpath"|sed -E 's#.*/##;s/:.*//'); line=$(echo "$ecpath"|grep -aoE '[0-9]+$')
  # only auto-edit the fragment itself (not headers)
  [ "$(echo "$fnbase" | tr 'A-Z' 'a-z')" = "$(basename "$real" | tr 'A-Z' 'a-z')" ] || { echo "HDR-STUCK $real: $(echo "$err"|sed -E 's#.*error: ##'|cut -c1-44)"; exit 1; }
  if echo "$err"|grep -aq '_asm'; then
    python3 port/port_fpu_asm.py "$real" >/dev/null 2>&1
    perl -i -pe 's/_asm\s*\{\s*int 3\s*\}/{}/gi' "$real"
    ./port/ccx_mfc.sh "$real" /dev/null 2>&1 | grep -aq "$line:.*_asm" && { echo "ASM-STUCK $real:$line"; exit 1; }
  elif echo "$err"|grep -aq 'was not declared'; then
    var=$(echo "$err"|grep -aoE "‘[A-Za-z_]\w*’ was not"|head -1|grep -aoE "[A-Za-z_]\w*"|head -1)
    [ -z "$var" ] && var=$(echo "$err"|grep -aoE '[A-Za-z_]\w* was not'|head -1|sed 's/ was not//')
    out=$(python3 port/fnhoist.py "$real" "$line" "$var" 2>&1|head -1)
    echo "$out"|grep -aq 'nothing\|no brace' && { echo "STUCK $real:$line ($var)"; exit 1; }
  else echo "STUCK $real:$line: $(echo "$err"|sed -E 's#.*error: ##'|cut -c1-44)"; exit 1; fi
done
echo "MAXITER $real"; exit 1
