#!/usr/bin/env bash
# port/panel_click.sh — front-end menu click gate, in a REAL window at a NON-DEFAULT resolution.
#
# WHY THIS EXISTS (PO-24). S128-S130 centred the front-end panel art, filtered stale controls and
# offset the click hit-test to match. Every gate stayed green and the shipped build had a DEAD front
# end: clicking SINGLE PLAYER did nothing. The whole suite runs at the DEFAULT resolution, where the
# panel origin is zero and those three changes do exactly nothing -- so the suite could not have
# caught it. Same shape as S118, when the suite pinned the hardware device away and shipped a
# hardware option that crashed on the first flight.
#
# So: a real GL window, a non-default resolution, and a click at a PIXEL the menu is actually drawn
# at -- not a row index the harness resolves for us, because that resolution step is itself part of
# what breaks.
#
# The menu position is LOCATED, not hardcoded: the script captures a frame, finds the menu's yellow
# text, and clicks its first row. A hardcoded coordinate would silently stop pointing at the menu
# the moment the layout changed, which is the failure this gate exists to catch (S62/S63).
#
#   port/panel_click.sh [WxH]
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WMIG="${WMIG:-$ROOT/build/wmig}"
BOB_DRIVE_C="${BOB_DRIVE_C:-$HOME/sgl/TUE/MigAlley/WP/drive_c}"
RUNDIR="$BOB_DRIVE_C/rowan/mig"
RES="${1:-1920x1080}"
OUT="${OUT:-/tmp/ma_panelclick}"
mkdir -p "$OUT"

echo "front-end menu click — real window @ $RES"

run() { # $1=log  $2=extra env (may be empty)
  ( cd "$RUNDIR" && timeout -k 5 -s KILL 90 gl-lock env \
      BOB_RUN_INIT=1 MA_DISABLE_3D=1 MA_IGNORE_SAVE_DATE=1 MA_TRACE_OLE=1 \
      MA_FORCE_RES="$RES" BOB_DUMP_FRAME=200 BOB_EXIT_AFTER_DUMP=1 \
      BOB_DRIVE_C="$BOB_DRIVE_C" ${2:+BOB_CLICKSEQ="$2"} "$WMIG" ) > "$1" 2>&1
}

# 1. capture the title screen and locate the menu
run "$OUT/locate.log" ""
cp -f /tmp/bobframe.ppm "$OUT/title.ppm" 2>/dev/null
POS=$(python3 - "$OUT/title.ppm" <<'PY'
import sys
f=open(sys.argv[1],'rb'); assert f.readline().strip()==b'P6'
w,h=map(int,f.readline().split()); f.readline(); d=f.read(w*h*3)
# the front-end menu is drawn in yellow; find rows that carry several such pixels
# Pick the row with the STRONGEST yellow-text signal, not the first one that has any: the
# background artwork contains yellow pixels too (the first cut of this gate located a pilot's
# life jacket and reported the game broken). Menu rows carry far more than art does.
best=(0,-1)
for y in range(h):
    n=0
    for x in range(0,w,2):
        i=(y*w+x)*3
        if d[i]>170 and d[i+1]>140 and d[i+2]<100: n+=1
    if n>best[0]: best=(n,y)
if best[0] < 8: print("NONE"); raise SystemExit
y=best[1]
xs=[x for x in range(w) if d[(y*w+x)*3]>170 and d[(y*w+x)*3+1]>140 and d[(y*w+x)*3+2]<100]
# middle of the longest contiguous run on that row = the middle of a menu label
runs=[];st=xs[0];prev=xs[0]
for x in xs[1:]:
    if x-prev>12: runs.append((st,prev)); st=x
    prev=x
runs.append((st,prev))
a,b=max(runs,key=lambda r:r[1]-r[0])
print("%d %d" % ((a+b)//2, y))
PY
)
if [ "$POS" = "NONE" ] || [ -z "$POS" ]; then
  echo "  could not locate the menu in the captured frame — cannot test the click"
  echo "  RESULT: FAIL"; exit 1
fi
CX=${POS% *}; CY=${POS#* }
echo "  menu located at pixel ($CX,$CY)"

# 2. click exactly there and require the game to act on it
run "$OUT/click.log" "60,$CX,$CY"
if grep -aq "OnSelectRlistbox" "$OUT/click.log"; then
  echo "  click accepted: $(grep -a 'OnSelectRlistbox' "$OUT/click.log" | head -1 | sed 's/^\[//')"
  echo "  RESULT: PASS"
  exit 0
fi
echo "  the menu was drawn at ($CX,$CY) but clicking there did nothing"
echo "  RESULT: FAIL — logs in $OUT"
exit 1
