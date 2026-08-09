# Sprint 81 — "Persist the campaign" (G2) — ✅ CLOSED 2026-08-08 — ⭐ campaign state persists under the *canonical* save name; `campaign_map` restored as a parity oracle

**Planned 2026-08-08 (PO pre-approved ceremonies). Autonomous. Committed ~8 pts.**
**Sprint Goal:** close G2's remaining item 2 — campaign state persistence — starting from S80's
named mechanism (the autosave writing `SaveGame/Auto Save.sa`, one character short).

| Story | Pts | Result |
|---|---|---|
| S81-1 root-cause the truncated filename | 3 | ✅ measured, not reasoned |
| S81-2 fix + prove cross-process round-trip | 3 | ✅ |
| S81-3 re-validate `campaign_map` as a parity oracle | 2 | ✅ restored to the gate at 0 px |

## Execution log

### S81-1 — root cause — DONE (instrumented, per BoB S143's lesson)
BoB's inbound §8u addendum ("when you catch yourself revising a mechanism a second time, stop
reasoning and instrument") landed just before planning, so the save path was **traced end-to-end**
(`MA_TRACE_SAVE`, gated) rather than argued from the code:

```
[save] fakefile(dir=52, 'Auto Save.sav' len=13) -> stored 'Auto Save.sav'
[save] lessfail(FAKE 0x00003408): wanted 'Auto Save.sav' (len 13)
                                -> got 'C:\rowan\mig\savegame\Auto Save.sa' (len 34)
[save] SaveGame('Auto Save.sav' len=13) -> buffer '...\Auto Save.sa' open=1
```

**Root cause.** `fileman::namenumberedfilelessfail` has **no "fake long file name" branch** — the
hard `fileman::namenumberedfile` does (return the name `fakefile()` stashed at
`namedirdir+fakefileoffset`). Without it, it always falls through to the DIR.DIR path, which lifts a
fixed **12-byte** 8.3 entry and NUL-terminates at byte 12. Under `MA_LINUX` the port routes the
buffered `FileMan::namenumberedfile(f, buf)` through the *lessfail* variant (for its graceful
unregistered-directory behaviour) — **so the save path used the one variant missing the branch.**
Every other name in the boot path is ≤ 11 chars and survived; `"Auto Save.sav"` is 13. The trace
shows the cut at exactly 12 for that one name and no other.

### S81-2 — fix + proof — DONE
Fix: give `namenumberedfilelessfail` the same fake-long-name branch (`MA_NO_LONGNAME` reverts).

**Blast radius, measured — not assumed.** This is a shared engine primitive, so every fake-file name
resolution in a full campaign boot was captured before and after and diffed. **Exactly one string
changes:**
```
- C:\rowan\mig\savegame\Auto Save.sa
+ C:\rowan\mig\savegame\Auto Save.sav
```
(`dcomms.dat`, `dreplay.dat`, `rbackup.dat`, `replay.dat`, `tblock.dat`, `*.sav` — all byte-identical.)

**Cross-process round-trip proven:**
- **Run A** (`MA_CAMP_LOOP=2`, fly two missions): starts at the pristine **6/25/50**, autosaves at
  each frag, advances `6/25/50 → 7/3/50 → 7/8/50`. Last autosave holds **7/3/50**.
- **Run B** (fresh process, single frag): comes up at **7/3/50** — the state run A left, loaded from
  the canonical `Auto Save.sav`.

**Honest framing — persistence was never *broken*.** The port saved *and* loaded under the same
truncated name, so the round trip was self-consistent and the campaign really did carry across runs
(that is why S80 saw the map date drift). What was broken is that it happened under a name nothing
outside the port looks at — the Windows/Wine build, the player's save list, and the canonical
`Auto Save.sav` (untouched since 2026-07-19) all saw nothing. Correctness and interop, not function.

### S81-3 — `campaign_map` restored as an oracle — DONE
S80 *excluded* it from the byte-identical gate ("renders mutable save state"). That is now reversed:
`port/parity_2d.sh` pins a committed reference save (`port/ref/save/campaign_pristine.sav`) around
the capture and restores the player's own save afterwards. Result: **0 px vs the committed
reference.** The reference was never wrong — the *state* had drifted, through the truncated name.
The gate is back to **5 screens**.

### Adopted from BoB — name the constant, don't copy the number
The convention's two magic numbers (`128`, `8`) were written out at **four** sites in MA (`fakefile`
stores the name; three resolvers re-derive the address) — which is precisely how two of them drifted
apart. BoB's file names them, so MA adopted the **naming**: `FILEMAN.H` now has
`enum {filenameindex=70, fakefileoffset=128, fakefileindex=8}` and all 13 + 4 sites use it.
**BoB's values are 800/50** (different buffer layout) and were deliberately *not* copied.

## Gates — all under `gl-lock` (siblings active throughout)
- **Build:** clean (the `FILEMAN.H` change rebuilt 207 TUs), 0 undefined symbols.
- **2D parity: 5/5 byte-identical** — `title` / `prefs_3d` / `prefs_others` / `quickmission` /
  **`campaign_map`** (restored this sprint).
- **Stress `stress_launch.sh`: 20/20 PASS.**
- **ASan `asan_all.sh 2 80`: PASS — 0 reports across all 4 paths**, each reached 2/2
  (flight + campaign map/fly/nextday; the campaign modes exercise the changed name resolver).
- *Note:* parity+stress were first run pre-refactor; the constant rename rebuilt 207 TUs, so the
  **whole set was re-run on the final binary** — those are the results recorded.

## Cross-port
- Shared lessons doc re-synced (BoB added §8w/§8x while this sprint ran); **§8y appended** —
  and it was renumbered from §8v→§8y under BoB's brand-new §8x collision protocol, which is
  working as designed. `tools/check_notes_sync.sh` ✓.
- **MA note 30 sent**, leading with a **correction**: note 29 §4 asked BoB to check their `fileman`
  for this bug. Checked their tree first this time — **BoB already has the branch, N/A for them.**
  A speculative "check yours" spends someone else's sprint; measurement precedes the ask.

## Result
G2's state-persistence item is **closed**: the campaign round-trips across processes under the
canonical `Auto Save.sav`. G2's three named items (flyable loop S80, persistence S81, edge/polish)
are down to edge/polish. A parity oracle that S80 retired is back in service, and the bug class —
*a self-consistent wrong value produces no symptom until something outside the system looks* — is
banked as §8y.
