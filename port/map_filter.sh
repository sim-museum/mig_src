#!/usr/bin/env bash
# port/map_filter.sh — gate for PO-30: the campaign map's icon FILTER buttons actually filter.
#
# WHY THIS EXISTS. The 30 red/blue filter buttons routed their clicks, highlighted, and did
# nothing, for two independent reasons that both looked like "the click works":
#   1. CMapFilters registers ON_EVENT_RANGE(CMapFilters, 1, 9999, Clicked, OnClickedFilter) and
#      the port's range registrar REFUSED any span wider than 4096 -- so the toolbar's single
#      handler was silently discarded and nothing listened.
#   2. the port fired Clicked WITHOUT toggling the button, while CRButtonCtrl::OnLButtonUp does
#      `m_bPressed=!m_bPressed;` first -- so the handler, which asks the button what state it is
#      now in, always read FALSE and asked to clear a filter that was already clear.
#
# Either fix alone leaves the map unchanged, which is why this gate asserts on the MAP, not on
# the click: it clicks the red "all" filter and requires the map to actually change. A log line
# saying the handler ran is exactly the half-truth that let this ship.
#
# The button is LOCATED by clicking along the filter row and reading back the id the toolbar
# reports, rather than by a hardcoded pixel -- the row is right-aligned, so its x moves with the
# resolution (S95's rule).
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WMIG="${WMIG:-$ROOT/build/wmig}"
BOB_DRIVE_C="${BOB_DRIVE_C:-$HOME/sgl/TUE/MigAlley/WP/drive_c}"
RUNDIR="$BOB_DRIVE_C/rowan/mig"
RES="${1:-1920x1080}"
OUT="${OUT:-/tmp/ma_mapfilter}"
NAV="30,r3;65,#1055;100,#2063:1"
IDC_FILTER_RED_ALL=2076
mkdir -p "$OUT"

SAVEFILE="$RUNDIR/SaveGame/Auto Save.sav"
PIN="$ROOT/port/ref/save/campaign_pristine.sav"
STASH="$(mktemp -u /tmp/ma_filt_save.XXXXXX)"
[ -f "$SAVEFILE" ] && cp -a "$SAVEFILE" "$STASH"
restore() { [ -f "$STASH" ] && cp -a "$STASH" "$SAVEFILE" && rm -f "$STASH"; }
trap restore EXIT INT TERM

run() { # $1=out.ppm $2=log $3=extra clicks
  cp -f "$PIN" "$SAVEFILE"
  # NB no gl-lock here: this gate is run UNDER gl-lock like its siblings, and nesting the lock
  # deadlocks until the timeout kills it -- which presents as an empty log and a gate that
  # "fails" without ever starting the game.
  ( cd "$RUNDIR" && timeout -k 5 -s KILL 160 env SDL_VIDEODRIVER=dummy \
      BOB_RUN_INIT=1 MA_DISABLE_3D=1 MA_IGNORE_SAVE_DATE=1 MA_FORCE_RES="$RES" \
      MA_TRACE_CLICK=1 BOB_DRIVE_C="$BOB_DRIVE_C" \
      BOB_CLICKSEQ="$NAV${3:+;$3}" MA_SHOT=300 MA_SHOT_PATH="$1" "$WMIG" ) > "$2" 2>&1
}

echo "map icon filters @ $RES"

# 1. find the red "all" button: sweep the red row and read back the ids the toolbar reports
SEQ=""; f=150
for x in $(seq 1500 12 1910); do SEQ="$SEQ${SEQ:+;}$f,$x,38"; f=$((f+4)); done
run "$OUT/probe.ppm" "$OUT/probe.log" "$SEQ"
XY=$(grep -a "tbclick] id=$IDC_FILTER_RED_ALL rect=(" "$OUT/probe.log" | head -1 |
     sed 's/.*rect=(\([0-9]*\),\([0-9]*\),\([0-9]*\),\([0-9]*\)).*/\1 \2 \3 \4/')
if [ -z "$XY" ]; then
  echo "  the filter toolbar never reported id $IDC_FILTER_RED_ALL — cannot test"
  echo "  RESULT: FAIL"; exit 1
fi
set -- $XY
CX=$(( $1 + $3 / 2 )); CY=$(( $2 + $4 / 2 ))
echo "  IDC_FILTER_RED_ALL located at pixel ($CX,$CY)"

# 2. map before and after one click on it
run "$OUT/before.ppm" "$OUT/before.log" ""
run "$OUT/after.ppm"  "$OUT/after.log"  "250,$CX,$CY"

grep -aq "tbclick] id=$IDC_FILTER_RED_ALL" "$OUT/after.log" \
  || { echo "  the click did not reach the button"; echo "  RESULT: FAIL"; exit 1; }

python3 - "$OUT/before.ppm" "$OUT/after.ppm" <<'PY'
import sys
from PIL import Image, ImageChops
a = Image.open(sys.argv[1]).convert('RGB'); b = Image.open(sys.argv[2]).convert('RGB')
# the MAP only: below the toolbar band, right of the ruler
box = (60, 110, a.size[0] - 20, a.size[1] - 10)
d = ImageChops.difference(a.crop(box), b.crop(box)).convert('L')
n = sum(1 for p in d.getdata() if p > 8)
print("  map pixels changed by the filter: %d" % n)
sys.exit(0 if n > 5000 else 1)
PY
if [ $? -eq 0 ]; then echo "  RESULT: PASS (the filter changes the map)"; exit 0; fi
echo "  the handler ran but the map did not change — the filter is inert"
echo "  RESULT: FAIL — captures in $OUT"; exit 1
