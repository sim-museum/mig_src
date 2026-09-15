#!/usr/bin/env bash
# GOLD3D-1 S10: capture a 3D frame AT A CHOSEN ALTITUDE, now that KEYHOLD-1 S3 made the dive
# timeline frame-accurate (the autofly ticks count presents, the same clock MA_TRACE_HUD reports,
# so "dump at frame N" and "the HUD said 5,280 ft at frame N" refer to the same instant).
#
#   gl-lock port/gold3d_state.sh <dumpframe> [autofly] [outdir]
#
# The verified profile (KEYHOLD-1 S3, BOB_AUTOFLY=dive:60):
#   frame  120   480   600   720   840   960  1080  1200  1320
#   alt  15944 13310 11561  9563  7452  5280  3060   804    44 ft
# so frame 960 is the gold campaign video's 5,116 ft state to within 164 ft.
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WMIG="${WMIG:-$ROOT/build/wmig}"
BOB_DRIVE_C="${BOB_DRIVE_C:-$HOME/sgl/TUE/MigAlley/WP/drive_c}"
RUNDIR="$BOB_DRIVE_C/rowan/mig"
FRAME="${1:-960}"
AF="${2:-dive:60}"
OUT="${3:-/home/admin/ma-gold3d}"          # never /tmp: a frame dump is game data
SECS="${SECS:-120}"
mkdir -p "$OUT"
[ -x "$WMIG" ] || { echo "no wmig at $WMIG" >&2; exit 2; }
log="$OUT/state_$FRAME.log"
ppm="$OUT/state_$FRAME.ppm"
rm -f "$ppm"
echo "capturing frame $FRAME with BOB_AUTOFLY=$AF (${SECS}s)"
( cd "$RUNDIR" && exec env BOB_RUN_INIT=1 MA_ENABLE_3D=1 \
      BOB_CLICKSEQ="${BOB_CLICKSEQ:-40,r1;95,r0}" BOB_AUTOFLY="$AF" \
      MA_TRACE_HUD=60 MA_DUMP_BACK="$FRAME" MA_DUMP_PATH="$ppm" \
      BOB_DRIVE_C="$BOB_DRIVE_C" "$WMIG" ) >"$log" 2>&1 &
pid=$!
for _ in $(seq 1 $((SECS*2))); do
  [ -s "$ppm" ] && break
  kill -0 "$pid" 2>/dev/null || break
  sleep 0.5
done
kill -KILL "$pid" 2>/dev/null; wait "$pid" 2>/dev/null
# State FIRST, pixels second: a capture that cannot say what the aeroplane was doing is not evidence.
echo "  HUD around the capture:"
grep -a "^\[hud\]" "$log" | tail -4 | sed 's/^/    /'
if [ -s "$ppm" ]; then
  echo "  wrote $ppm ($(stat -c%s "$ppm") bytes)"
  python3 "$ROOT/port/tools/gold3d_bands.py" "$ppm"
else
  echo "  ! no frame captured (see $log)"; exit 1
fi
