#!/usr/bin/env bash
# B2 A/B harness: capture native software-rasterizer frames at each flight view and
# compare them to the Wine pixel-oracle references in port/ref/wine/.
#
# For each view it: launches wmig into 3D flight, taps the view's F-key (BOB_KEYSEQ),
# lets the view settle, dumps the back surface (MA_DUMP_BACK -> /tmp/maback.ppm),
# then runs ab_compare.py against the matching Wine reference.
#
# Outputs land in port/out/ab/: <view>_native.png, _wine.png, _diff.png, _sidebyside.png.
#
# Usage:  port/ab.sh [view ...]      (default: all)
#         views: cockpit external chase satellite
# Env:    BOB_DRIVE_C  Wine drive_c (default below)
#         WMIG         binary (default /tmp/wmig)
#         DUMP_FRAME   back-Blt index to capture (default 220 -- well past the view switch)
#         KEY_AT       pump count at which to tap the view key (default 80)
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WMIG="${WMIG:-/tmp/wmig}"
BOB_DRIVE_C="${BOB_DRIVE_C:-$HOME/sgl/TUE/MigAlley/WP/drive_c}"
RUNDIR="$BOB_DRIVE_C/rowan/mig"
REF="$ROOT/port/ref/wine"
OUT="$ROOT/port/out/ab"
# DUMP_FRAME = back-Blt index to capture; KEY_AT = acquired-pump count at which to tap
# the view key. The view key must land BEFORE the dump: KEY_AT counts only pumps where
# the DI keyboard is acquired (acquisition happens a few seconds into flight), so a small
# KEY_AT fires shortly after acquisition and a larger DUMP_FRAME lets the new view settle.
DUMP_FRAME="${DUMP_FRAME:-300}"
KEY_AT="${KEY_AT:-10}"
# Two-click path into flight: Single Player -> Hot Shot (from stress_launch.sh).
CLICKSEQ="${BOB_CLICKSEQ:-40,588,231;95,588,217}"
TIMEOUT="${TIMEOUT:-40}"

# view -> "F-key DIK | reference png | label". cockpit = default launch view (no key).
declare -A KEY REFPNG
KEY[cockpit]=""          ; REFPNG[cockpit]="01_cockpit_fwd_gunsight.png"
KEY[external]="0x40"     ; REFPNG[external]="04_ext_chase_high.png"      # F6 outside
KEY[chase]="0x43"        ; REFPNG[chase]="05_ext_chase_low_airfield.png" # F9 chase
KEY[satellite]="0x44"    ; REFPNG[satellite]="06_ext_flyby_terrain.png"  # F10 satellite

[ -x "$WMIG" ] || { echo "no wmig at $WMIG (run port/rebuild.sh)"; exit 2; }
[ -d "$RUNDIR" ] || { echo "no rundir $RUNDIR (set BOB_DRIVE_C)"; exit 2; }
command -v python3 >/dev/null || { echo "need python3 + PIL"; exit 2; }
mkdir -p "$OUT"

VIEWS=("$@"); [ ${#VIEWS[@]} -eq 0 ] && VIEWS=(cockpit external chase satellite)

capture() {  # $1 = view name
  local v="$1" dik="${KEY[$1]}" log="$OUT/$1.log"
  local keyseq=""
  [ -n "$dik" ] && keyseq="$KEY_AT,$dik"
  rm -f /tmp/maback.ppm; : > "$log"
  echo "  [$v] launching (key=${dik:-none} dump#=$DUMP_FRAME)..."
  ( cd "$RUNDIR" && exec env BOB_RUN_INIT=1 MA_ENABLE_3D=1 \
        MA_DUMP_BACK="$DUMP_FRAME" BOB_CLICKSEQ="$CLICKSEQ" BOB_KEYSEQ="$keyseq" \
        BOB_DRIVE_C="$BOB_DRIVE_C" "$WMIG" ) >"$log" 2>&1 &
  local pid=$!
  local got=1
  for _ in $(seq 1 $((TIMEOUT*5))); do
    if grep -aq "dumped back-surface Blt" "$log" 2>/dev/null; then got=0; break; fi
    kill -0 "$pid" 2>/dev/null || break
    sleep 0.2
  done
  kill -KILL "$pid" 2>/dev/null; wait "$pid" 2>/dev/null
  if [ $got -ne 0 ] || [ ! -s /tmp/maback.ppm ]; then
    echo "    ! no frame captured (see $log)"; return 1
  fi
  cp /tmp/maback.ppm "$OUT/$v.ppm"
  return 0
}

echo "A/B harness: views=[${VIEWS[*]}]  dir=$RUNDIR"
fail=0
for v in "${VIEWS[@]}"; do
  [ -n "${REFPNG[$v]:-}" ] || { echo "  unknown view '$v'"; fail=1; continue; }
  ref="$REF/${REFPNG[$v]}"
  [ -f "$ref" ] || { echo "  [$v] missing ref $ref"; fail=1; continue; }
  if capture "$v"; then
    python3 "$ROOT/port/ab_compare.py" "$OUT/$v.ppm" "$ref" "$OUT/$v" \
      ${AB_CROP:+--crop="$AB_CROP"} || fail=1
  else
    fail=1
  fi
done
echo "----------------------------------------"
echo "Outputs in $OUT/  (open <view>_sidebyside.png: native | wine | diff)"
exit $fail
