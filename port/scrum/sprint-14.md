# Sprint 14 — M6 loadgame: file-select → Load works (loads a save → campaign map)

**Goal:** wire the loadgame flow so selecting a save and clicking Load actually loads it.
**Outcome: DONE, end-to-end.** Selecting "Auto Save" → Load → `CFiling::LoadGame` succeeds →
the **campaign operational map of Korea renders**. Crash-free.

## What was missing
The loadgame screen rendered (Sprint 11-13) but clicks on the **CLoad file list** went
nowhere: the port's idle click handler (`MIG.CPP`) only hit-tested the FullPanelDial's *menu*
listbox (`m_IDC_RLISTBOX`), so `CLoad::OnSelectRlistboxfile` never ran and `selectedfile`
stayed empty → `DoLoadGame` bailed at its `selectedfile==""` guard.

Confirmed the data path was otherwise ready: `CLoad::filename` is a **reference**
(`CString& filename`, LOAD.H:83) bound to the FullPanelDial's `selectedfile`, so selecting a
file writes it back automatically; and the Back/Load menu is the FullPanelDial's own menu
listbox (already click-wired → `OnSelectRlistbox` → `DoLoadGame`).

## Implementation
- **`ma_olecontrol.cpp` `ma_ole_listbox_click(sx,sy)` (NEW):** hit-tests every hosted,
  template-placed listbox that has a dialog id (the DDX_Control'd child-dialog listboxes like
  CLoad's `IDC_RLISTBOXFILE` — the menu listbox has id 0 and is handled separately). On a hit
  it resolves row/col via `MaMouse` and fires the owning dialog's **Select** event (dispid 1,
  args via `ma_evtA0/A1`) through the existing RTTI eventsink → `CLoad::OnSelectRlistboxfile`.
- **`MIG.CPP` idle click chain:** menu listbox → else **`ma_ole_listbox_click`** (child-dialog
  lists) → else `ma_ole_click` (buttons/combos). Ordered so the menu and buttons are unaffected.
- **`afxwin.h`:** declared `ma_ole_listbox_click`.

## Validated end-to-end (`MA_TRACE_OLE`)
```
[click] file-listbox id=1055 hit local=(70,10) -> row=0 col=0
[evt_fire] id=1055 dispid=1 type=...CLoad -> HANDLER CALLED     <- OnSelectRlistboxfile ran
[OnSelectRlistbox] row=0 col=1 x=1 ... onselect=1              <- Load menu item -> DoLoadGame
[DoLoadGame] selectedfile="Auto Save.sav"
[DoLoadGame] CFiling::LoadGame("Auto Save.sav") -> 1           <- save LOADED
[map] render operational map (page=0)                         <- campaign map (Korea) renders
```
Frame dump after load shows the Korean-peninsula operational map. Exit clean (no crash).

- **No regression:** 3D flight path (title→Single Player→Hot Shot→3D) stress **4/4**;
  Preferences combos still populate/cycle (the new listbox-click only fires when a click lands
  inside a child-dialog listbox that has a registered Select handler — buttons/combos fall
  through untouched).

## Notes / remaining
- Clicking the **same** file row twice routes to `OnSelectRlistboxfile` → `OnOK()` (the
  direct-confirm path) — also wired by the same change.
- The post-load campaign map renders structurally but greyish — the known separate M4/M8
  map-tile **colour-fidelity** item, not a loadgame issue.
- Gated diagnostics added/kept: `MA_TRACE_OLE` (`[DoLoadGame]`), `MA_TRACE_CLICK`
  (`[click] file-listbox …`).

## Next (Sprint 15)
Loadgame is functionally complete. Next front-end screens: credits / replay (the remaining
title sub-dialogs); or pivot to the M4/M8 map-tile colour fidelity now that load→map works.
