#!/usr/bin/env bash
# port/damage_elements.sh — gate for EPIC K step 6: the dossier's Damage tab lists the target's
# elements, so the player can see what a supply dump is made of before planning the strike.
#
# The PO's Wonju walkthrough, step 6: "Zoom in on the dump icon until the sub-target icons appear,
# then Damage tab -> top combo-box to list them. Groups of warehouses; you don't know which hold
# stores, so plan to hit as many as possible."
#
# It needed three things the port did not have:
#   - a way to address a TAB by index          (`#1002:r1`, resolved through CRTabsCtrl's own rect list)
#   - COMBOS inside an OOB dialog to be clickable at all -- they were drawn and inert, the same
#     shape as S87 (listbox rows) and S140 (scroll bars), one control type later
#   - a way to address a row of an OPEN dropdown (`#2398:r0`), so the recipe spells the user's TWO
#     clicks as two entries rather than one scaffold click that opens and selects at once
#
# ASSERTIONS:
#   1. the target is found BY NAME
#   2. the Damage tab takes the click
#   3. the combo opens a dropdown with >= 2 rows   — "<target>: All elements" and its damage state
#   4. picking row 0 fires the selection            — [ddclick] dropdown row 0
#   5. the element list then has REAL ROWS          — measured as caption ink in the list body.
#      Without 5, 1-4 all pass on a dialog that switched mode and drew nothing.
#
# NOTE what this gate does NOT assert: that the list fits its dialog. It does not -- the rows run
# past the buttons and over the map. That is PO-43 (CRListBoxCtrl::ResizeToFit, located in S155,
# two fixes tried and reverted), and this gate reproducing it on a SECOND dialog is useful
# information: PO-43 is not Intelligence-specific.
set -u
export MA_NO_HARDWARE="${MA_NO_HARDWARE:-1}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
. "$ROOT/port/gate_lib.sh"          # S171: assert_no_crash / assert_recipe_ran
assert_clean_start || exit 2          # S177: a stray wmig makes this gate report a content failure
WMIG="${WMIG:-$ROOT/build/wmig}"
BOB_DRIVE_C="${BOB_DRIVE_C:-$HOME/sgl/TUE/MigAlley/WP/drive_c}"
RUNDIR="$BOB_DRIVE_C/rowan/mig"
OUT="${OUT:-/tmp/ma_damage}"
TMO="${TMO:-240}"
TARGET="${TARGET:-Wonju}"
IDJ_TABCTRL=1002; IDC_COMBO_ELEMENTS=2398; DAMAGE_TAB=1
NAV="30,r3;65,#1055;100,#2063:1"
mkdir -p "$OUT"
[ -x "$WMIG" ] || { echo "no binary at $WMIG" >&2; exit 2; }

SAVEFILE="$RUNDIR/SaveGame/Auto Save.sav"
PIN="$ROOT/port/ref/save/campaign_pristine.sav"
STASH="$(mktemp -u /tmp/ma_dmg_save.XXXXXX)"
[ -f "$SAVEFILE" ] && cp -a "$SAVEFILE" "$STASH"
restore_save() { [ -f "$STASH" ] && cp -a "$STASH" "$SAVEFILE" && rm -f "$STASH"; }
trap restore_save EXIT INT TERM
[ -f "$PIN" ] && cp -f "$PIN" "$SAVEFILE"

log="$OUT/damage.log"; ppm="$OUT/damage.ppm"; rm -f "$ppm"
echo "Damage tab -> element list for \"$TARGET\" — wmig"
# Run this UNDER gl-lock; it does not take the lock itself (nesting deadlocks, S159).
( cd "$RUNDIR" && timeout -k 5 -s KILL "$TMO" env \
    SDL_VIDEODRIVER=dummy BOB_RUN_INIT=1 BOB_DRIVE_C="$BOB_DRIVE_C" MA_DISABLE_3D=1 \
    MA_IGNORE_SAVE_DATE=1 MA_TRACE_OOB=1 MA_TRACE_CLICK=1 MA_TRACE_TABS=1 \
    MA_MAP_ITEM_SCAN=250 MA_MAP_CLICK_FIRST=1 MA_MAP_CLICK_NAME="$TARGET" \
    BOB_CLICKSEQ="$NAV;420,#$IDJ_TABCTRL:r$DAMAGE_TAB;500,#$IDC_COMBO_ELEMENTS;580,#$IDC_COMBO_ELEMENTS:r0" \
    MA_SHOT=720 MA_SHOT_PATH="$ppm" "$WMIG" ) >"$log" 2>&1
pkill -x "$(basename "$WMIG")" 2>/dev/null

fail=0
assert_no_crash "$log" || fail=1
name=$(grep -a "\[mapitem\] name match" "$log" | head -1)
if [ -n "$name" ]; then echo "  ${name#*] }"; else echo "  no map item named \"$TARGET\" — FAIL"; fail=1; fi
if grep -aq "\[tabs\] click" "$log"; then echo "  Damage tab took the click: yes"
else echo "  the tab bar never took a click — FAIL"; fail=1; fi
dd=$(grep -a "\[tbclick\] combo id=$IDC_COMBO_ELEMENTS open dropdown" "$log" | tail -1)
if [ -n "$dd" ]; then echo "  ${dd#*] }"; else echo "  the combo never opened — FAIL"; fail=1; fi
if grep -aq "\[ddclick\] dropdown row 0" "$log"; then echo "  dropdown row 0 selected: yes"
else echo "  dropdown row 0 was never selected — FAIL"; fail=1; fi

if [ -s "$ppm" ]; then
  python3 - "$ppm" "$log" <<'PYEOF'
import re, sys
from PIL import Image
im = Image.open(sys.argv[1]).convert('RGB')
rect = None
for raw in open(sys.argv[2], 'rb'):
    line = raw.decode('latin-1')
    if 'art=26641' in line and '-> dialog' in line:
        rect = re.search(r'-> dialog (\d+)x(\d+) at \((-?\d+),(-?\d+)\)', line)
if not rect:
    print("  could not locate the dossier rect in the trace"); sys.exit(1)
w, h, x, y = (int(g) for g in rect.groups())
# the element list sits under the combo; sample the dialog's own width, below the tab+combo band
body = im.crop((max(x, 0), y + 100, min(x + w, im.size[0]), min(y + h, im.size[1])))
ink = sum(1 for r, g, b in body.getdata() if r > 140 and g > 120 and b < 120)
print("  element-row ink in the dossier body: %d px%s" % (ink, "" if ink > 1500 else "  <-- no rows"))
sys.exit(0 if ink > 1500 else 1)
PYEOF
  [ $? -eq 0 ] || fail=1
else
  echo "  no capture was produced — FAIL"; fail=1
fi

echo "----------------------------------------"
if [ "$fail" -eq 0 ]; then
  echo "PASS: the Damage tab lists the target's elements (capture $ppm)"
else
  echo "FAIL"; exit 1
fi
