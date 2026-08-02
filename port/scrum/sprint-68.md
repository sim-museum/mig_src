# Sprint 68 — "Icons and evidence" (autonomous)

**Goal:** discharge the debt S67 left — actually attribute the ASan finding — and find what
draws the Player Log's `?`/`✓`.

**Committed (~8 pts):**
| Story | Pts | Definition |
|---|---|---|
| S68-1 ASan A/B: attribute or downgrade | 3 | Build the **pre-S66** ASan binary and run it against HEAD, alternating, enough times to say something. S67 failed this by only testing the current build |
| S68-2 What draws `?`/`✓` | 2 | S67 established they are neither template controls nor part of the title art |
| S68-3 Per-face font selection | 2 | `ma_gdi_font_create` ignores the face name |
| S68-4 Cross-port note 25 + close | 1 | Note 25; docs md5-identical; board/burndown/parity/`RUNNING.md`/rollup; gates |

**Sprint outcome: 6 of 8 pts. S68-1 delivered an answer (a downgrade) rather than the
attribution it hoped for, and S68-2 landed a whole missing subsystem. S68-3 carried again.**

## Results

### S68-1 ASan A/B (3 pts) — ✅ answered (as a downgrade, not an attribution)
Built the pre-S66 binary properly this time: `git worktree add /tmp/ma-s65 0a69f94`, ASan
build there, saved as `/tmp/wmig-asan-S65`; HEAD's saved as `/tmp/wmig-asan-HEAD`. Harness
alternates **S65 → HEAD** within each iteration across all four modes, with
`detect_stack_use_after_return=1` forced, so machine-state drift hits both arms equally.

**Result: S65 0 hits / 12 runs · HEAD 0 hits / 12 runs.** Zero reports in either arm.

**Conclusion — downgraded to a watch item, with the reasoning stated so it can be
challenged.** What the A/B establishes is a *negative*: across 12 matched runs per arm
there is **no detectable difference between pre-S66 and HEAD**, so nothing supports the
idea that S66 introduced it. What it cannot establish is that the bug is gone — the
original sighting was 2 reports in a single 8-run suite, and it has now not reproduced in
roughly **50 runs** since (S66 ×12, S67 ×12, S68 ×24). A defect that fires once in ~50 runs
is entirely consistent with all of this evidence.

So: **not attributed to S66, not fixed, not closed.** Downgraded to a standing watch item —
if `asan_all.sh` ever reports `worldinc.h:257` / `worldinc.h:565` again, that report should
be treated as the *second* sighting of a known intermittent, and the log preserved
immediately (the S66 logs were lost to the next run's `rm -f`, which is why there is still
no stack trace for it). Recorded in `RUNNING.md` and the rollup so it survives this
session.

### S68-2 What draws `?`/`✓` (2 pts) — ✅ found, implemented, rendering
Followed the engine rather than the pixels. `RDialog`'s eventsink is the clue:
```
ON_EVENT(RDialog, IDJ_TITLE, 2 /* Cancel */, OnCancel, VTS_NONE)
ON_EVENT(RDialog, IDJ_TITLE, 3 /* OK */,     OnOK,     VTS_NONE)
```
— the title control *itself* raises Cancel/OK, so it draws its own buttons. `CRButtonCtrl`
does exactly that (`RBUTTONC.CPP:521-536`), gated on the persisted `CloseButton`/
`TickButton` flags. Traced the Player Log's title bag: **`close=0 tick=1`** — the ✓ was
always meant to be there.

It never appeared because of a much broader gap: **`CDC::DrawIcon` was a no-op stub
("icons not yet rasterised") and `LoadIconA` returned NULL, so no icon anywhere in the port
rendered.**

Where the icons live took one more step. `AfxGetInstanceHandle()` inside a control is that
control's own module, and scanning every shipped PE showed Mig.exe carries only
RT_GROUP_ICON 128/129 — while **`Rbutton.ocx`** (note the lowercase 'b'; an earlier
`ls | head` had truncated it out of view and briefly convinced me RButton wasn't installed)
carries **828–832**, precisely IDI_BYEUP / IDI_TICKUP / IDI_TICKDOWN / IDI_HELPUP.

Implemented real icon support in `ma_gdi.cpp`: resolve `RT_GROUP_ICON` → pick its first
entry → decode that `RT_ICON`. An icon's `biHeight` is **double** the real height — the XOR
colour bitmap followed by the 1bpp AND mask, both bottom-up, mask bit 1 = transparent — so
it needs its own decoder rather than the DIB path. 1/4/8/24/32 bpp handled; results cached.
`LoadIconA` resolves `MAKEINTRESOURCE` ids across Mig.exe → Rbutton.ocx → RTickBox.ocx and
ignores `hInstance` deliberately (same reasoning as S60's RTabs art).

**Result: the `?` and `✓` render on the title bar, red, as gold shows them.** Parity sweep
**5/5 byte-identical**, so the change is contained to the one screen that actually calls
`DrawIcon`.

This closes the last *chrome* deviation on parity #15. What remains there is the Career
content table — the half of I4 never pulled.

### S68-3 Per-face font selection (2 pts) — ⬜ not started
Third sprint carried. Not attempted; S68-2 took the time.

### S68-4 Cross-port note 25 + close (1 pt) — ✅

### Gates
- 2D parity sweep — **5/5 byte-identical** (`title`, `prefs_3d`, `prefs_controls`,
  `quickmission`, `campaign_map`); `map_playerlog` re-based for the new `?`/`✓`.
  Note `prefs_controls` matched this time because the joystick is attached again — it
  remains an unstable oracle, not a reliable gate member.
- **S68-1 A/B — S65 0/12, HEAD 0/12.** No difference between arms; see S68-1.
- `port/asan_all.sh` — **PASS 4/4 modes, 0 reports**.
- `port/stress_launch.sh` — **PASS 20/20**.

### Carry-over to S69
1. Per-face font selection (S68-3, third carry).
2. Cross-cutting **#2 combo chrome** — the largest remaining visual deviation.
3. Career content table (the other half of I4).
4. RScrlBar hosting; `ma_tabs_hit` click routing; #12 debrief capture.
