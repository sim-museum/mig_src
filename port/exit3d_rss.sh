#!/usr/bin/env bash
# EXIT3D-1: quick-mission flight (One-on-One row), then leave 3-D two ways; RSS sampled every second.
set -u
ROOT=/home/admin/ma; OUT=$HOME/ma-gates/exit3d
DC=$HOME/sgl/TUE/MigAlley/WP/drive_c; RUNDIR="$DC/rowan/mig"
run_arm() {
  local TAG=$1; shift
  ( cd "$RUNDIR" && timeout -k 5 -s INT ${RUN:-300} env DISPLAY=:0 BOB_RUN_INIT=1 MA_ENABLE_3D=1 MA_TRACE_3D=1 MA_NO_INTRO=1 MA_TRACE_CLICK=1 MA_TRACE_KEY=1 \
      BOB_CLICKSEQ="40,r1;60,r1;110,#2063:2" BOB_DRIVE_C="$DC" MA_EXIT3D_HARNESS=1 "$@" "$ROOT/build/wmig" ) > "$OUT/$TAG.log" 2>&1 &
  local GP=$!; : > "$OUT/$TAG.rss"
  for i in $(seq 1 ${RUN:-300}); do
    p=$(pgrep -x wmig | head -1); [ -z "$p" ] && { kill -0 $GP 2>/dev/null || break; }
    [ -n "$p" ] && echo "$i $(grep -a VmRSS /proc/$p/status 2>/dev/null | awk '{print $2}') $(grep -a VmSwap /proc/$p/status 2>/dev/null | awk '{print $2}')" >> "$OUT/$TAG.rss"
    sleep 1
  done
  wait $GP; echo "[$TAG] game exit=$?"
  grep -a -n "BOB_AUTOEXIT\|quit3d\|OnFlyingClosed\|back in front-end\|Killed\|Segmentation\|Aborted\|dumped back-surface\|acquired\|EXITKEY\|\[keyseq\]\|\[click\]" "$OUT/$TAG.log" | head -14 | cut -c1-150
  echo "[$TAG] peak RSS kB: $(sort -k2 -n "$OUT/$TAG.rss" | tail -1)"; tail -2 "$OUT/$TAG.rss"
}
run_arm exit3d_graceful BOB_AUTOEXIT=900
run_arm exit3d_altx BOB_KEYSEQ_FRAMES=1 BOB_KEYSEQ="700,0x2D,0x38"
echo MA-EXIT3D2-done
