#!/usr/bin/env bash
# port/overlay_text.sh — regression gate for the 3D OVERLAY TEXT layer (S102/S103).
#
# The overlay text layer (in-flight menus, the radio menu, the map window's text, the info line)
# was invisible for the port's whole life and then visible-but-unreadable, and neither state failed
# any gate: a screenshot cannot tell "no glyphs" from "glyphs drawn as solid blocks", and a
# whole-frame diff cannot either (two IDENTICAL flight runs differ by ~2700 px — S100).
#
# So this gate measures the SHAPE of the ink, not its presence:
#   letters  -> many short horizontal bright runs   (mean run a few px, max run < a character)
#   bars     -> few very long runs                  (the S101 symptom, reproducible with
#                                                    MA_NO_ALPHATEXT=1)
#   nothing  -> almost no ink                       (reproducible with MA_NO_GLYPHS=1)
# Both failure modes are therefore distinguishable from success by one measurement, in one run.
#
#   port/overlay_text.sh [screen ...]      (default: all)
#   ARMS=all port/overlay_text.sh          also runs the two control arms and prints them
#
# Screens:
#   menu   the in-flight command menu that opens on its own during a Hot Shot mission
# (radio [PO-7] and map [PO-6] join this table once S103 has MEASURED their regions and run
#  counts. A gate line whose thresholds were guessed rather than measured is worse than no line:
#  it either passes on nothing or fails on everything, and both teach you to ignore it.)
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WMIG="${WMIG:-$ROOT/build/wmig}"
BOB_DRIVE_C="${BOB_DRIVE_C:-$HOME/sgl/TUE/MigAlley/WP/drive_c}"
RUNDIR="$BOB_DRIVE_C/rowan/mig"
OUT="${OUT:-/tmp/ma_overlay_text}"
TMO="${TMO:-180}"
ARMS="${ARMS:-fix}"
CLICKSEQ="40,r1;95,r0"      # title -> Single Player -> Hot Shot (S63: rows, never pixels)

# screen | key taps (BOB_KEYSEQ, "pump,dik") | dump Blt | region x0,y0,x1,y1 | min bright runs
# DIK: r=0x13 (RADIOCOMMS), m=0x32 (GOTOMAPKEY) -- KEYMAPS.H
# Calibrated S102 on the three-arm A/B: letters 266 runs / max 5 · bars 105 / max 97 ·
# no-glyphs 61 / max 5. The floor sits between the background speckle (61) and the text (266).
RECIPES="
menu|:|700|225,40,460,100|150
"

mkdir -p "$OUT"
[ -x "$WMIG" ] || { echo "no binary at $WMIG" >&2; exit 2; }
want() { [ "$#" -eq 0 ] && return 0; for w in "$@"; do [ "$w" = "$SCREEN" ] && return 0; done; return 1; }

measure() {   # measure <ppm> <x0,y0,x1,y1> <minruns> <label>
  python3 - "$1" "$2" "$3" "$4" <<'PY'
import sys
from PIL import Image
im = Image.open(sys.argv[1]).convert('L')
x0,y0,x1,y1 = [int(v) for v in sys.argv[2].split(',')]
minruns = int(sys.argv[3])
px = im.crop((x0,y0,x1,y1)).load()
w,h = x1-x0, y1-y0
runs=[]; ink=0
for y in range(h):
    run=0
    for x in range(w):
        if px[x,y] > 200:
            run+=1; ink+=1
        elif run:
            runs.append(run); run=0
    if run: runs.append(run)
n=len(runs); mx=max(runs) if runs else 0; mean=(sum(runs)/n) if n else 0
if mx >= 20:      verdict = "BARS"          # a run longer than a character = a filled cell
elif n >= minruns: verdict = "LETTERS"
else:             verdict = "NO-TEXT"       # only background speckle
print("  %-6s ink=%-6d runs=%-5d mean=%.1f max=%-4d -> %s" % (sys.argv[4], ink, n, mean, mx, verdict))
sys.exit(0 if verdict=="LETTERS" else 1)
PY
}

run_arm() {   # run_arm <screen> <keyseq> <blt> <armname> [extra env...]
  local screen="$1" keyseq="$2" blt="$3" arm="$4"; shift 4
  rm -f /tmp/maback.ppm
  ( cd "$RUNDIR" && timeout -k 5 -s KILL "$TMO" env SDL_VIDEODRIVER=dummy BOB_RUN_INIT=1 \
      MA_ENABLE_3D=1 BOB_DRIVE_C="$BOB_DRIVE_C" BOB_CLICKSEQ="$CLICKSEQ" \
      ${keyseq:+BOB_KEYSEQ="$keyseq"} MA_DUMP_BACK="$blt" "$@" "$WMIG" \
  ) > "$OUT/${screen}_${arm}.log" 2>&1
  pkill -x "$(basename "$WMIG")" 2>/dev/null; sleep 1
  [ -s /tmp/maback.ppm ] || return 2
  cp /tmp/maback.ppm "$OUT/${screen}_${arm}.ppm"
  return 0
}

FAIL=0; RAN=0
echo "overlay text gate — $WMIG"
while IFS='|' read -r SCREEN KEYS BLT REGION MINRUNS; do
  [ -z "${SCREEN:-}" ] && continue
  want "$@" || continue
  [ "$KEYS" = ":" ] && KEYS=""
  RAN=$((RAN+1))
  echo "$SCREEN:"
  if run_arm "$SCREEN" "$KEYS" "$BLT" fix; then
    measure "$OUT/${SCREEN}_fix.ppm" "$REGION" "$MINRUNS" fix || FAIL=1
  else
    echo "  fix    NO CAPTURE (see $OUT/${SCREEN}_fix.log)"; FAIL=1
  fi
  if [ "$ARMS" = all ]; then
    run_arm "$SCREEN" "$KEYS" "$BLT" bars MA_NO_ALPHATEXT=1 && measure "$OUT/${SCREEN}_bars.ppm" "$REGION" "$MINRUNS" bars
    run_arm "$SCREEN" "$KEYS" "$BLT" none MA_NO_GLYPHS=1    && measure "$OUT/${SCREEN}_none.ppm" "$REGION" "$MINRUNS" none
    echo "  (control arms are EXPECTED to report BARS and NO-TEXT — they are the proof that"
    echo "   'LETTERS' above is caused by the glyph path and not by something else in the frame)"
  fi
done <<EOF
$RECIPES
EOF

echo "----------------------------------------"
if [ "$FAIL" -eq 0 ]; then echo "PASS: $RAN screen(s) render overlay text as letters"; exit 0
else echo "FAIL: an overlay-text screen is bars or blank (captures in $OUT)"; exit 1; fi
