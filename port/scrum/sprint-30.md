# Sprint 30 — "Just run it" (H1: bare launch, no env vars)

_2026-06-29 · Product Owner pre-approved planning + review._

## Sprint Goal
Make `./wmig` from the install dir boot the game with **no environment variables** — the
gateway to a shippable, distributable build (EPIC H). Mirror the sister BoB port, which
already boots bare `./bob`.

## User Story
**H1** — *As a user, I can install and run without manual env vars, so it's distributable.*
Acceptance (this slice): the launcher resolves the data dir from the cwd and auto-runs; the
old explicit-env path and a link-only safe default are both preserved.

## What shipped
`SRC/compat/bob_main.cpp`:
- **Derive `BOB_DRIVE_C` from cwd** — when neither `BOB_DRIVE_C` nor `BOB_GAME_DIR` is set,
  walk the cwd to its last `/drive_c` ancestor (the Wine-style install tree
  `<drive_c>/rowan/mig`) and set it. The existing chdir block then resolves the engine's
  relative `.` paths unchanged.
- **Auto-run gate** — drive `CMIGApp::InitInstance` whenever the data path is known
  (`haveData`), not only on explicit `BOB_RUN_INIT=1`.
- **Escape hatches preserved** — `BOB_RUN_INIT=1` force-runs; `BOB_NO_RUN` / `BOB_RUN_INIT=0`
  force link-only; explicit `BOB_DRIVE_C`/`BOB_GAME_DIR` override; a launch from a non-install
  dir with no env finds no data → stays link-only (no accidental run).

## Definition of Done
- ✅ Compiles clean; full rebuild + link OK (8.69 MB ELF).
- ✅ **Demo:** `cd <drive_c>/rowan/mig && ./wmig` (zero env) → `derived BOB_DRIVE_C=…`,
  Mig.exe resources loaded, `InitInstance() returned 1`, `Entering Run()`, SDL2 window,
  rendered frame 120, exit 0.
- ✅ Regression / hatch matrix verified: non-install dir → link-only; `BOB_NO_RUN` → link-only
  (accurate message); explicit `BOB_RUN_INIT=1 + BOB_DRIVE_C` → unchanged.
- ✅ STATUS.md run command updated to the bare form.

## Remaining in H1 (next, smaller)
Packaged artifact + README run instructions (the data-dir resolution — the hard part — is done).
Window title still reads "Rowan's Battle of Britain" (cross-port string; play-test backlog).

## Commit
`Sprint 30 (H1): bare launch -- derive data path from cwd, auto-run (no env vars)`.

## Retro
- Clean cross-port win: BoB's `bob_main.cpp` recipe dropped in near-verbatim (MA's data model adds
  the `rowan/mig` chdir, already present). Small, high-leverage, demonstrable — exactly the kind of
  shippability item to keep pulling now that the gameplay loop is functional.
