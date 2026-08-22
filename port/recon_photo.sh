#!/usr/bin/env bash
# port/recon_photo.sh — gate for EPIC K step 5: the target dossier's PHOTO button opens the 3D
# recon view of that target.
#
# WHY THIS EXISTS. The PO's Wonju walkthrough goes: find the supply dump on the map -> click it for
# the dossier -> **Photo** -> zoom out and memorise the terrain. Driving that with 3D enabled left
# the game with no further idle output. Under gdb the sim thread had ALREADY taken SIGSEGV in
# Inst3d::moveloop while the main thread was still inside Inst3d::Inst3d(bool), down in
# Three_Dee.InitialiseCache(): the worker was started ~40 lines before the landscape cache it reads
# existed. S69 fixed exactly that race in the NO-ARGUMENT Inst3d ctor and left its map-view twin
# alone; S160 applied the same deferral here.
#
# It was invisible to every gate we had, because they all run MA_DISABLE_3D=1 -- with 3D disabled
# the photo dialog shows its loader art and never launches 3D at all. That is why this gate runs
# WITH 3D and asserts on the rendered frame.
#
# FOUR assertions, because any one alone can pass for the wrong reason:
#   1. the target is found BY NAME          — proves the recipe addressed the right icon, not "the
#                                             first item", which on this save is a bridge (S158)
#   2. the Photo button's click is taken    — proves the click reached DossierButtons, not the map
#   3. a 3D frame is produced               — proves Launch3d got past the ctor at all
#   4. the frame is a RENDERED SCENE        — >=64 distinct colours AND no single colour covering
#                                             >=70% of it. NOT "thousands of colours": the software
#                                             rasterizer is 8-bit palettised and can never produce
#                                             more than 256, so a threshold above that fails a
#                                             perfectly good frame. Measure something the renderer
#                                             can actually produce (the S64 rule). The recon frame
#                                             measures 193 colours / 31.8% top share; a black or
#                                             flat frame is 1-2 colours at ~100%.
set -u
export MA_NO_HARDWARE="${MA_NO_HARDWARE:-1}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WMIG="${WMIG:-$ROOT/build/wmig}"
BOB_DRIVE_C="${BOB_DRIVE_C:-$HOME/sgl/TUE/MigAlley/WP/drive_c}"
RUNDIR="$BOB_DRIVE_C/rowan/mig"
OUT="${OUT:-/tmp/ma_recon}"
TMO="${TMO:-260}"
TARGET="${TARGET:-Wonju}"
IDC_PHOTO=2078
NAV="30,r3;65,#1055;100,#2063:1"        # title -> Campaign -> load "Auto Save" -> map
mkdir -p "$OUT"
[ -x "$WMIG" ] || { echo "no binary at $WMIG" >&2; exit 2; }

# PIN a known campaign save (S94): every icon on the map comes from campaign state, and the
# harness must put the player's own save back (S81 -- the ASan campaign gates once ate it).
SAVEFILE="$RUNDIR/SaveGame/Auto Save.sav"
PIN="$ROOT/port/ref/save/campaign_pristine.sav"
STASH="$(mktemp -u /tmp/ma_recon_save.XXXXXX)"
[ -f "$SAVEFILE" ] && cp -a "$SAVEFILE" "$STASH"
restore_save() { [ -f "$STASH" ] && cp -a "$STASH" "$SAVEFILE" && rm -f "$STASH"; }
trap restore_save EXIT INT TERM
[ -f "$PIN" ] && cp -f "$PIN" "$SAVEFILE"

log="$OUT/recon.log"; ppm="/tmp/maback.ppm"; rm -f "$ppm"
echo "dossier Photo -> 3D recon of \"$TARGET\" — wmig"
# NB no gl-lock here: run this gate UNDER gl-lock like its siblings. Nesting deadlocks and
# presents as an empty log (see map_filter.sh's header, and S159's panel_click.sh note).
( cd "$RUNDIR" && timeout -k 5 -s KILL "$TMO" env \
    SDL_VIDEODRIVER=dummy BOB_RUN_INIT=1 BOB_DRIVE_C="$BOB_DRIVE_C" MA_IGNORE_SAVE_DATE=1 \
    MA_TRACE_CLICK=1 MA_MAP_ITEM_SCAN=250 MA_MAP_CLICK_FIRST=1 MA_MAP_CLICK_NAME="$TARGET" \
    BOB_CLICKSEQ="$NAV;420,#$IDC_PHOTO@DossierButtons" \
    MA_DUMP_BACK="${DUMP:-120}" BOB_EXIT_AFTER_DUMP=1 "$WMIG" ) >"$log" 2>&1
rc=$?
pkill -x "$(basename "$WMIG")" 2>/dev/null

fail=0
name=$(grep -a "\[mapitem\] name match" "$log" | head -1)
if [ -n "$name" ]; then echo "  ${name#*] }"; else echo "  no map item named \"$TARGET\" — FAIL"; fail=1; fi

if grep -aq "\[oobclick\].*took" "$log"; then
  echo "  Photo button took the click: yes"
else
  echo "  Photo button never took the click — FAIL"; fail=1
fi

# A signal death is a hard fail; 124 (our own timeout) is expected -- the recon view has no exit
# in this recipe, and BOB_EXIT_AFTER_DUMP only fires if a frame was reached.
if [ "$rc" -ge 128 ]; then echo "  process died on signal $((rc-128)) — FAIL"; fail=1; fi
if grep -aq "AddressSanitizer\|Segmentation fault" "$log"; then echo "  crash reported in the log — FAIL"; fail=1; fi

if [ -s "$ppm" ]; then
  cp -f "$ppm" "$OUT/recon.ppm"
  python3 - "$OUT/recon.ppm" <<'PY'
import sys
from collections import Counter
from PIL import Image
im = Image.open(sys.argv[1]).convert('RGB')
c = Counter(im.getdata())
tot = float(im.size[0] * im.size[1])
cols = len(c)
top = c.most_common(1)[0][1] / tot
ok = cols >= 64 and top < 0.70
print("  frame %dx%d, %d distinct colours, top colour %.1f%%%s"
      % (im.size[0], im.size[1], cols, 100 * top,
         "" if ok else "  <-- flat: not a rendered scene"))
sys.exit(0 if ok else 1)
PY
  [ $? -eq 0 ] || fail=1
else
  echo "  no 3D frame was produced — FAIL"; fail=1
fi

echo "----------------------------------------"
if [ "$fail" -eq 0 ]; then
  echo "PASS: Photo opens the 3D recon of the named target (capture $OUT/recon.ppm)"
else
  echo "FAIL"; exit 1
fi
