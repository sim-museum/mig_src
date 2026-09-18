#!/usr/bin/env python3
"""metrics_ma.py FRAME... -- attitude-independent metrics for MA's external view (1200x1080).
sky: mean RGB of rows 0-8%.  haze: luma at 20/30/40% height (centre column band).
terrain: mean RGB + mean saturation of rows 70-95%.  hud: mean RGB of bright text px in the bottom strip.
aircraft: mean RGB of grey/white object px in the central box (the F-86 is near-white)."""
import sys; from PIL import Image
def stats(px,W,H,box,pred=None):
    x0,y0,x1,y1=box; acc=[0,0,0]; n=0; sat=0
    for y in range(y0,y1,2):
        for x in range(x0,x1,2):
            p=px[x,y]
            if pred and not pred(p): continue
            acc=[acc[0]+p[0],acc[1]+p[1],acc[2]+p[2]]; n+=1; sat+=max(p)-min(p)
    return (tuple(a//max(n,1) for a in acc), n, sat//max(n,1))
for f in sys.argv[1:]:
    im=Image.open(f).convert('RGB'); px=im.load(); W,H=im.size
    sky=stats(px,W,H,(0,0,W,int(H*.08)))
    haze=[sum(px[W//2,int(H*k)])//3 for k in (0.20,0.30,0.40)]
    terr=stats(px,W,H,(0,int(H*.70),W,int(H*.95)))
    hud=stats(px,W,H,(0,int(H*.965),int(W*.6),H),lambda p:max(p)>150)
    ac=stats(px,W,H,(int(W*.3),int(H*.28),int(W*.72),int(H*.66)),lambda p:max(p)>150 and max(p)-min(p)<40)
    print(f"{f.split('/')[-1]:28s} sky={sky[0]}  haze@20/30/40%={haze}  terrain={terr[0]} sat={terr[2]}  hud={hud[0]} n={hud[1]}  aircraft={ac[0]} n={ac[1]}")
