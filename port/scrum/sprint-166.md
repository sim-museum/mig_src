# Sprint 166 — "Two row-count opinions inside one control" (K5 cont.) — ✅ CLOSED 2026-08-22 (goal MET, 8/8)

**Planned 2026-08-22** (PO ceremonies pre-approved), on S165's stated question: *what does the wave
listbox think its row layout is, and is the header a row or chrome?*
**Answered by measurement in one run.** Neither guess was right.

| Story | Pts | Result |
|---|---|---|
| S166-1 make the listbox say what it believes | 2 | ✅ `MA_TRACE_LBROW=1` |
| S166-2 answer the question | 3 | ⭐ the row height was fine and the header is fine — **the row COUNT came from the wrong list** |
| S166-3 fix it, and reach the real wave row | 3 | ✅ `:r1` selects `1.Bomb`; the `Task` button fires |

## The answer

```
[lbrow] y=15 scroll=0 tmHeight=16 -> row=0   (count=0, fontptr=(nil), hwnd=0x…, vsep=0)
[lbrow] y=16 scroll=0 tmHeight=16 -> row=-1  (count=0, …)
```

`tmHeight=16` — the row height was never wrong. **`count=0`** was the whole story:

```c
short row=(short)((y+m_lVertScrollPos)/tm.tmHeight);
if (row>m_playerList.GetCount()) row=-1;      // GetRowFromY
```

`m_playerList` is populated **only by `AddPlayerNum`** — the multiplayer / player-log screens. Rows
come from `AddString` into **`m_list`**. So on every listbox that is not a player list,
`GetRowFromY` sees zero rows and answers **-1 for every row past the first**.

And the control's own hit path does **not** have this problem:

```c
m_iRowSel=(point.y+m_lVertScrollPos)/tm.tmHeight;              // OnLButtonDown
count=m_list.GetAt(m_list.FindIndex(m_iColSel))->GetCount()-1;
if (m_iRowSel>count) m_iRowSel=count;
```

**Two opinions about "how many rows does this control have", inside one control** — the same shape as
everything else this week (paint vs click walk, draw vs click type filter), one scale smaller.

## Why it was safe to correct rather than work around

`GetRowFromY` is an OCX dispatch method with **no caller anywhere in the game tree** — checked, not
assumed. Its only consumer is the port's own `#ID:rN` recipe resolver from S162, which was therefore
**silently limited to player lists from the day it was written**: my own tool had the same class of
hole it exists to find. Under `MA_LINUX` the clamp now uses `m_list` (max row count across columns);
out of range still answers -1 so an unsatisfiable recipe fails loudly rather than clicking elsewhere.
`MA_LB_PLAYERCLAMP=1` restores the original guard.

Result:

```
[clickid] id=2018 col=-101 -> (414,79)
[tbclick] listbox id=2018 local=(207,23) -> row=1 col=3 on 8CProfile   <- the real 1.Bomb row
[tbclick] id=2143 rect=(291,173,78,22) -> fire                          <- Task
```

## K5 still open — and again the next question is precise, not a guess

`Task` fires and `CProfile::OnClickedTask` runs `SetTaskTabs(currrow-1, currcol-2)` — **but no TASKS
dialog appears** (the drawn set is unchanged at four). Two candidates, distinguishable by one trace:

1. `OnSelectRlistboxctrl1(row,col)` never ran, so `currrow`/`currcol` are stale — the Select event
   reaches the dialog by `ma_evt_fire`, and this dialog's sink registration should be confirmed
   rather than assumed;
2. `SetTaskTabs` runs and builds a dialog that nothing paints — the S53/S123 family.

**S167 prints `currrow`/`currcol` at the top of `OnClickedTask` and settles it.** Recorded this way
because this sprint and the last two were each solved by measuring the thing rather than reasoning
about it, and S164 was got wrong by reasoning about it.

## Gates

`parity_2d` 5/5 byte-identical · `oob_sweep` OPEN=9 NONE=0 CRASH=0 · `dialog_scroll` PASS ·
`map_filter` PASS · `authorize_mission` PASS · `damage_elements` PASS · `help_click` PASS ·
`sysbox_exit` PASS (99.1 %) · `map_icon_click` PASS.

`dialog_scroll` and `damage_elements` are the two that matter for this change: it alters what
**every** `CRListBoxCtrl` in the game answers about its own row count, and those two drive real lists.
