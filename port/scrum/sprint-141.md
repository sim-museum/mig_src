# Sprint 141 — "The oracle had the bug too" (PO-35) — ⚠️ CLOSED 2026-08-16, diagnostic only

**Planned 2026-08-16, following the front-end regression S140 had to work around.**
**Sprint Goal:** understand why the title screen's menu measures as overflowing, and fix it.

| Story | Pts | Result |
|---|---|---|
| S141-1 print the fit test's terms | 2 | ✅ `MA_TRACE_LBFIT` |
| S141-2 fix the overflow | 3 | ✅ there is no overflow — the state is stale |
| S141-3 ...which uncovered a bigger one | 3 | ⭐ the title screen does not match gold |

**No code behaviour changed this sprint.** What changed is what we know.

## There is no overflow

`MA_TRACE_LBFIT` prints both terms of `UpdateScrollBar`'s two fit tests. For the title menu:

```
[lbfit] count=7 tmHeight=14 content=98 client=105x100  -> fits        (vertical)
[lbfit] cols=1 contentW=87 clientW=105 m_vert=0        -> fits        (horizontal)
[lbfit] cols=1 contentW=87 clientW=0   m_vert=16       -> H-OVERFLOWS (populate time)
```

At draw time it fits, comfortably, in both axes. The `H-OVERFLOWS` is the **populate-time** call,
when the port has not yet sized the listbox and the client rect is 0×0 — the zero-rect the file
has documented for sprints. That call **creates the scrollbars and calls `ShowWindow(SW_SHOW)`**,
and the front-end path never re-runs the test, so the bars stay marked visible for the life of the
screen. S140 saw them the moment anything drew them. So the front-end exclusion S140 shipped is
right, but the reason is staler state, not disagreeing metrics — my S140 note guessed "text
metrics disagree" and that guess was wrong.

## ⭐ And the title screen does not match gold

Comparing our title against the gold video's own title frame:

| | menu |
|---|---|
| gold (`port/ref/gold/title_menu_gold.png`) | yellow text directly on the artwork, no box |
| ours (`port/ref/native/title_menu_ours.png`) | an **opaque black rectangle** behind part of it |

`CRListBoxCtrl::OnDraw` fills its box when `!artnum`, and the port's `RDialog::OnRowanMessage`
returns **0** for `WM_GETARTWORK` deliberately — returning the real artnum sends hosted controls
down an offscreen-compositing path that renders all black. On Windows artnum is non-zero, so the
original never fills. The port's answer to one problem is the cause of this one.

It is not a one-line removal: **S70 recorded that skipping the fill globally "erased the title
menu"**, which is why S71 scoped it to the OOB path. The fix needs the port to be able to say "my
parent has already painted art behind me" without taking the offscreen path. That is the next
attempt, and it is squarely in the same family as every other finding this week — a stub answer
that is plausible, load-bearing, and wrong.

## The reason this survived 140 sprints

**The 2D parity reference encodes it.** `title.png` was captured from this port, black box and
all, so the gate has been asserting the defect is present, byte for byte, ever since. The rule was
already written down here — *a capture that shares a bug with the code under test is not
evidence* — and it still took a side-by-side with the gold video to see it. Worth re-reading the
other four references against gold before trusting them.

## Gates

parity 5/5 byte-identical (unchanged — this sprint added only traces).
