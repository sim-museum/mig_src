# Sprint 8 — M2 3D/map colour fidelity

**Goal:** close the colour gap between the native software rasterizer and the Wine pixel oracle
(the sky reads grey/dark; the map tiles read greyish). Investigate + fix.

## Quantitative finding
Captured a clean QM cockpit-forward flight frame vs the Wine reference (`port/ref/wine`):
- **Terrain colour MATCHES Wine** — native bottom-1/3 mean RGB ~[51,55,49] vs Wine ~[47,52,51].
  So the 256-colour palette + the 8→16bpp 565 LUT (`palette_table` in ma_xasm) are **correct**.
- **Sky is far too dark** — native top-1/4 mean ~[52,52,40] vs Wine ~[227,232,235]. A sky-specific
  issue, NOT a global palette problem.

## Root cause (narrowed precisely; fix is a focused follow-up)
- The sky/horizon colour is **correctly computed** as a light blue **(142,166,200)** — traced via
  `MA_TRACE_SKY` in `LandScape::SetFogMode` (LANDSCAP.CPP): `horizonBase=(142,166,200)`,
  `afterGamma=` same (gamma a no-op), `afterGrey=` same (`mono3d=0`, so `GreyPalette` is inert —
  mono3d only goes true during a B&W gun-camera replay). It is stored via `DoSetHorizonColour` ->
  `DD.hcRed/hcGreen/hcBlue`.
- **But `DD.hcRed/hcGreen/hcBlue` is only consumed by the HARDWARE D3D background-material path**
  (`WIN3D.CPP` `CreateMaterial`), which is stubbed in the Linux software port.
- A pre-fill of the locked back surface to the sky colour (tried in `render3d` after `DoLockScr`;
  surface confirmed 640×480×16 at the presented bits) is **OVERDRAWN** — even a bright-red test fill
  shows dark, so the `LandScape` render itself paints the sky area (dark) over it.
- **Therefore the fix point is the landscape's SOFTWARE sky-render colour source** (a fade
  table / sky texture / per-span colour that ignores the light-blue horizon colour and renders
  dark), not a pre-clear. That's the focused next step.
- Gated diagnostics added (default-off): `MA_TRACE_SKY` (LANDSCAP SetFogMode + 3DCODE render3d).

## Map tiles (M4 inc4 cross-ref)
The greyish operational-map tiles are the same class of colour question (8→565 palette path); the map
terrain renders structurally correct, colour tuning shared with this sky/landscape work.

## Status
No fix landed this sprint (the landscape software sky-render needs deeper work), but the issue is now
precisely localized: **palette correct, horizon colour correct, the landscape software sky-render is
the dark culprit.** No regression (QM flight round-trip clean, stress green).
