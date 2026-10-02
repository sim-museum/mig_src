#!/bin/bash
# FUNC-SWEEP-MA: tap every keyboard binding in controls.cfg once during one Hot Shot flight
# (port/keysweep_gen.py; flight-ending keys held back) and report crash / the last tap that fired.
# Runs on the disposable ~/ma-crawl copy. Needs a real GL display (use gl-lock).
set -u
ROOT=$(cd "$(dirname "$0")/.." && pwd)
GD="${GD:-$HOME/ma-crawl/drive_c/rowan/mig}"; OUT="${OUT:-$HOME/ma-gates/keysweep}"; mkdir -p "$OUT"
python3 "$ROOT/port/keysweep_gen.py" "$GD/controls.cfg" "${START_F:-900}" "${STEP_F:-40}" "$OUT/keys.map" > "$OUT/keys.txt"
LASTF=$(tail -n 1 "$OUT/keys.map" | cut -f1); SECS="${SECS:-$(( LASTF / 20 + 90 ))}"
echo "taps=$(wc -l < "$OUT/keys.map") last@frame${LASTF} timeout=${SECS}s"
( cd "$GD" && MA_TEST_INVULNERABLE="${INVULN:-1}" BOB_RUN_INIT=1 BOB_DRIVE_C="${GD%/rowan/mig}" MA_ENABLE_3D=1 MA_TRACE_3D=1 MA_TRACE_KEY=1 \
      BOB_CLICKSEQ="40,r1;95,r0" BOB_KEYSEQ_FRAMES=1 BOB_KEYSEQ="$(cat "$OUT/keys.txt")" \
      timeout -k 5 -s INT "$SECS" "$ROOT/build/wmig" > "$OUT/run.log" 2>&1 ); rc=$?
crash=$(grep -acE '=== CRASH|Segmentation fault|SIGSEGV|Aborted' "$OUT/run.log")
taps=$(grep -ac '\[keyseq\] tap' "$OUT/run.log"); closes=$(grep -ac 'flight close' "$OUT/run.log")
echo "rc=$rc crash=$crash taps-fired=$taps flight-closes=$closes"; grep -a '\[keyseq\] tap' "$OUT/run.log" | tail -n 1
