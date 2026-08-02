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

## Current state (2026-08-01, after Sprint 65)

- **The Player Log is now essentially complete as a dialog.** S61 gave it the tab bar,
  centring and tab switching; S63 gave the front end its authored colours; **S65 landed the
  "PLAYER LOG" title bar** — star roundel on the striped `FIL_TITLEB_BMP` chrome, caption
  from `IDS_PLAYERLOG`. Parity **#15 = CLOSE-minus**.
- **The title bar had resisted S60, S62 and S64, and nothing was ever missing.** IDD 276's
  bag always carried `IDS_PLAYERLOG` + the literal `Player Log` + `FIL_TITLEB_BMP` ×2.
  **Two individually-correct narrowing filters were each withholding half** — S58's
  tickbox-only caption rule and S64's art-name gate. Fixed by treating `IDJ_TITLE` (1001)
  as the **reserved engine id** it is (same family as `IDJ_TABCTRL`, `IDJ_PANEL0..9`),
  whose caption and art are design-time by definition.
- **⚠ S64's recorded root cause was wrong and the cause was our own tooling.** It said
  `ma_px_replay` never fires for id 1001; it always did — the `[px]` trace had a hard-coded
  60-line cap and the boot path replays 58+ bags, so that control fell off the end.
  **Absence of trace output was read as absence of behaviour.** The cap is now
  `MA_TRACE_PX_MAX`. Treat any capped trace that gates a conclusion with suspicion.
- **Rejected, so don't re-try it:** template membership is NOT a workable narrowing
  criterion for design-bag caption/art — the system-box "Quit"/"Size" buttons are
  `inTmpl=1` too. `MA_BTN_ART_NAMES` therefore stays a blanket opt-in flag.
- **Not started, third sprint running: font FACE** — the remaining half of cross-cutting
  deviation #1 (the colour half landed in S63). It keeps being planned and displaced.
- Gates: parity **4/4 byte-identical** (`map_playerlog` re-based for the title bar;
  `prefs_controls` excluded — it embeds live joystick state), ASan and stress per the
  sprint log.
- Next (S66): (1) **font FACE — protect it from being displaced again**; (2) title bar
  width (`UpdateTitle` sizes from `viewsize.right`) + the `?`/`✓` buttons; (3) a general
  narrowing criterion (template membership rejected); (4) Career content table (other half
  of I4); (5) RScrlBar hosting; (6) route real clicks to `ma_tabs_hit`; (7) #12 debrief.
