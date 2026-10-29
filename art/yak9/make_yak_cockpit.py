#!/usr/bin/env python3
"""MA-RED-ALL-1 (e): Yak-9 cockpit maps, derived from the F-51 cockpit (Rowan's Yak plane type uses CPTF51).

Pass 2: the airspeed dial (CPTF51D5) reads km/h: the mph numerals, ticks and range marks are erased and km/h ticks with
Soviet-style numerals 1..7 (x100 km/h) are drawn at the angles where the F-51 needle points for that speed.
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


def fill(px, w, hole):
    """Repaint the pixels in hole, outside in, with the per-channel median of the known pixels within 2 px."""
    for _ in range(60):
        if not hole: break
        done = {}
        for (x, y) in hole:
            nb = [px[(y + dy) * w + x + dx] for dx in range(-2, 3) for dy in range(-2, 3)
                  if (x + dx, y + dy) not in hole and 0 <= x + dx < w and 0 <= y + dy < w and px[(y + dy) * w + x + dx][3]]
            if len(nb) >= 4:
                done[(x, y)] = tuple(sorted(c[i] for c in nb)[len(nb) // 2] for i in range(3)) + (255,)
        if not done: break
        for (x, y), c in done.items(): px[y * w + x] = c
        hole -= set(done)


# F-51 ASI face: needle angle (deg clockwise from 12 o'clock) at mph, read off the dial's own ticks and numerals
ASI_MPH = [(50, 0), (150, 90), (250, 180), (300, 217), (400, 260), (500, 300), (700, 330)]
ASI_C, ASI_RIM = (63.5, 63.5), 55.5


def asi_angle(mph):
    for (v0, a0), (v1, a1) in zip(ASI_MPH, ASI_MPH[1:]):
        if mph <= v1: return a0 + (a1 - a0) * (max(mph, v0) - v0) / (v1 - v0)
    return ASI_MPH[-1][1]


def asi_kmh(px, w):
    from PIL import ImageDraw, ImageFont
    import math
    cx, cy = ASI_C
    hole = set()
    for y in range(w):
        for x in range(w):
            if math.hypot(x - cx, y - cy) > ASI_RIM or not px[y * w + x][3]: continue
            l, s = lum_sat(px[y * w + x][:3])
            if l > (0.30 if math.hypot(x - cx, y - cy) > 46 else 0.42) or (s > 0.25 and l > 0.15): hole.add((x, y))
    hole |= {(x + dx, y + dy) for (x, y) in hole for dx in (-1, 0, 1) for dy in (-1, 0, 1)
             if math.hypot(x + dx - cx, y + dy - cy) <= ASI_RIM}     # and the glyphs' dark outlines
    fill(px, w, hole)
    S = 4; ov = Image.new('RGBA', (w * S, w * S)); d = ImageDraw.Draw(ov)
    ink = (232, 232, 232, 255)
    fnt = ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed-Bold.ttf', 13 * S)
    small = ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed-Bold.ttf', 7 * S)
    def pol(r, a): return ((cx + 0.5 + r * math.sin(math.radians(a))) * S, (cy + 0.5 - r * math.cos(math.radians(a))) * S)
    for kmh in range(100, 701, 50):
        a = asi_angle(kmh / 1.609344); major = kmh % 100 == 0
        d.line([pol(ASI_RIM - (8 if major else 4.5), a), pol(ASI_RIM - 0.5, a)], fill=ink, width=(2 if major else 1) * S)
        if major: d.text(pol(ASI_RIM - 17, a), str(kmh // 100), font=fnt, fill=ink, anchor='mm')
    d.text(((cx + 0.5) * S, (cy + 0.5 + 16) * S), 'км/ч', font=small, fill=ink, anchor='mm')
    ov = ov.resize((w, w), Image.LANCZOS).load()
    for y in range(w):
        for x in range(w):
            r, g, b, a = ov[x, y]; o = px[y * w + x]
            if a and o[3]:
                k = a / 255; px[y * w + x] = tuple(int(o[i] * (1 - k) + ink[i] * k) for i in range(3)) + (255,)


DIALS = {'CPTF51D5.X8': asi_kmh}


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
        if sub == 'COCKPITS' and u in DIALS: DIALS[u](px, w)
        im = Image.new('RGBA', (w, h)); im.putdata(px)
        tmp = os.path.join(dst, f + '.png'); im.save(tmp)
        sys.argv = ['x', 'fromx8', tmp, os.path.join(src, f), os.path.join(dst, u)]
        ma_x8.main(); os.unlink(tmp)
        print(sub, u)
