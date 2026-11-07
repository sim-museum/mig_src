#!/bin/bash
# MA-KILLRATIO-1 (PO 10-27): AI many-on-many Sabre:MiG kill ratio by skill, same-level pairings.
# QM 4 (Dogfight 1: No Advantage, 4 F-86E v 4 MiG-15), player seat handed to the AI (MA_AI_PILOT=start),
# both sides at one skill (MA_QM_SKILL_BLUE/RED), headless, SECS each. Three runs at a time.
#   tools/ma_killratio_census.sh <outdir> "<skills>" "<starts>"     e.g.  ~/ma-gates/kr1 "0 6 9 12 15" "2 3 4 5 6 7"
# Extra env (e.g. MA_NO_SABRE_PERIOD=1) passes through. RED=<skill> sets the MiG skill (default: same as the Sabres). Summary: tools/ma_killratio_sum.py <outdir>
set -u
# A start is the second the AI takes the player seat (MA_AI_PILOT); until then the jet flies hands-off. QM 8 starts >= 29
# crashed it and its whole flight (strike32 s29-s33), so keep starts <= 25; QM 4 (kr18-kr21, starts to 33) was clean.
MAXS=25; [ "${QM:-4}" = 4 ] && MAXS=33
for s in $3; do (( s > MAXS )) && { echo "start $s > $MAXS: the hands-off player jet may crash its flight" >&2; exit 2; }; done
OUT="$1"; SKILLS="$2"; STARTS="$3"; mkdir -p "$OUT"
GD=/home/admin/ma-gates/scratch/rowan/mig
BIN=${BIN:-/home/admin/ma/build/wmig}; md5sum "$BIN" > "$OUT/binary.md5"
jobs=(); for k in $SKILLS; do for s in $STARTS; do jobs+=("$k $s"); done; done
lane() { local i=0 j k s d
  for j in "${jobs[@]}"; do
    if (( i % 3 == $1 )); then set -- $1 $j; k=$2; s=$3; d="$OUT/k${k}_s${s}"; mkdir -p "$d"
      ( cd "$GD" && BOB_DRIVE_C="${GD%/rowan/mig}" SDL_VIDEODRIVER=dummy timeout -s INT -k 10 ${SECS:-600} env \
          MA_ACMI_PATH="$d/fight.acmi" MA_TRACE_SHOOT=1 MA_NO_AUDIO=1 MA_NOJOY=${MA_NOJOY:-1} MA_AI_PILOT=$s MA_QUICKMISS=${QM:-4} MA_QM_SKILL_BLUE=$k MA_QM_SKILL_RED=${RED:-$k} \
          BOB_CLICKSEQ="40,r1;95,r0;140,#2063@RFullPanelDial:r0.1" "$BIN" > "$d/run.log" 2>&1 )
      echo "$k $s exit=$?" >> "$OUT/done.txt"
      # each fight's Tacview track, zipped (~50 MB -> ~8 MB); replaylab and Tacview open the .zip.acmi directly
      [ -f "$d/fight.acmi" ] && python3 -I -c "import sys,zipfile; zipfile.ZipFile(sys.argv[1][:-5]+'.zip.acmi','w',zipfile.ZIP_DEFLATED).write(sys.argv[1],'fight.acmi')" "$d/fight.acmi" && rm -f "$d/fight.acmi"
    fi; i=$((i+1)); done; }
lane 0 & lane 1 & lane 2 & wait
md5sum -c "$OUT/binary.md5" >/dev/null || echo "BINARY CHANGED DURING THE CENSUS" | tee -a "$OUT/done.txt"
echo CENSUS_DONE >> "$OUT/done.txt"
