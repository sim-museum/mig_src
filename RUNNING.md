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

## Current state (2026-08-01, after Sprint 61)

- **Sprint 61 CLOSED 7/8 pts — goal MET.** The campaign-map **Player Log now renders as a
  real tabbed dialog, centred over the map**: Career / Log of Missions / Last Mission tab
  bar with the genuine RTabs.ocx art, the selected tab raised, the pilot photo and the
  Name label + edit box, over a map showing the toolbar and the
  `6/25/50: Morning, planning` date row. **Tab switching is capture-proven**
  (`MA_OOB_PLAYERLOG_TAB=N`; see `port/ref/native/map_playerlog_tab1.png`).
  Parity **#15 PARTIAL → CLOSE-minus**.
- Four defects got it there, all looking like "the dialog is in the wrong place":
  1. **`borderwidth` was uninitialised heap** — startup never checks
     `RegQueryValueEx`'s return and the compat stub writes neither output, so a
     *different* garbage dialog origin came out on every run.
  2. **`OnGetXYOffset` was built on no-op `ClientToScreen`** — every dialog reported
     offset ~0, so the whole tree composited at the top-left.
  3. **`IDJ_PANEL0..9` placeholders were unregistered** — `AddChildren` stacked children
     *below* the parent instead of inside it.
  4. **The title-height nudge double-counted**, shifting the tab strip 27px under the
     page art.
- Sprint 60 (before it) landed the machinery: RTabs hosted, kind-driven template control
  hosting, and `MaSeedTemplateSize()`.
- **Open, and now the top backlog item: the R* RT_DLGINIT property-stream reader** (BoB
  note 17 traps 1/2). The Player Log's "PLAYER LOG" title bar and its `?`/`✓` buttons
  still draw nothing — `IDJ_TITLE` is in the template, hosted and unfiltered, but its art
  and caption live in that unparsed stream. It also unlocks the FONT/COLOR set behind the
  cross-cutting font deviation #1.
- Gates: **2D parity sweep 5/5 byte-identical** (title / prefs_3d / prefs_controls /
  quickmission / **campaign_map**), ASan **4/4 modes, 0 reports**, stress **20/20**.
- Next-sprint queue (S62): (1) **property-stream reader** → title bar + `?`/`✓` + fonts;
  (2) Career content table (Sorties/Combats/Kills/Losses — the remaining half of I4);
  (3) **RScrlBar unhosted** though created 16× on the map path; (4) route real mouse
  clicks to `ma_tabs_hit` (written, currently only the scripted SelectTab path is
  exercised); (5) #12 debrief capture; (6) RRadio OCX hosting.
