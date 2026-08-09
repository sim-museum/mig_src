# Sprint 100 — "There were never any glyphs" (PO-5) — ⚠️ CLOSED PARTIAL 2026-08-09 — ⭐ root cause found and fixed at source; the stub's own comment had said so since bring-up

**Planned 2026-08-09 (PO pre-approved ceremonies; continuous-sprint directive). Autonomous. ~8 pts.**
**Sprint Goal:** PO-5 — *"text doesn't print — ALT-D info about padlocked bogie, map screen
forward/waypoint/exit menu, 'R' radio command menu."*

| Story | Pts | Result |
|---|---|---|
| S100-1 root-cause the invisible overlay text | 4 | ✅ found, and it is not what S94 thought |
| S100-2 implement the missing rasteriser | 3 | ✅ proven deterministically |
| S100-3 show text on screen | 1 | ⬜ **not achieved** — blocked on scenario state, not the pipeline |

## The cause, and it was written down in the codebase all along

`COverlay` does not load its font as artwork. It **builds a glyph atlas at runtime** by asking
Windows to rasterise each character — `ImageMap_Desc::MakeChar` → `GetGlyphOutline(...,
GGO_GRAY8_BITMAP, ...)`. And the compat layer said:

```c
/* GetGlyphOutline glyph-rasterising API (OVERLAY renders overlay text via font
   glyphs). Stubbed for bring-up: returns 0 (no glyph bitmap) -> blank text now;
   a real path can rasterise via FreeType/SDL_ttf later. */
static inline DWORD GetGlyphOutlineA(...) { return 0; }
```

**Every glyph's alpha stayed zero.** The overlay text was laid out, positioned and composited
perfectly — and drawn completely transparent, because there was nothing in it. The stub had
announced the defect in its own comment since bring-up, and five sprints of investigation went past
it.

**This also retires S94's theory.** S94 traced the glyphs to palette slot 252, found `WHITE == 252`
made `SetPaletteEntry(252, GetPaletteEntry(WHITE))` a self-copy, and concluded the text was drawn
transparent through the palette. Writing a real white into 252 did *not* help — recorded honestly at
the time as "so the texels don't index 252 either". They didn't index anything: **there were no
texels.** The palette work was a correct observation about the wrong layer.

## The fix
`GGO_GRAY8_BITMAP` implemented against the stb_truetype faces `ma_gdi` already loads. Contract
details that matter, all taken from what `MakeChar` actually consumes:
- **levels are 0..64, not 0..255** — the caller masks `0x40404040` to split the saturated bit out
- rows are `gmBlackBoxX` bytes **padded to a DWORD**
- `gmptGlyphOrigin.y` is height above the baseline (stb's `y0` is negative above it)
- the `MAT2` is 16.16 fixed and the engine passes a **non-square** scale
  (`{46811,…},{60075,…}`), so the axes are scaled independently rather than assumed equal

## ⭐ How it was verified — after two invalid instruments in a row
1. **Screenshot**: showed "10 20 30 40" on the altitude ladder. Looked like proof. It is **cockpit
   art** — present with the fix *and* without it.
2. **Whole-frame A/B** (glyphs on vs off): 14187 px differ. Also worthless — **two IDENTICAL flight
   runs differ by 2715 px**, so a frame diff measures the simulation, not the change. *Checking
   that first is what stopped this becoming the sprint's conclusion.*
3. **The instrument that works** — count the ink in the atlas itself, which is deterministic and is
   exactly the thing that was broken:

```
[glyphatlas] page=0 non-zero alpha bytes = 2666 of 16384      (S100)
[glyphatlas] page=0 non-zero alpha bytes =    0 of 16384      (MA_NO_GLYPHS=1, the old stub)
```

`MA_NO_GLYPHS=1` is kept precisely so that A/B stays available: a switch that removes exactly the
glyphs is a claim a wrong fix cannot satisfy — which is the S99 rule applied on the first try this
time, not the third.

## What is NOT done — PO-5 stays open
**No capture yet shows overlay text on screen.** The atlas has ink and the blit path runs, but the
in-flight HUD line still does not appear: `DrawInfoBar` returns early on
`infoLineCount == 0 && messageTimer == 0` and the pinned save has `infoLineCount = 0` (S94's finding
(a), which remains true and is a *setting*), and the padlock readout needs an enemy actually
selected — the same scenario wall B7 and C4c/C4d are at. **The glyph pipeline was the port defect;
what is left is getting the game into a state that draws text.**

## Gates — all under `gl-lock`
- **parity 5/5 byte-identical** (the change touches the shared compat GDI, so this one mattered) ·
  **stress 20/20** · **ASan 0 reports** · sweep, map click, map drag, sysbox exit, help click

## Result
The oldest and most-investigated of the five play-test defects had a one-line cause that was
documented in the stub that caused it. Two plausible verification methods were rejected before one
that could not lie was used — and the second of those (frame diffing a nondeterministic simulation)
would have produced a confident, wrong sprint conclusion.
