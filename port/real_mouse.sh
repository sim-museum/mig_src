#!/usr/bin/env bash
# port/real_mouse.sh — S215: drive the REAL SDL mouse path, end to end.
#
# WHY THIS EXISTS
# ---------------
# S209 changed the present viewport to follow the drawable and left the click mapping dividing by
# the size the game had merely REQUESTED. The picture moved; the mouse did not follow it. The PO hit
# it in one click ("main screen, clicking on single player has no effect") and **every gate in this
# suite stayed green** -- parity_2d was 5/5 byte-identical through the whole regression.
#
# It could not have caught it. S192 found 17 of 19 gates never pump an SDL event, and every click
# recipe (BOB_CLICKSEQ `,rN` / `#ID`) resolves a CANVAS coordinate and injects it BELOW
# win_to_canvas -- i.e. below the exact layer that broke. A whole class of regression is invisible
# to the entire suite, by construction.
#
# This gate closes that hole the only way available: xdotool moves the real pointer and presses the
# real button on the real window, so the click travels
#     compositor -> SDL_MOUSEBUTTONDOWN -> pump_events -> win_to_canvas -> hit test -> handler
# with nothing synthesised. That is the "shallow scaffold" ideal from doc/scaffold-audit.md, and
# the first end-to-end exercise of that path in this port's history.
#
# WHAT IT ASSERTS
#   1. the window exists and its geometry is readable (else the coordinates below mean nothing)
#   2. a real click on the title menu's row 1 is RECEIVED at the right canvas point
#   3. it RESOLVES to row 1 -- the mapping is correct, not merely non-crashing
#   4. the screen actually ADVANCES: a second real click lands in the Single Player submenu, whose
#      listbox has a different rect (213x143, 5 rows) from the title menu's (105x100, 7 rows)
#
# Assertion 4 is the one that makes this a gate rather than a trace-watcher: a click that is
# received and mapped but changes nothing would satisfy 2 and 3.
#
# ⚠️ NEGATIVE CONTROL -- HONESTLY INCONCLUSIVE ON THIS MACHINE, AND THAT IS RECORDED, NOT HIDDEN.
# CONTROL=1 sets MA_VIEWPORT_SCRWH=1 (the pre-S209b viewport) and then tries to force the mismatch
# by resizing the window with xdotool. Both attempts to make it discriminate failed, for a reason
# worth knowing:
#   * with no resize, canvas == window, so the old mapping is EXACT and the arm proves nothing;
#   * with a resize, THE PORT UNDOES IT -- ensure_window re-asserts SDL_SetWindowSize every frame,
#     so the window snaps back to the canvas size within a frame or two.
# So the port itself prevents the steady-state mismatch. The bug S209b fixed lived in the TRANSITION
# (g_scrW/g_scrH are assigned immediately while the real resize is deferred, declined or clamped),
# which is transient by nature -- and that is exactly why it was invisible to everything.
# The gate therefore exits 2 (INCONCLUSIVE) in control mode rather than reporting a pass it has not
# earned. Its value is the PASS arm: it is the ONLY test in this suite that drives the real SDL
# mouse path at all. Do not read a green control here as "the mapping is protected"; read it as
# "this arm could not create the fault".
#
# ⚠️ Needs a real display AND the pointer. It MOVES the mouse. Do not run it while using the machine.
# CLICK DELIVERY: XSendEvent, not XTEST. On this desktop `xdotool click` (XTEST) delivers ZERO
# button events to the app -- measured: 1636 SDL motion events and 0 button events in 22 s -- while
# `xdotool click --window` (XSendEvent) delivers them (`[btn] SDL DOWN/UP button=1`). Motion goes
# through either way, which is what made this look like a port bug: real_mouse.sh went red and the
# obvious reading was "clicks are broken". They are not. The click still travels SDL queue ->
# pump_events -> win_to_canvas -> hit test, which is the layer these gates exist to cover.
# GEOMETRY IS PINNED TO 800x600 (MA_MAXIMIZE=0), DELIBERATELY. This gate asserts on literal rects
# -- `[ole_mouse] listbox rect=(530,210,105,100)` and a hardcoded CX=582 -- which are the 800x600
# layout. S325 flipped MA_MAXIMIZE ON by default, the window became 1920x1080, the title listbox
# moved to (1130,398) and every one of those literals stopped matching. The gate went red and read
# exactly like "real clicks are broken"; the port was fine. Third casualty of that flip, after the
# hover origin (S326) and this harness's click method.
# Maximised coverage belongs to port/real_hover.sh, which derives the rect from the port's own
# trace instead of a table and so cannot rot this way. Pass MA_MAXIMIZE=1 to override.
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WMIG="${WMIG:-$ROOT/build/wmig}"
BOB_DRIVE_C="${BOB_DRIVE_C:-$HOME/sgl/TUE/MigAlley/WP/drive_c}"
RUNDIR="$BOB_DRIVE_C/rowan/mig"
OUT="${OUT:-/tmp/ma_real_mouse}"
SETTLE="${SETTLE:-22}"
CONTROL="${CONTROL:-0}"
mkdir -p "$OUT"
[ -x "$WMIG" ] || { echo "no binary at $WMIG" >&2; exit 2; }
command -v xdotool >/dev/null || { echo "xdotool not installed -- this gate cannot run" >&2; exit 2; }

. "$ROOT/port/gate_lib.sh" 2>/dev/null || true

# COURTESY GUARD (S327): this gate seizes the pointer, and it now runs as part of gates_all.sh.
# If a human is moving the mouse, exit 2 (INCONCLUSIVE, reported as SKIP) rather than yanking the
# cursor away mid-click. MOUSE_FORCE=1 overrides.
if [ "${MOUSE_FORCE:-0}" != 1 ] && command -v xdotool >/dev/null; then
  _p0=$(xdotool getmouselocation --shell 2>/dev/null | head -2 | tr '\n' ' ')
  sleep 5
  _p1=$(xdotool getmouselocation --shell 2>/dev/null | head -2 | tr '\n' ' ')
  [ "$_p0" = "$_p1" ] || { echo "pointer is in use by a human -- refusing to grab it (MOUSE_FORCE=1 to override)" >&2; exit 2; }
fi
if command -v assert_clean_start >/dev/null 2>&1; then assert_clean_start || exit 2
elif pgrep -x wmig >/dev/null 2>&1; then
  echo "  REFUSING TO RUN: wmig already running (pid $(pgrep -x wmig | tr '\n' ' ')) -- it may be the player's."
  exit 2
fi

VP=""; [ "$CONTROL" = "1" ] && VP="MA_VIEWPORT_SCRWH=1"
log="$OUT/real_mouse.log"
echo "real mouse -- xdotool on the live window$([ "$CONTROL" = 1 ] && echo '   [NEGATIVE CONTROL: MA_VIEWPORT_SCRWH=1, must go RED when window != requested]')"

( cd "$RUNDIR" && env BOB_RUN_INIT=1 BOB_DRIVE_C="$BOB_DRIVE_C" MA_DISABLE_3D=1 \
    MA_MAXIMIZE=${MA_MAXIMIZE:-0} \
    MA_TRACE_OLE=1 MA_TRACE_CLICK=1 MA_TRACE_PRESENT=1 $VP "$WMIG" ) >"$log" 2>&1 &
GAME=$!
sleep "$SETTLE"

fail=0
say() { printf '  %-46s %s\n' "$1" "$2"; }

W=$(xdotool search --name "Mig Alley" 2>/dev/null | head -1)
if [ -z "${W:-}" ]; then say "window found" "NO -- xdotool cannot see it"; kill -9 $GAME 2>/dev/null; echo FAIL; exit 1; fi
eval "$(xdotool getwindowgeometry --shell "$W")"
say "window found" "id=$W at $X,$Y size ${WIDTH}x${HEIGHT}"

# S215: ACTIVATE the window before clicking. Under click-to-focus a click on an unfocused window is
# consumed activating it and never reaches the app -- the gate's first run measured exactly that
# (zero [ole_mouse] lines while the menu was demonstrably drawn), and a hand-run of the identical
# coordinates had worked minutes earlier only because that window happened to already have focus.
# Same family as S185/PO-60 (SDL delivers KEYS only to a focused window); this is the mouse half.
# Without this the gate tests the compositor's focus policy, not the port.
xdotool windowactivate --sync "$W" 2>/dev/null || xdotool windowfocus "$W" 2>/dev/null || true
sleep 1

# Report whether the control arm can discriminate at all (see header).
CANV=$(grep -a "\[present\] canvas=" "$log" | tail -1 | sed -n 's/.*canvas=\([0-9]*x[0-9]*\).*/\1/p')
VPRT=$(grep -a "\[present\] canvas=" "$log" | tail -1 | sed -n 's/.*viewport=\([0-9]*x[0-9]*\).*/\1/p')
WND=$(grep -a "\[present\] canvas=" "$log" | tail -1 | sed -n 's/.*window=\([0-9]*x[0-9]*\).*/\1/p')
say "geometry (canvas/viewport/window)" "${CANV:-?} / ${VPRT:-?} / ${WND:-?}"

# S215b: in CONTROL mode, RESIZE the window first so the presented rect and the requested mode
# genuinely differ -- otherwise the control cannot discriminate (its first run correctly reported
# INCONCLUSIVE rather than claiming a pass, because canvas==window makes the old mapping exact).
# A window manager resizing us is a real scenario, not a contrivance, so this also tests something
# worth testing: after an external resize, does a click still land where the player pointed?
# Coordinates are scaled by the same ratio the present uses, which is the whole point.
CX=582; CY=251
if [ "$CONTROL" = "1" ]; then
  xdotool windowsize "$W" 1200 800 2>/dev/null || true
  sleep 2
  eval "$(xdotool getwindowgeometry --shell "$W")"
  say "resized for the control arm" "window now ${WIDTH}x${HEIGHT} (canvas stays ${CANV:-?})"
  cw=${CANV%%x*}; ch=${CANV##*x}
  if [ -n "${cw:-}" ] && [ "${cw:-0}" -gt 0 ]; then
    CX=$(( 582 * WIDTH / cw )); CY=$(( 251 * HEIGHT / ch ))
  fi
fi

# 2+3: a real click on the title menu's row 1 (Single Player)
xdotool mousemove --sync $((X+CX)) $((Y+CY)); xdotool click --window "$W" 1
sleep 4
if grep -aq "\[ole_click\] local=.* -> row=1" "$log"; then
  say "real click received AND mapped to row 1" "yes -- $(grep -a '\[ole_mouse\] listbox rect=(530,210,105,100)' "$log" | tail -1 | sed 's/.*click=/click=/')"
else
  say "real click received AND mapped to row 1" "NO -- the SDL mouse path did not deliver it"; fail=1
fi

# 4: did the screen advance? the Single Player submenu is a different listbox
if [ "$CONTROL" = "1" ] && [ -n "${cw:-}" ] && [ "${cw:-0}" -gt 0 ]; then
  X2=$(( 636 * WIDTH / cw )); Y2=$(( 251 * HEIGHT / ch ))
else X2=636; Y2=251; fi
xdotool mousemove --sync $((X+X2)) $((Y+Y2)); xdotool click --window "$W" 1
sleep 4
if grep -aq "\[ole_mouse\] listbox rect=(530,210,213,143)" "$log"; then
  say "screen advanced (Single Player submenu)" "yes -- 213x143 listbox took the second click"
else
  say "screen advanced (Single Player submenu)" "NO -- the first click mapped but changed nothing"; fail=1
fi

kill -9 $GAME 2>/dev/null; wait $GAME 2>/dev/null

echo "----------------------------------------"
if [ "$CONTROL" = "1" ]; then
  if [ "${WIDTH}x${HEIGHT}" = "${CANV:-x}" ]; then
    echo "CONTROL INCONCLUSIVE: the resize did not take (window ${WIDTH}x${HEIGHT} == canvas ${CANV:-?}),"
    echo "                      so the old mapping is exact and this arm cannot discriminate."
    exit 2
  fi
  if [ "$fail" -ne 0 ]; then echo "CONTROL OK: with the old viewport behaviour the real click misses"; exit 0
  else echo "CONTROL BROKEN: passed with the fix reverted -- this gate is not testing S209b"; exit 1; fi
fi
if [ "$fail" -eq 0 ]; then echo "PASS: a real mouse click reaches the game, maps correctly, and advances the screen"
else echo "FAIL"; exit 1; fi
