# Sprint 22 — bogie spawn tuning: the Turkey Shoot spawn is CORRECT (measured); drift is post-spawn dynamics

**Goal:** the user reports the Turkey Shoot bogie is hard to find / drifts down. Reference: in Wine
the MiG spawns dead-center in the F-86 gunsight, player on its six. Tune our spawn to match.

**PO:** standing pre-approval. Run autonomously.

## Outcome: the spawn already MATCHES Wine. Measured it directly — no spawn bug.

Added `MA_QUICKMISS=<n>` (launch any quick mission via the Hot Shot menu path; 2 = Turkey Shoot) and
`MA_TRACE_BOGIE` (logs every aircraft's world position vs the player viewpoint each frame, in
`3DCODE.CPP do_objects`). Flew Turkey Shoot with **no flight input** and logged player + bogie world
coords over ~28 s.

### At spawn (frame 0) — exactly as the Wine screenshot shows
| | world X | world Y (alt) | world Z |
|---|---|---|---|
| PLAYER (F-86) | 41252497 | **152400** | 89295189 |
| BOGIE (MiG-15) | 41252497 | **152400** | 89345185 |

- Same X, **same Y = 152400 = FT_5000 exactly** (co-altitude), bogie **+49996 units ≈ 1640 ft ahead**
  in Z. → bogie dead-centered in the gunsight, player on its six. **Spawn geometry is correct.**

### Post-spawn (hands-off, ~28 s) — the two separate vertically, but lawfully
- **BOGIE:** X stays constant (~41252497), flies straight along +Z with a **steady, gentle descent**
  (152400 → 150264, ≈ −70 ft total, ~150 fpm). Turkey Shoot's targets are **ground nodes**
  (`qmiss.cpp`: `IDS_CIVILIAN`, `UID_NODE_sinuiju` …) → the MiG is transiting toward its objective.
  Not a dive-away. **Range CLOSES the whole time** (66806 → 46676) — the player is catching it.
- **PLAYER:** hands-off ballistic **climb** (152400 → 158473, +199 ft), climb rate accelerating while
  forward speed slightly drops = nose-up at full thrust / 504 kts (excess-thrust climb). A real pilot
  noses down to track; with no input it drifts up.

### Conclusion
The bogie spawns **exactly centered and co-altitude (matches Wine)**. The "drifts down / hard to see"
is **post-spawn flight dynamics**, dominated by the player's uncommanded climb (no input in the test),
plus the MiG's gentle en-route descent toward its ground target — both lawful. The earlier "saw no
bogie" was visibility (a small object ~1640 ft out), now confirmable with F1 padlock. **No spawn
change made** — there is no spawn bug to fix, and altering the flight model / AI to flatten the
gentle drift is high-regression-risk for no clear defect (precedent: Sprint 20, the "dark sky" that
measurement disproved).

### Open thread (optional, deeper, PO-steerable)
The player's hands-off nose-up climb affects all flight (gunnery/landing feel), not just Turkey Shoot.
If desired, a focused flight-model session could check the piloted aircraft's initial pitch trim vs
Wine — but it's the core flight integrator (highest regression risk in the port) and the aircraft
flies & lands correctly today, so it's deferred unless prioritized.

### Test hooks added (gated, default off)
- `MA_QUICKMISS=<index>` (`FULLPANE.CPP` SetUpHotShot) — launch a specific quick mission via the Hot
  Shot path (e.g. `=2` Turkey Shoot) for repeatable testing.
- `MA_TRACE_BOGIE` (`3DCODE.CPP` do_objects) — per-frame aircraft world-position trace vs the player.

## Known regression noted (separate, from Sprint 21)
Mouse-wheel zoom on the campaign map resizes the window and patchworks the tiles (the port ties the
present canvas to `m_size`, which zoom grows). Map keyboard/drag pan + Esc/Fly are unaffected. Filed
for the next map-polish pass; key `+`/`-` zoom has the same root cause.
