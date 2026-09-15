#!/usr/bin/env python3
"""GOLD3D-1 S7: sky/terrain band statistics for 3D-view parity, gold against port.

Why bands rather than a whole-frame diff: `port/ab.sh`'s RMSE compares two frames taken at
different moments of different flights, which S6 showed is not a fidelity measurement at all. Band
means and spreads are at least comparable across frames -- PROVIDED the two captures are in the same
flight state, which is the thing to check FIRST (see the noise floor this script prints).

    port/tools/gold3d_bands.py <image> [<image> ...]

Each image is reported as:
    sky      = rows 5%..15%   of the frame
    terrain  = rows 55%..85%
Both as mean RGB and mean per-channel standard deviation.

READ THE NOISE FLOOR BEFORE BELIEVING A DIFFERENCE. Measured on the gold
(~/gold standard/ma/260814_mig_complete_campaign.mp4, 8 frames 5 s apart during ONE 3D sequence):
    sky mean R    92.7 .. 126.7   (a 37% swing)
    terrain mean R 28.6 ..  72.7  (a 2.5x swing)
A parity claim smaller than that is measuring the moment, not the port.
"""
import sys
import numpy as np
from PIL import Image


def bands(path):
    a = np.asarray(Image.open(path).convert('RGB')).astype(float)
    h = a.shape[0]
    sky = a[int(h * 0.05):int(h * 0.15)].reshape(-1, 3)
    ter = a[int(h * 0.55):int(h * 0.85)].reshape(-1, 3)
    return sky.mean(axis=0), sky.std(axis=0).mean(), ter.mean(axis=0), ter.std(axis=0).mean()


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 2
    print("%-28s %-22s %7s %-22s %7s" % ("capture", "sky mean RGB", "sky sd", "terrain mean RGB", "ter sd"))
    for p in argv[1:]:
        try:
            sm, ss, tm, ts = bands(p)
        except Exception as e:
            print("%-28s  !! %s" % (p.split('/')[-1], e))
            continue
        print("%-28s %-22s %7.1f %-22s %7.1f" % (
            p.split('/')[-1],
            "%6.1f %6.1f %6.1f" % tuple(sm), ss,
            "%6.1f %6.1f %6.1f" % tuple(tm), ts))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
