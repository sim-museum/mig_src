#!/usr/bin/env bash
# port/replay_record.sh — S205 (PO-61/PO-64): a flown mission produces a PLAYABLE recording.
#
# WHY THIS EXISTS
# ---------------
# The PO: "Replay/View initial 3D view is correct, but VCR controls don't work - 3D view shows no
# motion." The VCR transport was innocent. Measured interleaving (S204): play un-pauses playback,
# the block read fails, and playback re-pauses itself -- so "the control does nothing" and "the
# transport ran" were both true.
#
# ROOT CAUSE (S205): `SetEndOfFile` was a compat stub -- `{ (void)h; return TRUE; }` -- that
# reported success and did nothing. `Replay::OpenRecordLog` opens replay.dat with OPEN_ALWAYS
# ("add to any file that is there") and truncates it via SetEndOfFile when ResetFileFlag is set;
# the game's own comment there reads "instead of deleting file, just truncate to zero". With the
# stub, the file was NEVER emptied and every flight ever flown was appended to it. Measured across
# the PO's sessions: 2,427,259 -> 2,459,480 -> 2,491,867 -> 2,551,847 bytes.
#
# Playback starts at the FIRST block in that file -- a stale one from an old session whose frame
# count was never back-patched. It read numframes=0, treated the block as empty, consumed no
# frames, and then looked for the next block header where the frame data still sat (the S204
# MAGIC MISMATCH at 19915). No motion, however good the newest recording was.
#
# WHAT IT ASSERTS, on a real flight (this needs real GL -- SDL_CreateWindow fails under the dummy
# driver, so there is no headless variant; run it under gl-lock):
#   1. the flight actually recorded      -- StopRecord reports Record=1 and a non-zero frame count
#   2. the back-patch ran                -- StoreRealFrameCounts fired with that count
#   3. the file was TRUNCATED            -- size is one flight's worth, not the accumulated history
#   4. the file is WELL-FORMED           -- the block magic is at SuperHeaderSize and the frame
#                                           counts on disk equal what StopRecord back-patched
#   5. the arithmetic closes             -- superheader + blockheader + numframes*sizeof(REPLAYPACKET)
#                                           == the file size exactly. This is the property that
#                                           actually makes it playable, and it is checkable without
#                                           driving the replay UI.
#
# ⚠️ NEGATIVE CONTROL BUILT IN: CONTROL=1 sets MA_NO_TRUNCATE=1, restoring the stub. The file must
# then GROW and assertion 3/5 must fail. A gate nobody has watched fail is indistinguishable from
# one that cannot fail (§8-BoB206) -- and this gate's whole subject is a stub that returned success.
#
# ⚠️ THIS GATE WRITES THE PLAYER'S replay.dat (a flight necessarily re-records it). It stashes and
# restores it, per the S81 rule that a gate must never eat the player's data.
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WMIG="${WMIG:-$ROOT/build/wmig}"
BOB_DRIVE_C="${BOB_DRIVE_C:-$HOME/sgl/TUE/MigAlley/WP/drive_c}"
RUNDIR="$BOB_DRIVE_C/rowan/mig"
DAT="$RUNDIR/Videos/replay.dat"
OUT="${OUT:-/tmp/ma_replay_record}"
TMO="${TMO:-200}"
FRAMES="${FRAMES:-400}"
CONTROL="${CONTROL:-0}"
mkdir -p "$OUT"
[ -x "$WMIG" ] || { echo "no binary at $WMIG" >&2; exit 2; }
if pgrep -x wmig >/dev/null 2>&1; then
  echo "  REFUSING TO RUN: wmig is already running (pid $(pgrep -x wmig | tr '\n' ' ')) -- S177."
  exit 2
fi

NOTRUNC=""
[ "$CONTROL" = "1" ] && NOTRUNC="MA_NO_TRUNCATE=1"
echo "replay record — fly a quick mission, then check the recording is playable$([ "$CONTROL" = 1 ] && echo '   [NEGATIVE CONTROL: MA_NO_TRUNCATE=1, must go RED]')"

[ -f "$DAT" ] && cp -a "$DAT" "$OUT/replay.dat.player"
before=$( [ -f "$DAT" ] && stat -c %s "$DAT" || echo 0 )
echo "  replay.dat before: $before bytes"

log="$OUT/record.log"
( cd "$RUNDIR" && timeout -k 5 -s KILL "$TMO" env \
    BOB_RUN_INIT=1 BOB_DRIVE_C="$BOB_DRIVE_C" MA_ENABLE_3D=1 \
    MA_TRACE_REPLAY=1 MA_TRACE_3D=1 $NOTRUNC \
    BOB_CLICKSEQ='40,r1;60,r1;110,#2063:2' BOB_AUTOEXIT="$FRAMES" \
    "$WMIG" ) >"$log" 2>&1
for p in $(pgrep -x wmig); do kill -9 "$p" 2>/dev/null; done

fail=0
say() { printf '  %-44s %s\n' "$1" "$2"; }
after=$( [ -f "$DAT" ] && stat -c %s "$DAT" || echo 0 )

# 1. did the flight record at all?
n=$(grep -a "StopRecord: Record=1 replayframecount=" "$log" | tail -1 | sed -n 's/.*replayframecount=\([0-9]*\).*/\1/p')
if [ -n "${n:-}" ] && [ "${n:-0}" -gt 0 ]; then say "flight recorded (StopRecord)" "yes — $n frames"
else say "flight recorded (StopRecord)" "NO — the run never flew, so nothing below means anything"; fail=1; fi

# 2. did the back-patch run?
if grep -aq "StoreRealFrameCounts(num=${n:-x} " "$log"; then say "frame count back-patched" "yes"
else say "frame count back-patched" "NO"; fail=1; fi

# 3+4+5. The property that actually makes a recording playable: the file holds EXACTLY ONE block,
#        the FIRST block carries the count StopRecord back-patched, and its header+frames end at
#        EOF. Size is NOT the invariant -- a longer flight legitimately makes a bigger file.
#
#        ⚠️ The first version of this gate checked "is the file small" and "does SOME block close at
#        EOF", and its own negative control PASSED: with the stub restored the file grew
#        20641 -> 41282 and the appended SECOND block closed at EOF just fine. Playback reads the
#        FIRST block, so that check tested something playback never does. The control caught it
#        before this gate was ever trusted -- which is the entire reason for having one.
echo "  replay.dat after:  $after bytes"
python3 - "$DAT" "${n:-0}" <<'PYCHK'
import sys, struct
path, n = sys.argv[1], int(sys.argv[2])
try: b = open(path,'rb').read()
except OSError as e:
    print("  %-44s %s" % ("file readable", "NO (%s)" % e)); sys.exit(1)
if n <= 0:
    print("  %-44s %s" % ("frame count known", "NO")); sys.exit(1)
PACKET = 11                                  # sizeof(REPLAYPACKET), packed
MAGIC  = b'\x78\x56\x34\x12'
ok = True

offs, o = [], b.find(MAGIC)
while o != -1:
    offs.append(o); o = b.find(MAGIC, o+1)
print("  %-44s %s" % ("exactly one block in the file",
      ("yes - 1 block header at %d" % offs[0]) if len(offs) == 1
      else ("NO - %d block headers at %s (playback reads the FIRST)" % (len(offs), offs[:4]))))
ok &= (len(offs) == 1)
if not offs: sys.exit(1)

first = offs[0]
want, found = (n, 0, n-1), None
for t in range(first, min(first+4096, len(b)-6)):
    if struct.unpack_from('<HHH', b, t) == want:
        found = t; break
if found is None:
    got = struct.unpack_from('<HHH', b, first+953) if first+959 <= len(b) else None
    print("  %-44s %s" % ("FIRST block carries this flight's count",
          "NO - (%d,0,%d) not in the first block%s" % (n, n-1, (" (it holds %s)" % (got,)) if got else "")))
    ok = False
else:
    print("  %-44s %s" % ("FIRST block carries this flight's count", "yes - (%d,0,%d) at %d" % (n, n-1, found)))
    total = found + 6 + 4 + n*PACKET
    close = (total == len(b))
    print("  %-44s %s" % ("first block's header+frames == EOF",
          ("yes - %d == %d" % (total, len(b))) if close else ("NO - %d vs %d, so %d stale byte(s) precede or follow" % (total, len(b), abs(len(b)-total)))))
    ok &= close
sys.exit(0 if ok else 1)
PYCHK
[ $? -eq 0 ] || fail=1

# restore the player's recording
if [ -f "$OUT/replay.dat.player" ]; then cp -a "$OUT/replay.dat.player" "$DAT"; fi

echo "----------------------------------------"
if [ "$CONTROL" = "1" ]; then
  if [ "$fail" -ne 0 ]; then echo "CONTROL OK: with the stub restored the recording is unplayable again"; exit 0
  else echo "CONTROL BROKEN: passed with the fix reverted — this gate is not testing S205"; exit 1; fi
fi
if [ "$fail" -eq 0 ]; then echo "PASS: the flight recorded $n frames into a well-formed, self-consistent replay.dat"
else echo "FAIL"; exit 1; fi
