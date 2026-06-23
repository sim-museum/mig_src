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

## Deeper trace (added `MA_TRACE_LIST`, gated) — narrowed further
Instrumented `CRListBoxCtrl::OnDraw` (`MA_TRACE_LIST`: entry, params, per-row draw coords):
- The **title menu** listbox renders its 7 rows correctly (white text, direct-to-pdc).
- The **file-list** listbox **never reaches the row-render code** — its `OnDraw` either
  early-returns or isn't invoked for that frame. Two early-return points before the row loop:
  `if (m_bDrawing) return;` (note: `m_bDrawing` is a **static** member shared across ALL
  listbox instances — a re-entrancy hazard) and `if (artnum && WM_GETOFFSCREENDC==NULL)
  return;`. The menu's parent returns 0 for `WM_GETARTWORK` (RDIALOG.CPP:1971, direct path);
  if CLoad returns a non-zero artnum, the file list takes the **offscreen-DC path** and
  bails when `WM_GETOFFSCREENDC` yields NULL.
- Also observed: loadgame navigation is somewhat **nondeterministic** (some runs fully build
  the CLoad child + file list, some don't) — to be hardened alongside the render fix.

## Sharpened (3-run check) — file-list `OnDraw` is never called
Ran the loadgame nav 3× with `MA_TRACE_LIST`: every run parses IDD 999 (CLoad built), but
**only one listbox instance ever enters `OnDraw`** — the title menu listbox (still showing
its 7 items; the title menu persists under the CLoad overlay). The **file-list listbox never
enters `OnDraw` at all**, so the gap is NOT an internal early-return — `ma_ole_draw_all` is
**skipping it** on its visibility gate (`!clientWnd->m_maVisible` / `!parent->m_maVisible`).
Yet a sibling CLoad control (the REdit savename, "Load Campaign:" field) DOES render — so the
CLoad parent is visible; it's the **file-list listbox client window that isn't `m_maVisible`**
(template-positioned listbox not being shown, unlike the game-positioned menu listbox and the
sibling template controls).

## Next (Sprint 13)
(a) Trace each hosted control's `m_maVisible` in `ma_ole_draw_all` (incl. skipped ones) to
confirm the file-list listbox client is the one hidden; (b) ensure the template-positioned
listbox gets shown (DDX_Control / ShowWindow path) so draw_all dispatches its `OnDraw`;
(c) then the file list shows "Auto Save"; (d) wire file-select→Load + the Back/Load menu;
(e) harden the loadgame nav nondeterminism. Gated diagnostic `MA_TRACE_LIST` in place.
