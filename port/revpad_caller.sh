#!/usr/bin/env bash
# PO-78: the reverse padlock ("bogey view", Ctrl+F6 = OUTREVLOCKTOG).
#
# HISTORY, because this item accumulated four withdrawn root causes and the gate should not let a
# fifth stand: the binding was blamed (S239/S332/S334 -- wrong, Ctrl+F6 IS bound to action 224),
# the loader was blamed (S334 -- withdrawn in full at S335, it drops nothing), and InitFlyingView
# was blamed for stealing the mode (S292 -- never observed to run). The measured answer (S345):
#
#   the reverse padlock is "view FROM the bogey", so with trackeditem2 == NULL there is nothing to
#   look from. DrawOutRevPadlock's own first line reverts the view. The mode was set, the setup
#   ran, ONE frame was drawn from a camera with nowhere to stand, and the view snapped back --
#   a single-frame camera jump. That is the PO's report verbatim: "it just causes the F86 to make
#   a quick twitch motion". S345 declines the toggle when there is no target.
#
# So there are TWO behaviours to hold, and a gate that checks only one of them is worthless:
#   A. NO TARGET  -> the toggle DECLINES. Mode never set, no setup, no frame, no twitch.
#   B. WITH TARGET-> the toggle WORKS. Mode set, DrawOutRevPadlock frames, no revert.
#
# The first cut of this gate ran A only and reported it as "RESULT: mode set but the setup routine
# never ran" -- exit 1 -- while its own counters printed "mode SET: 0". It called the FIX a
# failure and would have sent a later sprint hunting a defect that had already been repaired.
#
# A Ctrl-modified key cannot be reached through BOB_KEYSEQ (a synthesised DIK tap carries no SDL
# modifier state -- bob_video.cpp S92), so MA_REVPADLOCK_AT fires List6Toggle synthetically.
# ENTER (PADLOCKTOG, DIK 0x1C) has no modifier and CAN be sent, which is how B gets its target.
#
# Usage: port/revpad_caller.sh [AT]        (default AT=1 -- fire on the first dispatch)
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WMIG="${WMIG:-$ROOT/build/wmig}"
BOB_DRIVE_C="${BOB_DRIVE_C:-$HOME/sgl/TUE/MigAlley/WP/drive_c}"
RUNDIR="$BOB_DRIVE_C/rowan/mig"
OUT="$ROOT/port/out/revpad"; mkdir -p "$OUT"
AT="${1:-1}"
CLICKSEQ="${BOB_CLICKSEQ:-40,r1;95,r0}"   # S63 menu ROWS (from stress_launch.sh)
TIMEOUT="${TIMEOUT:-60}"

[ -x "$WMIG" ] || { echo "no wmig at $WMIG (run port/rebuild.sh)"; exit 2; }
[ -d "$RUNDIR" ] || { echo "no rundir $RUNDIR (set BOB_DRIVE_C)"; exit 2; }

# run <log> <at> [keyseq]  -- mirror stress_launch.sh's environment EXACTLY apart from the revpad
# envs. Its recipe reaches 3D flight 20/20; a cut of this gate that also dropped MA_TRACE_3D
# never reached InitFlyingView at all. Do not vary two things at once from a harness that works.
run() {
    local log="$1" at="$2" keyseq="${3:-}"
    : > "$log"
    ( cd "$RUNDIR" && exec env BOB_RUN_INIT=1 MA_ENABLE_3D=1 MA_TRACE_3D=1 \
          MA_DUMP_BACK="${FRAMES:-100}" \
          MA_REVPADLOCK_AT="$at" MA_TRACE_REVPAD=1 \
          ${keyseq:+BOB_KEYSEQ="$keyseq"} \
          BOB_CLICKSEQ="$CLICKSEQ" BOB_DRIVE_C="$BOB_DRIVE_C" "$WMIG" ) >"$log" 2>&1 &
    local pid=$!
    local i
    for i in $(seq 1 $((TIMEOUT*5))); do
        grep -aq "toggle declined\|DrawOutRevPadlock frame 200\|InitFlyingView RESET" "$log" 2>/dev/null && break
        kill -0 "$pid" 2>/dev/null || break
        sleep 0.2
    done
    sleep 2
    kill -KILL "$pid" 2>/dev/null; wait "$pid" 2>/dev/null
}

cnt() { local n; n=$(grep -ac "$1" "$2" 2>/dev/null); echo "${n:-0}"; }

fail=0

# ---------------------------------------------------------------- A: no padlock target
LOGA="$OUT/revpad_notarget.log"
echo "A. no padlock target -- the toggle must DECLINE (S345), not twitch the aircraft"
run "$LOGA" "$AT"
a_reach=$(cnt "probe BFViewSelect" "$LOGA")
a_enter=$(cnt "List6Toggle entered" "$LOGA")
a_decl=$(cnt  "toggle declined"    "$LOGA")
a_set=$(cnt   "mode SET to VM_OutRevPadlock" "$LOGA")
a_draw=$(cnt  "DrawOutRevPadlock frame"      "$LOGA")
printf '   reached flight %s | toggle entered %s | DECLINED %s | mode set %s | draw frames %s\n' \
       "$a_reach" "$a_enter" "$a_decl" "$a_set" "$a_draw"
if [ "$a_reach" -eq 0 ]; then
    echo "   INCONCLUSIVE: never got into flight -- harness failure, not a finding"; exit 2
elif [ "$a_enter" -eq 0 ]; then
    echo "   FAIL: the toggle never ran at all (the S240 dispatch entry is gone again?)"; fail=1
elif [ "$a_decl" -gt 0 ] && [ "$a_set" -eq 0 ] && [ "$a_draw" -eq 0 ]; then
    echo "   PASS: declined with no target -- no mode change, no frame, so no twitch"
else
    echo "   FAIL: with no target the toggle went ahead -- this is the PO's twitch, back again"; fail=1
fi

# ---------------------------------------------------------------- B: with a padlock target
# ENTER = PADLOCKTOG supplies trackeditem2. Tap it well before the toggle fires, and fire the
# toggle late enough that the padlock is established first.
LOGB="$OUT/revpad_target.log"
KEYSEQ="${BOB_KEYSEQ_B:-60,28;80,28}"
ATB="${ATB:-40}"
echo
echo "B. ENTER first (PADLOCKTOG, DIK 0x1C) -- with a target the view must actually WORK"
run "$LOGB" "$ATB" "$KEYSEQ"
b_reach=$(cnt "probe BFViewSelect" "$LOGB")
b_enter=$(cnt "List6Toggle entered" "$LOGB")
b_decl=$(cnt  "toggle declined"    "$LOGB")
b_set=$(cnt   "mode SET to VM_OutRevPadlock" "$LOGB")
b_draw=$(cnt  "DrawOutRevPadlock frame"      "$LOGB")
b_reset=$(cnt "InitFlyingView RESET"         "$LOGB")
printf '   reached flight %s | toggle entered %s | declined %s | mode SET %s | draw frames %s | RESET %s\n' \
       "$b_reach" "$b_enter" "$b_decl" "$b_set" "$b_draw" "$b_reset"
if [ "$b_reach" -eq 0 ]; then
    echo "   INCONCLUSIVE: never got into flight"
elif [ "$b_enter" -eq 0 ]; then
    echo "   INCONCLUSIVE: the synthetic toggle never fired at ATB=$ATB"
elif [ "$b_decl" -gt 0 ]; then
    echo "   INCONCLUSIVE: ENTER did not establish a padlock, so this run tested case A again."
    echo "   (tune BOB_KEYSEQ_B / ATB -- do NOT read this as 'the bogey view is broken')"
elif [ "$b_set" -gt 0 ] && [ "$b_draw" -gt 0 ] && [ "$b_reset" -eq 0 ]; then
    echo "   PASS: mode held and the bogey view drew $b_draw traced frames, never reverted"
    grep -a "camera=" "$LOGB" | head -2
else
    echo "   FAIL: with a target the view did not hold (set=$b_set draw=$b_draw reset=$b_reset)"; fail=1
fi

echo
[ "$fail" -eq 0 ] && { echo "PO-78 REVERSE PADLOCK: PASS"; exit 0; }
echo "PO-78 REVERSE PADLOCK: FAIL"; exit 1
