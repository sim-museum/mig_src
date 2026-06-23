# Sprint 13 — M6 loadgame: file list now shows save games ("Auto Save")

**Goal:** make the CLoad file list render its rows. **Outcome: DONE** — the loadgame file
list now visibly shows the save game ("Auto Save"). Root-caused and fixed a real
scroll-position bug; no regression.

## Root cause (after peeling back several trace-cap artifacts)
The file-list row *was* being drawn all along — at **y = -rowheight**, scrolled off the top
of the listbox and clipped away. `MA_TRACE_LIST` at the row `ExtTextOut`:
`str="Auto Save" at(1,-32) ... vscroll=32`.

Why `m_lVertScrollPos==32` for a 1-row list that obviously fits:
`CRListBoxCtrl::UpdateScrollBar()` runs at **populate time** (`CLoad::MakeFileList` →
`AddColumn`/`InsertRow`/`ReplaceString`), but the **Linux port sizes the listbox client
window only at draw time** (`ma_ole_draw_all` sets `m_maW/m_maH` per frame). So at populate
time `GetClientRect` returns a zero/stale size, `UpdateScrollBar` wrongly concludes the
content overflows (`m_vert=16`), computes `maxvalue = GetCount()*tmHeight - clientHeight + …`
≈ 32, and clamps `m_lVertScrollPos` to it (RLISTBXC.CPP:1330-1337). At draw time the row is
then offset to y=-32.

> **Trace-cap lesson:** several earlier "the file list never draws / isn't registered"
> conclusions were artifacts of `static int n; if(n++<N)` trace caps being **exhausted by
> the title menu listbox** (drawn every frame) *before* navigation. Gating the listbox traces
> on `rows!=7` (skip the 7-item menu) revealed the file list was reaching the row loop all
> along. Lesson recorded for future per-frame-control debugging: gate on identity, not a count.

## Fix (faithful, localized, `MA_LINUX`)
In `CRListBoxCtrl::OnDraw`, right after the row height (`tm.tmHeight`) is finalized, re-clamp
`m_lVertScrollPos` to the valid scroll range computed from the **real draw-time** `rcBounds`:
`maxScroll = max(0, GetCount()*tmHeight - visibleHeight)`. A list that fits → scroll 0; a
genuinely overflowing list is unaffected (its `maxScroll` is large). This self-corrects the
populate-time miscalculation without touching the scrollbar logic.

## Validation
- File list row trace after fix: `str="Auto Save" at(1,0) ... vscroll=0` — and the frame dump
  shows **"Auto Save"** rendered (white) at the top of the file list.
- The loadgame menu ("Back"/"Load") and all other lists unaffected (they already had vscroll=0).
- **No regression:** 3D launch stress 4/4; Preferences renders pixel-faithful (tabs + labels +
  combo values e.g. "640 X 480").

## Diagnostics added (gated `MA_TRACE_LIST`, default off)
`CRListBoxCtrl::OnDraw` entry/params/per-row-coords + offscreen-bail, and a `draw_all`
CT_LISTBOX visibility line — a coherent listbox-render diagnostic suite for the remaining
loadgame work.

## Status / remaining for a fully-usable loadgame
- ✅ loadgame screen renders: loadsave.bmp bg + CLoad controls + REdit savename + **file list
  with save names**.
- Remaining: wire **file-select → Load** (click a save row → `OnSelectRlistboxfile` →
  `OnClickedFileok`/`OnOK` → load the game) and the **Back/Load** menu; host **RScrlBar**
  (only needed when saves exceed the visible rows); harden the loadgame nav nondeterminism.

## Next (Sprint 14)
Wire the loadgame eventsink (file-select + Load button → actually load the selected save),
validate a load round-trip, then credits/replay screens.
