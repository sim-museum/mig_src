#!/usr/bin/env bash
# port/compass_wrap.sh — PO-81: does the gyro compass pinwheel across a HEADING WRAP,
# and does MA_COMPASS_WRAPFIX stop it?
#
# WHY THIS EXISTS
#   S331-S3 ran the A/B on tacview_export and proved NOTHING: that recipe flies STRAIGHT, so
#   `hdg` max step was 0.0 deg and the 0/360 crossing the whole hypothesis is about never
#   happened. A clean result there meant "not exercised", not "not broken" -- the same trap
#   PO-82 fell into twice. So this gate FLIES A CAMPAIGN MISSION (MA_CAMP_FLY drives the
#   frag->Fly chain and the aircraft follows a waypoint route, which turns), and it REFUSES TO
#   REPORT a verdict unless the heading actually crossed the wrap.
#
#   Usage: gl-lock port/compass_wrap.sh
#   exit 0 = fix works, 1 = fix does not work, 2 = INCONCLUSIVE (no wrap crossing observed)
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WMIG="${WMIG:-$ROOT/build/wmig}"
MIG="${MIG:-$HOME/sgl/TUE/MigAlley/WP/drive_c/rowan/mig}"
OUT="${OUT:-/tmp/compass_wrap}"; mkdir -p "$OUT"
SECS="${SECS:-200}"
CLICKS="30,r3;65,#1055;100,#2063:1"     # S63 campaign recipe (font-proof), from asan_all.sh
[ -x "$WMIG" ] || { echo "no binary at $WMIG" >&2; exit 2; }

# S81: MA_CAMP_FLY ADVANCES THE CAMPAIGN. The player's save is irreplaceable -- stash and restore
# it around both arms, exactly as asan_all.sh does. A diagnostic must never eat game state.
SAVE="$MIG/SaveGame/Auto Save.sav"
[ -f "$SAVE" ] && cp -a "$SAVE" "$OUT/player_autosave.bak"
restore() { [ -f "$OUT/player_autosave.bak" ] && cp -f "$OUT/player_autosave.bak" "$SAVE"; }
trap restore EXIT

arm () {  # $1 = tag, $2 = extra env
  ( cd "$MIG" && timeout -k 5 -s KILL "$SECS" env \
      BOB_RUN_INIT=1 BOB_DRIVE_C="$HOME/sgl/TUE/MigAlley/WP/drive_c" \
      MA_ENABLE_3D=1 MA_IGNORE_SAVE_DATE=1 MA_CAMP_FLY=1 MA_TRACE_COMPASS=1 \
      $2 BOB_CLICKSEQ="$CLICKS" "$WMIG" ) > "$OUT/$1.log" 2>&1
  echo "  $1: $(grep -ac '\[compass\]' "$OUT/$1.log") compass samples"
}
echo "PO-81 compass wrap A/B — campaign flight, ${SECS}s per arm"
arm off ""
arm on  "MA_COMPASS_WRAPFIX=1"

python3 - "$OUT" <<'PYEOF'
import sys, re
out = sys.argv[1]
def rows(p):
    r=[]
    try: t=open(p, errors='replace').read().split('\n')
    except Exception: return r
    for l in t:
        m=re.search(r'hdg=([-\d.]+) deg\s+accompass=(-?\d+).*acgyrocompass=(-?\d+)', l)
        if m: r.append((float(m.group(1)), int(m.group(2)), int(m.group(3))))
    return r
def err(r):
    e=[]
    for h,a,g in r:
        d=(g - h*-182.04 + 32768) % 65536 - 32768
        e.append(abs(d))
    e.sort(); return e
res={}
for tag in ("off","on"):
    r=rows("%s/%s.log"%(out,tag)); res[tag]=r
    if not r: print("  %s: NO SAMPLES"%tag); continue
    hs=[x[0] for x in r]
    # a wrap crossing shows as a large single-step change in heading
    steps=[abs(hs[i]-hs[i-1]) for i in range(1,len(hs))]
    crossed = any(s>180 for s in steps)
    e=err(r)
    print("  %-3s samples=%-5d hdg range %.0f..%.0f  wrap-crossed=%s  |gyro-hdg| median %.0f max %.0f"
          %(tag,len(r),min(hs),max(hs),"YES" if crossed else "no",e[len(e)//2],e[-1]))
    res[tag+"_crossed"]=crossed; res[tag+"_err"]=e
print("-"*40)
if not res.get("off") or not res.get("on"):
    print("INCONCLUSIVE: an arm produced no samples"); sys.exit(2)
if not res.get("off_crossed"):
    print("INCONCLUSIVE: the heading never crossed the wrap in the OFF arm --")
    print("the condition under test did not occur, so neither arm means anything.")
    print("This is the S331-S3 failure; do not read a clean result as a pass.")
    sys.exit(2)
eo, en = res["off_err"], res["on_err"]
mo, mn = eo[len(eo)//2], en[len(en)//2]
print("median |gyro - heading|:  OFF %.0f   ON %.0f"%(mo,mn))
if mo > 2000 and mn < mo/4:
    print("PASS: the card diverges across the wrap with the fix OFF and tracks with it ON"); sys.exit(0)
if mo <= 2000:
    print("FAIL/RETHINK: the OFF arm did NOT diverge, so the wrap is not what makes the card spin")
    sys.exit(1)
print("FAIL: the fix did not restore tracking"); sys.exit(1)
PYEOF
