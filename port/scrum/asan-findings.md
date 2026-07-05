# ASan findings — heap-bug oracle backlog

The `ASAN=1` build (`port/asan.sh`, Sprint 15) is the heap-corruption oracle — the single most
productive hardening tool on BoB (the sibling Rowan-engine port). This catalogues what it has
surfaced. Each line is a real root-cause UB fix (usually 1–2 lines), to be fixed + re-validated
(count → 0) one at a time, logged here with evidence.

Run: `port/asan.sh build` then drive a path (boot / 3D flight / campaign map). Reports →
`/tmp/wmig-asan.log.<pid>` (also stderr). Raw flight report preserved at
`port/reference/asan-flight-report.log`.

**Standing regression gate (S39):** `port/asan_flight.sh [RUNS] [DUMP_FRAME] [TIMEOUT_S]` runs N
instrumented Hot Shot flights and **fails if any run emits an ASan report** (multi-run because the tail
bugs were content-dependent singletons). Current state: **PASS — 0 reports.** Re-run after any change to
the flight/move/AI/collision/landscape code.

### Coverage map (which driven paths the oracle has swept)
| Path | Driver | State |
|------|--------|-------|
| Boot / front-end init | boot to title | ✅ clean (S15) |
| 3D flight (move/AI/collision/landscape/weapons) | `asan_flight.sh` / Hot Shot | ✅ **clean (S15→S38)** |
| Campaign serialiser (`PackageList::SaveBin`/`LoadGame`, strategic map) | **not yet driven under ASan** | ◻ **gap** — see below |

**Campaign-path coverage gap (next ASan sprint):** the quick-mission flight never reaches the campaign
`Package.dat` save/reload serialiser. This is exactly where BoB's fuzz found S64/S65; MA's twin **S65a**
(`MAPCODE.CPP` `LoadGame` `new char[]`/scalar `delete`) was fixed by inspection + cross-port (commit
`f027fcc`) but has **not** been ASan-exercised. To close it, drive the loadgame flow under ASan (S14 recipe:
title → Single Player → Load Game → select "Auto Save" → Load → the Korea strategic map renders) — a
scripted `BOB_CLICKSEQ` for that nav does not exist yet (S14 documented the internals, not the click
coordinates). Building that headless campaign-drive recipe is the entry task for the campaign-ASan sprint.

## Boot path — ✅ CLEAN (Sprint 15)
| Bug | Site | Fix | Status |
|-----|------|-----|--------|
| heap-overflow, dir-list 1-byte over-read (×2) | `Fileman.cpp:454/586` `translatedirlist`/`retranslatedirlist` | guard `datalength>0` before `*datascan` (`&&` short-circuit) | ✅ fixed + verified |
| global-overflow, `resolutions[-1]` | `FULLPANE.CPP:2033` `LaunchScreen` | resolve `m_currentres` (`==-1 → GetCurrentRes()`) before index | ✅ fixed + verified |
| uninit listbox size → title menu vanished under ASan | `RLISTBXC.CPP` ctor (`m_FontNum2`/`m_vertSeperation` unset → garbage font metrics → garbage `MoveWindow` size) | init font/seperation fields to PX_Long defaults in ctor | ✅ fixed + verified (also unblocked ASan flight nav) |

## 3D flight path — surfaced 2026-06-24, in progress
Counts are per ~30-frame instrumented flight (`MA_ENABLE_3D=1`, F-86 cockpit).

| Count | Type | Site | Notes / fix | Status |
|------:|------|------|-------------|--------|
| 16576 | alloc-dealloc-mismatch | `3dcom.cpp:10953` `shape::DrawSubShape` | `subco = new DoPointStruc[64]` freed scalar → `delete[]` | ✅ **fixed (S15)** |
| 7510 | new-delete-type-mismatch | `worldinc.h:708` `mobileitem::operator delete` | `{::delete(MovingItemPtr)obj;}` re-ran `~MovingItem` on an already-destructed base (double-destruction; the `delete ip` at `Viewsel.cpp:8161` already ran the dtor chain) → `{::operator delete(obj);}` (free only). **BoB R1.3d/e.** | ✅ **fixed (S16)** |
| 7510 | alloc-dealloc-mismatch | `3dcom.cpp:10386` `shape::dodigitdial` | `digits = new UByte[nodigits]` freed scalar → `delete[]` | ✅ **fixed (S16)** |
| 139 | alloc-dealloc-mismatch | `Landscap.cpp:730` `LandScape::ManageHighLandTextures` | `droppedTextures = new Dropped` (scalar) freed `delete[]` → plain `delete` (opposite of the usual; same site BoB R3.9 flagged) | ✅ **fixed (S16)** |
| 3 | heap-buffer-overflow | `3dcom.cpp:13436` `shape::LauncherToWorld` | OOB read — **base-item type-confusion**: `generate2(tmpitm->hdg/pitch/roll)` on a base 32-byte `item` (AAA ground site, no orientation). Fix (S37): gate the read on `itm->Status.size >= RotatedSize`; else identity orientation. | ✅ **fixed (S37)** — verified 0 |
| 7 | stack-buffer-overflow | `Landscap.cpp:7257–7289` `DoCloudLayer` | `_stripPoints=sizeof(HStripPtsA)/sizeof(fpCOORDS3D)` miscount: cloud `SHCoords` is 16B, `fpCOORDS3D` 12B → over-counts 24→32 → over-reads the stack array. Fix: `/sizeof(HStripPtsA[0])` | ✅ **fixed (S17)** — verified 0 |
| 1 | stack-buffer-overflow | `Landscap.cpp:2329` `PerspectivePoly` | `dp[3].clipFlags` on `DoPointStruc dp[3]` (valid 0–2) — typo for `dp[2]` (the triangle is dp[0]/dp[1]/dp[2] everywhere else). Fix: `dp[3]`→`dp[2]` | ✅ **fixed (S17)** — verified 0 |
| 1 | heap-use-after-free | `worldinc.h:715` `mobileitem … T_nationality` | **use-before-validate**: `ArtInt::PersonalThreat` (`Msgai.cpp:1794`) compares `trg->nationality==agg->nationality` **one statement before** its own `trg&&agg&&Status.size==AIRSTRUCSIZE` guard. `agg` = the bullet's `Launcher` (`BoxCol::DoCollision`), which can dangle if the firing aircraft left the world mid-flight. Fix (S38): early `return(false)` on the validity predicate before the deref (behaviour-preserving — the fall-through path already returns false). | ✅ **fixed (S38)** — verified 0 across 5 flights |
| 1 | heap-buffer-overflow | `KEYSTUB.CPP:288/290` `Reg3dConv` | scancode/shiftstate index + terminator — **BoB R1.3b exact match**. Fix: `if(scancode<MAXKEYS && shiftstate<8)` write-guard + `breakif(i==0 \|\| …)` terminator-read guard | ✅ **fixed (S17)** — verified 0 |
| 3 | heap-buffer-overflow | `lbmcpp.h:206/215/250` `FixLbmImageMap` | ILBM PackBits RLE decoder reads compressed source `*c++` past body/palette/alpha — **benign reads, bounding risks tile-decode corruption** (BoB deliberately left this) | ☐ deferred (benign, BoB's call) |
| 1 | global-buffer-overflow | `Math.cpp:1722` `MathLib::rnd()` | RNG table index past end (intermittent) | ☐ not seen since S15 (re-confirm) |
| 1 | heap-buffer-overflow | `Rchatter.cpp:1671` `RADIOMESSAGE::InitROL` | `target->vel` (MovingItem field) read through a `mobileitem*` that can point to a base `item` — **same base-item type-confusion** as LauncherToWorld. Fix (S37): gate on `target->Status.size >= MovingSize`; else zero velocity. | ✅ **fixed (S37)** — verified (content-dependent; no longer in the family) |
| 1 | alloc-dealloc-mismatch | `3dcom.cpp:18593` `shape::SetPilotedAcAnim` | `new UByte[]` freed scalar → `delete[] (UByte*)` — **BoB R1.3a/R3.9 match** | ✅ **fixed (S15)** |

**S15 post-fix re-validation (instrumented flight):** `DrawSubShape` 16576→**0**, `SetPilotedAcAnim`
1→**0** (and `Math::rnd` no longer fires); flight still reaches 3D; production stress 4/4. The two
dominant per-frame corruptors are gone. Remaining counts vary run-to-run with flight duration/content.

**S16 post-fix re-validation:** `dodigitdial` 7510→**0**, `mobileitem::operator delete` 7510→**0**,
`ManageHighLandTextures` 139→**0**; flight still reaches 3D; production stress 4/4. Across S15+S16 the
five **high-frequency** corruptors (~31.7k invalid heap ops/flight) are eliminated. **What remains is
all low-frequency singletons (1–3×/flight):** `LauncherToWorld` (heap-overflow ×3), `DoCloudLayer`
(stack-overflow ×7) + `PerspectivePoly`, `mobileitem … nationality` UAF (`worldinc.h:715`),
`Reg3dConv` (**BoB R1.3b**), `Rchatter::InitROL`, `FixLbmImageMap` (×3). These fire once per flight,
not per-frame — much lower severity. Sprint 17 backlog.

**S17 post-fix re-validation (instrumented 150-frame flight, `/tmp/wmig-asan.log.20879`):**
`Reg3dConv` → **0**, `PerspectivePoly` → **0**, `DoCloudLayer` → **0** (the ×7 stack overflow gone);
no new reports introduced; production stress **4/4**. The residual flight-path reports are exactly the
deferred set: `FixLbmImageMap` (×3), `LauncherToWorld` (×3), `mobileitem … nationality` UAF (×1).
`InitROL` and `Math::rnd` did not fire this run (content-dependent singletons).

**S37 post-fix re-validation (instrumented ~200-frame Hot Shot flight, `/tmp/wmig-asan.log`):** the
**base-item type-confusion pair is eliminated** — `LauncherToWorld` heap-buffer-overflow (×3) → **0**;
`InitROL` did not fire (content-dependent, but now guarded on `MovingSize`). Flight still reaches 3D;
production rebuild + stress **8/8** into sustained 120-frame flight, no regression. The **only** remaining
flight-path report is the lifetime UAF (`mobileitem … T_nationality`, `worldinc.h:715`) → Sprint 38.
Both fixes are `#if defined(MA_LINUX)` (identity orientation / zero velocity for static items — faithful:
a base `item` genuinely has no heading/velocity, so the Windows path read garbage and rotated by it).

**S38 post-fix re-validation — ★ FLIGHT PATH ASan-CLEAN.** With the lifetime UAF fixed, an instrumented
Hot Shot flight now produces **zero ASan reports**: verified across **5 flights** (4× 250-frame + 1× 600-frame,
`/tmp/wmig-asan.log.*` empty each run) after the identical pre-fix recipe reliably surfaced the UAF. Production
rebuild + `stress_launch 8/8`. The S15→S38 arc has eliminated **every** flight-path heap error the oracle
surfaced — per-frame corruptors (S15/S16), the mid-frequency set (S17), the base-item type-confusion pair
(S37), and the lifetime UAF (S38). The only known-remaining item is the **deliberately-benign** `FixLbmImageMap`
RLE over-read (now bounded by the adopted BoB `LBM_INBOUNDS` guard; BoB left it benign) and content-dependent
singletons (`Math::rnd`, `InitROL`) that no longer reproduce. **The S18 sub-epic is closed.**

### Deferred family — base-item type-confusion + lifetime (S18 sub-epic) — ✅ CLOSED (S37/S38)
The remaining flight-path reports are **not** the `new`/`delete` form-mismatch class — they're a
distinct family that needs the engine's **`item` type / object-lifetime model**, not a one-line form
fix, and they touch weapon/AI logic (gameplay-regression risk), so they're deferred to a focused
sprint rather than force-fixed at S17's close. All are **reads** (no heap-corrupting writes).
- **Base-item type-confusion** (`LauncherToWorld` 3dcom.cpp:13436 `tmpitm->hdg/pitch/roll`;
  `InitROL` Rchatter.cpp:1671 `target->vel`): the object is a **base `item`** (32 B, `Status.size==
  ItemSize` — e.g. an AAA ground site `new item` in `Persons3::make_itemS:1819`), but the code reads
  `hdg`/`pitch`/`roll`/`vel`, which live only in derived classes (`RotatedSize`/`MovingSize`+) → reads
  past the 32-byte allocation. The engine **has the discriminator** (`Status.size`, with assert-helpers
  at `WORLDINC.H:415–464`). Faithful fix: gate the mobile-field read on `Status.size >= RotatedSize`
  (use identity orientation / zero velocity for static items). Validate AAA muzzle/launcher position
  and tracer origin against Wine before shipping.
- **Lifetime UAF** (`mobileitem … T_nationality` worldinc.h:715): AI `ArtInt::PersonalThreat` reads a
  freed `mobileitem`'s nationality during `BoxCol::DoCollision`. Needs reference-nulling when an item
  leaves the world, not a form fix. No BoB fix.
- **Benign RLE over-read** (`FixLbmImageMap`): the ILBM PackBits decoder reads its compressed source one
  past the buffer; bounding it risks truncating legitimate tile decode (visual corruption). BoB hit the
  same and **deliberately left it** as a benign read. Lowest priority.

### Notes
- **Cross-port leverage:** `ManageHighLandTextures`/`FixLbmImageMap`/`SetPilotedAcAnim` (landscape-tile
  `new[]`/`delete`), `Reg3dConv` (index bound), and `mobileitem::operator delete` (delete-expression
  idiom) all have **proven BoB fixes** — mine `~/bob` PORT.md (R1.3a–e, R3.9) for the exact patches.
- **Priority order for Sprint 16:** the high-frequency corruptors first (dodigitdial 836×,
  mobileitem operator delete 836×, ManageHighLandTextures 129×), then the singletons.
