#!/usr/bin/env bash
# ASan campaign save-load + strategic-map regression gate (Sprint 40).
#
# Drives the loadgame flow headlessly under ASan:
#   title -> Load Game (row 3) -> select "Auto Save" -> Load -> Korea strategic map.
# This exercises CFiling::LoadGame -> `bis>>Miss_Man` -> Todays_Packages.LoadGame(bis)
# (PackageList::LoadGame, the S65a delete[] site) + the strategic-map render.
# Fails if any run emits an ASan report, or if the map never renders.
#
#   port/asan_campaign.sh [RUNS] [TIMEOUT_S]
#
# Requires: the ASan build (port/asan.sh build) AND a "SaveGame/Auto Save.sav".
# MA_IGNORE_SAVE_DATE=1 bypasses the build-date guard so a save from an earlier
# build still loads (the save FORMAT is stable across rebuilds).
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
RUNS="${1:-2}"
TIMEOUT="${2:-120}"   # S159: 80s was not enough for the ASan build to reach the map on this box
WMIG="${WMIG:-/tmp/wmig-asan}"
BOB_DRIVE_C="${BOB_DRIVE_C:-$HOME/sgl/TUE/MigAlley/WP/drive_c}"
RUNDIR="$BOB_DRIVE_C/rowan/mig"
# title -> Campaign -> load "Auto Save" -> operational map.
# S159: this recipe was HARDCODED PIXELS (30,588,263;65,40,108;100,68,565) and it stopped
# navigating -- the gate reported "NO-MAP / INCONCLUSIVE" twice about a binary whose map path is
# fine (proved by re-running the same gate with the recipe below: MAP-OK 2/2, ASan-clean, and by
# an A/B with MA_NO_ART_CLIP=1 that failed identically, so it was not the sprint's change).
# A gate that cannot navigate reports on the harness, not the code, and this one had been
# reporting INCONCLUSIVE.
# Root cause NOT established -- note that port/hw_gate.sh still passes with the same three pixels,
# so whatever moved is conditional (resolution left in settings.mig by another gate is the leading
# suspect since S103 made preferences actually load). Rather than chase it, use the symbolic form
# S62/S63 mandated after a font change silently broke every pixel recipe at once: `f,rN` (menu row)
# and `f,#ID` (control by dialog id), resolved from the controls' own metrics.
# The three remaining pixel recipes -- port/ab.sh, port/asan_flight.sh, port/hw_gate.sh -- are
# logged for the same treatment.
CLICKSEQ="${BOB_CLICKSEQ:-30,r3;65,#1055;100,#2063:1}"

[ -x "$WMIG" ] || { echo "no ASan binary at $WMIG — run: port/asan.sh build" >&2; exit 2; }
[ -f "$RUNDIR/SaveGame/Auto Save.sav" ] || { echo "no 'Auto Save.sav' in $RUNDIR/SaveGame" >&2; exit 2; }

export ASAN_OPTIONS="halt_on_error=0:detect_odr_violation=0:detect_leaks=0:abort_on_error=1:log_path=/tmp/wmig-asan.log:print_stats=0:symbolize=1"

echo "ASan campaign gate: $RUNS run(s), timeout ${TIMEOUT}s"
rm -f /tmp/wmig-asan.log.*
mapped=0
for i in $(seq 1 "$RUNS"); do
  out="/tmp/asan_campaign_gate_$i.out"
  ( cd "$RUNDIR" && timeout "$TIMEOUT" env BOB_RUN_INIT=1 MA_DISABLE_3D=1 MA_IGNORE_SAVE_DATE=1 \
        MA_TRACE_3D=1 BOB_CLICKSEQ="$CLICKSEQ" BOB_DRIVE_C="$BOB_DRIVE_C" "$WMIG" ) >"$out" 2>&1
  pkill -x "$(basename "$WMIG")" 2>/dev/null; sleep 0.5
  if grep -aq "\[map\] render operational map" "$out"; then mapped=$((mapped+1)); r="MAP-OK"; else r="NO-MAP"; fi
  echo "  run $i/$RUNS: $r"
done

nerr=0
for f in /tmp/wmig-asan.log.* ; do [ -e "$f" ] || continue; nerr=$((nerr + $(grep -ac "ERROR: AddressSanitizer" "$f"))); done

echo "----------------------------------------"
echo "runs rendering the strategic map: $mapped/$RUNS"
if [ "$mapped" -eq 0 ]; then echo "INCONCLUSIVE: map never rendered (load/nav regression?)"; exit 3; fi
if [ "$nerr" -eq 0 ]; then
  echo "PASS: 0 AddressSanitizer reports across $RUNS run(s) — campaign load+map path ASan-clean"
  exit 0
else
  echo "FAIL: $nerr AddressSanitizer report(s):"
  grep -ah "SUMMARY: AddressSanitizer" /tmp/wmig-asan.log.* | sort | uniq -c
  exit 1
fi
