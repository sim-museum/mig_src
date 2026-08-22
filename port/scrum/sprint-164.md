# Sprint 164 — "The click walk and the paint walk do not enumerate the same dialogs" (K5) — ⚠️ CLOSED PARTIAL 2026-08-22 — K5 not delivered; its blocker is named and measured

**Planned 2026-08-21** (PO ceremonies pre-approved). Script step 8: add a third F84 flight to the
Wonju wave. **Goal NOT met** — recorded as PARTIAL rather than claimed, per this project's own rule
about banking a sprint one step short of its acceptance criterion (S89).

| Story | Pts | Result |
|---|---|---|
| S164-1 open the TASKS dialog from the wave folder | 3 | ❌ blocked — the clicks do not reach the folder |
| S164-2 add a flight, assert on the `Flights` column | 3 | ⬜ not started |
| S164-3 *(unplanned)* a node's painted area must take its own clicks | 2 | ✅ landed, with an origin bug found on the way |

## What happened

Driving the wave folder's list (`#2018@CProfile:r0`) and then its `Task` button produced this:

```
[clickid] id=2018 col=-100 -> (414,63)   [type=1 rect(207,56 414x110) rel=0]
[tbclick] id=2058 rect=(391,52,48,48) pressed=1 -> fire
```

The recipe resolved the wave list correctly — and the click was taken by **`IDC_OVERVIEW`, a 48×48
button on the main campaign toolbar underneath**, which opened the Overview dialog. The wave folder
is drawn at (200,24) 429×180, straight over the toolbar row at y=52…100, so **this is reachable by a
player: clicking the first row of the mission you are editing opens an unrelated dialog.**

Logged as **PO-50**.

## The measurement that named the cause

```
[oob] painted 3 open dialog(s)          <- the OOB walk's own count, at its peak
[artclip] … 5 distinct dialogs drawn    <- (811,426) (1575,0) (0,867) (200,24) (724,406)
```

**Five dialogs are on the screen; the OOB walk knows about three.** The wave folder is one of the two
it does not enumerate — so the click walk, which iterates the same collection, cannot offer it a
point, and every click on it falls through to whatever is beneath.

This is S82's rule (*mirror the paint walk for hit-testing so hit rects cannot drift from drawn
rects*) failing one level further up than S82 addressed: not the rects, **the collection**. It is
also BoB's §8-BoB183 (*a control that is not in your walk's collection does not exist*) with
"control" replaced by "dialog" — which is worth sending back, because MA answered that note "N/A,
already closed" in S161 on the strength of the *control* case.

## What did land

A node whose painted area contains the click now **swallows** it even when none of its controls
wanted the point. Before, the swallow rule existed only at the **top-level logged child**, using
that child's rect — and a descendant is painted at its own `MaXYOffset()`, which is not constrained
to its ancestor's rect. `MA_NO_OOB_NODE_SWALLOW=1` reverts.

**An origin bug inside that fix, caught by its own trace.** The first version used the paint offset
alone, which put the Overview dialog's rect at **(0,0) 457×382** — it began swallowing clicks in the
top-left corner of the map. The node's screen rect is the offset **plus** its own `m_maX/m_maY`: for
a top-level logged child the offset is (0,0) and the position lives in `m_ma*`, while for a
descendant `m_ma*` is relative and the offset carries the ancestry. It was noticed only because the
swallow trace printed *which* node swallowed, and the answer was obviously wrong. **A trace that
names the actor, not just the action, is what turns a silent misbehaviour into a one-line diagnosis**
— the same reason S85 made ambiguous ids print unconditionally.

This does **not** fix the wave folder: that dialog is not in the walk at all, so there is no node to
do the swallowing.

## Next (S165)

1. **Fix the OOB paint trace's cap before anything else.** `[oobpaint]` is budgeted `if (_r++<40)`,
   so the whole budget is spent on the first dialog tree and the folder never appears in it. That is
   **"filter, don't cap"** — the trap this project has already booked twice (§8-MA83, S64→S67) and
   has now paid for a third time. Filter on the node being looked for, not a counter.
2. Then answer: by what route is the wave folder painted, and what collection should own it?

## Gates (after the swallow change)

`parity_2d` 5/5 byte-identical · `oob_sweep` OPEN=9 NONE=0 CRASH=0 · `map_icon_click` PASS ·
`map_filter` PASS · `dialog_scroll` PASS · `help_click` PASS · `sysbox_exit` PASS (99.1 % of the map
changed) · `authorize_mission` PASS · `damage_elements` PASS.

Nine for nine — which is the point of running them: the swallow change alters what happens to a
click that previously fell through to the toolbars, and that is exactly the kind of change that
breaks a gate two screens away.
