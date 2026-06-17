# Sprint 2 Board — "Front-end finished" (R1→R2 bridge)

**Status:** 🏃 ACTIVE · **PO ratified:** 2026-06-17 (standing pre-approval — PO approves every
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
| **F3** | RESOLUTIONS combo populated (5) | F3.1 enumerate SDL display modes · F3.2 feed mode list into the resolutions combo (`AddString`) · F3.3 selection drives the configured resolution | 🔨 |
| **C3** | Full menu mouse coverage (5) | C3.1 audit all front-end panels' clickable controls · C3.2 extend `ma_ole_click` hit-testing to any uncovered control types/panels · C3.3 hover/focus where the OnDraw path needs it | ⬜ |
| **F4** | Other front-end screens usable (13, STRETCH — may split) | F4.1 Campaign panel renders+navigates · F4.2 QuickMission panel · F4.3 Comms panel | ⬜ |

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

---

## Increment / Review notes (for Sprint Review)
*(fill in at review)*
