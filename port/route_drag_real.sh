#!/usr/bin/env bash
# port/route_drag_real.sh — can a PLAYER drag a route waypoint?
#
# WHY THIS EXISTS (S190, PO-55)
# ----------------------------
# `route_drag.sh` has been green since S172 and the feature was dead the whole time. The PO
# reported "can't drag the ocean waypoint, or any other waypoint icon" and was exactly right: no
# waypoint has EVER been draggable in this port. Every map click was delivered as down+up in a
# single tick (CMapDlg::MaDriveClick) — deliberately, to dodge an unported GetDC() inside
# OnMouseMove — and S96 then DISCARDED any release more than 4px from its press. A drag reached
# the game as nothing at all. The PO's session log contains one line for the whole gesture:
#     [click] release 958,594 suppressed: dragged 61 px from press
#
# route_drag.sh could not have caught this BY CONSTRUCTION, for two independent reasons:
#   1. it drives MA_MAP_DRAG -> CMapDlg::MaDriveDrag, a test-only entry point that calls
#      OnMouseMove directly, bypassing every layer between SDL and the map; and
#   2. it runs under SDL_VIDEODRIVER=dummy, where SDL_CreateWindow fails, so pump_events returns
#      early and NO SDL event is ever processed. Even a correct hook could not have worked there.
# It proves the engine's drag ARITHMETIC. This gate proves a HUMAN can drag. Both are worth having,
# and this one needs a REAL GL DISPLAY.
#
# WHAT IT ASSERTS
#   1. the press REACHED the map          — [mapdrag] press ... allowdrag=1, from a real SDL event
#   2. the drag ENGAGED                   — wasdragging=1. OnLButtonUp calls OnDragItem only then;
#                                           a click takes OnClickItem and opens a dossier instead,
#                                           and "the waypoint moved" alone cannot tell them apart
#   3. the WORLD position CHANGED         — info_waypoint::World is what the flight flies. Screen
#                                           coordinates move with zoom and scroll; world does not
#   4. nothing crashed, and the recipe ran to completion
#
# NB run this UNDER gl-lock; it does not take the lock itself (nesting deadlocks, S159).
set -u
export MA_NO_HARDWARE="${MA_NO_HARDWARE:-1}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
. "$ROOT/port/gate_lib.sh"
assert_clean_start || exit 2
WMIG="${WMIG:-$ROOT/build/wmig}"
BOB_DRIVE_C="${BOB_DRIVE_C:-$HOME/sgl/TUE/MigAlley/WP/drive_c}"
RUNDIR="$BOB_DRIVE_C/rowan/mig"
OUT="${OUT:-/tmp/ma_routedrag_real}"
TMO="${TMO:-520}"
TARGET="${TARGET:-Wonju}"
AUTHORISE=2023; LBFILE=1055
mkdir -p "$OUT"
[ -x "$WMIG" ] || { echo "no binary at $WMIG" >&2; exit 2; }

# The campaign save is restored afterwards: this gate authorises a mission and edits its route.
SAVEFILE="$RUNDIR/SaveGame/Auto Save.sav"
PIN="$ROOT/port/ref/save/campaign_pristine.sav"
STASH="$(mktemp -u /tmp/ma_routedragreal_save.XXXXXX)"
[ -f "$SAVEFILE" ] && cp -a "$SAVEFILE" "$STASH"
restore_save() { [ -f "$STASH" ] && cp -a "$STASH" "$SAVEFILE" && rm -f "$STASH"; }
trap restore_save EXIT INT TERM
[ -f "$PIN" ] && cp -f "$PIN" "$SAVEFILE"

log="$OUT/routedragreal.log"; rm -f "$log"
SEQ="30,r3;65,#$LBFILE;100,#2063:1;420,#$AUTHORISE@DossierButtons;520,#$LBFILE@CLoad:r0"
# One drag, named not hardcoded (S63). Egress is the waypoint the PO could not move.
DRAG="900,Egress+220,-60"
echo "route drag through REAL SDL events — \"$TARGET\" mission"
( cd "$RUNDIR" && timeout -k 5 -s KILL "$TMO" env \
    BOB_RUN_INIT=1 BOB_DRIVE_C="$BOB_DRIVE_C" MA_DISABLE_3D=1 \
    MA_IGNORE_SAVE_DATE=1 MA_TRACE_CLICK=1 \
    MA_MAP_ITEM_SCAN=250,1250 MA_MAP_CLICK_FIRST=1 MA_MAP_CLICK_NAME="$TARGET" \
    MA_MAP_DRAG_REAL="$DRAG" BOB_CLICKSEQ="$SEQ" "$WMIG" ) >"$log" 2>&1
for p in $(pgrep -x "$(basename "$WMIG")" 2>/dev/null); do kill "$p" 2>/dev/null; done

fail=0
assert_no_crash "$log" || fail=1
assert_recipe_ran "$log" || fail=1

if grep -aq "\[dragreal\] STALLED" "$log"; then
  echo "  $(grep -a '\[dragreal\] STALLED' "$log" | head -1 | sed 's/^\[dragreal\] //') — FAIL"; fail=1
fi

entry=$(grep -a "^\[dragreal\] entry" "$log" | head -1)
if [ -z "$entry" ]; then
  echo "  the drag never started (no [dragreal] entry line) — FAIL"; fail=1
else
  echo "  ${entry#\[dragreal\] }"
fi

press=$(grep -a "^\[mapdrag\] press" "$log" | head -1)
if [ -z "$press" ]; then
  echo "  the press never reached the map — FAIL"
  echo "    (this is the S189 defect: SDL saw the drag, the map never did)"
  fail=1
else
  echo "  ${press#\[mapdrag\] }"
  case "$press" in *"allowdrag=1"*) ;; *) echo "    the press did not land on a draggable item — FAIL"; fail=1;; esac
fi

rel=$(grep -a "^\[mapdrag\] release" "$log" | head -1)
if [ -z "$rel" ]; then
  echo "  no release was delivered — FAIL"; fail=1
else
  echo "  ${rel#\[mapdrag\] }"
  case "$rel" in
    *"wasdragging=1"*) ;;
    *) echo "    the gesture never ENGAGED a drag (it was treated as a click) — FAIL"; fail=1;;
  esac
  case "$rel" in
    *"moved=1"*) ;;
    *) echo "    the waypoint's WORLD position did not change — FAIL"; fail=1;;
  esac
fi

echo "  ----------------------------------------"
if [ "$fail" -eq 0 ]; then
  echo "  PASS: a real SDL drag moves a route waypoint (log $log)"; exit 0
fi
echo "  FAIL: see $log"; exit 1
