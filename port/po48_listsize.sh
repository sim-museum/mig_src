#!/usr/bin/env bash
# port/po48_listsize.sh -- PO-48 S5: the title menu listbox comes back DOUBLE SIZE after a campaign
# exit (S3: 105x100 -> 213x199, same seven items, same origin; S4: same layout index both times, and
# the after-exit title differs from the REAL GAME by 20,911 px).
#
# S4 named the experiment: print the size where it is SET. MA_TRACE_LISTSIZE (S5, in
# CRListBoxCtrl::Shrink and ::ResizeToFit) prints the terms of the round trip that computes it --
# Shrink stores bestwidth*16/tmHeight, ResizeToFit multiplies back by tmHeight/16 -- so the two
# title visits can be compared term by term instead of by their result.
#
# The drive is sysbox_exit.sh's, which is the recipe that reaches the map and answers the QUIT
# confirmation: title -> campaign -> map -> X -> Yes -> title.
set -u
export MA_NO_HARDWARE="${MA_NO_HARDWARE:-1}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WMIG="${WMIG:-$ROOT/build/wmig}"
BOB_DRIVE_C="${BOB_DRIVE_C:-$HOME/sgl/TUE/MigAlley/WP/drive_c}"
RUNDIR="$BOB_DRIVE_C/rowan/mig"
OUT="${OUT:-/tmp/ma_po48}"
TMO="${TMO:-200}"
NAV="30,r3;65,#1055;100,#2063:1"
mkdir -p "$OUT"
[ -x "$WMIG" ] || { echo "no binary at $WMIG" >&2; exit 2; }
pgrep -x "$(basename "$WMIG")" >/dev/null && { echo "REFUSING: wmig already running" >&2; exit 2; }
SAVEFILE="$RUNDIR/SaveGame/Auto Save.sav"
PIN="$ROOT/port/ref/save/campaign_pristine.sav"
STASH="$(mktemp -u /tmp/ma_po48_save.XXXXXX)"
[ -f "$SAVEFILE" ] && cp -a "$SAVEFILE" "$STASH"
restore_save() { [ -f "$STASH" ] && cp -a "$STASH" "$SAVEFILE" && rm -f "$STASH"; }
trap restore_save EXIT INT TERM
[ -f "$PIN" ] && cp -f "$PIN" "$SAVEFILE"
log="$OUT/po48.log"
( cd "$RUNDIR" && timeout -k 5 -s KILL "$TMO" env \
    SDL_VIDEODRIVER=dummy BOB_RUN_INIT=1 BOB_DRIVE_C="$BOB_DRIVE_C" MA_DISABLE_3D=1 \
    MA_IGNORE_SAVE_DATE=1 MA_TRACE_LISTSIZE=1 MA_DUMP_MENU=1 MA_TRACE_CLICK=1 \
    BOB_CLICKSEQ="$NAV;250,#10@CSystemBox;320,#2125@RMdlDlg" \
    "$WMIG" ) >"$log" 2>&1
pkill -x "$(basename "$WMIG")" 2>/dev/null
echo "PO-48 S5 -- listbox size terms"
echo "  [listsize] lines: $(grep -ac '^\[listsize\]' "$log")"
# Prove the run reached the state it claims to test BEFORE reading any size out of it.
grep -aq "evt_fire] id=10 .*CSystemBox" "$log" \
  && echo "  reached the X (CSystemBox) : yes" || echo "  ⚠ never reached the X -- there is no SECOND title visit in this log"
echo
grep -a '^\[listsize\]' "$log" | head -40
