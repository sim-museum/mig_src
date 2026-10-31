#!/usr/bin/env python3
"""MA-RED-ALL-1 (e): Yak-9 cockpit maps, derived from the F-51 cockpit (Rowan's Yak plane type uses CPTF51).

Pass 3: own palette (see YAK_SLOT): leather coaming, no teal, blue oxygen dial, ПОДЪЕМ for CLIMB; writes LANDMAPS/ and
LANDMAP2/PALCLUST.X8 too, which must ship with the maps.
Pass 2: the airspeed dial (CPTF51D5) reads km/h: the mph numerals, ticks and range marks are erased and km/h ticks with
Soviet-style numerals 10..80 (x10 km/h, as on the Yak-3/9 СКОРОСТЬ dial) are drawn at the angles where the F-51 needle points for that speed.
Pass 1: grey metal re-coloured (see shade()), and the US
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
# Grey metal per map, after the PO's Yak-3 cockpit reference (261028): black panel with a faint grain, light neutral
# grey side walls, neutral dial bezels. (Pass 1 tinted everything blue-grey.)
PANEL, WALL = 0.55, 2.0             # brightness factors (the palette's light greys are its mauve ramp; no neutral)
def shade(u):
    if u.startswith('CPITF51') or u == 'CPTF51C.X8': return PANEL
    if u in ('CPTF51BT.X8', 'CPTF51LH.X8'): return WALL
    return 1.0
STENCIL = {'CPTF51HD.X8': (60, 30, 220, 118)}   # "NO HAND HOLD" box on the 256x256 pad


def lum_sat(rgb):
    _, l, s = colorsys.rgb_to_hls(*(c / 255 for c in rgb)); return l, s


def erase(px, w, box, light=0.30):
    """Paint light stencil pixels inside box from their dark neighbours (repeated 3x3 averaging)."""
    x0, y0, x1, y1 = box
    hole = {(x, y) for y in range(y0, y1) for x in range(x0, x1)
            if px[y * w + x][3] and lum_sat(px[y * w + x][:3])[0] > light}
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
    lab = ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed-Bold.ttf', 10 * S)
    for kmh in range(100, 801, 10):     # Soviet ASI (Yak-3/9): x10 km/h, numerals every 50 to 400, then every 100
        a = asi_angle(kmh / 1.609344)
        num = kmh % 50 == 0 if kmh <= 400 else kmh % 100 == 0
        if kmh > 400 and kmh % 50: continue
        if not num and kmh % 50 and kmh > 300: continue
        d.line([pol(ASI_RIM - (7 if num else 3.5), a), pol(ASI_RIM - 0.5, a)], fill=ink, width=(2 if num else 1) * S)
        if num: d.text(pol(ASI_RIM - 15, a), str(kmh // 10), font=lab, fill=ink, anchor='mm')
    d.text(((cx + 0.5) * S, (cy + 0.5 - 13) * S), 'СКОРОСТЬ', font=small, fill=ink, anchor='mm')
    d.text(((cx + 0.5) * S, (cy + 0.5 + 14) * S), 'x10 км/ч', font=small, fill=ink, anchor='mm')
    ov = ov.resize((w, w), Image.LANCZOS).load()
    for y in range(w):
        for x in range(w):
            r, g, b, a = ov[x, y]; o = px[y * w + x]
            if a and o[3]:
                k = a / 255; px[y * w + x] = tuple(int(o[i] * (1 - k) + ink[i] * k) for i in range(3)) + (255,)


def ink(px, w, h, draw, colour=(232, 232, 232)):
    """Draw with draw(ImageDraw, S) at 4x on an overlay and blend it over the opaque pixels."""
    from PIL import ImageDraw
    S = 4; ov = Image.new('RGBA', (w * S, h * S)); draw(ImageDraw.Draw(ov), S)
    ov = ov.resize((w, h), Image.LANCZOS).load()
    for y in range(h):
        for x in range(w):
            a = ov[x, y][3]; o = px[y * w + x]
            if a and o[3]:
                k = a / 255; px[y * w + x] = tuple(int(o[i] * (1 - k) + colour[i] * k) for i in range(3)) + (255,)


def font(size):
    from PIL import ImageFont
    return ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed-Bold.ttf', size)


def relabel(box, text, size, at):
    """Erase the white English word in box and write text (as on the Yak-3R panel) centred at at."""
    def f(px, w):
        erase(px, w, box, 0.20)
        if text: ink(px, w, w, lambda d, S: d.text((at[0] * S, at[1] * S), text, font=font(size * S), fill=(255, 255, 255, 255), anchor='mm'))
    return f


DIALS = {'CPTF51D5.X8': asi_kmh,
         'CPTF5113.X8': relabel((40, 42, 86, 64), 'ПОДЪЕМ', 9, (64, 53)),     # vertical speed: "CLIMB"
         'CPTF5115.X8': relabel((44, 40, 86, 60), 'ПОДЪЕМ', 9, (64, 50)),
         'CPTF5112.X8': relabel((40, 104, 90, 112), '', 1, (64, 108))}      # turn needle: "4 MIN TURN" goes


# Pass 3 (MA-RED-YAK-COCKPIT-3, PO 10-31 "as much like this Yak-3R cockpit as possible"): the Yak maps get their own
# palette, so they can have the colours the game palette lacks (leather browns, neutral greys, the oxygen-dial blue).
# A CMAP chunk of YAK_SLOT bytes makes the game use texture palette YAK_SLOT, which is file palette 4*YAK_SLOT of
# LANDMAPS/PALCLUST.X8; slots 2-6 are unused copies of slot 0, so no other image changes.
YAK_SLOT = 2
KEEP = (253, 254)      # FONTMASK, transparent: kept as they are, with slot 0's colours


def leather(px, w, h):
    """The coaming rolls (CPTF51LH): matte tan leather with the dark diagonal wrap of the Yak-3R."""
    lo, mid, hi = (52, 33, 20), (128, 86, 52), (176, 132, 90)
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[y * w + x]
            if not a: continue
            l = 0.2 + 0.6 * ((r + g + b) / 765) ** 0.8          # matte: highlights pulled down, shadows lifted
            c = [lo[i] + (mid[i] - lo[i]) * l * 2 for i in range(3)] if l < 0.5 else \
                [mid[i] + (hi[i] - mid[i]) * (l - 0.5) * 2 for i in range(3)]
            if (x + 0.45 * y) % 6 < 1.3: c = [v * 0.62 for v in c]   # wrap bands
            px[y * w + x] = tuple(int(v) for v in c) + (255,)


def unteal(px):
    """F-51 teal and its dark green shading (hue ~140-200 deg) go neutral grey, as nothing in the Yak-3R is that colour;
    the purple-tinted dark greys of the panel go neutral black (the Yak-3R panel black)."""
    for i, (r, g, b, a) in enumerate(px):
        if not a: continue
        hh, l, s = colorsys.rgb_to_hls(r / 255, g / 255, b / 255)
        if (0.38 < hh < 0.56 and s > 0.25 and l > 0.10) or (0.72 < hh < 0.92 and l < 0.20):
            v = int(255 * l * (0.8 if l > 0.20 else 1.0)); px[i] = (v, v, v, 255)


def oxygen_blue(px, w):
    """Dark dial face of CPTF5116 painted the bright blue of the Yak-3R oxygen dials (КИСЛОРОД), numerals stay white."""
    import math
    for y in range(w):
        for x in range(w):
            r, g, b, a = px[y * w + x]
            if a and math.hypot(x - 63.5, y - 63.5) < 50 and (r + g + b) / 765 < 0.22:
                k = 0.75 + 2.5 * (r + g + b) / 765
                px[y * w + x] = tuple(min(255, int(c * k)) for c in (34, 96, 186)) + (255,)


maps = []
for sub in ('COCKPITS', 'COCKMASK'):
    src = os.path.join(game, sub)
    for f in sorted(os.listdir(src)):
        u = f.upper()
        if not (u.startswith('CPTF51') or u.startswith('CPITF51')) or not u.endswith('.X8'):
            continue
        raw, hdr, w, h, pix = ma_x8.read(os.path.join(src, f))
        px = []; k = shade(u)
        for i in pix:
            if i == 254: px.append((0, 0, 0, 0)); continue
            r, g, b = pal[i]; l, s = lum_sat((r, g, b))
            if s < 0.25 and l > (0.02 if k == WALL else 0.12):  # grey metal (not the black dial faces, not coloured markings): neutral, re-lit
                v = min(255, int((r + g + b) / 3 * k)); r, g, b = v, v, v
            px.append((r, g, b, 255))
        if u in STENCIL: erase(px, w, STENCIL[u])
        if u in DIALS: DIALS[u](px, w)
        unteal(px)
        if u == 'CPTF51LH.X8': leather(px, w, h)
        if u == 'CPTF5116.X8': oxygen_blue(px, w)
        maps.append((sub, u, hdr, w, h, pix, px))

# One 250-colour palette for all the Yak maps (median cut over every opaque pixel), at indices 1..250
import numpy as np
allpx = np.array([p[:3] for m in maps for p, i in zip(m[6], m[5]) if p[3] and i not in KEEP], np.uint8)
q = Image.fromarray(allpx.reshape(1, -1, 3)).quantize(250, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
cols = np.array(q.getpalette()[:750], np.int32).reshape(-1, 3)
for sub, u, hdr, w, h, pix, px in maps:
    a = np.array(px, np.int32)
    idx = np.empty(len(px), np.int64)
    for o in range(0, len(px), 8192):
        d = ((a[o:o + 8192, None, :3] - cols[None]) ** 2).sum(2); idx[o:o + 8192] = d.argmin(1) + 1
    outpix = bytes(i if i in KEEP else int(j) for i, j in zip(pix, idx))
    dst = os.path.join(out, sub); os.makedirs(dst, exist_ok=True)
    open(os.path.join(dst, u), 'wb').write(ma_x8.write(hdr, w, h, outpix, YAK_SLOT))
    print(sub, u)

# PALCLUST.X8 with the Yak palette in file palettes 4*YAK_SLOT..+3 (the game shows file bytes r,g,b as g,b,r: store b,r,g)
for d in ('LANDMAPS', 'LANDMAP2'):
    name = [f for f in os.listdir(os.path.join(game, d)) if f.upper() == 'PALCLUST.X8'][0]
    b = bytearray(open(os.path.join(game, d, name), 'rb').read())
    o = [o for tag, o, n in ma_x8.chunks(b, 12, len(b)) if tag == b'CMAP'][0]
    base = bytes(b[o:o + 768])
    yak = bytearray(base)
    for i, (r, g, bb) in enumerate(cols.tolist()): yak[3 * (i + 1):3 * (i + 2)] = bytes((bb, r, g))
    for p in range(4 * YAK_SLOT, 4 * YAK_SLOT + 4): b[o + 768 * p:o + 768 * (p + 1)] = yak
    os.makedirs(os.path.join(out, d), exist_ok=True)
    open(os.path.join(out, d, name), 'wb').write(b)
    print(d, name)
