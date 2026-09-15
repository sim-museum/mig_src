#!/usr/bin/env bash
# KEYHOLD-1 S3: run the SAME flight twice, changing ONLY the BOB_AUTOFLY argument, and print the
# key-push window and the elevator trace from both.
#
# S2 tallied 5 successes of `dive:<tick>` against 4 failures of `dive:<tick>:<stop>:<pull>` and
# concluded the form of the argument decides it -- but those nine runs were spread over three
# sprints and differed in more than the argument. This harness removes that confound: same binary,
# same mission, same run length, same traces, back to back.
#
# Usage: port/keyhold_ab.sh [seconds]        (default 100)
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WMIG="${WMIG:-$ROOT/build/wmig}"
BOB_DRIVE_C="${BOB_DRIVE_C:-$HOME/sgl/TUE/MigAlley/WP/drive_c}"
RUNDIR="$BOB_DRIVE_C/rowan/mig"
OUT="${OUT:-/home/admin/ma-keyhold}"          # never /tmp: game data and traces live under /home
SECS="${1:-100}"
CLICKSEQ="${BOB_CLICKSEQ:-40,r1;95,r0}"
mkdir -p "$OUT"
[ -x "$WMIG" ] || { echo "no wmig at $WMIG"; exit 2; }

run_one() {  # $1 = label, $2 = BOB_AUTOFLY value
  local lbl="$1" af="$2"
  local log="$OUT/$lbl.log"   # a single `local` expands all words BEFORE assigning, so $lbl is unset there
  : > "$log"
  echo "  [$lbl] BOB_AUTOFLY=$af  (${SECS}s)"
  ( cd "$RUNDIR" && exec env BOB_RUN_INIT=1 MA_ENABLE_3D=1 \
        BOB_CLICKSEQ="$CLICKSEQ" BOB_AUTOFLY="$af" \
        MA_TRACE_KEY=1 MA_TRACE_ELEV=1 MA_TRACE_HUD=120 \
        BOB_DRIVE_C="$BOB_DRIVE_C" "$WMIG" ) >"$log" 2>&1 &
  local pid=$!
  for _ in $(seq 1 $((SECS*2))); do kill -0 "$pid" 2>/dev/null || break; sleep 0.5; done
  kill -KILL "$pid" 2>/dev/null; wait "$pid" 2>/dev/null
  echo "    parse/push:"; grep -a "\[autofly\]" "$log" | head -14 | sed 's/^/      /'
  echo "    elevator:";   grep -a "\[elev\]"    "$log" | head -6  | sed 's/^/      /'
  echo "    altitude:";   grep -a "\[hud\]"     "$log" | sed -n '1p;$p' | sed 's/^/      /'
}

run_one simple   "dive:60"
run_one extended "dive:60:600:200"
echo "logs in $OUT"
