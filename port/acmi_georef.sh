#!/usr/bin/env bash
# port/acmi_georef.sh -- S305 gate: the .acmi theatre is pinned to real Korea.
#
# WHY THIS EXISTS
# ---------------
# The Lon/Lat in an .acmi were an admitted guess (S296: ref 127.0E/37.5N, origin never subtracted),
# which put a Sinuiju dogfight in the Sea of Japan. S305 solved the placement from two airfield
# positions in SRC/BFIELDS/MAINMIG.BFI. This gate keeps that solution honest: it drives the REAL
# ma_acmi_object() and reads the Lon/Lat it actually writes -- not a re-implementation of the
# formula, which would pass no matter what the shipped code did.
#
# It needs no GL and no flight: the conversion is pure arithmetic on the sampler's arguments.
#
# ⚠️ NEGATIVE CONTROL: CONTROL=1 sets MA_ACMI_REF back to the old 37.5,127.0 guess. Every
# assertion MUST go red. Without that, a gate whose tolerance is too loose reads identical to one
# that works.
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${OUT:-/tmp/ma_acmi_georef}"; mkdir -p "$OUT"
TOL_KM="${TOL_KM:-8}"
CONTROL="${CONTROL:-0}"

cat > "$OUT/drive.cpp" <<'C'
#include <stdio.h>
extern "C" {
int  ma_acmi_begin(const char*);
void ma_acmi_time(double);
void ma_acmi_object(unsigned long,double,double,double,double,double,double,
                    const char*,const char*,const char*,int);
void ma_acmi_end(void);
}
/* world CENTIMETRES straight out of SRC/BFIELDS/MAINMIG.BFI, converted the way Replay.cpp does */
struct AF { const char* n; double x, z; } af[] = {
  {"Kimpo",     61615953, 61326673}, {"Sinuiju",   41409080, 89304918},
  {"Suwon",     63391986, 57846698}, {"Pyongyang", 52997937, 77705411},
  {"Antung",    40092672, 88426496}, {"Taegu",     77954141, 42901566}, {0,0,0}};
int main(void){
  if(!ma_acmi_begin("georef")) return 2;
  ma_acmi_time(0.0);
  for(int i=0; af[i].n; i++)
    ma_acmi_object(i+1, af[i].x/100.0, af[i].z/100.0, 0,0,0,0, af[i].n, "Ground", "Blue", 0);
  ma_acmi_end(); return 0;
}
C

# compile the SHIPPED ma_acmi.cpp -- same source the game links
g++ -m32 -w -DFF_LINUX -DMA_LINUX -I"$ROOT/SRC/compat" -I"$ROOT/SRC/H" \
    -o "$OUT/drive" "$ROOT/SRC/compat/ma_acmi.cpp" "$OUT/drive.cpp" -lm 2>"$OUT/cc.log" \
  || { echo "  BUILD FAILED — see $OUT/cc.log"; tail -5 "$OUT/cc.log"; exit 2; }

echo "acmi georef — do the emitted Lon/Lat land on the real airfields?"
[ "$CONTROL" = 1 ] && echo "   [NEGATIVE CONTROL: old 37.5N/127.0E guess restored; every row MUST go RED]"
cd "$OUT"
ACMI=acmi_current.txt      # what ma_acmi_begin() names its working file
rm -f "$ACMI"
env $([ "$CONTROL" = 1 ] && echo MA_ACMI_REF=37.5,127.0) ./drive || { echo "  driver failed"; exit 2; }
[ -s "$ACMI" ] || { echo "  no recording written"; exit 2; }

python3 - "$ACMI" "$TOL_KM" "$CONTROL" <<'PY'
import re,sys,math
acmi,tol,control=sys.argv[1],float(sys.argv[2]),sys.argv[3]=='1'
real={"Kimpo":(37.558,126.791),"Sinuiju":(40.100,124.400),"Suwon":(37.239,127.007),
      "Pyongyang":(39.030,125.750),"Antung":(40.025,124.286),"Taegu":(35.894,128.659)}
bad=0;seen=0
for L in open(acmi,errors='ignore'):
    m=re.match(r'^[0-9a-f]+,T=([^|]*)\|([^|]*)\|.*Name=([A-Za-z]+)',L)
    if not m: continue
    lon,lat,n=float(m.group(1)),float(m.group(2)),m.group(3)
    if n not in real: continue
    seen+=1
    rla,rlo=real[n]
    dn=(lat-rla)*111.1; de=(lon-rlo)*111.32*math.cos(math.radians(rla))
    km=math.hypot(dn,de); ok=km<=tol
    if not ok: bad+=1
    print("  %-10s %8.4fN %9.4fE   %6.1f km  %s"%(n,lat,lon,km,"ok" if ok else "OFF"))
print("  ----------------------------------------")
if seen!=len(real): print("  ONLY %d/%d airfields emitted — the driver, not the calibration"%(seen,len(real))); sys.exit(2)
if control:
    if bad: print("CONTROL OK: the old guess is %d/%d rows out of tolerance"%(bad,seen)); sys.exit(0)
    print("CONTROL FAILED: the discarded guess passed — the tolerance is asleep"); sys.exit(1)
if bad: print("FAIL: %d/%d airfields outside %g km"%(bad,seen,tol)); sys.exit(1)
print("PASS: all %d airfields within %g km of their real coordinates"%(seen,tol))
PY
