#!/usr/bin/env bash
# port/sysbox_exit.sh — gate for PO-1: the player can leave the campaign from the map.
#
# Reported as "no way to exit from campaign — no exit, resize etc widgets on upper right". The
# cluster is CSystemBox (IDC_FILES -> CMainFrame::OnBye, IDC_ZOOMIN -> resize, IDC_THUMBNAIL ->
# minimise). CMainFrame created it and RDialog enabled it, but the map idle drew only the two
# toolbars, so it was never on screen; S94 drew it (blank, no art); S97 gave it its art and turned
# it on by default.
#
# The click is resolved by CONTROL ID QUALIFIED BY HOST CLASS (`#10@CSystemBox`), never a
# coordinate: the box is positioned from the canvas's right edge, so a hardcoded point stops
# meaning anything the moment the screen size changes — which it did, twice, in S96 (see S95's
# map_icon_click.sh for the same rule and why it exists).
#
# S138 (PO-29): the X does NOT exit. CMainFrame::OnBye opens the game's Save / Yes / Cancel
# confirmation and acts on the answer -- this gate used to assert the old, wrong behaviour,
# because CDialog::DoModal was a `return -1` stub and -1 fell through OnBye's `rv<2` branch
# straight to the title screen. The PO reported exactly that: "clicking on X from map drops you
# to the landing page, not to a save/quit/cancel dialog as it should."
#
# So the gate now walks the real path: click the X, require the CONFIRMATION to appear, answer
# Yes, and require the player to have left the map. The "Yes" button is LOCATED in the captured
# modal (its yellow caption row) rather than hardcoded -- the dialog is centred, so its buttons
# move with the resolution.
#
# PASS requires all four:
#   1. the exit handler actually ran     — [evt_fire] id=10 ... CSystemBox -> HANDLER CALLED
#   2. the confirmation appeared         — MA_MODAL_SHOT captures it; its buttons are found in it
#   3. we actually left the campaign map — the post-answer screen differs sharply from the map ref
#   4. the process survived
# (3) exists for the same reason map_drag.sh asserts motion: "the handler was called" does not
# prove anything happened, and a screen that merely stopped updating would pass a weaker check.
set -u
# S118 (PO-12 phase 4): hardware is now a PLAYER SETTING, so an unpinned gate would test
# whichever renderer settings.mig happens to hold. Pin the software path here -- that is
# what these references were captured from. MA_NO_HARDWARE=0 runs the same gate on the
# hardware path.
export MA_NO_HARDWARE="${MA_NO_HARDWARE:-1}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WMIG="${WMIG:-$ROOT/build/wmig}"
BOB_DRIVE_C="${BOB_DRIVE_C:-$HOME/sgl/TUE/MigAlley/WP/drive_c}"
RUNDIR="$BOB_DRIVE_C/rowan/mig"
OUT="${OUT:-/tmp/ma_sysbox}"
TMO="${TMO:-130}"
NAV="30,r3;65,#1055;100,#2063:1"

mkdir -p "$OUT"
[ -x "$WMIG" ] || { echo "no binary at $WMIG" >&2; exit 2; }

SAVEFILE="$RUNDIR/SaveGame/Auto Save.sav"
PIN="$ROOT/port/ref/save/campaign_pristine.sav"
STASH="$(mktemp -u /tmp/ma_sysbox_save.XXXXXX)"
[ -f "$SAVEFILE" ] && cp -a "$SAVEFILE" "$STASH"
restore_save() { [ -f "$STASH" ] && cp -a "$STASH" "$SAVEFILE" && rm -f "$STASH"; }
trap restore_save EXIT INT TERM
[ -f "$PIN" ] && cp -f "$PIN" "$SAVEFILE"

log="$OUT/exit.log"; ppm="$OUT/exit.ppm"; modal="$OUT/modal.ppm"
rm -f "$ppm" "$modal"

run() {  # $1=extra clicks  $2=log
  cp -f "$PIN" "$SAVEFILE"
  ( cd "$RUNDIR" && timeout -k 5 -s KILL "$TMO" env \
      SDL_VIDEODRIVER=dummy BOB_RUN_INIT=1 BOB_DRIVE_C="$BOB_DRIVE_C" MA_DISABLE_3D=1 \
      MA_IGNORE_SAVE_DATE=1 MA_TRACE_OLE=1 MA_TRACE_CLICK=1 \
      BOB_CLICKSEQ="$NAV;250,#10@CSystemBox${1:+;$1}" \
      MA_MODAL_SHOT="$modal" MA_SHOT=330 MA_SHOT_PATH="$ppm" \
      "$WMIG" ) >"$2" 2>&1
  RC=$?                    # the GAME's status: pkill below would otherwise overwrite it, and
  pkill -x "$(basename "$WMIG")" 2>/dev/null   # pkill exits 1 when nothing matched -> "CRASH"
  return 0
}

# pass 1: click the X and photograph the confirmation it opens
run "" "$log"
rc=$RC

echo "system box exit — $(basename "$WMIG")"
ok=0
fired=$(grep -ac "evt_fire] id=10 .*CSystemBox -> HANDLER CALLED" "$log")
if [ "$fired" -gt 0 ]; then echo "  exit handler (IDC_FILES -> OnBye) called: yes"
else echo "  exit handler NOT called — the click never reached CSystemBox"; ok=1; fi

if ! grep -aq "\[modal\] begin" "$log"; then
  echo "  the confirmation dialog never opened — the X quit without asking"
  echo "  RESULT: FAIL — see $log"; exit 1
fi
MRECT=$(grep -a '\[modal\] begin' "$log" | head -1 |
        sed 's/.*at (\([0-9]*\),\([0-9]*\)) size \([0-9]*\)x\([0-9]*\).*/\1 \2 \3 \4/')
echo "  confirmation opened at ${MRECT// /,}"

# locate the middle button ("Yes") among the modal's yellow captions
YES=$(python3 - "$modal" $MRECT <<'PY2'
import sys
from PIL import Image
im = Image.open(sys.argv[1]).convert('RGB'); px = im.load()
# Search ONLY the dialog's own rect, which it reports when it opens. The first cut searched the
# whole canvas for the strongest yellow row and found the MAP's chrome -- the modal is drawn over
# a live campaign map, so "the yellowest row on screen" is not the dialog.
ox, oy, mw, mh = (int(v) for v in sys.argv[2:6])
x0, y0, x1, y1 = ox, oy, min(ox + mw, im.size[0]), min(oy + mh, im.size[1])
def isyellow(x, y):
    r, g, b = px[x, y]; return r > 170 and g > 140 and b < 100
best = (0, -1)
for y in range(y0, y1):
    n = sum(1 for x in range(x0, x1, 2) if isyellow(x, y))
    if n > best[0]: best = (n, y)
if best[0] < 6: print("NONE"); raise SystemExit
y = best[1]
xs = [x for x in range(x0, x1) if isyellow(x, y)]
runs = []; st = xs[0]; prev = xs[0]
for x in xs[1:]:
    if x - prev > 12: runs.append((st, prev)); st = x
    prev = x
runs.append((st, prev))
# Save | Yes | Cancel -> the middle run is the one that quits (IDC_CANCEL -> EndDialog(1))
if len(runs) < 3: print("NONE"); raise SystemExit
a, b = runs[1]
print("%d %d" % ((a+b)//2, y))
PY2
)
if [ "$YES" = "NONE" ] || [ -z "$YES" ]; then
  echo "  could not find the confirmation's buttons in $modal"
  echo "  RESULT: FAIL"; exit 1
fi
echo "  \"Yes\" located at pixel (${YES// /,})"

# pass 2: click the X, then answer Yes
run "300,${YES// /,}" "$OUT/answer.log"
rc=$RC
grep -aq "\[modal\] click .* -> taken" "$OUT/answer.log" \
  || { echo "  the answer click missed the dialog"; echo "  RESULT: FAIL"; exit 1; }
log="$OUT/answer.log"

if [ "$rc" -ne 0 ] && [ "$rc" -ne 124 ] && [ "$rc" -ne 137 ]; then
  echo "  RESULT: CRASH (exit $rc) — see $log"; exit 1
fi

# Did we actually leave? Compare against the MAP AS CAPTURED IN THIS RUN (the modal shot, which
# is the live map with the dialog over it), outside the dialog's rect -- same canvas, same
# resolution, so this is a real pixel comparison. The previous version compared against the
# 800x600 committed reference and short-circuited on the size mismatch ("different size, must
# have left"), which would have passed with the map still on screen.
python3 - "$ppm" "$modal" $MRECT <<'PYEXIT'
import sys
from PIL import Image
shot, mapshot = sys.argv[1], sys.argv[2]
ox, oy, mw, mh = (int(v) for v in sys.argv[3:7])
try:
    a = Image.open(shot).convert("RGB"); b = Image.open(mapshot).convert("RGB")
except Exception as e:
    print("  left the campaign map: UNKNOWN - %s" % e); sys.exit(1)
if a.size != b.size:
    print("  left the campaign map: UNKNOWN - captures differ in size %s vs %s" % (a.size, b.size))
    sys.exit(1)
pa, pb = a.load(), b.load()
w, h = a.size
diff = tot = 0
for y in range(0, h, 2):
    inrow = oy <= y < oy + mh
    for x in range(0, w, 2):
        if inrow and ox <= x < ox + mw:   # skip where the dialog itself was drawn
            continue
        tot += 1
        if pa[x, y] != pb[x, y]: diff += 1
pct = 100.0 * diff / max(tot, 1)
print("  left the campaign map: %s (%.1f%% of the map area changed)" % ("yes" if pct > 25 else "NO", pct))
sys.exit(0 if pct > 25 else 1)
PYEXIT
