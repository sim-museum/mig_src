#!/usr/bin/env bash
# port/oob_sweep.sh — drive EVERY campaign-map toolbar dialog and report which ones open (S86).
#
# Until S82 the OOB dialogs were render-only (no clicks reached them at all); S84/S85 cleared the
# two that crashed. This sweep answers the obvious follow-up — "do the REST of them work?" — as a
# repeatable command rather than a one-off, in the same spirit as port/parity_2d.sh.
#
#   port/oob_sweep.sh [name ...]      (default: all)
#
# Each dialog is driven the way a player would: the campaign nav recipe to the map, then a click on
# the toolbar button, resolved by CONTROL ID QUALIFIED BY HOST CLASS (`#ID@CMainToolbar`). Numeric
# ids are not unique — RESOURCE.H has five symbols for 2074 — so the qualifier is what makes the
# recipe mean what it says (S85).
#
# Verdict per dialog:
#   OPEN   — the OOB paint walk reported a painted dialog, and the process survived
#   NONE   — the click fired but nothing opened (may be legitimate: some dialogs are gated on
#            campaign state; check the log before calling it a bug)
#   CRASH  — non-zero/abnormal exit
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WMIG="${WMIG:-$ROOT/build/wmig}"
BOB_DRIVE_C="${BOB_DRIVE_C:-$HOME/sgl/TUE/MigAlley/WP/drive_c}"
RUNDIR="$BOB_DRIVE_C/rowan/mig"
OUT="${OUT:-/tmp/oob_sweep}"
TMO="${TMO:-110}"
NAV="30,r3;65,#1055;100,#2063:1"        # title -> Campaign -> load "Auto Save" -> map
SHOT_AT="${SHOT_AT:-400}"

# name | control id | host class  (@Class keeps the id unambiguous — ids are reused, S85)
DIALOGS="
intelligence|2023|CMainToolbar
directives|2074|CMainToolbar
bases|2080|CMainToolbar
squads|2065|CMainToolbar
weather|2069|CMainToolbar
dis|2072|CMainToolbar
overview|2058|CMainToolbar
missionfolder|2140|CMainToolbar
playerlog|2064|CMainToolbar
"
# NOT on the planning map: IDC_MISSIONRESULTS (2055) is a control of CDebriefToolbar
# (DBRFTLBR.CPP:111/129), which replaces the main toolbar only while MMC.indebrief is set. A
# `#2055@CMainToolbar` probe correctly resolves to nothing — that is a correct negative, not a
# missing dialog, and it is recorded here so the next reader does not re-investigate it. Reaching
# it needs the post-mission debrief context (see MA_CAMP_FLY / the S80 loop).

mkdir -p "$OUT"
[ -x "$WMIG" ] || { echo "no binary at $WMIG" >&2; exit 2; }

# The campaign save is state the game WRITES (S81): stash it so a sweep never advances the
# player's campaign, exactly as asan_all.sh does.
SAVEFILE="$RUNDIR/SaveGame/Auto Save.sav"
PIN="$ROOT/port/ref/save/campaign_pristine.sav"
STASH="$(mktemp -u /tmp/ma_oobsweep_save.XXXXXX)"
[ -f "$SAVEFILE" ] && cp -a "$SAVEFILE" "$STASH"
restore_save() { [ -f "$STASH" ] && cp -a "$STASH" "$SAVEFILE" && rm -f "$STASH"; }
trap restore_save EXIT INT TERM
# S94: PIN a known save, do not merely preserve the player's. Every dialog here is reached by
# navigating the campaign through the Load dialog, so the whole sweep depends on campaign state.
# A PO play-test advanced the save and the next sweep reported 9 OPEN -> 0 with no code change --
# which reads exactly like a regression. parity_2d.sh learned this in S81; this gate had not.
[ -f "$PIN" ] && cp -f "$PIN" "$SAVEFILE"

want() { [ "$#" -eq 0 ] && return 0; for w in "$@"; do [ "$w" = "$NAME" ] && return 0; done; return 1; }

OPEN=0; NONE=0; CRASH=0
printf "OOB dialog sweep — %s\n" "$(basename "$WMIG")"
while IFS='|' read -r NAME ID HOST; do
  [ -z "${NAME:-}" ] && continue
  want "$@" || continue
  log="$OUT/$NAME.log"; ppm="$OUT/$NAME.ppm"; rm -f "$ppm"
  ( cd "$RUNDIR" && timeout -k 5 -s KILL "$TMO" env \
      SDL_VIDEODRIVER=dummy BOB_RUN_INIT=1 BOB_DRIVE_C="$BOB_DRIVE_C" MA_DISABLE_3D=1 \
      MA_IGNORE_SAVE_DATE=1 MA_TRACE_OOB=1 MA_TRACE_CLICK=1 \
      BOB_CLICKSEQ="$NAV;250,#$ID@$HOST" MA_SHOT="$SHOT_AT" MA_SHOT_PATH="$ppm" \
      "$WMIG" ) >"$log" 2>&1
  rc=$?
  pkill -x "$(basename "$WMIG")" 2>/dev/null
  fired=$(grep -ac "tbclick] id=$ID .*-> fire" "$log")
  painted=$(grep -ac "\[oob\] painted" "$log")
  syserr=$(grep -ac "SysError" "$log")
  if [ "$rc" -ne 0 ] && [ "$rc" -ne 124 ]; then
    printf "  %-15s CRASH (exit %d)%s\n" "$NAME" "$rc" "$([ "$syserr" -gt 0 ] && echo ' +SysError')"; CRASH=$((CRASH+1))
  elif [ "$painted" -gt 0 ]; then
    printf "  %-15s OPEN   (clicked=%s, %s paint passes)\n" "$NAME" "$fired" "$painted"; OPEN=$((OPEN+1))
  else
    printf "  %-15s NONE   (clicked=%s%s) — see %s\n" "$NAME" "$fired" \
           "$([ "$syserr" -gt 0 ] && echo ', SysError')" "$log"; NONE=$((NONE+1))
  fi
done <<EOF
$DIALOGS
EOF

echo "----------------------------------------"
echo "OPEN=$OPEN  NONE=$NONE  CRASH=$CRASH   (captures in $OUT)"
[ "$CRASH" -eq 0 ]
