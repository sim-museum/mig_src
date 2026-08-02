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

## Current state (2026-08-01, after Sprint 66)

- ⭐ **CROSS-CUTTING DEVIATION #1 IS SOLVED** — the front-end font/typeface, the biggest
  single visual gap in the parity epic since S56. Colour landed in S63; the **FACE** lands
  in S66.
- **The game ships its own typeface and the port was never loading it.**
  `drive_c/windows/Fonts/Intel.ttf` ("Copyright (c) Rowan Software, 1998") is what gold
  renders with, and `MIG.CPP` asks for it by name. Two independent reasons it never
  arrived: `ma_gdi_font_create` **ignores the requested face**, and the single global TTF
  load was **rejecting Intel.ttf** because `stbtt_InitFont` only accepts platform-3 cmap
  encodings 1/10 while Intel.ttf ships a **(3,0) SYMBOL** cmap → init failed and every run
  silently fell back to a system serif. Fixed by accepting symbol cmaps and routing glyph
  lookups through `ma_cp()` (symbol tables address characters at `0xF000+c`).
  Now: `[gdifont] loaded …/Intel.ttf (symbol cmap)`.
- **Verified against gold**: title menu in yellow small caps (identical face to the gold
  shot); Preferences matches gold's yellow small-caps tab bar, blue labels, yellow values.
  Residual on those rows is now only the **BDG tab** (resource delta) and **combo chrome**.
- **Largest remaining visual deviation is now cross-cutting #2 — combo chrome** (native
  black-filled vs gold's translucent).
- **Known-imperfect, flagged:** `ma_gdi_font_create` still ignores the face name, so *all*
  text now draws in Intel.ttf including text the game asked to be Arial. Gold appears to
  use the art face throughout, so it is not visibly wrong — but that is luck, not
  correctness. Per-face selection is on the S67 list.
- ⚠ **Parity references were REBASED in S66 (all 7), not verified byte-identical** — a
  typeface change moves every screen by design, same as S63. Byte-identical checking
  resumes in S67 against these baselines.
- ⚠️ **Open ASan finding (S66): 2 intermittent `stack-use-after-return`** in the packed-item
  proxy accessors — `worldinc.h:257` (`T_size::operator ITEM_SIZE()`) and `worldinc.h:565`
  (`T_shape::operator ShapeNum()`), the same MSVC-ism family as S41's. Seen once in ~20
  runs; 4 single-mode runs and a second full suite were clean. **Not attributed** — S66's
  diff is font-only but the pre-S66 binary was not tested. **First task in S67.** A single
  clean run proves nothing here; it needs several.
- Gates: stress **PASS 20/20**. `prefs_controls` stays excluded from the sweep (it embeds
  live joystick state).
- Next (S67): (0) **attribute the intermittent ASan finding above**; (1) Player Log title bar **width** (`UpdateTitle` sizes from
  `viewsize.right`) + the `?`/`✓` buttons — displaced in S65 and S66; (2) **per-face font
  selection**; (3) cross-cutting **#2 combo chrome**; (4) Career content table (other half
  of I4); (5) RScrlBar hosting; (6) `ma_tabs_hit` click routing; (7) #12 debrief capture.
