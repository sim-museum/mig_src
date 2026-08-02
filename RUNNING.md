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

## Current state (2026-08-01, after Sprint 62)

- **Sprint 62 CLOSED PARTIAL (5/8).** BoB's persisted-property-stream reader is adopted
  and **works** — all 58 bags on the boot path parse clean — but it ships **OPT-IN**:
  run with `MA_DLGINIT_PROPS=1` to enable it. The default path is byte-identical to S61.
- **What it buys, when enabled (gold-verified):** Preferences goes from white-serif
  labels to gold's **blue labels + yellow values**, and the title menu turns yellow.
  That solves the **colour half of cross-cutting deviation #1**, the port's biggest
  remaining visual gap. The font *face* half remains.
- **Why it is off by default — two measured blockers:**
  1. an **uninitialised read** shows as garbage text at the title screen's top-left. It
     *varies between runs*, which is the tell for uninit rather than a bad persisted
     value. Bisected past the stock caption and `PX_String`; not yet root-caused.
     Reproduce in ~2s: `MA_DLGINIT_PROPS=1 … MA_SHOT=30`.
  2. the persisted FontNum **changes the title-menu row pitch (~16px → ~28px)**, so every
     fixed-coordinate `BOB_CLICKSEQ` recipe lands on the wrong row — the `quickmission`
     capture came back showing Preferences. This breaks the parity capture recipes AND
     `asan_all.sh`'s drive recipes together.
- Sprints 60–61 before it took the campaign-map **Player Log** from a bare photo blit to a
  real tabbed dialog centred over the map (tab bar with genuine RTabs.ocx art, tab
  switching capture-proven). Parity #15 is **CLOSE-minus**.
- **⚠ `port/ref/native/prefs_controls.png` is not a stable reference** — that screen
  enumerates live hardware and the capture was taken with a joystick attached. With no
  joystick the panel reads "NOT CONNECTED / 0 axes" and the byte-compare fails for purely
  environmental reasons. Check `/dev/input/js*` before treating it as a regression.
- Gates (default path): parity **6/6 unregressed**, ASan **4/4 modes, 0 reports**,
  stress **20/20**.
- Next-sprint queue (S63 — now a well-defined sprint): (1) **root-cause the uninit
  garbage** (S61 precedent: unchecked Win32 stub + uninit locals); (2) **make the headless
  recipes font-independent** — drive by menu-row index instead of pixel coordinates, which
  pays back on every future font change; (3) switch the reader on by default and
  **re-verdict the whole parity set**; (4) Player Log title bar + `?`/`✓`; (5) Career
  content table (the other half of I4); (6) **RScrlBar still unhosted**.
