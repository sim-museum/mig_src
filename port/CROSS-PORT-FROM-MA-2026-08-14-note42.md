# Cross-port note 42 — MA → BoB (2026-08-14, MA Sprint 102)

## 1. Finding: a font atlas whose glyphs live in `alpha` drawn by a span filler that only reads `body`

MiG Alley's 3D overlay text (in-flight menus, radio menu, map window text, info line) rendered as
**solid filled cells** for the port's whole life. Cause, after five sprints of narrowing:

- `ImageMap_Desc::InitFont` builds the overlay font atlas with the glyph SHAPE in the image map's
  **`alpha`** plane (0..255 coverage from `MakeChar` → `GetGlyphOutline(GGO_GRAY8_BITMAP)`) and
  fills **`body` with a constant** (31 in MA).
- `polygon::DoPutC` dispatches the glyph quad as `IMAPPED` / `IMAPPED_M`. Both span fillers sample
  `body` and **never `alpha`** — checked against the shipped `GRAFPASM.ASM`, not just our nasm port.
- So the correct render was never possible on the **software** path. On Windows it never had to be:
  `direct_3d::PutC` textures the quad with the alpha map and modulates it by `fontColour`, and the
  shipped game draws text through the **hardware** path.
- Our port forces `Save_Data.fSoftware = true` (DoHardPoly stubbed), which quietly routed text into
  a path the game never used for it.

**Fix** (`SRC/GRAPHICS/Polygon.cpp`, `ma_putc_alpha_blit`): in `DoPutC`, blit the glyph directly —
coverage from `alpha`, colour from `fontColour`'s palette entry, into the rasteriser's own target
(`logicalscreenptr`/`BytesPerScanLine`, 16 bpp). The text quad is axis-aligned and 1:1, so it is an
exact blit. Guarded: anything unexpected falls through to the engine dispatch untouched.
`MA_NO_ALPHATEXT=1` reverts, which is the A/B that makes a text capture provable.

**Does this affect BoB?** Probably not directly — BoB runs D3D7 hardware, so its text should be
going through the hardware `PutC` as the game intended. Two questions worth a cheap check:
1. Does BoB's `COverlay`/`ImageMap_Desc` build a runtime glyph atlas the same way, and does
   anything in BoB ever take the software `polygon::DoPutC` (e.g. a fallback path, or 2D map text)?
   If yes, it has this bug latent.
2. Is `GetGlyphOutline(GGO_GRAY8_BITMAP)` implemented in BoB's compat layer? MA's was
   `return 0` **with a comment saying "blank text now"** until S100 — a stub whose comment
   describes a user-visible consequence is a bug report nobody filed.

## 2. Method worth stealing: measure the SHAPE of ink, not its presence

Neither failure state (no glyphs / filled cells) failed any gate, because a screenshot cannot tell
them apart and a whole-frame diff is noise (two identical MA flight runs differ by ~2700 px). What
separates them in one measurement is the **horizontal bright-run distribution** in the text band:

| state | runs | mean run | max run |
|---|---|---|---|
| letters | 266 | 2.3 px | 5 px |
| filled cells | 105 | 35.3 px | 97 px |
| no glyphs | 61 | 5.0 px | 5 px (background speckle only) |

`port/overlay_text.sh` encodes that as a gate. Generalises to any "is this text or a blob?"
question either port has.

## 3. Question for BoB: the hardware renderer (MA PO-12)

The PO has asked MA for a **hardware graphics option in Preferences**, citing that BoB already runs
hardware. From `ROWAN_ENGINE_LINUX_PORT_NOTES.md` we already know the APIs differ (BoB = D3D7 +
Lib3D software T&L; MA = DX5/6 **execute buffers**), so MA cannot take BoB's device wholesale.
What would help most, in rough order:

1. What does BoB's D3D7→GL device actually implement — a `IDirect3DDevice7::DrawPrimitive`-level
   shim, or something lower? Where does it live?
2. How is the texture cache/handle lifetime handled (MA's `RecordTextureUse`/`FlushPTDraw` shape)?
3. Anything in the render-state translation that was surprising (blend modes, alpha test, palette
   textures) — MA's text path needs exactly alpha-modulated palette textures.
4. Does BoB expose the software/hardware choice in its own Preferences, and if so where is it
   read (`CONFIG.CPP` `Save_Data.fSoftware`)?

No rush — MA will scope PO-12 from the MA side first.
