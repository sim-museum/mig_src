#!/bin/bash
# GATE: the waypoint-map NULL crash (PO 2026-09-03, MiG Alley AppImage on a fresh machine).
#
#   MapScr::SelectFromWaypointMap, case SEL_1 ("set next waypoint"), dereferenced
#   OverLay.curr_waypoint without checking it. NULL is a legitimate state -- the map-open code
#   (OVERLAY.CPP:4843) sets it whenever the controlled aircraft has no waypoint -- so choosing
#   that option crashed the RENDER thread with SIGSEGV. Latent in the original game; the port
#   records replays, so `_Replay.Record` keeps the offending branch live in ordinary
#   single-player, which is why the PO hit it.
#
# The gate carries its own NEGATIVE CONTROL, because a gate that cannot fail proves nothing:
#   arm A: MA_NO_WPNULLFIX=1  -> MUST crash (SIGSEGV). If it does not, the gate is not
#                                reaching the code path and a PASS on arm B would be worthless.
#   arm B: default            -> MUST survive.
#
# Headless (SDL dummy). MA_TEST_WPNULL forces the NULL that the harness mission does not
# naturally produce; MA_UISCR_KEYS drives M -> option 2 (waypointMapScr) -> option 1 (SEL_1).
set -u
SELF="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/$(basename "${BASH_SOURCE[0]}")"
cd "$(dirname "$SELF")/.." || exit 2
WMIG="${WMIG:-$PWD/build/wmig}"
BOB_DRIVE_C="${BOB_DRIVE_C:-/home/admin/sgl/TUE/MigAlley/WP/drive_c}"
RUNDIR="$BOB_DRIVE_C/rowan/mig"
TMO="${TMO:-200}"
OUT="${OUT:-/tmp/ma_wpnull_gate}"; mkdir -p "$OUT"
[ -x "$WMIG" ] || { echo "no binary at $WMIG" >&2; exit 2; }
[ -d "$RUNDIR" ] || { echo "no game dir at $RUNDIR" >&2; exit 2; }

run_arm() {   # run_arm <name> [extra env...]
  local name="$1"; shift
  ( cd "$RUNDIR" && timeout -k 5 -s KILL "$TMO" env SDL_VIDEODRIVER=dummy BOB_RUN_INIT=1 \
      MA_ENABLE_3D=1 BOB_DRIVE_C="$BOB_DRIVE_C" BOB_CLICKSEQ="40,r1;95,r0" \
      BOB_KEYSEQ="500,0x32" MA_UISCR_KEYS="0x03,0x02" MA_UISCR_KEY_FRAMES=6 \
      MA_TEST_WPNULL=1 MA_TRACE_WPSEL=1 "$@" "$WMIG" ) > "$OUT/$name.log" 2>&1
  local rc=$?
  pkill -x "$(basename "$WMIG")" 2>/dev/null; sleep 1
  return $rc
}

echo "waypoint-null crash gate — $WMIG"
run_arm guard_off MA_NO_WPNULLFIX=1; A=$?
reached_a=$(grep -ac "\[wpsel\] SEL_1" "$OUT/guard_off.log")
run_arm guard_on; B=$?
reached_b=$(grep -ac "\[wpsel\] SEL_1" "$OUT/guard_on.log")

echo "  arm A (guard OFF, negative control)  exit=$A  reached-SEL_1=$reached_a  expect SIGSEGV(139)"
echo "  arm B (guard ON,  the fix)           exit=$B  reached-SEL_1=$reached_b  expect survive"

FAIL=0
[ "$reached_a" -ge 1 ] || { echo "  FAIL: arm A never reached SEL_1 — the driver is not hitting the path"; FAIL=1; }
[ "$reached_b" -ge 1 ] || { echo "  FAIL: arm B never reached SEL_1 — the driver is not hitting the path"; FAIL=1; }
[ "$A" -eq 139 ]       || { echo "  FAIL: arm A did NOT crash — the negative control is dead, so arm B proves nothing"; FAIL=1; }
[ "$B" -eq 139 ]       && { echo "  FAIL: arm B CRASHED — the null guard has regressed"; FAIL=1; }
[ $FAIL -eq 0 ] && { echo "  wpnull: PASS"; exit 0; }
echo "  wpnull: FAIL"; exit 1
