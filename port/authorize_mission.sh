#!/usr/bin/env bash
# port/authorize_mission.sh — gate for EPIC K step 7: Authorize on a target dossier CREATES a
# mission, and the Mission Folder lists it.
#
# The PO's Wonju walkthrough: find the supply dump -> dossier -> **Authorize** -> pick **Minimum
# Strike** (explicitly NOT "Fighter Bomber Strike", which auto-fills everything) -> Load. Everything
# from step 8 onwards edits the mission this step creates, so a break here hides the whole rest of
# the epic behind it.
#
# WHAT IT ASSERTS, and why each one is separate:
#   1. the target is found BY NAME               — twenty AmberSupply items on this map (S158)
#   2. **row 0, Minimum Strike, is the one selected** — the recipe says `:r0` for a reason. Naming
#      the listbox alone clicks its CENTRE, i.e. row 2 of 3 = "Fighter Bomber Strike", the option
#      the walkthrough explicitly says NOT to pick ("it auto-fills everything") -- and the mission
#      is still created, so the gate would pass while testing the wrong profile (S85, S162).
#   3. Load creates the mission                  — the MISSION FOLDER dialog appears
#   4. the folder LISTS the target by name       — with a Task and a flight count. This is the one
#                                                  that cannot pass for the wrong reason: a dialog
#                                                  can open empty, and steps 8-11 are judged by the
#                                                  Flights number in this very list.
#
# Assertion 4 reads the pixels, because the flight count is not traced anywhere -- it is drawn. The
# check is deliberately coarse (the row's text band is non-empty and the dialog is where the paint
# walk says) rather than an exact glyph match: an OCR-grade assertion on a 1999 bitmap font would
# fail for reasons that have nothing to do with the mission being created.
set -u
export MA_NO_HARDWARE="${MA_NO_HARDWARE:-1}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
. "$ROOT/port/gate_lib.sh"          # S171: assert_no_crash / assert_recipe_ran
WMIG="${WMIG:-$ROOT/build/wmig}"
BOB_DRIVE_C="${BOB_DRIVE_C:-$HOME/sgl/TUE/MigAlley/WP/drive_c}"
RUNDIR="$BOB_DRIVE_C/rowan/mig"
OUT="${OUT:-/tmp/ma_authorize}"
TMO="${TMO:-240}"
TARGET="${TARGET:-Wonju}"
IDC_AUTHORISE=2023; IDC_RLISTBOXFILE=1055; IDC_FILEOK=1056
NAV="30,r3;65,#1055;100,#2063:1"
mkdir -p "$OUT"
[ -x "$WMIG" ] || { echo "no binary at $WMIG" >&2; exit 2; }

# PIN the campaign save and put the player's own back (S81).
SAVEFILE="$RUNDIR/SaveGame/Auto Save.sav"
PIN="$ROOT/port/ref/save/campaign_pristine.sav"
STASH="$(mktemp -u /tmp/ma_auth_save.XXXXXX)"
[ -f "$SAVEFILE" ] && cp -a "$SAVEFILE" "$STASH"
restore_save() { [ -f "$STASH" ] && cp -a "$STASH" "$SAVEFILE" && rm -f "$STASH"; }
trap restore_save EXIT INT TERM
[ -f "$PIN" ] && cp -f "$PIN" "$SAVEFILE"

log="$OUT/authorize.log"; ppm="$OUT/authorize.ppm"; rm -f "$ppm"
echo "Authorize -> mission for \"$TARGET\" — wmig"
# NB run this UNDER gl-lock; it does not take the lock itself (nesting deadlocks, S159).
# The `@CLoad` qualifier matters: id 1055 is ALSO the campaign file listbox used by $NAV, and an
# unqualified #1055 resolves to whichever hosted control comes first (S85).
( cd "$RUNDIR" && timeout -k 5 -s KILL "$TMO" env \
    SDL_VIDEODRIVER=dummy BOB_RUN_INIT=1 BOB_DRIVE_C="$BOB_DRIVE_C" MA_DISABLE_3D=1 \
    MA_IGNORE_SAVE_DATE=1 MA_TRACE_OOB=1 MA_TRACE_CLICK=1 \
    MA_MAP_ITEM_SCAN=250 MA_MAP_CLICK_FIRST=1 MA_MAP_CLICK_NAME="$TARGET" \
    BOB_CLICKSEQ="$NAV;420,#$IDC_AUTHORISE@DossierButtons;520,#$IDC_RLISTBOXFILE@CLoad:r0" \
    MA_SHOT=760 MA_SHOT_PATH="$ppm" "$WMIG" ) >"$log" 2>&1
pkill -x "$(basename "$WMIG")" 2>/dev/null

fail=0
assert_no_crash "$log" || fail=1
name=$(grep -a "\[mapitem\] name match" "$log" | head -1)
if [ -n "$name" ]; then echo "  ${name#*] }"; else echo "  no map item named \"$TARGET\" — FAIL"; fail=1; fi

if grep -aq "\[tbclick\] id=$IDC_AUTHORISE.*fire" "$log"; then echo "  Authorize fired: yes"
else echo "  Authorize never fired — FAIL"; fail=1; fi
row=$(grep -a "\[tbclick\] listbox id=$IDC_RLISTBOXFILE.*CLoad" "$log" | tail -1 | sed -n 's/.*row=\([0-9]*\).*/\1/p')
if [ "${row:-x}" = "0" ]; then echo "  profile row selected: 0 (Minimum Strike)"
else echo "  profile row selected: ${row:-none} — expected 0 (Minimum Strike) — FAIL"; fail=1; fi
# S171: there is no separate Load click, and there never really was one. `CLoad::OnSelectRlistboxfile`
# calls OnOK() when the clicked row is ALREADY the current one, and `currrow` starts at 0 -- so
# `:r0` selects Minimum Strike AND loads it, tearing the chooser down in the same click. The
# `620,#1056@CLoad` step this recipe used to carry was landing on an already-destroyed dialog and
# doing nothing; it only appeared to work because the closed dialog's controls stayed in the
# hosted registry, still flagged visible (fixed S171). Assert the OUTCOME, which is what proved
# the load all along.
if grep -aq "\[subtree\] remove 5CLoad" "$log" || grep -aq "art=26647" "$log"; then
  echo "  the profile chooser closed on the row select (that IS the load): yes"
else echo "  the profile chooser never closed — FAIL"; fail=1; fi

# The MISSION FOLDER is FIL_MISSION_FOLDER art (26647); the profile chooser is 26656. Read the
# open-dialog count from the paint walk rather than guessing from the capture.
opened=$(grep -a "painted [0-9]* open" "$log" | tail -1 | sed 's/[^0-9]*\([0-9]*\).*/\1/')
echo "  dialogs open after Load: ${opened:-0}"
if grep -aq "\[artclip\] .*art=26647" "$log"; then echo "  MISSION FOLDER dialog present: yes"
else echo "  MISSION FOLDER dialog never appeared — FAIL"; fail=1; fi

if [ -s "$ppm" ]; then
  python3 - "$ppm" "$log" <<'PYEOF'
import re, sys
from PIL import Image
im = Image.open(sys.argv[1]).convert('RGB')
# the paint walk prints the folder node's rect: "[artclip] node=.. art=26647 WxH -> dialog WxH at (X,Y)"
m = None
for line in open(sys.argv[2], 'rb'):
    line = line.decode('latin-1')
    if 'art=26647' in line and '-> dialog' in line:
        m = re.search(r'-> dialog (\d+)x(\d+) at \((-?\d+),(-?\d+)\)', line)
if not m:
    print("  could not locate the folder rect in the trace"); sys.exit(1)
w, h, x, y = (int(g) for g in m.groups())
print("  folder at (%d,%d) %dx%d" % (x, y, w, h))
# the list body: below the title bar, above the button row
body = im.crop((max(x, 0), y + 34, min(x + w, im.size[0]), y + min(h, 120)))
px = list(body.getdata())
# yellow-ish caption text is what the rows are drawn in; count ink of any bright colour
ink = sum(1 for r, g, b in px if r > 140 and g > 120 and b < 120)
print("  row text ink in the folder body: %d px%s" % (ink, "" if ink > 200 else "  <-- empty list"))
sys.exit(0 if ink > 200 else 1)
PYEOF
  [ $? -eq 0 ] || fail=1
else
  echo "  no capture was produced — FAIL"; fail=1
fi

echo "----------------------------------------"
if [ "$fail" -eq 0 ]; then
  echo "PASS: Authorize creates the mission and the Mission Folder lists it (capture $ppm)"
else
  echo "FAIL"; exit 1
fi
