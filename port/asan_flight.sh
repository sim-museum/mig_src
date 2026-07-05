#!/usr/bin/env bash
# ASan flight-path regression gate (Sprint 39).
#
# Codifies the S15->S38 milestone: an instrumented Hot Shot flight must produce
# ZERO AddressSanitizer reports. Runs N flights (the residual bugs were
# content-dependent singletons, so a single run under-samples) and fails if ANY
# run emits an ASan error.
#
#   port/asan_flight.sh [RUNS] [DUMP_FRAME] [TIMEOUT_S]
#
# Requires the ASan build:  port/asan.sh build   (-> /tmp/wmig-asan)
# Env: BOB_DRIVE_C (default below).
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
RUNS="${1:-3}"
DUMP="${2:-250}"
TIMEOUT="${3:-70}"
WMIG="${WMIG:-/tmp/wmig-asan}"
BOB_DRIVE_C="${BOB_DRIVE_C:-/home/m/sgl/TUE/MigAlley/WP/drive_c}"
RUNDIR="$BOB_DRIVE_C/rowan/mig"
# title -> Single Player -> Hot Shot (same nav as stress_launch.sh)
CLICKSEQ="${BOB_CLICKSEQ:-40,588,231;95,588,217}"

[ -x "$WMIG" ] || { echo "no ASan binary at $WMIG — run: port/asan.sh build" >&2; exit 2; }
[ -d "$RUNDIR" ] || { echo "no rundir $RUNDIR" >&2; exit 2; }

export ASAN_OPTIONS="halt_on_error=0:detect_odr_violation=0:detect_leaks=0:abort_on_error=1:log_path=/tmp/wmig-asan.log:print_stats=0:symbolize=1"

echo "ASan flight gate: $RUNS run(s), dump@${DUMP}f, timeout ${TIMEOUT}s"
rm -f /tmp/wmig-asan.log.*
reached=0
for i in $(seq 1 "$RUNS"); do
  out="/tmp/asan_flight_gate_$i.out"
  ( cd "$RUNDIR" && timeout "$TIMEOUT" env BOB_RUN_INIT=1 MA_ENABLE_3D=1 MA_DUMP_BACK="$DUMP" \
        BOB_CLICKSEQ="$CLICKSEQ" BOB_DRIVE_C="$BOB_DRIVE_C" "$WMIG" ) >"$out" 2>&1
  pkill -x "$(basename "$WMIG")" 2>/dev/null; sleep 0.5
  if grep -aq "dumped back-surface Blt" "$out"; then reached=$((reached+1)); r="3D-OK"; else r="NO-3D"; fi
  echo "  run $i/$RUNS: $r"
done

nerr=0
for f in /tmp/wmig-asan.log.* ; do
  [ -e "$f" ] || continue
  c=$(grep -ac "ERROR: AddressSanitizer" "$f")
  nerr=$((nerr + c))
done

echo "----------------------------------------"
echo "flights reaching 3D: $reached/$RUNS"
if [ "$reached" -eq 0 ]; then
  echo "INCONCLUSIVE: no run reached 3D (nav/click regression?) — not a clean pass"; exit 3
fi
if [ "$nerr" -eq 0 ]; then
  echo "PASS: 0 AddressSanitizer reports across $RUNS flight(s) — flight path ASan-clean"
  exit 0
else
  echo "FAIL: $nerr AddressSanitizer report(s):"
  grep -ah "SUMMARY: AddressSanitizer" /tmp/wmig-asan.log.* | sort | uniq -c
  exit 1
fi
