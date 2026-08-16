# Sprint 146 — "Nothing ever asked them to close" (PO-33) — ✅ CLOSED 2026-08-16 (goal MET)

**Planned 2026-08-16, resuming the defect S139 located but could not fix.**
**Sprint Goal:** no stale panel painted over the landing page after quitting a campaign.

| Story | Pts | Result |
|---|---|---|
| S146-1 find what closes a front-end panel's dialogs | 5 | ⭐ nothing does |
| S146-2 cascade the teardown | 2 | ✅ `LaunchMap` and `LaunchScreen` |
| S146-3 confirm the page is clean | 1 | ✅ `CLoad` gone from the owner list |

## S139's two dead ends were both symptoms of this

S139 tried removing the controls in `CDialog::EndDialog` and in `CWnd::DestroyWindow`, and found
neither runs for the surviving `CLoad` panel. The reason is that **nothing ever asks that dialog
to close at all**:

- `CMIGView::LaunchMap` — the front-end → campaign-map transition — does
  `m_pfullpane->DestroyWindow()`. On Windows that destroys the panel window and **every child
  window under it**, and the dialogs the panel launched are those children. In this port the
  panel's own hosted controls are dropped (S139's change, finally earning its keep), but its
  child dialogs are registered against **themselves**, so they survive with `m_maVisible` set and
  the global draw pass keeps painting them.
- `RFullPanelDial::LaunchScreen` has the same shape written out literally:
  `pdial[0]=pdial[1]=pdial[2]=NULL;` — forget the panels, do not destroy them. Again correct on
  Windows, where they die with the parent window.

Both now destroy what they are letting go of, using the teardown `RFullPanelDial::PaintShopDesc`
already spells out (`PreDestroyPanel` + `DestroyPanel`).

*This is the port's oldest recurring shape, and worth naming precisely: on Windows, window
destruction is a **cascade**. Every place this port lets a parent go, it has to perform by hand
what the window manager would have done — and each such place looks perfectly correct in the
source, because the source was written for the cascade.*

## Evidence

`port/ref/native/title_after_quit.png` — the landing page after quitting a campaign: title
artwork and menu, and nothing else. The trace shows the teardown firing
(`[ghost] LaunchMap: destroying pdial[0]=0xb5e7420`) and `CLoad` absent from the owner list
afterwards, where before it appeared with 4 controls on every pass.

## Found while looking

At 1920×1080 the **title screen's artwork occupies only the top-left 800×600** and the rest of the
window is black. The campaign map fills the screen (S145) and so does the 3D view (S122); the
front-end panels are the last class that does not, and gold fills the screen at 1920. Logged as
PO-37.

## Gates

parity 5/5 byte-identical · sweep 9 OPEN/0 CRASH · map icon click · help click · panel click ·
sysbox exit (92.8%).
