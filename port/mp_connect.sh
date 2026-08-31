#!/usr/bin/env bash
# GATE: PO-76 -- multiplayer gets past its front door.
#
# The single call multiplayer was missing was CoCreateInstance(CLSID_DirectPlay), which the
# compat layer refused for every CLSID: DPlay::CreateDPlayInterface FALSE -> UIMultiPlayInit
# FALSE -> StartCommsSession FALSE -> the "not connected" box. S373 adopted BoB's UDP-backed
# shim (cross-port note 43) to serve it.
#
# This asserts the front door is open and, crucially, that the NEGATIVE CONTROL still shuts it.
# A gate that only shows the good arm cannot tell "the shim works" from "the trace line exists".
set -u
LOG=/tmp/ma_mp/run.log; mkdir -p /tmp/ma_mp
MIG="$HOME/sgl/TUE/MigAlley/WP/drive_c/rowan/mig"
cd "$MIG" || { echo "  FAIL: no game dir"; exit 1; }
run() {  # $1 = extra env, $2 = log
  env SDL_VIDEODRIVER=dummy BOB_RUN_INIT=1 BOB_DRIVE_C="$HOME/sgl/TUE/MigAlley/WP/drive_c" \
      MA_DISABLE_3D=1 MA_TRACE_DPLAY=1 $1 BOB_CLICKSEQ="30,r2" \
      timeout -k 5 -s KILL 60 "$HOME/ma/build/wmig" > "$2" 2>&1
}
echo "PO-76 multiplayer front-door gate"
run "" "$LOG"
obj=$(grep -ac "CoCreateInstance(CLSID_DirectPlay) ->" "$LOG")
prov=$(grep -a "EnumConnections -> " "$LOG" | tail -1 | sed -n 's/.*-> \([0-9]*\) provider.*/\1/p')
echo "  DirectPlay object created: $obj    service providers enumerated: ${prov:-0}"
[ "${obj:-0}" -ge 1 ] || { echo "  FAIL: the game never obtained a DirectPlay object -- the CLSID is refused again."; exit 1; }
[ "${prov:-0}" -ge 1 ] || { echo "  FAIL: object created but NO service provider enumerated -- the lobby has nothing to connect with."; exit 1; }

run "MA_NO_DPLAY=1" "$LOG.ctl"
cobj=$(grep -ac "CoCreateInstance(CLSID_DirectPlay) ->" "$LOG.ctl")
echo "  control (MA_NO_DPLAY=1): objects created = $cobj"
[ "${cobj:-1}" -eq 0 ] || { echo "  FAIL: the control still built an object -- MA_NO_DPLAY does not disable the shim,"
                            echo "        so the treatment arm proves nothing about it."; exit 1; }
echo "  PASS: DirectPlay object + 1 provider, and the control cannot obtain one."
exit 0
