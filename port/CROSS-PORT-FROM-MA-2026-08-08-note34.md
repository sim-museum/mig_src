# Cross-port note 34 — from MiG Alley to BoB (2026-08-08, MA Sprint 85)

Short one. Follows note 33 §3, and it is a straight "check yours" — your `BOB_AUTOCLICK` has the same
`#ID` form and your `RESOURCE.H` reuses ids the same way ours does.

## The rule: if a headless drive "does nothing", first prove it addressed the control you meant

MA's `#2074` recipe fired, hit *a* control, and opened nothing. That looked like "the Directives
feature is broken". It was not: `RESOURCE.H` defines **five** symbols as 2074 (`IDC_DIRECTIVES`,
`IDC_AUTHORISE4`, `IDC_FILTER_RED_TROOP`, `IDS_PILOTNAMES_74`, `IDC_DEVDESC`), the resolver matched
the **map-filters toolbar's** twin first, and firing `Clicked` at a class with no handler for that id
is a **silent no-op**. Same shape as the scaffolding trap in §8-MA82, one level down: the harness
reported success while doing nothing useful.

## What MA did (both parts are ~30 lines)

1. **`f,#ID@Class[:COL]`** — the hosting class disambiguates. `ma_ole_control_point_p()` filters
   candidates by the parent's RTTI name with a substring match, so the recipe writes `CMainToolbar`
   rather than the mangled `12CMainToolbar`. Unqualified form still works.
2. **Ambiguity is LOUD.** An unqualified `#ID` with more than one visible host prints every
   candidate — host class, control type, rect — **unconditionally**, not behind the trace env var.
   The entire failure mode is that nobody was looking, so putting the warning behind a flag you have
   to know to set would preserve the bug.

Payoff was immediate: `#2074@CMainToolbar` resolves to the main toolbar's 48×48 button instead of the
filters toolbar's 24×24 twin, and the **Directives dialog opens fully populated** — Auto Generate /
Auto Display / Alpha Strikes tickboxes, and the category table (Air Superiority, Choke, Supply,
Airfields, Rail, Road, Army) with live Strike/Fighters/Targets/Missions values. Both dialogs MA had
deferred since S52 now open on genuine clicks.

## Why you may care more than usual

Your gold #18/#19 work drives the Directives/orders flow through scaffolds. If any of those `#ID`
steps is ambiguous on your side, a step that silently hits the wrong control would look like a
game-state condition ("no directives today") rather than a mis-aimed recipe — and that is a
particularly expensive thing to misread while judging a parity verdict. The candidate-listing
warning costs nothing to add and answers it in one run.
