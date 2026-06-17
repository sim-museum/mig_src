# Sprint 2 Board — "Front-end finished" (R1→R2 bridge)

**Status:** ✅ REVIEWED / CLOSED (2026-06-17) — **R1 functionally complete**. A2.4 + F2 + F3 done;
C3 partial (rendering-panel coverage delivered, remainder re-sliced with F4 → Sprint 3).
· **PO ratified:** 2026-06-17 (standing pre-approval — PO approves every
sprint in advance; Dev runs the cadence autonomously).
**Dates:** 2026-06-17 → +2 weeks
**Sprint Goal:** *The native front-end is complete and navigable: every settings combo behaves
like a real dropdown, the resolutions list is populated, and the mouse works across all panels.*
**Capacity:** ~20 pts (re-baselined from S1 actual velocity ~16–19) · **Committed:** 17 pts +
A2.4 carry + F4 as stretch.
**Release context:** R1 (stable launch & front-end) closes at the end of this sprint; the next PO
acceptance gate is **R2 — Flyable 3D** (Sprints 3–5).

---

## Restart / resume (Sprint 2 Day 0 — machinery restarted after reboot)
- ✅ **Rebuilt** from clean `/tmp`: `port/rebuild.sh` → `/tmp/wmig` (8.5 MB i386 ELF, 268 objs,
  0 non-runtime undefined symbols).
- ✅ **Display wedge cleared by reboot** — smoke launch opened the SDL2/GL window (NVIDIA GTX 1660
  SUPER), drove `Launch3d`, dumped a 94%-non-zero cockpit frame (`MA_DUMP_BACK=60`). A1 visually
  re-confirmed in one run.
- ⏳ **A2.4** (carried from S1): live preference-persistence round-trip — see below.
- ⏳ **A1 full re-validation**: re-run `port/stress_launch.sh 20` now that display works.

---

## Sprint Backlog

| ID | Story (pts) | Tasks | Status |
|---|---|---|---|
| **A2.4** | Persistence round-trip (carry, ~2) | clean-exit save rewrites `settings.mig` ✅ (PASS: mtime advanced, `[save] ma_save_preferences`) · reload-on-boot ✅ | ✅ **DONE** |
| **F2** | Combo real dropdown list (5) | F2.1 render dropdown options panel ✅ · F2.2 hit-test rows → select ✅ · F2.3 open/close state ✅ · F2.4 cycle fallback for ≤1-item ✅ | ✅ **DONE** |
| **F3** | RESOLUTIONS combo populated (5) | F3.1 mode list ✅ (real detection-enumerated modes; synthesize 640/800/1024 if none) · F3.2 feed into combo ✅ · F3.3 selection drives resolution ✅ (SDETAIL `PreDestroyPanel` writeback on valid `driverModes`) | ✅ **DONE** |
| **C3** | Full menu mouse coverage (5) | C3.1 audit ✅ (see finding below) · C3.2 rendering panels fully hit-tested ✅ (listbox/button/tab/combo+dropdown) · C3.3 the un-covered remainder (scrollbar drag + controls on not-yet-rendering panels) is **coupled to F4** | 🔨 partial → re-sliced |
| **F4** | Other front-end screens usable (13) | F4.1 Campaign · F4.2 QuickMission · F4.3 Comms | ⬜ → **Sprint 3** |

### C3 audit finding (backlog refinement)
On every front-end panel that currently renders (title + the 7 settings tabs), all
interactive control types are hosted and hit-tested: RListBox (row-nav via `ma_ole_mouse`),
RButton incl. the settings **tab bar** (`ma_ole_click`→eventsink), RCombo (now a real **dropdown**
via F2). The only un-hit-tested hosted type is **RScrlBar** (CLSID `0x505aee46`, `CT_OTHER`) — the
listbox scrollbar; the short settings lists fit without scrolling, so it isn't exercised yet. The
genuine remaining C3 scope ("hit-testing on *all* front-end panels") cannot be completed before
**F4** brings up the Campaign/QuickMission/Comms panels — you can't hit-test controls on a panel
that doesn't render. **Decision:** C3's rendering-panel coverage is delivered; the scrollbar +
not-yet-rendering-panel coverage is re-sliced into Sprint 3 with F4 (they share the work).

Status: ⬜ todo · 🔨 in progress · 🔍 in review · ✅ done

---

## Definition of Done (this sprint)
Per global DoD in `scrum.md` §2: compiles into the build set (`port/rebuild.sh`), `wmig` links
0-undef, demonstrated by **running the binary** (not just compiled), no regression to
title/Preferences/first-3D-frame, gated trace (`MA_TRACE_*`) added for any new subsystem,
`CLAUDE.md`/STATUS + memory note updated, committed on `linux-port`.

## Working agreements (carried from S1 Retro)
1. Run app launches/stress as **background tasks**, never inline > ~90s.
2. Kill only by exact name: `pkill -x wmig` (never `pkill -f` a pattern matching our own command).
3. Stress-test in **bounded batches** (≤5) with pauses; don't SIGKILL GL apps in tight loops.
4. Reach for **gdb on the hung pid** early when a launch hangs.
5. After any from-scratch `rm -rf port/build`, confirm the link TU count (~266–268).

---

## Daily Scrum log
*(async; 3 questions — yesterday / today / blockers)*

### Day 1 — 2026-06-17
- **Done (restart):** Sprint 1 closed & accepted; machinery restarted. Rebuilt `/tmp/wmig`;
  confirmed the S1 display wedge is gone (smoke launch → 94%-non-zero cockpit frame). Sprint 2
  planned & board created under standing PO pre-approval.
- **Done (sprint):** **A2.4 PASS** (clean-exit rewrites `settings.mig`; `[save]` trace). **A1
  re-validated 20/20** post-reboot. **F2 DONE** — combo dropdown: click opens a list panel below
  the box (combo's own font, current item highlighted), row-click selects + fires TextChanged,
  click-away closes; ≤1-item combos keep the cycle fallback. Validated on the Preferences "3d?"
  tab: opened the detail combo (Minimum/Low/Medium/High/Maximum), selected "Maximum", confirmed the
  box updates and the screen has no regression (screenshots `/tmp/dd_crop.png`, `/tmp/dd_after.png`).
- **Today/next:** F3 (RESOLUTIONS combo from SDL display modes), then C3 (full menu mouse coverage).
- **Blockers:** none (display recovered on reboot).

---

## Burndown
| Day | Remaining pts | Note |
|---|---|---|
| 1 | 17 (+~2 A2.4 carry) | Sprint start; restart-resume complete |
| 1 (eod) | ~10 | A2.4 ✅ + A1 re-val 20/20; **F2 (5) ✅**; F3/C3/F4 remain |
| 1 (late) | ~5 | **F3 (5) ✅** — resolutions combo lists 640/800/1024 (4:3 only), software state pinned; C3/F4 remain |
| 1 (close) | 0 committed | C3 audit done (rendering panels covered); C3-remainder + F4 re-sliced → Sprint 3. **R1 complete.** |

---

## Increment / Review notes (Sprint Review — PO standing-accept)
**Demoable increment (all run native, no Wine):**
- **Dependable launch** re-confirmed post-reboot: A1 **20/20** via `stress_launch.sh`; smoke launch →
  94%-non-zero cockpit frame.
- **Preferences persist**: A2.4 round-trip PASS (clean-exit rewrites `settings.mig`).
- **Combo dropdowns (F2)**: clicking a settings combo opens a real list panel in the combo's own
  font with the current item highlighted; row-click selects + fires the change; click-away closes.
  Demo: detail combo Minimum→Maximum (`/tmp/dd_crop.png`, `/tmp/dd_after.png`).
- **Resolutions populated (F3)**: the RESOLUTIONS combo lists 640×480 / 800×600 / 1024×768 (4:3
  only) and drops down all three (`/tmp/f3_dd.png`); software driver state pinned consistent.
- **Mouse coverage (C3, partial)**: every interactive control on the rendering panels is hit-tested.

**R1 ("Stable launch & front-end") is functionally complete**: deterministic 3D launch, persistence,
title + fully-interactive Preferences (labels, combos-as-dropdowns, populated resolutions, tab nav).

**Completed:** A2.4 (~2) + F2 (5) + F3 (5) = **12 pts** + A1 re-validation + restart/build infra.
**Carried/re-sliced:** C3 remainder + F4 → Sprint 3.

---

## Sprint Retrospective (Scrum Master + Dev)
**What went well**
- Reboot cleared the S1 display wedge cleanly; restart-resume (rebuild → A1 20/20 → A2.4) was fast.
- F2 reused the combo's own `OnDraw`/font + `CDC::FillSolidRect` → the dropdown matches the box for
  free; the open-state lives entirely in the router (one combo at a time), drawn on top after the
  control loop so geometry stays consistent for both draw and hit-test.
- F3's root cause was an *inconsistent driver state*, not "missing modes" — tracing the filter
  (`modesCount=4 … driverNo 0 vs driver_index 0`) beat the first guess ("populate a fresh table").
- The C3 audit surfaced the real C3⊂F4 coupling early, avoiding wasted speculative work.

**What hurt**
- `rebuild.sh` doesn't recompile MFC fragments when `/tmp/*_ok.txt` is absent (post-reboot) — it
  relied on persisted `objmfc/*.o`. Editing `SDETAIL.CPP` needed a manual fragment compile into the
  build tree before relinking. **Action:** when editing an MFC fragment, recompile its `.o`
  explicitly (or regenerate the ok-lists) — don't assume `rebuild.sh` picks it up.
- Case-twin/symlink trap revisited: `Sdetail.cpp`→`SDETAIL.CPP` is a symlink (edit the real target);
  `Win3d.cpp` (lowercase) is the one in `_HARD`, not `WIN3D.CPP`.

**Action items (Sprint 3 working agreements)**
1. Carry forward S1's: background launches, `pkill -x wmig` only, no SIGKILL-spamming GL apps.
2. MFC-fragment edits: `g++ … -include stdafx.h -include _mfc.h -c <frag> -o port/build/objmfc/<X>.o`
   before `rebuild.sh`, or regenerate the probe ok-lists.
3. Keep the `MA_TRACE_OLE`-gated `[F3]`/dropdown traces — cheap, paid off twice this sprint.

---

## ➡ Sprint 3 plan (next) — "Hands on the stick" + front-end finish
Pull (PO standing-approved): **C1** (DirectInput→SDL flight controls, 13) as the R2 headline, plus
the re-sliced front-end remainder: **F4** (Campaign/QuickMission/Comms panels, 13 — may split) and
the **C3 remainder** (scrollbar drag + new-panel hit-testing, folds into F4). Likely split across
two sprints; C1 leads (it's the R2 gate). Re-baselined velocity ≈ 14–15 pts.
