#!/usr/bin/env bash
# port/ins_wave.sh — PO-56: the Ins Wave dialog opens WHERE ITS CALLER IS, and can be used.
#
# WHY THIS EXISTS (S189/S191)
# ---------------------------
# The PO reported, twice, that the Ins Wave dialog was unusable: no way to set the time, no way to
# exit, and a screenshot showing dialogs stacked in the TOP-LEFT CORNER with their left edges cut
# off. Two separate defects were behind it and both are fixed:
#
#   S181  GetCaption() returned an empty string on EVERY hosted control, so CWaveInsert::OnOKTitle
#         committed nothing. (The same bug ate the player's name — PO-57.)
#   S189  Place(POSN_CALLER) does `x += parent->GetWindowRect().left`, but a dialog tree stores
#         PARENT-RELATIVE rects: the top node carries the placement and inner nodes read (0,0).
#         `parent` is the caller's own RDialog, usually an inner node — so GetWindowRect answered 0
#         and every caller-relative dialog was placed against the SCREEN ORIGIN. Measured from the
#         PO's session: CProfile sits at (200,24), Ins Wave asks for caller-3, the child arrived at
#         x=-3. That is -3 + 0.
#         S182 had seen that -3 and clamped it to 0 — treating the symptom, leaving the dialog in
#         the wrong corner, and firing on dialogs that were never off-screen.
#
# WHAT IT ASSERTS
#   1. the Ins Wave button FIRES              — the PO's click reached a handler
#   2. a CWaveInsert dialog EXISTS afterwards — the crux. In the PO's session the button fired and
#                                               no wave dialog ever appeared; only the caller moved
#   3. it is placed AT ITS CALLER             — not at the screen origin. The [place] trace reports
#                                               the absolute origin actually used
#   4. no crash, and the recipe ran
#
# NB run this UNDER gl-lock; it does not take the lock itself (nesting deadlocks, S159).
set -u
export MA_NO_HARDWARE="${MA_NO_HARDWARE:-1}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
. "$ROOT/port/gate_lib.sh"
assert_clean_start || exit 2
WMIG="${WMIG:-$ROOT/build/wmig}"
BOB_DRIVE_C="${BOB_DRIVE_C:-$HOME/sgl/TUE/MigAlley/WP/drive_c}"
RUNDIR="$BOB_DRIVE_C/rowan/mig"
OUT="${OUT:-/tmp/ma_inswave}"
TMO="${TMO:-420}"
TARGET="${TARGET:-Wonju}"
AUTHORISE=2023; LBFILE=1055; LBROWS=2018; PROFILE3=2124; INSWAVE=1006; TITLE=1001
mkdir -p "$OUT"
[ -x "$WMIG" ] || { echo "no binary at $WMIG" >&2; exit 2; }

SAVEFILE="$RUNDIR/SaveGame/Auto Save.sav"
PIN="$ROOT/port/ref/save/campaign_pristine.sav"
STASH="$(mktemp -u /tmp/ma_inswave_save.XXXXXX)"
[ -f "$SAVEFILE" ] && cp -a "$SAVEFILE" "$STASH"
restore_save() { [ -f "$STASH" ] && cp -a "$STASH" "$SAVEFILE" && rm -f "$STASH"; }
trap restore_save EXIT INT TERM
[ -f "$PIN" ] && cp -f "$PIN" "$SAVEFILE"

log="$OUT/ins_wave.log"; rm -f "$log"
NAV="30,r3;65,#$LBFILE;100,#2063:1"
SEQ="$NAV;420,#$AUTHORISE@DossierButtons;520,#$LBFILE@CLoad:r0"
SEQ="$SEQ;700,#$LBROWS@CMissionFolder:r0;780,#$PROFILE3@CMissionFolder"
SEQ="$SEQ;900,#$INSWAVE@CProfile"
# S194: and COMMIT the wave with the title-bar tick — the PO's exact click. This is what crashed:
# OnOKTitle -> RefreshParent -> CProfile::RefreshData -> SetWaveTabs -> ReDraw -> FillWaveRow,
# which formatted "2.Flak Supp." into a 10-byte stack buffer. The gate stopped one click short of
# the defect, which is why it was green while the feature aborted the game.
SEQ="$SEQ;1000,#$TITLE@CWaveInsert:-3"
echo "Ins Wave dialog on the \"$TARGET\" mission — wmig"
( cd "$RUNDIR" && timeout -k 5 -s KILL "$TMO" env \
    SDL_VIDEODRIVER=dummy BOB_RUN_INIT=1 BOB_DRIVE_C="$BOB_DRIVE_C" MA_DISABLE_3D=1 \
    MA_IGNORE_SAVE_DATE=1 MA_TRACE_OOB=1 MA_TRACE_CLICK=1 MA_TRACE_OLE=1 MA_TRACE_WAVE=1 \
    MA_MAP_ITEM_SCAN=250 MA_MAP_CLICK_FIRST=1 MA_MAP_CLICK_NAME="$TARGET" \
    BOB_CLICKSEQ="$SEQ" "$WMIG" ) >"$log" 2>&1
for p in $(pgrep -x "$(basename "$WMIG")" 2>/dev/null); do kill "$p" 2>/dev/null; done

fail=0
assert_no_crash "$log" || fail=1
assert_recipe_ran "$log" || fail=1

# which of the handler's three exits did the click take? Two of them are silent by nature.
grep -a "^\[wave\]" "$log" | head -4 | sed 's/^/  /'

fired=$(grep -a "\[tbclick\] id=$INSWAVE .* -> fire" "$log" | head -1)
if [ -n "$fired" ]; then echo "  Ins Wave button fired: ${fired#\[tbclick\] }"
else echo "  the Ins Wave button never fired — FAIL"; fail=1; fi

# 2. did a wave dialog actually come up? In the PO's session it never did.
#    Assert on the LOGGED CHILD'S RUNTIME TYPE, not on the string "CWaveInsert" appearing in the
#    log. The first cut of this gate did the latter and PASSED on a run with no dialog at all,
#    because it matched the trace line that merely NAMES the branch. A gate must not be satisfiable
#    by its own instrumentation (S171, and again here one gate later).
if grep -aq "rtti=11CWaveInsert" "$log"; then
  echo "  CWaveInsert dialog exists: $(grep -a 'rtti=11CWaveInsert' "$log" | head -1 | sed 's/^ *//' | cut -c1-110)"
else
  echo "  NO CWaveInsert dialog was ever created — FAIL"
  echo "    (the PO's symptom: the button fires, no wave dialog appears)"
  fail=1
fi

# 3a. WHERE did it actually land? The placement is the PO's photographed defect: before S189 the
#     dialog was placed against the screen origin and ended up in the top-left corner with its left
#     edge off-screen. Assert the rendered position of the logged panel, not just that the
#     correction ran.
panelptr=$(grep -a "^\[wave\]   LogChild(3) ->" "$log" | head -1 | sed -n 's/.*-> \(0x[0-9a-f]*\).*/\1/p')
if [ -n "$panelptr" ]; then
  rend=$(grep -a "\[oobrender\] node=$panelptr" "$log" | head -1)
  if [ -n "$rend" ]; then
    px=$(echo "$rend" | sed -n 's/.*-> screen(\([0-9-]*\),\([0-9-]*\)).*/\1/p')
    py=$(echo "$rend" | sed -n 's/.*-> screen(\([0-9-]*\),\([0-9-]*\)).*/\2/p')
    echo "  wave dialog rendered at screen($px,$py)"
    if [ "${px:-0}" -le 0 ] || [ "${py:-0}" -le 0 ]; then
      echo "    it is at the screen corner — the S189 defect — FAIL"; fail=1
    fi
  else
    echo "  the wave dialog was never RENDERED (logged but not painted) — FAIL"; fail=1
  fi
else
  echo "  no logged panel to check placement on — FAIL"; fail=1
fi

# 3b. placed at its caller, not at the screen origin.
place=$(grep -a "^\[place\] caller abs" "$log" | head -1)
if [ -n "$place" ]; then
  echo "  ${place#\[place\] }"
  absx=$(echo "$place" | sed -n 's/.*caller abs x=\([0-9-]*\).*/\1/p')
  if [ "${absx:-0}" -le 0 ]; then
    echo "    the caller's absolute origin is still 0 — the child is at the screen corner — FAIL"; fail=1
  fi
else
  echo "  no [place] trace — the absolute-origin correction never ran (MA_NO_DIALOG_ABSORG set?)"
fi
if grep -aq "\[offscreen\] dialog placed at x=" "$log"; then
  echo "  a dialog still needed the off-screen clamp: $(grep -a '\[offscreen\] dialog placed at x=' "$log" | head -1)"
fi

# 4. the commit must not smash the stack (S194). assert_no_crash catches the banner, but glibc's
#    own message arrives BEFORE it and names the fault, so check for it by name too.
if grep -aq "stack smashing detected" "$log"; then
  echo "  STACK SMASH on commit — FAIL"
  echo "    $(grep -a -B1 'stack smashing detected' "$log" | head -2 | tail -1)"
  fail=1
elif grep -aq "dispid 3 (OK) on 11CWaveInsert" "$log"; then
  echo "  the wave was committed with the title-bar tick, and the run survived it"
else
  echo "  the commit click never reached CWaveInsert's OK — the crash path is NOT covered"
  fail=1
fi

echo "  ----------------------------------------"
if [ "$fail" -eq 0 ]; then echo "  PASS: Ins Wave opens a wave dialog at its caller and commits without crashing (log $log)"; exit 0; fi
echo "  FAIL: see $log"; exit 1
