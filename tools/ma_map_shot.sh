#!/usr/bin/env bash
export MA_WINDOW_DISPLAY="${MA_WINDOW_DISPLAY:-1}"   # PO 10-26: harness windows open on the left screen (DP-1)
# tools/ma_map_shot.sh -- MA-D3: campaign 1 map ("Auto Save") on real GL in the scratch tree, frames every EVERY presents.
#   tools/ma_map_shot.sh <outdir> [extra env...]      FIRST=400 EVERY=100 COUNT=3 SECS=60
# Save and settings files are restored afterwards.
set -u
OUT="$1"; shift; mkdir -p "$OUT"; rm -f "$OUT"/f* "$OUT"/run.log
GD=/home/admin/ma-gates/scratch/rowan/mig; SG="$GD/SaveGame"
pgrep -x wmig >/dev/null && { echo "wmig already running" >&2; exit 2; }
cp "$SG/Auto Save.sav" "$OUT/autosave.bak"; cp "$SG/settings.mig" "$OUT/settings.bak"
( cd "$GD" && BOB_DRIVE_C="${GD%/rowan/mig}" timeout -s INT -k 5 "${SECS:-60}" env DISPLAY="${DISPLAY:-:0}" \
    BOB_RUN_INIT=1 MA_IGNORE_SAVE_DATE=1 BOB_CLICKSEQ="30,r3;65,#1055;100,#2063:1" \
    BOB_DUMP_FRAME="${FIRST:-400}" BOB_DUMP_FRAME_EVERY="${EVERY:-100}" BOB_DUMP_FRAME_COUNT="${COUNT:-3}" BOB_DUMP_PATH="$OUT/f" \
    "$@" ${BIN:-/home/admin/ma/build/wmig} > "$OUT/run.log" 2>&1 )
cp "$OUT/autosave.bak" "$SG/Auto Save.sav"; cp "$OUT/settings.bak" "$SG/settings.mig"
echo "frames=$(ls "$OUT" | grep -c '^f')"
