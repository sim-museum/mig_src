# Sprint 15 — Stand up the ASan heap-bug oracle (+ first fixes)

**Goal:** stand up an AddressSanitizer build of MiG — BoB's single most productive hardening tool
on this same engine, which MiG had never run — and start fixing what it finds.
**Outcome: DONE, and immediately productive.** The oracle built, boots clean (after 3 fixes),
and once a 4th fix unblocked menu navigation under instrumentation it reached **3D flight**, where
it surfaced a goldmine of real heap bugs — the dominant one firing **16,576×/flight**. Six bugs
fixed + verified this sprint; the rest catalogued as the Sprint 16 backlog. No production regression.

## Delivered

### 1. ASan build infrastructure (reusable)
- **`port/rebuild.sh ASAN=1`** — a parameterised diagnostic build: separate tree
  (`port/build-asan/`), links `/tmp/wmig-asan`, so the production `wmig` is never touched. Adds
  `-fsanitize=address -fsanitize-recover=address -g -fno-omit-frame-pointer` (recover+`halt_on_error=0`
  ⇒ one run reports **every** invalid access and continues). 32-bit ASan runtime confirmed working.
- **`port/asan.sh`** — runner with the right `ASAN_OPTIONS`
  (`halt_on_error=0:detect_odr_violation=0:detect_leaks=0` — ODR/leak noise off for the unity-twin
  + `--allow-multiple-definition` build). `build` / `run [args]` subcommands.

### 2. Boot path — 3 real bugs fixed + verified CLEAN
- `Fileman.cpp:454/586` (`translatedirlist`/`retranslatedirlist`): dir-list scan read 1 byte past
  the heap buffer — `while(*datascan<'0' && datalength>0)` evaluates `*datascan` before the length
  guard. Reordered so `&&` short-circuits at end-of-buffer. (×2 sites.)
- `FULLPANE.CPP:2033` (`LaunchScreen`): first call indexes `resolutions[m_currentres]` with
  `m_currentres==-1` (ctor value, never set before use) → reads 60 B before the `FullScreen` global.
  Resolve `m_currentres` first, the engine's own idiom (used at lines ~2113/3485).
- `RLISTBXC.CPP` ctor: `m_FontNum2`/`m_vertSeperation`/`m_horzSeperation`/`m_FontNum` were set only
  by `DoPropExchange` (PX_Long defaults). Read uninitialised at first layout (`ResizeToFit`/
  `GetListHeight` use `abs(m_FontNum2)` as a global-font index + add `m_vertSeperation` to tmHeight)
  → garbage listbox size → the **title menu drew at width<0 and vanished under ASan** (clicks then
  missed it; production got lucky garbage). Init to the documented defaults in the ctor.
  **This 4th fix unblocked menu navigation under instrumentation** → the oracle could reach flight.

### 3. 3D flight path — 2 dominant `new[]`/`delete` mismatches fixed + verified
The oracle on a ~30-frame F-86 flight surfaced 18 distinct heap-bug sites (`port/scrum/asan-findings.md`,
raw `port/reference/asan-flight-report.log`). Fixed the two clear `new[]`/scalar-`delete` mismatches:
- `3dcom.cpp:10953` `DrawSubShape`: `subco = new DoPointStruc[64]` freed scalar → `delete[]`.
  **Fired 16,576× per flight** — the dominant per-frame heap corruptor.
- `3dcom.cpp:18593` `SetPilotedAcAnim`: `new UByte[]` freed as scalar `delete (AircraftAnimData*)`
  → `delete[] ((UByte*)oldanim)`. (Exact BoB R1.3a/R3.9 family.)

**Re-validated:** `DrawSubShape` 16576→**0**, `SetPilotedAcAnim` 1→**0**; flight still reaches 3D.

### 4. Wine gold-standard reference screenshots catalogued (PO request, mid-sprint)
Reviewed 14 reference screenshots of MiG under Wine and preserved them in-repo
(`port/reference/wine-gold/`, the USB source is transient) with a catalogue + per-screen fidelity
notes (`README.md`). Covers title, all 7 Preferences tabs, Quick Mission, **in-cockpit 3D flight**,
external view, debrief, campaign select, and the **operational map of Korea** (full-colour — the
exact A/B target for the known greyish-map-tile fidelity gap). Native title menu A/B'd clean vs gold.

## Validation
- Boot under ASan: **clean** (0 reports) after the 3 boot fixes.
- 3D flight under ASan: reaches the rasterizer; dominant corruptors gone after the 2 fixes.
- **Production stress: 4/4** reached & sustained 100 3D frames — across all four rebuilds this
  sprint (boot fixes, listbox fix, 3D fixes). No regression. Preferences/menu render unchanged
  (native title menu matches the gold reference).

## Backlog handed to Sprint 16 (`port/scrum/asan-findings.md`)
High-frequency first: `dodigitdial` (≈7.5k×), `mobileitem::operator delete` (≈7.5k×, BoB R1.3d/e
delete-expression idiom), `ManageHighLandTextures` (≈140×, **BoB R3.9 exact match**). Then the
singletons: `LauncherToWorld`, `DoCloudLayer` (stack OOB), `FixLbmImageMap`, `Reg3dConv`
(**BoB R1.3b**), `Rchatter::InitROL`, `mobileitem` nationality UAF. Several have proven BoB patches
to mine from `~/bob` PORT.md.

## Next (Sprint 16)
Continue the ASan flight-path grind (above), or pivot to the **operational-map colour fidelity**
now that the gold reference (`14-operational-map.png`) is in-repo. Both are now well-scoped.
