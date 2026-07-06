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
| Terrain tiles render | ✅ colour (Wine-match) | ✅ **colour** (S45: "grey" was a frame-dump bug, not the render) | — (parity) |
| Unit/airfield icons | ✅ | ✅ **render (S46)** — airfield/squadron/supply markers + front-line + routes | — (parity) |
| Pan / zoom / drag | (msg-driven) | ✅ (idle SDL bridge) | MA ahead |
| Right-edge scale bar | ✅ (S83) | ❌ | add |
| Date/period readout | ✅ (S84) | ✅ **renders (S47)** — "M/D/YY: <period>, <phase>" drawn on the map | — |
| Footer band + accel readout | ✅ (S84) | ❌ | part of the CRToolBar epic |
| Event-log teletype (intel) | ✅ (S85) | ❌ | add |
| Toolbar buttons (bases/squadrons/pilots/mission-folder/…) | ✅ hosted `CRButtonCtrl`, drawn + **clickable** (S88–92) | classes exist (`CMainToolbar`/`MSCTLBR`/`RTOOLBAR`); RButton host exists (`ma_olebutton.cpp`); **not drawn on the map** | host on map (mirror BoB S88–92) |
| Unit-select / dossiers / mission-folder | (next BoB arc) | `OnLButtonDown` stock/unwired | wire click→select |
| Campaign loop (fly mission / advance day) | ✅ | ✅ **headless** (`MA_CAMP_FLY`/`MA_CAMP_NEXTDAY`, ASan-clean S41/42) | MA has the loop; needs the UI to drive it interactively |

## ✅ RESOLVED (2026-07-05, S45): the map ALWAYS rendered in full colour — the "grey speckle" was a frame-dump bug
**There was no map rendering bug.** The strategic map renders as a clean, full-colour Korean peninsula
(green terrain, blue rivers, red front-lines, sea, yellow highlands — verified by a direct `g_canvas` dump
AND a fixed `BOB_DUMP_FRAME` capture). The long-standing "greyish/speckled map" fidelity gap (S7/S14/S20
notes) was entirely an artifact of the **`BOB_DUMP_FRAME` diagnostic**:
- `present_dbg` does `glReadPixels(0,0,w,h,GL_RGB,GL_UNSIGNED_BYTE,buf)` then writes a PPM with `w*3`
  bytes/row. The default `GL_PACK_ALIGNMENT` is **4**, so GL pads each row up to a 4-byte multiple. For the
  **1021-wide** campaign map, `1021*3 = 3063` is **not** divisible by 4 → GL emits 3064-byte rows while the
  writer reads 3063 → **each row shifts 1 byte**, cumulatively rotating R/G/B into channel-shift noise that
  reads as grey-speckle. The 800-/640-wide front-end + flight frames are 4-divisible, so they always dumped
  clean — which masked the bug and made the map look uniquely "broken."
- **Fix (S45, `bob_video.cpp`):** `glPixelStorei(GL_PACK_ALIGNMENT, 1)` before the `glReadPixels` calls in
  `present_dbg`. Now captures at any width are pixel-accurate. 2-line change; flight capture unaffected.
- **Consequence:** MA's campaign map is at **colour parity** with BoB (not behind). The comparison row below
  is updated. All prior "map is greyish" assessments in STATUS/sprint docs were this artifact.

### (historical) The investigation that led here — ruled out tiles/palette/decode
The map appeared to render **grey terrain with RGB speckle**. Investigation **ruled out the
tiles, palette, and decode**, then localized the "desaturation" to the present path — which turned out to
be the `glReadPixels` alignment bug above, not the display:
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

## Milestone (S45–S46): the strategic-map DATA layer is at parity with BoB
Terrain (full colour, S45), unit/airfield icons + front-line + routes (S46) all render. What remains is the
**chrome + interaction** — and the chrome is one coherent piece of new infrastructure:

### The remaining work is the CRToolBar-hosting epic (mirror BoB S88–92)
The titlebar (date/period readout), the main toolbars, and the filter/action buttons are **all `CRToolBar`
docking windows** hosting `CRButton`/`CRStatic` controls — e.g. `TitleBar : CRToolBar` with `IDC_DATE` a
text `CRButton` set via `SetString(GetDateName(...))`. The port hosts **FullPanelDial** panels
(`ma_ole_draw_all`) but does **not** yet render `CRToolBar`s on the map. Hosting them is new infra:
- draw each docked `CRToolBar`'s child `CRButton`/`CRStatic` (position from the `IDDT_*TOOLBAR`/titlebar
  templates) over the map each idle — the same host→`OnDraw` path as the config panels, one layer up;
- **text buttons** (titlebar date, IDC_DATE) are tractable first (no art); **icon buttons** need the
  sprite-sheet (`IconsUI`/`ICON_PAGE_1`) art path BoB documented in shared-notes §8b;
- then wire clicks → the real `ON_EVENT` handlers (the S18 eventsink seam, like BoB S92).

This is the natural next epic (several sprints). The map is already usable/legible without it.

## Proposed sprint backlog (prioritised: functional visibility first, then fidelity)
1. **S45 — map date/period readout** (mirror BoB S84): wire `TitleBar::Redraw`'s date/period string to
   render on the map (headless: frame-capture the text). Smallest concrete chrome win.
2. ~~**S46 — verify + fix unit/airfield icons**~~ — ✅ **DONE (S46)**: icons never drew because `DrawIcons`
   took its view bounds from the compat `CDC::GetBoundsRect` (returned garbage + `DCB_SET` → an overflowed
   world rect that excluded all 652 items). Fixed to use `GetClientRect` (like `UpdateBitmaps`). Also
   defaulted the standard map view-filters ON (the filter toolbar isn't hosted yet, so they were all-off →
   no icons). Map now shows airfield/squadron/supply markers + front-line + routes. ASan-clean.
3. **S47 — map toolbar buttons**: host `CMainToolbar`/`MSCTLBR` `CRButtonCtrl`s on the map (mirror BoB
   S88–92; MA already hosts RButton). Draw each map idle; then wire clicks → the real handlers (the S18
   eventsink seam, one layer up — like BoB S92).
4. **S48 — mission-folder / unit-select interaction**: click a squadron/airfield/mission → dossier/frag.
5. ~~**S49 — map colour fidelity**~~ — ✅ **DONE (S45)**: was a `BOB_DUMP_FRAME` `glReadPixels` alignment
   bug, not a render issue; the map was always in colour. No further work.
6. **S50 — scale bar + event-log teletype** (mirror BoB S83/S85) — chrome polish.

Headless DoD throughout: frame-capture (`BOB_DUMP_FRAME`/`MA_DUMP_BACK` → PPM→PNG) for rendering; trace
markers (`MA_TRACE_OLE`/`MA_TRACE_3D`) for click→handler wiring. Campaign-drive recipe + `MA_IGNORE_SAVE_DATE`
from S40; `MA_CAMP_FLY`/`MA_CAMP_NEXTDAY` for the loop.
