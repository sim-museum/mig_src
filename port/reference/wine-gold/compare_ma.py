#!/usr/bin/env python3
"""compare_ma.py A B OUT.png -- mean |diff| on MA's 1200x1080 external view, by region, plus a side-by-side."""
import sys; from PIL import Image, ImageChops
a=Image.open(sys.argv[1]).convert('RGB'); g=Image.open(sys.argv[2]).convert('RGB')
if a.size!=g.size: a=a.resize(g.size, Image.BILINEAR); print(f"note: A resized to {g.size}")
d=ImageChops.difference(a,g).convert('L'); W,H=g.size
def md(box):
    h=d.crop(box).histogram(); n=sum(h); return sum(i*c for i,c in enumerate(h))/n
R={'whole':(0,0,W,H),'sky':(0,0,W,int(H*.22)),'haze/horizon':(0,int(H*.22),W,int(H*.45)),
   'aircraft box':(int(W*.3),int(H*.28),int(W*.72),int(H*.66)),'terrain':(0,int(H*.66),W,int(H*.96)),'HUD strip':(0,int(H*.96),W,H)}
for k,b in R.items(): print(f"  {k:13s} mean|diff| {md(b):6.2f}")
s=Image.new('RGB',(W*2,H)); s.paste(a,(0,0)); s.paste(g,(W,0)); s.save(sys.argv[3]); print("side-by-side ->",sys.argv[3])
