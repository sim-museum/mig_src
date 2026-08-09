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
# PASS requires all three:
#   1. the exit handler actually ran     — [evt_fire] id=10 ... CSystemBox -> HANDLER CALLED
#   2. we actually left the campaign map — the post-click screen differs sharply from the map ref
#   3. the process survived
# (2) exists for the same reason map_drag.sh asserts motion: "the handler was called" does not
# prove anything happened, and a screen that merely stopped updating would pass a weaker check.
set -u
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

log="$OUT/exit.log"; ppm="$OUT/exit.ppm"; rm -f "$ppm"
( cd "$RUNDIR" && timeout -k 5 -s KILL "$TMO" env \
    SDL_VIDEODRIVER=dummy BOB_RUN_INIT=1 BOB_DRIVE_C="$BOB_DRIVE_C" MA_DISABLE_3D=1 \
    MA_IGNORE_SAVE_DATE=1 MA_TRACE_OLE=1 MA_TRACE_CLICK=1 \
    BOB_CLICKSEQ="$NAV;250,#10@CSystemBox" MA_SHOT=330 MA_SHOT_PATH="$ppm" \
    "$WMIG" ) >"$log" 2>&1
rc=$?
pkill -x "$(basename "$WMIG")" 2>/dev/null

echo "system box exit — $(basename "$WMIG")"
ok=0
fired=$(grep -ac "evt_fire] id=10 .*CSystemBox -> HANDLER CALLED" "$log")
if [ "$fired" -gt 0 ]; then echo "  exit handler (IDC_FILES -> OnBye) called: yes"
else echo "  exit handler NOT called — the click never reached CSystemBox"; ok=1; fi

if [ "$rc" -ne 0 ] && [ "$rc" -ne 124 ] && [ "$rc" -ne 137 ]; then
  echo "  RESULT: CRASH (exit $rc) — see $log"; exit 1
fi

python3 - "$ppm" "$ROOT/port/ref/native/campaign_map.png" <<'PY'
import sys
from PIL import Image
shot, ref = sys.argv[1], sys.argv[2]
try:
    a = Image.open(shot).convert("RGB"); b = Image.open(ref).convert("RGB")
except Exception as e:
    print(f"  left the campaign map: UNKNOWN — {e}"); sys.exit(1)
if a.size != b.size:
    print(f"  left the campaign map: yes (screen {a.size} vs map ref {b.size})"); sys.exit(0)
da, db = list(a.getdata()), list(b.getdata())
diff = sum(1 for x, y in zip(da, db) if x != y)
pct = 100.0 * diff / len(da)
print(f"  left the campaign map: {'yes' if pct > 25 else 'NO'} ({pct:.1f}% of pixels differ from the map)")
sys.exit(0 if pct > 25 else 1)
PY
[ $? -ne 0 ] && ok=1

[ "$ok" -eq 0 ] && { echo "  RESULT: PASS — the player can leave the campaign"; exit 0; }
echo "  RESULT: FAIL — see $log"; exit 1
