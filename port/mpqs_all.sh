#!/bin/bash
# MA-MPQS-1: every single mission the multiplayer Quick Missions list offers, flown by two players
# (host + joiner on disposable copies). Acceptance per mission: both enter the 3-D, both sync, each sees
# the other aircraft move, no crash. The engage arm's form-up/fire/hit lines are reported, not required.
cd "$(dirname "$0")/.."
mkdir -p "$HOME/ma-gates/mpqs"
export MA_TRACE_QMLIST=1
for m in ${MISSIONS:-4 5 6 7 8 9 10 11 12 13 14 15}; do
  out=$HOME/ma-gates/mpqs/m$m
  MA_QUICKMISS=$m GT=qm ARM=engage OUT=$out HGD=${HGD:-$HOME/ma-keys/drive_c/rowan/mig} \
    CGD=${CGD:-$HOME/ma-crawl/drive_c/rowan/mig} timeout 900 bash port/mp_engage.sh > $out.txt 2>&1
  name=$(grep -a "incomms=1 mission $m " $out/host.log 2>/dev/null | head -n1 | sed -E 's/.*"(.*)".*/\1/')
  ok=1; for k in 'host enters the 3-D' 'client enters the 3-D' 'host syncs' 'client syncs' 'host sees the other aircraft move' 'client sees the other aircraft move'; do
    grep -a "$k" $out.txt | grep -q PASS || ok=0; done
  crash=$(cat $out/host.log $out/client.log 2>/dev/null | grep -acE '=== CRASH|SIGSEGV|Aborted')
  sel=$(grep -aE "Scenario:|incomms=1 mission $m " $out/client.log 2>/dev/null | head -n0 | wc -l)
  printf 'mission %2d %-34s %s crash=%s\n' $m "\"$name\"" $([ $ok = 1 ] && echo PASS || echo FAIL) $crash
done
