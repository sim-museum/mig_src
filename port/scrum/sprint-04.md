# Sprint 4 Board — "Looks right + finish the front-end" (R1 polish → R2)

**Status:** ✅ F4 DONE (single-player front-end complete: Quick Mission + Campaign render & navigate;
Comms = out-of-scope multiplayer). Cross-port refcount insurance applied. B2 → Sprint 5.
· **PO ratified:** 2026-06-17 (standing pre-approval, incl. starting this
sprint). **PO steer:** F4 first; Wine is available for B2 reference (later).
**Sprint Goal:** *The single-player front-end screens beyond Preferences (Quick Mission / Campaign /
Comms) render and navigate natively.*
**Capacity:** ~13–14 pts. **Committed:** F4 (13, may split) + C3-remainder (folds in). B2 deferred
within this sprint (F4 first, per PO).

---

## Sprint Backlog

| ID | Story (pts) | Tasks | Status |
|---|---|---|---|
| **F4** | Other front-end screens usable (13) | F4.1 nav mapped ✅ · F4.2 **Quick Mission ✅** · F4.3 **Campaign ✅** · F4.4 Comms = multiplayer lobby, **out of scope** (DirectPlay, §8) | ✅ **DONE** (single-player) |
| **C3 rem.** | Scrollbar + new-panel hit-testing | new panels' controls (buttons/combos/listbox) hit-tested by the existing global `ma_ole_click`/`ma_ole_mouse` scan ✅; standalone scrollbar drag still unwired (low pri) | ✅ (new panels covered) |
| **B2** | 3D fidelity A/B vs Wine (start) | capture Wine reference frames; structural/pixel compare | ⬜ (after F4) |

Status: ⬜ todo · 🔨 in progress · 🔍 in review · ✅ done

### F4 starting position
The panel system (`FullScreen` defs + `RFullPanelDial::LaunchScreen`, FULLPANE.CPP) and OCX hosting
(RListBox/RStatic/RButton/RCombo+dropdown) already drive `options3d` (Preferences) end-to-end. The
target screens — `quickmission` (1323), `campaignselect` (1025), comms — are defined the same way, so
F4 = reach them, then grind the per-panel uninitialised-UI / missing-art / null-deref gaps (same
class as Phase 4). Quick Mission is the highest value: it leads into 3D flight, which now flies (C1).

---

## Definition of Done (this sprint)
Per global DoD: compiles, links 0-undef, demonstrated by running (a target panel renders + navigates
natively), no regression (A1, title, Preferences, first-3D, C1 controls), gated trace, docs+memory
updated, committed.

## Working agreements (carried)
Background launches; `pkill -x wmig` only; MFC-fragment edits → manual `.o` recompile into
`port/build/objmfc/` before relink.

---

## Daily Scrum log
### Day 1 — 2026-06-17
- **Done:** Sprint 3 closed; Sprint 4 planned (PO pre-approved, F4-first). Located the panel system.
- **Today:** map the title→QuickMission nav and launch the Quick Mission panel; diagnose gaps.
- **Blockers:** none.

---

### F4 root cause + fix (the unlock)
The front-end booted to `demotitle` (cut-down 5-item demo menu: Preferences/Hot Shot/Replay/
Credits/Quit), so the single-player path (Quick Mission / Campaign) was unreachable. Cause: the
MA_LINUX boot path (`MIG.CPP:506`) hard-launched `&demotitle` (an earlier-phase simplification),
while the engine's real path (`MAINFRM.CPP:221`) uses `&title`. The data install is the FULL game
(campaign smackers `c1_int.smk`, `FRONTMAP/`), so `CheckForDemo`'s demo gate doesn't apply.
**Fix:** `MIG.CPP:506` (MA_LINUX) now launches `&title`. Result: the full 7-item title renders;
**Single Player → Quick Mission navigates** and the Quick Mission setup panel renders natively —
Mission/Flight/Target-Zone/I.D. labels + combos ("Landing/Takeoff practice", "Kimpo Airfield"), the
mission-description text, and Back/Variants/Fly buttons (`/tmp/qm_screen.png`, 94% non-black). No
crash through `SetQuickState`. Campaign (singleplayer row 2) + Comms (title row 2) now reachable via
the same path. Gated diag added: `MA_TRACE_DEMO`, `MA_FORCE_TITLE`, `MA_TRACE_EXIST` (Fileman.cpp).
*Twin gotcha re-hit: edited `FILEMAN.CPP` but `_FILE` includes `Fileman.cpp` — reverted, fixed the
right twin.*

## Burndown
| Day | Remaining pts | Note |
|---|---|---|
| 1 | 13 | Sprint start |
| 1 (mid) | ~6 | **F4.2 Quick Mission renders+navigates ✅** (title-vs-demotitle boot fix, MIG.CPP). Campaign/Comms reachable next. |
| 1 (mid) | ~6 | Regression caught + fixed: title menu moved Hot Shot under Single Player → A1 stress nav broke (0/4 HANG). Updated `stress_launch.sh` `BOB_CLICKSEQ` to title→SinglePlayer→HotShot → **A1 8/8** again. |
| 1 (mid) | ~3 | Cross-port: read BoB's note, applied the refcount-UAF insurance (`bob_video.cpp` D3D7 surf/DD real `int ref`), replied in the shared doc. A1 6/6. |
| 1 (eod) | ~0 | **F4.3 Campaign ✅** (5 Korean-war phases + dates, Back/Film/Background/Objectives/Begin) and **F4.4 Comms = out-of-scope** (multiplayer/DirectPlay, §8; nav degrades gracefully via the engine's NOT_CONNECTED). **F4 single-player front-end DONE.** |

### F4.3/F4.4 results
- **Campaign** (`title→Single Player→Campaign`, `SetCampState`): renders natively — the campaign-phase
  list with date ranges (North Korea Invades … The Spring Offensive) + Back/Film/Background/Objectives/
  Begin buttons (`/tmp/camp_screen.png`, 97% non-black). No crash. (Minor cosmetic: a small black-square
  artifact top-right.)
- **Comms** (`title→Comms`, `StartComms`): the multiplayer **select-service lobby**. `StartComms`
  returns FALSE because `_DPlay.StartCommsSession()` (DirectPlay) is stubbed → the engine shows
  NOT_CONNECTED and stays on title (the `if(retval&&nextscreen)LaunchScreen` guard skips the launch).
  **Correct out-of-scope boundary** (scrum.md §8: Multiplayer is parked) — nav fires, degrades cleanly.
- **Quick Mission "Fly" → flight** (menu→fly→exit→menu round-trip) is the natural next step — use the
  sibling BoB recipe (F12→`CloseWindow`→`OnCancel`→`OnFlyingClosed`→menu, hand-deliver the swallowed
  WM_COMMAND). Tracked for Sprint 5.

---

## Increment / Review notes (Sprint Review — PO standing-accept)
**Demoable (native, no Wine):** the **full single-player front-end** now renders and navigates —
title (7 items) → **Quick Mission** setup (labels/combos/mission-text/Fly, `/tmp/qm_screen.png`) and
title → Single Player → **Campaign** select (Korean-war phases + dates + action buttons,
`/tmp/camp_screen.png`). Both reached by real menu clicks, no crashes through `SetQuickState`/
`SetCampState`. Comms (multiplayer lobby) correctly gated out by stubbed DirectPlay. Cross-port:
applied BoB's refcount-UAF insurance + replied in the shared notes. A1 8/8 / 6/6, no regression.
**Completed:** F4 (single-player) + C3 new-panel coverage + the refcount fix. **R1 front-end is
now complete end-to-end.**

## Sprint Retrospective (Scrum Master + Dev)
**What went well**
- The F4 blocker was a one-liner (`MIG.CPP` demotitle→title) hiding behind a deep-looking symptom;
  tracing the boot screen choice (LaunchFullPane caller) instead of guessing beat the rabbit hole.
- The panel system + OCX hosting from Phase 4 generalised for free — Quick Mission and Campaign
  rendered with zero per-panel code once the menu reached them.
- Cross-port collaboration paid off both ways (refcount insurance in; boot-gate + HUD-SIGFPE out).

**What hurt**
- The title change silently moved the flight entry one menu level deeper → A1 stress 0/4 HANG. Caught
  it because A1 is a gate on every change. **Lesson: any menu-layout change re-checks the click-driven
  harnesses.**
- Re-hit the FILEMAN.CPP-vs-Fileman.cpp twin trap (edited the uncompiled twin). Standing hazard.

**Action items**
1. After any front-end menu/layout change, re-run A1 (it's click-driven) before committing.
2. (carry) MFC-fragment edits → manual `.o` into `objmfc/`; verify the compiled twin (`_FILE` →
   `Fileman.cpp`, `_MFC` → `MIG.cpp`→`MIG.CPP`).

## ➡ Sprint 5 plan (next) — "Fly the mission + looks right" (R2)
- **Quick Mission "Fly" → 3D flight → exit → menu** (the menu↔flight one-process round-trip; adopt the
  BoB recipe: F12→`CloseWindow`→`OnCancel`→`OnFlyingClosed`→menu + hand-deliver the swallowed
  WM_COMMAND). This connects the now-working QM front-end to the now-working C1 flight = a playable
  Quick Mission.
- **B2** (3D fidelity A/B vs Wine — Wine reference available).
- Stretch: full-sweep SEGV hardening; standalone scrollbar drag; the Campaign top-right artifact.
