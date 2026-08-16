#!/usr/bin/env bash
# port/dialog_scroll.sh — gate for S140: a campaign dialog's list can actually be scrolled.
#
# WHY THIS EXISTS. RScrlBar was never a hosted control type, so every scrollable campaign dialog
# (Intelligence, Squadrons, Bases, the mission log) listed more rows than it could show with no
# way to reach them. Hosting it was not enough on its own -- three further things were each
# individually fatal and each looked like "hosted, fine":
#   1. the bars are placed by CRListBoxCtrl::UpdateScrollBar from GetClientRect, which the port
#      only fills at DRAW time -- so every bar was Move()d to a NEGATIVE rect and dropped;
#   2. the placement is written to the control, but the draw walk reads the CLIENT CWnd;
#   3. the bars are children of the LISTBOX, not the dialog, so neither draw walk nor the click
#      walk (both keyed on parent==dialog) ever saw them.
#
# So this gate asserts on the LIST, not on the bar: click the bar and require the rows to move.
# "The scrollbar is drawn" would have passed at step 3 with a bar that did nothing.
set -u
export MA_NO_HARDWARE="${MA_NO_HARDWARE:-1}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WMIG="${WMIG:-$ROOT/build/wmig}"
BOB_DRIVE_C="${BOB_DRIVE_C:-$HOME/sgl/TUE/MigAlley/WP/drive_c}"
RUNDIR="$BOB_DRIVE_C/rowan/mig"
OUT="${OUT:-/tmp/ma_dlgscroll}"
NAV="30,r3;65,#1055;100,#2063:1;200,#2023@CMainToolbar"   # -> the Intelligence dialog
mkdir -p "$OUT"

SAVEFILE="$RUNDIR/SaveGame/Auto Save.sav"
PIN="$ROOT/port/ref/save/campaign_pristine.sav"
STASH="$(mktemp -u /tmp/ma_dlgscroll_save.XXXXXX)"
[ -f "$SAVEFILE" ] && cp -a "$SAVEFILE" "$STASH"
restore() { [ -f "$STASH" ] && cp -a "$STASH" "$SAVEFILE" && rm -f "$STASH"; }
trap restore EXIT INT TERM

run() {  # $1=out.ppm $2=log $3=extra clicks
  cp -f "$PIN" "$SAVEFILE"
  ( cd "$RUNDIR" && timeout -k 5 -s KILL 130 env SDL_VIDEODRIVER=dummy \
      BOB_RUN_INIT=1 MA_DISABLE_3D=1 MA_IGNORE_SAVE_DATE=1 BOB_DRIVE_C="$BOB_DRIVE_C" \
      MA_TRACE_SCROLL=1 BOB_CLICKSEQ="$NAV${3:+;$3}" MA_SHOT=320 MA_SHOT_PATH="$1" \
      "$WMIG" ) > "$2" 2>&1
  pkill -x "$(basename "$WMIG")" 2>/dev/null
  return 0
}

echo "campaign dialog scrolling — $(basename "$WMIG")"

# 1. find a vertical bar the dialog actually drew, and its DOWN arrow
run "$OUT/base.ppm" "$OUT/base.log" ""
BAR=$(grep -a "\[scroll\] draw" "$OUT/base.log" |
      sed 's/.*at (\([0-9]*\),\([0-9]*\)) \([0-9]*\)x\([0-9]*\).*/\1 \2 \3 \4/' |
      awk '$3 < $4 && $4 > 40 {print; exit}')     # vertical: taller than wide
if [ -z "$BAR" ]; then
  echo "  the dialog drew no vertical scrollbar at all"
  echo "  RESULT: FAIL — see $OUT/base.log"; exit 1
fi
set -- $BAR
CX=$(( $1 + $3 / 2 )); CY=$(( $2 + $4 - 6 ))     # the down arrow, at the bar's foot
echo "  vertical bar at ($1,$2) $3x$4 — clicking its down arrow at ($CX,$CY)"

# 2. click it and require the LIST to move
run "$OUT/after.ppm" "$OUT/after.log" "260,$CX,$CY"
grep -aq "\[scroll\] click" "$OUT/after.log" || { echo "  the click never reached a scrollbar"; echo "  RESULT: FAIL"; exit 1; }
echo "  $(grep -a '\[scroll\] click' "$OUT/after.log" | head -1 | sed 's/.*local/local/')"

python3 - "$OUT/base.ppm" "$OUT/after.ppm" $1 $2 $3 $4 <<'PYS'
import sys
from PIL import Image, ImageChops
a = Image.open(sys.argv[1]).convert('RGB'); b = Image.open(sys.argv[2]).convert('RGB')
bx, by, bw, bh = (int(v) for v in sys.argv[3:7])
# the ROWS: the strip to the left of the bar, over its height. Excluding the bar itself matters --
# the thumb moves whether or not the list does, and that is exactly the false pass to avoid.
box = (max(bx - 460, 0), by, bx, by + bh)
d = ImageChops.difference(a.crop(box), b.crop(box)).convert('L')
n = sum(1 for p in d.getdata() if p > 8)
print("  list pixels changed: %d" % n)
sys.exit(0 if n > 2000 else 1)
PYS
if [ $? -eq 0 ]; then echo "  RESULT: PASS (the list scrolls)"; exit 0; fi
echo "  the bar moved but the rows did not"
echo "  RESULT: FAIL — captures in $OUT"; exit 1
