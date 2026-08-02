# Sprint 63 — "Switch the properties on" (autonomous)

**Goal:** finish S62. The persisted-property reader is built, parses all 58 boot-path bags
clean and has a gold-verified payoff — it just ships OFF because of two blockers. Clear
both, switch it on by default, and re-verdict the parity set.

**Committed (~8 pts):**
| Story | Pts | Definition |
|---|---|---|
| S63-1 Root-cause the uninit garbage | 3 | The varying garbage text at the title screen's top-left under `MA_DLGINIT_PROPS=1` is root-caused and fixed; title capture clean at 6× contrast |
| S63-2 Font-independent click recipes | 3 | The parity + `asan_all.sh` drive recipes stop depending on fixed pixel rows: a click-by-menu-row-index helper resolves the row at runtime, so a font/pitch change cannot invalidate the gate again |
| S63-3 Reader ON by default + re-verdict | 1 | Reader default-on (`MA_NO_DLGINIT_PROPS=1` escape retained); full parity set re-captured; every moved verdict re-stated in `screen-parity.md` |
| S63-4 Cross-port note 20 + close | 1 | MA note 20 to `bob/doc/` — **owed from S62**, whose close story did not land — carrying the adoption report, MA's two divergences, and S63's findings; lessons doc md5-identical; board/burndown/parity/`RUNNING.md`/rollup; gates |

**Not pulled:** Player Log title bar + `?`/`✓` (the original S62-2 target — it becomes
reachable once the reader is on, but it is a verification job of its own and S62 already
showed it is not a one-liner); Career content table; RScrlBar hosting; `ma_tabs_hit`
click routing; #12 debrief capture.

**Planning notes (2026-08-01):**
- **Environment:** session UNLOCKED, no stray `wmig`, build current.
- **Tree state at planning needed sorting out first.** The working tree held an
  **uncommitted full revert of Sprint 62** (−475 lines, plus deletion of
  `port/scrum/sprint-62.md` and the S62 board entry) that no session in this
  conversation's history made. Verified it was byte-identical to the pre-S62 commit
  `f40a9ee` — i.e. it contained *zero* unique information and was fully reproducible from
  history — so restoring HEAD destroyed nothing. Preserved it in a stash regardless
  (`S62 revert found in working tree at S63 start`) rather than discarding it outright,
  then confirmed HEAD builds and its **default path is still byte-identical** to the
  `title` reference before building on it.
- **Blocker 1 (uninit garbage)** already has its search narrowed by S62: run-to-run
  variance says uninit read; the stock caption and `PX_String` are exonerated. The S61
  precedent — a local passed as an out-param to a **stubbed Win32 API** and then read
  without checking the return — is the first pattern to test, per
  `[[rowan-port-uninit-and-stub-traps]]`. Repro is ~2s: `MA_DLGINIT_PROPS=1` + the plain
  title screen.
- **Blocker 2 is the more valuable fix and should be done as the durable one.** The
  recipes break because they encode *pixel rows* for menu items. Resolving the row at
  runtime removes a whole class of future breakage — every subsequent font, DPI or
  layout change would otherwise re-break the gate. Doing it half-way (re-deriving the
  constants for the new pitch) buys one sprint and re-breaks on the next change.
- **Order matters:** S63-2 before S63-3, because the gate must be trustworthy *before*
  the default flips. That was exactly S62's reasoning for shipping opt-in.
