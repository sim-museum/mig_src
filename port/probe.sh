#!/usr/bin/env bash
# Try-compile every GAME translation unit and report how many succeed, plus the
# most common first-error messages so shim work can be prioritised.
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
DIRS="GENERAL HARDWARE GRAPHICS INPUT 3D MATH MOVECODE MODEL AIRCRAFT AI MISSMAN FILES TEXT COMMS"
LOG="$ROOT/port/build/probe.log"
ERRAGG="$ROOT/port/build/errors.txt"
: > "$LOG"; : > "$ERRAGG"
ok=0; fail=0; total=0
for d in $DIRS; do
  for f in SRC/$d/*.CPP; do
    [ -e "$f" ] || continue
    # Skip Watcom "unity" aggregator stubs that #include other .cpp files
    # (the real build compiles the individual constituents).
    if grep -qiE 'include[[:space:]]*["<][^">]*\.cpp' "$f" 2>/dev/null; then continue; fi
    total=$((total+1))
    err=$(./port/cc.sh "$f" 2>&1)
    if [ -z "$err" ]; then
      ok=$((ok+1)); echo "OK   $f" >> "$LOG"
    else
      fail=$((fail+1)); echo "FAIL $f" >> "$LOG"
      # First error line, normalised (strip file:line, keep the message gist).
      echo "$err" | grep -m1 -E 'error:' | sed -E 's/^[^ ]+: error: //; s/[0-9]+/N/g; s/‘[^’]*’/X/g' >> "$ERRAGG"
    fi
  done
done
echo "==================================================="
echo "COMPILE OK: $ok / $total   (FAIL: $fail)"
echo "==================================================="
echo "Top first-error categories:"
sort "$ERRAGG" | uniq -c | sort -rn | head -25
