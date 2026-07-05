# Sprint 41 — "Campaign mission-gen ASan sweep" (autonomous, headless DoD) — 2 bugs fixed

**Context:** Autonomous (PO unavailable). Extends the S40 campaign coverage from *load→map* into *mission
generation → fly*, using the existing `MA_CAMP_FLY` auto-drive hook. This is campaign-only code (distinct
from the quick-mission flight already swept), and it immediately paid off.

**Sprint Goal:** Sweep the campaign mission-generation + fly path under ASan; fix what it finds.

## Recipe (headless campaign fly)
```
MA_ENABLE_3D=1 MA_IGNORE_SAVE_DATE=1 MA_CAMP_FLY=1 \
BOB_CLICKSEQ="30,588,263;65,40,108;100,68,565"
```
Load "Auto Save" → strategic map → (`MA_CAMP_FLY`) `OnClickedFrag2` → singlefrag briefing → Fly →
`FragFly` → `StartFlying` → campaign 3D mission. Exercises `Persons2::LoadSetPiece`/`Persons3::make_airgrp`
(campaign mission build) + `RFullPanelDial::FragInit` (briefing-panel construction).

## Bugs found + fixed (both campaign-only; the flight/load sweeps never reach them)

### 1. `make_airgrp` global-buffer-overflow (`Persons3.cpp:836`)
`GR_Pack_TakeTime[GR_WaveNum-1][gotgrpnum]` read with **`gotgrpnum == -1`** (the unset-group sentinel) →
a **negative second index**, reading 4 bytes before `GR_Pack_TakeTime[8][3]` (lands in the adjacent global
`GR_Scram_Squad`). Distinct from BoB **S54** (`GR_Scram_*[8]` indexed *>8*) — so the earlier "S54 not shared"
verdict stands; this is a **new MA campaign find**. **Fix:** gate the per-group take-off lookup on
`gotgrpnum ∈ [0,3)`; with no valid group, keep the pre-set `GR_TakeOffTime` / `TOS_SIMPLE` default.

### 2. `AddChildren` stack-use-after-scope (`RDIALOG.CPP:537`)
`dial->edges = *diallist[i]->edges;` dereferenced a `DialBox::edges` (`const Edges*`) pointing at a **dead
stack temporary**. `RFullPanelDial::FragInit:3373` builds a **named local** `DialBox topbit(..., EDGES_NOSCROLLBARS_NODRAGGING)`;
the inline `EDGES_` macro expands to an `Edges(...)` temporary that dies at the end of that declaration
statement, but `DialBox` stored `&temporary` → `topbit.edges` dangles for the rest of `FragInit`, and
`AddChildren` (called later inside `LaunchDial`) reads `*edges`. (The `DialBox` temporaries *inside* the
`LaunchDial(...)` full-expression stay alive; only the earlier named local dangles.) **Fix:** give `topbit`'s
`Edges` function-scope lifetime — a named `const Edges` local declared before it. Localised to `FragInit`;
no change to the core `RDIALOG.H` `DialBox`.

Both `#if defined(MA_LINUX)`, byte-preserving edits on ISO-8859 source.

## Validation (headless DoD)
- Re-ran the `MA_CAMP_FLY` ASan sweep: **0 ASan reports** (both were present pre-fix); the chain still
  completes frag → singlefrag → `StartFlying` → 3D (back-surface dumped).
- **No regression:** `port/asan_flight.sh` PASS, `port/asan_campaign.sh` PASS, `port/stress_launch.sh` 4/4.
- Compiles clean (`_BFIELDS` unity + the `FULLPANE` MFC fragment); `wmig` links (8.7 MB), 0 undefined symbols.

## Increment
ASan coverage now reaches **campaign mission generation + fly** — and it took two real, campaign-only heap
bugs with it (a negative-index global overflow in the mission builder, and a dangling-`Edges` stack
use-after-scope in the briefing-panel construction). Confirms the S39/S40 thesis: the campaign path carries
its own bugs, invisible to the quick-mission sweeps.

**Next:** in-campaign day-advance / `PackageList::SaveBin` writeback (needs campaign progression past a
single mission) — the last un-swept campaign serialiser half.
