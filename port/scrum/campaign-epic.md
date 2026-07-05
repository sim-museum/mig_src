# Campaign epic — MA vs BoB comparison + roadmap

_Opened 2026-07-05. "There's a long way to go with campaign — compare with `~/bob`."_

BoB ran an 11-sprint campaign-map arc (S83→S93) and its strategic map now "mirrors the Wine reference
end-to-end": terrain + unit icons, right-edge Nm scale bar, wooden footer band, live event-log teletype,
date/time/accel readout, and **two rows of clickable `CRButtonCtrl` toolbar buttons**. MA is **ahead on the
map _view_** (pan/zoom/drag via the `ma_map_nav_*` SDL idle bridge; `StretchDIBits` tiles) but **behind on
everything around it**.

## Side-by-side

| Campaign feature | BoB | MA | Gap |
|---|---|---|---|
| Reach campaign map (load save → map) | ✅ | ✅ (S40 headless drive) | — |
| Terrain tiles render | ✅ colour (Wine-match) | ◐ **grey + speckle** | **fidelity (see below)** |
| Unit/airfield icons | ✅ | code exists (`CMIGView::DrawIcons`) — render unverified | wire + verify |
| Pan / zoom / drag | (msg-driven) | ✅ (idle SDL bridge) | MA ahead |
| Right-edge scale bar | ✅ (S83) | ❌ | add |
| Footer band + date/time/accel | ✅ (S84) | code exists (`TitleBar::Redraw` builds the date/period string) — not wired to render | wire |
| Event-log teletype (intel) | ✅ (S85) | ❌ | add |
| Toolbar buttons (bases/squadrons/pilots/mission-folder/…) | ✅ hosted `CRButtonCtrl`, drawn + **clickable** (S88–92) | classes exist (`CMainToolbar`/`MSCTLBR`/`RTOOLBAR`); RButton host exists (`ma_olebutton.cpp`); **not drawn on the map** | host on map (mirror BoB S88–92) |
| Unit-select / dossiers / mission-folder | (next BoB arc) | `OnLButtonDown` stock/unwired | wire click→select |
| Campaign loop (fly mission / advance day) | ✅ | ✅ **headless** (`MA_CAMP_FLY`/`MA_CAMP_NEXTDAY`, ASan-clean S41/42) | MA has the loop; needs the UI to drive it interactively |

## The map colour-fidelity finding (investigated 2026-07-05, scoped — NOT a quick fix)
The map renders **grey terrain with RGB speckle** instead of colour. Investigation:
- Tiles are **8bpp, comp=0 (uncompressed) BMPs**, W=H=256, blitted via `ma_gdi` `StretchDIBits` with the
  embedded palette (`DIB_RGB_COLORS`). Rendered at load-zoom via the `m_MapFiles` branch (ZOOMTHRESHOLD3),
  not the `FIL_MIDMAP` branch.
- **`PalTrans` (`MIGVIEW.CPP:585`) is EMPTY** — its body is fully commented out (only a `&0xf0f0f0 + 0x040404`
  brightness tweak), so the dropped `getdata(..., PalTrans)` arg was **not** the colour source. Ruled out.
- **`g_maPal` (the D3D/display palette) is NOT it** — forcing the tiles through it renders the terrain
  **all black** (g_maPal is unset/black during the 2D map). Ruled out.
- So the grey+speckle is a **decode discrepancy** (embedded-palette offset/stride, or the tiles genuinely
  ship a low-colour palette) — the terrain _shape_ is correct, so pixel indices+stride are ~right; the
  palette lookup is producing near-grey with scattered colour. **Next step to resolve:** dump a real
  `m_MapFiles` tile to disk (`MA_DUMP_TILE` hook, add to the ZOOMTHRESHOLD3 branch at ~`MIGVIEW.CPP:2406`,
  `fwrite(pData, bfSize)`) and read it with PIL **offline** — if PIL renders it colour, the bug is in
  `ma_gdi`'s 8bpp path (offset/`biSize`); if PIL renders it grey, the tile's palette is the issue and the
  real colour comes from a source not yet found. (BoB's map uses a D3D7→GL FBO path, so its colour solution
  doesn't transfer directly.)

## Proposed sprint backlog (prioritised: functional visibility first, then fidelity)
1. **S45 — map date/period readout** (mirror BoB S84): wire `TitleBar::Redraw`'s date/period string to
   render on the map (headless: frame-capture the text). Smallest concrete chrome win.
2. **S46 — verify + fix unit/airfield icons** (`DrawIcons`): confirm icons render on the map; fix if not.
3. **S47 — map toolbar buttons**: host `CMainToolbar`/`MSCTLBR` `CRButtonCtrl`s on the map (mirror BoB
   S88–92; MA already hosts RButton). Draw each map idle; then wire clicks → the real handlers (the S18
   eventsink seam, one layer up — like BoB S92).
4. **S48 — mission-folder / unit-select interaction**: click a squadron/airfield/mission → dossier/frag.
5. **S49 — map colour fidelity** (the investigation above) — deferred behind functional visibility since
   a grey relief map is usable; colour is polish.
6. **S50 — scale bar + event-log teletype** (mirror BoB S83/S85) — chrome polish.

Headless DoD throughout: frame-capture (`BOB_DUMP_FRAME`/`MA_DUMP_BACK` → PPM→PNG) for rendering; trace
markers (`MA_TRACE_OLE`/`MA_TRACE_3D`) for click→handler wiring. Campaign-drive recipe + `MA_IGNORE_SAVE_DATE`
from S40; `MA_CAMP_FLY`/`MA_CAMP_NEXTDAY` for the loop.
