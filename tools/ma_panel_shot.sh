#!/usr/bin/env bash
# tools/ma_panel_shot.sh -- GAUGES-1: Hot Shot quick flight on real GL (scratch tree), frames every EVERY presents.
#   tools/ma_panel_shot.sh <outdir> [extra env...]
#   POSE="yaw pitch roll [z]"  send synthetic opentrack poses (head tracking) for the whole run
#   PANEL=1                    hold numpad-Enter (quick view 21, the instrument panel) from pump 300
#   CAMP=1                     campaign 1 flight (MA_CAMP_FLY) instead of Hot Shot; saves restored after
#   FIRST=2000 EVERY=30 COUNT=10 SECS=90
set -u
OUT="$1"; shift; mkdir -p "$OUT"; rm -f "$OUT"/f* "$OUT"/run.log
GD=/home/admin/ma-gates/scratch/rowan/mig
pgrep -x wmig >/dev/null && { echo "wmig already running" >&2; exit 2; }
SP=""; [ -n "${POSE:-}" ] && { python3 /home/admin/ff-gates/headtrack/send.py $POSE & SP=$!; }
C="BOB_CLICKSEQ=40,r1;95,r0;140,#2063@RFullPanelDial:r0.1"; SG="$GD/SaveGame"
if [ -n "${CAMP:-}" ]; then C="BOB_CLICKSEQ=30,r3;65,#1055;100,#2063:1 MA_CAMP_FLY=1 MA_IGNORE_SAVE_DATE=1 BOB_RUN_INIT=1 MA_ENABLE_3D=1"
  cp "$SG/Auto Save.sav" "$OUT/autosave.bak"; cp "$SG/settings.mig" "$OUT/settings.bak"; fi
K=""; [ -n "${PANEL:-}" ] && K="BOB_KEYSEQ=300,0x9c,,999999"
( cd "$GD" && BOB_DRIVE_C="${GD%/rowan/mig}" timeout -s INT -k 5 "${SECS:-90}" env DISPLAY="${DISPLAY:-:0}" $K \
    BOB_DUMP_FRAME="${FIRST:-2000}" BOB_DUMP_FRAME_EVERY="${EVERY:-30}" BOB_DUMP_FRAME_COUNT="${COUNT:-10}" BOB_DUMP_PATH="$OUT/f" $C "$@" ${BIN:-/home/admin/ma/build/wmig} > "$OUT/run.log" 2>&1 )
[ -n "$SP" ] && kill $SP
[ -n "${CAMP:-}" ] && { cp "$OUT/autosave.bak" "$SG/Auto Save.sav"; cp "$OUT/settings.bak" "$SG/settings.mig"; }
echo "frames=$(ls "$OUT" | grep -c '^f') poses=$(grep -a -c '\[headtrack\] pose' "$OUT/run.log")"
