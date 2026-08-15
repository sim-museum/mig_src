# Sprint 117 — "Lines, points, and the depth the engine meant" (PO-12 phase 3c) — ✅ CLOSED 2026-08-15 (goal MET)

**Planned 2026-08-15 (PO pre-approved ceremonies; continuous-sprint directive). Autonomous. ~8 pts.**
**Sprint Goal:** close the three gaps S116 measured against the software oracle — the info line, the
lower cockpit coaming, and the opcodes the walk stepped over.

| Story | Pts | Result |
|---|---|---|
| S117-1 draw `D3DOP_LINE` and `D3DOP_POINT` | 2 | ✅ 133824 lines, 8271 points — the attitude gizmo appears |
| S117-2 the info line | 4 | ✅ two faults: a coverage-mask texture, and depth |
| S117-3 the lower cockpit coaming | 2 | ✅ same depth fault — one cause, two symptoms |

## Evidence

`port/ref/native/hw_cockpit_full.png` against `port/ref/native/sw_cockpit_ref.png` (the software
renderer at the same frame). The hardware frame now carries everything the oracle does: canopy
frame, instrument panel, compass, altimeter tape, gunsight and pipper, artificial horizon, the
wireframe attitude gizmo, the lower coaming, and the info line —
`Speed: 379Kts   Mach: 0.64   Alt: 17662ft   Hdg: 279   Thrust: 0`.

## What was wrong

**1. Lines and points were never drawn.** 133824 `D3DOP_LINE` and 8271 `D3DOP_POINT` instructions
per flight, stepped over since S115. `D3DPOINT` is a *run* (`wCount` vertices from `wFirst`), not a
single index — worth reading the macro rather than assuming. These are the engine's thin geometry;
the attitude gizmo top-left is theirs.

**2. The font texture is a COVERAGE MASK, and modulating by it erases the text.** The glyph texture
is ARGB4444 with the shape in alpha and **RGB uniformly zero** — deliberately: `direct_3d::SetPalette`
has a "knobble" block that pins the `FONTMASK` palette entry to `0x08`, a marker rather than a
colour, and `direct_3d::PutC` puts the real colour (`fontColour`) in the **vertex**
(`shadePolyColor = fontColour`). Plain `GL_MODULATE` computes `tex.RGB × vertex` = black.
The renderer now detects this **from the texels** — RGB uniformly blank while alpha varies — and
switches to `GL_COMBINE`: colour from the primary colour, alpha from the texture. Measured, not
inferred from the call site, so it cannot mis-fire on ordinary art.

This is the same finding S102 made for the software path, arriving from the other direction: the
glyph shape lives in `alpha`, and the colour is the engine's `fontColour`.

**3. ⭐ Depth: the render state was not persistent, and the z sense was inverted.** Two faults, one
symptom, and between them they cost the info line *and* the coaming — which looked like two
unrelated missing features.

- **Render state is persistent across execute buffers**, exactly as on a real device. The walk
  reset it to invented defaults on every `Execute`. The proof is in the census: the engine sets
  `D3DRENDERSTATE_ZENABLE` **exactly once in a whole flight** (state 7, ×1) because it expects the
  device to remember it. Re-asserting a default every buffer overrode the engine behind its back.
- **`glOrtho` negates z.** With `near=-1, far=1`, `depth = (1−z)/2` — the reverse of D3D, where 0
  is the near plane and larger z is farther. So *farther* geometry won the depth test, and the
  overlay batches, which sit at the near end and are drawn last precisely so they sit on top, were
  rejected. `glScalef(1,1,-1)` makes depth increase with the game's z, so `LESSEQUAL` means what
  the engine means by it.

The engine's own values, once persistence was fixed, are `ZENABLE=1` and `ZFUNC=4` (LESSEQUAL) —
so depth testing stays **on**, as intended. Disabling it would have "fixed" the screenshot and left
the world sorting itself wrongly.

## Method note

The step that mattered was refusing to accept a plausible cause. Depth was suspected and *cleared*
in S115 (`MA_EXEC_NODEPTH` changed nothing) — but that test ran while blend was still multiplying
everything to zero, so it proved nothing. Re-tested here after the other faults were gone, depth was
the answer. **A control that runs while a known fault is still present does not clear its suspect.**

The glyph hunt was likewise measurement at each step: 94305 `PutC` calls (the path runs) → 343498
glyph-sized alpha-textured triangles reaching y 144..484 (the quads exist, in the right place) →
marking those batches magenta (they land exactly where the info line's characters belong) → the
bound texture has `rgb!=0 0/16384` (the texture is a mask) → 99.9% of glyph batches carry vertex
colour `ffffffff` (the colour is white and correct). Each answer eliminated a whole class.

## Hot-path hygiene

`getenv` was being called **per texel** in the `PrepTexture` trace and per draw in three others.
All cached now. The S115 sequence scaffolding (`seq BEGIN/DRAW/END`) is removed: it answered its
question — the order was always correct.

## Gates

parity 5/5 byte-identical · sweep 9 OPEN/0 CRASH · map click · map drag · sysbox exit · help click ·
overlay text 3/3 · stress 20/20.

## Next

The hardware renderer now matches the software one on the cockpit view. Remaining before the option
can ship (phase 4): fog and specular (states 28/29 are read but not applied), the
`IDirect3DViewport::Clear` path, a sweep of other views (external, map, padlock) against the same
oracle, and then the Preferences primary-graphics option itself with automatic software fallback.
