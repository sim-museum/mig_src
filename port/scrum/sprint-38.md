# Sprint 38 — "ASan S18: lifetime UAF" ★ flight path ASan-clean (autonomous, headless DoD)

**Context:** Autonomous (PO unavailable), headless DoD (the ASan oracle is the acceptance test). Closes the
second half of the deferred S18 sub-epic — the last residual flight-path report.

**Sprint Goal:** Eliminate the `mobileitem … T_nationality` heap-use-after-free; reach an ASan-clean flight.

## Committed (5 pts)
- **S18b** — `PersonalThreat` nationality use-after-free.

## Root cause (subtler than the deferred note implied)
The report reads as a *lifetime* UAF but is really a **use-before-validate**:

```
bool ArtInt::PersonalThreat(AirStrucPtr trg, AirStrucPtr agg, bool process) {
    if (trg->nationality==agg->nationality)   // <-- Msgai.cpp:1794, derefs BEFORE the guard
        return(false);
    if (trg && agg && trg->Status.size==AIRSTRUCSIZE && agg->Status.size==AIRSTRUCSIZE) {
        ... // all meaningful work is inside this guard
    }
    return(false);                             // <-- guard-false path already returns false
}
```

- Call path: `TransObj::MobileBullet` → `BoxCol::NineSectorCol → … → DoCollision:1885` →
  `PersonalThreat((AirStrucPtr)trgTest, (AirStrucPtr)hitter->Launcher)`.
- `agg` = the bullet's **`Launcher`** (the aircraft that fired it). If that aircraft left the world while
  its round is still in flight, `Launcher` dangles → `agg->nationality` (`worldinc.h:715`,
  `T_nationality::operator Nationality()`) is a **heap-use-after-free read** of a freed 32-byte region.
- The engine already carries the correct validity guard (`trg && agg && Status.size==AIRSTRUCSIZE`) — line
  1794 simply ran one statement ahead of it.

## Fix (`#if defined(MA_LINUX)`, byte-preserving edit on ISO-8859 source)
Insert an early validity return **before** the nationality deref:
```
if (!(trg && agg && trg->Status.size==AIRSTRUCSIZE && agg->Status.size==AIRSTRUCSIZE))
    return(false);
```
**Behaviour-preserving:** for a valid AirStruc pairing the guard passes and control proceeds exactly as
before; for any invalid/stale pairing the function already returns false at its fall-through (line 1902), so
the result is unchanged — only the UAF deref is avoided. No AI/gameplay logic altered for real aircraft.

## Validation (headless DoD)
- ASan oracle rebuild + **5 instrumented flights** (4× 250-frame, 1× 600-frame): **0 ASan reports** every
  run — after the identical pre-fix recipe reliably surfaced the UAF (clean A/B).
- Production rebuild + `port/stress_launch.sh 8 120 22`: **8/8**, no regression.
- Compiles clean into `_AI`; `wmig` links (8.7 MB), 0 undefined symbols.

## Increment — ★ milestone
**The entire instrumented 3D flight path is now ASan-clean.** The S15→S38 arc eliminated every heap error
the oracle surfaced: per-frame corruptors (S15/16), the mid-frequency set (S17), the base-item
type-confusion pair (S37), and this lifetime UAF (S38). The S18 sub-epic is closed; only the
deliberately-benign, already-bounded `FixLbmImageMap` RLE over-read remains (BoB's call, guarded).

**Next (Sprint 39):** promote the ASan-clean result to a standing regression gate + cross-port completeness
sweep; then pick the next autonomous, headless-DoD backlog item.
