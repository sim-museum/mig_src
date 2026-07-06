# Sprint 49 — "CRToolBar Phase-2: main toolbar icons render" (autonomous, headless DoD)

**Goal:** make the campaign map's main toolbar buttons render their icons (blank after S48's Phase-1 draw).

## Root cause
`CRToolBar : public CDialog` does **not** inherit `RDialog::OnRowanMessage`. `CWnd::SendMessage(WM_GETFILE)`
(≥0x400) dispatches to `OnRowanMessage`, but the toolbar's was the base `CWnd` no-op (`return 0`) — so the
button `OnDraw`→`DrawBitmap`→`SendMessage(WM_GETFILE)` art fetch returned 0 → blank buttons. (`CRToolBar`
*has* `OnGetFile`; nothing routed to it.)

## Fixes
1. **`CRToolBar::OnRowanMessage`** (`RTOOLBAR.CPP`/`RTOOLBAR.H`, `MA_LINUX`): routes `WM_GETFILE`→`OnGetFile`,
   `WM_RELEASELASTFILE`→`OnReleaseLastFile`, `WM_PLAYSOUND`→`OnPlaySound`, and 0 for
   `WM_GETARTWORK`/`XYOFFSET`/`GLOBALFONT`/`OFFSCREENDC` (mirrors `RDialog::OnRowanMessage`).
2. **Control-id→icon table** (`ma_olebutton.cpp` `ma_button_apply_icon`, applied in `ma_ole_draw_toolbar`
   from `Hosted.id`): `IDC_BASES`→`FIL_ICON_BASES(0x6a63)`, `IDC_SQUADS`→`0x6a66`, `IDC_WEATHER`→`0x6a69`,
   `IDC_DIS`→`0x6a6c`, `IDC_FRAG2`→`0x6a96`, `IDC_PLAYERLOG`→`0x6a7b`, `IDC_OVERVIEW`→`0x6a7e`,
   `IDC_PACKAGES`→`0x6a75`, `IDC_AUTHORISE`→`0x6a78`. (The `.rc` DLGINIT doesn't differentiate them — BoB §8b.)
3. **`OnGetFile` hygiene** (the map redraws every idle): **cache** the icon fileblocks (load once, never free)
   + **guard the dir range** `0x6800..0x7100`. A FileNum in an unloaded dir (`FIL_ICON_DIRECTIVES=0x6607`,
   dir 0x66) made `fileman::makedirectoryname` `SayAndQuit`→`exit()`, which then tripped a **pre-existing**
   static-teardown `Curve`/`Shape`/`fileman` new[]/delete mismatch (102 ASan reports). Guarding returns NULL
   (blank button) instead of quitting.

## Validation (headless DoD)
- Frame capture: the main toolbar renders as **distinct per-button icons** (Bases hangar, Weather globe, Dis
  calendar, Frag lightning, …) over the colour map — default-on (`MA_NO_MAP_TOOLBARS` disables).
- **`port/asan_campaign.sh` PASS** (0 reports, 2/2 map) — was 102+ before the guard. **`port/asan_flight.sh`
  PASS** (no regression from the shared compat edits). Compiles clean; links.

## Increment
The campaign map now has the **main action toolbar** (Bases/Squadrons/Packages/Dis/Weather/Frag/…) rendering
as real icon buttons — the core of BoB's S88–92 chrome, at visual parity for the main bar.

**Next:** filter-toolbar icons (Phase 2b, blue/red state table), DIRECTIVES art, then **Phase-3 clicks**
(`ma_evt_fire`→`OnClickedBases`…, watch shared `(dlgId,ctrlId)` per BoB S94); + the latent Curve
static-teardown new[]/delete (separate).
