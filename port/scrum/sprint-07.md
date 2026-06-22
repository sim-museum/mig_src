# Sprint 7 — M4 campaign flow (increment 1: reach + render the operational map)

**Goal:** drive the single-player **campaign** flow — campaign select → operational map → (mission →
briefing → fly → debrief). Increment 1 = reach the campaign and **render the operational Korea map**.

## What shipped (M4 inc1)
- **Campaign flow mapped + validated end-to-end to the map:** title → Single Player → Campaign
  (`SetCampState`, gamestate=CAMP) → `campaignselect` (phases + dates + Back/Film/Background/
  Objectives/Begin action bar) → **Begin** (`LaunchMapFirstTime` → `LaunchMap` → `m_pView->LaunchMap`)
  → the operational map. No crash through the whole path.
- **`StretchDIBits` implemented** (`ma_gdi.cpp` `ma_gdi_stretch_dibits`, wired in `bob_dx_extra.h`):
  was a no-op stub, so the map never drew. Now a real nearest-neighbour scaled DIB blit with a source
  sub-rect (8/24/32-bpp + BI_RLE8), reusing the SetDIBitsToDevice decode. This is the primitive the
  map (and other scaled-bitmap UI) needs.
- **★ The campaign operational MAP renders natively** — the recognizable **Korea peninsula**
  (coastline, terrain, front-line/route), drawn by `CMIGView::UpdateBitmaps` StretchDIBits-ing the
  scrolled/zoomed `FIL_MIDMAP` tiles into the screen canvas. Reached via the real
  title→SP→Campaign→Begin click path. (`m_mapdlg.GetDC()` resolves to the screen canvas in compat, so
  the map's tile blits land on the presented canvas.)
- **Idle-loop map drive** (`MIG.CPP`, gated `MA_ENABLE_MAP`, default-off): when the campaign map is
  active (`LaunchMap` sets `m_currentpage=0`, `m_pfullpane=NULL` so the panel render path is skipped),
  no WM_PAINT fires, so drive `UpdateBitmaps(GetDC())` each idle + present. Gated off until the map UI
  is complete (so the front-end/flight default path is unaffected).
- **Diagnostics** (`ma_olecontrol.cpp`, gated `MA_TRACE_CLICK`/`MA_TRACE_OLE`): button rects + listbox
  rect/hit traces — used to decode the menu nav coords (title→SP=(588,231); SP→Campaign=(588,248);
  campaignselect action bar rect=(20,560,296,18), Begin=col4≈(270,568)).

Validated: campaign map renders 100% non-black (Korea visible), no crash, exit 0. **No regression:**
the `MA_ENABLE_MAP` gate is off by default → front-end + flight round-trip clean, 3D stress 3/3.

## Scope finding — the operational-map UI is the M4 bulk
The campaign mission→briefing→fly path routes entirely through the operational map + `CMainFrame`
toolbars (`MAINTBAR.CPP`/`MMC.NextMission`) — there's no bypass. The map is a large UI subsystem
comparable to the Phase-4 front-end: the scrollable/zoomable Korea map (now rendering), plus
airfield/unit **icons** (`DrawIcons`), the **toolbars** (mission log / directives / map filters), map
**interaction** (click a target → plan/select a mission), the **briefing**, then `StartFlying` (reuse
M1) → debrief → back to map. Increment 1 delivered the map render + the flow reaching it.

## ➡ Next (M4 inc2+)
- Map **colour/palette** (tiles render greyish — same palette/565 question as the 3D sky, M2).
- **Icons + toolbars** (DrawIcons airfields/units; CMainFrame toolbar render + interaction).
- **Mission select → briefing → fly → debrief → map** round-trip (reuse M1 flight + flight-close).
- Campaign **save/load** + registry-to-disk persistence.
