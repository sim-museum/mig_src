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
#      in the fix. MA_TRY_HARDWARE pins it -- at display-init time (MIG.CPP:713), the only
#      point where DDRWINIT can still build Direct3D from the value.
#      The assertion reads the `[hw] display init:` line, which MIG.CPP prints from the
#      value DDRWINIT actually used. Do NOT assert the later `[hw] renderer:` line: it is
#      printed next to the assignment, so a gate that sets the flag and then checks that
#      line is only confirming its own `export` -- it would pass with the renderer never
#      having changed at all.
#   2. resolved > 0. Otherwise an instrument that never runs reads exactly like a pass.
#   3. FAILED == 0. The regression this gate exists for.
set -u
LOG=/tmp/ma_texfail/fly.log; mkdir -p /tmp/ma_texfail
SECS=${SECS:-260}
MIG="$HOME/sgl/TUE/MigAlley/WP/drive_c/rowan/mig"
echo "PO-82 texture-resolution gate -- ${SECS}s Hot Shot sortie (hardware renderer pinned)"
cd "$MIG" || { echo "  FAIL: no game dir"; exit 1; }
env BOB_RUN_INIT=1 BOB_DRIVE_C="$HOME/sgl/TUE/MigAlley/WP/drive_c" \
    MA_ENABLE_3D=1 MA_TRACE_3D=1 MA_TRY_HARDWARE=1 MA_TRACE_TEXFAIL=1 \
    BOB_CLICKSEQ="40,r1;95,r0" \
    timeout -k 5 "$SECS" "$HOME/ma/build/wmig" > "$LOG" 2>&1
rend=$(grep -a "\[hw\] display init:" "$LOG" | tail -1)
case "$rend" in
  *fSoftware=0*) echo "  renderer: hardware (pinned at display init)" ;;
  *fSoftware=1*) echo "  INCONCLUSIVE: display init built the SOFTWARE rasterizer -- it never resolves"
                 echo "                a texture, so this flight cannot test the fix. The MA_TRY_HARDWARE"
                 echo "                pin did not take; check ma_hardware_available()."; exit 2 ;;
  *)             echo "  INCONCLUSIVE: display init was never traced -- the sortie did not get that far."; exit 2 ;;
esac
last=$(grep -a "^\[texfail\]" "$LOG" | tail -1)
[ -z "$last" ] && { echo "  INCONCLUSIVE: the texture instrument never reported."; exit 2; }
res=$(echo "$last" | sed -n 's/.*resolved=\([0-9]*\).*/\1/p')
fail=$(echo "$last" | sed -n 's/.*FAILED=\([0-9]*\).*/\1/p')
echo "  resolved=$res  FAILED=$fail"
[ "${res:-0}" -gt 0 ] || { echo "  INCONCLUSIVE: zero textures resolved -- nothing was measured."; exit 2; }
if [ "${fail:-1}" -eq 0 ]; then echo "  PASS: no failed texture resolutions in $res lookups."; exit 0
else echo "  FAIL: $fail failed resolutions -- the S348 registry fix has regressed."; exit 1; fi
