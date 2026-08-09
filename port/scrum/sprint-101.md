# Sprint 101 — "Show the text" (PO-5 cont.) — ⚠️ CLOSED PARTIAL 2026-08-09 — atlas text now marks the screen, but as blocks not letters

**Planned 2026-08-09 (PO pre-approved ceremonies; continuous-sprint directive). Autonomous. ~5 pts.**
**Sprint Goal:** get S100's now-inked glyph atlas to produce *readable* overlay text on screen.

| Story | Pts | Result |
|---|---|---|
| S101-1 find a scenario that draws atlas text | 3 | ✅ `DrawTopText` draws "Speed…" at (0,471) every frame |
| S101-2 show it legibly | 2 | ⬜ **not achieved** — it renders as solid blocks |

## Two false positives, both caught by the same discipline
1. **"RUDDER TRIM"** appears legibly at the bottom of the flight view — genuine, readable overlay
   text, and for about a minute it looked like PO-5 closing. **It renders identically with
   `MA_NO_GLYPHS=1`**, so it does not come through the glyph atlas at all; it is a different text
   path that was already working. *Not evidence.* (It does qualify the PO's report slightly: *some*
   in-flight text was always printing.)
2. Which raised the right question — **does the atlas path run at all?** A counter in `PutC3`
   settles it: it does, drawing `S p e e d` at (0,471) every frame. So the failure was never
   "the text is not drawn".

`MA_NO_GLYPHS` earning its keep twice in two sprints is the argument for keeping a disable switch
next to every fix of this kind.

## Where it actually stands
With glyphs on, the bottom-left band at y≈471 fills with white marks that are **absent when glyphs
are off** — so S100's rasteriser is reaching the screen. But the marks are **solid bars, not
letters**: the glyph cells are being filled rather than shaped.

That is a specific, narrow next step, and the suspects are the packing contract in
`ImageMap_Desc::MakeChar`, which is unusually particular:
- it masks `0x40404040` to separate *saturated* texels, so any conversion that lands too many
  values on exactly 64 turns every pixel fully opaque — the exact symptom seen
- it rebuilds a 4-flag nibble (`t>>14`, `t>>7`, `&0x3C0`, `>>6`) and ORs `vsets[t]` back in
- rows are 3 DWORDs written then `trg += 29`, i.e. a 128-byte atlas stride and an 11×14 cell

The next attempt should dump one glyph's 0..64 buffer and the resulting atlas cell side by side,
rather than reasoning about the packing from the source.

## PO-5 status, stated plainly
**Open.** S100 fixed the root cause (no glyphs existed at all) and proved it deterministically;
S101 shows those glyphs now reach the screen but render unshaped. The defect has moved from "the
font atlas is empty" to "the atlas-to-screen packing is wrong" — a different and much smaller
problem, and one with a named location.

## Gates
No gate-visible change (`PutC3` gained one `getenv`-gated counter). S100's committed results stand.

## Result
Two candidate proofs rejected in one sprint, both by asking the same question — *would this look
the same if the fix were absent?* Neither rejection was expensive; accepting either would have
closed PO-5 wrongly.
