# Sprint 31 — "Shippable polish" (H1 finish + window title)

_2026-06-29 · Product Owner pre-approved planning + review._

## Sprint Goal
Close the user-facing half of EPIC H (shippability): correct the window identity and give a
distributor/user real run + install instructions — so a fresh user can get from "I have the
binary" to "the game is running" without reading the source.

## Sprint Backlog & outcome

| Item | Detail | Result |
|---|---|---|
| Window title | SDL caption read "Rowan's Battle of Britain (Linux native port)" (cross-port string never updated) → **"Mig Alley (Linux native port)"** (`bob_video.cpp`). | ✅ |
| **H1** README run/install instructions | `README.md` was a 2-line stub ("Rowan's Battle of Britain source"). Rewrote it: what the port is + status, 32-bit runtime-lib install (multiarch), the bare-launch run recipe + env overrides, build + ASan instructions, repo layout. | ✅ |

## Definition of Done
- ✅ Compiles clean; full rebuild + link OK.
- ✅ **Demo:** `strings wmig` shows the new caption and **no** "Rowan's Battle" string; bare
  `./wmig` boots (SDL2 window, frame dump, exit 0) with the corrected title.
- ✅ README run instructions verified against the actual `ldd` deps (SDL2/GL/OpenAL/FluidSynth/
  libstdc++, all i386) and the verified bare-launch behaviour from S30.
- ✅ No regression.

## Backlog impact
- **H1** → ✅ done (data-dir resolution S30 + run/install README S31). The "packaged artifact"
  (a distro/AppImage) is the only residual, re-sliced to a future packaging task (H1-pkg).
- Play-test backlog: window-title item closed.

## Impediment flagged to PO (not blocking this sprint)
The **Replay hang** (debrief → "Replay" launches the unimplemented playback subsystem and
blocks) was scoped for this sprint but **needs interactive reproduction** — it is reached only
after flying a mission to the debrief screen and clicking Replay, which the headless harness
can't drive, so a graceful-degrade fix can't be DoD-demonstrated. Options for the PO:
(a) run one interactive session so the fix can be validated live, or (b) accept it as a
deferred known-issue (it degrades a non-core feature; core loop is unaffected). Same class as
the S21 Quit-hang / 4× speed bugs, which were also found by interactive play.

## Commit
`Sprint 31: window title -> "Mig Alley"; real README (H1 run/install instructions)`.

## Retro
- Picking headlessly-verifiable items kept the sprint cleanly closeable. The remaining
  play-test backlog (Replay hang, radar gunsight ranging, debrief Claims header, campaign-map
  wheel-zoom) is mostly **interactive-repro-gated** — batch these for a PO-driven play-test
  session rather than spending headless sprints guessing.
