#!/usr/bin/env python3
"""PREDICT-METRIC-1: put a number on "predictable" from a census directory's Tacview tracks.

  python3 tools/ma_predict.py <census dir> [...]      (finds every */fight.zip.acmi below each dir)

Doctrine-agnostic, so it also measures Rowan's AI (MA_ACL_HERO=off), which prints no [acl] trace.
A THREAT EVENT is an enemy that, after 10 s without, comes within 1000 m in the pilot's rear hemisphere with his flight
path within 20 deg of the pilot (the hero doctrines' defend gate). The RESPONSE is what the threatened jet does in the
next 5 s, classified from its track: turn (into the attacker's side / away / none), vertical (climb / level / dive by
mean flight-path angle, +-10 deg) and a roll reversal (bank through 45 deg both ways). The SITUATION bin is attacker
angle-off (4), range (3), energy difference (3) and altitude (3).
Prints per side: events, response entropy (bits; 0 = always the same response) and the repeat rate (per situation bin
with >= 3 events, the share of the commonest response, event-weighted; 1.0 = fully predictable).
FIRST HIT (the PO's 11-04 example, "migs dive after first hit"): of the jets that take a first hit, the share whose mean
flight path over the next 5 s is below -30 deg. Needs the track's hit events (wmig from PREDICT-METRIC-1 s2 on).
"""
import math, sys, zipfile, glob, os
from collections import defaultdict, Counter

DT = 0.5            # resample step (s)
G = 9.81

def tracks(path):
    """{id: (side, [(t, x, y, alt, roll)])} sampled every DT, and {id: time of its first hit}."""
    z = zipfile.ZipFile(path); d = z.read(z.namelist()[0]).decode('latin-1')
    t = 0.0; nxt = 0.0; take = True; side = {}; last = {}; out = defaultdict(list); hit = {}
    for ln in d.splitlines():
        if ln.startswith('0,Event=Message|') and '|hit by ' in ln:
            hit.setdefault(ln.split('|')[1], t); continue
        if ln.startswith('#'):
            t = float(ln[1:]); take = t >= nxt
            if take: nxt = t + DT
            continue
        if not take or ln.startswith('-') or ',T=' not in ln: continue
        i, rest = ln.split(',', 1)
        f = rest.split(',')
        p = f[0][2:].split('|')
        if i not in side:
            for kv in f[1:]:
                if kv.startswith('Color='): side[i] = kv[6:]
        o = last.get(i, [''] * 9)
        p = [p[k] if k < len(p) and p[k] != '' else o[k] for k in range(9)]
        last[i] = p
        try: out[i].append((t, float(p[6]), float(p[7]), float(p[2]), float(p[3])))
        except ValueError: pass
    return {i: (side.get(i, '?'), s) for i, s in out.items() if side.get(i) in ('Blue', 'Red') and len(s) > 20}, hit

def first_hit_dives(tr, hit):
    """yield (side, dived) for each jet's first hit with 5 s of track after it."""
    for i, th in hit.items():
        if i not in tr: continue
        side, s = tr[i]
        ks = [k for k in range(1, len(s)) if th <= s[k][0] <= th + 5.0]
        if len(ks) < int(4.0 / DT): continue
        g = [math.degrees(math.atan2(state(s, k)[1][2], math.hypot(*state(s, k)[1][:2]))) for k in ks]
        yield side, sum(g) / len(g) < -30

def state(s, k):
    """position, velocity (m/s) and roll at sample k."""
    t0, x0, y0, a0, _ = s[k - 1]; t1, x1, y1, a1, r1 = s[k]
    dt = max(t1 - t0, 1e-3)
    return (x1, y1, a1), ((x1 - x0) / dt, (y1 - y0) / dt, (a1 - a0) / dt), r1

def ang(u, v):
    nu = math.sqrt(sum(a * a for a in u)) + 1e-9; nv = math.sqrt(sum(a * a for a in v)) + 1e-9
    c = sum(a * b for a, b in zip(u, v)) / (nu * nv)
    return math.degrees(math.acos(max(-1.0, min(1.0, c))))

def events(tr):
    """yield (side, situation bin, response class) for each threat event in one fight."""
    ids = list(tr)
    idx = {i: {round(p[0] / DT): k for k, p in enumerate(tr[i][1])} for i in ids}
    W = int(5.0 / DT)
    for me in ids:
        side, s = tr[me]; quiet_until = -1e9; on = False
        for k in range(1, len(s) - W - 1):
            t = s[k][0]; key = round(t / DT)
            P, V, roll = state(s, k); best = None
            for en in ids:
                if tr[en][0] == side: continue
                j = idx[en].get(key)
                if j is None or j < 1: continue
                Q, U, _ = state(tr[en][1], j)
                d = [P[n] - Q[n] for n in range(3)]; r = math.sqrt(sum(a * a for a in d))
                if r > 1000: continue
                if ang(V, [-a for a in d]) < 90: continue          # attacker not in my rear hemisphere
                if ang(U, d) > 20: continue                         # his flight path not on me
                if best is None or r < best[0]: best = (r, Q, U)
            if best is None: on = False; continue
            if on or t < quiet_until: quiet_until = t + 10; continue
            on = True; quiet_until = t + 10
            r, Q, U = best
            # situation bin
            aoff = ang(V, U)                                         # angle between the two flight paths
            vme = math.sqrt(sum(a * a for a in V)); vhe = math.sqrt(sum(a * a for a in U))
            de = (Q[2] + vhe * vhe / (2 * G)) - (P[2] + vme * vme / (2 * G))
            sb = (min(3, int(aoff // 45)), 0 if r < 400 else 1 if r < 700 else 2,
                  0 if de < -500 else 1 if de < 500 else 2, 0 if P[2] < 3000 else 1 if P[2] < 7000 else 2)
            # response over the next 5 s
            P5, V5, _ = state(s, k + W)
            h0 = math.atan2(V[0], V[1]); h5 = math.atan2(V5[0], V5[1])
            dh = (h5 - h0 + math.pi) % (2 * math.pi) - math.pi
            side_att = (Q[0] - P[0]) * V[1] - (Q[1] - P[1]) * V[0]  # >0: attacker on my right (x east, y north)
            turn = 'none' if abs(math.degrees(dh)) < 15 else ('into' if (dh > 0) == (side_att > 0) else 'away')  # dh > 0: right turn
            # flight-path angle averaged over the window
            g = [math.degrees(math.atan2(state(s, k + n)[1][2], math.hypot(*state(s, k + n)[1][:2]))) for n in range(1, W + 1)]
            gm = sum(g) / len(g)
            vert = 'climb' if gm > 10 else 'dive' if gm < -10 else 'level'
            rolls = [s[k + n][4] for n in range(W + 1)]
            rev = 'rev' if max(rolls) > 45 and min(rolls) < -45 else ''
            yield side, sb, f'{turn}/{vert}' + ('/' + rev if rev else '')

def entropy(c):
    n = sum(c.values()); return -sum(v / n * math.log2(v / n) for v in c.values() if v)

def main(dirs):
    files = sorted(f for d in dirs for f in glob.glob(os.path.join(d, '**', 'fight.zip.acmi'), recursive=True))
    resp = defaultdict(Counter); bins = defaultdict(lambda: defaultdict(Counter)); fh = defaultdict(Counter)
    for f in files:
        try: tr, hit = tracks(f)
        except Exception as e: print(f'skip {f}: {e}', file=sys.stderr); continue
        for side, sb, rc in events(tr):
            resp[side][rc] += 1; bins[side][sb][rc] += 1
        for side, dived in first_hit_dives(tr, hit):
            fh[side][dived] += 1
    print(f'{len(files)} fights')
    for side, name in (('Blue', 'UN'), ('Red', 'MiG')):
        c = resp[side]; n = sum(c.values())
        if not n: print(f'{name}: no threat events'); continue
        big = [b for b in bins[side].values() if sum(b.values()) >= 3]
        nb = sum(sum(b.values()) for b in big)
        rr = sum(max(b.values()) for b in big) / nb if nb else float('nan')
        top = ', '.join(f'{k} {100 * v / n:.0f}%' for k, v in c.most_common(4))
        print(f'{name}: {n} threat events  entropy {entropy(c):.2f} bits (max {math.log2(18):.2f})  '
              f'repeat rate {rr:.2f} over {len(big)} bins  | {top}')
        h = fh[side]; nh = h[True] + h[False]
        print(f'{name}: first hits {nh}' + (f', diving 5 s later {100 * h[True] / nh:.0f}%' if nh else ' (none, or the track has no hit events)'))

if __name__ == '__main__':
    main(sys.argv[1:] or ['.'])
