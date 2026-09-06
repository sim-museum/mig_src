#!/usr/bin/env bash
# GATE (MP-2, S21): "Select sides is missing" -- the locker room's SELECT SIDE radio appears once a
# game type other than Death Match is chosen, because a click on the GAME TYPE radio now reaches
# the control and fires its Selected event (CLockerRoom::OnSelectedRradioGametype -> RedrawSide).
#
# Root cause (2026-09-06): ma_ole_click's TYPE FILTER had no CT_RADIO -- the radio click arm lived
# only in the toolbar dispatcher -- so every radio on a full-screen panel was drawn and inert.
# The side selector is hidden under Death Match BY DESIGN and can only be shown by that event.
#
# HEADLESS (SDL dummy; 2-D front end). Two arms:
#   1. the fix: click Quick Missions (#2323 centre row), then Red (#2324 centre row);
#      the side radio must be drawn afterwards and both clicks must register as hits.
#   2. NEGATIVE CONTROL: MA_NO_RADIO_CLICK=1 restores the old filter; the side radio must
#      then never draw and no radio click may register. Without this arm a gate that
#      counted draws could pass on a screen that happened to show the radio for another reason.
set -u
ROOT="/home/admin/ma"
WMIG="${WMIG:-$ROOT/build/wmig}"
DRIVE_C="${BOB_DRIVE_C:-$HOME/sgl/TUE/MigAlley/WP/drive_c}"
RUNDIR="$DRIVE_C/rowan/mig"
OUT="${OUT:-/tmp/ma_mp_sideselect}"; mkdir -p "$OUT"
TMO="${TMO:-120}"
SEQ="40,r2;90,#2063:1;200,#2323:2;300,#2324:1"

. "$ROOT/port/gate_lib.sh" 2>/dev/null || true
if command -v assert_clean_start >/dev/null 2>&1; then
  assert_clean_start || exit 2
elif pgrep -x "$(basename "$WMIG")" >/dev/null 2>&1; then
  echo "  REFUSING TO RUN: a wmig is already up -- it may be the player's own game."; exit 2
fi

fail=0
say(){ printf '  %-52s %s\n' "$1" "$2"; }
run_arm() { # label extra-env log
  ( cd "$RUNDIR" && timeout -k 5 -s KILL "$TMO" env \
      SDL_VIDEODRIVER=dummy BOB_RUN_INIT=1 BOB_DRIVE_C="$DRIVE_C" MA_DISABLE_3D=1 MA_MAXIMIZE=0 \
      MA_NO_INTRO=1 MA_TRACE_RADIO=1 MA_TRACE_CLICK=1 BOB_CLICKSEQ="$SEQ" \
      MA_SHOT=420 MA_SHOT_PATH="$OUT/$1.ppm" ${2:+$2} "$WMIG" ) >"$3" 2>&1
}

echo "MP-2 -- the locker room shows SELECT SIDE once a game type is chosen"
LOG="$OUT/fix.log"; run_arm fix "" "$LOG"
command -v assert_no_crash >/dev/null 2>&1 && { assert_no_crash "$LOG" || fail=1; }
grep -aq '\[radioclick\] front-end id=2323 .*hit=1' "$LOG" && say "GAME TYPE click reaches the radio (Selected fires)" "yes" \
   || { say "GAME TYPE click reaches the radio (Selected fires)" "NO"; fail=1; }
n=$(grep -a -c 'radiodraw\] panel id=2324' "$LOG")
[ "$n" -gt 20 ] && say "SELECT SIDE radio drawn afterwards (frames)" "$n" \
   || { say "SELECT SIDE radio drawn afterwards (frames)" "$n -- NOT SHOWN"; fail=1; }
grep -aq '\[radioclick\] front-end id=2324 .*hit=1 sel=1' "$LOG" && say "clicking Red on SELECT SIDE registers (sel=1)" "yes" \
   || { say "clicking Red on SELECT SIDE registers (sel=1)" "NO"; fail=1; }
grep -aq 'radiofilter\] id=2324 .*vis=1' "$LOG" && say "side radio window is visible (vis=1)" "yes" \
   || { say "side radio window is visible (vis=1)" "NO"; fail=1; }

echo "  --- negative control: MA_NO_RADIO_CLICK=1 (the old type filter) ---"
NLOG="$OUT/control.log"; run_arm control "MA_NO_RADIO_CLICK=1" "$NLOG"
if grep -aq '\[radioclick\] front-end' "$NLOG"; then
  say "control must NOT register a radio click" "IT DID -- the arm proves nothing"; fail=1
else say "control registers no radio click" "correct"; fi
m=$(grep -a -c 'radiodraw\] panel id=2324' "$NLOG")
[ "$m" -eq 0 ] && say "control never shows SELECT SIDE (frames)" "$m" \
   || { say "control shows SELECT SIDE anyway (frames)" "$m -- the draw is not click-gated"; fail=1; }

echo "----------------------------------------"
if [ "$fail" -eq 0 ]; then echo "PASS: SELECT SIDE appears from a real click, and only from one"; exit 0; fi
echo "FAIL (logs in $OUT)"; exit 1
