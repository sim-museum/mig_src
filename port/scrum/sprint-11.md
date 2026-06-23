# Sprint 11 — M6: REdit control hosting + CLoad loadgame sub-dialog scoping

**Goal:** render the loadgame "CLoad" sub-dialog (IDD_LOADFULL) natively — the remaining
title-screen grind. Sprint-9 flagged it as needing "sub-dialog hosting + the un-hosted
REdit control."

## Outcome
- **REdit (CREditCtrl) OCX hosting — DONE.** The text-entry control (load/save savename
  field, visitorsbook, variant name) is now a first-class hosted control type.
- **CLoad sub-dialog fully scoped + present-gap precisely localized** (not yet a visible
  loadgame screen — the blocker is a render-path issue, characterized below).
- **No regression:** 3D launch stress 5/5; Preferences screen renders pixel-faithful
  (tabs + labels + combo values), confirming the OCX-router changes are safe.

## What landed

### REdit control hosting (the documented "Next")
- **`SRC/compat/ma_oleedit.cpp` (NEW)** — host glue for `CREditCtrl`, mirroring
  `ma_olestatic.cpp`: `ma_edit_create/set_string/setprop/getprop/draw`. Dispids per
  REDITCTL.CPP's map: 1 FontNum + stock ForeColor/Caption/Enabled. Text via `SetText`
  (→ `m_maText`, what `CREditCtrl::OnDraw` renders through `InternalGetText`).
- **`ma_olecontrol.cpp` router** — new `CT_EDIT` type: CLSID `0x499e2be6` → `ma_edit_create`;
  getprop/setprop/draw dispatch; `ma_ole_set_label` now also feeds edits a template default
  caption.
- **`port/rebuild.sh`** — new `oleedit` build mode (`-ISRC/REDIT`, afxctl prelude) compiling
  `ma_oleedit.cpp` + `REDIT/REDITCTL.CPP` into `objole/`.
- **Two reusable compat fixes** uncovered while compiling REDITCTL.CPP:
  - `afxwin.h`: `ON_WM_GETDLGCODE()` was the one message-map macro NOT stubbed → it leaked
    out of `CREditCtrl`'s BEGIN/END_MESSAGE_MAP as a bare namespace-scope token and broke
    the next declaration. Added the empty stub (next control that uses it just works).
  - `compat_winuser.h`: added the `DLGC_WANT*` / `DLGC_*` WM_GETDLGCODE return flags.
- **Verified:** with `MA_TRACE_OLE`, the savename control now registers as `type=5` (CT_EDIT)
  and `draw_all` dispatches it each frame (origin (40,189), 202×26, vis=1) — previously it
  fell to `CT_OTHER` (a no-op).

### CLoad sub-dialog: fully scoped
Clicking title-menu row 3 (loadgame, click `588,260`) reaches it. `IDD_LOADFULL` (resource
999) parses, `CLoad::OnInitDialog` runs, and the dialog instantiates exactly:
RButton (FILEOK) ✓hosted, RListBox (file list, **count=1** = the Auto Save) ✓hosted,
RStatic (LOADNAME) ✓hosted, **REdit (SAVENAME)** ✓hosted *(this sprint)*, RScrlBar ×2
✗ (`0x505aee46`, the file-list scrollbar — still unhosted). `loadsave.bmp` (the loadgame
background) loads.

## The remaining gap — CORRECTED in Sprint 12 (see sprint-12.md)
> **Correction (Sprint 12, 2026-06-23):** the original claim here — that the loadgame screen
> "does not present" and shows the title — was WRONG, due to a background misidentification.
> `loadsave.bmp` (the loadgame background) is a photo of two pilots by a truck; I mistook it
> for the title. The real title is the blue "Mig Alley" jet splash (`title.bmp`). **The
> loadgame screen DOES render**: `loadsave.bmp` background + the CLoad controls (the
> "Load Campaign:" labelled field is visible). The actual remaining gap is the **file-list
> (RListBox) rows not being visibly rendered** (it draws with count=1, but the single
> "Auto Save" entry isn't visible on the photo). The FullPanelDial pointer is reused across
> screens (that's why `fp` looked constant) — it is NOT evidence of a failed transition.

## Diagnostics (gated, default off)
`MA_TRACE_EDIT` (new — REdit prop sets), `MA_TRACE_OLE` create-cap raised 30→400 (so a
sub-dialog's controls are all visible in the create/draw trace), `MA_TRACE_DLG`,
`MA_TRACE_PAGE`, `BOB_DUMP_FRAME=N BOB_EXIT_AFTER_DUMP=1`.

## Build / rebuild note
New `oleedit` objects (`ma_oleedit.o`, `REDITCTL.o`); touched `ma_olecontrol.cpp` (router),
`afxwin.h` + `compat_winuser.h` (compat). Recompile those + relink. `afxwin.h` is force-included
across MFC TUs → a from-scratch MFC rebuild is the safe path for the macro change (the link
here recompiled only the affected objects + relinked; full rebuild deferred to next from-scratch).

## Next (Sprint 12 candidates)
- Drive the LaunchDial child (CLoad) paint into the present path → first visible loadgame
  screen; then host RScrlBar (`0x505aee46`) + wire the CLoad eventsink (file select → Load).
- Backlog: campaign save/resume E2E; broader 3D fidelity A/B vs Wine.
