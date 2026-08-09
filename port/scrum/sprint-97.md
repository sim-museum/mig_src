# Sprint 97 — "A way out" (PO-1) — ✅ CLOSED 2026-08-09 — the exit widgets are visible, correct and they work

**Planned 2026-08-09 (PO pre-approved ceremonies; continuous-sprint directive). Autonomous. ~8 pts.**
**Sprint Goal:** finish PO-1 — *"no way to exit from campaign — no exit, resize etc widgets on
upper right"* — which S94 root-caused and half-fixed.

| Story | Pts | Result |
|---|---|---|
| S97-1 give the system box its art | 3 | ✅ identified against gold |
| S97-2 ship it on by default (font leak) | 3 | ✅ fixed, diff confined to the box rect |
| S97-3 the ghost cluster it exposed | 1 | ✅ parent-scoped draw made explicit |
| S97-4 gate it | 1 | ✅ `port/sysbox_exit.sh` |

## The state S94 left it in
`CSystemBox` was drawn and click-routed but **opt-in behind `MA_SYSBOX=1`**, for two reasons: every
button rendered **blank**, and drawing the box **corrupted the map's date text**. Both are fixed, so
it is now **on by default** (`MA_NO_SYSBOX` reverts) — which is the point, since a widget the player
cannot see is not an exit.

## 1. The art — and why matching names to ids would have shipped the wrong pictures
`F_GRAFIX.G` has `FIL_ICON_THUMBNAIL`, `FIL_ICON_ZOOMIN`, `FIL_ICON_CLOSE1` — named after the three
control ids. **Two of the three are wrong.** `FIL_ICON_THUMBNAIL`/`FIL_ICON_ZOOMIN` render as
unrelated map glyphs (a red square, a circular badge).

The Wine gold (#7) settles it: a 24-wide left column with **minimise** over **restore**, and a large
**X** on the right. The X is `IDC_FILES` → `CMainFrame::OnBye()` — *the exit*. Identified by probing
candidates against the gold crop, which the new `MA_BTN_ART="id=0xNNNN,…"` hook makes a two-minute
question instead of a rebuild each time. Final map: `IDC_THUMBNAIL`→`0x6a99`,
`IDC_ZOOMIN`→`0x6a9c` (`FIL_ICON_SCREENSIZE` — and `IDC_ZOOMIN` drives `OnGoBig`/`OnGoNormal`, which
*is* the screen-size widget, so the result is self-consistent), `IDC_FILES`→`0x6aa0`.

## 2. The font leak — a widget must not change the state of the screen it draws on
S94's parity failure was the map's **date readout**, top left, nowhere near the box: drawing the box
left a different GDI font selected in the screen DC, and the date text — drawn from the same DC
later in the frame — inherited it. Fixed by saving and restoring the DC's font around the draw.

**The check that proves it**, which S94 did not have: the only pixels that differ from the previous
reference are **x 724–795, y 4–51** — exactly the box's 72×48 rect at its position. Nothing else on
the screen moved.

## 3. ⚠ The bug the art *revealed*: a ghost that had been drawing all along
With art, a second copy of the cluster appeared at the **top-left**, and it **outlived the campaign
and sat on the title screen**. `ma_ole_draw_all` was drawing the box's controls at their raw
template origin *as well as* the parent-scoped map draw. **It had always been doing this — with no
art it painted nothing, so nobody saw it.**

The map toolbars avoid this only because their parent `CDialog` is created **hidden**; `CSystemBox`'s
parent is visible. Depending on a parent's hidden-ness is an accident, so the port now says it:
`ma_ole_set_parent_scoped(dialog)` marks a dialog as composited by the parent-scoped path only.

*Worth noting how this was found: it did not show up in any gate. The parity `title` capture is a
clean boot that never enters the campaign, so it stayed byte-identical while the title screen was
visibly wrong after an exit. It was found by looking at the screenshot of the thing just built.*

## 4. Proven end to end
Clicking the X returns the player to the **title screen with the main menu** (Preferences / Single
Player / Multi-Player / Load Game / Replay / Credits / Quit). `port/sysbox_exit.sh` asserts three
things — the handler ran, the screen really left the map (100% of pixels differ), and the process
survived — and clicks by **control id qualified by host class** (`#10@CSystemBox`), never a
coordinate, since the box is positioned from the screen's right edge (S95's rule; S96 moved that
edge twice).

`campaign_map`'s reference is re-baselined to include the widgets — intended, and the other four
screens stayed byte-identical.

## Gates — all under `gl-lock`
- **parity 5/5** · **sweep 9 OPEN / 0 CRASH** · **map click PASS** · **map drag PASS** ·
  **new sysbox exit PASS** · **stress 20/20** · **ASan 0 reports**

## Result
PO-1 closed. **Four of the five play-test defects are now closed** (PO-1, PO-2, PO-3, and PO-4 is
next with its cause already named); PO-5 remains the deep one. A player can now exit the campaign,
open a recon dossier, and drag the map without the screen falling apart — three things that were all
broken twelve minutes into the first human play-test.
