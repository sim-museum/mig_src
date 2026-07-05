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

## The map colour-fidelity finding (investigated 2026-07-05 — localized, NOT yet fixed)
The map renders **grey terrain with RGB speckle** instead of colour. Deep investigation **ruled out the
tiles, palette, and decode** — the bug is downstream in the composite/present path:
- Tiles are **8bpp, comp=0 BMPs**, W=H=256, blitted via `ma_gdi_stretch_dibits` with the embedded palette.
- **The tiles + palette are CORRECT and COLOURED.** Dumped a real tile's palette+indices from inside
  `ma_gdi_stretch_dibits` and reconstructed it offline with PIL → **beautiful colour**: green/olive terrain,
  blue rivers, red front-lines, dark-blue sea (only 21/256 palette entries are grey; pal[64]=(152,129,47),
  pal[100]=(170,129,33), etc.). So palette/decode are fine.
- **Not minification.** Traced the blits: every map tile draws **256×256 → 256×256, `minify=0` (1:1)** — no
  scaling, so no nearest-neighbour aliasing. (A box-filter minify path was prototyped for genuinely
  zoomed-out views but is inert at the default 1:1 load zoom — worth keeping for wheel-zoom-out later.)
- **`PalTrans` (`MIGVIEW.CPP:585`) is EMPTY** (body commented out) — ruled out. **`g_maPal` → all black** —
  ruled out.
- **The smoking gun:** the composited canvas centre is **exactly R=G=B=97** (pure grey) even though the
  1:1-drawn source tiles are colour. Exact channel-equality = a **desaturation / averaging** happening
  *between* the colour tile draw and the presented frame — i.e. in the mapdlg offscreen→screen blit or the
  present path, **not** in the tile decode.
- **Next step (dedicated session):** find the actual 2D map present/composite path (NOT `ma_gl_blit_bgra` —
  a `MA_DUMP_CANVAS` hook there never fired, so the map presents via a different route), dump the **mapdlg
  offscreen canvas** there: if it's colour → the offscreen→screen blit desaturates (fix that blit); if it's
  grey → the tiles are drawing into a grey/wrong-format offscreen (fix the map DC setup). The fix is a
  small, high-impact change once the desaturating step is pinned. (BoB's map uses a D3D7→GL FBO path, so its
  colour solution doesn't transfer.)

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
