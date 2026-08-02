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

## Current state (2026-08-01, after Sprint 64)

- **S63 (8/8) put the persisted design-time property reader ON by default** — the front end
  draws with its authored colours: setting **values yellow matching gold**, Preferences tab
  bar yellow, labels in gold's blue family, title menu yellow with its black box gone.
  **The colour half of cross-cutting deviation #1 is solved.**
- **S64 (6/8, partial) mostly deleted wrong work, which was its value:**
  - S63's residual "native renders LARGER than gold" is **false**. Measured: glyph band
    10 px (gold) vs 11 px (native), row pitch 52 vs 51 — the **same absolute font size**.
    The apparent difference was comparing a 1280×1003 gold shot with an 800×600 native
    capture. **Rule now explicit: the game picks its panel ART SET by resolution, so no
    parity verdict may rest on relative size, spacing or density.** Cross-cutting #1's
    residual is re-scoped to **font FACE only**.
  - Two real bugs fixed: **`GetFileNum()` was a stub returning 0** (so every control whose
    art is named rather than numbered silently lost its artwork — the sound half of BoB's
    trap 2), and **`CString(LPCWSTR)` was declared but never defined** (link-only failure).
  - Applying persisted art names is implemented but **shipped OFF** (`MA_BTN_ART_NAMES=1`):
    it resurrected the invisible system-box "Quit"/"Size" buttons on every front-end
    screen. Caught by the parity sweep.
- **Open, third sprint running: the Player Log title bar.** Now precisely located rather
  than mysterious — `ma_px_replay` **never fires for id 1001**, so IDD 276's bag for
  `IDJ_TITLE` is not reaching `bagmap` even though its DLGINIT stream starts with `e9 03`.
  A bag storage/keying question in the RT_DLGINIT walk, not an art or draw question.
- **Test recipes are font-independent** (S63): `BOB_CLICKSEQ` takes `f,rN` (menu row) and
  `f,#ID[:COL]` (control by dialog id). Never re-introduce fixed pixel rows.
- **`prefs_controls` is not a stable oracle** — it embeds live joystick state; excluded
  from the byte-identical sweep. The other five screens carry the gate.
- Gates: **parity 5/5 byte-identical** (check resumed in S64 after S63's deliberate
  rebase), ASan **4/4 modes 0 reports**, stress **20/20**.
- Next (S65): (1) why `ma_px_replay` skips id 1001 → the title bar; (2) a narrowing
  criterion so persisted art names can ship on; (3) font FACE, and whether a 1280-res
  capture path is worth building; (4) Career content table; (5) RScrlBar hosting;
  (6) route real clicks to `ma_tabs_hit`; (7) #12 debrief capture.
