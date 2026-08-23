#!/usr/bin/env bash
# port/overlay_text.sh — regression gate for the 3D OVERLAY TEXT layer (S102/S104).
#
# The overlay text layer (in-flight radio menu, map window text, info line) was invisible for the
# port's whole life and then visible-but-unreadable, and neither state failed any gate: a
# screenshot cannot tell "no glyphs" from "glyphs drawn as solid blocks", and a whole-frame diff
# cannot either (two IDENTICAL flight runs differ by ~2700 px — S100).
#
# So this gate measures the SHAPE of the ink, with the metric chosen per screen from what that
# screen's background actually allows. Both are calibrated against the two control arms that
# reproduce the historical failures exactly: MA_NO_ALPHATEXT=1 (solid blocks, the S101 state) and
# MA_NO_GLYPHS=1 (nothing at all, the pre-S100 state).
#
#   port/overlay_text.sh [screen ...]      (default: all)
#   ARMS=all port/overlay_text.sh          also run the control arms and print their scores
#
# Screens:
#   radio     the radio command menu — R opens it, it closes itself after 5 s (PO-7)
#   waypoint  the in-flight waypoint map — M then option 2; the screen the gold video shows (PO-6)
#   infoline  the bottom info line — checked on the LOG, see below (PO-8)
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
OUT="${OUT:-/tmp/ma_overlay_text}"
TMO="${TMO:-200}"
ARMS="${ARMS:-fix}"
CLICKSEQ="40,r1;95,r0"      # title -> Single Player -> Hot Shot (S63: rows, never pixels)

# The capture is ARMED BY THE EVENT, not aimed at a frame number: MA_UISCR_SHOT=N dumps the back
# surface N frames after an in-flight UI screen is promoted. These screens are opened by a key and
# close themselves after five seconds, and the pump counter that delivers the key runs at a
# completely different rate from the Blt counter that numbers frames — an absolute MA_DUMP_BACK
# number cannot hit one (measured: a tap at pump 500 and a dump at Blt 560 missed by seconds).
# Same lesson as S80.
#
# screen | key taps (BOB_KEYSEQ "pump,dik") | frames after promote | region x0,y0,x1,y1 | min edges
#        | armed key (MA_UISCR_KEY="dik,frames"), pressed N frames after a screen opens
# DIK: r=0x13 (RADIOCOMMS), m=0x32 (GOTOMAPKEY), 2=0x03 (RPM_20 = menu option 2) -- from the game's
# own binding dump (MA_DUMP_BINDINGS=1).
# `waypoint` is the screen the PO's gold video shows at ~90 s: M opens the map menu, option 2 opens
# waypointMapScr, whose right panel reads "1.Next WP = Highlighted WP / 2.Accel To Next WP / 0.Exit"
# and whose bottom strip is the waypoint table. Calibrated S107: letters 1179 edges vs 0 with
# MA_NO_ALPHATEXT=1. Its option key MUST be armed from the promote, not scheduled on the pump
# counter -- see the MA_UISCR_KEY note in OVERLAY.CPP.
# Calibrated S104 over the panel rect: letters 848 edges · solid blocks 351 · (the panel itself is
# light, so a bright-pixel count measures the PANEL and cannot separate the two — the metric has to
# be edge density here, unlike the flat-background case).
RECIPES="
radio|500,0x13|30|262,30,390,132|600|
waypoint|500,0x32|40|370,38,620,150|600|0x03,5
"
# The INFO LINE (PO-8) is checked on the log, not on pixels, and that is deliberate: it is drawn
# over live terrain and cockpit, so a bright-run count in that band measures the SCENERY. Measured
# S103: 53 runs with the info line ON, 59 with it OFF. What is unambiguous is whether the layer
# ran: DrawInfoBar returns early on infoLineCount==0 and says so.
INFOLINE_BLT=700

mkdir -p "$OUT"
[ -x "$WMIG" ] || { echo "no binary at $WMIG" >&2; exit 2; }
want() { [ "$#" -eq 0 ] && return 0; for w in "$@"; do [ "$w" = "$SCREEN" ] && return 0; done; return 1; }

measure() {   # measure <ppm> <x0,y0,x1,y1> <minedges> <label>
  # S188: the region argument is now IGNORED. It used to be a hardcoded rectangle, calibrated when
  # in-flight capture ran at a smaller back-surface size. Flight now renders at the display
  # resolution (1920x1080 here), the overlay panel sits at fixed pixel offsets rather than
  # proportional ones, and that fixed rectangle landed on EMPTY SKY. Every arm then scored the same
  # ~78 edges -- fix, MA_NO_ALPHATEXT and MA_NO_GLYPHS alike -- and the gate called a perfectly
  # legible radio menu "BLOCKS-OR-BLANK". A gate whose control arms score the SAME as its fix arm
  # is not measuring what it names, and that tell was sitting in the output of `ARMS=all`.
  #
  # So LOCATE the panel by its own translucent UI grey, the way panel_click.sh locates the menu,
  # and measure inside what was found. And split the two failures the old verdict conflated:
  #   NO PANEL         -- the screen never opened; nothing was drawn, so nothing can be said
  #                       about glyphs. This is what the `waypoint` screen actually does.
  #   BLOCKS-OR-BLANK  -- the panel is there and the ink inside it is wrong. The original defect.
  python3 - "$1" "$2" "$3" "$4" <<'MEASPY'
import sys
from PIL import Image
im  = Image.open(sys.argv[1]).convert('RGB')
rgb = im.load(); W,H = im.size
minedges = int(sys.argv[3]); label = sys.argv[4]

def locate():
    """The overlay panel is a large solid block of one flat UI grey in the upper screen."""
    from collections import Counter
    c = Counter(rgb[x,y] for y in range(0,H//2,2) for x in range(0,W,2))
    # NB the brightness window must span BOTH panels: the radio menu's translucent grey is
    # (120,128,128) and the waypoint map's paper panel is (232,240,240). The first cut of this
    # locator stopped at 190 and so found the radio panel but not the waypoint one -- and then
    # reported "the screen never opened" about a screen that had rendered perfectly, map, route
    # line, options and all. Widening it is not a loosened tolerance; it is the actual range of
    # the two panels this gate measures.
    for col,_ in c.most_common(12):
        r,g,b = col
        if not (abs(r-g) < 20 and abs(g-b) < 20 and 90 < r < 252): continue
        xs = [x for x in range(W) if sum(1 for y in range(H//2) if rgb[x,y] == col) > 20]
        ys = [y for y in range(H//2) if sum(1 for x in range(W) if rgb[x,y] == col) > 20]
        if not xs or not ys: continue
        if max(xs)-min(xs) < 40 or max(ys)-min(ys) < 40: continue
        return (min(xs), min(ys), max(xs)+1, max(ys)+1, col)
    return None

found = locate()
if found is None:
    print("  %-6s NO PANEL -- the screen never opened (nothing to measure)" % label)
    sys.exit(1)
x0,y0,x1,y1,col = found
box = (x0,y0,x1,y1)
g = im.convert('L').load()
bw = x1 - x0
# Measure the panel's INTERIOR only, row by row: the widest contiguous stretch of panel colour on
# that row, and only if it spans at least PURE of the panel's width. The waypoint map's panel is a
# notepad with a drawn SPIRAL BINDING across its top, and the binding is chrome, not ink -- with
# the whole bounding box measured, the MA_NO_GLYPHS control arm scored 1094 against a threshold of
# 600 and the gate would have passed a screen with no text on it at all, which is precisely the
# defect it exists to catch. Every one of those 1094 edges was in the binding.
#
# PURE was chosen by sweeping it against the control arms, which is what the arms are for:
#     0.30   radio 1347/56/0    waypoint 1188... no: 2282 fix vs 1094 blocks   <- no separation
#     0.60   radio 1347/56/0    waypoint 2120 fix vs  932 blocks               <- no separation
#     0.75   radio 1347/56/0    waypoint 1188 fix vs    0 blocks               <- clean
#     0.90   radio 1347/56/0    waypoint    0 fix vs    0 blocks               <- eats the text too
PURE = 0.75
edges = 0; longest = 0; rows = 0
for y in range(y0, y1):
    # widest run of panel colour, tolerating gaps up to GAP px so ink does not split a row
    GAP = 40
    best = (0, 0, 0); a = None; gap = 0; last = x0
    for x in range(x0, x1):
        if rgb[x,y] == col:
            if a is None: a = x
            gap = 0; last = x
        elif a is not None:
            gap += 1
            if gap > GAP:
                if last - a > best[0]: best = (last - a, a, last)
                a = None; gap = 0
    if a is not None and last - a > best[0]: best = (last - a, a, last)
    if best[0] < PURE * bw: continue          # a border/decoration row, not panel interior
    rows += 1
    a2, b2 = best[1], best[2]
    run = 0; prev = g[a2,y]
    for x in range(a2+1, b2+1):
        v = g[x,y]
        if abs(v-prev) > 25:
            edges += 1
            if run > longest: longest = run
            run = 0
        else:
            run += 1
        prev = v
if rows == 0:
    print("  %-6s PANEL FOUND BUT NO INTERIOR ROWS at %d,%d..%d,%d -- the locator is wrong here"
          % (label, x0, y0, x1, y1))
    sys.exit(1)
verdict = "LETTERS" if edges >= minedges else "BLOCKS-OR-BLANK"
print("  %-6s panel=%d,%d..%d,%d rows=%-4d edges=%-5d longest-flat-run=%-4d -> %s"
      % (label, x0, y0, x1, y1, rows, edges, longest, verdict))
sys.exit(0 if verdict == "LETTERS" else 1)
MEASPY
}

run_arm() {   # run_arm <screen> <keyseq> <frames> <armname> [extra env...]
  local screen="$1" keyseq="$2" frames="$3" arm="$4"; shift 4
  rm -f /tmp/maback.ppm
  ( cd "$RUNDIR" && timeout -k 5 -s KILL "$TMO" env SDL_VIDEODRIVER=dummy BOB_RUN_INIT=1 \
      MA_ENABLE_3D=1 BOB_DRIVE_C="$BOB_DRIVE_C" BOB_CLICKSEQ="$CLICKSEQ" \
      ${keyseq:+BOB_KEYSEQ="$keyseq"} ${MA_UIKEY:+MA_UISCR_KEY="$MA_UIKEY"} \
      MA_UISCR_SHOT="$frames" "$@" "$WMIG" \
  ) > "$OUT/${screen}_${arm}.log" 2>&1
  pkill -x "$(basename "$WMIG")" 2>/dev/null; sleep 1
  [ -s /tmp/maback.ppm ] || return 2
  cp /tmp/maback.ppm "$OUT/${screen}_${arm}.ppm"
  return 0
}

FAIL=0; RAN=0
echo "overlay text gate — $WMIG"
while IFS='|' read -r SCREEN KEYS FRAMES REGION MINEDGES UIKEY; do
  [ -z "${SCREEN:-}" ] && continue
  want "$@" || continue
  RAN=$((RAN+1))
  echo "$SCREEN:"
  export MA_UIKEY="${UIKEY:-}"
  if run_arm "$SCREEN" "$KEYS" "$FRAMES" fix; then
    measure "$OUT/${SCREEN}_fix.ppm" "$REGION" "$MINEDGES" fix || FAIL=1
  else
    echo "  fix    NO CAPTURE (see $OUT/${SCREEN}_fix.log)"; FAIL=1
  fi
  if [ "$ARMS" = all ]; then
    run_arm "$SCREEN" "$KEYS" "$FRAMES" bars MA_NO_ALPHATEXT=1 && measure "$OUT/${SCREEN}_bars.ppm" "$REGION" "$MINEDGES" bars
    run_arm "$SCREEN" "$KEYS" "$FRAMES" none MA_NO_GLYPHS=1    && measure "$OUT/${SCREEN}_none.ppm" "$REGION" "$MINEDGES" none
    echo "  (the control arms are EXPECTED to score BLOCKS-OR-BLANK — they are the proof that"
    echo "   'LETTERS' above is caused by the glyph path and not by something else in the frame)"
  fi
done <<EOF
$RECIPES
EOF

if [ "$#" -eq 0 ] || [ "${1:-}" = infoline ]; then
  echo "infoline:"
  RAN=$((RAN+1))
  rm -f /tmp/maback.ppm
  ( cd "$RUNDIR" && timeout -k 5 -s KILL "$TMO" env SDL_VIDEODRIVER=dummy BOB_RUN_INIT=1 \
      MA_ENABLE_3D=1 BOB_DRIVE_C="$BOB_DRIVE_C" BOB_CLICKSEQ="$CLICKSEQ" \
      MA_DUMP_BACK="$INFOLINE_BLT" MA_TRACE_FONT=1 MA_TRACE_PREFS=1 "$WMIG" \
  ) > "$OUT/infoline_fix.log" 2>&1
  pkill -x "$(basename "$WMIG")" 2>/dev/null; sleep 1
  log="$OUT/infoline_fix.log"
  if grep -aq "infoLineCount=[1-9].* -> drawing" "$log"; then
    echo "  fix    $(grep -a -m1 'infobar' "$log" | sed 's/^ *//')  -> DRAWING"
  else
    echo "  fix    $(grep -a -m1 'infobar' "$log" | sed 's/^ *//')  -> NOT DRAWN"; FAIL=1
  fi
  grep -a -m1 "loaded settings.mig" "$log" | sed 's/^/  /' || true
fi

echo "----------------------------------------"
if [ "$FAIL" -eq 0 ]; then echo "PASS: $RAN overlay-text screen(s) OK"; exit 0
else echo "FAIL: an overlay-text screen is blocks or blank (captures in $OUT)"; exit 1; fi
