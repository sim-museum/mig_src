#!/usr/bin/env python3
"""PO-27: measure how 'tiled' a campaign-map capture is.  See port/map_zoom.sh."""
import sys, statistics
from PIL import Image

cap, tag, log = sys.argv[1], sys.argv[2], (sys.argv[3] if len(sys.argv) > 3 else None)
im = Image.open(cap).convert('RGB')
W, H = im.size
# map area: below the toolbars (48 px) and clear of the left panel; trimmed from the right so the
# scale bar / ruler cannot enter the statistic.
X0, Y0, X1, Y1 = 520, 60, W - 40, H - 40
px = im.load()

same = tot = 0
for y in range(Y0, Y1, 2):
    for x in range(X0, X1 - 1):
        tot += 1
        if px[x, y] == px[x + 1, y]:
            same += 1
coarse = same / tot if tot else 0.0

# per-column mean |dRGB| across the column boundary (x-1 -> x)
cols = []
for x in range(X0 + 1, X1):
    s = 0
    n = 0
    for y in range(Y0, Y1, 4):
        a = px[x - 1, y]; b = px[x, y]
        s += abs(a[0]-b[0]) + abs(a[1]-b[1]) + abs(a[2]-b[2])
        n += 1
    cols.append((x, s / n))
med = statistics.median(v for _, v in cols)
hi = sorted(cols, key=lambda t: -t[1])[:6]

# tile pitch, if the run said one
pitch = None
if log:
    for line in open(log, 'rb').read().decode('latin-1').splitlines():
        if '[maptile] client=' in line and 'tile=' in line:
            pitch = int(line.split('tile=')[1].split()[0])
print("    coarse(identical h-neighbours) %.3f   column |dRGB| median %.2f" % (coarse, med))
print("    strongest columns: " + ", ".join("x=%d %.1f" % (x, v) for x, v in hi))
if pitch:
    at = [(x, v) for x, v in cols if (x - X0) % pitch < 2]
    print("    tile pitch %d px -> %d boundary columns in the window" % (pitch, len(at)))
