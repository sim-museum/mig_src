# Sprint 56 — "Parity oracle stood up" (autonomous, headless DoD)

**Goal:** close the inherited WIP (IMAGEMAP.CPP A/B), land the EPIC I gold-shot inventory
(I1), and get a first native Player Log capture for the new I4 story.
**Committed:** ~8 pts (I1 3, WIP-judgment 2, I4 capture spike 3). Prior session agent died
mid-sprint leaving WIP; this session re-ran the experiment and closed.

| Item | Pts | Status | Evidence |
|---|---|---|---|
| WIP judgment: IMAGEMAP.CPP LBM A/B instrumentation | 2 | ✅ KEPT (proven) | Controlled A/B, 2 runs each way, same FLY view: `MA_TRACE_LBM` traces deterministic per mode; the ONLY differing decode is a 2×2 imagemap whose row data ends at the buffer edge — unbounded reads 24655/24636 bytes (19 B past the heap block = the ASan overflow the committed bounds fix targets). Default path bit-identical (env-gated). Verdict: bounds fix valid; `MA_LBM_NOBOUND`/`MA_TRACE_LBM` kept as the standing A/B toggle (julia-racer QA note §5). Evidence table in `screen-parity.md` |
| I1 gold-shot inventory + parity table | 3 | ✅ | `port/scrum/screen-parity.md`: all 14 gold shots identified (+ #15 I4 gold), native captures committed (`port/ref/native/*.png`, 13 screens), per-shot verdicts: 5 CLOSE, 6 PARTIAL, 2 not-yet-captured (title, debrief). Oracle provenance flagged: gold = **BDG 0.85F patched build** (resource deltas ≠ render bugs — BoB S123 lesson) |
| I4 native Player Log capture (spike) | 3 | ✅ capture (story stays open) | New gated hook `MA_OOB_PLAYERLOG=1` (`MIG.CPP` map idle → `CMainToolbar::OpenPlayerlog()` after 40 idles) makes the S53/S54 OOB path headless-scriptable. `port/ref/native/map_playerlog.png`: Career-tab pilot photo renders over the map. Named gaps for I4 proper: placement (top-left vs centred), no frame/title/tab bar (CRTabs), stats table + Name edit not drawn |
| Sprint close: boards, statuses, inbound notes committed | — | ✅ | This board; scrum.md EPIC I + burndown; inbound `port/CROSS-PORT-FROM-BOB-2026-07-25.md` + `port/QA_METHOD_GOLD_PARITY_from-julia-racer.md`; `port/BOB_PORT_LESSONS.md` §8e |

## Regression state
- 4 A/B FLY launches + 2 CAMP map runs this session: all reached render, no crash (launch
  gate de-facto 6/6; full `asan_all.sh` unchanged since S55 — no engine-behaviour change
  landed: IMAGEMAP change is env-gated, MIG.CPP hook is env-gated).

## Carry-over / next (S57 candidates)
- I4 proper (8): dialog frame + CRTabs tab bar + content-dialog controls + centring.
- I2 keystones from BoB S123 (cross-port note 13): (dlgId,ctrlId)-scoped label lookup
  (fixes #7/#8 missing labels), ShowWindow tracking, ListX/ListY anchor check.
- Recapture #10/#11 at gold-matching scene state; capture #1 title + #12 debrief.
- PO question: parity target = BDG 0.85F resources or 2000 source resources?
