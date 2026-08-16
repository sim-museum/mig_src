# Sprint 140 — "Hosted, placed, and still invisible" (PO-34) — ✅ CLOSED 2026-08-16 (goal MET)

**Planned 2026-08-16 (PO: continuous sprints on the campaign GUI; the control was named as next
in S136's census).**
**Sprint Goal:** the campaign dialogs' lists can be scrolled.

| Story | Pts | Result |
|---|---|---|
| S140-1 host RScrlBar | 3 | ✅ eighth control on this recipe |
| S140-2 why is a hosted bar still invisible? | 3 | ⭐ three more gaps, each individually fatal |
| S140-3 make the click move the list | 2 | ✅ fires the Scroll event the listbox sinks |

S136's CLSID census named this: **26 × 0x505aee46 — RScrlBar — unhosted**. Every scrollable
campaign dialog listed more rows than it could show, with no way to reach them.

## Hosting it was a quarter of the job

Each of these looked like "hosted, fine", and each on its own kept the bars off the screen:

1. **Placed at a negative rect.** `CRListBoxCtrl::UpdateScrollBar` computes the bars from
   `GetClientRect` — `Move(rcBounds.right-16, rcBounds.top, rcBounds.right, ...)`. The port sizes
   a listbox only at DRAW time, so at populate time the rect is 0×0 and every bar was moved to
   `(-16,0)-(0,0)`: a negative extent, which the draw path drops on `if (w<=0||h<=0) return`.
   This file already documented the zero rect — S-earlier worked around its *symptom* by
   re-clamping the scroll position. The placement is now re-run at draw time, where the size is
   real, and the bars land at e.g. `(1176,458) 16x232`.
2. **The placement went to the wrong object.** `Move` records the rect on the control; the draw
   walk reads the **client** CWnd. The router now mirrors it back, exactly as the listbox path
   already does for its own rect (`MA_SYNC_RECT`).
3. **The bars are children of the LISTBOX, not the dialog.** `UpdateScrollBar` does
   `Create(..., this, 1000)`. Both draw walks and the click walk select on `parent == dialog`, so
   none of them could ever have seen a scrollbar. They are now drawn with their listbox, at its
   origin plus their own rect, and clicked the same way. (First cut matched only the client key
   and found nothing at all — the parent recorded is the listbox *control*.)
4. **Nothing delivered the notification.** The bar's `DoFireScroll` calls `FireScroll`, and
   `COleControl::FireEventV` is a no-op here. The listbox sinks it by id —
   `ON_EVENT(CRListBoxCtrl, 1000, 1 Scroll, OnScrollVert)` / `1001 ... OnScrollHorz` — so the host
   fires it, as it does for every other hosted control.

The click itself calls the control's **own** `OnLButtonDown`, which is unusual for this port: the
host normally supplies the behaviour because the genuine handler needs an MFC message context.
Here it does not — it needs `GetClientRect` (which the host fills), and `SetCapture`/`SetTimer`/
`RedrawWindow`, which are safe no-ops. So the arrow/page/thumb arithmetic stays in the one place
that already had it, and the click walk cannot drift from the paint walk because they are the
same code.

Also implemented `COleControl::SetRectInContainer` / `GetRectInContainer` — `Move` is literally
`SetRectInContainer(rect)`, and the port *is* the container.

## The front end is deliberately excluded

Parity caught a real regression, twice, and it is worth recording exactly:

- drawing the bars on the front-end path put **a vertical and a horizontal scrollbar across the
  title screen's menu** (title, 2839px, bbox 530,210–635,310) — the gold has never had one there;
- gating only the draw still left **71px**, because re-running `UpdateScrollBar` sets
  `m_vert`/`m_horz`, which the listbox's own `OnDraw` then reserves space for, narrowing the rows.

The title menu is being measured as overflowing a box it plainly fits, so the port's text metrics
disagree with the original's by enough to trip `height > rcBounds.bottom-rcBounds.top`. **That is
a real defect and it is not this one.** The re-run is gated on `ma_oob_lb_draw` — the flag the
port already uses to tell the OOB path from the front-end path — so campaign dialogs get their
bars and the parity-locked front end is untouched. Parity is byte-identical on all 5 screens.

## Evidence

`port/ref/native/dialog_scrollbar.png` — the Intelligence dialog's supply list with a working
vertical bar. Clicking its down arrow moves the list one row: `pos 0 -> 16 (min 0 max 312 page
232)`, **20,844 pixels of rows change**.

New gate **`port/dialog_scroll.sh`** asserts on the ROWS, in a strip that deliberately EXCLUDES
the bar itself — the thumb moves whether or not the list does, and that is exactly the false pass
this defect would have produced at gap 3.

## Gates

parity 5/5 byte-identical · sweep 9 OPEN/0 CRASH · map icon click · sysbox exit · map filter ·
help click · **dialog scroll (new)**. Both build systems updated.
