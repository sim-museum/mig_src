#!/usr/bin/env python3
"""MA-RED-LOG-1: the Red side's panel title bar. Rowan's AXART/titleb.bmp (550x28) opens with a US star (white star on a
navy disc). Red: the colours swapped round -- a red star on a white disc, as on the Soviet/Chinese/North Korean MiGs.
Each disc pixel is recoloured and mapped to the nearest colour of the bar's own palette (no new colours).
Usage: make_titleb_red.py <titleb.bmp> <outdir>"""
import sys, os
from PIL import Image
im = Image.open(sys.argv[1]); pal = im.getpalette()[:768]; cols = [tuple(pal[i:i + 3]) for i in range(0, 768, 3)]
def nearest(c): return min(range(256), key=lambda k: sum((a - b) ** 2 for a, b in zip(cols[k], c)))
cx, cy, r = 15.0, 14.0, 13.5
out = im.copy()
for y in range(im.size[1]):
    for x in range(32):
        if (x - cx) ** 2 + (y - cy) ** 2 > r * r: continue
        R, G, B = cols[im.getpixel((x, y))]; lum = (R + G + B) / 3
        t = min(1.0, max(0.0, (lum - 50) / (235 - 50)))          # 0 = navy disc, 1 = white star
        star, disc = (226, 5, 12), (239, 231, 247)
        out.putpixel((x, y), nearest(tuple(int(disc[i] + (star[i] - disc[i]) * t) for i in range(3))))
os.makedirs(f'{sys.argv[2]}/AXART', exist_ok=True)
out.save(f'{sys.argv[2]}/AXART/titleb_red.bmp'); print(f'{sys.argv[2]}/AXART/titleb_red.bmp')
