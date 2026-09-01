#!/usr/bin/env bash
# tools/bob_mp_packet.sh -- R7.1: two processes exchange a DirectPlay packet.
#
# Proves the TRANSPORT, not the UI, and deliberately so. The two-process test cannot go through the
# front end yet: gl-lock serialises the display so host and client cannot both run, and the lobby's
# "Create Game" click does not fire. Those are harness faults; driving the UI here would confound
# them with the netcode. This drives the object through the SAME COM entry point the game uses
# (ma_dplay_create <- CoCreateInstance(CLSID_DirectPlay)), so if this passes and the game still
# cannot host, the fault is provably in the UI path.
#
# Needs NO display -- pure sockets, so it runs anywhere and does not queue behind gl-lock.
#
# THREE ARMS, all required:
#   solo  no host running   -> must find 0 sessions and return DP_OK (an empty list is the honest
#                              answer; reporting an error would misdescribe "nobody is hosting")
#   host  Open(CREATE)      -> must RECEIVE the client's packet
#   join  EnumSessions/Open -> must find the session and Send
#
# The solo arm is the negative control: it proves discovery can come back EMPTY, so a passing
# host/join arm means a packet really crossed rather than the probe reporting success regardless.
set -u
ROOT="/home/admin/ma"
PROBE="${PROBE:-/tmp/ma_dplay_probe}"
OBJ="$ROOT/build/CMakeFiles/ma_obj.dir/SRC/compat/ma_dplay.cpp.o"
if [ ! -x "$PROBE" ] || [ "$ROOT/port/dplay_probe.cpp" -nt "$PROBE" ] || [ "$OBJ" -nt "$PROBE" ]; then
  echo "  building probe..."
  g++ -m32 -fno-pie -no-pie -w -DMA_LINUX -DFF_LINUX -I "$ROOT/SRC/compat" -I "$ROOT/SRC/H" \
      "$ROOT/port/dplay_probe.cpp" "$OBJ" -o "$PROBE" || { echo "  FAIL: probe did not build"; exit 2; }
fi
export MA_DPLAY_PORT="${MA_DPLAY_PORT:-47625}"
fail=0
say(){ printf '  %-44s %s\n' "$1" "$2"; }
bad(){ say "$1" "$2"; fail=1; }

echo "MA PO-76 -- two processes exchange a DirectPlay packet (port $MA_DPLAY_PORT)"

# 1. negative control FIRST: discovery must come back empty when nobody is hosting.
if "$PROBE" solo >/tmp/ma_dp_solo.txt 2>&1; then say "control: no host -> 0 sessions" "yes"
else bad "control: no host -> 0 sessions" "NO -- discovery found a session with no host"; fi

# 2. host + client
( "$PROBE" host >/tmp/ma_dp_host.txt 2>&1 & )
sleep 2
"$PROBE" join >/tmp/ma_dp_join.txt 2>&1; jrc=$?
sleep 3
grep -q "^PASS host" /tmp/ma_dp_host.txt && say "host RECEIVED the client's packet" "yes" \
                                      || bad "host RECEIVED the client's packet" "NO"
[ "$jrc" -eq 0 ] && say "client found the session and sent" "yes" \
                 || bad "client found the session and sent" "NO (rc=$jrc)"
pkill -x ma_dplay_probe 2>/dev/null

echo "----------------------------------------"
if [ "$fail" -eq 0 ]; then
  echo "PASS: discovery, join and a packet across two processes"
  grep -a "RECEIVED" /tmp/ma_dp_host.txt | sed 's/^/  /'
  exit 0
fi
echo "FAIL -- see /tmp/dp_{solo,host,join}.txt"; exit 1
