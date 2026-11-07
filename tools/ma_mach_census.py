#!/usr/bin/env python3
"""MACH-1: how fast (in Mach) each type flies in census fights, from the ACMI tracks.

  ma_mach_census.py DIR [DIR ...]        (census dirs holding */fight.zip.acmi or */fight.acmi)

Once a second per aircraft: speed from the change of position, Mach from the ISA speed of sound at its altitude
(troposphere lapse to 11 km, isothermal above). Per type: seconds flown, seconds at or above Mach 0.86 / 0.92 / 1.0,
seconds in a steep dive (pitch below -15 deg) at or above 0.86, and the highest Mach held for a second.
"""
import glob
import io
import math
import os
import sys
import zipfile


def sound(alt):
    t = 288.15 - 0.0065 * alt if alt < 11000.0 else 216.65
    return math.sqrt(1.4 * 287.05 * t)


def tracks(path):
    if path.endswith(".zip.acmi"):
        z = zipfile.ZipFile(path)
        return io.TextIOWrapper(z.open(z.namelist()[0]), errors="replace")
    return open(path, errors="replace")


def fight(path, acc):
    names, last, prev, t = {}, {}, {}, 0.0
    for ln in tracks(path):
        if ln.startswith("#"):
            try:
                t = float(ln[1:])
            except ValueError:
                pass
            continue
        if ",T=" not in ln or ln[0] in "0-":
            continue
        oid, rest = ln.split(",", 1)
        if "Name=" in rest:
            names[oid] = rest.split("Name=", 1)[1].split(",", 1)[0].strip()
        f = rest[2:].split(",", 1)[0].split("|")
        cur = last.get(oid, [None] * 9)
        cur = [(f[i] if i < len(f) and f[i] != "" else cur[i]) for i in range(9)]
        last[oid] = cur
        sec = int(t)
        p = prev.get(oid)
        if p is None or sec > p[0]:
            try:
                alt, pitch, u, v = float(cur[2]), float(cur[4]), float(cur[6]), float(cur[7])
            except (TypeError, ValueError):
                continue
            if p is not None and 0 < t - p[1] < 3.0:
                d = math.sqrt((u - p[2]) ** 2 + (v - p[3]) ** 2 + (alt - p[4]) ** 2) / (t - p[1])
                m = d / sound(alt)
                if m < 2.0:                                     # a respawn or teleport is not flight
                    a = acc.setdefault(names.get(oid, "?"), [0, 0, 0, 0, 0, 0.0])
                    a[0] += 1
                    a[1] += m >= 0.86
                    a[2] += m >= 0.92
                    a[3] += m >= 1.0
                    a[4] += m >= 0.86 and pitch < -15.0
                    a[5] = max(a[5], m)
            prev[oid] = (sec, t, u, v, alt)


def main(dirs):
    acc, n = {}, 0
    for d in dirs:
        for f in sorted(glob.glob(os.path.join(d, "*", "fight.zip.acmi")) + glob.glob(os.path.join(d, "*", "fight.acmi"))):
            fight(f, acc)
            n += 1
    print("%d fights" % n)
    for name, a in sorted(acc.items()):
        if a[0] < 60:
            continue
        print("  %-8s %7d s flown   M>=0.86 %5.1f%%   M>=0.92 %5.2f%%   M>=1.0 %5.2f%%   steep dive at M>=0.86 %5.2f%%   max M %.2f" % (
            name, a[0], 100.0 * a[1] / a[0], 100.0 * a[2] / a[0], 100.0 * a[3] / a[0], 100.0 * a[4] / a[0], a[5]))


if __name__ == "__main__":
    main(sys.argv[1:])
