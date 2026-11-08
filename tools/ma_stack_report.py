#!/usr/bin/env python3
"""Read the AI-track census stack: one line per stage and case with sweep, escort and the 60/40 blend.

  ma_stack_report.py STAGE [STAGE ...]      e.g. C RB S T M FG P   (sweep/s<STAGE>, bomb/esc<STAGE>)

Targets (PO): case 1 (hero Sabres v hero MiGs) about 1.3:1 and never below 1:1; case 2 (hero Sabres v average MiGs)
5-12:1. The blend is 0.6 x sweep + 0.4 x escort (the war's sorties were mostly sweeps). Also per stage the median
open-fire range by side from [fire] lines (GUNS-WANDER-1), where the binary prints them: sweep AND escort fights since
CENSUS-CI-1 (the earlier report counted sweeps only, so its n were smaller).

CENSUS-CI-1: the exchange is MiGs lost (shape 289) : Sabres lost (265), counted per start (one fight = one run.log).
Losses inside one fight are correlated, so the interval comes from a bootstrap over STARTS (sweep and escort starts
resampled separately, the blend recomputed each time), not from treating losses as independent. A cell is called OK or
OUT only when its 80 % interval says so; otherwise UNDECIDED (run another stage before judging).
"""
import glob
import os
import random
import re
import statistics
import sys

ROOT = "/home/admin/ma-gates"
MIG, SABRE = "289", "265"
LEVEL = 0.80          # two-sided interval
NBOOT = 2000
CAP = 99.0            # a resample with no Sabre lost counts as this ratio


def scan(d, fire):
    """Per start in census dir d: (MiGs lost, Sabres lost); also appends [fire] ranges to fire."""
    out = []
    for f in sorted(glob.glob(os.path.join(d, "*", "run.log"))):
        m = s = 0
        for ln in open(f, errors="replace"):
            if ln.startswith("[lost]"):
                g = re.search(r"shape=(\d+)", ln)
                if g:
                    m += g.group(1) == MIG
                    s += g.group(1) == SABRE
            elif ln.startswith("[fire]"):
                g = re.search(r"nat=(\d+) .*range=([\d.]+)", ln)
                if g:
                    fire["MiG" if g.group(1) == "1" else "UN"].append(float(g.group(2)))
        out.append((m, s))
    return out


def ratio(m, s):
    return m / s if s else (CAP if m else 0.0)


def total(starts):
    return (sum(a for a, _ in starts), sum(b for _, b in starts))


def blend(sw, es):
    return 0.6 * ratio(*total(sw)) + 0.4 * ratio(*total(es))


def interval(sw, es, rng):
    """Bootstrap over starts: the LEVEL interval of the blend (or of the one arm present)."""
    vals = []
    for _ in range(NBOOT):
        a = [rng.choice(sw) for _ in sw] if sw else []
        b = [rng.choice(es) for _ in es] if es else []
        vals.append(blend(a, b) if sw and es else ratio(*total(a or b)))
    vals.sort()
    lo = vals[int((1 - LEVEL) / 2 * NBOOT)]
    hi = vals[min(NBOOT - 1, int((1 + LEVEL) / 2 * NBOOT))]
    return lo, hi


def verdict(c, lo, hi):
    if c == 1:            # about 1.3, never below 1
        return "OK" if lo >= 1.0 else "OUT" if hi < 1.0 else "UNDECIDED"
    return "OK" if lo >= 5.0 and hi <= 12.0 else "OUT" if hi < 5.0 or lo > 12.0 else "UNDECIDED"


def main(stages):
    rng = random.Random(1107)
    for st in stages:
        sw_dir, es_dir = f"{ROOT}/sweep/s{st}", f"{ROOT}/bomb/esc{st}"
        fire = {"UN": [], "MiG": []}
        for c in (1, 2):
            sw = scan(f"{sw_dir}/c{c}a", fire) + scan(f"{sw_dir}/c{c}b", fire)
            es = scan(f"{es_dir}/c{c}", fire)
            if not sw and not es:
                continue
            lo, hi = interval(sw, es, rng)
            s, e = total(sw), total(es)
            bl = "%.2f" % blend(sw, es) if sw and es else "-"
            tgt = "1.3, >= 1" if c == 1 else "5-12"
            arm = "blend" if sw and es else "sweep only" if sw else "escort only"
            print("%-3s case %d  sweep %s (%.2f, n %d)  escort %s (%.2f, n %d)  blend %s  %d%% %.2f..%.2f (%s)  [target %s] %s" % (
                st, c, "%d:%d" % s if sw else "-", ratio(*s), len(sw), "%d:%d" % e if es else "-", ratio(*e), len(es),
                bl, int(LEVEL * 100), lo, hi, arm, tgt, verdict(c, lo, hi)))
        fr = {k: (len(v), statistics.median(v)) for k, v in fire.items() if v}
        if fr:
            print("     open-fire median: " + "  ".join("%s %.0f m (n %d)" % (k, v[1], v[0]) for k, v in fr.items()))


if __name__ == "__main__":
    main(sys.argv[1:] or ["C"])
