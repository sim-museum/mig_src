#!/usr/bin/env bash
# port/frag_review.sh — gate for EPIC K step 14: the FRAG screen. Change the callsign, pick your
# aircraft, review the mission before flying.
#
# The PO's script, step 14: "Frag icon on the toolbar — change callsign, pick your aircraft, final
# review. You're lead by default."
#
# WHAT IT ASSERTS, and why each reads GAME STATE rather than pixels:
#   1. the frag screen is up            — FlyableAircraftAvailable=1 and LaunchFullPane(singlefrag).
#                                         S168's finding: `Frag` printed "-> fire" and did nothing,
#                                         because that trace is printed BEFORE the handler runs.
#   2. the review lists real pilots     — the roster is the "final review". Names come from the
#                                         game's own roster, not from a caption we invented.
#   3. the callsign CHANGES             — `Todays_Packages.pack[p][w][g].callname`, the value that
#                                         actually reached the package. A combo can repaint a new
#                                         caption without the write landing; that is precisely what
#                                         this must be able to fail on.
#   4. the aircraft seat CHANGES        — `MMC.playeracnum`, the seat the player flies, and it must
#                                         equal flight*4 + slot. CFragPilot::OnClickedPlayer refuses
#                                         a DEAD pilot's slot and one taken by another comms player,
#                                         so a click that legitimately does nothing is
#                                         indistinguishable from a broken one without this.
#
# The frag screen hosts THREE CFragPilot sub-dialogs — one per package — with identical control ids,
# so `@CFragPilot` is ambiguous with itself. `@CFragPilot#N` names the Nth by SCREEN POSITION
# (top-to-bottom): map order is by pointer, and "the second row" has to mean the one the player sees
# second (S173). The ambiguity warning added in S171 is what caught this.
set -u
export MA_NO_HARDWARE="${MA_NO_HARDWARE:-1}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
. "$ROOT/port/gate_lib.sh"          # S171: assert_no_crash / assert_recipe_ran
WMIG="${WMIG:-$ROOT/build/wmig}"
BOB_DRIVE_C="${BOB_DRIVE_C:-$HOME/sgl/TUE/MigAlley/WP/drive_c}"
RUNDIR="$BOB_DRIVE_C/rowan/mig"
OUT="${OUT:-/tmp/ma_frag_gate}"
TMO="${TMO:-480}"
TARGET="${TARGET:-Wonju}"
AUTHORISE=2023; LBFILE=1055; LBROWS=2018; FRAG=2126; CALLNAME=2356; SLOT=2144
INST="${INST:-1}"; CALLROW="${CALLROW:-4}"
mkdir -p "$OUT"
[ -x "$WMIG" ] || { echo "no binary at $WMIG" >&2; exit 2; }

SAVEFILE="$RUNDIR/SaveGame/Auto Save.sav"
PIN="$ROOT/port/ref/save/campaign_pristine.sav"
STASH="$(mktemp -u /tmp/ma_frag_save.XXXXXX)"
[ -f "$SAVEFILE" ] && cp -a "$SAVEFILE" "$STASH"
restore_save() { [ -f "$STASH" ] && cp -a "$STASH" "$SAVEFILE" && rm -f "$STASH"; }
trap restore_save EXIT INT TERM
[ -f "$PIN" ] && cp -f "$PIN" "$SAVEFILE"

log="$OUT/frag.log"; ppm="$OUT/frag.ppm"; rm -f "$ppm"
SEQ="30,r3;65,#$LBFILE;100,#2063:1;420,#$AUTHORISE@DossierButtons;520,#$LBFILE@CLoad:r0"
SEQ="$SEQ;700,#$LBROWS@CMissionFolder:r0;800,#$FRAG@CMissionFolder"
SEQ="$SEQ;950,#$CALLNAME@CFragPilot#$INST;1010,#$CALLNAME@CFragPilot#$INST:r$CALLROW"
SEQ="$SEQ;1080,#$SLOT@CFragPilot#$INST"
echo "frag screen for the \"$TARGET\" mission — wmig"
# NB run this UNDER gl-lock; it does not take the lock itself (nesting deadlocks, S159).
( cd "$RUNDIR" && timeout -k 5 -s KILL "$TMO" env \
    SDL_VIDEODRIVER=dummy BOB_RUN_INIT=1 BOB_DRIVE_C="$BOB_DRIVE_C" MA_DISABLE_3D=1 \
    MA_IGNORE_SAVE_DATE=1 MA_TRACE_CLICK=1 MA_TRACE_OLE=1 MA_TRACE_FRAG=1 \
    MA_MAP_ITEM_SCAN=250 MA_MAP_CLICK_FIRST=1 MA_MAP_CLICK_NAME="$TARGET" \
    BOB_CLICKSEQ="$SEQ" MA_SHOT=1300 MA_SHOT_PATH="$ppm" "$WMIG" ) >"$log" 2>&1
pkill -x "$(basename "$WMIG")" 2>/dev/null

fail=0
assert_no_crash "$log" || fail=1
assert_recipe_ran "$log" || fail=1

# 1. the frag screen came up
fl=$(grep -a "\[frag\] FlyableAircraftAvailable" "$log" | head -1)
if echo "$fl" | grep -aq "FlyableAircraftAvailable=1"; then
  echo "  ${fl#\[frag\] }"
else echo "  the mission never reported a flyable aircraft — FAIL"; fail=1; fi

# 2. the review: the roster the player checks before flying
names=$(grep -a '\[edtbt\] caption' "$log" | sed 's/.*"\([^"]*\)".*/\1/' \
        | grep -aE '^[A-Z]([A-Za-z.]*)? [A-Z]' | sort -u)
n=$(echo "$names" | grep -ac . || true)
echo "  pilot roster: $n distinct name(s) — $(echo "$names" | head -3 | tr '\n' ',' | sed 's/,$//')..."
if [ "${n:-0}" -ge 8 ]; then echo "  the review lists a full roster: yes"
else echo "  the roster has only ${n:-0} names — expected >= 8 — FAIL"; fail=1; fi

# 3. the callsign reached the package
cs=$(grep -a "\[frag\] callname" "$log" | tail -1)
cs0=$(grep -a "\[frag\] callname" "$log" | head -1)
if [ -n "$cs" ] && [ "$cs" != "$cs0" ]; then
  echo "  ${cs0#\[frag\] }"
  echo "  ${cs#\[frag\] }"
  a=$(echo "$cs0" | sed -n 's/.*<- \([0-9]*\) .*/\1/p'); b=$(echo "$cs" | sed -n 's/.*<- \([0-9]*\) .*/\1/p')
  if [ "${a:-x}" != "${b:-y}" ]; then echo "  the callsign changed in the package: yes ($a -> $b)"
  else echo "  the callsign was rewritten with the SAME value — FAIL"; fail=1; fi
else echo "  the callsign never reached the package — FAIL"; fail=1; fi

# 4. the seat the player flies
seat=$(grep -a "\[frag\] player seat" "$log" | tail -1)
if [ -n "$seat" ]; then
  echo "  ${seat#\[frag\] }"
  ac=$(echo "$seat"  | sed -n 's/.*acnum=\([0-9]*\).*/\1/p')
  fl2=$(echo "$seat" | sed -n 's/.*(flight \([0-9]*\),.*/\1/p')
  sl=$(echo "$seat"  | sed -n 's/.*ac \([0-9]*\) in flight.*/\1/p')
  want=$(( fl2 * 4 + sl ))
  if [ "${ac:-x}" = "$want" ]; then echo "  the seat matches the slot clicked (flight $fl2, ac $sl -> acnum $ac): yes"
  else echo "  acnum=$ac but flight $fl2 slot $sl is seat $want — FAIL"; fail=1; fi
  if [ "${ac:-0}" -ne 0 ]; then echo "  the player is no longer in the default lead seat: yes"
  else echo "  the seat is still 0 (lead) — the click changed nothing — FAIL"; fail=1; fi
else echo "  no aircraft was ever selected — FAIL"; fail=1; fi

echo "----------------------------------------"
if [ "$fail" -eq 0 ]; then
  echo "PASS: EPIC K step 14 — the frag screen reviews the mission, and callsign and aircraft both reach game state"
else
  echo "FAIL"; exit 1
fi
