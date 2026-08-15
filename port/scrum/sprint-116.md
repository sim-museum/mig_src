# Sprint 116 — "The textures arrive" (PO-12 phase 3b) — ✅ CLOSED 2026-08-15 (goal MET)

**Planned 2026-08-15 (PO pre-approved ceremonies; continuous-sprint directive). Autonomous. ~8 pts.**
**Sprint Goal:** bind the textures, so the hardware cockpit stops being white silhouettes.

| Story | Pts | Result |
|---|---|---|
| S116-1 upload the game's two texture formats to GL | 3 | ✅ ARGB4444 direct, 8-bit expanded |
| S116-2 fill the textures the game actually draws from | 3 | ✅ `IDirect3DTexture::Load` was a no-op |
| S116-3 give the palettized art its colours | 2 | ✅ a real `IDirectDrawPalette` |

## Evidence

`port/ref/native/hw_cockpit_textured.png` — the hardware frame now shows the metallic canopy frame
with its shading, the instrument panel, the compass gauge, the altimeter tape, the trim knob, the
gunsight glass with its reflections and yellow pipper, the artificial horizon instrument, and
textured terrain. **6722 distinct colours, up from 86 when the geometry first landed (S115).**

`port/ref/native/sw_cockpit_ref.png` — the software renderer at the same frame, as the oracle.
Side by side, the hardware frame matches it on the canopy, panel, instruments and gunsight, and is
still missing two things (see "What is still missing").

## What was wrong — measured, in this order

**1. Both texture formats had to be identified, not guessed.** `MA_TRACE_TEX=1` records what the
game asks `CreateSurface` for, and it uses exactly the two formats `EnumTextureFormats` offered it
in S110 and nothing else:

```
1024 x  64x64  16bpp  R=0f00 G=00f0 B=000f A=f000     <- ARGB4444 (transparent art)
  64 x  64x64   8bpp  all masks 0                      <- palettized (opaque art)
   9 x 256x256  8bpp  ...
```

ARGB4444 needs no conversion pass: the 16-bit word is A,R,G,B from the high nibble down, which is
exactly `GL_BGRA` + `GL_UNSIGNED_SHORT_4_4_4_4_REV` (with `_REV` the format's FIRST component sits
in the LOW nibble, so B,G,R,A low-to-high *is* A,R,G,B high-to-low). The surface now remembers the
masks it was created with, because a 4444 texture read back as 565 is unrecognisable art with no
alpha, and nothing would have said so.

**2. ⭐ `IDirect3DTexture::Load` was a no-op, so every texture was empty.** The first upload trace
was unambiguous — **"0/4096 non-zero texels"** for every texture, in both formats. That is a
different failure from a wrong pixel format and would have been very easy to misread as one; the
right response to "the art looks wrong" is to check that there IS art. The engine fills a texture
in two steps (`Win3d.cpp:4412`):

```c
PrepTexture(lpDD2TSurf, ...)             // writes texels into the SYSTEM surface
pVrt->lpd3dTexture->Unload();
pVrt->lpd3dTexture->Load(lpD3DText);     // dest->Load(src): system -> video
```

The renderer uploads from the surface the texture *handle* names — the destination — which no one
had ever written to. `Load` now copies the texels across and marks the surface dirty.

**3. `CreatePalette` returned NULL, so the palettized art was black.** The engine keeps `MAX_PALS`
palettes, calls `SetPalette` per texture and `SetEntries` to update them in place; with NULL there
was nowhere for the colours to live. A real vtbl-backed `IDirectDrawPalette` now stores its 256
entries, and the surface remembers which palette its texels index. **A global display palette is
not a substitute** — the game deliberately uses several, chosen per texture.

## The dirty edge

Textures are re-uploaded when the game rewrites them, not every frame: `Unlock` on the DX2 face
(which is how `PrepTexture` writes) forwards a dirty flag to the DX1 surface that owns the pixels
and the GL texture name; `Load` and `SetPalette` set it too.

## What is still missing (S117)

Against the software oracle, the hardware frame lacks:

- **the bottom info line** (`Speed / Mach / Alt / Hdg / Thrust`) — this is the engine's own text
  path, `direct_3d::PutC`;
- **the lower cockpit coaming**, so terrain shows where the dark panel should be;
- **`D3DOP_LINE` (76224) and `D3DOP_POINT` (5329)** instructions, still stepped over by the walk.

## Gates

parity **5/5 byte-identical** (the palette object is new on a path the 2D front-end also uses —
this is the gate that mattered) · sweep 9 OPEN/0 CRASH · map click · map drag · sysbox exit · help
click · overlay text 3/3 · stress 20/20.
