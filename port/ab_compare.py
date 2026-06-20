#!/usr/bin/env python3
"""B2 A/B compare: native software-rasterizer frame vs Wine pixel-oracle frame.

Takes a native capture (PPM/PNG, 640x480 from MA_DUMP_BACK) and a Wine reference
PNG (window capture, any size), normalizes both to 640x480, and emits:
  - <out>_native.png   the native frame
  - <out>_wine.png     the reference, cropped+resized to 640x480
  - <out>_sidebyside.png   native | wine | abs-diff heatmap
  - prints pixel-difference stats (RMSE, mean abs, %changed, per-channel mean)

The Wine captures are window grabs that include some chrome/border and the game's
known horizontal-offset glitch, so PIXEL-EXACT match is NOT expected -- these stats
are a *structural* signal (does the native frame have the same gross content: sky
band, horizon line, terrain mass, cockpit framing). Use --crop L,T,R,B to trim the
window chrome from the Wine grab before resizing (fractions 0..1 or px).

Usage:
  ab_compare.py NATIVE.ppm WINE.png OUT_PREFIX [--crop L,T,R,B] [--wine-only]
"""
import sys, os
import numpy as np
from PIL import Image

W, H = 640, 480


def load_any(path):
    return Image.open(path).convert("RGB")


def parse_crop(s, w, h):
    """L,T,R,B as fractions (<=1.0) or pixels -> (left, top, right, bottom) px box."""
    parts = [float(x) for x in s.split(",")]
    if len(parts) != 4:
        raise SystemExit("--crop needs L,T,R,B")
    l, t, r, b = parts
    def px(v, dim):
        return int(round(v * dim)) if 0.0 <= v <= 1.0 else int(round(v))
    return (px(l, w), px(t, h), w - px(r, w), h - px(b, h))


def prep_wine(img, crop):
    if crop:
        img = img.crop(parse_crop(crop, img.width, img.height))
    return img.resize((W, H), Image.BILINEAR)


def stats(a, b):
    a = a.astype(np.int32); b = b.astype(np.int32)
    d = np.abs(a - b)
    rmse = float(np.sqrt(np.mean((a - b) ** 2)))
    mae = float(d.mean())
    # "changed" = any channel differing by > 24 (8-bit), tolerant of palette/quantize
    changed = float((d.max(axis=2) > 24).mean()) * 100.0
    per = d.reshape(-1, 3).mean(axis=0)
    return rmse, mae, changed, per


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    opts = [a for a in sys.argv[1:] if a.startswith("--")]
    crop = None
    wine_only = "--wine-only" in opts
    for o in opts:
        if o.startswith("--crop="):
            crop = o.split("=", 1)[1]
    if len(args) < 3:
        raise SystemExit(__doc__)
    native_path, wine_path, out = args[0], args[1], args[2]

    wine = prep_wine(load_any(wine_path), crop)
    wine.save(f"{out}_wine.png")

    if wine_only or not os.path.exists(native_path):
        print(f"[wine-only] wrote {out}_wine.png ({W}x{H})")
        return

    native = load_any(native_path).resize((W, H), Image.NEAREST)
    native.save(f"{out}_native.png")

    na = np.asarray(native); wa = np.asarray(wine)
    diff = np.abs(na.astype(np.int32) - wa.astype(np.int32)).astype(np.uint8)
    Image.fromarray(diff).save(f"{out}_diff.png")

    # side-by-side: native | wine | diff
    sbs = Image.new("RGB", (W * 3 + 8, H), (32, 32, 32))
    sbs.paste(native, (0, 0))
    sbs.paste(wine, (W + 4, 0))
    sbs.paste(Image.fromarray(diff), (W * 2 + 8, 0))
    sbs.save(f"{out}_sidebyside.png")

    rmse, mae, changed, per = stats(na, wa)
    print(f"  native : {native_path}")
    print(f"  wine   : {wine_path}  crop={crop or 'none'}")
    print(f"  RMSE        : {rmse:6.2f}   (0=identical, 255=max)")
    print(f"  mean |diff| : {mae:6.2f}")
    print(f"  changed >24 : {changed:5.1f}%")
    print(f"  per-chan |d|: R={per[0]:.1f} G={per[1]:.1f} B={per[2]:.1f}")
    print(f"  -> {out}_sidebyside.png  (native | wine | diff)")


if __name__ == "__main__":
    main()
