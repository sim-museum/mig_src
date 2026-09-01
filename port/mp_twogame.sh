#!/usr/bin/env bash
# GATE (PO-76): TWO GAME INSTANCES, one hosting and one joining, through the game's own UI.
#
# This is the epic's own acceptance shape. Everything before it stopped short of it:
#   mp_packet.sh  -- two PROBES through the shim's entry point, no game UI
#   mp_uihost.sh  -- the game hosts, a PROBE joins
#   this gate     -- the game hosts, THE GAME joins, and real game data crosses
#
# Both instances are headless (2-D front end renders under SDL_VIDEODRIVER=dummy, S417), so this
# takes no display lock and both can run at once.
#
# ⚠️ Menus on these screens are HORIZONTAL: entries are COLUMNS (#2063:N), not rows.
#   service-select:  0 Back   1 CREATE GAME   2 JOIN GAME
#   select-session:  0 Back   1 SELECT
#   multiplayer:     0 Back   1 CONTINUE
#
# NEGATIVE CONTROL: run the client with NO HOST. It must find 0 sessions. Without that arm a
# passing run cannot distinguish "joined the host" from "reported success regardless".
set -u
ROOT="/home/admin/ma"
WMIG="${WMIG:-$ROOT/build/wmig}"
DRIVE_C="${BOB_DRIVE_C:-$HOME/sgl/TUE/MigAlley/WP/drive_c}"
RUNDIR="$DRIVE_C/rowan/mig"
OUT="${OUT:-/tmp/ma_mp_twogame}"; mkdir -p "$OUT"
HOST_SEQ="40,r2;90,#2063:1;180,#2063:1"
# ⚠️ These frame numbers are load-bearing and were arrived at by measurement, not taste.
# Moving SELECT later (400/700) BROKE the join: by then the screen has moved on and the click lands
# somewhere else. 170 works. My reason for moving it -- "discovery is asynchronous, the list may
# still be empty" -- was plausible and wrong: the run that made me believe it had failed because the
# HOST had been killed by its own timeout, not because the click was early. Two different faults
# with one symptom ("0 sessions"), and I changed the wrong thing first.
JOIN_SEQ="40,r2;90,#2063:2;170,#2063:1;250,#2063:1"

. "$ROOT/port/gate_lib.sh" 2>/dev/null || true
if command -v assert_clean_start >/dev/null 2>&1; then
  assert_clean_start || exit 2
elif pgrep -x "$(basename "$WMIG")" >/dev/null 2>&1; then
  echo "  REFUSING TO RUN: a wmig is already up -- it may be the player's own game."; exit 2
fi

fail=0
say(){ printf '  %-46s %s\n' "$1" "$2"; }

# Kill ONLY the wmig processes this gate started. `kill $!` reaches the `timeout` wrapper's
# subshell, not the game under it, so the first cut of this gate left two strays per run and then
# refused to start on its own mess. Snapshot first, kill the difference -- BoB's S392 lesson
# (bob_safe_kill.sh), which MA's gates never adopted: a gate must never kill a wmig it did not
# start, because one of them may be the player's own game.
SNAP=" $(pgrep -x "$(basename "$WMIG")" 2>/dev/null | tr '\n' ' ') "
kill_ours() {
  local p
  for p in $(pgrep -x "$(basename "$WMIG")" 2>/dev/null); do
    case "$SNAP" in *" $p "*) continue ;; esac      # existed before us: not ours, never touch it
    kill "$p" 2>/dev/null
  done
  sleep 1
}
trap kill_ours EXIT
# $4 (optional) = MA_SHOT frame; a client given one dumps and EXITS instead of idling until its
# timeout, which is what let the first cut of this gate outlive its own host: the host's `timeout`
# fired at 240 s while the client was still sitting in its 200 s idle, so the probe found nothing
# and the log said "0 session(s)" for a host that had been alive when the click sequence ran.
# Run to COMPLETION in the foreground. The client must finish its click sequence before any arm is
# evaluated, and `wait $pid` cannot do that here: `launch` reports the pid from INSIDE a command
# substitution, so that pid is not a child of this shell and `wait` returns instantly. The gate then
# judged a client that was still starting up, and killed it -- which read exactly like "the join
# failed". Only the host needs backgrounding, because it must outlive the client.
run_fg() {  # $1=seq $2=log $3=timeout
  ( cd "$RUNDIR" && timeout -k 5 -s KILL "$3" env \
      SDL_VIDEODRIVER=dummy BOB_RUN_INIT=1 BOB_DRIVE_C="$DRIVE_C" MA_DISABLE_3D=1 \
      MA_MAXIMIZE=0 MA_TRACE_DPLAY=1 MA_TRACE_CLICK=1 BOB_CLICKSEQ="$1" "$WMIG" ) >"$2" 2>&1
}

launch() {  # $1=seq $2=log $3=timeout [$4=shot frame]
  ( cd "$RUNDIR" && timeout -k 5 -s KILL "$3" env \
      SDL_VIDEODRIVER=dummy BOB_RUN_INIT=1 BOB_DRIVE_C="$DRIVE_C" MA_DISABLE_3D=1 \
      MA_MAXIMIZE=0 MA_TRACE_DPLAY=1 MA_TRACE_CLICK=1 BOB_CLICKSEQ="$1" \
      ${4:+MA_SHOT=$4} ${4:+MA_SHOT_PATH=$OUT/shot.ppm} "$WMIG" ) >"$2" 2>&1 &
  echo $!
}

echo "PO-76 -- two GAME instances: one hosts from its UI, the other joins from its UI"

HLOG="$OUT/host.log"; CLOG="$OUT/client.log"
HP=$(launch "$HOST_SEQ" "$HLOG" 600)
i=0; while [ $i -lt 90 ]; do grep -aq "Open(CREATE)" "$HLOG" && break; kill -0 "$HP" 2>/dev/null || break; sleep 1; i=$((i+1)); done
grep -aq "Open(CREATE)" "$HLOG" && say "instance 1 hosted from its UI" "yes" \
  || { say "instance 1 hosted from its UI" "NO"; fail=1; }

run_fg "$JOIN_SEQ" "$CLOG" 200
sleep 2   # let the host log what the client sent before it is stopped

grep -aq 'EnumSessions: found' "$CLOG" && say "instance 2 DISCOVERED the session" "yes" \
  || { say "instance 2 DISCOVERED the session" "NO"; fail=1; }
if grep -aq "client joined from" "$HLOG"; then
  say "instance 2 JOINED it (host assigned a pid)" "$(grep -a 'client joined from' "$HLOG" | sed 's/.*assigned //' | head -1)"
else
  say "instance 2 JOINED it" "NO"; fail=1
fi
bytes=$(grep -a "received .* data bytes from pid" "$HLOG" | sed 's/.*received \([0-9]*\) data bytes.*/\1/' | paste -sd+ | bc 2>/dev/null)
pkts=$(grep -ac "received .* data bytes from pid" "$HLOG")
if [ "${pkts:-0}" -gt 0 ]; then
  say "REAL GAME DATA crossed (host <- client)" "$pkts packet(s), ${bytes:-0} bytes"
else
  say "REAL GAME DATA crossed" "NO"; fail=1
fi
kill_ours

echo "  --- negative control: a client with NO host ---"
NLOG="$OUT/client_nohost.log"
run_fg "$JOIN_SEQ" "$NLOG" 120
if grep -aq 'EnumSessions: found' "$NLOG"; then
  say "control must find NO session" "IT FOUND ONE -- the arm proves nothing"; fail=1
else
  say "control finds no session" "correct"
fi

echo "----------------------------------------"
[ "$fail" -eq 0 ] && { echo "PASS: two game instances, joined through the UI, exchanging data"; exit 0; }
echo "FAIL (host $HLOG, client $CLOG)"; exit 1
