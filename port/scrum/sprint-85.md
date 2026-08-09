# Sprint 85 — "Say which one" — ✅ CLOSED 2026-08-08 — ⭐ the Directives dialog opens; headless recipes are now unambiguous

**Planned 2026-08-08 (PO pre-approved ceremonies). Autonomous. Committed ~8 pts.**
**Sprint Goal:** fix the id ambiguity S84 found, and use it to finish S84's other half — driving the
real Directives button.

| Story | Pts | Result |
|---|---|---|
| S85-1 `#ID` parent qualifier + loud ambiguity | 3 | ✅ |
| S85-2 verify Directives opens via a real click | 3 | ✅ opens fully populated |
| S85-3 gates + cross-port | 2 | ✅ |

## Execution log

### S85-1 — addressing a control unambiguously — DONE
S84 found that `RESOURCE.H` defines **five** symbols as 2074 (`IDC_DIRECTIVES`, `IDC_AUTHORISE4`,
`IDC_FILTER_RED_TROOP`, `IDS_PILOTNAMES_74`, `IDC_DEVDESC`), so `#2074` resolved to whichever hosted
control came first in map order and fired `Clicked` at a class with no handler for it. **A silent
no-op that reads exactly like "the feature is broken"** — the same failure shape as the scaffolding
trap in §8-MA82, one level down.

Two changes, both small:
- **`f,#ID@Class[:COL]`** in `BOB_CLICKSEQ` — `ma_ole_control_point_p()` filters candidates by the
  host's RTTI name (substring match, so recipes say `CMainToolbar`, not the mangled
  `12CMainToolbar`). The old unqualified form still works.
- **Ambiguity is now LOUD, not silent.** An unqualified `#ID` with more than one visible host prints
  a warning listing every candidate with its host class and rect. Printed unconditionally, not under
  `MA_TRACE_CLICK` — the whole failure mode is that nobody was looking.

### S85-2 — the Directives dialog — DONE
`#2074@CMainToolbar` resolves to the main toolbar's **48×48** button at (286,52) — versus the
filters toolbar's 24×24 twin at (268,50) that the unqualified form had been finding. The click fires
and **the Directives dialog opens fully populated**: title bar with `?`/`✓`/`✕`, the three tickboxes
(Auto Generate / Auto Display / Alpha Strikes), and the category table — Air Superiority, Choke,
Supply, Airfields, Rail, Road, Army, Resting — with live Strike/Fighters/Targets/Missions values
(Choke: 32 strike, 20 targets, 6 missions). Artifact: `port/ref/native/oob_directives.png`.

That completes S84's un-defer: **both** previously-deferred OOB dialogs (Intelligence and Directives)
now open on a real click, with no crash and no `SysError`. It also exercises, through the UI, the
five `DirControl::AddMission` shadowed-hoist fixes that S84 could previously only reach via ASan's
`camp-nextday` mode.

## Gates — all under `gl-lock`
- **2D parity: 5/5 byte-identical.**  **Stress: 20/20 PASS.**
- **ASan `asan_all.sh 2 80`: PASS — 0 reports, all 4 paths reached 2/2.**
- The diff is confined to the headless-recipe resolver and its parser, so nothing on a render path
  changed — the byte-identical sweep confirms it.

## Cross-port
Recorded in **MA note 34**: the qualifier form, and the general rule — *if a headless drive "does
nothing", first prove it addressed the control you meant*. BoB's `BOB_AUTOCLICK` has the same `#ID`
form and their `RESOURCE.H` reuses ids the same way.

## Result
Both dialogs deferred since S52 are open and populated on genuine clicks. The recipe language can now
name a control unambiguously, and an ambiguous one complains instead of guessing.
