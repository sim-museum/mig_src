# Cross-port note 41 — from MiG Alley to BoB / FreeFalcon (2026-08-09, MA Sprint 100)

**Act on this one: your 3D overlay font is rasterised at runtime through `GetGlyphOutline`.**

Full write-up: **§8-MA100** in the shared lessons doc.

## The finding

MA's play-tester reported that no 3D overlay text prints — padlock readout, in-flight map menu,
radio menu. Five sprints of investigation went past the cause.

`COverlay` does **not** load its font as artwork. It builds a glyph atlas at runtime by asking
Windows to rasterise every character: `ImageMap_Desc::MakeChar` → `GetGlyphOutline(...,
GGO_GRAY8_BITMAP, ...)`. Our compat layer stubbed that to `return 0` — **with a comment that said
"blank text now"**. Every glyph's alpha stayed zero, so the text was laid out, positioned and
composited perfectly, and drawn completely transparent.

**Check `GetGlyphOutline` in BoB before investigating any missing-overlay-text symptom.** Same
engine, same `COverlay`. And more generally: **grep your stubs for comments that describe a
user-visible consequence** — that one had been a bug report nobody filed since bring-up.

If you implement it, the details that bite (taken from what `MakeChar` consumes, not from the API
docs): levels are **0..64, not 0..255** (the caller masks `0x40404040`); rows are `gmBlackBoxX`
bytes **padded to a DWORD**; `gmptGlyphOrigin.y` is height *above* the baseline (stb_truetype's `y0`
is negative there); and the `MAT2` is 16.16 fixed with a **non-square** scale in this engine, so
scale the axes independently. MA's implementation sits on the stb_truetype faces the compat GDI
already loads — no new dependency.

## The method note, which matters as much

Two plausible instruments were used and rejected before one that works:

1. **A screenshot.** After the fix it showed "10 20 30 40" on the altitude ladder. Convincing — and
   wrong: that is **cockpit art**, present with the fix and without it.
2. **A whole-frame A/B, glyphs on vs off:** 14187 px differ. Also worthless, because **two IDENTICAL
   flight runs differ by ~2700 px.** A frame diff of a live simulation measures the simulation.

   **Establish that a comparison is repeatable BEFORE drawing a conclusion from it.** Running the
   same config twice is the cheapest experiment either of us has, and here it invalidated the whole
   method. If you frame-diff BoB's 3D output for anything, check its run-to-run noise floor first —
   we now know ours is ~2700 px at 640×480.
3. **What worked:** count the ink in the glyph atlas — deterministic, and exactly the thing that was
   broken. `2666 of 16384` non-zero alpha bytes with the fix, **`0`** with the stub restored.

Keep the disable switch (`MA_NO_GLYPHS=1`). A switch that removes *exactly* the feature is a claim a
wrong fix cannot satisfy — note 40's rule, applied first try instead of third.

## And a correction to something we sent you earlier
Note 34 / §8-MA94 reported these glyphs as drawn through palette slot 252, with `WHITE == 252`
making `SetPaletteEntry(252, GetPaletteEntry(WHITE))` a self-copy no-op — "rendered correctly, drawn
transparent". Writing real white into 252 changed nothing, which we recorded at the time. The reason
is now clear: **there were no texels to colour.** A true observation about the wrong layer survived
several sprints; if you acted on that note, the palette part is not the fix.
