#!/usr/bin/env bash
# R11 -- ACMI export orientation gate (S276/S327).
#
# WHY THIS EXISTS
#   The yaw-sense bug (aircraft flying backwards in Tacview) survived days of analysis because the
#   only check being run was INTERNAL: yaw against the course implied by the same file's U/V. That
#   passed at 3.8 deg while the file was wrong, because yaw and course SHARE the convention -- they
#   agree with each other and both disagree with Tacview. A self-consistency check can never
#   validate a convention. This gate checks the convention itself.
#
# WHAT IT ASSERTS  (Tacview orients the model from Yaw ALONE; Heading does not affect it)
#   written Yaw     ~= (360 - course) mod 360     <- negated
#   written Heading ~= course                     <- true compass, kept honest
#
# THE TRAP THIS GATE MUST NOT FALL INTO
#   Yaw 000 is a FIXED POINT of the negation, so near north-south the two hypotheses are
#   indistinguishable. Measured on a real sortie: whole-file medians came out 8.31 (raw) vs 10.54
#   (negated) -- which reads as "still broken" on a file that was actually FIXED. Restricted to
#   east-west segments the same file gave 141.53 vs 7.15. So: EAST-WEST SEGMENTS ONLY.
#
# Usage: tools/bob_acmi_orientation.sh <file.acmi> [more.acmi ...]
#        exit 0 = PASS, 1 = FAIL (raw fits better -> regression), 2 = INCONCLUSIVE (no usable data)
set -u
# Suite-compatible: with no argument, find MA's own exports. A gate that cannot be run by the
# suite protects nothing (S327's own lesson about real_mouse sitting outside $ALL for 22 sprints).
if [ $# -eq 0 ]; then
  MIG="${MIG:-$HOME/sgl/TUE/MigAlley/WP/drive_c/rowan/mig}"
  # ONLY the live export. Videos/*.acmi are an ARCHIVE -- every one predates S327 and is
  # therefore legitimately "wrong", so scanning them would leave this gate permanently red, which
  # trains the reader to ignore it. Pass paths explicitly to audit the archive on purpose.
  set -- $(ls -1 "$MIG/acmi_current.txt" 2>/dev/null)
  [ $# -ge 1 ] || { echo "acmi_orientation: SKIP -- no live acmi_current.txt under $MIG"; exit 2; }
fi
rc=0
for f in "$@"; do
  python3 - "$f" <<'PYEOF'
import sys,math,re
F=sys.argv[1]
t=None; tr={}
try: fh=open(F, errors='replace')
except Exception as e:
    print("%-34s INCONCLUSIVE: %s"%(F.split('/')[-1],e)); sys.exit(2)
for line in fh:
    line=line.strip()
    if line.startswith('#'):
        try: t=float(line[1:])
        except: pass
        continue
    m=re.match(r'^([0-9a-fA-F]+),T=([^,]*)', line)
    if not m or t is None or m.group(1)=='0': continue
    g=m.group(2).split('|')
    if len(g)<9: continue
    try: tr.setdefault(m.group(1),[]).append((float(g[5]),float(g[6]),float(g[7]),float(g[8])))
    except ValueError: pass
def ad(a,b): return (a-b+180)%360-180
raw=[];neg=[];hdg=[]
for oid,p in tr.items():
    for i in range(1,len(p)):
        du=p[i][1]-p[i-1][1]; dv=p[i][2]-p[i-1][2]
        if du*du+dv*dv < 4: continue                     # need real movement
        c=math.degrees(math.atan2(du,dv))%360
        if abs(math.sin(math.radians(c))) < 0.85: continue  # EAST-WEST only (see header)
        y=p[i][0]%360
        raw.append(abs(ad(y,c))); neg.append(abs(ad((360-y)%360,c))); hdg.append(abs(ad(p[i][3]%360,c)))
name=F.split('/')[-1]
if len(raw) < 50:
    print("%-34s INCONCLUSIVE: only %d east-west segments (need >=50) -- this sortie cannot "
          "discriminate; fly an east-west leg"%(name,len(raw)))
    sys.exit(2)
raw.sort(); neg.sort(); hdg.sort(); md=lambda a:a[len(a)//2]
mr,mn,mh=md(raw),md(neg),md(hdg)
# ALWAYS print both hypotheses. Reporting only "yaw matches course" is what hid the original bug.
print("%-34s E-W segs=%-6d  yaw_raw=%6.2f  yaw_negated=%6.2f  heading=%6.2f"%(name,len(raw),mr,mn,mh))
if mn < mr and mn <= 10.0 and mh <= 10.0:
    print("%-34s   PASS: yaw is negated (Tacview-correct) and heading is true compass"%"")
    sys.exit(0)
if mr < mn:
    print("%-34s   FAIL: RAW compass yaw fits better -- the S276/S327 fix has REGRESSED; "
          "aircraft will fly backwards in Tacview"%"")
    sys.exit(1)
print("%-34s   FAIL: neither hypothesis is clean (negated=%.2f heading=%.2f, want <=10)"%("",mn,mh))
sys.exit(1)
PYEOF
  r=$?
  # FAIL must dominate INCONCLUSIVE: a regression that shares a run with an undiscriminating
  # sortie must not be downgraded to "could not tell".
  case "$r" in
    1) rc=1 ;;
    2) [ "$rc" -eq 0 ] && rc=2 ;;
  esac
done
exit $rc
