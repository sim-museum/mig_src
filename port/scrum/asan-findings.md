# ASan findings — heap-bug oracle backlog

The `ASAN=1` build (`port/asan.sh`, Sprint 15) is the heap-corruption oracle — the single most
productive hardening tool on BoB (the sibling Rowan-engine port). This catalogues what it has
surfaced. Each line is a real root-cause UB fix (usually 1–2 lines), to be fixed + re-validated
(count → 0) one at a time, logged here with evidence.

Run: `port/asan.sh build` then drive a path (boot / 3D flight / campaign map). Reports →
`/tmp/wmig-asan.log.<pid>` (also stderr). Raw flight report preserved at
`port/reference/asan-flight-report.log`.

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
| 836 | new-delete-type-mismatch | `worldinc.h:708` `mobileitem::operator delete` | custom `operator delete` re-runs dtor / wrong type — BoB R1.3d/e family (delete-expression idiom → `::operator delete(obj)`) | ☐ backlog |
| 836 | alloc-dealloc-mismatch | `3dcom.cpp:10386` `shape::dodigitdial` | same `new[]`/scalar-`delete` family as DrawSubShape | ☐ backlog |
| 129 | alloc-dealloc-mismatch | `Landscap.cpp:730` `LandScape::ManageHighLandTextures` | landscape-tile buffer `new[]`/`delete` mismatch — **BoB R3.9 exact match** (ground-impact crash) | ☐ backlog (high) |
| 3 | heap-buffer-overflow | `3dcom.cpp:13436` `shape::LauncherToWorld` | OOB read | ☐ backlog |
| 7 | stack-buffer-overflow | `Landscap.cpp:7257–7289` `DoCloudLayer` | cloud-layer local array OOB (several lines) | ☐ backlog |
| 1 | stack-buffer-overflow | `Landscap.cpp:2329` `PerspectivePoly` | local array OOB | ☐ backlog |
| 1 | heap-use-after-free | `worldinc.h:715` `mobileitem … T_nationality` | UAF reading nationality after free | ☐ backlog |
| 1 | heap-buffer-overflow | `KEYSTUB.CPP:290` `Reg3dConv` | scancode/shiftstate index + terminator — **BoB R1.3b exact match** (proven fix: bound the indices) | ☐ backlog (easy) |
| 3 | heap-buffer-overflow | `lbmcpp.h:206/215/250` `FixLbmImageMap` | LBM tile image-map decode over-read (body/palette/alpha) — BoB R3.9 neighbour | ☐ backlog |
| 1 | global-buffer-overflow | `Math.cpp:1722` `MathLib::rnd()` | RNG table index past end (intermittent) | ☐ backlog |
| 1 | heap-buffer-overflow | `Rchatter.cpp:1671` `RADIOMESSAGE::InitROL` | radio-chatter ROL parse over-read (surfaced after S15 fixes) | ☐ backlog |
| 1 | alloc-dealloc-mismatch | `3dcom.cpp:18593` `shape::SetPilotedAcAnim` | `new UByte[]` freed scalar → `delete[] (UByte*)` — **BoB R1.3a/R3.9 match** | ✅ **fixed (S15)** |

**S15 post-fix re-validation (instrumented flight):** `DrawSubShape` 16576→**0**, `SetPilotedAcAnim`
1→**0** (and `Math::rnd` no longer fires); flight still reaches 3D; production stress 4/4. The two
dominant per-frame corruptors are gone. Remaining counts vary run-to-run with flight duration/content.

### Notes
- **Cross-port leverage:** `ManageHighLandTextures`/`FixLbmImageMap`/`SetPilotedAcAnim` (landscape-tile
  `new[]`/`delete`), `Reg3dConv` (index bound), and `mobileitem::operator delete` (delete-expression
  idiom) all have **proven BoB fixes** — mine `~/bob` PORT.md (R1.3a–e, R3.9) for the exact patches.
- **Priority order for Sprint 16:** the high-frequency corruptors first (dodigitdial 836×,
  mobileitem operator delete 836×, ManageHighLandTextures 129×), then the singletons.
