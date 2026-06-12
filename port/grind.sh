#!/usr/bin/env bash
# Grind one standalone TU to compile: auto-fix for-scope leaks, FPU asm, int3.
# Usage: port/grind.sh SRC/DIR/FILE.CPP [maxiter]
cd "$(dirname "$0")/.."
f="$1"; max="${2:-60}"; dir=$(dirname "$f")
o="port/build/obj2/$(basename "$f" | sed 's/\.[Cc][Pp][Pp]$//').o"
for n in $(seq 1 "$max"); do
  err=$(./port/ccx.sh "$f" "$o" 2>&1 | grep -m1 -aE 'error:')
  [ -z "$err" ] && { echo "OK $f"; exit 0; }
  ecpath=$(echo "$err"|grep -aoiE '[A-Za-z_0-9]+\.cpp:[0-9]+'|head -1)
  fnbase=$(echo "$ecpath"|sed -E 's#.*/##;s/:.*//'); line=$(echo "$ecpath"|grep -aoE '[0-9]+$')
  path=$(find "$dir" -maxdepth 1 -name "$fnbase" 2>/dev/null|head -1)
  if echo "$err"|grep -aq '_asm' && [ -n "$path" ]; then
    python3 port/port_fpu_asm.py "$path" >/dev/null 2>&1
    perl -i -pe 's/_asm\s*\{\s*int 3\s*\}/{}/gi' "$path"
    ./port/ccx.sh "$f" "$o" 2>&1 | grep -aq "$fnbase:$line:.*_asm" && { echo "ASM-STUCK $f@$fnbase:$line"; exit 1; }
  elif echo "$err"|grep -aq 'was not declared' && [ -n "$path" ]; then
    var=$(echo "$err"|grep -aoE '[A-Za-z_]\w* was not'|head -1|sed 's/ was not//')
    out=$(python3 /tmp/fnhoist2.py "$path" "$line" "$var" 2>&1|head -1)
    echo "$out"|grep -aq 'nothing\|no brace' && { echo "STUCK $f@$fnbase:$line ($var): $err" | cut -c1-90; exit 1; }
  else echo "STUCK $f@$fnbase:$line: $(echo "$err"|sed -E 's#.*error: ##'|cut -c1-50)"; exit 1; fi
done
echo "MAXITER $f"; exit 1
