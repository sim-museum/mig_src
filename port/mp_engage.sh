#!/usr/bin/env bash
# port/mp_engage.sh -- MPFLY-1/2 (2026-09-26): two headless wmig instances, host + joiner, on
# SEPARATE data trees, that fly together and ENGAGE, and assert what ma_mp_two_instance.sh never
# did: that each peer holds the other's aircraft where the owner says it is, that hits cross the
# wire, and that the joiner built the SAME world as the host (the MPFLY-1 fatal was the joiner
# loading a runway battlefield twice and running RunwaySBAND out of UIDs).
#
#   ARM=engage (default)  both fly; the host forms up 150 m behind the joiner and fires
#   ARM=jip               the host flies FIRST, the joiner joins the game in progress
#
#   HGD / CGD   host / joiner data dirs (<drive_c>/rowan/mig). Default: the dev tree for the host and
#               ~/ma-gates/mpfly/po_joiner_drive_c (a copy of the PO's installed tree) for the joiner.
#   BIN         the binary (default build/wmig).  BFDELAY  host bfield send delay ms (default 300 --
#               a slow link, the MPFLY-1 window).  OUT  log dir.
# Headless (SDL dummy), so the two windows cannot occlude each other (occluded-window-runs-at-1fps).
# Takes NO gl-lock itself; run it under one.
set -u
ROOT=/home/admin/ma
BIN="${BIN:-$ROOT/build/wmig}"
HGD="${HGD:-/home/admin/sgl/TUE/MigAlley/WP/drive_c/rowan/mig}"
CGD="${CGD:-/home/admin/ma-gates/mpfly/po_joiner_drive_c/rowan/mig}"
ARM="${ARM:-engage}"
OUT="${OUT:-/home/admin/ma-gates/ma_mp_engage/$ARM}"
BFDELAY="${BFDELAY:-300}"
PORT="${MA_DPLAY_PORT:-47734}"
CLIENT_DELAY=30
case "$ARM" in
  engage) SECS=330; HOST_FLY_MS=190000; CLIENT_FLY_MS=170000
          # the joiner levels off; the SHOOTER puts itself 150 m behind the joiner's aircraft as it
          # holds it (a negative distance = behind) and fires down the line
          HENV="MA_MP_FORMUP=20:-150 MA_MP_FIRE_AT=21:8"; CENV="MA_MP_LEVEL=17" ;;
  jip)    SECS=420; HOST_FLY_MS=90000; CLIENT_FLY_MS=200000; HENV=""; CENV="" ;;
  # MPKILL-1: the host stays 80 m behind the joiner (re-forming every 2 s, at the joiner's speed)
  # and fires for 25 s -- long enough to shoot it down; both scoreboards must then agree.
  kill)   SECS=360; HOST_FLY_MS=190000; CLIENT_FLY_MS=170000
          # the joiner is forced into the ground at +24 s, a few seconds after the first hit (two
          # unflown jets rarely trade more than one hit): the hitter must get the kill on BOTH peers
          HENV="MA_MP_FORMUP=20:-80 MA_MP_FORMUP_EVERY=2 MA_MP_FIRE_AT=21:25"; CENV="MA_MP_LEVEL=17 MA_MP_CRASH_AT=24" ;;
  # MPCHAT-1: nobody flies; each side clicks the Ready Room chat box, types a line and presses RETURN
  chat)   SECS=200; HOST_FLY_MS=999999; CLIENT_FLY_MS=999999; HENV=""; CENV=""
          HPOST="160000,#2144@CReadyRoom"; HTYPE="164000,hello from the host\\n"
          CPOST="140000,#2144@CReadyRoom"; CTYPE=";144000,hi from the joiner\\n" ;;
  *) echo "ARM must be engage, jip, kill or chat" >&2; exit 2 ;;
esac
# QM + jip: the QM frag/seat steps add ~40 s to each side, so the host (killed by its timeout 4 s
# after the joiner arrived, jip_qm1) gets a longer window and a levelled jet to still be flying in.
# jip_qm2: a QM host whose unflown jet crashes has no respawn -- its flight ends ~130 s in -- so the
# joiner must arrive well inside that: it flies 70 s earlier than in DM/TP.
if [ "$ARM" = jip ] && [ "${GT:-dm}" = qm ]; then SECS=540; CLIENT_FLY_MS=130000; HENV="MA_MP_LEVEL=17 MA_MP_LEVEL_EVERY=3${HENV:+ $HENV}"; fi
HENV="${HENV_OVERRIDE:-$HENV}"; CENV="${CENV_OVERRIDE:-$CENV}"
# GT=dm (default) | tp: the host picks the game type in its locker room (#2323 row) and a side
# (#2324 row 0 = UN); in Team Play the joiner picks the other side (row 1 = Red).
GT="${GT:-dm}"
case "$GT" in
  dm) ;;
  tp) HX="50000,#2323@CLockerRoom:1;54000,#2324@CLockerRoom:0${HX:+;$HX}"; CX="100000,#2324@CLockerRoom:${JSIDE:-1}${CX:+;$CX}" ;;
  # qm: the host picks Quick Mission (#2323 row 2); seats are then taken on the frag screen
  # (after the joiner is in the locker room: once the host leaves for the QM picker, nobody can join)
  # QM screens (FULLPANE.CPP): commsquick = [Title, Variants, Ready Room]; the host's Ready Room is
  # what creates the session, so it must be reached before the joiner looks (host 92 s). In the QM
  # Ready Room button 1 is FRAG (not Fly); on commsfrag button 1 is FLY, seats are #2144..2147@CFragPilot.
  qm) HX="50000,#2323@CLockerRoom:2${HX:+;$HX}"; HPOST="66000,#2063@RFullPanelDial:r0.2${HPOST:+;$HPOST}"
      HTAIL="$((HOST_FLY_MS + 6000)),#2144@CFragPilot;$((HOST_FLY_MS + 25000)),#2063@RFullPanelDial:r0.1"
      CTAIL="$((CLIENT_FLY_MS + 6000)),#${JSEAT:-2145}@CFragPilot;$((CLIENT_FLY_MS + 15000)),#2063@RFullPanelDial:r0.1" ;;
  *) echo "GT must be dm, tp or qm" >&2; exit 2 ;;
esac
[ -x "$BIN" ] || { echo "no binary at $BIN" >&2; exit 2; }
[ -d "$CGD" ] || { echo "no joiner tree at $CGD (copy an installed drive_c there)" >&2; exit 2; }
mkdir -p "$OUT"; rm -f "$OUT"/host.log "$OUT"/client.log
export MA_TRACE_UIDBAND=1 MA_TRACE_MPPOS=1 MA_TRACE_DPLAY=1 MA_TRACE_AGG=1 MA_DPLAY_PORT="$PORT" MA_DPLAY_HOST=127.0.0.1
hseq="20000,r2;40000,#2063@RFullPanelDial:r0.1;${HX:+$HX;}${HNEXT:-60000},#2063@RFullPanelDial:r0.1;${HPOST:+$HPOST;}${HOST_FLY_MS},#2063@RFullPanelDial:r0.1${HTAIL:+;$HTAIL}"
cseq="20000,r2;55000,#2063@RFullPanelDial:r0.2;62000,#2326@CSelectSession:r0;66000,#2063@RFullPanelDial:r0.1;74000,#2321@CLockerRoom;82000,#2320@CLockerRoom;${CX:+$CX;}120000,#2063@RFullPanelDial:r0.1;${CPOST:+$CPOST;}${CLIENT_FLY_MS},#2063@RFullPanelDial:r0.1${CTAIL:+;$CTAIL}"
echo "MA mp engage  arm=$ARM  host=$HGD  joiner=$CGD  bfdelay=${BFDELAY}ms"
( cd "$HGD" && BOB_DRIVE_C="${HGD%/rowan/mig}" SDL_VIDEODRIVER=dummy timeout -s INT "$SECS" env $HENV MA_MP_BFSEND_DELAY_MS="$BFDELAY" \
    MA_DUMP_MENU=1 BOB_CLICKSEQ_MS=1 MA_TRACE_3D=1 MA_TRACE_ADDPLAYER=1 MA_TRACE_IAMIN=1 BOB_CLICKSEQ="$hseq" MA_TYPESEQ="${HTYPE:-}" "$BIN" ) >"$OUT/host.log" 2>&1 &
hpid=$!
sleep "$CLIENT_DELAY"
( cd "$CGD" && BOB_DRIVE_C="${CGD%/rowan/mig}" SDL_VIDEODRIVER=dummy timeout -s INT "$((SECS - CLIENT_DELAY))" env $CENV \
    MA_DUMP_MENU=1 BOB_CLICKSEQ_MS=1 MA_TRACE_3D=1 MA_TRACE_ADDPLAYER=1 MA_TRACE_IAMIN=1 BOB_CLICKSEQ="$cseq" \
    MA_TYPESEQ="73000,Viper2;85000,MAGAME${CTYPE:-}" "$BIN" ) >"$OUT/client.log" 2>&1 &
cpid=$!
wait $hpid 2>/dev/null; wait $cpid 2>/dev/null
fail=0
say() { printf '  %-58s %s\n' "$1" "$2"; }
chk() { if eval "$2"; then say "$1" PASS; else say "$1" "FAIL${3:+ -- $3}"; fail=1; fi; }
if [ "$ARM" = chat ]; then
  chk "joiner receives the host's line" "grep -aq 'chat\] received from .*hello from the host' '$OUT/client.log'"
  chk "host receives the joiner's line" "grep -aq 'chat\] received from .*hi from the joiner' '$OUT/host.log'"
  printf '  logs: %s/{host,client}.log\n' "$OUT"
  [ "$fail" = 0 ] && echo "  MA MP ENGAGE ($ARM): PASS" || echo "  MA MP ENGAGE ($ARM): FAIL"
  exit $fail
fi
for w in host client; do
  chk "$w enters the 3-D" "grep -aq 'Launch3d returned' '$OUT/$w.log'"
  chk "$w: no UID band ran out" "! grep -aq 'assignuid FAILED' '$OUT/$w.log'" "$(grep -a -m1 'assignuid FAILED' "$OUT/$w.log")"
  chk "$w syncs (csync=1)" "grep -aq 'csync=1' '$OUT/$w.log'"
done
# the joiner loads exactly the host's battlefield list, in order, each once
hb=$(grep -a '\[bfload\]' "$OUT/host.log" | awk '{print $2}' | tr '\n' ' ')
cb=$(grep -a '\[bfload\]' "$OUT/client.log" | awk '{print $2}' | tr '\n' ' ')
chk "joiner loaded the host's battlefields, same order" "[ -n \"$hb\" ] && [ \"$hb\" = \"$cb\" ]" "host: $hb / joiner: $cb"
# each peer sees the OTHER's aircraft move: the remote slot's position changes over the flight
for w in host client; do
  n=$(grep -a '\[mppos\]' "$OUT/$w.log" | grep -av '(me)' | awk '{print $7}' | sort -u | wc -l)
  chk "$w sees the other aircraft move ($n distinct positions)" "[ $n -ge 5 ]"
done
if [ "$ARM" = kill ]; then
  ht=$(grep -a '\[mpscore\] table' "$OUT/host.log" | tail -n 1); ct=$(grep -a '\[mpscore\] table' "$OUT/client.log" | tail -n 1)
  chk "joiner was hit by the host, then died" "grep -aq 'mpcoll\] from slot 0 uid=.* type=0' '$OUT/client.log' && grep -aq 'slot1 k=0 d=1' <<<\"\$ct\""
  chk "host is credited with the kill (slot0 k=1)" "grep -q 'slot0 k=1' <<<\"\$ht\""
  chk "both scoreboards agree" "[ -n \"\$ht\" ] && [ \"\$ht\" = \"\$ct\" ]" "host:\$ht / joiner:\$ct"
fi
if [ "$ARM" = engage ]; then
  chk "host formed up behind the joiner" "grep -aq 'mpcombat\] formup' '$OUT/host.log'"
  chk "host fired" "grep -aq 'SHOOT held' '$OUT/host.log'"
  hs=$(grep -a -o 'slot=[0-9](me)' "$OUT/host.log" | head -1 | tr -dc 0-9)
  ju=$(grep -a -o '(me) uid=0x[0-9a-f]*' "$OUT/client.log" | head -1 | sed 's/.*uid=//')
  # a hit on the JOINER's aircraft reported by the HOST (the shooter reports its bullets' hits)
  for w in host client; do
    chk "$w: host-reported hit on the joiner's aircraft ($ju)" \
        "[ -n '$hs' ] && [ -n '$ju' ] && grep -aq '\[mpcoll\] from slot $hs uid=$ju ' '$OUT/$w.log'"
  done
fi
if [ "$ARM" = jip ]; then
  chk "joiner is not refused (no 'Incorrect password')" "! grep -aq 'Incorrect password' '$OUT/client.log'"
  chk "host sees the joiner arrive (AddPlayerToGame)" "grep -aq '\[addplayer\]' '$OUT/host.log'"
fi
printf '  logs: %s/{host,client}.log\n' "$OUT"
[ "$fail" = 0 ] && echo "  MA MP ENGAGE ($ARM): PASS" || echo "  MA MP ENGAGE ($ARM): FAIL"
exit $fail
