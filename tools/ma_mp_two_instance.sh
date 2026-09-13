#!/usr/bin/env bash
# tools/ma_mp_two_instance.sh -- MPTEST-MA: two wmig instances, host + client, over loopback.
#
# The screen path, read off MA_DUMP_MENU with every COLUMN of the horizontal action bar printed
# (S6 -- reading column 0 only made the bar look like it held just "Back"):
#
#   main menu            r2                              Multi-Player
#   CSelectService       -- do NOT click the provider row #2325:r0. S9 measured that clicking it
#                        advances straight to the locker room BY ITSELF, which silently implies
#                        Create Game and means the Create/Join choice is never made. The bar is
#                        already on screen at that point, and GetSessions (FULLPANE.CPP:3797) calls
#                        UISelectServiceProvider itself, so the provider does not need choosing
#                        first. Click the BAR.
#   service bar          #2063@RFullPanelDial:r0.1       "Create Game"   (host)
#                        #2063@RFullPanelDial:r0.2       "Join Game"     (client)
#   CSelectSession (client) #2326:r0 -- the HOST'S SESSION, listed by name. S11 proved the client
#                        reaches this screen and the list reads row 0 "MiG Alley", but the harness
#                        walked past it to the locker room without selecting anything, so no join
#                        was ever requested. Click the row.
#   CLockerRoom (client) #2321 Name, #2320 Session (MA_TYPESEQ), #2323:r0 GAME TYPE,
#                        #2324:r0 SELECT SIDE (created BY the game-type click), bar :r0.1 "Continue"
#   CReadyRoom           bar row 0: Quit / Fly / Visitors / Radio / Paint Shop / Prefs
#                        #2063:r0.1 = Fly
#
# Everything is scheduled in WALL-CLOCK MILLISECONDS (BOB_CLICKSEQ_MS), because the pump stalls
# exactly where the comms screens block -- a tick-scheduled click there is never delivered, not late.
set -u
ROOT=/home/admin/ma
BIN="${BIN:-$ROOT/build/wmig}"
GD="${GD:-/home/admin/sgl/TUE/MigAlley/WP/drive_c/rowan/mig}"
OUT="${OUT:-/home/admin/Documents/260912/logs/ma_mp2}"
SECS="${SECS:-260}"
CLIENT_DELAY="${CLIENT_DELAY:-30}"
# S10: the host does NOT open its session at "Create Game". CreateCommsGame (FULLPANE.CPP:3837) only
# selects the provider and sets UIPlayerType=PLAYER_HOST; the session is created in UINewPlayer,
# called from the READY ROOM (READY.CPP:204) after the locker-room CONTINUE. So the host must press
# Continue BEFORE the client probes, or the client enumerates an empty lobby and gives up -- which
# is exactly what run 3 measured (client "EnumSessions -> 0 session(s)", host with no Open() at all).
HOST_OPEN_MS="${HOST_OPEN_MS:-60000}"     # locker-room Continue -> UINewPlayer -> Open(CREATE)
HOST_FLY_MS="${HOST_FLY_MS:-190000}"
# S8: the shim (SRC/compat/ma_dplay.cpp) prints nothing unless MA_TRACE_DPLAY is set, so run 1's
# silent logs said nothing about the link -- the empty player table was the only real evidence.
# It also answers discovery ONLY from pump(), which runs only when the game calls
# Receive/GetMessageCount/EnumSessions, so "nobody can find my session" and "the host is not
# pumping" are the same symptom from outside; the pump counter separates them.
MA_DPLAY_PORT="${MA_DPLAY_PORT:-47624}"
MA_DPLAY_HOST="${MA_DPLAY_HOST:-127.0.0.1}"
export MA_TRACE_DPLAY=1 MA_DPLAY_PORT MA_DPLAY_HOST
mkdir -p "$OUT"
[ -x "$BIN" ] || { echo "no binary at $BIN" >&2; exit 2; }
echo "MA two-instance  (host fly at ${HOST_FLY_MS}ms, client +${CLIENT_DELAY}s, ${SECS}s)"
( cd "$GD" && timeout -s INT "$SECS" env MA_DUMP_MENU=1 BOB_CLICKSEQ_MS=1 MA_TRACE_CLICK=1 \
    BOB_CLICKSEQ="20000,r2;40000,#2063@RFullPanelDial:r0.1;${HOST_OPEN_MS},#2063@RFullPanelDial:r0.1;${HOST_FLY_MS},#2063@RFullPanelDial:r0.1" \
    "$BIN" ) >"$OUT/host.log" 2>&1 &
hpid=$!
sleep "$CLIENT_DELAY"
( cd "$GD" && timeout -s INT "$((SECS - CLIENT_DELAY))" env MA_DUMP_MENU=1 BOB_CLICKSEQ_MS=1 MA_TRACE_CLICK=1 \
    BOB_CLICKSEQ="20000,r2;55000,#2063@RFullPanelDial:r0.2;62000,#2326@CSelectSession:r0;70000,#2321@CLockerRoom;82000,#2320@CLockerRoom;96000,#2323@CLockerRoom:r0;108000,#2324@CLockerRoom:r0;120000,#2063@RFullPanelDial:r0.1" \
    MA_TYPESEQ="73000,Viper2;85000,MAGAME" \
    "$BIN" ) >"$OUT/client.log" 2>&1 &
cpid=$!
wait $hpid 2>/dev/null; wait $cpid 2>/dev/null
fail=0
say() { printf '  %-46s %s\n' "$1" "$2"; }
grep -aq 'CReadyRoom' "$OUT/host.log"   && say "host reaches the Ready Room" "PASS" || { say "host reaches the Ready Room" "FAIL"; fail=1; }
grep -aq 'CLockerRoom' "$OUT/client.log" && say "client reaches the locker room" "PASS" || { say "client reaches the locker room" "FAIL"; fail=1; }
grep -aq 'CReadyRoom' "$OUT/client.log" && say "client reaches the Ready Room" "PASS" || { say "client reaches the Ready Room" "FAIL"; fail=1; }
hp=$(grep -ac 'pump #' "$OUT/host.log"); cp=$(grep -ac 'pump #' "$OUT/client.log")
say "shim speaks (host/client pump lines)" "$([ "$hp" -gt 0 ] && [ "$cp" -gt 0 ] && echo PASS || echo "FAIL h=$hp c=$cp")"
[ "$hp" -gt 0 ] && [ "$cp" -gt 0 ] || fail=1
grep -aq 'probe' "$OUT/host.log" && say "host answers a discovery probe" "PASS" || { say "host answers a discovery probe" "FAIL"; fail=1; }
# S9: 'row 2: [0]' also matches the MAIN MENU's third item ("Load Game"), so run 2 scored this PASS
# on a host whose Ready Room table held one row. Scope it to a row that carries the table's own
# column count (the player table prints six columns; every menu list prints one).
if grep -aq 'row 2: \[0\].*\[5\]' "$OUT/host.log"; then say "host table shows a SECOND player" "PASS"
else say "host table shows a SECOND player" "FAIL -- only the host is listed"; fail=1; fi
n=$(grep -ac 'row 1: \[0\]' "$OUT/host.log")
say "host's player table has rows ($n dumps)" "$([ "$n" -gt 0 ] && echo PASS || echo FAIL)"
printf '  logs: %s/{host,client}.log\n' "$OUT"
[ "$fail" = 0 ] && echo "  MA TWO-INSTANCE: PASS" || echo "  MA TWO-INSTANCE: FAIL"
exit $fail
