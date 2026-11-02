#!/usr/bin/env python3
"""MA-RED-LOG-1: the Red Player Log photo from the PD-China photo of Wang Hai (c. 1952; SOURCE.txt), toned like the UN
WATER2/player.bmp (346x418, a dark blue-grey monochrome photo of a US pilot). Usage: make_player_red.py <outdir>"""
import sys, os
from PIL import Image, ImageOps
src = Image.open(__file__.rsplit('/', 1)[0] + '/wang_hai_c1952.jpg').convert('L')   # 400x561
W, H = 346, 418
crop = src.crop((0, 0, 400, round(400 * H / W)))                                  # head, shoulders and cockpit rim
photo = ImageOps.autocontrast(crop.resize((W, H), Image.LANCZOS), cutoff=1)
photo = photo.point(lambda v: int(255 * (v / 255) ** 2.6))                         # the bright sky down to the UN photo's dark tone (mean ~45)
# the UN photo's ramp, measured: luminance 0 -> (12,16,19), 32 -> (39,55,65), 64 -> (57,81,96), ~100 -> (74,104,125)
out = ImageOps.colorize(photo, black=(8, 11, 13), mid=(30, 42, 50), white=(60, 84, 101))
outdir = sys.argv[1] if len(sys.argv) > 1 else '.'
os.makedirs(f'{outdir}/WATER2', exist_ok=True)
out.quantize(colors=236, method=Image.MEDIANCUT, dither=Image.FLOYDSTEINBERG).save(f'{outdir}/WATER2/player_red.bmp')
print(f'{outdir}/WATER2/player_red.bmp')
