#!/usr/bin/env bash
# Unified ASan regression suite (Sprint 43) — the full S15->S42 coverage in one command.
#
# Runs every driven path the ASan oracle covers and fails if ANY run emits a report:
#   1. flight      — Hot Shot quick-mission 3D flight            (S15->S38)
#   2. camp-map     — load "Auto Save" -> Korea strategic map     (S40; PackageList::LoadGame / S65a)
#   3. camp-fly     — campaign mission-gen -> fly                 (S41; make_airgrp / FragInit)
#   4. camp-nextday — day-advance strategic sim                  (S42; NextMission / NextDay)
#
#   port/asan_all.sh [RUNS_PER_MODE] [TIMEOUT_S]
#
# Requires the ASan build (port/asan.sh build -> /tmp/wmig-asan) and a "SaveGame/Auto Save.sav".
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
RUNS="${1:-2}"
TIMEOUT="${2:-80}"
WMIG="${WMIG:-/tmp/wmig-asan}"
BOB_DRIVE_C="${BOB_DRIVE_C:-$HOME/sgl/TUE/MigAlley/WP/drive_c}"
RUNDIR="$BOB_DRIVE_C/rowan/mig"
CLICKSEQ_FLY="40,588,231;95,588,217"                     # title -> Single Player -> Hot Shot
CLICKSEQ_CAMP="30,588,263;65,40,108;100,68,565"          # title -> Load Game -> Auto Save -> Load

[ -x "$WMIG" ] || { echo "no ASan binary at $WMIG — run: port/asan.sh build" >&2; exit 2; }
[ -d "$RUNDIR" ] || { echo "no rundir $RUNDIR" >&2; exit 2; }
[ -f "$RUNDIR/SaveGame/Auto Save.sav" ] || { echo "no 'Auto Save.sav' in $RUNDIR/SaveGame" >&2; exit 2; }

export ASAN_OPTIONS="halt_on_error=0:detect_odr_violation=0:detect_leaks=0:abort_on_error=1:log_path=/tmp/wmig-asan.log:print_stats=0:symbolize=1"

# mode -> extra env + click seq + the progress marker that proves the path was reached
run_mode() {
  local name="$1" marker="$2" clickseq="$3"; shift 3
  local reached=0 i out
  for i in $(seq 1 "$RUNS"); do
    out="/tmp/asan_all_${name}_$i.out"
    rm -f /tmp/wmig-asan.log.*
    ( cd "$RUNDIR" && timeout "$TIMEOUT" env BOB_RUN_INIT=1 BOB_DRIVE_C="$BOB_DRIVE_C" \
        BOB_CLICKSEQ="$clickseq" "$@" "$WMIG" ) >"$out" 2>&1
    pkill -x "$(basename "$WMIG")" 2>/dev/null; sleep 0.4
    grep -aq "$marker" "$out" && reached=$((reached+1))
    for f in /tmp/wmig-asan.log.* ; do [ -e "$f" ] || continue
      GLOBAL_ERR=$((GLOBAL_ERR + $(grep -ac "ERROR: AddressSanitizer" "$f")))
      grep -ah "SUMMARY: AddressSanitizer" "$f" >> /tmp/asan_all_summaries.txt
    done
  done
  printf "  %-12s reached %d/%d\n" "$name" "$reached" "$RUNS"
  [ "$reached" -eq 0 ] && { echo "    !! $name never reached '$marker' (nav regression?)"; MODE_FAIL=1; }
}

GLOBAL_ERR=0; MODE_FAIL=0; : > /tmp/asan_all_summaries.txt
echo "ASan suite: $RUNS run(s)/mode, timeout ${TIMEOUT}s"
run_mode flight       "dumped back-surface"          "$CLICKSEQ_FLY"  MA_ENABLE_3D=1 MA_DUMP_BACK=200
run_mode camp-map     "render operational map"       "$CLICKSEQ_CAMP" MA_DISABLE_3D=1 MA_IGNORE_SAVE_DATE=1 MA_TRACE_3D=1
run_mode camp-fly     "driving singlefrag Fly"       "$CLICKSEQ_CAMP" MA_ENABLE_3D=1 MA_IGNORE_SAVE_DATE=1 MA_CAMP_FLY=1 MA_TRACE_3D=1 MA_DUMP_BACK=200
run_mode camp-nextday "driving NextDay"              "$CLICKSEQ_CAMP" MA_DISABLE_3D=1 MA_IGNORE_SAVE_DATE=1 MA_CAMP_NEXTDAY=1 MA_TRACE_3D=1

echo "----------------------------------------"
if [ "$GLOBAL_ERR" -eq 0 ] && [ "$MODE_FAIL" -eq 0 ]; then
  echo "PASS: 0 AddressSanitizer reports across all paths (flight + campaign map/fly/nextday)"
  exit 0
else
  [ "$GLOBAL_ERR" -gt 0 ] && { echo "FAIL: $GLOBAL_ERR ASan report(s):"; sort /tmp/asan_all_summaries.txt | uniq -c; }
  [ "$MODE_FAIL" -ne 0 ] && echo "FAIL: a path was not reached (see above)"
  exit 1
fi
