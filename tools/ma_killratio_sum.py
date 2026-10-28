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
for d in sorted(glob.glob(sys.argv[1] + '/k*_s*')):
    k = int(re.search(r'/k(\d+)_s', d).group(1))
    log = open(d + '/run.log', encoding='latin-1').read()
    lost = re.findall(r'\[lost\] id=\d+ nat=\d+ shape=(\d+)', log)
    sl = sum(s in SABRE for s in lost); ml = sum(s in MIG for s in lost)
    crash = 1 if re.search(r'CRASH|Segmentation|Floating point', log) else 0
    t = tot[k]; t[0] += ml; t[1] += sl; t[2] += 1; t[3] += crash
    print(f"{d.rsplit('/', 1)[1]}: Sabre kills {ml}  MiG kills {sl}{'  CRASH' if crash else ''}")
print()
for k in sorted(tot):
    ml, sl, n, cr = tot[k]; lo, hi = wilson(ml, ml + sl)
    print(f"skill {k:2d}: {n} fights ({cr} crashed)  Sabre:MiG kills {ml}:{sl}  ratio {ratio(ml/(ml+sl)) if ml+sl else float('nan'):.2f}"
          f"  95% {ratio(lo):.2f}..{ratio(hi):.2f}  {'PASS' if ml + sl and ratio(hi) >= 1 else 'BELOW 1:1' if ml + sl else 'no kills'}")
