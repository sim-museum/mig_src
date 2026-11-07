#!/usr/bin/env python3
"""Read the AI-track census stack: one line per stage and case with sweep, escort and the 60/40 blend.

  ma_stack_report.py STAGE [STAGE ...]      e.g. C RB S T M FG P   (sweep/s<STAGE>, bomb/esc<STAGE>)

Targets (PO): case 1 (hero Sabres v hero MiGs) about 1.3:1 and never below 1:1; case 2 (hero Sabres v average MiGs)
5-12:1. The blend is 0.6 x sweep + 0.4 x escort (the war's sorties were mostly sweeps). Also per stage the median
open-fire range by side from [fire] lines (GUNS-WANDER-1), where the binary prints them.
"""
import glob
import os
import re
import statistics
import subprocess
import sys

ROOT = "/home/admin/ma-gates"
SUM = "/home/admin/ma/tools/ma_killratio_sum.py"


def exchange(d):
    if not os.path.isdir(d):
        return None
    out = subprocess.run([sys.executable, SUM, d], capture_output=True, text=True).stdout.strip().splitlines()
    m = re.search(r"lost (\d+):(\d+)", out[-1]) if out else None
    return (int(m.group(1)), int(m.group(2))) if m else None


def add(a, b):
    if a is None:
        return b
    if b is None:
        return a
    return (a[0] + b[0], a[1] + b[1])


def ratio(x):
    return x[0] / x[1] if x and x[1] else float("inf") if x and x[0] else 0.0


def logs(dirs):
    for d in dirs:
        for f in glob.glob(os.path.join(d, "*", "run.log")):
            yield f


def fire_ranges(dirs):
    r = {"UN": [], "MiG": []}
    for f in logs(dirs):
        for ln in open(f, errors="replace"):
            if ln.startswith("[fire]"):
                m = re.search(r"nat=(\d+) .*range=([\d.]+)", ln)
                if m:
                    r["MiG" if m.group(1) == "1" else "UN"].append(float(m.group(2)))
    return {k: (len(v), statistics.median(v)) for k, v in r.items() if v}


def main(stages):
    for st in stages:
        sw, es = f"{ROOT}/sweep/s{st}", f"{ROOT}/bomb/esc{st}"
        for c in (1, 2):
            s = add(exchange(f"{sw}/c{c}a"), exchange(f"{sw}/c{c}b"))
            e = exchange(f"{es}/c{c}")
            if s is None and e is None:
                continue
            bl = 0.6 * ratio(s) + 0.4 * ratio(e) if s and e else None
            tgt = "1.3, >= 1" if c == 1 else "5-12"
            ok = "" if bl is None else (" OK" if (bl >= 1.0 if c == 1 else 5.0 <= bl <= 12.0) else " OUT")
            print("%-3s case %d  sweep %s (%.2f)  escort %s (%.2f)  blend %s  [target %s]%s" % (
                st, c, "%d:%d" % s if s else "-", ratio(s) if s else 0, "%d:%d" % e if e else "-", ratio(e) if e else 0,
                "%.2f" % bl if bl is not None else "-", tgt, ok))
        fr = fire_ranges([f"{sw}/c1a", f"{sw}/c1b", f"{sw}/c2a", f"{sw}/c2b"])
        if fr:
            print("     open-fire median: " + "  ".join("%s %.0f m (n %d)" % (k, v[1], v[0]) for k, v in fr.items()))


if __name__ == "__main__":
    main(sys.argv[1:] or ["C"])
