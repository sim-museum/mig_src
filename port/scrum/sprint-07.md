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

## M4 increment 2 — campaign mission → briefing (frag) chain
Drove the campaign map → mission → briefing → (fly) path:
- **Found the chain:** the map's frag button `CMainToolbar::OnClickedFrag2()` (= `MainToolBar()`,
  `m_toolbar2`) → when a flyable mission exists → `LaunchFullPane(&singlefrag)`; **singlefrag is a
  FullScreen panel** (the existing render system) with a **Fly** button → `FragFly` → `StartFlying`
  (the M1 3D-flight path) → `OnFlyingClosed` (CAMP branch) → back to map. Gated drive `MA_CAMP_FLY`
  (stage 1: frag→singlefrag; stage 2: singlefrag→Fly).
- **The campaign briefing (singlefrag) renders natively** with **real mission data**: flight callsign
  combos (Viper/Rattler/Cobra/Condor/Tiger/…), pilot lines, Map/Fly/Preferences action bar.
  `FlyableAircraftAvailable()` is TRUE — the campaign generated the player's mission.
- **Fixes (general robustness, not gated):**
  - `OnGetGlobalFont` (FULLPANE.CPP): bound-check `fontnum` — a freshly-hosted listbox's `m_FontNum2`
    can be garbage → `g_AllFonts[huge]` out-of-bounds crash. Clamp to a valid font.
  - **DialBox copy ctor** (RDIALOG.H): the `cond?DialBox(...):*ND` ternaries copy `*(DialBox*)NULL`
    (the "no-dialog" sentinel) on GCC; the stock ctor crashed dereferencing it AND never copied the
    `diallist[]` child array (which `AddChildren` always recurses into → garbage). Now null-guarded
    (empty terminator) + fully copies `diallist`. Generalises the pattern (campaign FragInit pilot
    lines when numopts==0).
  - **Canvas clear on map→panel** (`ma_gdi_clear_screen` + MIG.CPP): the map fills the whole canvas;
    clear it once when a panel takes over so a non-full-screen panel doesn't show the map through.
- **Open (intermittent):** the frag→singlefrag→Fly path hits a file-resolution `SysError`
  ("File number past end of Dir.Dir", `Fileman.cpp:935`) on some runs — a campaign-asset FileNum out
  of range; when it doesn't fire, singlefrag renders fully. Needs tracing which FileNum/dir (likely a
  mission/briefing graphic computed from campaign period data). Then the Fly→3D campaign-flight
  round-trip (reuse M1) + the map toolbar/icon UI + campaign save/load.

No regression: all M4 work is gated (`MA_ENABLE_MAP`/`MA_CAMP_FLY`, default-off); default front-end +
flight round-trip clean, 3D stress 2/2.

## M4 increment 3 — ★ FLYABLE CAMPAIGN MISSION (5/5 reliable)
Closed the inc2 blocker and reached a flyable campaign mission end-to-end:
- **Root cause of the intermittent SysError:** a freshly-hosted campaign **frag button** has an
  uninitialised bitmap-FileNum OCX property -> `CRButtonCtrl::DrawBitmap` requests a garbage FileNum
  (e.g. 0x2B0F03E1, high bits set) via `WM_GETFILE` -> `RDialog::OnGetFile` -> `fileblock(garbage)` ->
  `Fileman::namenumberedfile` "past end of Dir.Dir" -> fatal `EmitSysErr`/`SayAndQuit`/exit (and a
  flaky teardown). Traced via `MA_TRACE_FILENUM` + a gdb backtrace.
- **Fix (general, not gated):** (1) `RDialog::OnGetFile` rejects FileNums with high bits set
  (`>0xFFFF`; valid = `(dir<<8)|index`, dirs 0..113 <= ~0x7100) -> the button just skips its bitmap;
  (2) `Fileman::namenumberedfile` past-end path returns "" (not the fatal SysError, and not "//" which
  re-opens DIR.DIR) as a backstop.
- **Result: a campaign mission now FLIES in 3D natively.** Full chain (gated `MA_ENABLE_MAP`
  +`MA_CAMP_FLY`): title -> Single Player -> Campaign -> Begin -> operational map -> frag button
  (`OnClickedFrag2`) -> **singlefrag briefing** -> **Fly** (`FragFly`->`StartFlying`) ->
  `Launch3d` -> **3D campaign flight** (F-86 cockpit, terrain, horizon, gunsight HUD, "NO HAND HOLD"
  instrument). **Validated 5/5** campaign 3D launches; **no regression** (QM round-trip clean, title
  100%, 3D stress 4/4).
- **Next:** the campaign flight->exit->debrief->map round-trip (the campaign `OnFlyingClosed` branch
  goes to `LaunchMap`); then the real map toolbar/target-SELECTION UI to replace the gated drives
  (so the mission data inits naturally instead of via the frag-button shortcut) + map icons (DrawIcons)
  + campaign save/load. Map/3D palette colour shared with M2.
