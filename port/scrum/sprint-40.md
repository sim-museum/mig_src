# Sprint 40 — "Campaign-path ASan sweep" (autonomous, headless DoD)

**Context:** Autonomous (PO unavailable). Closes the coverage gap S39 identified: the quick-mission flight
never reaches the campaign `Package.dat` save/reload serialiser — the exact code family where BoB's fuzz
found S64/S65, and where MA's twin **S65a** fix (`f027fcc`) had only ever been reasoned about, never
ASan-exercised.

**Sprint Goal:** Drive the campaign save-load path headlessly under ASan and confirm it (and S65a) are clean.

## Delivered (8 pts)

### 1. First headless campaign-drive recipe
No scripted nav to the campaign map existed (S14 documented the loadgame internals, not click coords).
Discovered it empirically with `MA_TRACE_OLE`/`MA_TRACE_CLICK` + a frame dump:
- **Menu-row map:** the title menu listbox is `row = 1 + (y-231)/16` at x=588 → **Load Game = row 3 = (588,263)**
  (probed via `[OnSelectRlistbox] row=N`).
- **Loadgame screen** (captured to PNG): file-list "Auto Save" row at ~(40,108); "Load" menu item at ~(68,565).
- **Recipe:** `BOB_CLICKSEQ="30,588,263;65,40,108;100,68,565"` → `[DoLoadGame] CFiling::LoadGame("Auto Save.sav")
  -> 1` → `[map] render operational map`.

### 2. `MA_IGNORE_SAVE_DATE` — build-date save guard bypass (port fix + test hook)
The load initially failed: `SAVEGAME.CPP:305` `if (strcmp(date,date2)) SysErr("Savegame dates differ")`,
where `date2` is the current build's `__DATE__`. **Every rebuild voids every prior save**, even though the
port keeps the save FORMAT byte-stable (`-fpack-struct=1`, unchanged structs). Added an `#if defined(MA_LINUX)`
env-gated bypass (`&& !getenv("MA_IGNORE_SAVE_DATE")`) — default-off, so no behaviour change unless set. With
it, the Jun-25 "Auto Save" loads cleanly on today's build and renders the Korea map (format confirmed compatible).

### 3. `port/asan_campaign.sh` — standing campaign ASan gate
Drives the recipe under the ASan build over N runs; fails if any run emits an ASan report or the map never
renders. Complements `port/asan_flight.sh`.

## Validation (headless DoD)
- **`port/asan_campaign.sh 2 80` → PASS**: 2/2 rendered the strategic map, **0 ASan reports**.
- The swept path runs `CFiling::LoadGame` → `bis>>Miss_Man` → `SAVEGAME.CPP:386 Todays_Packages.LoadGame(bis)`
  → **`PackageList::LoadGame`** (MAPCODE.CPP, the **S65a** `new char[]`/`delete[]` site) → map render. A scalar
  `delete` would have fired an alloc-dealloc-mismatch here → **S65a is now ASan-validated on a live load**.
- **No regression:** `port/asan_flight.sh 2` still PASS (flight path ASan-clean after the `_MISSMAN` rebuild).
- Compiles clean; `wmig` links (8.7 MB), 0 undefined symbols.

## Increment
ASan coverage now spans **boot + 3D flight + campaign save-load/strategic-map** — all clean. The campaign
serialiser (S65a) is live-validated, and there is a **reusable headless campaign harness** for future work.

**Next:** the *in-campaign* sim (day advance / mission generation / `SaveBin` writeback) — reachable only by
progressing a loaded campaign; the next campaign-ASan target (would also exercise `PackageList::SaveBin`, BoB's
S64 site, which MA does not share but whose writeback path is still un-swept).
