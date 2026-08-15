# Sprint 120 — "The landscape has its own pipeline" (PO-15) — ✅ CLOSED 2026-08-15 (goal MET) — ⭐ terrain renders in hardware

**Planned 2026-08-15 (PO opened PO-15 as an epic and directed continuous sprints). Autonomous. ~8 pts.**
**Sprint Goal:** find why hardware terrain is black ink while objects standing on it draw
correctly, and fix it.

| Story | Pts | Result |
|---|---|---|
| S120-1 is the terrain geometry there at all? | 3 | ✅ yes — 600 land Executes, all with vertices |
| S120-2 follow the landscape's own texture path | 3 | ✅ separate pipeline, never fed |
| S120-3 fix and verify against the software oracle | 2 | ✅ 4080/4096 texels, terrain renders |

## ⭐ The PO's observation was the whole diagnosis

> *"Objects (huts, control tower) are visible but not the landing strips themselves."*

Same scene, same renderer, one class of surface drawn and another not. That rules out geometry,
depth, blending and the projection in one sentence, and points at a texture path that objects do
not use. It was worth more than any measurement I took.

## What the landscape actually does

Terrain does **not** load textures the way objects do. Objects go through `CreateTexture` /
`PrepTexture` / `IDirect3DTexture::Load`. The landscape has a parallel pipeline:

```
TileMake::UpdateColorData / PatchColorData / Render2Map      build a tile
  direct_3d::RenderTileToDDSurface                            (Win3d.cpp:12844)
    pSurf->Lock(NULL,&sd,…)                                   a SYSTEM texture surface
    rsd.lpSurface      = sd.lpSurface;
    rsd.lPitch         = sd.lPitch;
    rsd.dwRGBBitCount  = sd.ddpfPixelFormat.dwRGBBitCount;    <-- 0
    TileMake::RenderTile2Surface(pTileData,&rsd)              rasterise the tile
    pSurf->Unlock(…)
  pVidNext->Blt(NULL,pNext,NULL,…)                            system -> video texture
```

**Root cause: the compat `IDirectDrawSurface2::Lock` never filled `ddpfPixelFormat`.** The tile
rasteriser was handed a bit count of **0** — no format to write in — so every land tile came back
blank. Blank tiles blitted to video, and index 0 is the engine's transparent key, so each land tile
uploaded fully transparent and the cleared black showed through. Objects never take this path,
which is exactly the split the PO saw.

The fix also makes `Lock` report the **owner's** live pixels rather than the view's copy, and
`Blt` on that face is now real (it was `{ return DD_OK; }` — the third stub of that shape found in
two days, after `GetAttachedSurface` and `Load`'s palette).

## The measurement chain

Each step eliminated a class, and the *negative* results mattered most:

| step | result | eliminated |
|---|---|---|
| `FlushLandDraw` census | 600 calls, **all with vertices** | "terrain isn't submitted" |
| land `Execute` dump | 545–921 verts, screen coords sane, colour `fffefefe` | geometry, transform |
| `MA_EXEC_FALSECOLOUR=2` | horizon band **stays black** | "drawn but mis-shaded" |
| bound texture probe | 64×64 8-bit, palette attached, **0 of 4096 texels set** | narrowed to the texture |
| `UploadLandTexture` / `GetLandBufferPtr` counts | **never called** | a whole dead branch (its sys-RAM half is commented out in the shipped source) |
| DX2 `Blt` instrumentation | blits run, **source carries no data** | "the blit is dropping it" |

That last one is what turned it: the copy was working and the *source* was empty, so the fault was
upstream of everything I had been changing.

**A wrong turn worth recording:** I "fixed" this once already in S119 by carrying the palette across
`Load`, and declared terrain fixed on a capture taken at 17,000 ft where the ground is mostly haze.
The palette fix was a real bug but not this one. *Measuring the right quantity in the wrong
conditions is not a measurement* — the oracle had to be a low-altitude frame, which is what the PO
was looking at all along.

## Evidence

`port/ref/native/hw_terrain.png` — quick mission at 4,530 ft, hardware: brown/olive Korean terrain
below the horizon, 7595 distinct colours in the lower view (was 1 flat black). Matches
`qm1_sw.png`, the software renderer at the same altitude and heading.

## Gates

parity 5/5 byte-identical · sweep 9 OPEN/0 CRASH · map click · map drag · sysbox · help click ·
stress 20/20 software and hardware.

## Next in this epic

The terrain draws; the remaining landscape questions are quality, not blockers — runway/airfield
detail tiles at close range, the mip chain (`RenderTileToDDSurface` builds one and the port only
uploads level 0), and fog/specular. Then the PO's campaign-GUI list.
