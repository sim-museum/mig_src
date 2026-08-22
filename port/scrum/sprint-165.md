# Sprint 165 — "The click walk never descended the level the paint walk does" (PO-50) — ✅ CLOSED 2026-08-22 (goal MET, 8/8) — ⭐ and it corrects S164's diagnosis

**Planned 2026-08-22** (PO ceremonies pre-approved), on S164's own stated next step.
**Sprint Goal:** clicks land on the campaign dialog they were aimed at, so K5 has a path.

| Story | Pts | Result |
|---|---|---|
| S165-1 fix the OOB paint trace: filter, don't cap | 2 | ✅ every distinct node prints once, ever |
| S165-2 find why the wave folder takes no clicks | 3 | ⭐ the paint walk descends a second level of logged children; the click walk did not |
| S165-3 fix it | 3 | ✅ PO-50 closed — the wave list and the `Task` button both take their clicks |

## ⚠ First: S164's diagnosis was wrong, and this is the correction

S164 reported *"the OOB walk paints 3 dialogs while 5 are on screen"* and concluded the wave folder
was outside the walk's collection. **That was a misreading.** `[oob] painted N` is a per-frame
counter of top-level children painted in that pass, not a count of the dialogs on screen; taking it
as the latter produced a confident, specific and incorrect claim which went into the sprint record,
the board, `STATUS.md` and a cross-port note.

The trace this sprint added shows what is actually true: **the paint walk visits all five top-level
dialogs, and so does its recursion. The CLICK walk visits four.**

**The lesson is not "read the counter more carefully".** It is that a *summary number* was used to
infer a *set difference*. The fix was to print the sets:

```
[oobrender] node=… m_ma=(200,24 432x184) -> screen(200,24)     <- paint DOES visit the wave folder
[oobvisit]  … four top-level nodes, none of them (200,24)      <- click does NOT
```

Two lines of trace, and the answer was unambiguous. **When the question is "does A see the same
things as B", print both sets and diff them — never compare their counts.**

## The finding

`ma_map_paint_oob` walks each toolbar's logged children **and then each of those children's own
logged children** (its `gslot` loop, there since S123, because a dialog can be logged on another
dialog). `ma_map_click_oob` had only the first level.

The campaign **wave folder is a logged child of the Mission Folder**, not of `m_toolbar2`. So it was
painted, and no click could ever reach it — every click on it fell through to what was underneath,
which on the campaign map is the main toolbar:

```
[clickid] id=2018 -> (414,63)                        the wave list, correctly resolved
[tbclick] id=2058 rect=(391,52,48,48) -> fire        IDC_OVERVIEW, on the toolbar beneath it
```

**Clicking a row of the mission you are editing opened the Overview dialog.** After the fix:

```
[tbclick] listbox id=2018 local=(207,7) -> row=0 col=3 on 8CProfile
[oobclick] node=… depth=1 took (414,63)
[tbclick] id=2143 rect=(291,173,78,22) -> fire        the Task button
```

Grandchildren take the click **before** their parent, because they are painted after it and
therefore sit on top — S82's *topmost gets first refusal*. `MA_NO_OOB_GRANDCHILD=1` reverts.

**Coverage now matches: 4 top-level nodes visited by the paint walk, 4 by the click walk** — and the
Overview dialog no longer opens by accident, which is why the count is 4 rather than 5.

## The trace fix, which is why the rest was possible

`[oobpaint]` was budgeted `if (_r++<40)`, so the whole budget went to the first dialog tree and a
dialog that opened later never appeared at all. **Third booking of "filter, don't cap"** in this
project (§8-MA83, S64→S67, now here) — and it cost S164 its diagnosis. Both walks now print **every
distinct node exactly once, ever**: bounded by the number of nodes, not by frames, so nothing can
starve a late arrival out of the log.

## K5 is not delivered — the next question is precise

`Task` fires, but `CProfile::OnClickedTask` uses `currrow-1`, and the click landed on **row 0**,
the header. `#2018@CProfile:r1` reports *"row 1 not mapped by GetRowFromY (h=110)"* — the control
believes it has fewer rows than the two the screen shows. **S166: what does this listbox think its
row layout is, and is the header a row or chrome?** That is one trace on `GetRowFromY`, not a guess.

## Gates

`parity_2d` 5/5 byte-identical · `oob_sweep` OPEN=9 NONE=0 CRASH=0 · `map_icon_click` PASS ·
`map_filter` PASS · `dialog_scroll` PASS · `help_click` PASS · `sysbox_exit` PASS (99.1 %) ·
`authorize_mission` PASS · `damage_elements` PASS.

Nine for nine. This change makes a whole class of clicks stop falling through to the toolbars, which
is exactly the kind of change that quietly breaks a dialog two screens away — `oob_sweep` opening
all nine dialogs and `sysbox_exit` still leaving the campaign are the two that would have caught it.
