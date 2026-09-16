#!/usr/bin/env bash
# port/weather_panel.sh — capture the campaign map's Cloud Base weather panel, headless.
#
# WEATHERPANEL-1. The 2026-09-15 gold campaign video shows this dialog
# (port/reference/wine-gold/260915_gold_weather_panel.png) with all ten of its fields legible, which
# makes it the one campaign screen that can be graded value by value rather than by eye.
#
#   port/weather_panel.sh [outdir]
#
# Prints the atmosphere state behind the panel (MA_OOB_WEATHER's own trace) and writes the frame.
# No display needed: SDL_VIDEODRIVER=dummy + MA_SHOT.
#
# ⚠️ Four of the ten fields are RANDOM per mission (pressure and gusts follow TempVar; both wind rows
# are Math_Lib.rnd draws), so this is NOT a pixel oracle and must not be turned into one. What it
# checks is the six DETERMINISTIC fields -- conditions, visibility, temperature, cloud layer and the
# two contrail altitudes -- which are computed from constants this port compiles.
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WMIG="${WMIG:-$ROOT/build/wmig}"
BOB_DRIVE_C="${BOB_DRIVE_C:-$HOME/sgl/TUE/MigAlley/WP/drive_c}"
RUNDIR="$BOB_DRIVE_C/rowan/mig"
OUT="${1:-/home/admin/ma-weather}"          # never /tmp: a frame dump is game data
SHOT="${SHOT:-200}"
mkdir -p "$OUT"
[ -x "$WMIG" ] || { echo "no wmig at $WMIG" >&2; exit 2; }
if pgrep -x wmig >/dev/null 2>&1; then
  echo "  REFUSING TO RUN: wmig is already running (pid $(pgrep -x wmig | tr '\n' ' '))." >&2
  echo "  It may be the player's own session." >&2; exit 2
fi
ppm="$OUT/weather.ppm"; log="$OUT/weather.log"
rm -f "$ppm"
( cd "$RUNDIR" && timeout -k 5 -s KILL 200 env \
    SDL_VIDEODRIVER=dummy BOB_RUN_INIT=1 BOB_DRIVE_C="$BOB_DRIVE_C" \
    MA_DISABLE_3D=1 MA_IGNORE_SAVE_DATE=1 MA_NO_HARDWARE=1 \
    BOB_CLICKSEQ="30,r3;65,#1055;100,#2063:1" MA_OOB_WEATHER=1 \
    MA_SHOT="$SHOT" MA_SHOT_PATH="$ppm" "$WMIG" ) >"$log" 2>&1
pkill -x wmig 2>/dev/null
grep -a "^\[weather\]" "$log" | sed 's/^/  /'
if [ -s "$ppm" ]; then
  python3 - "$ppm" "$OUT/weather_panel.png" <<'PY'
import sys
from PIL import Image
Image.open(sys.argv[1]).crop((470, 295, 800, 720)).save(sys.argv[2])
print("  panel ->", sys.argv[2])
PY
else
  echo "  ! no capture (see $log)"; exit 1
fi
