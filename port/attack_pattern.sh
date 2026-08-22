#!/usr/bin/env bash
# port/attack_pattern.sh — gate for EPIC K step 9: the TASKS dialog's attack PATTERN can be
# changed, the change reaches game state, and it survives closing and reopening the dialog —
# while the attack METHOD stays Dive Bomb throughout.
#
# The PO's script, step 9: "Leave attack method at Dive Bomb; change the attack pattern to
# Individual Targets (many targets, divide the effort)."
#
# ⚠ NAMED DIVERGENCE FROM THE SCRIPT'S PREMISE: on this mission the port's attack pattern is
# ALREADY "Individual targets" when the dialog first opens — the Minimum Strike profile sets
# info_airgrp::attpattern to 2, and CFlt_Task shows it at index 1 of the six DIVE patterns. The
# script reads as if the default were something else. Whether gold's default differs cannot be
# read off the video (it only ever shows the post-change state), so this gate does NOT claim the
# default is wrong. It proves the MECHANISM instead, and ends in the state the script asks for:
#
#   default -> row 3 (Spaced target selection) -> close+reopen -> still row 3
#           -> row 1 (Individual targets)      -> close+reopen -> still row 1
#
# Reading back after a close/reopen is the whole point: a combo that merely repaints its own
# caption would pass a "did it change?" check while nothing reached the mission. The dialog is
# closed at the PROFILE level and reopened from the Mission Folder, because CProfile::SetTaskTabs
# only rebuilds the task sheet when the wave changes — re-clicking the same cell just re-selects
# the tab and refills nothing.
#
# The readback comes from the combo's own SetIndex trace (the caption is drawn, never logged).
set -u
export MA_NO_HARDWARE="${MA_NO_HARDWARE:-1}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
. "$ROOT/port/gate_lib.sh"          # S171: assert_no_crash / assert_recipe_ran
WMIG="${WMIG:-$ROOT/build/wmig}"
BOB_DRIVE_C="${BOB_DRIVE_C:-$HOME/sgl/TUE/MigAlley/WP/drive_c}"
RUNDIR="$BOB_DRIVE_C/rowan/mig"
OUT="${OUT:-/tmp/ma_pattern}"
TMO="${TMO:-620}"
TARGET="${TARGET:-Wonju}"
AUTHORISE=2023; LBFILE=1055; LBROWS=2018; PROFILE3=2124; PATTERN=2149; TITLE=1001
mkdir -p "$OUT"
[ -x "$WMIG" ] || { echo "no binary at $WMIG" >&2; exit 2; }

SAVEFILE="$RUNDIR/SaveGame/Auto Save.sav"
PIN="$ROOT/port/ref/save/campaign_pristine.sav"
STASH="$(mktemp -u /tmp/ma_pattern_save.XXXXXX)"
[ -f "$SAVEFILE" ] && cp -a "$SAVEFILE" "$STASH"
restore_save() { [ -f "$STASH" ] && cp -a "$STASH" "$SAVEFILE" && rm -f "$STASH"; }
trap restore_save EXIT INT TERM
[ -f "$PIN" ] && cp -f "$PIN" "$SAVEFILE"

log="$OUT/pattern.log"; ppm="$OUT/pattern.ppm"; rm -f "$ppm"
NAV="30,r3;65,#$LBFILE;100,#2063:1"
# reach the Main Duty cell of wave 1; that cell click opens the task sheet by itself
OPEN="700,#$LBROWS@CMissionFolder:r0;780,#$PROFILE3@CMissionFolder;860,#$LBROWS@CProfile:r1.2"
REOPEN() { echo "$1,#$TITLE@CProfile:-3;$2,#$LBROWS@CMissionFolder:r0;$3,#$PROFILE3@CMissionFolder;$4,#$LBROWS@CProfile:r1.2"; }
SEQ="$NAV;420,#$AUTHORISE@DossierButtons;520,#$LBFILE@CLoad:r0;$OPEN"
SEQ="$SEQ;1020,#$PATTERN@CFlt_Task;1080,#$PATTERN@CFlt_Task:r3;$(REOPEN 1180 1280 1360 1440)"
SEQ="$SEQ;1560,#$PATTERN@CFlt_Task;1620,#$PATTERN@CFlt_Task:r1;$(REOPEN 1720 1820 1900 1980)"
echo "attack pattern on the \"$TARGET\" strike — wmig"
# NB run this UNDER gl-lock; it does not take the lock itself (nesting deadlocks, S159).
( cd "$RUNDIR" && timeout -k 5 -s KILL "$TMO" env \
    SDL_VIDEODRIVER=dummy BOB_RUN_INIT=1 BOB_DRIVE_C="$BOB_DRIVE_C" MA_DISABLE_3D=1 \
    MA_IGNORE_SAVE_DATE=1 MA_TRACE_OOB=1 MA_TRACE_CLICK=1 MA_TRACE_OLE=1 \
    MA_MAP_ITEM_SCAN=250 MA_MAP_CLICK_FIRST=1 MA_MAP_CLICK_NAME="$TARGET" \
    BOB_CLICKSEQ="$SEQ" MA_SHOT=2150 MA_SHOT_PATH="$ppm" "$WMIG" ) >"$log" 2>&1
pkill -x "$(basename "$WMIG")" 2>/dev/null

fail=0
assert_no_crash "$log" || fail=1
assert_recipe_ran "$log" || fail=1
# A recipe entry that can never resolve holds every later entry: catch that before reading results,
# or a truncated run reads as "the values never changed" (S171).
if grep -aq "\[clickseq\] STALLED" "$log"; then
  echo "  $(grep -a '\[clickseq\] STALLED' "$log" | head -1 | sed 's/^\[clickseq\] //') — FAIL"; fail=1
fi
# Two live copies of one dialog make @Class meaningless and the resolver picks by pointer (S171).
if grep -aq "\[clickid\] WARNING id=.* AMBIGUOUS" "$log"; then
  echo "  $(grep -a 'AMBIGUOUS' "$log" | head -1 | sed 's/^\[clickid\] //') — FAIL"; fail=1
fi

pat=$(grep -a 'SetIndex [0-9]*/6' "$log" | sed 's/.*SetIndex \([0-9]*\)\/6 "\([^"]*\)".*/\1:\2/')
meth=$(grep -a 'SetIndex [0-9]*/3 "' "$log" | grep -a 'Dive\|Low\|High' | sed 's/.*"\([^"]*\)".*/\1/')
echo "  attack pattern, each time the dialog filled it:"
echo "$pat" | sed 's/^/    /'
echo "  attack method, each time: $(echo "$meth" | tr '\n' ' ')"

want=$'1:Individual targets\n3:Spaced target selection\n1:Individual targets'
if [ "$pat" = "$want" ]; then
  echo "  pattern default -> Spaced (survives reopen) -> Individual (survives reopen): yes"
else
  echo "  expected the three readbacks 1 / 3 / 1 — FAIL"; fail=1
fi
if [ -n "$meth" ] && [ -z "$(echo "$meth" | grep -av '^Dive Bomb$')" ]; then
  echo "  attack method never left Dive Bomb: yes"
else
  echo "  attack method changed or was never read — FAIL"; fail=1
fi

echo "----------------------------------------"
if [ "$fail" -eq 0 ]; then
  echo "PASS: EPIC K step 9 — the attack pattern changes, persists across a dialog reopen, and the method stays Dive Bomb"
else
  echo "FAIL"; exit 1
fi
