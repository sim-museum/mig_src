# Sprint 47 — "Campaign map date/period readout" (autonomous, headless DoD)

**Context:** Campaign roadmap. BoB's map shows a date/time/accel readout (S84). MA's `TitleBar::Redraw`
builds the string but `TitleBar` is a `CRToolBar` that isn't hosted on the map yet.

**Sprint Goal:** Show the campaign date/period on the map without waiting for the full CRToolBar-hosting epic.

## Approach
Rather than host the titlebar `CRToolBar`, draw its string directly on the map. `MIGVIEW` has `MMC`,
`GetDateName`, and `RESLIST` in scope, and `pDC->TextOut` renders map text. Added (in `UpdateBitmaps`, after
`DrawIcons`/`PlotRoutes`, `#if MA_LINUX`) a top-left readout with the exact string `TitleBar::Redraw` uses:
`GetDateName(MMC.currdate, DATE_SHORT) + ": " + RESLIST(MORNING, currperiod) + ", " + RESLIST(PLANNING, indebrief)`,
drawn with a 1px shadow for legibility over terrain.

## Validation (headless DoD)
- Frame capture: **"6/25/50: Morning, planning"** (Korean-war start date) renders top-left of the map.
- No regression: `port/asan_campaign.sh` PASS (0 reports, 2/2 map render). Compiles clean.

## Increment
Campaign map now shows terrain + icons + front-line + routes + **date/period** — the readout half of BoB's
footer chrome, without the CRToolBar infra. The remaining chrome (footer band, accel, toolbar buttons) is
the CRToolBar-hosting epic (scoped in `campaign-epic.md`).
