#!/bin/bash
# MA-SABRE-SA-1 (10-30): interleaved A/B(/C/D) kill-ratio census. Same QM 4 set-up as ma_killratio_census.sh, but the
# arms alternate fight by fight, so a drift in machine load (wall-clock SECS, three lanes) hits every arm alike.
# kr18/kr19 (identical code, run hours apart) scored 1:15 and 4:6, and kr20/kr21 7:13 and 0:25.
#   tools/ma_killratio_ab.sh <outbase> <skill> "<starts>" "<envA>" "<envB>" ["<envC>" "<envD>"]
#   e.g. tools/ma_killratio_ab.sh ~/ma-gates/ab1 12 "$(seq -s' ' 2 17)" "NONE=1" "MA_VIS_MIGREAR=1"
# Arm n's fights go to <outbase>/<A|B|C|D>/k<skill>_s<start>; summarise each with tools/ma_killratio_sum.py <outbase>/A.
set -u
OUT="$1"; K="$2"; STARTS="$3"; shift 3; ARMS=("$@"); L=(A B C D)
GD=/home/admin/ma-gates/scratch/rowan/mig
BIN=${BIN:-/home/admin/ma/build/wmig}; mkdir -p "$OUT"; md5sum "$BIN" > "$OUT/binary.md5"
for a in "${!ARMS[@]}"; do mkdir -p "$OUT/${L[$a]}"; echo "${ARMS[$a]}" > "$OUT/${L[$a]}/arm_env.txt"; done
jobs=(); for s in $STARTS; do for a in "${!ARMS[@]}"; do jobs+=("$a $s"); done; done
lane() { local i=0 j a s d
  for j in "${jobs[@]}"; do
    if (( i % 3 == $1 )); then set -- $1 $j; a=$2; s=$3; d="$OUT/${L[$a]}/k${K}_s${s}"; mkdir -p "$d"
      ( cd "$GD" && BOB_DRIVE_C="${GD%/rowan/mig}" SDL_VIDEODRIVER=dummy timeout -s INT -k 10 ${SECS:-600} env ${ARMS[$a]} \
          MA_TRACE_SHOOT=1 MA_AI_PILOT=$s MA_QUICKMISS=${QM:-4} MA_QM_SKILL_BLUE=$K MA_QM_SKILL_RED=${RED:-$K} \
          BOB_CLICKSEQ="40,r1;95,r0;140,#2063@RFullPanelDial:r0.1" "$BIN" > "$d/run.log" 2>&1 )
      e=$?; echo "$K $s exit=$e" >> "$OUT/${L[$a]}/done.txt"; echo "${L[$a]} $K $s exit=$e" >> "$OUT/done.txt"
    fi; i=$((i+1)); done; }
lane 0 & lane 1 & lane 2 & wait
md5sum -c "$OUT/binary.md5" >/dev/null || echo "BINARY CHANGED DURING THE CENSUS" | tee -a "$OUT/done.txt"
echo CENSUS_DONE >> "$OUT/done.txt"
