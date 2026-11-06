#!/usr/bin/env python3
# MA-KILLRATIO-1 summary: per skill, Sabre:MiG kills (= MiG losses : Sabre losses) with a Wilson 95% interval.
import re, sys, glob, math, collections
SABRE, MIG = {'265'}, {'264', '289'}
def wilson(k, n, z=1.96):
    if n == 0: return (0.0, 1.0)
    p = k / n; d = 1 + z*z/n; c = (p + z*z/(2*n)) / d; h = z*math.sqrt(p*(1-p)/n + z*z/(4*n*n)) / d
    return (max(0.0, c - h), min(1.0, c + h))
def ratio(p): return float('inf') if p >= 1 else p / (1 - p)
tot = collections.defaultdict(lambda: [0, 0, 0, 0])
byshape = collections.defaultdict(collections.Counter)
# a crash kills the process, so its signal is printed by the census shell, not into run.log: read the exit codes too
# (census done.txt lines '<skill> <start> exit=<code>'; 124 = the planned timeout)
exits = {}
try:
    for ln in open(sys.argv[1] + '/done.txt'):
        m = re.match(r'(\d+) (\d+) exit=(\d+)', ln)
        if m: exits[(int(m.group(1)), int(m.group(2)))] = int(m.group(3))
except OSError: pass
for d in sorted(glob.glob(sys.argv[1] + '/k*_s*')):
    k = int(re.search(r'/k(\d+)_s', d).group(1))
    log = open(d + '/run.log', encoding='latin-1').read()
    # by side, not shape: the F-80 (266) and Yak-9 (270) were in neither shape set, so every F-80 v Yak loss went
    # uncounted ("no kills" in yk1-yk4); nat 2 = UN, 1 = Red
    lost = re.findall(r'\[lost\] id=\d+ nat=(\d+) shape=(\d+)', log)
    sl = sum(n == '2' or s in SABRE for n, s in lost); ml = sum(n == '1' or s in MIG for n, s in lost)
    st = int(re.search(r'_s(\d+)$', d).group(1)); ex = exits.get((k, st), 124)
    crash = 1 if re.search(r'CRASH|Segmentation|Floating point', log) or ex not in (0, 124) else 0
    t = tot[k]; t[0] += ml; t[1] += sl; t[2] += 1; t[3] += crash
    for n, sh in lost: byshape[k][sh] += 1
    print(f"{d.rsplit('/', 1)[1]}: Sabre kills {ml}  MiG kills {sl}{'  CRASH' if crash else ''}")
print()
for k in sorted(tot):
    ml, sl, n, cr = tot[k]; lo, hi = wilson(ml, ml + sl)
    print(f"skill {k:2d}: {n} fights ({cr} crashed)  Sabre:MiG kills {ml}:{sl}  ratio {ratio(ml/(ml+sl)) if ml+sl else float('nan'):.2f}"
          f"  95% {ratio(lo):.2f}..{ratio(hi):.2f}  {'PASS' if ml + sl and ratio(hi) >= 1 else 'BELOW 1:1' if ml + sl else 'no kills'}")
    # PO 11-04: the historical figure is the Sabre-v-MiG exchange (MiGs lost : Sabres lost), apart from bomber losses
    b = byshape[k]; xm, xs = b.get('289', 0), b.get('265', 0); xlo, xhi = wilson(xm, xm + xs)
    print(f"          exchange MiG:Sabre lost {xm}:{xs}  {ratio(xm/(xm+xs)) if xm+xs else float('nan'):.2f}  95% {ratio(xlo):.2f}..{ratio(xhi):.2f}"
          f"   UN lost B-29 {b.get('269', 0)} F-80 {b.get('266', 0)} F-86 {xs}")
