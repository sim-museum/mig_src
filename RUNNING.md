# MiG Alley — Run & Check Progress

## Run the game

```bash
cd /home/admin/sgl/TUE/MigAlley/WP/drive_c/rowan/mig && ./wmig
```

Bare launch (no env vars) from the install dir — boots to the title menu; Single
Player → Hot Shot is two clicks to a flyable 3D mission. Requires a healthy GL
display session.

Rebuild after source changes: `cd /home/admin/ma/build && ninja` (fallback:
`bash port/rebuild.sh`).

## Check progress

| What | Where |
|---|---|
| Backlog + burndown (per-sprint rows) | `scrum.md` — EPIC I is the screen-parity epic; §6 is the burndown |
| Per-gold-shot parity verdicts | `port/scrum/screen-parity.md` (one verdict line per Wine gold shot) |
| Native captures for side-by-side | `port/ref/native/` |
| Per-sprint boards | `port/scrum/sprint-NN.md` (latest: sprint-57) |
| Live product snapshot | `STATUS.md` |
| History | `git log --oneline` on `linux-port` |

Gold standard: `/run/media/admin/BEA6-BBCE/ma/` (14 PNGs) + the Player Log shot
`/home/admin/Pictures/Screenshots/Screenshot From 2026-07-19 20-33-27.png` (I4).
Oracle ruling: the gold shots as-is = the BDG 0.85F patched build
(resources read from `English/TEXT/miglang.dll` + patched `Mig.exe` since S57).

## Current state (2026-08-01)

- Sprint 59 closed, and its **deferred stress gate is now CLEARED: PASS 20/20**
  on the same commit that scored 0/20 while the desktop was locked — the
  environmental diagnosis is confirmed and S59 is green across all gates.
- **Sprint 60 CLOSED PARTIAL (5/8 pts) — the sprint goal was not met.** The
  Player Log's tab bar still does not appear. What landed is the machinery and
  two engine root causes:
  - Template-declared OCX controls that no dialog class `DDX_Control`-binds were
    never created. S57 fixed that for RStatic only; hosting is now kind-driven
    (RStatic / RButton / **RTabs**). `IDJ_TITLE` is an RButton — hosted since
    Phase 4, absent purely because nothing instantiated it from the template.
  - ★ **No RDialog in a dialog tree ever learned its own size** (the ctor zeroes
    `homesize`/`viewsize`; the refresh line is commented out at
    `RDIALOG.CPP:147`), so `RDialog::OnSize` handed the tab control a zero-width
    `MoveWindow`. Fixed with `MaSeedTemplateSize()`, scoped to the tree builders
    — a `CDialog::Create`-wide version regressed the front end (canvas 644→600).
  - RTabs is hosted (`ma_oletabs.cpp`); all three tabs register with the gold
    captions and the real 297×31 tab art loads from **RTabs.ocx's own PE**.
    The Career tab's **Name label + edit box now render** (unplanned bonus).
- Gates: **2D parity sweep byte-identical ×4** (title / prefs_3d /
  prefs_controls / quickmission vs their committed references), ASan **4/4
  modes, 0 reports**, stress **20/20**. `stress_launch.sh` now finds
  `build/wmig` on its own.
- Next-sprint queue (S61): **(1) composite the tab bar + title bar at the right
  offset** — suspect `RDialog::OnGetXYOffset`, whose parent-walk only
  accumulates when `parent->artnum == artnum` and every node in this tree is
  `artnum == 0`; (2) the `RDEmptyD` garbage viewsize from `Place()` centring
  against an uninitialised `m_pView` rect; (3) the Career content table (the
  half of I4 never pulled); (4) **RScrlBar is created 16× and unhosted**;
  (5) #12 debrief capture; (6) RRadio OCX hosting.
