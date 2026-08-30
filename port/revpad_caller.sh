#!/usr/bin/env bash
# PO-78: name what takes the reverse-padlock view back.
#
# The PO reports Ctrl+F6 in a replay "does not show bogie view, it just causes the F86 to make a
# quick twitch motion". S292 narrowed that to three candidates that look identical from outside:
#   (a) the toggle never runs
#   (b) it runs, and the mode is reset before a frame is drawn
#   (c) the mode sticks but the draw produces nothing
# and stalled, because observing them required the PO to press the key by hand. MA_REVPADLOCK_AT
# fires List6Toggle synthetically (a Ctrl-modified key cannot be reached through BOB_KEYSEQ at
# all -- a synthesised DIK tap carries no SDL modifier state, bob_video.cpp S92), so the whole
# sequence can be driven headlessly. MA_TRACE_REVPAD prints entry, the mode set, the setup call,
# the reset, and a backtrace of the RESETTER.
#
# This gate does not judge pixels. It reports which of (a)/(b)/(c) actually happened.
#
# Usage: port/revpad_caller.sh [AT]        (default AT=1 -- fire on the first InitFlyingView)
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WMIG="${WMIG:-$ROOT/build/wmig}"
BOB_DRIVE_C="${BOB_DRIVE_C:-$HOME/sgl/TUE/MigAlley/WP/drive_c}"
RUNDIR="$BOB_DRIVE_C/rowan/mig"
OUT="$ROOT/port/out/revpad"; mkdir -p "$OUT"
AT="${1:-1}"
CLICKSEQ="${BOB_CLICKSEQ:-40,r1;95,r0}"   # S63 menu ROWS (from stress_launch.sh): ab.sh's absolute
                                          # pixel form is the pre-S63 recipe and no longer resolves
TIMEOUT="${TIMEOUT:-60}"
LOG="$OUT/revpad.log"

[ -x "$WMIG" ] || { echo "no wmig at $WMIG (run port/rebuild.sh)"; exit 2; }
[ -d "$RUNDIR" ] || { echo "no rundir $RUNDIR (set BOB_DRIVE_C)"; exit 2; }

echo "PO-78: firing the reverse padlock at InitFlyingView call $AT"
: > "$LOG"
# Mirror stress_launch.sh's environment EXACTLY apart from the two revpad envs. Its recipe
# reaches 3D flight 20/20, and the first cut of this gate -- same binary, same CLICKSEQ, but
# without MA_TRACE_3D/MA_DUMP_BACK -- never reached InitFlyingView at all. Do not vary two
# things at once from a harness that is known to work.
( cd "$RUNDIR" && exec env BOB_RUN_INIT=1 MA_ENABLE_3D=1 MA_TRACE_3D=1 \
      MA_DUMP_BACK="${FRAMES:-100}" \
      MA_REVPADLOCK_AT="$AT" MA_TRACE_REVPAD=1 \
      BOB_CLICKSEQ="$CLICKSEQ" BOB_DRIVE_C="$BOB_DRIVE_C" "$WMIG" ) >"$LOG" 2>&1 &
pid=$!
for _ in $(seq 1 $((TIMEOUT*5))); do
    grep -aq "InitFlyingView RESET" "$LOG" 2>/dev/null && break
    kill -0 "$pid" 2>/dev/null || break
    sleep 0.2
done
sleep 2
kill -KILL "$pid" 2>/dev/null; wait "$pid" 2>/dev/null

echo "---- revpad trace ----"
grep -a "\[revpad\]" "$LOG" | head -30
echo "----------------------"

# Report which candidate the evidence supports. Each is a distinct log line, so this is a
# reading of the trace, not an inference from a single "it did not work".
reached=$(grep -ac "probe BFViewSelect" "$LOG" 2>/dev/null; true)
fired=$(grep -ac "firing List6Toggle" "$LOG" 2>/dev/null; true)
entered=$(grep -ac "List6Toggle entered" "$LOG" 2>/dev/null; true)
setmode=$(grep -ac "mode SET to VM_OutRevPadlock" "$LOG" 2>/dev/null; true)
setupran=$(grep -ac "InitOutRevPadlock ran" "$LOG" 2>/dev/null; true)
reset=$(grep -ac "InitFlyingView RESET" "$LOG" 2>/dev/null; true)

echo "  BFViewSelect reached        : $reached"
echo "  toggle fired synthetically  : $fired"
echo "  List6Toggle entered         : $entered"
echo "  mode SET                    : $setmode"
echo "  InitOutRevPadlock ran       : $setupran"
echo "  mode RESET by InitFlyingView: $reset"

if [ "$reached" -eq 0 ]; then
    echo "INCONCLUSIVE: BFViewSelect was never reached -- the run did not get into flight."
    echo "  (that is a harness failure, not a finding about the padlock)"
    exit 2
elif [ "$entered" -eq 0 ]; then
    echo "RESULT (a): the toggle never runs."
    exit 1
elif [ "$reset" -gt 0 ]; then
    echo "RESULT (b): the mode is SET and then TAKEN BACK by InitFlyingView. Caller backtrace:"
    grep -a -A24 "InitFlyingView RESET" "$LOG" | grep -av "\[revpad\] \*\*\*" | head -24
    exit 1
elif [ "$setupran" -gt 0 ]; then
    echo "RESULT (c): mode set and setup ran, and nothing took it back -- the draw is the suspect."
    exit 1
else
    echo "RESULT: mode set but the setup routine never ran."
    exit 1
fi
