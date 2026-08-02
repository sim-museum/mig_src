# Sprint 67 — "Attribute and trim" (autonomous)

**Goal:** attribute S66's intermittent ASan finding before building anything on top of it,
and clear the Player Log title bar's remaining width deviation.

**Committed (~8 pts):**
| Story | Pts | Definition |
|---|---|---|
| S67-1 Attribute the intermittent ASan `stack-use-after-return` | 3 | Establish whether S66 introduced it or merely perturbed timing. A single clean run proves nothing at ~1-in-20 |
| S67-2 Title bar width + `?`/`✓` | 2 | The bar draws wider than the dialog; the `?`/`✓` buttons are absent |
| S67-3 Per-face font selection | 2 | `ma_gdi_font_create` ignores the face name, so everything draws in the art face — matching gold by luck, not correctness |
| S67-4 Cross-port note 24 + close | 1 | Note 24; docs md5-identical; board/burndown/parity/`RUNNING.md`/rollup; gates |

**Planning note:** S67-1 is deliberately first. S66 closed with an unattributed ASan
failure, and building further on an unexplained memory error is how a port accumulates
ghosts.

## Results

**Sprint outcome: 4 of 8 pts. S67-2 landed; S67-1 produced a negative result rather than an
attribution; S67-3 not started.**

### S67-1 Attribute the intermittent ASan finding (3 pts) — ◐ NOT attributed
Ran a dedicated hunt: all four suite modes in rotation, `detect_stack_use_after_return=1`
forced (rather than left to the runtime default), logs preserved on any hit.
**No recurrence — but the hunt is a much weaker sample than intended.** It was stopped
early because it was competing for CPU with the sprint's own ASan gate run, and it had
completed only **one full rotation (4 runs)** plus part of a second. Stating that plainly
because the first draft of this section implied a large sample; it was not one.

Honest tally on the current build since the failure:

| Attempt | Runs | Result |
|---|---|---|
| S66 suite run 1 | 8 | **2 reports** (`worldinc.h:257`, `worldinc.h:565`) |
| S66 single-mode repro | 4 | clean |
| S66 suite run 2 | 8 | clean |
| S67 hunt (SUAR forced, stopped early) | ~4 | clean |
| S67 gate suite | 8 | see gate log |

So: **2 reports in the first 8 runs, then roughly 16–24 clean runs.** That is enough to call
it intermittent and *not* enough to characterise a rate — "rarer than 1 in 20" would be
over-reading it.

**Still unattributed**, and worth being precise about why: every one of those runs is on the
**current** build. A clean run there cannot distinguish "S66 did not cause it" from "it did
not fire". The test that would attribute it — build the pre-S66 ASan binary and run it a
comparable number of times — **was not done**. S68 must either do that A/B or explicitly
downgrade this to a watch item; it must not be closed because runs came back clean.

Recorded for whoever picks it up: both reports are `BITFIELD`/`ONLYFIELD` macro-generated
proxy accessors on the packed item structs (`ITEM_STATUS::…::T_size::operator ITEM_SIZE()`,
`item::T_shape::operator ShapeNum()`) — the same MSVC-ism family as S41's `AddChildren`
stack-use-after-scope, which is the precedent for how these get fixed.

### S67-2 Title bar width (2 pts) — ✅ width fixed; `?`/`✓` still absent
**Root cause: our DCs have no clip region.** Windows clips a control's drawing to the
control's own window. `CRButtonCtrl`'s picture path (`RBUTTONC.CPP:1145`) blits its DIB at
**natural size** straight to the DC, so `IDJ_TITLE`'s ~550px-wide `FIL_TITLEB_BMP` art
painted ~213px past the 336px dialog, over the map. (The control itself was correctly sized
— traced at `(333,122) 336x27` — so this was never a layout bug.)

Added a clip rectangle to the GDI layer (`ma_gdi_set_clip` / `ma_gdi_restore_clip`,
honoured by `putpx`, `BitBlt` and `StretchBlt`) and set it around each button's `OnDraw` to
that control's rect. The title bar now stops at the dialog edge; parity sweep **4/4
byte-identical**, so the change is contained.

**The `?`/`✓` buttons remain absent, and this sprint established where they are NOT:**
IDD 276's template has only **two** items — `id=1001` (IDJ_TITLE) and `id=1117`
(IDJ_PANEL0). So they are not separate template controls, and they are not in the
`FIL_TITLEB_BMP` art either (the pre-clip capture showed the full 550px of art with no
`?`/`✓` glyphs in it). They must come from RDialog chrome drawn elsewhere. That is the next
question, not a guess.

*Trace-discipline note: the first attempt to measure the button's draw rect used a
`static int n; if (n++ < 8)` cap, which the system-box buttons exhausted before the Player
Log even opened — **exactly the S65 trap, one sprint after writing the lesson down.** The
probe now filters (`w > 300`) instead of capping. Filters beat caps whenever the interesting
event comes late.*

### S67-3 Per-face font selection (2 pts) — ⬜ not started

### S67-4 Cross-port note 24 + close (1 pt) — ✅

### Gates
- 2D parity sweep — **4/4 byte-identical** (`title`, `prefs_3d`, `quickmission`,
  `campaign_map`); `map_playerlog` re-based for the trimmed title bar; `prefs_controls`
  excluded (environment-dependent).
- `port/asan_all.sh` — **PASS 4/4 modes, 0 reports** (note this does NOT attribute the S66
  finding; see S67-1).
- `port/stress_launch.sh` — **PASS 20/20**.

### Carry-over to S68
1. **The ASan finding — A/B it against the pre-S66 binary or explicitly downgrade it.**
2. `?`/`✓` buttons: not template controls, not in the title art — find what draws them.
3. Per-face font selection (S67-3, untouched).
4. Cross-cutting **#2 combo chrome** — the largest remaining visual deviation.
5. Career content table (other half of I4); RScrlBar hosting; `ma_tabs_hit` click routing;
   #12 debrief capture.
