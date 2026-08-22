#!/usr/bin/env bash
# port/add_flight.sh — gate for EPIC K step 8: ADD A THIRD FLIGHT to the Wonju strike through
# the Squadron slot's Flights spin-box, and have the change reach the mission.
#
# The PO's script, step 8: "In Mission Folder -> Profile, you have one Wave of 2 F84 flights on
# bombing duty. Click Task (or the F84 (2) duty field) and add a third flight — either via the
# Squadron slot's Flights spin-box, or by clicking the 3rd flight slot."  This gate takes the
# spin-box route, which is the one that was impossible before S170: RSpinBut was the last R*
# type the port never hosted, so every InvokeHelper on one was a silent no-op and the control was
# never created, drawn or clickable.
#
# THE ROUTE, and why each hop is its own assertion:
#   1. the Main Duty CELL is selected     — the wave table is Wave/ToT/Main Duty/AAA Cover/Air
#                                            Cover, and `:r1` (row centre) lands in column 3, so
#                                            the Task button — which reads currcol — opened the
#                                            FLAK tab. The recipe looked right and edited the
#                                            wrong duty. `:r1.2` names the cell (S170).
#   2. the duty field opens ChooseSquad   — IDC_ACTYPE is an RedtBt, and CT_EDTBT was drawn but
#                                            inert until S170. It is the ONLY door to the dialog
#                                            that owns the spin-box.
#   3. the spinner actually MOVES         — CRSpinButCtrl refuses UP at `index > count-2`, so a
#                                            spinner at its limit takes the click and correctly
#                                            does nothing. Asserting "the click was delivered"
#                                            would pass on that. Assert the INDEX CHANGED.
#   4. the dialog's own handler runs      — ChooseSquad::OnTextChangedRspinbutctrl1 -> SetFlights.
#   5. **the Mission Folder reads 3**     — the walkthrough calls this "the cheapest end-to-end
#                                            assertion in the epic": one number, and it only moves
#                                            if the flight reached the mission. Read from the
#                                            folder's own AddString trace, i.e. from what the game
#                                            put in the list, not from pixels.
set -u
export MA_NO_HARDWARE="${MA_NO_HARDWARE:-1}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
. "$ROOT/port/gate_lib.sh"          # S171: assert_no_crash / assert_recipe_ran
WMIG="${WMIG:-$ROOT/build/wmig}"
BOB_DRIVE_C="${BOB_DRIVE_C:-$HOME/sgl/TUE/MigAlley/WP/drive_c}"
RUNDIR="$BOB_DRIVE_C/rowan/mig"
OUT="${OUT:-/tmp/ma_addflight}"
TMO="${TMO:-420}"
TARGET="${TARGET:-Wonju}"
WANT_BEFORE="${WANT_BEFORE:-2}"; WANT_AFTER="${WANT_AFTER:-3}"
# control ids (SRC/MFC/RESOURCE.H)
AUTHORISE=2023; LBFILE=1055; FILEOK=1056; LBROWS=2018; PROFILE3=2124
TASK=2143; ACTYPE=2296; SPIN=1072; TITLE=1001
mkdir -p "$OUT"
[ -x "$WMIG" ] || { echo "no binary at $WMIG" >&2; exit 2; }

SAVEFILE="$RUNDIR/SaveGame/Auto Save.sav"
PIN="$ROOT/port/ref/save/campaign_pristine.sav"
STASH="$(mktemp -u /tmp/ma_addflight_save.XXXXXX)"
[ -f "$SAVEFILE" ] && cp -a "$SAVEFILE" "$STASH"
restore_save() { [ -f "$STASH" ] && cp -a "$STASH" "$SAVEFILE" && rm -f "$STASH"; }
trap restore_save EXIT INT TERM
[ -f "$PIN" ] && cp -f "$PIN" "$SAVEFILE"

log="$OUT/add_flight.log"; ppm="$OUT/add_flight.ppm"; rm -f "$ppm"
NAV="30,r3;65,#$LBFILE;100,#2063:1"
# `:r0` both selects Minimum Strike and loads it -- CLoad::OnSelectRlistboxfile calls OnOK when
# the row is already current, and currrow starts at 0. There is no separate Load click (S171).
SEQ="$NAV;420,#$AUTHORISE@DossierButtons;520,#$LBFILE@CLoad:r0"
SEQ="$SEQ;700,#$LBROWS@CMissionFolder:r0;780,#$PROFILE3@CMissionFolder"
SEQ="$SEQ;860,#$LBROWS@CProfile:r1.2;940,#$TASK@CProfile;1020,#$ACTYPE@CFlt_Task"
SEQ="$SEQ;1120,#$SPIN@ChooseSquad:0;1200,#$TITLE@ChooseSquad:-3"
echo "add a flight to \"$TARGET\" via the Flights spin-box — wmig"
# NB run this UNDER gl-lock; it does not take the lock itself (nesting deadlocks, S159).
( cd "$RUNDIR" && timeout -k 5 -s KILL "$TMO" env \
    SDL_VIDEODRIVER=dummy BOB_RUN_INIT=1 BOB_DRIVE_C="$BOB_DRIVE_C" MA_DISABLE_3D=1 \
    MA_IGNORE_SAVE_DATE=1 MA_TRACE_OOB=1 MA_TRACE_CLICK=1 MA_TRACE_OLE=1 MA_TRACE_SPIN=1 \
    MA_MAP_ITEM_SCAN=250 MA_MAP_CLICK_FIRST=1 MA_MAP_CLICK_NAME="$TARGET" \
    BOB_CLICKSEQ="$SEQ" MA_SHOT=1400 MA_SHOT_PATH="$ppm" "$WMIG" ) >"$log" 2>&1
pkill -x "$(basename "$WMIG")" 2>/dev/null

fail=0
assert_no_crash "$log" || fail=1
assert_recipe_ran "$log" || fail=1
cell=$(grep -a "\[tbclick\] listbox id=$LBROWS.*CProfile" "$log" | tail -1 | sed -n 's/.*-> \(row=[0-9]* col=[0-9]*\).*/\1/p')
if [ "$cell" = "row=1 col=2" ]; then echo "  wave table cell selected: $cell (Main Duty)"
else echo "  wave table cell selected: ${cell:-none} — expected row=1 col=2 (Main Duty) — FAIL"; fail=1; fi

if grep -aq "\[tbclick\] edtbt id=$ACTYPE.*Clicked on .*CFlt_Task" "$log"; then
  echo "  duty field opened the squadron dialog: yes"
else echo "  the duty field (IDC_ACTYPE) never fired — FAIL"; fail=1; fi

spin=$(grep -a "\[spin\] click" "$log" | tail -1)
if [ -n "$spin" ]; then
  echo "  ${spin#\[spin\] }"
  b=$(echo "$spin" | sed -n 's/.*index \([0-9-]*\) -> \([0-9-]*\).*/\1/p')
  a=$(echo "$spin" | sed -n 's/.*index \([0-9-]*\) -> \([0-9-]*\).*/\2/p')
  if [ "${a:-x}" != "${b:-y}" ]; then echo "  spinner index moved: $b -> $a"
  else echo "  the spinner took the click but did NOT move (index $b) — FAIL"; fail=1; fi
else echo "  the spin-box was never clicked — FAIL"; fail=1; fi

if grep -aq "\[evt_fire\] id=$SPIN dispid=1 type=.*ChooseSquad -> HANDLER CALLED" "$log"; then
  echo "  ChooseSquad::OnTextChangedRspinbutctrl1 ran: yes"
else echo "  the spin change never reached the dialog's handler — FAIL"; fail=1; fi

# Assertion 5: the folder's Flights column, in the order the game listed it.
counts=$(grep -a -A3 "AddString\[0\] \"$TARGET" "$log" | grep -a 'AddString\[3\]' | sed 's/.*"\([^"]*\)".*/\1/' | tr -d ' ')
first=$(echo "$counts" | head -1); last=$(echo "$counts" | tail -1)
echo "  Mission Folder Flights for \"$TARGET\": $(echo "$counts" | tr '\n' ' ')"
if [ "${first:-x}" = "$WANT_BEFORE" ] && [ "${last:-x}" = "$WANT_AFTER" ]; then
  echo "  flight count $WANT_BEFORE -> $WANT_AFTER: yes"
else
  echo "  flight count ${first:-none} -> ${last:-none} — expected $WANT_BEFORE -> $WANT_AFTER — FAIL"; fail=1
fi

echo "----------------------------------------"
if [ "$fail" -eq 0 ]; then
  echo "PASS: EPIC K step 8 — a third flight added through the Flights spin-box reaches the mission"
else
  echo "FAIL"; exit 1
fi
