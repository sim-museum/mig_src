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

## Current state (2026-08-01, after Sprint 67)

- **The Player Log is done as a dialog**: tab bar + centring + tab switching (S61), authored
  colours (S63), the "PLAYER LOG" title bar (S65), and as of S67 the title bar is **trimmed
  to the dialog width**. Parity **#15 = CLOSE-minus**.
- ⭐ **Cross-cutting deviation #1 (font/typeface) is SOLVED** — colour S63, FACE S66. The
  game ships `Intel.ttf` ("Copyright (c) Rowan Software, 1998") and stb_truetype had been
  **rejecting it** over a (3,0) SYMBOL cmap, so every run silently fell back to a system
  serif. **Largest remaining visual deviation is now cross-cutting #2, combo chrome.**
- **S67 added a clip region to the GDI layer.** Windows clips a control's drawing to its own
  window; ours never did, so `CRButtonCtrl`'s natural-size DIB blit let the title art run
  ~213px past the dialog and over the map. `ma_gdi_set_clip`/`ma_gdi_restore_clip` are
  honoured by the pixel-put, BitBlt and StretchBlt paths and applied around each button's
  `OnDraw`.
- ⚠️ **STILL OPEN AND UNATTRIBUTED: the intermittent ASan `stack-use-after-return`**
  (`worldinc.h:257` `T_size::operator ITEM_SIZE()`, `worldinc.h:565`
  `T_shape::operator ShapeNum()`). Seen once in ~20 runs at S66; S67's dedicated hunt (all
  modes, `detect_stack_use_after_return=1` forced) found **no recurrence** — but it was
  stopped early after ~4 runs (CPU contention with the gate), so no rate can be claimed:
  2 reports in the first 8 runs, then ~16–24 clean. **A clean run on the current build
  cannot attribute it.** The test that
  would (build the pre-S66 ASan binary and run it the same number of times) has NOT been
  done. **S68 must either do that A/B or consciously downgrade this to a watch item — it
  must not be dropped because a run came back clean.**
- **`?`/`✓` buttons — narrowed, not solved:** they are NOT template controls (IDD 276 has
  only 1001 and 1117) and NOT in the `FIL_TITLEB_BMP` art (the pre-clip capture showed all
  550px of it). They come from RDialog chrome drawn elsewhere.
- **Known-imperfect:** `ma_gdi_font_create` still ignores the face name, so everything draws
  in the art face. Matches gold — by luck, not correctness. (S67-3, not started.)
- Gates: parity **4/4 byte-identical** (`map_playerlog` re-based for the trimmed title bar;
  `prefs_controls` excluded as environment-dependent). ASan/stress per the sprint log.
- **Process rule earned twice: FILTER, DON'T CAP.** A `static int n; if (n++<N)` trace
  budget is always spent by whatever happens early in a run; a predicate on what you are
  looking for cannot be starved. This trap produced a wrong root cause in S64 and repeated
  in S67 one sprint after being documented.
- Next (S68): (1) **ASan A/B or explicit downgrade**; (2) what draws `?`/`✓`;
  (3) per-face font selection; (4) cross-cutting **#2 combo chrome**; (5) Career content
  table (other half of I4); (6) RScrlBar hosting; (7) `ma_tabs_hit` click routing.
