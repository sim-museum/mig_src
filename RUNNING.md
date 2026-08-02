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

## Current state (2026-08-02, after Sprint 68)

- **Parity #15 (Player Log) has no chrome deviations left.** Tab bar + centring + tab
  switching (S61), authored colours (S63), title bar (S65), title trimmed to the dialog
  (S67), and **`?`/`✓` buttons rendering (S68)**. Only the Career **content table** remains
  — the half of I4 never pulled.
- ⭐ **S68 found a whole missing subsystem: icons.** `CDC::DrawIcon` was a no-op stub and
  `LoadIconA` returned NULL, so **no icon anywhere in the port had ever rendered** —
  silently, for the port's entire life. A stub that returns *success* never gets reported.
  Now real: `RT_GROUP_ICON` → `RT_ICON` decoding (the group is a directory naming the image
  by id; `biHeight` is **doubled** — XOR bitmap then 1bpp AND mask, both bottom-up, mask
  bit 1 = transparent), cached, blitted alpha-keyed through the viewport/clip path.
  **Icons live in `Rbutton.ocx` (828–832), not Mig.exe** — third instance of "inside a
  control, `AfxGetInstanceHandle()` is that control's own module" (after `Intel.ttf` and
  the RTabs art). `MA_TRACE_ICON` traces resolution.
- ⚠️ **ASan finding: DOWNGRADED TO A WATCH ITEM — not attributed, not fixed, not closed.**
  S68 did the A/B properly (pre-S66 binary built via `git worktree add /tmp/ma-s65
  0a69f94`; alternating S65↔HEAD across all modes, `detect_stack_use_after_return=1`
  forced): **S65 0 hits / 12 runs, HEAD 0 hits / 12 runs.** That is a *negative* — no
  difference between arms, so nothing supports S66 having introduced it — but it does not
  show the bug is gone: the single sighting was 2 reports in one 8-run suite and it has not
  recurred in ~50 runs, entirely consistent with a ~1-in-50 defect.
  **If `worldinc.h:257` / `worldinc.h:565` reports again, treat it as the SECOND sighting of
  a known intermittent and preserve the log immediately** — the S66 logs were lost to the
  next run's `rm -f`, which is why there is still no stack trace for it.
- **Per-face font selection: THIRD consecutive carry.** `ma_gdi_font_create` still ignores
  the requested face, so everything draws in the art face (matches gold by luck). It is
  always the story that gets displaced — that is a prioritisation problem, not bad luck.
  S69 should schedule it first (the S66 tactic that worked) or drop it from the plan.
- Gates: parity **5/5 byte-identical** (`map_playerlog` re-based for the `?`/`✓`).
  `prefs_controls` matched this run only because the joystick is attached — it remains an
  unstable oracle.
- Next (S69): (1) **per-face fonts, scheduled FIRST**; (2) cross-cutting **#2 combo
  chrome** — the largest remaining visual deviation; (3) Career content table;
  (4) RScrlBar hosting; (5) `ma_tabs_hit` click routing; (6) #12 debrief capture.
