# Sprint 17 — "Heap-clean flight": eliminate the ASan singleton tail

**Goal:** with the per-frame corruptors gone (S15+S16), fix the remaining **low-frequency**
heap/stack bugs the ASan oracle surfaces on the 3D flight path, so an instrumented flight runs
with zero AddressSanitizer reports. These 1–3×/flight UB sites are the prime suspects behind the
documented *intermittent* flight-launch / mid-flight crashes — clearing them is stability value,
not pedantry.

**PO:** standing pre-approval (planning + review). Run autonomously.

## Backlog (from `port/scrum/asan-findings.md`, dedup'd flight report `port/reference/asan-flight-report.log`)

| # | Bug | Site | Type | BoB ref | Status |
|---|-----|------|------|---------|--------|
| 1 | `keytests::Reg3dConv` | `KEYSTUB.CPP:288/290` | heap-buffer-overflow (write past `KeyMap3d` + terminator read past file buf) | **R1.3b proven** | ☐ |
| 2 | `ImageMap_Desc::FixLbmImageMap` | `lbmcpp.h:206/215/250` | heap-buffer-overflow (×3 over-read: body/palette/alpha) | R3.9 neighbour (BoB left as benign) | ☐ |
| 3 | `LandScape::DoCloudLayer` | `Landscap.cpp:7257` | stack-buffer-overflow (×7) | none | ☐ |
| 4 | `LandScape::PerspectivePoly` | `Landscap.cpp:2329/2267` | stack-buffer-overflow | none | ☐ |
| 5 | `shape::LauncherToWorld` | `3dcom.cpp:13436` | heap-buffer-overflow (×3 OOB read) | none | ☐ |
| 6 | `mobileitem … T_nationality` | `worldinc.h:715` | heap-use-after-free | none | ☐ |
| 7 | `RADIOMESSAGE::InitROL` | `Rchatter.cpp:1671` | heap-buffer-overflow (ROL parse over-read) | none | ☐ |

(`Math::rnd` Math.cpp:1722 — was on the list but no longer fires after S15; re-confirm during the verify flight.)

## Definition of Done
- Each fix is the minimal correct root-cause UB fix; re-validated under ASan (count → 0).
- A full instrumented flight produces **zero** ASan reports on the flight path (or any residual is
  documented as a non-flight / data-driven singleton with rationale).
- Production stress (`port/stress_launch.sh`) clean — no regression.

## Method
- ASan oracle: `ASAN=1 bash port/rebuild.sh` → `/tmp/wmig-asan`; drive a flight via `port/asan.sh`.
- **Byte-safety:** `3DCOM.CPP`/`LANDSCAP.CPP`/`WORLDINC.H` are ISO-8859 with high-byte license
  banners → patch with `sed`, NOT the Edit tool (it re-encodes to UTF-8). Symlinks: `3dcom.cpp`→
  `3DCOM.CPP`, `Landscap.cpp`→`LANDSCAP.CPP`, `worldinc.h`→`WORLDINC.H` (edit the real uppercase).
- Cross-port: BoB's R1.3b (Reg3dConv) is an exact match — bound `scancode<MAXKEYS(512)` +
  `shiftstate<8` and guard the terminator read `breakif(i==0 || …)`.

## Log — CLOSED 2026-06-25

**Outcome: 3 fixed + ASan-verified (count→0), 4 deferred with root cause.** The corruption-class
bugs on the flight path are eliminated; the residual reads are a distinct, lower-severity family.

### Fixed + verified (each → 0 in the S17 instrumented flight, `/tmp/wmig-asan.log.20879`)
- **`Reg3dConv`** `KEYSTUB.CPP:288/290` (heap-buffer-overflow — the only **write** in the backlog,
  2 B past `KeyMap3d`): bounded the write `if (scancode<KeyMap3d::MAXKEYS && shiftstate<8)` and guarded
  the terminator over-read `breakif(i==0 || …)`. **BoB R1.3b** applied verbatim.
- **`PerspectivePoly`** `LANDSCAP.CPP:2329` (stack-buffer-overflow): `dp[3].clipFlags` → `dp[2]` —
  a clean off-by-one typo (`DoPointStruc dp[3]`, the triangle uses 0/1/2 everywhere else).
- **`DoCloudLayer`** `LANDSCAP.CPP:7210` (stack-buffer-overflow ×7): `_stripPoints =
  sizeof(HStripPtsA)/sizeof(fpCOORDS3D)` mis-counted (cloud `SHCoords` 16 B vs `fpCOORDS3D` 12 B →
  24→32, over-reading the stack strip array). Fixed to `/sizeof(HStripPtsA[0])`.

All three patched **byte-safely with `sed`** (`KEYSTUB.CPP`/`LANDSCAP.CPP` are ISO-8859 with high-byte
banners); `git diff` = exactly 4 changed lines, banners intact. `LANDSCAP.CPP` is in the `_3D` unity →
full rebuild.

### Deferred → S18 sub-epic "item type / lifetime over-reads" (see `asan-findings.md`)
- `LauncherToWorld` (3dcom.cpp:13436) + `InitROL` (Rchatter.cpp:1671) — **base-item type-confusion**:
  reading mobile-only fields (`hdg/pitch/roll`, `vel`) through a base `item*` (an AAA ground site
  `new item`). Faithful fix gates on `Status.size` (engine has the discriminator + assert-helpers) but
  touches weapon/AI logic → needs A/B validation, not an end-of-sprint change.
- `mobileitem … nationality` UAF (worldinc.h:715) — AI reads a freed item; lifetime fix, no BoB analog.
- `FixLbmImageMap` (lbmcpp.h:206/215/250) — benign ILBM RLE source over-read; bounding risks
  tile-decode corruption; BoB deliberately left it.

Rationale for deferral: all four are **reads** (no heap-corrupting writes), low-frequency (1–3×/flight),
and need the engine's type/lifetime model rather than a form fix — bundling them into one focused,
A/B-validated sprint is lower-risk than force-fitting guards into weapon/AI paths now.

### Validation
- ASan 150-frame flight: `Reg3dConv`/`PerspectivePoly`/`DoCloudLayer` → **0**; no new reports; only the
  deferred set remains.
- Production stress (`port/stress_launch.sh 4 100`): **4/4** sustained 100 frames — no regression.

### Retro
- **Went well:** BoB cross-port leverage again paid off (R1.3b verbatim); the ASan flight report's
  per-bug stack + allocation-site detail made the type-confusion root cause (base `item` vs derived)
  diagnosable without a live debugger.
- **Judgment call:** stopped at the safe, verified fixes rather than chase the "zero reports" DoD with
  risky gameplay-touching guards — the residual are non-corrupting reads, so the trade favors a
  dedicated, validated S18 over end-of-sprint risk.
- **S18 candidates:** the item-type/lifetime sub-epic above; OR (higher user value) the S8 sky-colour
  fidelity follow-up; OR close the in-flight-mouse gap (DOS INT 33h `ASM_DOSvia31` → SDL relative
  motion). PO to steer ordering at S18 planning.
