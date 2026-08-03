# Sprint 78 — "Chase the loop blocker" (G2) — ⚠️ CLOSED PARTIAL 2026-08-03 (blocker precisely located; S77 corrected)

**Planned 2026-08-03 (PO pre-approved ceremonies). Autonomous. DoD: verify/fix the S77
flyable-loop `gamestate` blocker, or locate the real one precisely.**

## Context
S77 hypothesised the flyable multi-mission loop was blocked by `gamestate` (the campaign fly
supposedly hitting `OnFlyingClosed`'s HOT/QUICK branch → `quickmissiondebrief`, `indebrief=0`).
This sprint tested that directly.

## Execution log

### S78-1 — S77 corrected; the real blocker is a leaked fileblock — PARTIAL
Traced `gamestate` at `OnFlyingClosed` (`FULLPANE.CPP:2603`) on the `MA_CAMP_FLY`+`BOB_AUTOEXIT`
path (headless, dummy):

- **`gamestate=3=CAMP`** — the campaign fly *does* set the campaign gamestate. **S77's
  hypothesis was wrong.**
- **The campaign branch executes and the campaign ADVANCES.** Traced: `[debrief] CAMP branch:
  indebrief=TRUE set, calling NextMission (MapPlayback=0)` — `FULLPANE.CPP:2678`
  `MMC.indebrief=TRUE` + `:2698` `MMC.NextMission()` both run. So the campaign progression on a
  flown mission is wired.
- **The real blocker: a leaked fileblock.** Immediately after `NextMission`, the campaign-debrief
  map reload (`FULLPANE.CPP:2706-2709`, `for (f=FIL_MAP_BUTTON1..FIL_ICON_NEXT_PERIOD) delete new
  fileblock(f)`) hit **`[SysError] Opened file block (6a63) again without closing!`** (FILEMAN
  `:1542`) and the debrief setup **hung** (no debrief screen, no further pages; clean timeout,
  no crash). File `0x6a63` = **`FIL_ICON_BASES`** (`F_GRAFIX.G:181`) — the airfield/base map
  icons — which is *within* the reload loop's own range (`FIL_MAP_BUTTON1 0x6a04 ..
  FIL_ICON_NEXT_PERIOD 0x6aa8`), i.e. it was already open (leaked) before the loop.
- **Why `indebrief=0` in S77:** the S77 idle trace read `indebrief` *after* the debrief setup had
  already stalled/reset on the SysError path — not because the campaign branch wasn't taken.

**Concrete G2 next step (precise):** find where the port's **map-render icon path** opens
`FIL_ICON_BASES` (0x6a63) and leaves it open (the literal constant appears only in a compat
button mapping — the game opens it via a computed FIL, likely `DrawIcons` for airfield/base
icons), and close it (or make the debrief reload tolerant of an already-open block). Then the
flyable loop (fly → campaign debrief → Next Period → next mission) should complete; the
`DebriefToolBar().OnClickedNextPeriod()` drive (S77) is ready to re-add.

## Diagnostics kept
Two gated `MA_TRACE_3D` traces on the campaign-debrief path (`OnFlyingClosed` gamestate +
the CAMP-branch `indebrief` set), consistent with the codebase's `MA_TRACE_*` diagnostic
pattern — they document the exact flow and drive the next leak-hunt. No behavioural change
(no-op unless `MA_TRACE_3D` is set).

## Gates
The only change is two gated trace `fprintf`s (no behaviour when unset). A normal campaign map
capture is unaffected; the campaign paths still run under ASan (the leak SysError is pre-existing,
non-fatal at the FILEMAN level — the hang is in the debrief setup, exercised only on the
flyable-loop path which the ASan suite does not drive to completion).

## Result
S77's `gamestate` hypothesis is disproved; the campaign genuinely advances on a flown mission,
and the flyable-loop blocker is now a *specific, named* bug — a leaked `FIL_ICON_BASES` fileblock
crashing the campaign-debrief reload. The epic is a concrete bug-fix away from the flyable loop.
(measure-don't-assume, twice over: S77 assumed gamestate; S78 measured CAMP and found the fileblock.)
