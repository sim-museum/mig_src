#!/usr/bin/env bash
# A4 — 3D-launch stress harness (Sprint 1, story A1).
# Repeatedly launches wmig into 3D flight and classifies each run as:
#   OK     — software rasterizer sustained N frames (MA_DUMP_BACK marker seen), no crash
#   SEGV   — exit 139 (SIGSEGV)
#   FPE    — exit 136 (SIGFPE)
#   ABORT  — exit 134 (SIGABRT)
#   NO3D   — process exited/closed before reaching the frame target (window-close / early exit)
#   HANG   — neither died nor hit the frame target within the per-run timeout
#
# Usage: port/stress_launch.sh [RUNS] [FRAME_TARGET] [TIMEOUT_S]
# Env:   BOB_DRIVE_C must point at the Wine drive_c (default below).
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
RUNS="${1:-20}"
FRAMES="${2:-100}"
TIMEOUT="${3:-25}"
WMIG="${WMIG:-/tmp/wmig}"
BOB_DRIVE_C="${BOB_DRIVE_C:-/home/m/sgl/TUE/MigAlley/WP/drive_c}"
RUNDIR="$BOB_DRIVE_C/rowan/mig"
# Full title menu (F4, Sprint 4): flight = Single Player (title row 1) -> Hot Shot (singleplayer
# row 0). Was one click on demotitle's "Hot Shot" row before MIG.CPP launched the full title.
CLICKSEQ="${BOB_CLICKSEQ:-40,588,231;95,588,217}"
LOGDIR="${LOGDIR:-/tmp/ma_stress}"

mkdir -p "$LOGDIR"
[ -x "$WMIG" ] || { echo "no wmig at $WMIG"; exit 2; }
[ -d "$RUNDIR" ] || { echo "no rundir $RUNDIR"; exit 2; }

declare -A TALLY
ok=0
echo "Stress: $RUNS runs, frame target=$FRAMES, timeout=${TIMEOUT}s, dir=$RUNDIR"
for i in $(seq 1 "$RUNS"); do
  log="$LOGDIR/run_$i.log"
  : > "$log"
  rm -f /tmp/maback.ppm
  ( cd "$RUNDIR" && exec env BOB_RUN_INIT=1 MA_ENABLE_3D=1 MA_TRACE_3D=1 \
        MA_DUMP_BACK="$FRAMES" BOB_CLICKSEQ="$CLICKSEQ" BOB_DRIVE_C="$BOB_DRIVE_C" \
        "$WMIG" ) >"$log" 2>&1 &
  pid=$!
  result="HANG"
  for _ in $(seq 1 $((TIMEOUT*5))); do
    if grep -aq "dumped back-surface Blt" "$log" 2>/dev/null; then
      result="OK"; kill -KILL "$pid" 2>/dev/null; break
    fi
    if ! kill -0 "$pid" 2>/dev/null; then
      wait "$pid"; code=$?
      case $code in
        139) result="SEGV";;
        136) result="FPE";;
        134) result="ABORT";;
        *)   result="NO3D";;   # exited on its own before the frame target
      esac
      break
    fi
    sleep 0.2
  done
  kill -KILL "$pid" 2>/dev/null; wait "$pid" 2>/dev/null
  TALLY[$result]=$(( ${TALLY[$result]:-0} + 1 ))
  [ "$result" = OK ] && ok=$((ok+1))
  printf "  run %2d/%-2d : %s\n" "$i" "$RUNS" "$result"
done

echo "----------------------------------------"
echo "PASS: $ok/$RUNS reached & sustained $FRAMES 3D frames"
for k in "${!TALLY[@]}"; do printf "  %-6s %d\n" "$k" "${TALLY[$k]}"; done
echo "Logs in $LOGDIR/"
# exit 0 only on a clean sweep
[ "$ok" -eq "$RUNS" ]
