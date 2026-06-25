# Sprint 20 — M2 sky fidelity: the "broken sky" was stale; measure the real residual

**Goal:** land the sky-routing fix teed up by Sprint 19 (sky renders dark/brown vs Wine).

**PO:** standing pre-approval. Run autonomously.

## Outcome: the premise was wrong — **the sky is not broken.** Re-measured; reframed the residual.

S8 (and S9, S19) were built on a measurement of the flight sky as **dark brown ~(53,51,31)**. That
number is **stale.** Ran the existing `MA_TRACE_SKY` instrumentation + measured the current rendered
frame (`MA_DUMP_BACK`), no code change:

- **Sky vertex colours feeding the polys are correct + bright** (`[skycol]`): top=(142,166,200) ramping
  through (216,232,248)…(255,255,255). `mono3d=0`. 12 software gouraud-gradient polys drawn.
- **`HorizonFadeData[0..15]` is the correct light blue** (152,180,216), and the gouraud filler
  (`XASM_GouraudHoriLine`) reads it (Sprint 19).
- **The actual rendered sky is light blue, not brown:** zenith (152,180,216), mid (149,177,212),
  low (138,164,197) — a clean, coherent blue.

**Why the old "brown" number was stale:** S8/S9 measured *before* the M2 fix (`1a70d2d`, "native blue
sky renders") corrected the `logicalscreenptr` drift so the sky polys write to the *presented* surface.
Before that fix the presented sky region showed terrain/garbage (brown); after it, the sky polys land
correctly → blue. S8/S9/S19 chased a defect that intervening work had already fixed. **Sprint 19's
"fix the polytype routing" is moot** (the routing was already verified correct on the software path).

## The real residual (quantified vs the actual Wine reference PNGs, not the stale number)
| Region (cockpit-fwd, level flight) | Native | Wine (`port/ref/wine/0{1,2}_cockpit_fwd_*`) |
|---|---|---|
| sky zenith (top of frame) | (152,180,216) | (224–244, 231–246, 234–242) |
| sky mid | (149,177,212) | (223–228, 226–232, 229–234) |

Native renders the engine's **raw horizon-blue** (~152,180,216); Wine's near-horizon sky is **~75
units brighter / whiter**. Cause: Wine renders via the **D3D hardware background-material path**
(`WIN3D.CPP CreateMaterial`, which applies near-horizon brightening/haze) — **stubbed in the software
port**. The software path faithfully paints the engine's computed horizon colours, with no hardware
material brightening. (In a level cockpit view you see *near-horizon* sky, the saturated-blue bottom of
the gradient — the white zenith is above the frame — so this is the lower gradient, rendered correctly.)

## Decision for the fix (next sprint / PO-steerable)
This is **not a rasterizer bug** — it's a **fidelity-target choice**:
- **(A) Match Wine:** replicate the D3D background-material near-horizon brightening as a bounded
  colour transform on the software sky vertices (brighten/desaturate toward white near the horizon).
  A focused, low-risk colour tweak in `InfiniteStrip`'s vertex colours or a post-fade lighten.
- **(B) Accept the faithful software look:** the software path already renders the engine's intended
  horizon colours; "match the D3D hardware look" is arguably the *wrong* target for a software renderer.

Recommend treating the sky as **functionally correct** (coherent blue gradient) and the brightness
delta as a **known minor tone gap** vs the D3D-hardware reference — low priority. If a brighter sky is
wanted, (A) is a small, bounded change.

## Value delivered
- **Closed the long-running "sky too dark/brown" thread** (S8/S9/S19) by disproving its stale premise.
- Quantified the true residual against the *actual* Wine reference (not the stale S8 number).
- Localized the cause (missing D3D background-material brightening), reframing it from "rasterizer bug"
  to "fidelity-target choice." No code change → no regression.
