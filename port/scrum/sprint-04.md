# Sprint 4 Board — "Looks right + finish the front-end" (R1 polish → R2)

**Status:** 🏃 ACTIVE · **PO ratified:** 2026-06-17 (standing pre-approval, incl. starting this
sprint). **PO steer:** F4 first; Wine is available for B2 reference (later).
**Sprint Goal:** *The single-player front-end screens beyond Preferences (Quick Mission / Campaign /
Comms) render and navigate natively.*
**Capacity:** ~13–14 pts. **Committed:** F4 (13, may split) + C3-remainder (folds in). B2 deferred
within this sprint (F4 first, per PO).

---

## Sprint Backlog

| ID | Story (pts) | Tasks | Status |
|---|---|---|---|
| **F4** | Other front-end screens usable (13) | F4.1 nav mapped ✅ · F4.2 **Quick Mission panel renders+navigates ✅** · F4.3 Campaign panel 🔨 (now reachable) · F4.4 Comms panel ⬜ | 🔨 |
| **C3 rem.** | Scrollbar + new-panel hit-testing | extend `ma_ole_click`/mouse to the new panels' controls as they come up | ⬜ (with F4) |
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
