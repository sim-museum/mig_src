# Sprint 12 — M6 loadgame: correction + real gap localized (file-list rows)

**Goal (as planned):** drive the CLoad child-dialog paint into the present path so the
loadgame screen renders. **Finding:** that goal was based on a false premise — the loadgame
screen *already renders*. This sprint corrects the record and localizes the genuine gap.

## Correction to Sprint 11 (important)
Sprint 11 concluded the loadgame screen "does not present" (shows the title). **That was
wrong** — a background misidentification:
- `loadsave.bmp` (the loadgame screen's background) is a **photo of two pilots by a truck**.
- The real **title** is the blue **"Mig Alley" jet splash** (`title.bmp`) with the right-side
  menu (Preferences / Single Player / Multi-Player / Load Game / Replay / Credits / Quit).

I had been calling every loadgame dump "title". Verified by rendering both BMPs directly and
by the no-click title frame. **The loadgame screen renders correctly:** `loadsave.bmp`
background + the CLoad controls. The **REdit** hosted in Sprint 11 renders visibly (a
black field with white border + text at (40,189) — the savename area). The FullPanelDial
pointer being constant across screens (the Sprint-11 "evidence") is just pointer reuse, not
a failed transition.

## The genuine remaining gap — file-list rows render invisibly
The CLoad file list (`IDC_RLISTBOXFILE`, a `CRListBoxCtrl` at (10,102), 294×260) draws each
frame with **count=1** (the one save, "Auto Save"), but its row content is **not visible** on
the canvas — the region shows only the photo. By contrast the title **menu** listbox (same
control class) renders its rows fine.

The difference is how each is populated:
- Menu listbox: `AddString("Preferences")` … — simple single-column rows. **Renders.**
- File list (`CLoad::MakeFileList`, LOAD.CPP:261): `AddColumn(200)` + `InsertRow(r)` +
  `ReplaceString(name, r, 0)` + `SetHilightRow(0)` — **column-based** rows. **Row invisible.**

All those methods are wired to the real `CRListBoxCtrl` in the router (`ma_olecontrol.cpp`
F_AddColumn/F_InsertRow/F_ReplaceString/P_HilightRow), and `OnDraw` is the real control's —
so the row data is stored, but the column-path render produces no visible text where the
AddString path does. **Hypotheses to check next (Sprint 13):** (a) the highlighted row
(`SetHilightRow(0)`) draws a select-colour fill that hides the text; (b) the column-cell text
colour/position differs from the simple-string path; (c) the column rendering needs column
metrics the port doesn't set. A focused `CRListBoxCtrl::OnDraw` column-vs-string-path
comparison (the menu listbox works as the reference) should isolate it quickly.

## Status
- **REdit hosting (Sprint 11) validated visibly** — the savename field renders on the
  loadgame screen.
- **loadgame screen renders** (background + controls); only the file-list row text is not yet
  visible. Remaining for a fully-usable loadgame: file-list row visibility + the
  file-select→Load eventsink (so picking a save and clicking Load loads it) + RScrlBar host
  (only needed when the save list exceeds the visible rows).
- No code change landed this sprint (it was a measurement/correction sprint); no regression.

## Next (Sprint 13)
Fix the column-based listbox row rendering (file list shows "Auto Save"); then wire
file-select→Load and the Back/Load menu, completing the loadgame screen.
