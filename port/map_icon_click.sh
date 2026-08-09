#!/usr/bin/env bash
# port/map_icon_click.sh — gate for PO-3: clicking a map icon opens its dossier ("recon") dialog.
#
# The engine always had the chain (CMapDlg::OnLButtonDown -> FindMapItem -> m_buttonid, then
# OnLButtonUp -> OnClickItem -> CMainToolbar::OpenDossier -> CTargetDossier::MakeSheet). The port
# never delivered a mouse event to m_mapdlg, so every icon on the campaign map was dead. S95 routes
# map clicks; this gate keeps them routed.
#
# WHY IT DOES NOT HARDCODE A COORDINATE: icon positions move with campaign state, canvas size and
# scroll. A point picked by hand went stale between two runs during S95 itself — same binary, same
# save, different frame. So the run asks the map's OWN hit-test where an icon is
# (MA_MAP_ITEM_SCAN=<frame>) and clicks that (MA_MAP_CLICK_FIRST=1). A gate that depends on a
# coordinate a human read off a screenshot reports failures that are not regressions (cf. the S94
# OOB-sweep save-state lesson).
#
# PASS = the OOB paint walk reports a painted dialog after the click, and the process survives.
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WMIG="${WMIG:-$ROOT/build/wmig}"
BOB_DRIVE_C="${BOB_DRIVE_C:-$HOME/sgl/TUE/MigAlley/WP/drive_c}"
RUNDIR="$BOB_DRIVE_C/rowan/mig"
OUT="${OUT:-/tmp/ma_mapclick}"
TMO="${TMO:-130}"
SCAN_AT="${SCAN_AT:-250}"
NAV="30,r3;65,#1055;100,#2063:1"        # title -> Campaign -> load "Auto Save" -> map

mkdir -p "$OUT"
[ -x "$WMIG" ] || { echo "no binary at $WMIG" >&2; exit 2; }

# Same rule as oob_sweep.sh (S94): PIN a known campaign save. Every icon on the map comes from
# campaign state, so an advanced save changes what is clickable and the result stops meaning
# anything about the code.
SAVEFILE="$RUNDIR/SaveGame/Auto Save.sav"
PIN="$ROOT/port/ref/save/campaign_pristine.sav"
STASH="$(mktemp -u /tmp/ma_mapclick_save.XXXXXX)"
[ -f "$SAVEFILE" ] && cp -a "$SAVEFILE" "$STASH"
restore_save() { [ -f "$STASH" ] && cp -a "$STASH" "$SAVEFILE" && rm -f "$STASH"; }
trap restore_save EXIT INT TERM
[ -f "$PIN" ] && cp -f "$PIN" "$SAVEFILE"

log="$OUT/mapclick.log"; ppm="$OUT/mapclick.ppm"; rm -f "$ppm"
( cd "$RUNDIR" && timeout -k 5 -s KILL "$TMO" env \
    SDL_VIDEODRIVER=dummy BOB_RUN_INIT=1 BOB_DRIVE_C="$BOB_DRIVE_C" MA_DISABLE_3D=1 \
    MA_IGNORE_SAVE_DATE=1 MA_TRACE_OOB=1 MA_TRACE_CLICK=1 \
    MA_MAP_ITEM_SCAN="$SCAN_AT" MA_MAP_CLICK_FIRST=1 \
    BOB_CLICKSEQ="$NAV" MA_SHOT=$((SCAN_AT + 150)) MA_SHOT_PATH="$ppm" \
    "$WMIG" ) >"$log" 2>&1
rc=$?
pkill -x "$(basename "$WMIG")" 2>/dev/null

hits=$(grep -a "scan done" "$log" | head -1)
hit=$(grep -a "\[mapclick\].*hit id=" "$log" | head -1)
painted=$(grep -ac "\[oob\] painted" "$log")

echo "map icon click — $(basename "$WMIG")"
echo "  ${hits:-no scan line}"
echo "  ${hit:-no click line}"
if [ "$rc" -ne 0 ] && [ "$rc" -ne 124 ] && [ "$rc" -ne 137 ]; then
  echo "  RESULT: CRASH (exit $rc) — see $log"; exit 1
fi
case "$hit" in
  *"hit id=0("*|"") echo "  RESULT: FAIL — the click resolved to no map item; see $log"; exit 1;;
esac
if [ "$painted" -gt 0 ]; then
  echo "  RESULT: PASS — dossier painted ($painted passes), capture $ppm"; exit 0
fi
echo "  RESULT: FAIL — item hit but no dialog painted; see $log"; exit 1
