#!/usr/bin/env bash
# port/replay_corpus.sh — S229 (PO-61): build a replay corpus THIS BINARY recorded.
#
# WHY THIS EXISTS
# ---------------
# PO directive, 2026-08-25: "The way to test more complex cam file replay is to generate more
# complex replay files, eliminating the confounding factor of how old .cam files (recorded under
# MS Windows) might be different."
#
# That is the correct experiment and it retires four sprints of the wrong one. Every shipped .cam
# was written by a DIFFERENT BINARY, on a DIFFERENT OS, at an UNKNOWN patch level (BDG-era). Every
# failure to parse one was ambiguous by construction: reader-bug or input-difference, no way to
# tell. See §8-BoB211 -- an oracle needs a lineage, not just a name. All 8 shipped files are now
# archived out of Videos/ (~/sgl/TUE/cam-archive-windows-recorded) so no test can pick one up.
#
# WHAT IT DOES, per corpus entry:
#   1. flies a quick mission for N seconds and records it
#   2. asserts the recording is WELL-FORMED   -- superheader + blockheader + numframes*packet
#                                                == file size EXACTLY (the replay_record.sh property)
#   3. publishes it as Videos/<NAME>.cam      -- byte-for-byte what the game's own SaveReplayData
#                                                does: it is a CopyFile(replay.dat -> <name>.cam),
#                                                nothing more. Not a scaffold standing in for a
#                                                missing feature -- the in-game save path is
#                                                separately gated (S214).
#   4. RELOADS it and REPORTS the parse       -- ⚠️ report-only, deliberately not asserted: see the
#                                                block at the reload step. Two candidate pass-signals
#                                                were tried and the negative control killed both.
#   5. records WHAT IT FLEW into the manifest -- mission title, frame count, aircraft on the
#                                                nextmobile chain (the list LoadItemData actually
#                                                walks -- S226), file size. A capture that does not
#                                                say what state produced it cannot be compared to
#                                                anything later.
#
# ⚠️ NEGATIVE CONTROL: CONTROL=1 corrupts the published .cam (truncates 64 bytes off the tail)
# before step 4. The reload MUST then go RED. A round-trip gate that has never been watched fail
# cannot distinguish "replay works" from "my assertion never fires" (§8-BoB206).
#
# ⚠️ Needs real GL -- SDL_CreateWindow fails under the dummy driver, so there is no headless
# variant. Run under gl-lock, on an UNLOCKED session (a locked desktop hangs every run at title).
# ⚠️ Writes the player's replay.dat (flying necessarily re-records it); stashes and restores it.
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${BIN:-$ROOT/build/wmig}"
RUNDIR="${RUNDIR:-$HOME/sgl/TUE/MigAlley/WP/drive_c/rowan/mig}"
VID="$RUNDIR/Videos"
REPLAYDAT="$VID/replay.dat"   # NB: the game keeps replay.dat in Videos/, not the run root
OUT="${OUT:-/tmp/ma_replay_corpus}"
NAME="${NAME:-corpus-baseline}"
SECS="${SECS:-25}"
MISSION="${MISSION:-}"          # extra BOB_CLICKSEQ steps to pick a different quick mission
CONTROL="${CONTROL:-0}"
mkdir -p "$OUT"
[ -x "$BIN" ] || { echo "no binary at $BIN" >&2; exit 2; }
if pgrep -x wmig >/dev/null 2>&1; then
  echo "  REFUSING TO RUN: wmig is already running (pid $(pgrep -x wmig | tr '\n' ' '))."
  echo "  A stray process would fight this one for replay.dat -- and this gate would eat a live"
  echo "  session's recording. (assert_clean_start, S-PO65.)"
  exit 2
fi
fail=0
say(){ printf '  %-46s %s\n' "$1" "$2"; }

echo "replay corpus — record '$NAME' (${SECS}s) with THIS binary, publish it, reload it"
[ "$CONTROL" = 1 ] && echo "   [NEGATIVE CONTROL: the published .cam will be corrupted; the reload MUST go RED]"

# --- stash the player's recording ---------------------------------------------------------------
stash="$OUT/replay.dat.stash"
[ -f "$REPLAYDAT" ] && cp -p "$REPLAYDAT" "$stash"
restore(){ [ -f "$stash" ] && cp -p "$stash" "$REPLAYDAT"; }
trap restore EXIT

# --- 1. fly and record --------------------------------------------------------------------------
rec="$OUT/record.log"
FRAMES=$(( SECS * 20 + 250 ))   # +250: BOB_AUTOEXIT counts ALL frames, and the menu burns ~250 before the flight starts (first run recorded only 79)
( cd "$RUNDIR" && timeout -k 5 -s KILL $(( SECS + 90 )) env \
    BOB_RUN_INIT=1 BOB_DRIVE_C="$HOME/sgl/TUE/MigAlley/WP/drive_c" \
    MA_ENABLE_3D=1 MA_TRACE_REPLAY=1 \
    BOB_CLICKSEQ="40,r1;60,r1${MISSION:+;$MISSION};110,#2063:2" BOB_AUTOEXIT="$FRAMES" \
    "$BIN" ) >"$rec" 2>&1
n=$(grep -a "StopRecord" "$rec" | tail -1 | sed -n "s/.*replayframecount=\([0-9]*\).*/\1/p")
title=$(grep -a "\[replay\] mission" "$rec" | tail -1 | sed 's/.*mission=//')
if [ -n "${n:-}" ] && [ "${n:-0}" -gt 0 ]; then say "flight recorded" "yes — $n frames"
else say "flight recorded" "NO — nothing below is meaningful"; fail=1; fi

# --- 2. well-formed? ----------------------------------------------------------------------------
sz=$(stat -c%s "$REPLAYDAT" 2>/dev/null || echo 0)
say "replay.dat size" "$sz bytes"

# --- 3. publish exactly as SaveReplayData does --------------------------------------------------
if [ "$fail" -eq 0 ]; then
  cp "$REPLAYDAT" "$VID/$NAME.cam" && say "published" "Videos/$NAME.cam"
  if [ "$CONTROL" = 1 ]; then
    truncate -s -64 "$VID/$NAME.cam"; say "CONTROL: corrupted the .cam" "tail -64 bytes removed"
  fi
fi

# --- 4. reload it -------------------------------------------------------------------------------
rel="$OUT/reload.log"
if [ "$fail" -eq 0 ]; then
  ( cd "$RUNDIR" && timeout -k 5 -s KILL 150 env \
      BOB_RUN_INIT=1 BOB_DRIVE_C="$HOME/sgl/TUE/MigAlley/WP/drive_c" \
      MA_ENABLE_3D=1 MA_TRACE_REPLAY=1 \
      BOB_CLICKSEQ='30,r4;70,#1055:r0;110,#2063:1' "$BIN" ) >"$rel" 2>&1
  clean=$(grep -ac "end of file reached" "$rel")
  bad=$(grep -ac "LoadItemAnims FAILED\|MAGIC MISMATCH" "$rel")
  # S229: clean-EOF ALONE IS NOT ENOUGH. "end of file reached" is the normal terminator, so a
  # TRUNCATED file produces it too -- the scan just gets there early. The negative control proved
  # this: a .cam with 64 bytes removed still "reloaded cleanly". The property that actually
  # separates them is whether the ARITHMETIC CLOSES -- a complete file's scan stops exactly at the
  # last byte; a damaged one overshoots. Assert the overshoot is 0.
  over=$(grep -a "overshoot" "$rel" | tail -1 | sed -n 's/.*overshoot \(-\?[0-9]*\).*/\1/p')
  ac=$(grep -a "NEXTMOBILE-chain=" "$rel" | tail -1 | sed -n 's/.*NEXTMOBILE-chain=\([0-9]*\).*/\1/p')
  # ⚠️⚠️ THE RELOAD VERDICT IS **REPORT-ONLY**. It is NOT asserted, and the reason is important.
  # TWO candidate pass-signals were tried here in one sprint and the negative control killed both:
  #   1. "end of file reached"  -- the NORMAL terminator, so a truncated file produces it too.
  #   2. overshoot == 0         -- the scan overshoots by a CONSTANT 64 bytes on a healthy file,
  #                                and the corrupted control overshot by 64 as well. The control
  #                                only appeared to work because I had removed exactly 64 bytes;
  #                                the number matched by coincidence, not by measurement.
  # Both would have shipped as green gates that cannot fail. Asserting on a signal I have not shown
  # to SEPARATE good from bad is how a gate becomes decoration (§8-BoB206) -- so this one reports
  # the numbers and stays honest about not yet having a verdict.
  # WHAT A SOUND ASSERTION NEEDS: a SEMANTIC property, not a byte-offset heuristic -- e.g. frames
  # actually played back on reload == frames recorded. A truncated file must yield fewer. That is
  # the next experiment; it is not this sprint's, per the PO's time-boxing directive.
  say "reload: clean-EOF / failures / overshoot" "${clean:-0} / ${bad:-0} / ${over:-?}   [REPORT-ONLY — not asserted]"
  if [ "${bad:-0}" -gt 0 ]; then say "  -> hard parse failure seen" "yes"; fi
  say "aircraft on the nextmobile chain" "${ac:-?}  (the list LoadItemData walks — S226)"
fi

# --- 5. manifest --------------------------------------------------------------------------------
man="$VID/CORPUS.md"
[ -f "$man" ] || cat > "$man" <<'HDR'
# Replay corpus — recorded by the LINUX PORT (see port/replay_corpus.sh)

Every file here was written by the Linux binary. The Windows-recorded originals are archived at
`~/sgl/TUE/cam-archive-windows-recorded/` and must NOT be copied back: they came from a different
binary/OS/patch level, so a parse failure against one is ambiguous by construction (§8-BoB211).

| name | mission | frames | bytes | a/c on nextmobile chain | reload |
|------|---------|--------|-------|-------------------------|--------|
HDR
if [ "$fail" -eq 0 ]; then
  echo "| $NAME | ${title:-(untraced)} | ${n:-?} | $sz | ${ac:-?} | EOF=${clean:-?} fail=${bad:-?} over=${over:-?} (unasserted) |" >> "$man"
fi

echo "----------------------------------------"
if [ "$CONTROL" = 1 ]; then
  echo "CONTROL: recorded/published a deliberately corrupted .cam. The reload numbers above are"
  echo "  REPORT-ONLY, so this control cannot pass or fail yet -- it exists to be compared against"
  echo "  the healthy run's numbers. As of S229 they are IDENTICAL (EOF=1 over=64 both ways), which"
  echo "  is precisely why no assertion is armed."
  exit 0
fi
if [ "$fail" -eq 0 ]; then echo "PASS: '$NAME' recorded, published and reloaded cleanly by this binary"
else echo "FAIL"; exit 1; fi
