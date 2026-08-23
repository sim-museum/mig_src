#!/usr/bin/env bash
# port/route_drag.sh — gate for EPIC K step 13: DRAG the mission route's waypoints on the campaign
# map. This is the first press-move-release interaction in the port; every click it has learned so
# far is press-and-release in one place.
#
# The PO's script, step 13: "Route: drag Egress inland (target is close to the front line, go home
# direct); drag the IP to within 4 miles of the target so it's visible the instant you drop out of
# accel mode and padlock works immediately; drag the two AAA waypoints over the target area."
#
# WHAT IT ASSERTS, and why each is separate:
#   1. the drag ENGAGED               — `dragging=1`. CMapDlg::OnMouseMove starts a drag only once
#                                       the pointer leaves a 3px box around the press point AND
#                                       AllowDragItem() agrees. MaDriveClick issues down+up in one
#                                       tick precisely to keep this FALSE, so a gate that only
#                                       checked "the waypoint moved" could pass on a CLICK, whose
#                                       OnLButtonUp calls OnClickItem instead of OnDragItem.
#   2. the WORLD position changed     — info_waypoint::World is what the flight flies. Screen
#                                       coordinates move with zoom and scroll; world does not.
#   3. the map AGREES afterwards      — a re-scan finds the waypoint near where it was dropped. This
#                                       catches "the data moved but nothing redrew", which assertion
#                                       2 alone cannot see.
#   4. the IP lands within 4 MILES    — the script's own number, measured in the game's own units
#                                       (RANGES.H: METRES250KM = 25000000, i.e. centimetres) AFTER
#                                       OnDragItem clamps the waypoint into the theatre and recalcs
#                                       the route — where it lands is not where it was dropped.
#   5. a NON-waypoint refuses to drag — AllowDragItem() is `uid in [WayPointBAND, WayPointBANDEND)`.
#                                       Dragging the target itself must move nothing. Without this
#                                       a hit-test that dragged whatever was under the cursor would
#                                       pass every assertion above.
#
# NOT asserted here: "the edited route is what the flight flies" in the sense of actually flying it
# — that is K10-K13. What IS asserted is that the edit reached `info_waypoint::World`, the data the
# flight reads.
set -u
export MA_NO_HARDWARE="${MA_NO_HARDWARE:-1}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
. "$ROOT/port/gate_lib.sh"          # S171: assert_no_crash / assert_recipe_ran
assert_clean_start || exit 2          # S177: a stray wmig makes this gate report a content failure
WMIG="${WMIG:-$ROOT/build/wmig}"
BOB_DRIVE_C="${BOB_DRIVE_C:-$HOME/sgl/TUE/MigAlley/WP/drive_c}"
RUNDIR="$BOB_DRIVE_C/rowan/mig"
OUT="${OUT:-/tmp/ma_routedrag}"
TMO="${TMO:-520}"
TARGET="${TARGET:-Wonju}"
MAXMILES="${MAXMILES:-4}"
AUTHORISE=2023; LBFILE=1055
mkdir -p "$OUT"
[ -x "$WMIG" ] || { echo "no binary at $WMIG" >&2; exit 2; }

SAVEFILE="$RUNDIR/SaveGame/Auto Save.sav"
PIN="$ROOT/port/ref/save/campaign_pristine.sav"
STASH="$(mktemp -u /tmp/ma_routedrag_save.XXXXXX)"
[ -f "$SAVEFILE" ] && cp -a "$SAVEFILE" "$STASH"
restore_save() { [ -f "$STASH" ] && cp -a "$STASH" "$SAVEFILE" && rm -f "$STASH"; }
trap restore_save EXIT INT TERM
[ -f "$PIN" ] && cp -f "$PIN" "$SAVEFILE"

log="$OUT/routedrag.log"; ppm="$OUT/routedrag.ppm"; rm -f "$ppm"
# Authorise the mission (`:r0` selects Minimum Strike AND loads it, S171) — the route waypoints do
# not exist until there is a mission. Then drag, then scan again to see where things ended up.
SEQ="30,r3;65,#$LBFILE;100,#2063:1;420,#$AUTHORISE@DossierButtons;520,#$LBFILE@CLoad:r0"
DRAG="900,Initial Point@$TARGET;1000,Egress+220,-60;1100,$TARGET+150,0"
echo "route drag on the \"$TARGET\" mission — wmig"
# NB run this UNDER gl-lock; it does not take the lock itself (nesting deadlocks, S159).
( cd "$RUNDIR" && timeout -k 5 -s KILL "$TMO" env \
    SDL_VIDEODRIVER=dummy BOB_RUN_INIT=1 BOB_DRIVE_C="$BOB_DRIVE_C" MA_DISABLE_3D=1 \
    MA_IGNORE_SAVE_DATE=1 MA_TRACE_OOB=1 MA_TRACE_CLICK=1 \
    MA_MAP_ITEM_SCAN=250,1250 MA_MAP_CLICK_FIRST=1 MA_MAP_CLICK_NAME="$TARGET" \
    MA_MAP_DRAG="$DRAG" BOB_CLICKSEQ="$SEQ" MA_SHOT=1450 MA_SHOT_PATH="$ppm" "$WMIG" ) >"$log" 2>&1
pkill -x "$(basename "$WMIG")" 2>/dev/null

fail=0
assert_no_crash "$log" || fail=1
assert_recipe_ran "$log" || fail=1
if grep -aq "\[mapdrag\] STALLED" "$log"; then
  echo "  $(grep -a '\[mapdrag\] STALLED' "$log" | head -1 | sed 's/^\[mapdrag\] //') — FAIL"; fail=1
fi

check_drag() {   # $1 = waypoint name, $2 = want-dragging (1|0), $3 = want-moved (1|0)
  local name="$1" wantdrag="$2" wantmove="$3" press rel
  press=$(grep -a "\[mapdrag\] press .*\"Waypoint: $name\"" "$log" | head -1)
  [ -z "$press" ] && press=$(grep -a "\[mapdrag\] press .*\"$name" "$log" | head -1)
  if [ -z "$press" ]; then echo "  \"$name\": never pressed — FAIL"; return 1; fi
  local allow; allow=$(echo "$press" | sed -n 's/.*allowdrag=\([01]\).*/\1/p')
  rel=$(grep -a "\[mapdrag\] released" "$log" | sed -n "$(grep -an "\[mapdrag\] press .*$name" "$log" | head -1 >/dev/null; echo 1)p")
  # take the release line that follows this press
  local pn; pn=$(grep -an "\[mapdrag\] press .*$name" "$log" | head -1 | cut -d: -f1)
  rel=$(awk -v n="$pn" 'NR>n && /\[mapdrag\] released/ {print; exit}' "$log")
  if [ -z "$rel" ]; then echo "  \"$name\": pressed but never released — FAIL"; return 1; fi
  local dragging moved
  dragging=$(echo "$rel" | sed -n 's/.*dragging=\([01]\).*/\1/p')
  moved=$(echo "$rel"    | sed -n 's/.*moved=\([01]\).*/\1/p')
  echo "  \"$name\": allowdrag=$allow dragging=$dragging moved=$moved"
  local rc=0
  [ "$allow"    = "$wantdrag" ] || { echo "    expected allowdrag=$wantdrag — FAIL"; rc=1; }
  [ "$dragging" = "$wantdrag" ] || { echo "    expected dragging=$wantdrag — FAIL"; rc=1; }
  [ "$moved"    = "$wantmove" ] || { echo "    expected moved=$wantmove — FAIL"; rc=1; }
  return $rc
}

check_drag "Initial Point" 1 1 || fail=1
check_drag "Egress"        1 1 || fail=1
# 5. the target is not a waypoint: the map must refuse to drag it
check_drag "$TARGET"       0 0 || fail=1

# 4. the separation the drop achieved, in the script's own units
sep=$(grep -a '\[mapdrag\] separation' "$log" | head -1)
if [ -n "$sep" ]; then
  echo "  ${sep#\[mapdrag\] }"
  miles=$(echo "$sep" | sed -n 's/.*(\([0-9.]*\) miles).*/\1/p')
  if [ -n "$miles" ] && awk -v m="$miles" -v x="$MAXMILES" 'BEGIN{exit !(m<=x)}'; then
    echo "  the IP is within $MAXMILES miles of the target: yes"
  else
    echo "  the IP is ${miles:-?} miles from the target — the script asks for <= $MAXMILES — FAIL"; fail=1
  fi
else
  echo "  no separation was measured — FAIL"; fail=1
fi

# 3. the map agrees: after the drags, where does the map itself say the waypoints are?
echo "  the map's own post-drag positions:"
awk '/mapitem\] scan done/{s++} s>=1 && /band=0x0100/' "$log" | sed 's/^\[mapitem\] /    /' | tail -8
for wp in "Initial Point" "Egress"; do
  dest=$(grep -a "\[mapdrag\] entry .*\"$wp\"" "$log" | head -1 | sed -n 's/.*(\([0-9]*\),\([0-9]*\))$/\1 \2/p')
  now=$(awk '/mapitem\] scan done/{s++} s>=1' "$log" | grep -a "Waypoint: $wp" | tail -1 | sed -n 's/^\[mapitem\] (\([0-9]*\),\([0-9]*\)).*/\1 \2/p')
  if [ -n "$dest" ] && [ -n "$now" ]; then
    d=$(awk -v a="$dest" -v b="$now" 'BEGIN{split(a,p," ");split(b,q," ");dx=p[1]-q[1];dy=p[2]-q[2];print int(sqrt(dx*dx+dy*dy))}')
    if [ "$d" -le 40 ]; then echo "  \"$wp\" redrew ${d}px from where it was dropped: yes"
    else echo "  \"$wp\" redrew ${d}px from where it was dropped — too far, the map did not follow — FAIL"; fail=1; fi
  else
    echo "  \"$wp\" could not be located after the drag — FAIL"; fail=1
  fi
done

echo "----------------------------------------"
if [ "$fail" -eq 0 ]; then
  echo "PASS: EPIC K step 13 — route waypoints drag, the world position follows, and non-waypoints refuse"
else
  echo "FAIL"; exit 1
fi
