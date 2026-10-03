#!/bin/bash
# EPIC-MP-CAMPAIGN: two instances in Rowan's revived co-op campaign (GameType COMMSCAMPAIGN).
# Host: Multi-Player -> Create -> Locker Room: Campaign (#2323 row 3), campaign MA_COMMSCAMP_INDEX (4 = Spring
# Offensive) -> Continue -> campaign map -> frag (#1905@CMainToolbar) -> campaign Ready Room (session opens).
# Joiner: starts JDELAY s later -> Join -> session row -> Locker Room -> Continue -> guest campaign Ready Room.
# Disposable trees only (HGD/CGD). Stages are traced with MA_DUMP_MENU; extend with HX/CX extra clicks.
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${BIN:-$ROOT/build/wmig}"
HGD="${HGD:-$HOME/ma-keys/drive_c/rowan/mig}"; CGD="${CGD:-$HOME/ma-crawl/drive_c/rowan/mig}"
OUT="${OUT:-$HOME/ma-gates/mpcamp/run}"; SECS="${SECS:-420}"; JDELAY="${JDELAY:-110}"; PORT="${MA_DPLAY_PORT:-47734}"
CAMPIDX="${CAMPIDX:-4}"
mkdir -p "$OUT"; rm -f "$OUT"/host.log "$OUT"/client.log
export MA_TRACE_DPLAY=1 MA_TRACE_UIDBAND=1 MA_TRACE_MPPOS=1 MA_TRACE_AGG=1 MA_DPLAY_PORT="$PORT" MA_DPLAY_HOST=127.0.0.1
hseq="20000,r2;40000,#2063@RFullPanelDial:r0.1;50000,#2323@CLockerRoom:3;60000,#2063@RFullPanelDial:r0.1;${HFRAG:-100000},#1905@CMainToolbar${HX:+;$HX}"
cseq="20000,r2;35000,#2063@RFullPanelDial:r0.2;42000,#2326@CSelectSession:r0;46000,#2063@RFullPanelDial:r0.1;54000,#2321@CLockerRoom;60000,#2320@CLockerRoom;${CCONT:-90000},#2063@RFullPanelDial:r0.1${CX:+;$CX}"
echo "MA mp campaign  host=$HGD  joiner=$CGD  joiner +${JDELAY}s  ${SECS}s"
( cd "$HGD" && BOB_DRIVE_C="${HGD%/rowan/mig}" SDL_VIDEODRIVER=dummy timeout -s INT "$SECS" env ${HENV:-} MA_COMMSCAMP_INDEX="$CAMPIDX" \
    MA_DUMP_MENU=1 BOB_CLICKSEQ_MS=1 MA_TRACE_3D=1 MA_TRACE_ADDPLAYER=1 BOB_CLICKSEQ="$hseq" "$BIN" ) >"$OUT/host.log" 2>&1 &
sleep "$JDELAY"
( cd "$CGD" && BOB_DRIVE_C="${CGD%/rowan/mig}" SDL_VIDEODRIVER=dummy timeout -s INT "$((SECS - JDELAY))" env ${CENV:-} \
    MA_DUMP_MENU=1 BOB_CLICKSEQ_MS=1 MA_TRACE_3D=1 MA_TRACE_ADDPLAYER=1 BOB_CLICKSEQ="$cseq" "$BIN" ) >"$OUT/client.log" 2>&1 &
wait
for s in host client; do
  printf "%-6s crash=%s stall=%s 3D=%s panels=%s\n" "$s" "$(grep -acE '=== CRASH|SIGSEGV' "$OUT/$s.log")" \
    "$(grep -a 'STALLED' "$OUT/$s.log" | head -1 | sed 's/.*entry \([0-9]*\) ("\([^"]*\)").*/\1:\2/')" \
    "$(grep -ac 'Launch3d returned' "$OUT/$s.log")" "$(grep -aoE 'art [0-9]+ -> [^ ]+ art [0-9]+' "$OUT/$s.log" | awk '{print $NF}' | tr '\n' ' ')"
done
# acceptance (as port/mp_engage.sh): both in 3D, both sync, same battlefields in order, each sees the other move
# compare the FLIGHT's battlefields: from its main-world load (the last 0x8a01) on -- the host also loaded the map
# world while planning, the joiner did not
hb=$(grep -a '\[bfload\]' "$OUT/host.log" | awk '{print $2}' | tr '\n' ' ' | sed 's/.*file=0x8a01 /file=0x8a01 /')
cb=$(grep -a '\[bfload\]' "$OUT/client.log" | awk '{print $2}' | tr '\n' ' ' | sed 's/.*file=0x8a01 /file=0x8a01 /')
ok=1
for s in host client; do
  sync=$(grep -ac 'csync=1' "$OUT/$s.log"); n=$(grep -a '\[mppos\]' "$OUT/$s.log" | grep -av '(me)' | awk '{print $7}' | sort -u | wc -l)
  d3=$(grep -ac 'Launch3d returned' "$OUT/$s.log"); cr=$(grep -acE '=== CRASH|SIGSEGV' "$OUT/$s.log")
  printf "  %-6s 3D=%s sync=%s sees-other=%s crash=%s\n" "$s" "$d3" "$((sync>0))" "$n" "$cr"
  [ "$d3" -ge 1 ] && [ "$sync" -ge 1 ] && [ "$n" -ge 5 ] && [ "$cr" -eq 0 ] || ok=0
done
[ -n "$hb" ] && [ "$hb" = "$cb" ] && echo "  same battlefields: yes" || { echo "  same battlefields: NO"; ok=0; }
[ $ok = 1 ] && echo "MP CAMPAIGN: PASS" || echo "MP CAMPAIGN: FAIL"
