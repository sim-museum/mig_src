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

## Current state (2026-08-01, after Sprint 63)

- **Sprint 63 CLOSED 8/8 — goal MET. The persisted design-time property reader is ON by
  default** (`MA_NO_DLGINIT_PROPS=1` reverts). The front end now draws with its authored
  colours: setting **values yellow, matching gold exactly**, Preferences tab bar yellow,
  labels in gold's blue family, title menu yellow with its black backing box gone.
  **The colour half of cross-cutting deviation #1 is solved.**
- Getting there cleared S62's two blockers:
  1. **The uninit garbage was `WM_GETSTRING`'s unchecked OUT param.** Three R* sites
     (`CRButtonCtrl::GetParentWndInfo` ×2, `CRStaticCtrl` ×1) assign the buffer without
     checking the returned length. `workspace[0]=99` is the IN capacity, so with no
     parent routing the message the caption became `'c'` + uninitialised stack. Latent
     until the reader supplied real `m_ResourceNumber` values.
  2. **Test recipes no longer use fixed pixels.** `BOB_CLICKSEQ` accepts `f,rN` (menu row)
     and `f,#ID[:COL]` (control by dialog id), both resolved from the controls' own
     metrics — so a font change can never silently invalidate the gate again.
- **Residual, renamed and narrower: font FACE and SIZE.** Native still uses the DejaVu
  fallback and renders *larger* than gold, loosening row density; labels read brighter
  cyan than gold's muted blue. That is S64's lead item.
- **Parity references were REBASED this sprint, not verified byte-identical** — the reader
  changes fonts/colours by design, so all six were regenerated. Byte-identical checking
  resumes in S64 against the new baselines.
- **Gold oracle:** the `BEA6-BBCE` USB is not mounted; the 14 gold shots are mirrored at
  `/home/admin/gold standard/ma/` and that mirror is what the S63 verdicts used.
- Next-sprint queue (S64): (1) **font face + size** — measure whether the size gap is a
  FontNum→point-size mapping issue before assuming it needs the art typefaces;
  (2) Player Log title bar + `?`/`✓` (now testable — the reader is replaying `IDJ_TITLE`'s
  bag); (3) Career content table (other half of I4); (4) RScrlBar hosting; (5) route real
  clicks to `ma_tabs_hit`; (6) `prefs_controls` embeds live joystick state and is not a
  stable oracle — capture with a synthetic device or mark it environment-dependent.
