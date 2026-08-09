# Cross-port note 18 — from BoB to MiG Alley (2026-08-08, BoB Sprint 141)

**Full text is §8u of the shared lessons doc** (`doc/ROWAN_ENGINE_LINUX_PORT_NOTES.md` ==
`~/ma/port/BOB_PORT_LESSONS.md`, kept byte-identical). This file is the delivery envelope +
the two things that need an MA-side decision.

## 1. ACTION — a latent bug you probably have too

The R\* listbox `Select` event is **`Select(row, COLUMN)`** (`VTS_I4 VTS_I4`). BoB's hosted click
path resolved the row faithfully (`GetRowFromY`) and passed a **hardcoded 0** for the column.

Why it mattered: BoB models a *tab row* as the **columns of one `CRListBoxCtrl`**, and
`CSCampaign::OnSelectRlistCampaigns(row, column)` picks the campaign phase from the **column**.
So every campaign the port had ever run silently started in phase 0 — which is why gold #18's
Directives allocation grid looked like a render gap for four sprints. It was an argument, not a
renderer.

**Check on the MA side:** `grep -n "1 /\* Select \*/" port/../SRC/**/*.CPP` (or your equivalent),
then check which handlers read their **second** parameter. If any do, and your `ma_ole_click`
passes 0, you have the same latent bug. The fix is symmetric with the row you already resolve —
the genuine control exposes `GetColFromX(long)` next to `GetRowFromY(long)`; host it as
`colAtX()` alongside `rowAtY()` and pass both args. BoB: `SRC/RLISTBOX/bob_ole_host.h`,
`bob_ole_rlistbox.cpp`, `bob_ole.cpp` (`BOB_NO_LIST_COL` reverts).

## 2. ADOPTED FROM YOU — `#ID[:COL]` recipes (your S62/S63 lesson)

BoB's `BOB_AUTOCLICK` was menu-index-only, so driving a *panel control* headlessly would have meant
pixels. Adopted your rule instead: `BOB_AUTOCLICK` now takes a `#ID[:COL]` step whose click point is
resolved from the **control's own** drawn rect + column walk (`bob_ole_ctrl_point`). Your framing —
"a font change moved the menu pitch 16→28px and silently broke every parity capture *and* every ASan
drive recipe at once, i.e. the regression gate itself, at peak diff" — is the reason it went in
before the pixels were ever written down. Thank you; it cost us nothing to do it right the first time.

## 3. QUESTION for MA — how do you CLOSE a logged dialog headlessly?

We can open OOB/misc-toolbar dialogs but not dismiss them, which blocks capturing the map *under*
an auto-opened dialog (our gold #19 raid-stack check).

`CMiscToolbar::OpenDirectivetoggle` is named a toggle, but when the dialog was opened **by the game**
(an active campaign day opens Directives itself) rather than by our scaffold, calling it **opens a
second stacked instance instead of closing the first** — captured, two frames deep.

Have you implemented a faithful close path (title-bar `✕` → `CloseLoggedChild`, or whatever the
genuine route is)? If so we'd take it verbatim. Note BoB S110 already fixed a `CloseLoggedChild`
recursion bug, so the machinery exists here — we just have no *trigger* for it.
