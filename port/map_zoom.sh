#!/usr/bin/env bash
# port/map_zoom.sh — PO-27 ("the map zoom button zooms the map"; PO 2026-08-15, screenshot:
# "zooming the map worked except it produced tiles").
#
# Captures the campaign map at a sequence of ZOOM LEVELS from the game's own zoom-in button
# (IDC_ZOOMIN == #7 on CMiscToolbar -- addressed by CONTROL ID, never by pixel, per S63) and
# measures two properties of each capture:
#
#   coarse   the fraction of horizontally adjacent pixel pairs that are IDENTICAL. A map tile
#            magnified by StretchDIBits repeats each source pixel, so magnification shows up
#            here directly: 1x ~ the art's own run structure, 2x doubles it, 4x again.
#   seam     tile boundaries fall every `zoomsquaresize` pixels. For each candidate boundary
#            column the mean |ΔRGB| across it is compared with the median of the same measure
#            over all columns; a visible seam is a boundary column far above that.
#
# Both numbers are computed on the MAP AREA ONLY (below the toolbars, right of the panel), so
# the dialogs and the ruler cannot flatter or spoil them.
#
#   port/map_zoom.sh [n-zoom-clicks ...]      (default: 0 1 2 3)
set -u
export MA_NO_HARDWARE="${MA_NO_HARDWARE:-1}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WMIG="${WMIG:-$ROOT/build/wmig}"
BOB_DRIVE_C="${BOB_DRIVE_C:-$HOME/sgl/TUE/MigAlley/WP/drive_c}"
RUNDIR="$BOB_DRIVE_C/rowan/mig"
OUT="${OUT:-/tmp/ma_mapzoom}"
TMO="${TMO:-150}"
RES="${RES:-1920x1080}"
. "$ROOT/port/gate_lib.sh" 2>/dev/null || true
command -v assert_clean_start >/dev/null 2>&1 && { assert_clean_start || exit 2; }

mkdir -p "$OUT"
[ -x "$WMIG" ] || { echo "no binary at $WMIG" >&2; exit 2; }

SAVEDIR="$RUNDIR/SaveGame"
PIN="$ROOT/port/ref/save/campaign_pristine.sav"
PIN_SET="$ROOT/port/ref/save/settings_pristine.mig"
pin() {
  [ -f "$PIN" ] || return 0
  ma_safe_backup "$SAVEDIR/Auto Save.sav" "$OUT/player_autosave.bak" || { echo "  save backup refused; not pinning" >&2; PIN_SKIPPED=1; return 1; }
  PIN_SKIPPED=0; cp -f "$PIN" "$SAVEDIR/Auto Save.sav"
  [ -f "$PIN_SET" ] && { [ -f "$SAVEDIR/settings.mig" ] && cp -a "$SAVEDIR/settings.mig" "$OUT/player_settings.bak"; cp -f "$PIN_SET" "$SAVEDIR/settings.mig"; }
}
unpin() {
  [ "${PIN_SKIPPED:-1}" = "1" ] || ma_safe_restore "$OUT/player_autosave.bak" "$SAVEDIR/Auto Save.sav"
  if [ -f "$OUT/player_settings.bak" ]; then cp -a "$OUT/player_settings.bak" "$SAVEDIR/settings.mig"; fi
}

# to the campaign map (parity_2d's recipe), then N zoom-in clicks on the misc toolbar
BASE="30,r3;65,#1055;100,#2063:1"
for N in "${@:-0 1 2 3}"; do
  SEQ="$BASE"; t=130
  for i in $(seq 1 "$N"); do SEQ="$SEQ;$t,#7@CMiscToolbar"; t=$((t+15)); done
  SHOT=$((t+40))
  ppm="$OUT/zoom$N.ppm"; rm -f "$ppm"
  pin
  ( cd "$RUNDIR" && timeout -k 5 -s KILL "$TMO" env \
      SDL_VIDEODRIVER=dummy BOB_RUN_INIT=1 BOB_DRIVE_C="$BOB_DRIVE_C" MA_DISABLE_3D=1 \
      MA_IGNORE_SAVE_DATE=1 MA_TRACE_MAPTILE=1 MA_FORCE_RES="$RES" MA_MAXIMIZE=1 \
      BOB_CLICKSEQ="$SEQ" MA_SHOT="$SHOT" MA_SHOT_PATH="$ppm" \
      "$WMIG" ) >"$OUT/zoom$N.log" 2>&1
  pkill -x "$(basename "$WMIG")" 2>/dev/null
  unpin
  [ -s "$ppm" ] || { echo "  zoom$N: NO CAPTURE (see $OUT/zoom$N.log)"; continue; }
  echo "  zoom$N: m_zoom=$(grep -a '\[maptile\] client=' "$OUT/zoom$N.log" | tail -1 | sed 's/.*zoom=\([0-9]*\).*/\1/')  tile=$(grep -a '\[maptile\] client=' "$OUT/zoom$N.log" | tail -1 | sed 's/.*tile=\([0-9]*\).*/\1/')"
  python3 "$ROOT/port/tools/map_zoom_measure.py" "$ppm" "$N" "$OUT/zoom$N.log"
done
