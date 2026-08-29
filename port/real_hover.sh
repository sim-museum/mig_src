#!/usr/bin/env bash
# port/real_hover.sh — S326: does the HIGHLIGHT follow the real pointer?
#
# WHY THIS EXISTS
# ---------------
# S325 flipped MA_MAXIMIZE on by default. The maximised suite went 12/12 and parity_2d stayed
# 5/5 byte-identical -- and the PO immediately hit a regression the whole suite was blind to:
#   "hovering the mouse causes the wrong menu item to highlight, you have to keep clicking on
#    the desired menu item several times before it activates"
#
# ma_ole_mouse was the ONE hit-test S317/S318 never routed through ma_ole_origin. At 800x600 the
# panel origin is (0,0), so the omission was invisible; maximised it is (320,28) and the highlight
# tracked a point ~320 px from the cursor.
#
# The suite could not have caught it, for a structural reason worth stating plainly:
# EVERY existing click gate tests CLICKING. panel_click, maximized_nav, help_click, map_icon_click
# -- all of them synthesise a press. Not one of them MOVES a cursor and asks what lit up. Hover was
# not merely untested, it was UNOBSERVABLE: both [ole_mouse] and [ole_click] traces sit behind
# `clicked`, so the single path carrying the bug printed nothing at all. S326 added [hover] for
# exactly this gate.
#
# WHAT IT ASSERTS -- an INVARIANT, not a pixel table
#   For a set of real pointer positions spanning the title menu's listbox:
#     1. every position resolves to a row (the point is inside the control's rect)
#     2. the row INCREASES MONOTONICALLY as the pointer moves down
#     3. hovering the reported centre of row k yields row k
#     4. HOVER AND CLICK AGREE at the same point -- the exact disagreement the PO reported
# Row geometry is read back out of the [hover] trace's own rect, so nothing is hardcoded to a
# resolution: the same assertions run at 800x600 and maximised, which is the point.
#
# NEGATIVE CONTROL -- DISCRIMINATING (unlike real_mouse.sh's, which is honestly inconclusive)
# CONTROL=1 sets MA_NO_HOVERORG=1, which restores the EXACT pre-S326 line (raw m_maX/m_maY with
# no origin resolution), so assertion 3 must FAIL. A green control means the gate proves nothing and
# must be treated as broken, not as a pass.
# The first control used MA_NO_DRAWORG=1 and came up GREEN: that flag only selects a different branch
# inside ma_ole_origin, whose fallback still resolves to the right origin. It disabled something
# ADJACENT to the fix rather than the fix, and the gate caught itself.
#
# WARNING: needs a real display AND the pointer. It MOVES the mouse.
# CLICK DELIVERY: XSendEvent, not XTEST. On this desktop `xdotool click` (XTEST) delivers ZERO
# button events to the app -- measured: 1636 SDL motion events and 0 button events in 22 s -- while
# `xdotool click --window` (XSendEvent) delivers them (`[btn] SDL DOWN/UP button=1`). Motion goes
# through either way, which is what made this look like a port bug: real_mouse.sh went red and the
# obvious reading was "clicks are broken". They are not. The click still travels SDL queue ->
# pump_events -> win_to_canvas -> hit test, which is the layer these gates exist to cover.
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WMIG="${WMIG:-$ROOT/build/wmig}"
BOB_DRIVE_C="${BOB_DRIVE_C:-$HOME/sgl/TUE/MigAlley/WP/drive_c}"
RUNDIR="$BOB_DRIVE_C/rowan/mig"
OUT="${OUT:-/tmp/ma_real_hover}"
SETTLE="${SETTLE:-22}"
CONTROL="${CONTROL:-0}"
MAXIM="${MAXIM:-1}"
mkdir -p "$OUT"
[ -x "$WMIG" ] || { echo "no binary at $WMIG" >&2; exit 2; }
command -v xdotool >/dev/null || { echo "xdotool not installed -- this gate cannot run" >&2; exit 2; }

# COURTESY GUARD: this gate seizes the pointer. If a human is moving it, do not fight them --
# exit 2 (inconclusive) rather than yanking the cursor out from under someone mid-click.
if [ "${HOVER_FORCE:-0}" != 1 ]; then
  P0=$(xdotool getmouselocation --shell 2>/dev/null | head -2 | tr '\n' ' ')
  sleep 5
  P1=$(xdotool getmouselocation --shell 2>/dev/null | head -2 | tr '\n' ' ')
  [ "$P0" = "$P1" ] || { echo "pointer is in use by a human -- refusing to grab it (HOVER_FORCE=1 to override)" >&2; exit 2; }
fi

LOG="$OUT/hover.log"
: > "$LOG"
cd "$RUNDIR" || exit 2

CTLENV=""; [ "$CONTROL" = 1 ] && CTLENV="MA_NO_HOVERORG=1"
env MA_MAXIMIZE=$MAXIM MA_TRACE_HOVER=1 $CTLENV "$WMIG" >"$LOG" 2>&1 &
PID=$!
trap 'kill -9 $PID 2>/dev/null' EXIT
sleep "$SETTLE"

WID=$(xdotool search --pid $PID --name . 2>/dev/null | tail -1)
[ -n "$WID" ] || { echo "no window for pid $PID -- cannot run" >&2; exit 2; }
eval "$(xdotool getwindowgeometry --shell "$WID")"
echo "window $WID at ${X},${Y} ${WIDTH}x${HEIGHT}  (MA_MAXIMIZE=$MAXIM CONTROL=$CONTROL)"

# ---- phase 1: DISCOVER the listbox by sweeping, rather than hardcoding a rect -------------------
# A coarse grid finds where [hover] fires at all; the trace reports the control's own org/size, so
# the row geometry below is derived from the port's answer, not from a table that would silently
# rot the next time a layout changes.
: > "$OUT/sweep.txt"
for fx in $(seq 4 4 96); do
  for fy in $(seq 4 4 96); do
    xdotool mousemove --sync $((X + WIDTH*fx/100)) $((Y + HEIGHT*fy/100)) 2>/dev/null
  done
done
sleep 2
grep -a '^\[hover\]' "$LOG" > "$OUT/sweep.txt" || true
NSW=$(wc -l < "$OUT/sweep.txt")
echo "  sweep: $NSW hover samples"
if [ "$NSW" -eq 0 ]; then
  echo "FAIL: ma_ole_mouse never ran -- the hover call site is not reached at all" ; exit 1
fi

# EVERY distinct rect the sweep found -- not just the commonest one. The first cut took the MODAL
# org/size, which made this gate NON-DETERMINISTIC: the front end hosts more than one listbox, the
# sweep meets them in whatever order the pointer wanders, and one run locked onto (1130,398) while
# the next locked onto (810,370). The control arm then measured a listbox whose origin the fix does
# not affect, passed, and the gate declared itself broken -- correctly, but the reason only showed
# up in a side-by-side origin diff of the two arms' logs.
RECTS=$(sed -n 's/^\[hover\] org=(\([0-9-]*\),\([0-9-]*\)) draw=([0-9-]*,[0-9-]*) size=(\([0-9-]*\),\([0-9-]*\)).*/\1 \2 \3 \4/p' \
        "$OUT/sweep.txt" | sort -u)
NRECT=$(echo "$RECTS" | grep -c .)
echo "  $NRECT distinct hosted rect(s):"; echo "$RECTS" | sed 's/^/      /'

# ---- phase 2: assert the four invariants ON EVERY RECT ------------------------------------------
FAILED=0
while read -r OX OY OW OH; do
  [ -n "${OX:-}" ] || continue
  [ "${OW:-0}" -gt 0 ] && [ "${OH:-0}" -gt 0 ] || continue
  echo "  --- rect ($OX,$OY) ${OW}x${OH} ---"
  # 0. THE ONE ASSERTION THAT IS NOT SELF-REFERENTIAL. Everything below probes points derived from
  # the hit-test origin, so it stays consistent no matter where that origin is -- which is why the
  # first version of this gate passed in both arms (PO-80). drawOx/drawOy are recorded by PAINT,
  # so they are an independent fact: if the hit test disagrees with where the control was drawn,
  # the player hovers one item and a different one lights up. That is the PO's actual report.
  # take the first sample where the control HAS been painted. drawOx is -1 until the first paint
  # records it (S84), and the sweep's earliest samples can predate that -- taking head -1 reported
  # "never painted" for a control the fixed arm demonstrably resolves THROUGH drawOx.
  DRAW=$(grep -a "^\[hover\] org=($OX,$OY) " "$OUT/sweep.txt" | grep -v "draw=(-1,-1)" | head -1 | grep -oE "draw=\(-?[0-9]+,-?[0-9]+\)" | tr -d 'draw=()')
  DX=${DRAW%,*}; DY=${DRAW#*,}
  if [ "${DX:--1}" = "-1" ]; then
    # SKIP, do not test. A rect with no painted sample is the PRE-PAINT TRANSIENT of a control:
    # drawOx is -1 until the first paint records it (S84), so ma_ole_origin falls back to the raw
    # origin and the sweep briefly sees the control at a second position. The fixed arm reported
    # (1130,398) AND (810,370) for one listbox for exactly this reason, and probing the phantom
    # failed assertion 2 -- a FAIL on a rect no user can ever hover. What was never painted was
    # never on screen.
    echo "    -- skipped: never painted (pre-paint transient, not a real control)"
    continue
  elif [ "$DX" = "$OX" ] && [ "$DY" = "$OY" ]; then
    echo "    0. hit-test origin == DRAW origin            yes ($DX,$DY)"
  else
    echo "    0. hit-test origin == DRAW origin            NO  <-- hit=($OX,$OY) drawn=($DX,$DY)"
    echo "       the highlight tracks a point $((OX-DX)),$((OY-DY)) px from the cursor"
    FAILED=1; continue
  fi
  : > "$OUT/probe.txt"
  CX=$((OX + OW/2))
  for i in $(seq 0 11); do
    PY=$((OY + OH*i/12 + OH/24))
    WM=$(grep -ac '^\[hover\]' "$LOG")
    xdotool mousemove --sync $((X + CX)) $((Y + PY)) 2>/dev/null
    sleep 0.25
    R=$(grep -a '^\[hover\]' "$LOG" | tail -n +$((WM+1)) | tail -1 | sed -n 's/.*-> row=\([0-9-]*\).*/\1/p')
    echo "$PY ${R:-NONE}" >> "$OUT/probe.txt"
  done
  if grep -q NONE "$OUT/probe.txt"; then
    echo "    1. every probe resolves                    NO"; FAILED=1; continue; fi
  echo "    1. every probe resolves                    yes"
  if ! awk '{if($2<p)exit 1;p=$2}' "$OUT/probe.txt"; then
    echo "    2. row increases monotonically             NO"; FAILED=1; continue; fi
  echo "    2. row increases monotonically             yes"
  BAD=0
  for R in $(awk '{print $2}' "$OUT/probe.txt" | sort -nu); do
    YS=$(awk -v r=$R '$2==r{print $1}' "$OUT/probe.txt")
    LO=$(echo "$YS" | head -1); HI=$(echo "$YS" | tail -1); MID=$(( (LO+HI)/2 ))
    WM=$(grep -ac '^\[hover\]' "$LOG")
    xdotool mousemove --sync $((X + CX)) $((Y + MID)) 2>/dev/null; sleep 0.25
    G=$(grep -a '^\[hover\]' "$LOG" | tail -n +$((WM+1)) | tail -1 | sed -n 's/.*-> row=\([0-9-]*\).*/\1/p')
    [ "$G" = "$R" ] || { echo "       row $R centre y=$MID lit row ${G:-NONE}"; BAD=1; }
  done
  if [ $BAD != 0 ]; then
    echo "    3. row k's centre lights row k             NO  <-- WRONG ITEM HIGHLIGHTS"; FAILED=1; continue; fi
  echo "    3. row k's centre lights row k             yes"
  MY=$((OY + OH/2))
  WM=$(grep -ac '^\[hover\]' "$LOG")
  xdotool mousemove --sync $((X + CX)) $((Y + MY)) 2>/dev/null; sleep 0.4
  HR=$(grep -a '^\[hover\]' "$LOG" | tail -n +$((WM+1)) | grep -a move | tail -1 | sed -n 's/.*-> row=\([0-9-]*\).*/\1/p')
  WM=$(grep -ac '^\[hover\]' "$LOG")
  xdotool click --window "$WID" 1 2>/dev/null; sleep 0.6
  CR=$(grep -a '^\[hover\]' "$LOG" | tail -n +$((WM+1)) | grep -a CLICK | tail -1 | sed -n 's/.*-> row=\([0-9-]*\).*/\1/p')
  if [ -n "$HR" ] && [ "$HR" = "$CR" ]; then
    echo "    4. hover and click agree                   yes (both row $HR)"
  else
    echo "    4. hover and click agree                   NO  <-- hover=${HR:-NONE} click=${CR:-NONE}"; FAILED=1
  fi
done <<< "$RECTS"

echo "----------------------------------------"
if [ "$CONTROL" = 1 ]; then
  # The control MUST fail. A green control means the gate is not measuring what it claims.
  if [ $FAILED = 0 ]; then
    echo "CONTROL PASSED -- THE GATE IS BROKEN, not the port."
    echo "  MA_NO_HOVERORG=1 restores the exact pre-S326 line; assertion 3 must break on the title menu."
    exit 1
  fi
  echo "PASS(control): the fault is detected when the origin fix is disabled"
  exit 0
fi
[ $FAILED = 0 ] && { echo "PASS: the highlight follows the real pointer"; exit 0; }
echo "FAIL: the highlight does not follow the real pointer"; exit 1
