# Sprint 37 — "ASan S18: base-item type-confusion" (autonomous, headless DoD)

**Context:** Run autonomously (PO unavailable). Selected from the backlog for a **fully headless
Definition of Done**: the ASan oracle is the acceptance test — no interactive/PO play-test needed.
Top-priority EPIC A (runtime stabilization). Closes the first half of the deferred S18 sub-epic.

**Sprint Goal:** Eliminate the base-item type-confusion pair the ASan flight-path grind left deferred.

## Committed (8 pts)
- **S18a** — `LauncherToWorld` + `InitROL` base-item type-confusion (heap-buffer-overflow family).

## Root cause
Two engine sites dereference derived-class fields through a pointer that can hold a **base 32-byte
`item`** (`Status.size == ItemSize`, e.g. an AAA ground site `new item` in `Persons3::make_itemS:1819`):

| Site | Read | Field lives in | ASan |
|------|------|----------------|------|
| `3dcom.cpp:13436` `shape::LauncherToWorld` | `generate2(tmpitm->hdg, tmpitm->pitch, tmpitm->roll,…)` | `rotitem` (`RotatedSize`) | READ size 2, 0 B after a 32-byte region; ×3/flight |
| `Rchatter.cpp:1671` `RADIOMESSAGE::InitROL` | `speed = target->vel * 10000` | `MovingItem` (`MovingSize`) | same family; content-dependent |

The engine ships the discriminator (`ITEM_STATUS Status.size`, ordered enum `ItemBaseSize < WayPointSize
< ItemSize < HdgSize < HPSize < RotatedSize < MovingSize < …`; see `WORLDINC.H:226`). Windows tolerated
the over-read (garbage rotation/velocity, non-crashing); ASan flags it and it's latent UB on any host.

## Fix (both `#if defined(MA_LINUX)`, byte-preserving edits — ISO-8859 source)
- `3dcom.cpp:13436`: `if (itm->Status.size >= RotatedSize)` → read the angles; else
  `generate2(Angles(0),Angles(0),Angles(0),…)` (identity orientation).
- `Rchatter.cpp:1671`: `speed = (target->Status.size >= MovingSize) ? (target->vel * 10000) : 0;`

Faithful: a static base `item` genuinely has no heading/velocity, so identity/zero is the correct value
(the Windows path was rotating the launcher offset by garbage). No gameplay logic changed for real
mobile items (they are `>= RotatedSize`/`MovingSize`).

## Validation (headless DoD)
- ASan oracle rebuild + instrumented ~200-frame Hot Shot flight: **`LauncherToWorld` heap-buffer-overflow
  ×3 → 0**; `InitROL` did not fire (now guarded). Only remaining report = the S38 lifetime UAF.
- Production rebuild + `port/stress_launch.sh 8 120 22`: **8/8** reached & sustained 120 3D frames, no
  regression (no SEGV/FPE/ABORT).
- Compiles clean into `_3D` + `_HARD` unities; `wmig` links (8.7 MB), 0 undefined symbols.

## Increment
The 3D flight path is one heap-corruption class cleaner: static ground items (AAA sites) no longer
over-read orientation/velocity during weapon-launcher and radio-chatter processing. Demonstrable via the
ASan oracle (report count for the pair → 0) and the stress harness.

**Next:** Sprint 38 — the single residual `mobileitem … T_nationality` lifetime UAF (`worldinc.h:715`).
