#!/usr/bin/env bash
# port/map_drag.sh — gate for PO-2: dragging the campaign map pans it, losslessly, and is not a click.
#
# The PO reported "click-drag on the map messes up the display". Root cause was in the compat GDI:
# a blit that merely OVERHANGS the screen grew the canvas instead of being clipped. The map is
# tiled, so every scrolled frame had tiles hanging off the edges and the whole screen got bigger,
# mid-drag, frame after frame. (It was also happening silently at rest: the front end establishes
# an 800x600 screen and overhanging tiles inflated the campaign map to 1021x644.)
#
# THREE assertions, because any one of them alone can pass for the wrong reason:
#   1. one-way drag  != baseline   — proves the drag actually moves the map
#   2. round trip    == baseline   — proves panning is lossless (no corruption, no canvas growth)
#   3. the drag release is SUPPRESSED as a click — proves a pan does not also open a dossier
#      (since S95 routed map clicks to CMapDlg, a drag that ended in a click edge opened one)
#
# Assertion 1 exists because it was needed: the first version of this test reported a perfect
# lossless round trip while the drag was doing NOTHING AT ALL — the hook pushed SDL events and the
# event queue was never drained without a window (the other half of the S93 bug). "0 px differ" is
# indistinguishable from "nothing happened" unless something else proves motion occurred.
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WMIG="${WMIG:-$ROOT/build/wmig}"
BOB_DRIVE_C="${BOB_DRIVE_C:-$HOME/sgl/TUE/MigAlley/WP/drive_c}"
RUNDIR="$BOB_DRIVE_C/rowan/mig"
OUT="${OUT:-/tmp/ma_mapdrag}"
TMO="${TMO:-130}"
NAV="30,r3;65,#1055;100,#2063:1"
SHOT_AT="${SHOT_AT:-400}"

mkdir -p "$OUT"
[ -x "$WMIG" ] || { echo "no binary at $WMIG" >&2; exit 2; }

SAVEFILE="$RUNDIR/SaveGame/Auto Save.sav"
PIN="$ROOT/port/ref/save/campaign_pristine.sav"
STASH="$(mktemp -u /tmp/ma_mapdrag_save.XXXXXX)"
[ -f "$SAVEFILE" ] && cp -a "$SAVEFILE" "$STASH"
restore_save() { [ -f "$STASH" ] && cp -a "$STASH" "$SAVEFILE" && rm -f "$STASH"; }
trap restore_save EXIT INT TERM

run() { # $1=log $2=ppm $3=drag spec (optional)
  [ -f "$PIN" ] && cp -f "$PIN" "$SAVEFILE"
  ( cd "$RUNDIR" && timeout -k 5 -s KILL "$TMO" env \
      SDL_VIDEODRIVER=dummy BOB_RUN_INIT=1 BOB_DRIVE_C="$BOB_DRIVE_C" MA_DISABLE_3D=1 \
      MA_IGNORE_SAVE_DATE=1 MA_TRACE_CLICK=1 BOB_CLICKSEQ="$NAV" \
      MA_SHOT="$SHOT_AT" MA_SHOT_PATH="$2" ${3:+BOB_DRAG="$3"} \
      "$WMIG" ) >"$1" 2>&1
  pkill -x "$(basename "$WMIG")" 2>/dev/null
}

run "$OUT/base.log"  "$OUT/base.ppm"
run "$OUT/oneway.log" "$OUT/oneway.ppm" "250,400,300,500,380,20"
run "$OUT/round.log"  "$OUT/round.ppm"  "250,400,300,500,380,20;300,500,380,400,300,20"

echo "map drag — $(basename "$WMIG")"
python3 - "$OUT" <<'PY'
import sys
from PIL import Image
out = sys.argv[1]
def load(n):
    im = Image.open(f"{out}/{n}.ppm").convert("RGB")
    return im.size, list(im.getdata())
try:
    sb, b = load("base"); s1, d1 = load("oneway"); s2, d2 = load("round")
except Exception as e:
    print(f"  RESULT: FAIL — could not read captures ({e})"); sys.exit(1)
ok = True
if not (sb == s1 == s2):
    print(f"  screen size changed across runs: {sb} {s1} {s2} — canvas grew during a drag"); ok = False
else:
    print(f"  screen {sb[0]}x{sb[1]} stable across all three runs")
moved = sum(1 for x, y in zip(b, d1) if x != y)
back  = sum(1 for x, y in zip(b, d2) if x != y)
print(f"  one-way drag vs baseline : {moved} px differ  (expect > 0 — the drag must move the map)")
print(f"  round trip  vs baseline  : {back} px differ  (expect 0 — panning must be lossless)")
if moved == 0: print("  the drag changed nothing — the pan never happened"); ok = False
if back  != 0: print("  the round trip did not restore the view — panning loses or corrupts pixels"); ok = False
sys.exit(0 if ok else 1)
PY
pyrc=$?

sup=$(grep -ac "suppressed: dragged" "$OUT/oneway.log")
if [ "$sup" -gt 0 ]; then
  echo "  drag release suppressed as a click: yes ($sup)"
else
  echo "  drag release suppressed as a click: NO — a pan will also fire a map click"
  pyrc=1
fi

[ "$pyrc" -eq 0 ] && { echo "  RESULT: PASS"; exit 0; }
echo "  RESULT: FAIL — logs in $OUT"; exit 1
