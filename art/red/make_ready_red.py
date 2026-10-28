#!/usr/bin/env python3
"""MA-RED-ALL-1 (d): the Red ready-room background from the PD-China photo of Wang Hai (c. 1952), in the style of the
UN ready.bmp: colour photo of the pilot on the right, his aircraft's skin darkened on the left under the frag panels.
Hand-tinted by region (no ML colouriser): pale sky, olive-brown leather flying suit, natural-metal MiG, red stars and
side number. Writes 8-bit BMPs at 1280x1024, 1024x768 and 800x600.  Usage: make_ready_red.py <outdir>"""
import sys
from PIL import Image, ImageDraw, ImageFilter, ImageOps, ImageChops
src = Image.open(__file__.rsplit('/', 1)[0] + '/wang_hai_c1952.jpg').convert('L')   # 400x561
W, H = src.size

def tint(gray, dark, light):
    """map luminance 0..255 onto a colour ramp from `dark` to `light` (RGB)"""
    return ImageOps.colorize(gray, black=dark, white=light)

def mask(poly, blur=6):
    m = Image.new('L', (W, H), 0); ImageDraw.Draw(m).polygon(poly, fill=255); return m.filter(ImageFilter.GaussianBlur(blur))

col = tint(src, (40, 48, 58), (214, 226, 238))                    # sky / default: cool grey-blue
metal = tint(src, (34, 36, 40), (205, 208, 210))                   # natural-metal MiG
suit = tint(src, (30, 22, 14), (176, 150, 104))                    # leather flying suit, helmet
face = tint(src, (52, 30, 20), (226, 186, 150))
red = tint(src, (60, 6, 6), (230, 60, 50))
fuselage = mask([(0, 330), (60, 290), (400, 330), (400, 561), (0, 561)])
canopy = mask([(15, 330), (60, 285), (200, 280), (330, 300), (390, 380), (60, 370)], 4)
pilot = mask([(95, 260), (115, 160), (140, 120), (150, 95), (195, 92), (205, 150), (245, 200), (255, 270), (240, 330), (110, 330)], 5)
facem = mask([(140, 110), (182, 102), (186, 150), (150, 158)], 3)
stars = mask([(95, 375), (320, 395), (325, 470), (130, 470)], 3)
number = mask([(0, 455), (300, 455), (300, 561), (0, 561)], 2)
digits = ImageOps.invert(src).point(lambda v: 255 if v > 150 else 0).filter(ImageFilter.GaussianBlur(1))
col = Image.composite(metal, col, fuselage)
col = Image.composite(tint(src, (20, 28, 36), (170, 190, 200)), col, canopy)
col = Image.composite(suit, col, ImageChops.multiply(pilot, src.point(lambda v: 255 if v < 175 else 0).filter(ImageFilter.GaussianBlur(1.5))))	# only the figure, not the sky around it
col = Image.composite(face, col, facem)
starsel = ImageChops.multiply(stars, src.point(lambda v: 255 if v < 110 else 0).filter(ImageFilter.GaussianBlur(0.8)))	# the painted stars, not the metal between them
col = Image.composite(red, col, starsel)
col = Image.composite(red, col, ImageChops.multiply(number, digits))

def build(w, h):
    out = Image.new('RGB', (w, h))
    ph = h; pw = int(W * ph / H); photo = col.resize((pw, ph), Image.LANCZOS)
    x0 = w - pw
    # left: the MiG's own skin (the photo's fuselage band), stretched, blurred and darkened under the panels
    skin = col.crop((300, 300, W, 370)).resize((x0 + pw // 3, h), Image.LANCZOS).filter(ImageFilter.GaussianBlur(w / 160))
    out.paste(skin, (0, 0))
    grad = Image.linear_gradient('L').rotate(90, expand=True).resize((pw // 3, h))   # soft seam
    out.paste(photo, (x0, 0))
    seam = Image.composite(photo.crop((0, 0, pw // 3, h)), skin.crop((x0, 0, x0 + pw // 3, h)), ImageOps.invert(grad).point(lambda v: 255 - v))
    out.paste(seam, (x0, 0))
    dark = Image.new('RGB', (w, h), (6, 4, 4)); m = Image.new('L', (w, h), 0)
    ImageDraw.Draw(m).rectangle([0, 0, x0 + pw // 6, h], fill=150)                 # the UN art darkens its panel area too
    out = Image.composite(dark, out, m.filter(ImageFilter.GaussianBlur(w / 64)))
    return out.quantize(colors=236, method=Image.MEDIANCUT, dither=Image.FLOYDSTEINBERG)

outdir = sys.argv[1] if len(sys.argv) > 1 else '.'
for (w, h, d) in ((1280, 1024, 'DIAL1280'), (1024, 768, 'DIAL1024'), (800, 600, 'DIAL800')):
    import os; os.makedirs(f'{outdir}/{d}', exist_ok=True)
    build(w, h).save(f'{outdir}/{d}/ready_red.bmp')
    print(f'{outdir}/{d}/ready_red.bmp')
