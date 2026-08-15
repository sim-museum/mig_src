# Sprint 102 — "Letters, not bars" (PO-5) — ✅ CLOSED 2026-08-14 (goal MET) — ⭐ overlay text is legible

**Planned 2026-08-14 (PO pre-approved ceremonies; continuous-sprint directive). Autonomous. ~8 pts.**
**Sprint Goal:** make the 3D overlay text render as *letters*, and take the PO's new gold VIDEOS
into the tooling as a first-class oracle.

| Story | Pts | Result |
|---|---|---|
| S102-1 keep or kill S101's named suspect (MakeChar packing) | 2 | ✅ **killed** — a cell dump shows a clean graded 'S' |
| S102-2 find and fix what actually eats the glyph shape | 4 | ✅ **fixed** — `DoPutC` never sampled the alpha plane |
| S102-3 gold VIDEO oracle tooling | 1 | ✅ `port/tools/gold_video.sh` (frame/crop/sheet/geom) |
| S102-4 log the PO's six new defects + the hardware-graphics epic | 1 | ✅ EPIC J + PO-6…PO-12 on the board |

## ⭐ The defect: the software renderer never looks at the glyph

`ImageMap_Desc::InitFont` builds the overlay font atlas with the glyph SHAPE in `alpha`
(0..255 coverage from `MakeChar`) and a **constant `body`** — every texel 31. `polygon::DoPutC`
dispatches the glyph quad to `IMAPPED` / `IMAPPED_M`, and **both span fillers sample `body` and
never `alpha`** (confirmed against the shipped `GRAFPASM.ASM`, which the port's `ma_xasm.nasm`
reproduces faithfully). So every glyph came out as a filled 11×14 cell — S101's "solid bars".

**It was never a bug on Windows.** `direct_3d::PutC` textures the quad with the alpha map and
modulates it by `fontColour`; the shipped game draws text through the **hardware** path. The port
forces `Save_Data.fSoftware = true` because `DoHardPoly` is stubbed, and that quietly routed text
into a path the game never used for it. **Sixth PO defect in a row whose cause is a stub
rerouting work into an unexercised path — not a bug in the game.**

**Fix** (`SRC/GRAPHICS/Polygon.cpp`, `ma_putc_alpha_blit`): render the glyph the way the hardware
does — coverage from `alpha`, colour from the palette entry of `fontColour`, blended into the
software rasteriser's own target (`logicalscreenptr` / `BytesPerScanLine`, 16 bpp). The quad
`PutC3` builds is axis-aligned with a 1:1 texture rect, so this is an exact blit, not an
approximation of what the rasteriser was doing. Anything unexpected (no alpha plane, wrong bpp,
out-of-range rect) returns false and falls through to the engine path untouched.

## Evidence — three arms, prediction stated first

Predicted before the run: *fix* → legible letters; *`MA_NO_ALPHATEXT=1`* → solid bars (the S101
state); *`MA_NO_GLYPHS=1`* → nothing at all. Measured, same recipe, back-surface dump at Blt 700
(`SDL_VIDEODRIVER=dummy`, `BOB_CLICKSEQ="40,r1;95,r0"`, `MA_ENABLE_3D=1`) — the in-flight command
menu, bright pixels in the text band x 225–460, y 40–100:

| arm | bright px | what the frame shows |
|---|---|---|
| fix | 610 | **"1. Pincer attack. / 2. Multi-wave attack. / 3. Select target / 4. Continue"** |
| `MA_NO_ALPHATEXT=1` | 3711 | four solid white bars |
| `MA_NO_GLYPHS=1` | 303 | nothing (background only) |

Both controls matter and both were run: the *treatment* shows letters, the *engine-dispatch*
control reproduces the old symptom exactly, and the *no-glyphs* control proves the ink comes from
S100's atlas. An earlier attempt at Blt **250** produced three **byte-identical** captures — the
page-0 font map is not touched that early, so the dump was simply before any text. That is worth
recording: *identical captures in an A/B mean the recipe missed the feature, not that the change
does nothing.*

## S101's suspect, killed cheaply

S101 closed naming `MakeChar`'s `0x40404040` packing. `MA_GLYPH_DUMP=S` prints the cell as ASCII
art, and the letter is plainly there and graded:

```
[glyphcell] 'S' cell(7,4) at atlas(78,57)  body[ch]=7 alpha[ch]=0
|.o++o......|   |:o+o:......|   |:o..o:.....|
|:o:.o:.....|   |..:o+:.....|   |:o++o:.....|
|:o.........|   |.:..:o.....|   |..::.......|
```

Two minutes of looking beat a sprint of reasoning about shifts — which is exactly what S101's own
closing note told the next attempt to do.

## Also found (logged, not fixed here)

- `[doputc] body[0]=254` — the polytype choice for the font map (`*body == ARTWORKMASK` →
  `IMAPPED_M`) is decided by a **width-table byte**: `MakeChar` writes `body[uChar&0x7F] =
  advance+1`, and for the char that folds onto index 0 that advance happens to be 253. The font's
  mask/no-mask decision has always been an accident of a font metric. Harmless now that the port
  does not use either span type for text, but it belongs in the notes.
- `fontColour = 252 (WHITE)` and `SetPaletteEntry(252, GetPaletteEntry(252))` is a self-copy
  no-op (S94's finding, still true and still harmless): the port reads the entry *after* the copy,
  so the engine's intent is preserved either way.

## Gold videos are now an oracle

`port/tools/gold_video.sh` — `list` / `frame` / `crop` / `sheet` / `geom` over the PO's two
recordings (2026-08-14). Geometry measured, not assumed, because the two differ: **short =
1280×1024** windowed at (320,28); **full = 1200×1080** at (480,0). A video oracle is what makes
PO-6…PO-11 checkable at all — they are *behaviours* (what R does, what appears after ALT+X), and
a still cannot settle those.

## Gates

parity 5/5 · sweep 9 OPEN/0 CRASH · map click · map drag · sysbox exit · help click ·
stress 20/20 · ASan 0. (This diff touches the shared software rasteriser, so the parity set is
the gate that matters.)

## Result

PO-5 **CLOSED** — six sprints after the PO reported "the text does not display", and the last
three each moved it one honest step: S99 refused to ship a decoder that lied, S100 fixed the empty
atlas, S101 refused two false positives and named the wrong suspect, and S102 killed that suspect
in two minutes by *looking at the thing*. The fix also lights the path for PO-6 (map window text),
PO-7 (radio menu) and PO-8 (info line): all three are drawn by `PutC3` → `DoPutC`.
