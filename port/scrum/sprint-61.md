# Sprint 61 — "The Player Log lands" (autonomous)

**Goal:** finish what S60 started. The Player Log's tab bar and title bar are already
created, populated, sized and drawn — they just land at the wrong place. Give the dialog
tree real screen origins so they composite where they belong, then re-verdict #15.

**Committed (~8 pts):**
| Story | Pts | Definition |
|---|---|---|
| S61-1 Dialog trees get real screen origins | 3 | `OnGetXYOffset` returns each node's true absolute origin; the `RDEmptyD` garbage origin is gone. Acceptance: **the Career / Log of Missions / Last Mission tab bar and the "PLAYER LOG" title bar are visible in `map_playerlog`**, at plausible positions, with the front-end parity captures still byte-identical |
| S61-2 Re-capture + #15 verdict flip | 2 | `map_playerlog.png` re-captured and side-by-sided against gold #15; `screen-parity.md` verdict + named deviations updated; I4 status re-stated honestly |
| S61-3 Tab selection + `?`/`✓` title buttons | 2 | Tab clicks route `ma_tabs_hit` → `SelectTab` → `WM_SELECTTAB` so a non-default tab can be shown (capture at least one); identify the `?`/`✓` buttons in the IDD 276 template |
| S61-4 Cross-port note 19 + close | 1 | MA note 19 to `bob/doc/` (the `ClientToScreen` finding is shared-engine); both lessons-doc copies md5-identical; board + burndown + parity + `RUNNING.md` + `SCRUM_STATUS.md`; gates run |

**Not pulled:** the Career content table (Sorties/Combats/Kills/Losses) — still the other
half of I4 and still a sprint of its own; RScrlBar hosting (S60 find, backlog); #12
debrief capture; cross-cutting font (#1) / chrome (#2).

**Planning notes (evidence gathered at planning, 2026-08-01):**
- Environment: session **UNLOCKED** (`LockedHint=no`, `ScreenSaver GetActive=false`), no
  stray `wmig`, build current, tree clean at `cdccb99`.
- **Root cause of S60's miss, found at planning:** `CWnd::ClientToScreen` and
  `ScreenToClient` are **complete no-ops** in the compat layer (`afxwin.h:690-693`).
  `RDialog::OnGetXYOffset` (`RDIALOG.CPP:1894`) is built entirely on them:
  ```
  GetClientRect(rect); windowrect = rect; ClientToScreen(windowrect);
  ... newparent->parent->GetClientRect(parentrect); ClientToScreen(parentrect);
  offsetx = parentrect.left - windowrect.left;      // 0 - 0 == 0, always
  ```
  So **every** dialog reports offset ~0 and the whole tree composites at the top-left —
  which is simultaneously #15's "dialog draws at top-left, not centred" deviation AND the
  reason the tab bar is invisible: it is drawn at (0,0) and the Career page's background
  art, painted later in the walk, covers it. The `artnum == artnum` parent-walk condition
  suspected at S60 close is a red herring; the arithmetic is degenerate either way.
- Second, independent defect (same story): the top `RDEmptyD` node carries a garbage
  origin — measured `m_ma=(978990,978859 -1957003x-1956942)`. `MakeParentDialog`'s
  `Place()` centring reads `m_pView->GetWindowRect(apppos)`, and the view's rect is
  suspect. Must be fixed before any parent-chain accumulation, or it poisons every child.
- **Risk, carried forward from S60 explicitly:** making `ClientToScreen` real is a global
  semantic change to a function the front-end panels also call. S60's lesson was that a
  `CDialog::Create`-wide change regressed the front end. **Prefer the scoped fix**
  (give `OnGetXYOffset` the true origin directly) unless the global one measures clean.
  Either way the 4× byte-identical parity sweep is the gate, run before commit.

## Results

*(filled in as stories land)*
