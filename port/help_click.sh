#!/usr/bin/env bash
# port/help_click.sh — gate for PO-4: the "?" on a dialog title bar reaches the help system.
#
# Reported as: dialogs come up, but clicking "?" yields no documentation screen. The click was
# being dropped in FOUR places, all in the port:
#   1. the title-bar hit router returned early for the help band ("nothing to route to yet")
#   2. WM_COMMANDHELP was not defined at all -- while ON_MESSAGE expanded to nothing it never
#      evaluated its argument, so the symbol had never been required to exist (§8-MA83)
#   3. CWnd::SendMessage only dispatched WM_USER+ messages, and WM_COMMANDHELP is 0x0365
#   4. CWnd::OnCommandHelp was a non-virtual stub returning 0, so CMainFrame's override -- the
#      thing that actually opens help -- could never be reached through a CWnd*
#
# WHAT THIS GATE DOES AND DOES NOT CLAIM. It proves the click reaches CMainFrame::OnCommandHelp,
# which calls CWinApp::WinHelp(HID_BASE_RESOURCE + IDD_x). It does NOT prove a documentation
# screen appears, because there is no WinHelp viewer in the port yet -- WinHelp is still a stub.
# The shipped English/TEXT/MIG.HLP has 44 topics and 35 context mappings (see
# port/tools/hlp_probe.py); rendering them is separate, scoped work. Naming that boundary here is
# the point: this gate must not be read as "help works".
#
# The "?" is clicked via `#1001@CPlyr_log:?`, which asks the title-bar control's OWN hit-test where
# its help band is. Its glyphs move with the dialog's width and font, so a recipe naming a pixel
# would be testing that pixel (S95's rule; S96 moved the screen edge twice in one sprint).
set -u
# S118 (PO-12 phase 4): hardware is now a PLAYER SETTING, so an unpinned gate would test
# whichever renderer settings.mig happens to hold. Pin the software path here -- that is
# what these references were captured from. MA_NO_HARDWARE=0 runs the same gate on the
# hardware path.
export MA_NO_HARDWARE="${MA_NO_HARDWARE:-1}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WMIG="${WMIG:-$ROOT/build/wmig}"
BOB_DRIVE_C="${BOB_DRIVE_C:-$HOME/sgl/TUE/MigAlley/WP/drive_c}"
RUNDIR="$BOB_DRIVE_C/rowan/mig"
OUT="${OUT:-/tmp/ma_help}"
TMO="${TMO:-130}"
NAV="30,r3;65,#1055;100,#2063:1"

mkdir -p "$OUT"
[ -x "$WMIG" ] || { echo "no binary at $WMIG" >&2; exit 2; }

SAVEFILE="$RUNDIR/SaveGame/Auto Save.sav"
PIN="$ROOT/port/ref/save/campaign_pristine.sav"
STASH="$(mktemp -u /tmp/ma_help_save.XXXXXX)"
[ -f "$SAVEFILE" ] && cp -a "$SAVEFILE" "$STASH"
restore_save() { [ -f "$STASH" ] && cp -a "$STASH" "$SAVEFILE" && rm -f "$STASH"; }
trap restore_save EXIT INT TERM
[ -f "$PIN" ] && cp -f "$PIN" "$SAVEFILE"

log="$OUT/help.log"; ppm="$OUT/help.ppm"; rm -f "$ppm"
( cd "$RUNDIR" && timeout -k 5 -s KILL "$TMO" env \
    SDL_VIDEODRIVER=dummy BOB_RUN_INIT=1 BOB_DRIVE_C="$BOB_DRIVE_C" MA_DISABLE_3D=1 \
    MA_IGNORE_SAVE_DATE=1 MA_TRACE_CLICK=1 MA_TRACE_HELP=1 \
    BOB_CLICKSEQ="$NAV;250,#2064@CMainToolbar;340,#1001@CPlyr_log:?" \
    MA_SHOT=420 MA_SHOT_PATH="$ppm" \
    "$WMIG" ) >"$log" 2>&1
rc=$?
pkill -x "$(basename "$WMIG")" 2>/dev/null

echo "help click — $(basename "$WMIG")"
ok=0
if [ "$rc" -ne 0 ] && [ "$rc" -ne 124 ] && [ "$rc" -ne 137 ]; then
  echo "  RESULT: CRASH (exit $rc) — see $log"; exit 1
fi

resolved=$(grep -a "clickid] id=1001 col=-2 ->" "$log" | head -1)
if [ -n "$resolved" ]; then echo "  help glyph located by the control itself: ${resolved#*-> }"
else echo "  the title bar never reported a help band — see $log"; ok=1; fi

band=$(grep -ac "TITLE local=.*dispid 0 (Help)" "$log")
if [ "$band" -gt 0 ]; then echo "  click landed in the help band: yes"
else echo "  click did NOT land in the help band"; ok=1; fi

routed=$(grep -ac "HELP -> WM_COMMANDHELP .* returned 1" "$log")
if [ "$routed" -gt 0 ]; then echo "  WM_COMMANDHELP handled by CMainFrame: yes (returned 1)"
else
  zero=$(grep -ac "HELP -> WM_COMMANDHELP .* returned 0" "$log")
  if [ "$zero" -gt 0 ]; then echo "  WM_COMMANDHELP sent but NOTHING handled it (returned 0)"
  else echo "  WM_COMMANDHELP was never sent"; fi
  ok=1
fi

# S112: assert on the CAPTURE, not only on the log. "The message was handled" was exactly the
# half-truth this gate used to report -- routing had been correct since S98 while the player still
# saw nothing. The panel paints a wide single-colour title strip; look for it.
if [ -s "$ppm" ] && python3 - "$ppm" <<'PY2'
import sys
from PIL import Image
im = Image.open(sys.argv[1]).convert('RGB'); w, h = im.size
px = im.load()
band = [px[x, 120] for x in range(60, w - 60, 4)]
same = max(band.count(c) for c in set(band))
sys.exit(0 if same > len(band) * 0.8 else 1)
PY2
then echo "  documentation panel on screen: yes"
     res=$(grep -a -m1 "\[help\] context .* -> topic" "$log")
     [ -n "$res" ] && echo "  ${res#*] }"
else echo "  documentation panel on screen: NO"; ok=1
fi
echo "  NOTE (S114): the panel shows the topic's REAL TEXT, extracted from the game's own help"
echo "        SOURCE (SRC/<LANG>/HELP/MIG.RTF) by port/tools/rtf_help.py -- the compiled MIG.HLP's"
echo "        Hall compression never had to be decoded."
[ "$ok" -eq 0 ] && { echo "  RESULT: PASS (click -> documentation panel)"; exit 0; }
echo "  RESULT: FAIL — see $log"; exit 1
