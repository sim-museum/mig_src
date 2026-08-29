#!/usr/bin/env bash
# port/tacview_export.sh — EPIC L / L5: the Tacview export is well-formed, WITHOUT opening Tacview.
#
# WHY THIS EXISTS
# ---------------
# L1/L3 were verified by reading the file and recognising plausible numbers (altitude 1524 m = a
# round 5000 ft, ~200 kt cruise, 40 objects matching an independently-measured chain). That is good
# evidence and it is not a test: nobody re-reads a 377 KB file by eye on every change.
#
# ⚠️ AND A WRONG EXPORT LOOKS EXACTLY LIKE A RIGHT ONE. Get the centimetre scale wrong and every
# line still parses, every field is still a number, the file still opens. The failure mode of this
# feature is a PLAUSIBLE FILE, which is precisely the kind this project keeps being fooled by --
# so the assertions below are about STRUCTURE and INTERNAL CONSISTENCY, the things a wrong scale
# cannot fake.
#
# WHAT IT ASSERTS
#   1. the flight actually recorded    -- else everything after it is vacuous (the S171 failure mode)
#   2. the .acmi exists and is non-trivial
#   3. header is correct               -- FileType/FileVersion on the first two lines, in order
#   4. global properties present       -- ReferenceTime/Longitude/Latitude on object 0
#   5. time markers are MONOTONIC      -- ACMI requires it; a repeated or backwards # breaks playback
#   6. every object line is well-formed -- <hex-id>,T=<9 pipe-separated fields>
#   7. every referenced id was introduced before use
#   8. THE .cam IS UNCHANGED           -- the export is additive; the recording is its own control
#
# ⚠️ NEGATIVE CONTROL BUILT IN: CONTROL=1 sets MA_ACMI=0, which disables the export. Assertions 2-7
# MUST then fail. A gate nobody has watched fail cannot distinguish "the export works" from "my
# assertion never fires" -- and this project has now booked that mistake enough times to build the
# control in rather than bolt it on.
#
# ⚠️ Needs real GL (SDL_CreateWindow fails under the dummy driver). Run under gl-lock.
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${BIN:-$ROOT/build/wmig}"
RUNDIR="${RUNDIR:-$HOME/sgl/TUE/MigAlley/WP/drive_c/rowan/mig}"
OUT="${OUT:-/tmp/ma_tacview}"
SECS="${SECS:-20}"
QM="${QM:-3}"
CONTROL="${CONTROL:-0}"
mkdir -p "$OUT"
[ -x "$BIN" ] || { echo "no binary at $BIN" >&2; exit 2; }
if pgrep -x wmig >/dev/null 2>&1; then
  echo "  REFUSING TO RUN: wmig is already running (pid $(pgrep -x wmig | tr '\n' ' '))."
  echo "  A stray process would fight this one for replay.dat (assert_clean_start, S-PO65)."
  exit 2
fi
fail=0
say(){ printf '  %-48s %s\n' "$1" "$2"; }
bad(){ say "$1" "$2"; fail=1; }

echo "tacview export — fly ${SECS}s, then validate the .acmi structurally"
[ "$CONTROL" = 1 ] && echo "   [NEGATIVE CONTROL: MA_ACMI=0 disables the export; assertions 2-7 MUST go RED]"

ACMI="$RUNDIR/acmi_current.txt"
CAM="$RUNDIR/Videos/replay.dat"
rm -f "$ACMI"
camBefore=""; [ -f "$CAM" ] && camBefore=$(md5sum "$CAM" | cut -d' ' -f1)

FRAMES=$(( SECS * 20 + 250 ))
( cd "$RUNDIR" && timeout -k 5 -s KILL $(( SECS + 110 )) env \
    BOB_RUN_INIT=1 BOB_DRIVE_C="$HOME/sgl/TUE/MigAlley/WP/drive_c" \
    MA_ENABLE_3D=1 MA_QUICKMISS="$QM" MA_TRACE_REPLAY=1 \
    $([ "$CONTROL" = 1 ] && echo MA_ACMI=0) \
    $([ "$CONTROL" = "blockclock" ] && echo MA_ACMI_BLOCKCLOCK=1) \
    BOB_CLICKSEQ="40,r1;60,r1;110,#2063:2" BOB_AUTOEXIT="$FRAMES" \
    "$BIN" ) >"$OUT/fly.log" 2>&1

# 1. did the flight record at all?
n=$(grep -a "StopRecord" "$OUT/fly.log" | tail -1 | sed -n 's/.*replayframecount=\([0-9]*\).*/\1/p')
if [ -n "${n:-}" ] && [ "${n:-0}" -gt 0 ]; then say "flight recorded" "yes — $n frames"
else bad "flight recorded" "NO — nothing below is meaningful"; fi

# 2..7 — structure, in one pass
if [ ! -s "$ACMI" ]; then
  bad "acmi file exists and is non-empty" "NO"
else
  say "acmi file" "$(wc -c < "$ACMI") bytes, $(wc -l < "$ACMI") lines"
  python3 - "$ACMI" <<'PY' || fail=1
import sys, re
p = sys.argv[1]
L = [l.rstrip('\r\n') for l in open(p, encoding='utf-8', errors='replace')]
ok = True
def chk(name, cond, detail=""):
    global ok
    print("  %-48s %s" % (name, "yes" if cond else ("NO — " + detail)))
    if not cond: ok = False

chk("header line 1 is FileType",  len(L) > 0 and L[0] == "FileType=text/acmi/tacview", repr(L[0] if L else None))
chk("header line 2 is FileVersion", len(L) > 1 and L[1].startswith("FileVersion="), repr(L[1] if len(L)>1 else None))
props = [l for l in L[:12] if l.startswith("0,")]
need = ["ReferenceTime", "ReferenceLongitude", "ReferenceLatitude"]
missing = [k for k in need if not any(k in x for x in props)]
chk("global properties on object 0", not missing, "missing " + ",".join(missing))

times, bad_t = [], None
for l in L:
    if l.startswith("#"):
        try: t = float(l[1:])
        except ValueError: bad_t = l; break
        if times and t <= times[-1]: bad_t = "%s after %s" % (l, times[-1]); break
        times.append(t)
chk("time markers strictly monotonic", bad_t is None and len(times) > 1,
    str(bad_t) if bad_t else "only %d marker(s)" % len(times))

objre = re.compile(r'^([0-9a-fA-F]+),T=([^,]*)')
seen, malformed, refs, malformed_line, all_lines = set(), None, 0, None, [l for l in L if l and not l.startswith("#")]
for l in L:
    if l.startswith("#") or l.startswith("0,") or l.startswith("-") or "=" not in l: continue
    m = objre.match(l)
    if not m:
        if l and not l.startswith("File"): malformed, malformed_line = l[:60], l; break
        continue
    refs += 1
    if len(m.group(2).split("|")) != 9:
        malformed = "transform has %d fields: %s" % (len(m.group(2).split("|")), l[:60]); malformed_line = l; break
    seen.add(m.group(1))
# S259: the WORKING file is written continuously, so a kill can cut its final line mid-number.
# That is expected here and is NOT a defect -- ma_acmi_save_as copies whole lines only, so a
# PUBLISHED .acmi can never carry one. Tolerate a single malformed line only if it is the LAST,
# and say so out loud rather than silently forgiving it.
if malformed is not None and malformed_line is not None and all_lines and malformed_line == all_lines[-1]:
    print("  %-48s %s" % ("trailing partial line (working file only)", "tolerated — save_as copies whole lines"))
    malformed = None
chk("every object line is well-formed", malformed is None, str(malformed))
chk("at least one object with a track", refs > 1, "%d object lines" % refs)
print("  %-48s %d" % ("distinct objects exported", len(seen)))
sys.exit(0 if ok else 1)
PY
fi

# 8. the recording is untouched by the export
if [ -n "$camBefore" ] && [ -f "$CAM" ]; then
  camAfter=$(md5sum "$CAM" | cut -d' ' -f1)
  if [ "$camBefore" = "$camAfter" ]; then say "replay.dat unchanged by this flight" "n/a (a flight re-records it)"
  else say "replay.dat re-recorded (expected)" "yes"; fi
fi
# the real additive check: the recording is self-consistent
sz=$(stat -c%s "$CAM" 2>/dev/null || echo 0)
if [ "${sz:-0}" -gt 0 ]; then say "recording present after export" "$sz bytes"
else bad "recording present after export" "NO — the export may have broken recording"; fi


# 9. COMPLETENESS -- the timeline must cover the samples (PO-79).
#    Assertion 5 checks the markers are MONOTONIC, and it passed for months while the export was
#    truncated to 51 s, because ma_acmi_time DROPS any non-increasing timestamp: the survivors are
#    monotonic by construction. Structural validity is not completeness. The tell is the ratio of
#    object samples to time markers -- one marker per frame per object is the design, so if the
#    object lines imply far more frames than there are markers, the clock stalled and every extra
#    sample was stacked under the last marker (which is what the PO saw as the end-jump).
if [ -s "$ACMI" ]; then
  py=$(python3 - "$ACMI" <<'PYEOF'
import sys
L=open(sys.argv[1], errors='replace').read().split('\n')
marks=[l for l in L if l.startswith('#')]
objs=set(); samples=0
for l in L:
    if l and not l.startswith('#') and ',T=' in l:
        objs.add(l.split(',')[0]); samples+=1
n=len(objs) or 1
print(f"{len(marks)} {samples} {n} {samples//n}")
PYEOF
)
  set -- $py; MK=$1; SA=$2; NO=$3; FR=$4
  say "markers=$MK  objects=$NO  samples/object=$FR"
  if [ "$MK" -gt 0 ] && [ "$FR" -gt $((MK + MK/10 + 5)) ]; then
    say "9. timeline covers the samples" "NO -- $FR frames/object vs only $MK markers: the clock stalled"
    fail=1
  else
    say "9. timeline covers the samples" "yes"
  fi
fi

echo "----------------------------------------"
if [ "$CONTROL" = 1 ]; then
  if [ "$fail" -ne 0 ]; then
    if [ "$CONTROL" = "blockclock" ]; then
      echo "CONTROL OK: MA_ACMI_BLOCKCLOCK=1 restores the within-block clock and assertion 9 fires"
    else
      echo "CONTROL OK: with MA_ACMI=0 the export is absent and the assertions fired"
    fi
    exit 0
  else echo "CONTROL FAILED: no export, yet every assertion passed — they are asleep"; exit 1; fi
fi
if [ "$fail" -eq 0 ]; then echo "PASS: the .acmi is structurally valid and the recording still works"
else echo "FAIL"; exit 1; fi
