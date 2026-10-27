#!/usr/bin/env python3
"""MA-P5: open every combo on one Preferences tab and pick a dropdown row, headless, on a disposable data copy.
Combos are NOT in MA_DUMP_MENU's target list (it lists their labels, type=2 statics), so the UI crawler never
reached them; this finds the boxes from MA_TRACE_CLICK's "[click] combo box=" lines after a probe click.
Usage: tools/ma_prefs_combo_pick.py <tab row 0-6> [offset px below the box: 12 = row 0, 30 = row 1/2] [outdir]"""
import os, re, subprocess, sys
tab = int(sys.argv[1]); off = int(sys.argv[2]) if len(sys.argv) > 2 else 30
out = sys.argv[3] if len(sys.argv) > 3 else os.path.expanduser('~/ma-gates/prefs_combo_pick'); os.makedirs(out, exist_ok=True)
gd = os.path.expanduser('~/ma-crawl/drive_c/rowan/mig'); B = os.path.expanduser('~/ma/build/wmig')
def run(seq, log, secs):
    env = dict(os.environ, BOB_DRIVE_C=gd.rsplit('/rowan/mig', 1)[0], SDL_VIDEODRIVER='dummy', BOB_CLICKSEQ_MS='1',
               MA_TRACE_CLICK='1', BOB_CLICKSEQ=';'.join(seq))
    with open(log, 'wb') as f:
        r = subprocess.run(['timeout', '-k', '5', '-s', 'INT', str(secs), B], cwd=gd, env=env, stdout=f, stderr=subprocess.STDOUT)
    return r.returncode, open(log, 'rb').read().decode('latin-1')
pre = ['9000,r0', '15000,#2063:%d' % tab]
_, o = run(pre + ['20000,5,700'], '%s/tab%d_probe.log' % (out, tab), 30)
boxes = []
for m in re.finditer(r'\[click\] combo box=\((-?\d+),(-?\d+),(\d+),(\d+)\)', o):
    b = tuple(map(int, m.groups()))
    if b not in boxes: boxes.append(b)
seq, T = list(pre), 21000
for (x, y, w, h) in boxes:
    seq += ['%d,%d,%d' % (T, x + w // 2, y + h // 2), '%d,%d,%d' % (T + 1500, x + w // 2, y + h + off)]; T += 3000
rc, o = run(seq, '%s/tab%d_pick.log' % (out, tab), T // 1000 + 8)
crash = sum(o.count(s) for s in ('=== CRASH', 'SIGSEGV', 'Segmentation', 'Aborted'))
rows = re.findall(r'\[click\] dropdown row (\d+)', o)
print('tab%d boxes=%d rc=%d crash=%d opened=%d picked=%d rows=%s' % (tab, len(boxes), rc, crash, o.count('combo open dropdown'), len(rows), ''.join(rows)))
sys.exit(1 if crash else 0)
