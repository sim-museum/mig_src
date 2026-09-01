#!/bin/bash
# port/gates_all.sh â run the whole MA gate suite in one go and print one verdict.
#
# S187. Until now "re-run the gates" meant remembering which of the ~23 scripts in port/ are
# actually gates and running each by hand. That is exactly the condition under which a suite
# silently shrinks: a gate that nobody remembers is a gate that never runs, and it goes stale
# without ever going red. BoB has had `tools/bob_gates.sh` since S199 for the same reason.
#
# Contract, matching the individual gates:
#   * every gate exits 0 on PASS, non-zero on FAIL â this runner adds no judgement of its own
#   * gates DO NOT take gl-lock (nesting deadlocks, S159), so the runner takes it ONCE for all
#   * the binary's md5 is printed before and after: a suite run against a binary that changed
#     underneath it is not a result, and S186 is recent enough that that matters
#
# Usage:  port/gates_all.sh [gate ...]      (no args = all of them)
#         MA_GATES_NOLOCK=1                 skip gl-lock (already holding it)
set -u
SELF="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/$(basename "${BASH_SOURCE[0]}")"
cd "$(dirname "$SELF")/.." || exit 2
WMIG="${WMIG:-$PWD/build/wmig}"
[ -x "$WMIG" ] || { echo "no binary at $WMIG" >&2; exit 2; }

# The gate list, in cheapest-first order so a broken build fails fast.
# DELIBERATELY OMITTED: `stress_launch.sh` (20 full 3D launches, ~15 min) and `hw_gate.sh`. They
# are real gates and they are NOT run here â pass them by name when you want them. Saying so out
# loud rather than letting the suite quietly under-cover is the point (cf. BoB's "no silent caps").
ALL="parity_2d overlay_text panel_click maximized_nav help_click dialog_scroll map_filter map_drag
     map_icon_click authorize_mission damage_elements recon_photo add_flight attack_pattern
     flak_suppression route_drag route_drag_real ins_wave frag_review sysbox_exit oob_sweep
     real_mouse real_hover acmi_orientation texfail spacefix mp_connect mp_packet revpad_caller"
# S391 (PO-76): mp_packet is the TRANSPORT half of multiplayer, cross-ported from BoB's R7.1. It
# needs no display (pure sockets) so it never queues behind gl-lock, and it carries its own negative
# control: the `solo` arm must find ZERO sessions with no host, which is what makes a passing
# host/join arm mean a packet really crossed rather than the probe reporting success regardless.
# S381: revpad_caller was written in S338 and never listed here either -- the third gate in this
# file to be built and then left unrun. Worse than unrun: its verdict logic reported the S345 FIX
# as a failure ("mode set but the setup routine never ran", exit 1, while its own counter printed
# "mode set: 0"), so had anyone run it they would have been sent after a repaired defect. It now
# flies twice, ~2 min: once with no padlock target (the toggle must decline -- that decline IS the
# cure for the PO\'s twitch) and once with ENTER pressed first (the bogey view must then hold).
# S372: texfail and spacefix were built (S365, S371) and left OUT of this list, which makes them
# gates nobody runs -- they protect nothing until something invokes them, and the whole reason
# BoB's campaign gate stayed green for nine sprints while being unpassable is that no one ever
# ran it. Both fly a real sortie (~200-260s each) and both need the HARDWARE renderer, which
# they pin themselves via MA_TRY_HARDWARE, so they cost the suite roughly eight minutes. That is
# the price of the two PO-visible defects they cover: white textures (PO-82) and collapsed
# spaces in the in-flight radio text (PO-75).
# S327: real_mouse/real_hover ARE IN THE SUITE NOW. real_mouse existed since S215 and was never
# listed here, so the ONLY gate that drives a real pointer never ran: all 22 others inject below
# win_to_canvas, so the whole suite could stay green while no real click worked. It went red for
# five sprints after S325 without anyone seeing it. A gate outside the suite protects nothing.
# Both exit 2 (SKIP, not PASS) when a human is using the pointer or there is no display.
# NB collapse to ONE line. $ALL is written across three source lines for legibility, and the
# gl-lock re-entry below passes it inside a `bash -c` string: with the newlines intact, bash read
# lines 2 and 3 as separate COMMANDS and the suite silently ran only the first seven gates while
# still printing a confident "GATES: 5 passed, 2 FAILED" verdict. A suite that under-runs without
# saying so is worse than no suite. Unquoted expansion here is deliberate â it does the collapsing.
GATES="$(echo ${*:-$ALL})"

# S188: refuse to start, and refuse to CONTINUE, with a stray wmig about. A gate run leaves an
# ORPHAN when its `timeout` wrapper dies before the game does: the process is reparented to init,
# so nothing is left that can kill it, and it sits in the run directory holding it. S177 already
# lost a gate to exactly this (damage_elements failed for a whole sprint because a stray held the
# directory); S188 lost half an hour to one that silently BLOCKED the next arm from starting.
# Refuse rather than kill: a stray wmig may be the PO's own game, and killing that is worse than
# stopping. Exit 2, not 1 -- this is "could not run", not "the product is broken".
stray_check() {   # $1 = when
  local pids; pids=$(pgrep -x "$(basename "$WMIG")" 2>/dev/null | tr '\n' ' ')
  [ -z "$pids" ] && return 0
  echo "### STRAY $(basename "$WMIG") $1: pid(s) $pids"
  echo "###   Not killing it â it may be the PO's own game. Close it (or kill those pids) and rerun."
  return 1
}
stray_check "before the suite" || exit 2

md5_before=$(md5sum "$WMIG" | cut -d' ' -f1)
echo "### MA GATE SUITE â $(date '+%Y-%m-%d %H:%M') â binary md5=$md5_before"
echo

run_all() {
  local pass=0 fail=0 skip=0 failed="" skipped=""
  for g in $GATES; do
    local s="port/$g.sh"
    if [ ! -x "$s" ]; then
      echo "### $g â MISSING ($s not executable)"; fail=$((fail+1)); failed="$failed $g"; continue
    fi
    echo "### $g"
    local t0=$SECONDS
    "$s" 2>&1 | sed 's/^/  /'
    local rc=${PIPESTATUS[0]} dt=$((SECONDS-t0))
    if [ "$rc" -eq 0 ]; then
      echo "  -> PASS (${dt}s)"; pass=$((pass+1))
    elif [ "$rc" -eq 2 ]; then
      # rc=2 is INCONCLUSIVE, not pass. The real-pointer gates seize the mouse and refuse to fight
      # a human for it; a missing display or absent xdotool lands here too. Counted and named
      # separately, never folded into the pass total -- "12/12 clean" while two gates never ran is
      # the kind of confident under-report this suite has been burned by before.
      echo "  -> SKIP rc=2 (${dt}s) -- could not run, NOT a pass"; skip=$((skip+1)); skipped="$skipped $g"
    else
      echo "  -> FAIL rc=$rc (${dt}s)"; fail=$((fail+1)); failed="$failed $g"
    fi
    # An orphan left by THIS gate will wreck the next one, and the next one's failure will look
    # like a product defect. Say so at the boundary where it is still attributable.
    stray_check "left running by $g" || {
      echo "### ABORTING the suite: every later gate would run against a held run directory"
      fail=$((fail+1)); failed="$failed <aborted-after-$g>"; break
    }
    echo
  done
  echo "========================================"
  local md5_after; md5_after=$(md5sum "$WMIG" | cut -d' ' -f1)
  if [ "$md5_after" != "$md5_before" ]; then
    echo "### BINARY CHANGED MID-SUITE ($md5_before -> $md5_after) â THIS RESULT IS VOID"
    return 2
  fi
  echo "### binary unchanged (md5=$md5_after) â suite valid"
  local sk=""; [ "$skip" -gt 0 ] && sk=", $skip SKIPPED (did not run) â$skipped"
  if [ "$fail" -eq 0 ]; then
    echo "### GATES: $pass/$pass clean$sk"
    return 0
  fi
  echo "### GATES: $pass passed, $fail FAILED â$failed$sk"
  return 1
}

# S352: REFUSE TO NEST. S159 recorded that the gates must never take gl-lock themselves because
# nesting deadlocks -- but nothing stopped the SUITE being launched inside a lock, and
# `gl-lock bash port/gates_all.sh` does exactly that: the outer lock is held while the re-entry
# below waits for the same lock, forever, with an empty log and a live process. It looks like a
# hung gate rather than a harness mistake, and it cost a full sprint slot to recognise.
# gl-lock exports no marker, so ask it who holds the display and compare against our own process
# tree. If an ancestor holds it, we are already inside and must run directly.
if [ -z "${MA_GATES_NOLOCK:-}" ] && command -v gl-lock >/dev/null 2>&1; then
  _holder=$(gl-lock --status 2>/dev/null | sed -n 's/.*pid=\([0-9]*\).*/\1/p' | head -1)
  if [ -n "$_holder" ]; then
    _p=$$
    while [ -n "$_p" ] && [ "$_p" != "1" ] && [ "$_p" != "0" ]; do
      if [ "$_p" = "$_holder" ]; then
        echo "### already inside gl-lock (held by pid $_holder, an ancestor) -- running without re-acquiring"
        MA_GATES_NOLOCK=1
        break
      fi
      _p=$(ps -o ppid= -p "$_p" 2>/dev/null | tr -d ' ')
    done
  fi
fi
if [ -n "${MA_GATES_NOLOCK:-}" ]; then
  run_all; rc=$?
else
  # gl-lock serialises the display; the gates themselves must never take it (S159).
  gl-lock bash -c "MA_GATES_NOLOCK=1 '$SELF' $GATES"; rc=$?
fi
echo "### DONE (rc=$rc)"
exit $rc
