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

## Current state (2026-07-27)

- Sprint 58 closed (`b5f544a`): S57 parity fixes capture-proven; the capture
  path is display-independent — dummy-run canvas byte-identical (`cmp`) to a
  GL-run canvas (standing acceptance bar). #1/#7/#8 → CLOSE. Uninit-PX root
  cause fixed in RLISTBOX ctor.
- Sprint 59 closed (`3d1d94c` + close commit): #9 stray combo root-caused
  (Windows parent-rect clipping of controls parked outside the 335-dlu dialog;
  `!WS_VISIBLE` style now routed) → CLOSE-minus; mission-text word-wrap
  (`DT_WORDBREAK`); uninit-PX audit widened to RSTATIC/RBUTTON/RCOMBO/REDTBT;
  DI mouse presence made unconditional. Notes 17 exchanged both directions.
- Gates: ASan 4/4 modes PASS. **Stress gate deferred — the desktop session is
  LOCKED**; a locked session never presents GL windows (swap blocks after 3
  frames). After unlocking: `flock /home/admin/.gl-display.lock -c 'bash
  port/stress_launch.sh'`. Headless work is unaffected.
- Next-sprint queue: #12 debrief capture, I4 Player Log (8 pts, full sprint),
  RRadio OCX hosting, BoB note-17 traps 1/2 (property-stream reader adoption).
