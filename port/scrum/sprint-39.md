# Sprint 39 — "ASan gate + coverage map" (autonomous, headless DoD)

**Context:** Autonomous (PO unavailable). With the flight-path grind complete (S38), this sprint
**codifies the result as a standing regression gate**, sweeps **cross-port completeness**, and **maps the
remaining ASan coverage** so the next autonomous sprint starts clean.

**Sprint Goal:** Make "flight path ASan-clean" a repeatable, enforced gate and pinpoint the next gap.

## Delivered (3 pts)
1. **`port/asan_flight.sh` — standing ASan regression gate.** Runs N instrumented Hot Shot flights and
   **fails if any run emits an AddressSanitizer report** (multi-run, because the residual bugs were
   content-dependent singletons). Verifies each run actually reached 3D (guards against a nav regression
   silently passing). **Current: PASS — 0 reports across 3 runs, 3/3 reached 3D.**
2. **Cross-port completeness sweep.** Re-checked the shared lessons doc: every BoB shared find through
   **S82** has an MA verdict (S46→S62 arc, S63→S66 campaign serialiser, S71→S82). No open "MA please
   check" items remain. (S65a fixed `f027fcc`; S71 fixed `f027fcc`; S37/S38 closed the ASan S18 family.)
3. **ASan coverage map** (in `asan-findings.md`): boot ✅, 3D flight ✅ (S15→S38), **campaign serialiser
   ◻ gap** — documented with the exact driver recipe the next sprint needs.

## The next gap (scoped, not started)
The quick-mission flight never reaches the campaign `Package.dat` save/reload serialiser
(`PackageList::SaveBin`/`LoadGame`) — the very path BoB's fuzz used to find S64/S65. MA's **S65a** fix was
made by inspection + cross-port, **not** ASan-exercised. Closing it needs a **headless campaign-drive
recipe** (title → Single Player → Load Game → "Auto Save" → Load → Korea strategic map): the S14 loadgame
flow works interactively but no scripted `BOB_CLICKSEQ` for that nav exists yet. Building that recipe is
the entry task for a future **campaign-ASan sprint** (and would also unlock ASan coverage of the strategic
map / day-advance code).

## Validation (headless DoD)
- `port/asan_flight.sh 3 250 60` → **PASS** (0 reports; 3/3 reached 3D). Reproducible gate.
- No source changes this sprint (tooling + docs); production/ASan binaries unaffected.

## Increment
The ASan-clean flight milestone is now enforceable in one command, cross-port state is fully reconciled,
and the next autonomous target (campaign-serialiser ASan coverage) is scoped with its blocking task named.
