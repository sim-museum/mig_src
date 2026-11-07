#!/usr/bin/env python3
"""NET-LATENCY-1: how far apart two instances of one multiplayer flight draw each player's aircraft.

  ma_acmi_mpdiff.py host.log client.log

Both instances run with MA_TRACE_MPPOS=1, which prints "[mppos] frame F slot S me M x X y Y z Z" every 0.5 s for
every player, keyed by Rowan's shared lockstep FrameCount. Joining the two logs on (frame, slot) gives, for each
player, the distance between where its owner has it (me 1) and where the other instance draws it (me 0) at the
same simulation frame. World units are centimetres.
"""
import re
import sys

PAT = re.compile(r"\[mppos\] frame (\d+) slot (\d+) me (\d) x (-?\d+) y (-?\d+) z (-?\d+)")


def load(path):
    """Rowan's FrameCount is a byte: it wraps every 256 frames (~21 s). Unwrapped here in file order, so frames from
    different laps do not collide (both peers start at 0 together, within a few frames)."""
    out, last, lap = {}, None, 0
    for ln in open(path, errors="replace"):
        m = PAT.search(ln)
        if m:
            f, s, me, x, y, z = (int(g) for g in m.groups())
            if last is not None and f < last - 128:
                lap += 256
            last = f
            out[(f + lap, s)] = (me, x, y, z)
    return out


def table(A, B, off):
    per = {}
    for (f, slot), (ma, *pa) in A.items():
        k = (f + off, slot)
        if k not in B:
            continue
        mb, *pb = B[k]
        d = sum((p - q) ** 2 for p, q in zip(pa, pb)) ** 0.5 / 100.0
        owner = "host" if ma else "client" if mb else "neither"
        per.setdefault((slot, owner), []).append(d)
    return per


def med(per):
    allv = sorted(v for ds in per.values() for v in ds)
    return allv[len(allv) // 2] if allv else float("inf"), len(allv)


def main(a):
    if len(a) != 2:
        raise SystemExit(__doc__)
    A, B = load(a[0]), load(a[1])
    m0, n0 = med(table(A, B, 0))
    # FrameCount should be one simulation moment on both peers; if it is not, some other offset fits far better
    best = min(((med(table(A, B, o))[0], o) for o in range(-3000, 3001, 6) if med(table(A, B, o))[1] >= 10), default=None)
    if best and best[1] != 0 and best[0] < m0 / 4:
        print("NOTE: frame offset %+d fits better (median %.1f m) than the same frame (median %.1f m): the two peers' "
              "FrameCount differ by that much" % (best[1], best[0], m0))
    per = table(A, B, 0)
    if not A or not B:
        raise SystemExit("no [mppos] samples: were both run with MA_TRACE_MPPOS=1?")
    if not per:
        print("(no samples on the same frame number; the lag fit below interpolates)")
    for (slot, owner), ds in sorted(per.items()):
        ds.sort()
        print("slot %d (flown on the %s): n %4d  median %7.1f m  p95 %7.1f m  max %7.1f m" % (
            slot, owner, len(ds), ds[len(ds) // 2], ds[int(len(ds) * 0.95)], ds[-1]))
    # display lag: how far BEHIND (in frames of the owner's count) the other peer draws each player's aircraft; the
    # owner's track is interpolated between its samples, so the answer is finer than the sampling
    for slot in sorted({k[1] for k in A} | {k[1] for k in B}):
        for own, view, who in ((A, B, "host"), (B, A, "client")):
            otr = sorted((f, tuple(v[1:])) for (f, sl), v in own.items() if sl == slot and v[0] == 1)
            vtr = [(f, tuple(v[1:])) for (f, sl), v in view.items() if sl == slot and v[0] == 0]
            if len(otr) < 4 or len(vtr) < 4:
                continue
            fs = [f for f, _ in otr]

            def at(f):
                import bisect
                i = bisect.bisect_left(fs, f)
                if i <= 0 or i >= len(fs):
                    return None
                (f0, p0), (f1, p1) = otr[i - 1], otr[i]
                a = (f - f0) / (f1 - f0)
                return tuple(p0[k] + a * (p1[k] - p0[k]) for k in range(3))
            best = None
            for l10 in range(-600, 601, 5):          # -60..+60 frames, 0.5 frame steps
                L = l10 / 10.0
                e = []
                for f, p in vtr:
                    q = at(f - L)                      # the owner's position L frames earlier
                    if q:
                        e.append(sum((x - y) ** 2 for x, y in zip(p, q)) ** 0.5 / 100.0)
                if len(e) >= max(4, len(vtr) // 2):
                    e.sort(); m = e[len(e) // 2]
                    if best is None or m < best[1]:
                        best = (L, m, len(e))
            if best:
                print("slot %d (flown on the %s): drawn %+.1f frames behind its owner (median residual %.1f m, n %d)" % (
                    slot, who, best[0], best[1], best[2]))


if __name__ == "__main__":
    main(sys.argv[1:])
