#!/usr/bin/env bash
# GATE: PO-75 -- in-flight text must keep its spaces.
#
# The PO's campaign video showed radio text as "ToViper2: wearerolling": letters spaced
# correctly, word gaps almost gone. S371 measured the cause -- the port synthesises glyph widths
# from a substituted TrueType face instead of reading Rowan's font map, and that face gives space
# an advance of 2 px while letters get 5-6. The engine's own table says a space is WIDER than a
# letter (bigWidths[' ']=8 vs bigWidths['o']=7). The fix takes the engine's metric for glyphs with
# an EMPTY bitmap only.
#
# This asserts THREE things, and the third is the one that stops the fix being cheated:
#   1. space's advance equals the engine's own bigWidths value (the fix worked),
#   2. it is not the old synthesised value (the fix is actually in),
#   3. at least one INKED glyph still has body != bigWidths -- proving the fix did NOT just copy
#      the whole table over the synthesised widths, which would relayout every string in the game
#      while making assertion 1 pass perfectly.
set -u
LOG=/tmp/ma_spacefix/fly.log; mkdir -p /tmp/ma_spacefix
SECS=${SECS:-200}
MIG="$HOME/sgl/TUE/MigAlley/WP/drive_c/rowan/mig"
echo "PO-75 space-advance gate -- ${SECS}s sortie"
cd "$MIG" || { echo "  FAIL: no game dir"; exit 1; }
env BOB_RUN_INIT=1 BOB_DRIVE_C="$HOME/sgl/TUE/MigAlley/WP/drive_c" \
    MA_ENABLE_3D=1 MA_TRACE_3D=1 MA_TRY_HARDWARE=1 MA_TRACE_FONTW=1 \
    BOB_CLICKSEQ="40,r1;95,r0" \
    timeout -k 5 "$SECS" "$HOME/ma/build/wmig" > "$LOG" 2>&1

row=$(grep -a "\[fontw\] ch= 32 ' '" "$LOG" | tail -1)
[ -z "$row" ] && { echo "  INCONCLUSIVE: the font instrument never reported for space --"
                   echo "                the sortie did not build the 3D overlay font."; exit 2; }
body=$(echo "$row" | sed -n "s/.*body=\([0-9]*\).*/\1/p")
big=$(echo  "$row" | sed -n "s/.*bigWidths=\([0-9]*\).*/\1/p")
echo "  space: body=$body  bigWidths=$big"
[ "${body:-0}" -eq "${big:-1}" ] || { echo "  FAIL: space advance $body != the engine's $big -- words will run together."; exit 1; }
[ "${body:-0}" -gt 3 ] || { echo "  FAIL: space advance is still the synthesised value ($body)."; exit 1; }

# Control: an inked glyph must still differ, or the fix has overwritten every width.
diff_seen=0
for ch in 111 109 105 102; do
  r=$(grep -a "\[fontw\] ch=$ch " "$LOG" | tail -1)
  [ -z "$r" ] && continue
  b=$(echo "$r" | sed -n "s/.*body=\([0-9]*\).*/\1/p"); w=$(echo "$r" | sed -n "s/.*bigWidths=\([0-9]*\).*/\1/p")
  [ "${b:-0}" -ne "${w:-0}" ] && { echo "  control: inked glyph ch=$ch keeps body=$b vs bigWidths=$w (untouched)"; diff_seen=1; break; }
done
[ "$diff_seen" -eq 1 ] || { echo "  FAIL: every glyph now matches bigWidths -- the fix has relaid out ALL text, not just spaces."; exit 1; }
echo "  PASS: spaces take the engine's own width; inked glyphs are untouched."
exit 0
