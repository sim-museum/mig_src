#!/usr/bin/env bash
# port/flak_suppression.sh — gate for EPIC K step 11: add flak suppression to the Wonju strike.
#
# The PO's script, step 11: "Flak suppression: Task dialogue -> AAA cover tab -> Squadron slot
# (Off Duty) -> pick the F84 squadron. Change its stores to rockets and guns."
#
# ⚠ NAMED DIVERGENCE, and it is the game's own arithmetic, not a port defect: on this pinned save
# (campaign day one) the ChooseSquad dialog's own **Available** column reads
#   F86 1: 0   F86 2: 0   F80: 1   F84: 0   F51: 0   B29: 0
# so the F84 the script names has no spare flight, and ChooseSquad::OnSelectRlistboxctrl1 refuses
# any squadron with `numavail < 4`. The port therefore flies the flak slot with **F80**, which is
# the same divergence already recorded against **K4** ("gold reads F84 where the port reads F80 —
# the game's own choice from the squadrons available on the pinned save's date"). This gate asserts
# the refusal EXPLICITLY rather than quietly assigning something else: a squadron the game says is
# unavailable must not be assignable, and the click must be shown to have landed.
#
# WHAT IT ASSERTS:
#   1. the AAA Cover cell opens the flak slot  — `:r1.3`, the column that decides which duty the
#                                                task sheet edits (S170; the row centre is col 3
#                                                by luck, and relying on luck is what S170 fixed)
#   2. the slot goes Off Duty -> a real squadron with a flight (`F80 (1/1)`), read from the duty
#      field's own caption — the only readout that names the squadron in a slot
#   3. an UNAVAILABLE squadron is refused      — the F84 row click lands and the slot does not
#                                                become F84
#   4. the stores become "Rockets & Fuel tanks" — the gold PAYLOAD frame reads
#      `8 Rockets (140 lb) / 250 gall External Fuel`; committed through CWeapons::OnOK, which reads
#      the listbox's HIGHLIGHTED row, so a select that did not highlight would commit the old value
#   5. the Mission Folder Flights count moves 2 -> 3 — the suppression flight reached the mission
set -u
export MA_NO_HARDWARE="${MA_NO_HARDWARE:-1}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
. "$ROOT/port/gate_lib.sh"          # S171: assert_no_crash / assert_recipe_ran
assert_clean_start || exit 2          # S177: a stray wmig makes this gate report a content failure
WMIG="${WMIG:-$ROOT/build/wmig}"
BOB_DRIVE_C="${BOB_DRIVE_C:-$HOME/sgl/TUE/MigAlley/WP/drive_c}"
RUNDIR="$BOB_DRIVE_C/rowan/mig"
OUT="${OUT:-/tmp/ma_flak}"
TMO="${TMO:-560}"
TARGET="${TARGET:-Wonju}"
WANT_STORES="${WANT_STORES:-Rockets & Fuel tanks}"
AUTHORISE=2023; LBFILE=1055; LBROWS=2018; PROFILE3=2124; ACTYPE=2296; WEAPONS=2144
WEAPLIST=2347; TITLE=1001; SQROW=4   # row 4 of the squadron list is F84, the one the script names
mkdir -p "$OUT"
[ -x "$WMIG" ] || { echo "no binary at $WMIG" >&2; exit 2; }

SAVEFILE="$RUNDIR/SaveGame/Auto Save.sav"
PIN="$ROOT/port/ref/save/campaign_pristine.sav"
STASH="$(mktemp -u /tmp/ma_flak_save.XXXXXX)"
[ -f "$SAVEFILE" ] && cp -a "$SAVEFILE" "$STASH"
restore_save() { [ -f "$STASH" ] && cp -a "$STASH" "$SAVEFILE" && rm -f "$STASH"; }
trap restore_save EXIT INT TERM
[ -f "$PIN" ] && cp -f "$PIN" "$SAVEFILE"

log="$OUT/flak.log"; ppm="$OUT/flak.ppm"; rm -f "$ppm"
NAV="30,r3;65,#$LBFILE;100,#2063:1"
SEQ="$NAV;420,#$AUTHORISE@DossierButtons;520,#$LBFILE@CLoad:r0"
SEQ="$SEQ;700,#$LBROWS@CMissionFolder:r0;780,#$PROFILE3@CMissionFolder;860,#$LBROWS@CProfile:r1.3"
SEQ="$SEQ;1020,#$ACTYPE@CFlt_Task;1120,#$LBROWS@ChooseSquad:r$SQROW;1220,#$TITLE@ChooseSquad:-3"
SEQ="$SEQ;1340,#$WEAPONS@CFlt_Task;1440,#$WEAPLIST@CWeapons:r2;1560,#$TITLE@CWeapons:-3"
echo "flak suppression on the \"$TARGET\" strike — wmig"
# NB run this UNDER gl-lock; it does not take the lock itself (nesting deadlocks, S159).
( cd "$RUNDIR" && timeout -k 5 -s KILL "$TMO" env \
    SDL_VIDEODRIVER=dummy BOB_RUN_INIT=1 BOB_DRIVE_C="$BOB_DRIVE_C" MA_DISABLE_3D=1 \
    MA_IGNORE_SAVE_DATE=1 MA_TRACE_OOB=1 MA_TRACE_CLICK=1 MA_TRACE_OLE=1 \
    MA_MAP_ITEM_SCAN=250 MA_MAP_CLICK_FIRST=1 MA_MAP_CLICK_NAME="$TARGET" \
    BOB_CLICKSEQ="$SEQ" MA_SHOT=1750 MA_SHOT_PATH="$ppm" "$WMIG" ) >"$log" 2>&1
pkill -x "$(basename "$WMIG")" 2>/dev/null

fail=0
assert_no_crash "$log" || fail=1
assert_recipe_ran "$log" || fail=1
if grep -aq "\[clickseq\] STALLED" "$log"; then
  echo "  $(grep -a '\[clickseq\] STALLED' "$log" | head -1 | sed 's/^\[clickseq\] //') — FAIL"; fail=1
fi
if grep -aq "\[clickid\] WARNING id=.* AMBIGUOUS" "$log"; then
  echo "  $(grep -a AMBIGUOUS "$log" | head -1 | sed 's/^\[clickid\] //') — FAIL"; fail=1
fi

cell=$(grep -a "\[tbclick\] listbox id=$LBROWS.*CProfile" "$log" | tail -1 | sed -n 's/.*-> \(row=[0-9]* col=[0-9]*\).*/\1/p')
if [ "$cell" = "row=1 col=3" ]; then echo "  wave table cell selected: $cell (AAA Cover)"
else echo "  wave table cell selected: ${cell:-none} — expected row=1 col=3 (AAA Cover) — FAIL"; fail=1; fi

# what ChooseSquad itself says is available, straight out of its listbox
avail=$(grep -a -A3 'AddString\[0\] "F84"' "$log" | grep -a 'AddString\[3\]' | head -1 | sed 's/.*"\([^"]*\)".*/\1/')
echo "  ChooseSquad's own Available column for F84: ${avail:-none}"
if ! grep -aq "\[tbclick\] listbox id=$LBROWS.*row=$SQROW.*ChooseSquad" "$log"; then
  echo "  the F84 row was never clicked — FAIL"; fail=1
else
  echo "  F84 row clicked: yes"
fi

caps=$(grep -a '\[edtbt\] caption' "$log" | sed 's/.*"\([^"]*\)".*/\1/')
if echo "$caps" | grep -aq '^Off Duty$'; then echo "  the flak slot started Off Duty: yes"
else echo "  the flak slot was never Off Duty — FAIL"; fail=1; fi
squad=$(echo "$caps" | grep -aoE '^F[0-9]+ \([0-9]+/[0-9]+\)$' | tail -1)
if [ -n "$squad" ]; then echo "  the flak slot now holds: $squad"
else echo "  the flak slot never took a squadron — FAIL"; fail=1; fi
if echo "$squad" | grep -aq '^F84'; then
  echo "  F84 was assigned despite reporting 0 available — FAIL (the availability rule did not hold)"; fail=1
else
  echo "  the unavailable squadron (F84) was refused: yes"
fi

if grep -aq "\[tbclick\] listbox id=$WEAPLIST.*row=2.*CWeapons" "$log"; then echo "  payload row 2 selected: yes"
else echo "  the payload list was never addressed — FAIL"; fail=1; fi
if echo "$caps" | grep -aqx "$WANT_STORES"; then echo "  stores now read: $WANT_STORES"
else echo "  stores never became \"$WANT_STORES\" — FAIL"; fail=1; fi

counts=$(grep -a -A3 "AddString\[0\] \"$TARGET" "$log" | grep -a 'AddString\[3\]' | sed 's/.*"\([^"]*\)".*/\1/' | tr -d ' ')
first=$(echo "$counts" | head -1); last=$(echo "$counts" | tail -1)
echo "  Mission Folder Flights for \"$TARGET\": $(echo "$counts" | tr '\n' ' ')"
if [ "${first:-x}" = "2" ] && [ "${last:-x}" = "3" ]; then echo "  flight count 2 -> 3 (the suppression flight): yes"
else echo "  flight count ${first:-none} -> ${last:-none} — expected 2 -> 3 — FAIL"; fail=1; fi

echo "----------------------------------------"
if [ "$fail" -eq 0 ]; then
  echo "PASS: EPIC K step 11 — the AAA-cover slot takes a squadron and rocket stores, and reaches the mission"
else
  echo "FAIL"; exit 1
fi
