#!/usr/bin/env bash
# GATE (PO-76): the GAME'S OWN UI hosts a multiplayer session, and another process can join it.
#
# Everything before this gate tested the transport through the SHIM's entry point (mp_packet.sh:
# two probes, no game UI). This one drives the actual front end -- title -> Multi-Player ->
# CREATE GAME -> CONTINUE -- and then asks an independent process to discover and join what the
# game opened. That is the half of PO-76 that was open: "if the transport passes and the game still
# cannot host, the fault is provably in the UI, not the netcode."
#
# ⚠️ HEADLESS. These are 2-D front-end screens, and S417 measured that they render under
# SDL_VIDEODRIVER=dummy. This gate needs no display and must never take gl-lock.
#
# ⚠️ The menu on the service-select screen is HORIZONTAL, so its entries are COLUMNS (#2063:1),
# not rows (r1). r1 there resolves against the PROVIDER listbox, which has one row, and silently
# clicks the wrong thing.
#
# NEGATIVE CONTROL: MA_NO_ONDESTROY=1. The host path depends on CLockerRoom::OnDestroy() running --
# it is what copies the Name box into _DPlay.PlayerName, and UINewPlayer aborts on an empty name
# BEFORE calling Open(). With the dispatch disabled the game must NOT open a session, and the probe
# must find nothing. Without that arm, a passing test could not tell hosting from a stale session
# left by an earlier run.
set -u
ROOT="/home/admin/ma"
WMIG="${WMIG:-$ROOT/build/wmig}"
DRIVE_C="${BOB_DRIVE_C:-$HOME/sgl/TUE/MigAlley/WP/drive_c}"
RUNDIR="$DRIVE_C/rowan/mig"
PROBE="${PROBE:-/tmp/ma_dplay_probe}"
OBJ="$ROOT/build/CMakeFiles/ma_obj.dir/SRC/compat/ma_dplay.cpp.o"
OUT="${OUT:-/tmp/ma_mp_uihost}"; mkdir -p "$OUT"
TMO="${TMO:-200}"
SEQ="40,r2;90,#2063:1;180,#2063:1"

. "$ROOT/port/gate_lib.sh" 2>/dev/null || true
if command -v assert_clean_start >/dev/null 2>&1; then
  assert_clean_start || exit 2
elif pgrep -x "$(basename "$WMIG")" >/dev/null 2>&1; then
  echo "  REFUSING TO RUN: a wmig is already up -- it may be the player's own game."; exit 2
fi

if [ ! -x "$PROBE" ] || [ "$ROOT/port/dplay_probe.cpp" -nt "$PROBE" ] || [ "$OBJ" -nt "$PROBE" ]; then
  echo "  building probe..."
  g++ -m32 -fno-pie -no-pie -w -DMA_LINUX -DFF_LINUX -I "$ROOT/SRC/compat" -I "$ROOT/SRC/H" \
      "$ROOT/port/dplay_probe.cpp" "$OBJ" -o "$PROBE" || { echo "  FAIL: probe did not build"; exit 2; }
fi

fail=0
say(){ printf '  %-46s %s\n' "$1" "$2"; }

# One arm. $1 = label, $2 = extra env ("" or MA_NO_ONDESTROY=1), $3 = log
run_host() {
  local log="$3"
  ( cd "$RUNDIR" && timeout -k 5 -s KILL "$TMO" env \
      SDL_VIDEODRIVER=dummy BOB_RUN_INIT=1 BOB_DRIVE_C="$DRIVE_C" MA_DISABLE_3D=1 \
      MA_MAXIMIZE=0 MA_TRACE_DPLAY=1 BOB_CLICKSEQ="$SEQ" ${2:+$2} \
      "$WMIG" ) >"$log" 2>&1 &
  local gpid=$!
  # wait for the host to actually open (or for the run to end)
  local i=0
  while [ $i -lt 120 ]; do
    grep -aq "Open(CREATE)" "$log" && break
    kill -0 "$gpid" 2>/dev/null || break
    sleep 1; i=$((i+1))
  done
  echo "$gpid"
}

echo "PO-76 -- the game's own UI hosts a session another process can join"

# ---- arm 1: the real thing ------------------------------------------------------------------
HLOG="$OUT/host.log"
GPID=$(run_host "host" "" "$HLOG")
grep -aq "Open(CREATE)" "$HLOG" && say "the game opened a session from its UI" "yes" \
   || { say "the game opened a session from its UI" "NO"; fail=1; }
sess=$(grep -a 'Open(CREATE) session' "$HLOG" | sed 's/.*session "\(.*\)".*/\1/' | head -1)
say "session name the UI supplied" "\"${sess:-}\""
[ -n "$sess" ] || fail=1

PLOG="$OUT/probe.log"
"$PROBE" join >"$PLOG" 2>&1; prc=$?
found=$(grep -a "found session" "$PLOG" | wc -l)
grep -aq "client pid = " "$PLOG" && joined=yes || joined=no
say "an independent process JOINED it" "$joined"
[ "$joined" = yes ] || fail=1
grep -aq "Send -> DP_OK" "$PLOG" && say "and sent it a packet" "yes" \
   || { say "and sent it a packet" "no"; fail=1; }
kill "$GPID" 2>/dev/null; wait "$GPID" 2>/dev/null

# ---- arm 2: NEGATIVE CONTROL ------------------------------------------------------------------
echo "  --- negative control: MA_NO_ONDESTROY=1 (the write-back the host path needs) ---"
NLOG="$OUT/host_noond.log"
NPID=$(run_host "noond" "MA_NO_ONDESTROY=1" "$NLOG")
if grep -aq "Open(CREATE)" "$NLOG"; then
  say "control must NOT open a session" "IT DID -- the arm proves nothing"; fail=1
else
  say "control does not open a session" "correct"
fi
grep -aq 'PlayerName="" (len 0)' "$NLOG" && say "control fails on the empty name, as diagnosed" "yes" \
   || say "control failed for some other reason" "check $NLOG"
kill "$NPID" 2>/dev/null; wait "$NPID" 2>/dev/null

echo "----------------------------------------"
if [ "$fail" -eq 0 ]; then
  echo "PASS: the UI hosts, another process joins and sends, and the control cannot host"; exit 0
fi
echo "FAIL (host log $HLOG, probe log $PLOG)"; exit 1
