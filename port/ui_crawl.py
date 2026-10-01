#!/usr/bin/env python3
"""MA UI crawler (FUNC-SWEEP-MA): click every clickable control reachable from the title screen, each in a
FRESH headless launch, and record crash / quit / which screen it led to.

Discovery uses the game's own control table: MA_DUMP_MENU prints every clickable control (and every list
row) each time the visible set changes, so the LAST dump in a log is the screen the clicks led to.
Clicks: BOB_CLICKSEQ="ms,x,y;..." with BOB_CLICKSEQ_MS=1. Depth 1 = every target on the title screen;
depth 2/3 = every target on each new screen a click opened.

Runs on a DISPOSABLE data copy (default ~/ma-crawl/drive_c/rowan/mig): clicking Delete/Save is the point.
Usage: port/ui_crawl.py [--gd DIR] [--out DIR] [--max-runs N] [--depth N]
"""
import argparse, os, re, subprocess, time

ap = argparse.ArgumentParser()
ap.add_argument('--gd', default=os.path.expanduser('~/ma-crawl/drive_c/rowan/mig'))
ap.add_argument('--bin', default=os.path.expanduser('~/ma/build/wmig'))
ap.add_argument('--out', default=os.path.expanduser('~/ma-gates/crawl'))
ap.add_argument('--max-runs', type=int, default=500)
ap.add_argument('--depth', type=int, default=3)
A = ap.parse_args()
os.makedirs(A.out + '/runs', exist_ok=True)
T0, STEP, SETTLE = 9000, 6000, 6000      # ms: first click, between clicks, after the last click
nruns = 0

def parse_last_dump(txt):
    i = txt.rfind('[menu] ----')
    if i < 0: return None
    targets, cur = [], None
    for ln in txt[i:].split('\n')[1:]:
        if not ln.startswith('[menu]'): break
        m = re.match(r'\[menu\] #(-?\d+)@(\S*)\s+rect\((-?\d+),(-?\d+) (\d+)x(\d+)\)\s+centre\((-?\d+),(-?\d+)\)\s+type=(\d+)\s+"(.*)"', ln)
        if m:
            cid, cls, x, y, w, h, cx, cy, typ, cap = m.groups()
            cur = dict(id=int(cid), cls=cls, x=int(x), y=int(y), w=int(w), h=int(h), cx=int(cx), cy=int(cy), type=int(typ), cap=cap, rows=[])
            targets.append(cur); continue
        m = re.match(r'\[menu\]\s+row (\d+): \[\d+\] "(.*)"', ln)
        if m and cur is not None: cur['rows'].append(m.group(2))
    out = []
    for t in targets:
        # rows: the panel's own front menu takes the game's "rN" token (pixel row maths missed
        # every row -- measured: "Quit" at the computed y left the game running); a hosted list takes
        # "#ID:rN". Plain controls click their dumped centre (the same rect ma_ole_click tests).
        if t['rows']:
            for k, r in enumerate(t['rows']):
                tok = ('r%d' % k) if 'RFullPanelDial' in t['cls'] else ('#%d:r%d' % (t['id'], k))
                out.append(('#%d:row%d "%s"' % (t['id'], k, r[:24]), tok))
        else:
            out.append(('#%d "%s"' % (t['id'], t['cap'][:24]), '%d,%d' % (t['cx'], t['cy'])))
    return out

def run(clicks, tag):
    global nruns
    nruns += 1
    seq = ';'.join('%d,%s' % (T0 + i * STEP, tok) for i, tok in enumerate(clicks))
    secs = (T0 + max(0, len(clicks) - 1) * STEP + SETTLE) / 1000.0 + 2
    log = '%s/runs/%s.log' % (A.out, tag)
    env = dict(os.environ, BOB_DRIVE_C=A.gd.rsplit('/rowan/mig', 1)[0], SDL_VIDEODRIVER='dummy',
               MA_DUMP_MENU='1', BOB_CLICKSEQ_MS='1')
    if seq: env['BOB_CLICKSEQ'] = seq
    with open(log, 'wb') as f:
        p = subprocess.run(['timeout', '-k', '5', '-s', 'INT', '%.0f' % secs, A.bin], cwd=A.gd, env=env,
                           stdout=f, stderr=subprocess.STDOUT)
    txt = open(log, 'rb').read().decode('latin-1')
    stalled = txt.count('STALLED on entry')
    crash = txt.count('=== CRASH') + txt.count('Segmentation fault') + txt.count('SIGSEGV') + txt.count('Aborted')
    state = 'CRASH' if crash else ('alive' if p.returncode in (124, 130, -2, 137) else 'exited(%d)' % p.returncode)
    if stalled and state == 'alive': state = 'alive-stalled'
    return state, parse_last_dump(txt), log

tsv = open(A.out + '/crawl.tsv', 'a')
def rec(path, state, before, after, log):
    sig = lambda s: tuple(sorted(n for n, _ in s)) if s else ()
    changed = 'newscreen' if after is not None and sig(after) != sig(before) else 'same'
    tsv.write('\t'.join([path, state, changed, str(len(after or [])), os.path.basename(log)]) + '\n'); tsv.flush()
    print('%-70s %-10s %-9s targets=%d' % (path[:70], state, changed, len(after or [])), flush=True)
    return changed == 'newscreen'

state, title, log = run([], 'title')
print('title targets:', [n for n, _ in title or []], flush=True)
frontier = [([], title, 'title')]
seen_screens = {tuple(sorted(n for n, _ in title))}
for depth in range(A.depth):
    nxt = []
    for clicks, targets, pdesc in frontier:
        for (name, tok) in targets or []:
            if nruns >= A.max_runs: break
            path = clicks + [tok]
            tag = re.sub(r'[^A-Za-z0-9]+', '_', ('%s_%s' % (pdesc, name)))[-120:]
            st, after, lg = run(path, tag)
            new = rec('%s > %s' % (pdesc, name), st, targets, after, lg)
            if st == 'alive' and new and after:
                key = tuple(sorted(n for n, _ in after))
                if key not in seen_screens:
                    seen_screens.add(key); nxt.append((path, after, '%s > %s' % (pdesc, name)))
    frontier = nxt
print('=== CRAWL COMPLETE runs=%d screens=%d ===' % (nruns, len(seen_screens)), flush=True)
