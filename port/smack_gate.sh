#!/bin/bash
# E1 gate: the port's Smacker decoder must agree with ffprobe on every clip it will play.
set -u
REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SMK="${MA_SMK_DIR:-/home/admin/sgl/TUE/MigAlley/INSTALL/MA_iso/smacker}"
BIN="$REPO/build/smacktest"
fails=0
check() { local ok=$1; shift; printf "  %s  %s\n" "$([ "$ok" = 1 ] && echo PASS || echo FAIL)" "$*"; [ "$ok" = 1 ] || fails=$((fails+1)); }
[ -x "$BIN" ] || { echo "no smacktest binary at $BIN"; exit 1; }
for f in Kimpo.smk intro.smk c1_int.smk; do
  p="$SMK/$f"; [ -f "$p" ] || { check 0 "$f present"; continue; }
  out=$("$BIN" "$p" 2>/dev/null); rc=$?
  want=$(ffprobe -v error -count_frames -select_streams v:0 -show_entries stream=nb_read_frames,width,height -of csv=p=0 "$p")
  ww=$(echo "$want" | cut -d, -f1); wh=$(echo "$want" | cut -d, -f2); wf=$(echo "$want" | cut -d, -f3)
  gw=$(sed -n 's/.*width=\([0-9]*\).*/\1/p' <<<"$out"); gh=$(sed -n 's/.*height=\([0-9]*\).*/\1/p' <<<"$out")
  gf=$(sed -n 's/.*frames=\([0-9]*\) nonblack.*/\1/p' <<<"$out"); nb=$(sed -n 's/.*nonblack=\([0-9]*\).*/\1/p' <<<"$out")
  ar=$(sed -n 's/.*audio_rate=\([0-9]*\).*/\1/p' <<<"$out"); as=$(sed -n 's/.*audio_samples=\([0-9]*\).*/\1/p' <<<"$out")
  echo "$f: decoder ${gw}x${gh} ${gf} frames (${nb} non-black) audio ${ar} Hz ${as} samples | ffprobe ${ww}x${wh} ${wf} frames"
  check $([ "$rc" = 0 ] && echo 1 || echo 0) "$f decodes to the end (rc=$rc)"
  check $([ "$gw" = "$ww" ] && [ "$gh" = "$wh" ] && echo 1 || echo 0) "$f size matches ffprobe"
  check $([ "$gf" = "$wf" ] && echo 1 || echo 0) "$f frame count matches ffprobe ($gf vs $wf)"
  check $([ "${nb:-0}" -gt $(( ${gf:-1} / 2 )) ] && echo 1 || echo 0) "$f most frames carry picture (not black)"
  if ffprobe -v error -select_streams a:0 -show_entries stream=codec_type -of csv=p=0 "$p" | grep -q audio; then
    check $([ "${as:-0}" -gt 0 ] && echo 1 || echo 0) "$f audio track decodes ($as samples)"
  fi
done
[ $fails = 0 ] && echo "SMACK GATE: PASS" || echo "SMACK GATE: FAIL ($fails)"
exit $fails
