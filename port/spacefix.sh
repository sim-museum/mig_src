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
#
# GOLDMATCH-MA-1 S3 (2026-09-18): THE PREMISE ABOVE WAS REFUTED BY THE GOLD. 260915_ma_campaign.mp4's
# HUD strip reads "Speed:378Kts" -- the ORIGINAL collapses the space too (bigWidths[] is
# StrPixelLen2's layout table, not the glyph advance, and the original fills body[] from GDI the
# way the port fills it from stb_truetype). The wide space is now OPT-IN (MA_SPACEFIX=1). This gate
# therefore runs TWO arms:
#   default        -- the gold's behaviour: space advance is NARROW (2 <= body < bigWidths),
#   MA_SPACEFIX=1  -- the knob still works: body == bigWidths, and the inked-glyph control holds.
# Logs live under $HOME/ma-gates (never /tmp -- project rule).
set -u
OUTD="${OUTD:-$HOME/ma-gates/ma_spacefix}"; mkdir -p "$OUTD"
SECS=${SECS:-200}
MIG="$HOME/sgl/TUE/MigAlley/WP/drive_c/rowan/mig"
echo "PO-75 space-advance gate -- ${SECS}s sortie, two arms (default = gold's narrow space; MA_SPACEFIX=1 = wide)"
cd "$MIG" || { echo "  FAIL: no game dir"; exit 1; }
sortie() {  # $1 arm name, $2 extra env
  local log="$OUTD/$1.log"
  env BOB_RUN_INIT=1 BOB_DRIVE_C="$HOME/sgl/TUE/MigAlley/WP/drive_c" \
      MA_ENABLE_3D=1 MA_TRACE_3D=1 MA_TRY_HARDWARE=1 MA_TRACE_FONTW=1 \
      BOB_CLICKSEQ="40,r1;95,r0" $2 \
      timeout -k 5 "$SECS" "$HOME/ma/build/wmig" > "$log" 2>&1
  local row; row=$(grep -a "\[fontw\] ch= 32 ' '" "$log" | tail -1)
  [ -z "$row" ] && { echo "  INCONCLUSIVE ($1): the font instrument never reported for space -- the sortie did not build the 3D overlay font."; return 2; }
  SP_BODY=$(echo "$row" | sed -n "s/.*body=\([0-9]*\).*/\1/p"); SP_BIG=$(echo "$row" | sed -n "s/.*bigWidths=\([0-9]*\).*/\1/p")
  echo "  $1: space body=$SP_BODY  bigWidths=$SP_BIG"
  return 0
}
FAIL=0
# Arm 1: default -- the gold's narrow space.
sortie default "" || exit 2
if [ "${SP_BODY:-0}" -ge 2 ] && [ "${SP_BODY:-0}" -lt "${SP_BIG:-8}" ]; then echo "  default: PASS -- narrow space, as the gold draws it"
else echo "  default: FAIL -- space advance $SP_BODY is not narrow (want 2 <= body < $SP_BIG; the gold's HUD strip has no visible space)"; FAIL=1; fi
# Arm 2: the opt-in wide space must still work, without touching inked glyphs.
sortie spacefix "MA_SPACEFIX=1" || exit 2
if [ "${SP_BODY:-0}" -eq "${SP_BIG:-1}" ]; then echo "  spacefix: PASS -- MA_SPACEFIX=1 gives the engine's width"
else echo "  spacefix: FAIL -- MA_SPACEFIX=1 did not set the space advance to bigWidths ($SP_BODY != $SP_BIG)"; FAIL=1; fi
diff_seen=0
for ch in 111 109 105 65 87; do
  r=$(grep -a "\[fontw\] ch=$ch " "$OUTD/spacefix.log" | tail -1); [ -z "$r" ] && continue
  b=$(echo "$r" | sed -n "s/.*body=\([0-9]*\).*/\1/p"); w=$(echo "$r" | sed -n "s/.*bigWidths=\([0-9]*\).*/\1/p")
  [ "${b:-0}" -ne "${w:-0}" ] && { echo "  control: inked glyph ch=$ch keeps body=$b vs bigWidths=$w (untouched)"; diff_seen=1; break; }
done
[ "$diff_seen" -eq 1 ] || { echo "  FAIL: every glyph matches bigWidths under MA_SPACEFIX=1 -- the knob relaid out ALL text, not just spaces."; FAIL=1; }
[ "$FAIL" -eq 0 ] && echo "  PASS: default is the gold's narrow space; MA_SPACEFIX=1 widens spaces only." 
exit $FAIL
