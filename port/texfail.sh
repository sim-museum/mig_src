#!/usr/bin/env bash
# GATE: PO-82 texture resolution -- protects the S348 texture-registry fix.
#
# S348 replaced a fixed 4096-entry array with a map after handles climbed past it, taking
# 820,000 FAILED lookups per sortie to 0. Those failures are what the PO saw as white
# explosions. This gate re-flies and asserts the failure count stays at zero.
#
# S364: it asserts THREE things, because two of them were learned the hard way.
#   1. the renderer is HARDWARE. fSoftware is a persisted player setting and an earlier
#      run left it on software; the software rasterizer never calls ma_tex_desc, so four
#      consecutive measurements read "no textures resolved" and looked like a regression
#      in the fix. MA_FORCE_HARDWARE pins it, and this checks the pin actually took.
#   2. resolved > 0. Otherwise an instrument that never runs reads exactly like a pass.
#   3. FAILED == 0. The regression this gate exists for.
set -u
LOG=/tmp/ma_texfail/fly.log; mkdir -p /tmp/ma_texfail
SECS=${SECS:-260}
MIG="$HOME/sgl/TUE/MigAlley/WP/drive_c/rowan/mig"
echo "PO-82 texture-resolution gate -- ${SECS}s Hot Shot sortie (hardware renderer pinned)"
cd "$MIG" || { echo "  FAIL: no game dir"; exit 1; }
env BOB_RUN_INIT=1 BOB_DRIVE_C="$HOME/sgl/TUE/MigAlley/WP/drive_c" \
    MA_ENABLE_3D=1 MA_TRACE_3D=1 MA_FORCE_HARDWARE=1 MA_TRACE_TEXFAIL=1 \
    BOB_CLICKSEQ="40,r1;95,r0" \
    timeout -k 5 "$SECS" "$HOME/ma/build/wmig" > "$LOG" 2>&1
rend=$(grep -a "\[hw\] renderer:" "$LOG" | tail -1)
case "$rend" in
  *hardware*) echo "  renderer: hardware (pinned)" ;;
  *software*) echo "  INCONCLUSIVE: ran on the SOFTWARE rasterizer -- it never resolves a"
              echo "                texture, so this flight cannot test the fix. MA_FORCE_HARDWARE"
              echo "                did not take; check ma_hardware_available()."; exit 2 ;;
  *)          echo "  INCONCLUSIVE: the renderer was never traced -- the sortie did not reach 3D."; exit 2 ;;
esac
last=$(grep -a "^\[texfail\]" "$LOG" | tail -1)
[ -z "$last" ] && { echo "  INCONCLUSIVE: the texture instrument never reported."; exit 2; }
res=$(echo "$last" | sed -n 's/.*resolved=\([0-9]*\).*/\1/p')
fail=$(echo "$last" | sed -n 's/.*FAILED=\([0-9]*\).*/\1/p')
echo "  resolved=$res  FAILED=$fail"
[ "${res:-0}" -gt 0 ] || { echo "  INCONCLUSIVE: zero textures resolved -- nothing was measured."; exit 2; }
if [ "${fail:-1}" -eq 0 ]; then echo "  PASS: no failed texture resolutions in $res lookups."; exit 0
else echo "  FAIL: $fail failed resolutions -- the S348 registry fix has regressed."; exit 1; fi
