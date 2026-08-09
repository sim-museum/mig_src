# Sprint 84 — "Open it once" — ✅ CLOSED 2026-08-08 — ⭐ the Intelligence dialog opens, populated, after ~30 sprints deferred

**Planned 2026-08-08 (PO pre-approved ceremonies). Autonomous. Committed ~8 pts.**
**Sprint Goal:** clear the `0x6a78` double-open S83 named, and un-defer the two OOB dialogs.

| Story | Pts | Result |
|---|---|---|
| S84-1 root-cause the `0x6a78` double-open | 3 | ✅ traced, not guessed |
| S84-2 fix it + un-defer Authorise/Directives | 3 | ✅ Authorise verified populated; defer removed |
| S84-3 gates + cross-port | 2 | ✅ |

## Execution log

### S84-1 — root cause — DONE (backtrace, not argument)
`0x6a78` is **`FIL_ICON_MISSIONRESULTS`** — a toolbar *icon*, so this was never S79's preload case.
Added a `MA_TRACE_FILEOPEN` backtrace at the fatal branch in `fileblocklink::makelink` (the previous
two instances of this bug were each argued about before being traced). It printed:

```
ma_oob_paint_tree_rec → ma_oob_render_node → ma_ole_draw_toolbar → ma_button_draw
  → CRButtonCtrl::OnDraw → DrawBitmap → SendMessage(WM_GETFILE)
  → RDialog::OnGetFile → new fileblock → makelink   ← fatal duplicate open
```

**Mechanism.** `makelink` serves reuse only from the **freed** cache; finding the FileNum in
`openfiles` means somebody still holds it, which the engine treats as fatal. `RDialog::OnGetFile`
holds its block in a **per-dialog** `m_pfileblock`. The map toolbar's Authorise button and the
Authorise dialog's own button use the *same art*, so whichever painted second opened a block the
first was still holding. **This was latent until S82 made the OOB dialogs paint every idle.**

**Fix:** `fileman::MA_GetOpenFileData()` (sibling of S79's `MA_IsFileOpen`) returns the already-open
block's data, and `OnGetFile` serves that instead of duplicating the open — deliberately *not*
storing it in `m_pfileblock`, since we don't own it and must not release someone else's block.
`MA_NO_SHARED_FILEBLOCK=1` reverts.

### S84-2 — and four more shadowed hoists — DONE
Removing the defer immediately produced a **different** SEGV: `CSupply::AddChokeMission` — the same
half-applied for-scope hoist S83 fixed in `AddSupplyMission`. **S83's sweep had missed it**, because
the regex only matched `int|long|short|unsigned` and these siblings declare **`char i`**. Re-swept
type-agnostically:
- `CSupply::AddChokeMission`, `AddTrafficMission`, `AddAirfieldsMission` — 3 more.
- `DirControl::AddMission` and siblings in `COMIT_E.CPP` — **5 more**. (So the original stale note
  blaming `CComit_e` had the right class for the *other half* of the bug all along.)

**The hoisted type must match the original loop variable.** These declare `char i`, and
`char i = MAX_TARGETS-1` is **299 truncated to 43** — a quirk of the shipped game. Kept deliberately:
the gold shots are the oracle, and widening to `int` would silently change how many table entries
shift. The bug fixed is only the *shadowing*.

**Result: the Intelligence (Authorise) dialog opens fully populated** — five tabs
(Supply/Choke/Traffic/Airfields/Army), the "Sort by: MSR: East" combo, and a real objective table
(Chosin, Pungsan Supply Dispersal, Kapsan, Chongjin Marshalling Yd. …) with MSR/Activity/Capacity.
Capture: `port/ref/native/oob_intelligence.png`. The defer is removed (`MA_OOB_DEFER_DIALOGS=1`
restores it if either ever regresses).

### Bonus fix: `#ID` recipes were resolving toolbar buttons ~50px off
Hand-computing a toolbar button's screen position failed twice this sprint — the exact trap S62/S63
banned for menus. Cause: a toolbar-hosted control's position is the offset passed in at **paint**
time (the map idle draws toolbar1 at 4,26 and toolbar2 at 4,52), but the resolver added the parent
`CRToolBar`'s `m_maX/m_maY`, which are 0. Fix: `Hosted` now records `drawOx/drawOy` — **what paint
actually did** — and the resolver uses it (the same principle as S82's click walk mirroring the
paint walk). `#2074` now resolves correctly with no hand arithmetic.

### ⚠ Finding: numeric control ids are AMBIGUOUS in recipes
With the resolver fixed, `#2074` still opened nothing — and that is correct. `RESOURCE.H` defines
**five** symbols as 2074: `IDS_PILOTNAMES_74`, `IDC_AUTHORISE4`, `IDC_DIRECTIVES`,
`IDC_FILTER_RED_TROOP`, `IDC_DEVDESC`. The resolver found the *filters-toolbar* twin
(`IDC_FILTER_RED_TROOP`, `CMapFilters`), fired `Clicked` on it, and `CMapFilters` registers no
handler for that id — a no-op, not a crash. **So `#ID` needs a parent qualifier to be unambiguous**;
booked for S85. The Directives dialog's own code path (`DirControl::AddMission` ×5) is exercised by
the ASan `camp-nextday` mode, which passes.

## Gates — all under `gl-lock`
- **2D parity: 5/5 byte-identical.**  **Stress: 20/20 PASS.**
- **ASan `asan_all.sh 2 80`: PASS — 0 reports, all 4 paths reached 2/2.** The `camp-nextday` mode
  exercises `DirControl::AddMission` (five of this sprint's eight shadowed-hoist fixes), so the
  Directives code path is covered even though its toolbar button could not be addressed
  unambiguously (see the id-collision finding).

## Result
A crash chain that had two OOB dialogs deferred since S52 is fully cleared: one shadowed loop
variable (×9 across two classes) and one per-dialog fileblock held across a shared icon. The
Intelligence dialog now opens with real campaign data.
