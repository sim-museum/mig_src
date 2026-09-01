#!/usr/bin/env bash
# GATE: PO-77 -- ETO_CLIPPED actually clips, an EMPTY rect does not, and the clip restores.
#
# Headless on purpose. S399 wired ETO_CLIPPED into 40 call sites across 10 live files; S400 found a
# degenerate rect would blank a control's text; S401 found the wiring was INERT for real text
# because the TTF glyph writer never tested the clip. None of that was visible without pixels --
# but the clip is pure software, so this renders into a bitmap and counts pixels either side of the
# clip edge. Reading the code gave the wrong answer twice before this probe was written.
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OBJ="$ROOT/build/CMakeFiles/ma_obj.dir/SRC/compat/ma_gdi.cpp.o"
PROBE=/tmp/ma_clip_probe
[ -f "$OBJ" ] || { echo "  no ma_gdi object -- build first"; exit 2; }
if [ ! -x "$PROBE" ] || [ "$ROOT/port/clip_probe.cpp" -nt "$PROBE" ] || [ "$OBJ" -nt "$PROBE" ]; then
  g++ -m32 -fno-pie -no-pie -w -DMA_LINUX -I "$ROOT/SRC/compat" -I "$ROOT/SRC/H" \
      "$ROOT/port/clip_probe.cpp" "$OBJ" -o "$PROBE" || { echo "  FAIL: probe did not build"; exit 2; }
fi
echo "PO-77 clip gate -- ETO_CLIPPED, empty rect, and restore"
"$PROBE" 2>&1 | grep -av gdifont
exit ${PIPESTATUS[0]}
