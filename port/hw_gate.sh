#!/usr/bin/env bash
# port/hw_gate.sh — run the standing gates on the HARDWARE renderer (S119).
#
# WHY THIS EXISTS. S118 shipped the hardware option with every gate green, and the PO found it
# broken within ten minutes of play-testing: it crashed entering 3D, the campaign screens stopped
# repainting, and Fly left a blank window. The gates could not have caught any of it, because they
# pin MA_NO_HARDWARE=1 -- which withdraws the D3D device entirely, a configuration no player will
# ever have. Pinning is right for the software references (they were captured that way), but a
# suite that only ever pins the feature away is not testing the feature.
#
# So: same gates, MA_NO_HARDWARE=0. Run this alongside the software suite whenever the renderer,
# the DirectDraw compat or the display path changes.
#
# RUN IT DIRECTLY -- never `gl-lock port/hw_gate.sh`. This script takes the display lock ITSELF
# for its parity arm, and flock is not reentrant: wrapping it deadlocks until the timeout kills
# it, leaving an orphaned lock holder that then blocks every later GL run. Cost two attempts and a
# manual lock cleanup on 2026-08-16. The same applies to any gate that calls gl-lock internally.
#
#   port/hw_gate.sh [runs]
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
RUNS="${1:-20}"
export MA_NO_HARDWARE=0        # offer the device
export MA_TRY_HARDWARE=1       # and select it, whatever settings.mig happens to say

fail=0
echo "=== hardware-renderer gate suite ==="

# 1. The renderer-INDEPENDENT front-end screens must be byte-identical to the software references:
#    the front end is a software canvas either way, so a DIFF here means the renderer is leaking
#    into the 2D path -- which is exactly how S118's empty Resolutions combo showed up.
#
#    prefs_3d and prefs_others are deliberately EXCLUDED. They report the renderer and its
#    capabilities, so they SHOULD differ on hardware: prefs_3d reads "Primary Display Driver"
#    instead of "Software Driver", and prefs_others shows three options that software leaves off.
#    Asserting byte-identity there would be asserting that choosing hardware changes nothing
#    visible, which is the opposite of the feature. (First cut of this gate did exactly that and
#    "failed" on correct behaviour.)
echo "--- parity, renderer-independent screens (title quickmission campaign_map)"
# NB match the PASS line anywhere, not `tail -1`: gl-lock prints its own "display released" line
# last, so tailing one line never sees the verdict. (That, not the game, is what failed this arm
# on its first two runs.)
if gl-lock "$ROOT/port/parity_2d.sh" title quickmission campaign_map 2>&1 | grep -q "^PASS"; then
  echo "  parity: PASS"
else
  echo "  parity: FAIL"; fail=1
fi

# 2. Flight must start and survive on the hardware path. This is the one that would have caught
#    S119's SIGSEGV and the blank-window hang.
echo "--- stress launch x$RUNS (hardware)"
if "$ROOT/port/stress_launch.sh" "$RUNS" 100 25 2>&1 | grep -q "^PASS"; then
  echo "  stress: PASS"
else
  echo "  stress: FAIL"; fail=1
fi

# 3. The campaign path: the PO reached the campaign map and Fly left the window blank. Reuse the
#    campaign recipe and require the process to survive and present.
echo "--- campaign map (hardware)"
RUNDIR="${BOB_DRIVE_C:-$HOME/sgl/TUE/MigAlley/WP/drive_c}/rowan/mig"
log=/tmp/hw_gate_campaign.log
( cd "$RUNDIR" && timeout 100 gl-lock env BOB_RUN_INIT=1 MA_ENABLE_3D=1 \
    BOB_CLICKSEQ="30,588,263;65,40,108;100,68,565" BOB_DUMP_FRAME=600 BOB_EXIT_AFTER_DUMP=1 \
    BOB_DRIVE_C="${BOB_DRIVE_C:-$HOME/sgl/TUE/MigAlley/WP/drive_c}" \
    "$ROOT/build/wmig" ) > "$log" 2>&1
code=$?
if [ $code -eq 139 ]; then echo "  campaign: FAIL (SIGSEGV)"; fail=1
elif grep -aq "dumped frame" "$log"; then echo "  campaign: PASS"
else echo "  campaign: FAIL (no frame presented, exit=$code)"; fail=1
fi

echo "----------------------------------------"
[ $fail -eq 0 ] && echo "PASS: hardware renderer gates" || echo "FAIL: see above"
exit $fail
