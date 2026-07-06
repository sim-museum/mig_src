# Sprint 46 — "Campaign map unit/airfield icons" (autonomous, headless DoD)

**Context:** Campaign roadmap, next functional gap after S45. BoB's strategic map shows the unit-icon layer
(airfields/squadrons/raid markers); MA's map showed terrain only. `CMIGView::DrawIcons` exists and is
called from `UpdateBitmaps` (the map render), so the path executed — but drew nothing.

**Sprint Goal:** Get the strategic-map unit/airfield icons to render.

## Root cause (two gates, both fixed)
`MA_TRACE_ICONS` showed **652 items in the world but 0 icons drawn**. Traced:
1. **Garbage view bounds.** `DrawIcons` derived its visible-world rect from `pDC->GetBoundsRect(&bounds,0)`.
   The compat `CDC::GetBoundsRect` returns uninitialised bounds + `DCB_SET` → `bounds=(135080475,-3656612,
   36441,…)` → an **overflowed** world rect `[2146698002 .. -2146698002]` that excluded **every** item, so
   `DrawIconTest` was never even reached. `UpdateBitmaps` already uses `GetClientRect` for the same purpose.
   **Fix (`MIGVIEW.CPP`, `#if MA_LINUX`):** use `GetClientRect(&bounds)` → sane `bounds=(0,0,608,408)` and a
   valid world rect; ~220 items now fall in view.
2. **Map filters all-off.** With bounds fixed, `DrawIconTest` still returned `FIL_NULL` — the icon categories
   are gated on `Save_Data.mapfilters[...]`, which the loaded save has off, and the **filter toolbar that
   normally toggles them is not hosted yet**. **Fix (`MIG.CPP`):** default the standard view-filters
   (airfields / supply / bridges / waypoints / front-line) ON the first map render (`MA_NO_MAP_FILTERS`
   inspects the raw state). Icons draw: **33** at the default zoom.

## Validation (headless DoD)
- `MA_TRACE_ICONS`: `icons-drawn=33` by default (was 0). Frame capture (`BOB_DUMP_FRAME`, now pixel-accurate
  post-S45): the map shows **red airfield/squadron/supply markers + the orange front-line + white routes**
  over the colour terrain — a proper strategic map, matching BoB.
- **No regression:** `port/asan_campaign.sh` PASS (0 ASan reports, 2/2 map render). Compiles clean; links.

## Increment
The campaign map is now at **icon parity with BoB** (terrain + unit/airfield icons + front-line + routes).

**Next (roadmap):** date/period readout (`TitleBar::Redraw`), then host the `CMainToolbar`/`MSCTLBR`
`CRButtonCtrl` toolbar buttons on the map (mirror BoB S88–92) + wire clicks → mission-folder/unit-select.
