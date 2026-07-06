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

### Phase-1 mechanism — fully mapped (2026-07-05 investigation; experiments reverted, tree clean)
Traced the whole hosting path so a focused continuation can implement it cleanly:
- **The map toolbar buttons are RButton** (CLSID `78918646`) — the SAME control the config panels use,
  **already hosted** by `ma_olebutton.cpp`. (The `461a1fe3`/REDTBT control is the `IDD_BASES` *dossier*
  buttons, a red herring — not the map chrome.)
- **The toolbars are `CRToolBar : CDialog`, created in `CMainFrame::Initialise()`** (`MAINFRM.CPP:284+`),
  and their RButtons register in the OCX host on create. Verified addresses: the bulk (~28 buttons, 24×24,
  laid out in a horizontal row at `cxy=(48,0),(72,0),…`) belong to **`m_toolbar1` (CMapFilters)**;
  `m_toolbar2` (CMainToolbar) has the rest. **They register with `m_maVisible=0` (parent CDialog hidden)**,
  so `ma_ole_draw_all` skips them (`if (parent && !parent->m_maVisible) continue`).
- **The map branch never composites hosted controls.** `ma_ole_draw_all` is called only in the FullPanelDial
  branch (`MIG.CPP:943`); the campaign-map branch (`MIG.CPP:957`, `m_currentpage==0`) draws the map but
  never calls it. So even a visible toolbar wouldn't draw.
- **Two blockers found when wiring it (both need solving for Phase 1):**
  1. **Stale-control bleed.** Making the toolbars visible + calling `ma_ole_draw_all` in the map branch also
     draws the **previous screen's controls** — the CLoad loadgame dialog's "Load Campaign"/"Auto Save"/
     "Back Load" are still registered+visible (not removed on the map transition) → they bleed onto the map.
     Fix: `ma_ole_remove_by_parent(<CLoad dialog>)` (or the panel's controls) when `LaunchMap`/`m_currentpage=0`
     happens, **or** a **targeted toolbar-only draw** that iterates just the `m_toolbar*` parents' controls
     instead of the global `ma_ole_draw_all`.
  2. **Button faces.** RButton `OnDraw` draws the button art via `NormalFileNum`; the toolbar icons are
     **sprite-sheet regions** (`IconsUI`/`ICON_PAGE_1`) per BoB shared-notes §8b — without that the buttons
     draw blank. Text-only bars (titlebar date, done directly in S47) sidestep this.

### ✅ Phase-1 DONE (S48, targeted toolbar draw) — Phase-2 (art) is the RButton WM_GETFILE pipeline
**Built + verified (env-gated `MA_MAP_TOOLBARS`, no default-map regression):**
- `ma_ole_draw_toolbar(dialog, screenHdc, ox, oy)` (`ma_olecontrol.cpp`) — **parent-scoped** draw: iterates
  only the hosted controls whose `parent == dialog` (no global `ma_ole_draw_all` → **no stale-control bleed**,
  BoB's blocker 1 solved). Wired in the map branch for `m_toolbar1`/`m_toolbar2`. **Verified via trace: 40
  toolbar RButtons drawn at correct positions** (30 filters + 10 main), no bleed, no crash.
- `ma_button_set_filenum(ctrl, fn)` (`ma_olebutton.cpp`) — reusable API to set a button's `NormalFileNum`
  (Phase-2 needs it for the control-id→icon table).
- **The control-id→icon table data is ready:** `IDC_BASES`→`FIL_ICON_BASES(0x6a63)`, `IDC_SQUADS`→
  `FIL_ICON_SQUADRONS(0x6a66)`, `IDC_WEATHER`→`0x6a69`, `IDC_DIS`→`0x6a6c`, `IDC_DIRECTIVES`→`0x6607`,
  `IDC_FRAG2`→`FIL_ICON_FRAG(0x6a96)` (1:1 by function; the button ids are tracked in `Hosted.id` via
  `ma_ole_set_id`). `F_GRAFIX.G` has no `FIL_xICON_*` skew, so per-file FileNums should serve directly.

### ✅ Phase-2 DONE (S49) — main toolbar renders per-button icons, default-on, ASan-clean
Root cause of the blank buttons: **`CRToolBar : public CDialog` doesn't inherit `RDialog::OnRowanMessage`**,
so the button's `SendMessage(WM_GETFILE)` (≥0x400 → `OnRowanMessage`) hit the base `CWnd` no-op (`return 0`)
— the icon never loaded. Fixes:
1. **`CRToolBar::OnRowanMessage` override** (`RTOOLBAR.CPP`, `MA_LINUX`) routing `WM_GETFILE`→`OnGetFile`
   (+ `WM_RELEASELASTFILE`, and `0` for `WM_GETARTWORK`/`XYOFFSET`/`GLOBALFONT`/`OFFSCREENDC`). Now the
   button `OnDraw`→`DrawBitmap`→`WM_GETFILE`→`CRToolBar::OnGetFile`→`fileblock`/`getdata` serves the art.
2. **Control-id→icon table** (`ma_olebutton.cpp` `ma_button_apply_icon`, applied in `ma_ole_draw_toolbar`
   from `Hosted.id`): `IDC_BASES`→`0x6a63`, `IDC_SQUADS`→`0x6a66`, `IDC_WEATHER`→`0x6a69`, `IDC_DIS`→`0x6a6c`,
   `IDC_FRAG2`→`0x6a96`, `IDC_PLAYERLOG`→`0x6a7b`, `IDC_OVERVIEW`→`0x6a7e`, `IDC_PACKAGES`→`0x6a75`,
   `IDC_AUTHORISE`→`0x6a78` (1:1 by function; all dir 0x6a).
3. **Fileblock hygiene in `OnGetFile`** (the map redraws every idle): **cache** the icon fileblocks (load
   once, never free → no per-frame churn / dtor thrash) + **guard the dir range** (`0x6800..0x7100`) — a
   FileNum in an unloaded dir (e.g. `FIL_ICON_DIRECTIVES=0x6607`, dir 0x66) made `fileman::makedirectoryname`
   `SayAndQuit`→`exit()`, which then tripped a **pre-existing static-teardown `Curve`/`Shape` new[]/delete
   mismatch** (102×). Guarding it returns NULL (blank button) instead of quitting.
- Default-on (`MA_NO_MAP_TOOLBARS` disables). **`asan_campaign` + `asan_flight` gates both PASS (0 reports).**

**Remaining (Phase 2b / 3):** filter-toolbar icons (28 buttons, `FIL_ICON_B_*`/`R_*` blue/red states — a bigger
table); the DIRECTIVES icon (dir 0x66 unloaded — find the right art or leave blank); positioning polish; then
**Phase-3 clicks** → `ma_evt_fire(toolbar, &typeid(*toolbar), ctrlId, 1)` → `OnClickedBases`/… (watch shared
`(dlgId,ctrlId)` per BoB S94); the pre-existing static-teardown `Curve` new[]/delete is a separate latent fix.

### (historical) Phase-2 investigation notes

### Recommended Phase-1 slice — now with BoB's drop-in recipe (they finished this exact epic, S88–92)
BoB confirmed my plan **is** their S88→S92 order and handed over concrete answers (cross-port note 4,
`port/CROSS-PORT-FROM-BOB-2026-07-05d.md`). Use them — skips most of the exploration:
1. **Targeted toolbar-draw** (solves the stale-control bleed): a `ma_ole_draw_toolbar(dialog, ox, oy)` that
   iterates **only the hosted controls whose `parent == that toolbar`** (never the global `ma_ole_draw_all`).
   ~15 lines. Add an **id-filtered variant** `ma_ole_draw_toolbar_ids(dialog, ox, oy, ids[], n)` (BoB needed
   it for the TitleBar, which hosts the date *and* accel buttons — I draw the date text myself (S47), so
   filter to just the accel/action button ids; also for the filters-row/main-row split).
   - **Gotcha:** RButton `OnDraw`→`DrawBitmap`→`SetDIBitsToDevice` may lose the CDC viewport (HDC is a
     sentinel) → blits to (0,0). BoB added a settable `setdibits_origin(sx,sy)` around each control's
     `OnDraw`. Check MA's `ma_gdi` `SetDIBitsToDevice` honours an origin (its `StretchDIBits` does).
2. **Position** the bars (filters/main/debrief rows); verify button **rects** land via frame capture (faces
   can be blank here).
3. **Sprite-sheet faces (Phase 2, the meaty part).** Resolve each `FIL_ICON_*` → its `ICON_PAGE` value =
   `ICON_PAGE_1 (0x10000) + <index into h/iconnum.g>`; set the button's `NormalFileNum` to that; `OnDraw`'s
   transparent branch (`filenum >= 0x10000`) draws the sheet region via `WM_GETFILE`→`IconDescUI`→`MaskIcon`
   — the **same path MA's map unit-icons already use** (S46). **The `.rc` DLGINIT defaults most buttons to
   ONE shared art string** (BoB: all → `FIL_ICON_BASES`) — reconstruct a **control-id→icon table** (1:1 by
   function: `IDC_SQUADS`→`ICON_SQUADRONS`, `IDC_WEATHER`→`ICON_WEATHER`, …) or every button shows the same
   icon. (My `F_GRAFIX.G` has **no** `FIL_xICON_*` skew — confirmed — so the ICON_PAGE route sidesteps BoB's
   per-file-art trap.)
4. **Clicks (Phase 3):** hit-test the drawn rect → `ma_evt_fire(toolbar, &typeid(*toolbar), ctrlId, /*Clicked*/1)`
   (the S18 eventsink) → `OnClickedBases`/…. **Watch shared control-ids** (BoB S94): the `.rc` rect table
   keyed by id-alone collides when an id is reused across dialogs → make the lookup **`(dlgId,ctrlId)`-aware**,
   fall back to by-id.
5. **Phase 4:** filter toggles + mission-folder/unit-select (OOB dossier dialogs — heed §8c's full-expression
   `Edges` idiom; and BoB's OOB-render blockers: `DoPaint` manages its own fileblock, off-screen
   `OnGetXYOffset` layout).

BoB offered its `bob_ole_draw_toolbar`/`draw_toolbar_ids` source verbatim if wanted.

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
