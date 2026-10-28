#!/bin/bash
# MA-KILLRATIO-1 (PO 10-27): AI many-on-many Sabre:MiG kill ratio by skill, same-level pairings.
# QM 4 (Dogfight 1: No Advantage, 4 F-86E v 4 MiG-15), player seat handed to the AI (MA_AI_PILOT=start),
# both sides at one skill (MA_QM_SKILL_BLUE/RED), headless, SECS each. Three runs at a time.
#   tools/ma_killratio_census.sh <outdir> "<skills>" "<starts>"     e.g.  ~/ma-gates/kr1 "0 6 9 12 15" "2 3 4 5 6 7"
# Extra env (e.g. MA_NO_SABRE_PERIOD=1) passes through. Summary: tools/ma_killratio_sum.py <outdir>
set -u
OUT="$1"; SKILLS="$2"; STARTS="$3"; mkdir -p "$OUT"
GD=/home/admin/ma-gates/scratch/rowan/mig
BIN=${BIN:-/home/admin/ma/build/wmig}; md5sum "$BIN" > "$OUT/binary.md5"
jobs=(); for k in $SKILLS; do for s in $STARTS; do jobs+=("$k $s"); done; done
lane() { local i=0 j k s d
  for j in "${jobs[@]}"; do
    if (( i % 3 == $1 )); then set -- $1 $j; k=$2; s=$3; d="$OUT/k${k}_s${s}"; mkdir -p "$d"
      ( cd "$GD" && BOB_DRIVE_C="${GD%/rowan/mig}" SDL_VIDEODRIVER=dummy timeout -s INT -k 10 ${SECS:-600} env \
          MA_TRACE_SHOOT=1 MA_AI_PILOT=$s MA_QUICKMISS=${QM:-4} MA_QM_SKILL_BLUE=$k MA_QM_SKILL_RED=$k \
          BOB_CLICKSEQ="40,r1;95,r0;140,#2063@RFullPanelDial:r0.1" "$BIN" > "$d/run.log" 2>&1 )
      echo "$k $s exit=$?" >> "$OUT/done.txt"
    fi; i=$((i+1)); done; }
lane 0 & lane 1 & lane 2 & wait
md5sum -c "$OUT/binary.md5" >/dev/null || echo "BINARY CHANGED DURING THE CENSUS" | tee -a "$OUT/done.txt"
echo CENSUS_DONE >> "$OUT/done.txt"
