#!/usr/bin/env bash
# port/tools/gold_video.sh — frame access for the PO's gold-standard VIDEOS (added S102).
#
# The PO recorded the Windows build under Wine playing a campaign mission end to end
# (2026-08-14). Unlike the still gold shots, a video shows BEHAVIOUR — what a key press does,
# what appears after a click, what the mission-result screen looks like — which is what the
# current defect list is about. Treat it exactly like the PNG golds: the oracle, as-is.
#
#   gold_video.sh list                          # the videos + their durations
#   gold_video.sh frame <video> <t> [out.png]   # one full-res frame at t seconds
#   gold_video.sh sheet <video> <every> [out]   # contact sheet, one tile every <every> s
#   gold_video.sh crop  <video> <t> <x,y,w,h> [out.png]
#
# <video> is `short` (start campaign + exit, 45 s), `full` (complete mission, 353 s) or
# `wonju` (S158: the PO's Wonju supply-depot attack, 344 s, EPIC K — built AND flown from
# scratch; its written step list is `$GOLD/wonju_script.txt`).
#
# GEOMETRY (measure, never assume — the two recordings differ):
#   both are 1920x1080 desktop captures with the game WINDOWED and letterboxed inside.
#   `gold_video.sh geom <video> <t>` prints the non-black bounding box of a frame so a
#   comparison against a native capture can state the gold's own pixel scale. Per the S64
#   lesson, never judge SIZE or DENSITY across that boundary — judge layout order, art,
#   content and colour.
set -u
GOLD="${MA_GOLD_VIDEO_DIR:-$HOME/gold standard/ma}"
SHORT="$GOLD/260814_mig_alley_start_campaign_and_exit.mp4"
FULL="$GOLD/260814_mig_complete_campaign.mp4"
WONJU="$GOLD/wonju_attack.mp4"
OUTDIR="${MA_GOLD_OUT:-/tmp/ma_gold}"
mkdir -p "$OUTDIR"

pick() {
  case "${1:-}" in
    short) echo "$SHORT" ;;
    full)  echo "$FULL" ;;
    wonju) echo "$WONJU" ;;
    *)     echo "unknown video '${1:-}' (want: short | full | wonju)" >&2; exit 2 ;;
  esac
}

case "${1:-}" in
list)
  for v in short full wonju; do
    f="$(pick $v)"
    if [ -f "$f" ]; then
      d=$(ffprobe -v error -show_entries format=duration -of csv=p=0 "$f")
      s=$(ffprobe -v error -select_streams v:0 -show_entries stream=width,height,r_frame_rate -of csv=p=0 "$f")
      printf "%-6s %6.1fs  %s  %s\n" "$v" "$d" "$s" "$f"
    else
      printf "%-6s MISSING  %s\n" "$v" "$f"
    fi
  done
  ;;
frame)
  f="$(pick "${2:-}")"; t="${3:?t seconds}"; out="${4:-$OUTDIR/${2}_t${3}.png}"
  ffmpeg -v error -ss "$t" -i "$f" -frames:v 1 "$out" -y && echo "$out"
  ;;
crop)
  f="$(pick "${2:-}")"; t="${3:?t}"; r="${4:?x,y,w,h}"; out="${5:-$OUTDIR/${2}_t${3}_crop.png}"
  IFS=, read -r X Y W H <<<"$r"
  ffmpeg -v error -ss "$t" -i "$f" -frames:v 1 -vf "crop=$W:$H:$X:$Y" "$out" -y && echo "$out"
  ;;
sheet)
  f="$(pick "${2:-}")"; every="${3:-10}"; out="${4:-$OUTDIR/${2}_sheet.png}"
  ffmpeg -v error -i "$f" -vf "fps=1/$every,scale=480:270,tile=6x6" -frames:v 1 "$out" -y && echo "$out"
  ;;
geom)
  f="$(pick "${2:-}")"; t="${3:-30}"
  tmp="$OUTDIR/_geom.png"
  ffmpeg -v error -ss "$t" -i "$f" -frames:v 1 "$tmp" -y
  python3 - "$tmp" <<'PY'
import sys
from PIL import Image
im = Image.open(sys.argv[1]).convert('L')
bbox = im.point(lambda p: 255 if p > 8 else 0).getbbox()
print("non-black bbox (x0,y0,x1,y1) =", bbox, " -> %dx%d" % (bbox[2]-bbox[0], bbox[3]-bbox[1]))
PY
  ;;
*)
  sed -n '2,25p' "$0"; exit 1 ;;
esac
