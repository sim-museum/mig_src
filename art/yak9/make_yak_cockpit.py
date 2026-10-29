#!/usr/bin/env python3
"""MA-RED-ALL-1 (e): Yak-9 cockpit maps, derived from the F-51 cockpit (Rowan's Yak plane type uses CPTF51).

Pass 1: the grey interior and panel take the Soviet blue-grey of Yak cockpits (dial faces stay black), and the US
"NO HAND HOLD" stencil is erased from the gunsight crash pad. Writes same-named .X8 files to OUT/COCKPITS and
OUT/COCKMASK; the game opens them from COCKPITS/YAK9/ and COCKMASK/YAK9/ when the player flies a Yak.
  make_yak_cockpit.py GAME_DIR [OUT]        (GAME_DIR = the rowan/mig data directory)
"""
import os, sys, colorsys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'tools'))
import ma_x8
from PIL import Image

game = sys.argv[1]
out = sys.argv[2] if len(sys.argv) > 2 else os.path.join(os.path.dirname(os.path.abspath(__file__)), 'out')
pal = ma_x8.palette()
TINT = (0.92, 1.02, 1.12)          # blue-grey
STENCIL = {'CPTF51HD.X8': (60, 30, 220, 118)}   # "NO HAND HOLD" box on the 256x256 pad


def lum_sat(rgb):
    _, l, s = colorsys.rgb_to_hls(*(c / 255 for c in rgb)); return l, s


def erase(px, w, box):
    """Paint light stencil pixels inside box from their dark neighbours (repeated 3x3 averaging)."""
    x0, y0, x1, y1 = box
    hole = {(x, y) for y in range(y0, y1) for x in range(x0, x1)
            if px[y * w + x][3] and lum_sat(px[y * w + x][:3])[0] > 0.30}
    for _ in range(40):
        if not hole: break
        nxt = set()
        for (x, y) in hole:
            nb = [px[(y + dy) * w + x + dx] for dx in (-1, 0, 1) for dy in (-1, 0, 1)
                  if (x + dx, y + dy) not in hole and 0 <= x + dx < w and 0 <= y + dy < w and px[(y + dy) * w + x + dx][3]]
            if len(nb) >= 2:
                px[y * w + x] = tuple(sum(c[i] for c in nb) // len(nb) for i in range(3)) + (255,)
            else:
                nxt.add((x, y))
        hole = nxt


for sub in ('COCKPITS', 'COCKMASK'):
    src = os.path.join(game, sub); dst = os.path.join(out, sub); os.makedirs(dst, exist_ok=True)
    for f in sorted(os.listdir(src)):
        u = f.upper()
        if not (u.startswith('CPTF51') or u.startswith('CPITF51')) or not u.endswith('.X8'):
            continue
        raw, hdr, w, h, pix = ma_x8.read(os.path.join(src, f))
        px = []
        for i in pix:
            if i == 254: px.append((0, 0, 0, 0)); continue
            r, g, b = pal[i]; l, s = lum_sat((r, g, b))
            if s < 0.25 and l > 0.12:  # grey metal (not the black dial faces, not coloured markings)
                r, g, b = (min(255, int(c * t)) for c, t in zip((r, g, b), TINT))
            px.append((r, g, b, 255))
        if u in STENCIL: erase(px, w, STENCIL[u])
        im = Image.new('RGBA', (w, h)); im.putdata(px)
        tmp = os.path.join(dst, f + '.png'); im.save(tmp)
        sys.argv = ['x', 'fromx8', tmp, os.path.join(src, f), os.path.join(dst, u)]
        ma_x8.main(); os.unlink(tmp)
        print(sub, u)
