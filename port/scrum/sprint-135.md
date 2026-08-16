# Sprint 135 — "The ruler was built, initialised, and never painted" (PO-22) — ✅ CLOSED 2026-08-15 (goal MET) — ⭐ found a 53-site bug class

**Planned 2026-08-15 (PO: continuous sprints, campaign dialogs, gold videos as reference).**
**Sprint Goal:** put the distance ruler on the campaign map.

| Story | Pts | Result |
|---|---|---|
| S135-1 does the game have a scale bar? | 2 | ✅ it does, fully initialised, never painted |
| S135-2 paint it where the dock manager would | 3 | ✅ `CScaleBar::MaPaintAt` |
| S135-3 the label said `0 ` + garbage | 3 | ⭐ a 53-site varargs bug class |

## The ruler

`CMainFrame` has always done:

```c
m_toolbar4.Create(CScaleBar::IDD,view);
m_toolbar4.Init(this,200,400,48,AFX_IDW_DOCKBAR_LEFT,4);   // -> m_align=4, m_width=48
view->m_pScaleBar=&m_toolbar4;
```

so the bar existed, knew it was on the left edge and knew how wide it should be. What the port
lacks is the **dock manager** that would size it and send it `WM_PAINT` — so `OnPaint` had never
run once. `MaPaintAt` supplies exactly that missing geometry (client rect, viewport origin, a
dialog-sized font) and calls the bar's own `OnPaint`. Every number comes from the game's arithmetic
— `grad = zoom * dist.longcm * 10 / 65536` — so the ruler cannot disagree with the map it annotates.

Three small compat gaps had to be closed first, each a "returns a plausible value" stub of the
familiar shape:

- `CDC::SetTextAlign` was `{ return 0; }`, so every `TA_CENTER`/`TA_RIGHT` draw came out
  left-aligned. It is now kept in the DC, where GDI keeps it. Every R-control asks for
  `TA_LEFT|TA_TOP` — the default — so nothing else moved: the parity diff is bounded to the
  ruler's own 48px strip, on one screen, and the other four are byte-identical.
- `CDC::GetCurrentFont` returned **NULL**, and its one caller immediately calls `GetLogFont` on it
  to centre the labels. It survived only because that method never touches `this`.
- `GetLogFont` filled an all-zero LOGFONT, so the centring offset was always 0.

## ⭐ The label read `0 ` + five garbage bytes

`sprintf(string,"0 %s",LoadResString(Save_Data.dist.longabbr))`. MFC's `CString` is a single
pointer, and passing one through `...` is the classic MFC idiom that works **by accident** on
MSVC: the ABI copies the 4-byte object and `%s` receives `m_pchData`. GCC passes the object by
invisible reference instead, so `%s` receives the **address of the CString** and prints the raw
bytes of the pointer — 4-5 bytes of garbage, every time.

An explicit `(LPCTSTR)` fixes it, and the trace confirmed the mechanism before anything was
changed wholesale: `n=7 "0 <garbage>"` became `n=4 "0 Nm"`.

**53 sites in 11 files:** `WPGROUND` `WPSHARE` `WPDETAIL` (waypoints), `WEAPONS` (payload),
`FLT_TASK`, `LSTMSNLG` (mission log), `PROFILE`, `SQUICK1`, `RCOMBOX` (dates), `FRAGPILT`,
`SCALEBAR`. That is precisely the list of campaign screens the PO reports as missing their text
(PO-28), so this is very likely more than a ruler fix. Parity is unchanged on all five screens,
which is the expected result: none of those dialogs is in the parity set.

*This is the §8 family again in a new place — a construct that is well-defined on the original
toolchain and silently wrong on this one, producing plausible-looking output rather than a crash.*

## Filter, don't cap — six

`MA_TRACE_TEXT` printed the first 24 draws. Hunting a ruler label, all 24 were consumed by the
title screen before the map even opened. It now takes a **substring** and traces every match,
uncapped (`MA_TRACE_TEXT=1` keeps the old behaviour). Sixth time this rule has been booked.

## A gate that asserted geometry

`help_click.sh` checked for the documentation panel by sampling a fixed span of row 120 and
requiring 80% of it to be one colour. S134 centred the panel and capped its width for legibility,
and the gate failed **on a correct change**. It now finds the longest single-colour run anywhere
on the row: a panel of any width or position passes, a map or title screen does not. Same lesson
as `hw_gate`'s first cut, which "failed" because hardware correctly reported a different driver.

## Evidence

`port/ref/native/map_ruler.png` — the campaign map with the ruler: black strip, white rule at the
inner edge, ticks every 10 units, labels every 50, headed "0 Nm". Structure matches `g12.png` from
the gold campaign video.

## Gates

parity 5/5 (campaign_map reference rebased — the diff is bounded to x<48, the ruler's own strip) ·
sweep 9 OPEN/0 CRASH · map icon click · map drag · sysbox exit · help click.
