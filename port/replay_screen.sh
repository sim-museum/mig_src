#!/usr/bin/env bash
# port/replay_screen.sh -- S203 (PO-63): the title menu's LOWER rows are reachable by recipe,
# and the Replay screen they lead to is live.
#
# WHY THIS EXISTS
# ---------------
# The title menu listbox is 105x100 and draws SEVEN rows of 28px = 199px. Measured off the
# capture: ink runs at y=215,238,266,294,322,350,378 for a control at y=210 h=100. Nothing clips
# it (Windows clips a child to its parent window; this path does not) and the GOLD title screen
# shows the whole list too, so drawing all seven is CORRECT. But every listbox hit test bounded
# the click by m_maH, so rows 4-6 were painted and could never be clicked by any route.
#
#   row 0 Preferences   1 Single Player   2 Multi-Player   3 Load Game
#   row 4 REPLAY        5 Credits         6 Quit
#
# Row 4 is Replay -- which is exactly why PO-61 (the PO's replay crash) had no gate and no
# headless repro: the whole _Replay subsystem sat behind a row no recipe could address. S183
# recorded this as PO-63 and its diagnosis was right; the fix is to hit-test the height PAINT
# covered (Hosted::drawH, recorded by ma_ole_draw_all from the control's own GetListHeight) rather
# than the template rect. It can only WIDEN what accepts a click, so it cannot move a pixel --
# port/parity_2d.sh is the check for that and stays 5/5.
#
# ⚠️ NEGATIVE CONTROL IS BUILT IN. MA_NO_DRAWH=1 reverts to bounding by m_maH, and the gate then
# has to go RED. A gate nobody has watched fail is indistinguishable from a gate that cannot fail
# -- BoB's flagship campaign gate was unpassable from the day it was written and green for nine
# sprints because nobody ever saw it red (BoB S206, cross-port note §8-BoB206). Run both arms.
#
# Both assertions key on evidence THIS RECIPE emits (the other half of §8-BoB206: an assertion
# keyed on a trace the recipe does not switch on can never pass). MA_TRACE_CLICK is set below and
# both lines below come from it.
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WMIG="${WMIG:-$ROOT/build/wmig}"
BOB_DRIVE_C="${BOB_DRIVE_C:-$HOME/sgl/TUE/MigAlley/WP/drive_c}"
RUNDIR="$BOB_DRIVE_C/rowan/mig"
OUT="${OUT:-/tmp/ma_replay_screen}"
TMO="${TMO:-120}"
CONTROL="${CONTROL:-0}"
mkdir -p "$OUT"
[ -x "$WMIG" ] || { echo "no binary at $WMIG" >&2; exit 2; }
if pgrep -x wmig >/dev/null 2>&1; then
  echo "  REFUSING TO RUN: wmig is already running (pid $(pgrep -x wmig | tr '\n' ' ')) -- S177."
  exit 2
fi

log="$OUT/replay.log"
NODRAWH=""
[ "$CONTROL" = "1" ] && NODRAWH="MA_NO_DRAWH=1"
echo "replay screen -- title menu row 4 -> CLoad$([ "$CONTROL" = 1 ] && echo '   [NEGATIVE CONTROL: MA_NO_DRAWH=1, must go RED]')"
( cd "$RUNDIR" && timeout -k 5 -s KILL "$TMO" env \
    SDL_VIDEODRIVER=dummy BOB_RUN_INIT=1 BOB_DRIVE_C="$BOB_DRIVE_C" \
    MA_DISABLE_3D=1 MA_NO_HARDWARE=1 MA_TRACE_CLICK=1 $NODRAWH \
    BOB_CLICKSEQ="30,r4;60,#1055:r0" \
    MA_SHOT=100 MA_SHOT_PATH="$OUT/replay.ppm" \
    "$WMIG" ) >"$log" 2>&1
for p in $(pgrep -x wmig); do kill -9 "$p" 2>/dev/null; done

fail=0
say() { printf '  %-46s %s\n' "$1" "$2"; }

# 0. IS THERE ANYTHING TO TEST? The overflow this gate exists for depends on the live window
#    resolution (which comes from settings.mig -- S103 made preferences really load, so the menu's
#    geometry follows the player's chosen mode). If the list happens to FIT its rect, rows 4-6 are
#    reachable without the fix and every assertion below passes for the wrong reason. Say so
#    loudly rather than bank a vacuous PASS: a gate that cannot fail is the thing this project
#    keeps being bitten by (§8-BoB206). Deliberately NOT a hard failure -- fitting is not a defect.
geom=$(grep -a "\[click\] listbox id=2063 " "$log" | head -1)
rh=$(echo "$geom" | sed -n 's/.*rect=([0-9]*,[0-9]*,[0-9]*,\([0-9]*\)).*/\1/p')
dh=$(echo "$geom" | sed -n 's/.*drawH=\([0-9-]*\).*/\1/p')
if [ -n "${rh:-}" ] && [ -n "${dh:-}" ] && [ "$dh" -gt "$rh" ] 2>/dev/null; then
  say "menu overflows its rect (the PO-63 condition)" "yes -- draws ${dh}px in a ${rh}px control"
else
  say "menu overflows its rect (the PO-63 condition)" "NO (rect=${rh:-?} drawH=${dh:-?})"
  echo "     ^ at this resolution the menu FITS, so rows 4-6 are reachable without the fix and"
  echo "       the assertions below prove nothing about PO-63. Re-run at the resolution that"
  echo "       reproduces it, or treat this run as inconclusive rather than as a pass."
fi

# 1. the title menu's row 4 is actually accepted (this is PO-63 itself)
if grep -aq "\[click\] listbox id=2063 .*HIT" "$log"; then
  say "title menu row 4 accepted" "yes -- $(grep -a '\[click\] listbox id=2063' "$log" | head -1 | sed 's/.*drawH=/drawH=/')"
else
  say "title menu row 4 accepted" "NO -- FAIL"; fail=1
fi

# 2. the Replay screen is live: its own file list took a click and named a row.
#    CLoad's list is id 1055; a row-0 hit proves the screen is up AND populated, which a
#    screenshot alone cannot (a painted-but-inert list looks identical -- S82).
if grep -aq "\[click\] file-listbox id=1055 .*row=0" "$log"; then
  say "Replay screen live (CLoad list row 0)" "yes"
else
  say "Replay screen live (CLoad list row 0)" "NO -- FAIL"; fail=1
fi

# 3. safety
if grep -aq "Segmentation\|=== CRASH" "$log"; then say "no crash" "CRASHED -- FAIL"; fail=1
else say "no crash" "yes"; fi

echo "----------------------------------------"
if [ "$CONTROL" = "1" ]; then
  if [ "$fail" -ne 0 ]; then echo "CONTROL OK: with MA_NO_DRAWH=1 the row is unreachable again (the PO-63 bug)"; exit 0
  else echo "CONTROL BROKEN: passed with the fix reverted -- this gate is not testing PO-63"; exit 1; fi
fi
if [ "$fail" -eq 0 ]; then echo "PASS: menu row 4 reaches the Replay screen and its file list is live"
else echo "FAIL"; exit 1; fi
