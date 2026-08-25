#!/usr/bin/env bash
# port/replay_autosave.sh -- never lose a good recording again.
#
# WHY THIS EXISTS
# ---------------
# The PO flew a One-on-One, the 2-aircraft replay played correctly end to end (the PO-61 goal), and
# the recording was then GONE: replay.dat is truncated at the start of the next flight and on exit
# (correctly -- that is what S205's real SetEndOfFile restored), so a successful recording survives
# only until the next thing happens. I captured the log evidence and not the file. That is the THIRD
# .cam data loss in this project (PO-65 accounts for the first two).
#
# WHAT IT DOES
#   Polls replay.dat. When it is non-zero AND its size has been STABLE for two consecutive polls
#   (i.e. StopRecord has finished back-patching, not mid-write), and its content differs from the
#   last snapshot, it copies it to Videos/auto/<HHMMSS>-<frames>f-<size>b.cam.
#
# WHY A WATCHER AND NOT A GAME-CODE HOOK
#   A snapshot is pure observation -- it cannot perturb recording, alignment or timing, and it needs
#   no edit to Replay.cpp. Given how much of this sprint block was spent discovering that my own
#   instruments changed what they measured (S231: a trace that dereferenced a wild pointer; S233: a
#   capture tool that reported black for a rendering app), an out-of-process copier is the right
#   shape for this job.
#
#   It is READ-ONLY with respect to the game's own files: it only ever reads replay.dat and writes
#   into Videos/auto/, which the game does not enumerate.
set -u
V="${V:-$HOME/sgl/TUE/MigAlley/WP/drive_c/rowan/mig/Videos}"
OUT="$V/auto"
mkdir -p "$OUT"
last_sum=""; prev_size=-1
echo "[autosave] watching $V/replay.dat -> $OUT/"
while true; do
  if [ -s "$V/replay.dat" ]; then
    sz=$(stat -c%s "$V/replay.dat" 2>/dev/null || echo 0)
    if [ "$sz" = "$prev_size" ] && [ "$sz" -gt 1000 ]; then
      sum=$(md5sum "$V/replay.dat" 2>/dev/null | cut -d' ' -f1)
      if [ -n "$sum" ] && [ "$sum" != "$last_sum" ]; then
        # frame count sits in the block header; label with size, which is always knowable
        name="$OUT/$(date +%H%M%S)-${sz}b.cam"
        cp "$V/replay.dat" "$name" && echo "[autosave] kept $name"
        last_sum="$sum"
      fi
    fi
    prev_size="$sz"
  else
    prev_size=-1
  fi
  sleep 1
done
