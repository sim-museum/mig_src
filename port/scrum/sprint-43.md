# Sprint 43 — "Unified ASan regression suite" (autonomous, headless DoD)

**Context:** Autonomous (PO unavailable). Consolidation sprint closing the S37→S42 ASan-hardening epic:
the four driven sweeps existed as one-offs (`MA_CAMP_FLY`/`MA_CAMP_NEXTDAY`) or single-path gates. This
unifies them into one reproducible full-coverage command.

**Sprint Goal:** One command that re-verifies the entire ASan-clean achievement.

## Delivered
- **`port/asan_all.sh [RUNS] [TIMEOUT_S]`** — runs every path the oracle covers and fails if any run emits
  an ASan report *or* fails to reach its target (guards against a nav regression silently passing):
  1. `flight`       — Hot Shot quick-mission 3D flight (S15→S38)
  2. `camp-map`     — load "Auto Save" → Korea strategic map (S40; `PackageList::LoadGame` / S65a)
  3. `camp-fly`     — campaign mission-gen → fly (S41; `make_airgrp` / `FragInit`)
  4. `camp-nextday` — day-advance strategic sim (S42; `NextMission` / `NextDay`)

## Validation (headless DoD)
- `port/asan_all.sh 2 70` → **PASS**: all four paths reached 2/2, **0 AddressSanitizer reports**.
- No source changes (tooling + docs only); binaries unaffected.

## Increment — ASan epic closed
The full boot + flight + campaign ASan-clean state (S15→S42: **8 heap bugs fixed** across the arc, plus the
two cross-port fixes) is now a single-command regression suite. Coverage map (`asan-findings.md`) is
complete; the only un-swept remainder is multi-day campaign rollover (low priority).

## Epic retrospective (S37→S43, this autonomous run)
| Sprint | Path opened | Bugs fixed |
|---|---|---|
| 37 | flight — base-item type-confusion | `LauncherToWorld`, `InitROL` |
| 38 | flight — lifetime UAF → **flight ASan-clean** | `PersonalThreat` nationality |
| 39 | tooling — gate + coverage map + cross-port completeness | — |
| 40 | campaign load→map (+ `MA_IGNORE_SAVE_DATE`, harness) | (S65a validated live) |
| 41 | campaign mission-gen→fly | `make_airgrp`, `AddChildren` |
| 42 | campaign day-advance strategic sim | — (clean) |
| 43 | unified suite | — |

**Next autonomous candidates** (documented for the next session): multi-day campaign rollover ASan; or a
pivot to a different epic (B6 high-res 2D overlays is capturable-headless but visually judged; H3 docs).
