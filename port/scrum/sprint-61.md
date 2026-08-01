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

**Sprint outcome: 7 of 8 pts. The sprint goal WAS met — the Player Log's tab bar renders
and the dialog is centred over the map.** The title bar and the `?`/`✓` buttons did not
land; both turned out to be blocked on the same unbuilt component, diagnosed below.

### S61-1 Dialog trees get real screen origins (3 pts) — ✅
Four distinct defects, each found by measurement, each fixed:

1. **`borderwidth` was uninitialised heap.** `MIG.CPP`'s startup reads
   `Control Panel\desktop\WindowMetrics` into a local `buff`/`type` and **never checks
   `RegQueryValueEx`'s return code**. Harmless on Windows (the keys always exist); here
   the compat `RegQueryValueEx` is a stub that returns `ERROR_FILE_NOT_FOUND` and touches
   neither output, so `if (type==REG_SZ)` tested uninitialised stack and `*(int*)buff`
   read uninitialised stack. `borderwidth` feeds `MakeParentDialog`'s sizing, so every
   top-level dialog tree got a **different garbage origin on every run** —
   `m_ma=(978990,978859)` one run, `(979004,978793)` the next. That run-to-run variance
   is what distinguished it from a logic bug. Zeroing both makes the failed query
   deterministic and `borderwidth` lands on a sane 2px.
2. **`OnGetXYOffset` was built on no-op `ClientToScreen`.** `CWnd::ClientToScreen` /
   `ScreenToClient` are empty in the compat layer, and the stock body derives its offset
   purely from them — so every subtraction was `0 - 0` and **every dialog reported
   offset ~0**. Replaced under `MA_LINUX` with the accumulated origin up the RDialog
   `parent` chain. Deliberately NOT fixed by giving `ClientToScreen` global screen
   semantics: it is called from panel code that relies on the identity behaviour, and S60
   was burned by exactly that class of global change.
3. **The `IDJ_PANEL0..9` placeholders were never registered.** `AddChildren` looks up
   `GetDlgItem(IDJ_PANEL0 + i)` to find *where* a child dialog goes; with no registration
   it fell through to the `else` branch and **stacked each child BELOW its parent**. On
   the Player Log that put the tab box at y=396 on a 400px dialog — off the bottom. These
   are plain native template controls (IDD 276 declares 1117 at px(0,27,336,370)), so
   registering a bare `CWnd` at the template rect is enough: `GetDlgItem` finds it,
   nothing is hosted against it, nothing draws. Tab host: y=396 → **y=27**.
4. **The title-height nudge double-counted.** With (3) fixed, each child already sits
   below the title bar (the placeholder rect starts at y=27), so the stock
   `offsety += titleHeight` added it a second time. Worse, it is gated on
   `top->fchild->artnum == artnum`, so it applied to the art-less tab host but NOT to the
   art-bearing tab page — shifting them 27px out of step so the page art painted over the
   tab strip. Measured: tab host y=176 vs page y=179, i.e. only the top 3px of the tab bar
   survived. Dropped from the `MA_LINUX` branch.

**Scoping note (S60's lesson applied prospectively, and it paid).** Fix (1) needed the
view's rect, so `MakeParentDialog` syncs `m_pView` from the GDI canvas. Left installed,
that measurably changed unrelated behaviour: the campaign map's tile loop consults the
view's client rect and, with a real rather than 0×0 rect, drew one more tile row
straddling the bottom edge at y=644 — which Windows would clip but our auto-growing canvas
did not, silently turning the capture into 1021×**900**. Wrapped in an RAII
`MaViewRectScope` that restores the previous rect, so the fix is confined to the one
computation that needs it. Caught by the parity sweep, not by eye.

### S61-2 Re-capture + #15 verdict flip (2 pts) — ✅
`map_playerlog.png` re-captured (1021×644, matching `campaign_map.png`). #15
**PARTIAL → CLOSE-minus**: the dialog is centred over the map, the Career / Log of
Missions / Last Mission tab bar renders with the real RTabs.ocx art, the selected tab is
raised, the Career photo + Name label + Name edit box render, and the map beneath shows
the toolbar and the `6/25/50: Morning, planning` date row as gold does. Named residuals
below.

### S61-3 Tab selection + `?`/`✓` title buttons (2 pts) — ◐ 1 of 2
- **Tab selection works and is capture-proven.** New gated hook
  `MA_OOB_PLAYERLOG_TAB=N` drives `GetDlgItem(IDJ_TABCTRL)->SelectTab(N)` in the same tick
  as the open — exactly what `CMainToolbar::OnClickedMissionlog` does for tab 2. Must be
  same-tick: once the dialog is up the map-idle branch stops running. New reference
  `port/ref/native/map_playerlog_tab1.png` shows **"Log of Missions" raised** with the
  Career tab's content correctly hidden.
- **`?`/`✓` buttons NOT done — and neither is the title bar (S60-2 carry-over).** Both are
  blocked on the same thing, now precisely diagnosed: `IDJ_TITLE` (1001) IS in IDD 276's
  template as an RButton, is not filtered, and is now hosted (S61 exempts it from the
  "skip caption-less controls" rule, since a title bar is captioned at runtime) — but it
  still draws nothing because **its art and caption live in its RT_DLGINIT property
  stream** (`idd=276 DLGINIT FOUND sz=188`, first id `e9 03` = 1001), which MA does not
  yet fully parse. That is precisely BoB note 17 traps 1/2 — the property-stream reader
  adoption already on MA's backlog behind the font cross-cut. It is a component, not a
  fix; pulling it here would have blown the sprint.

### S61-4 Cross-port note 19 + close (1 pt) — ✅
MA note 19 to `bob/doc/`; shared lessons-doc §8h; both copies md5-identical; board,
burndown, parity table, `RUNNING.md`, `SCRUM_STATUS.md` updated.

### Gates
- **2D parity regression sweep — CLEAN, 5/5 byte-identical** (`title`, `prefs_3d`,
  `prefs_controls`, `quickmission`, **and `campaign_map`** — added this sprint precisely
  because the view-rect change touches map drawing; it is what caught the 900px canvas
  regression above).
- `port/asan_all.sh` — PASS 4/4 modes, 0 reports.
- `port/stress_launch.sh` — PASS 20/20.

### Carry-over to S62
1. **The R* property-stream reader (BoB note 17 traps 1/2).** Now the top item, not a
   nice-to-have: it blocks the Player Log title bar, the `?`/`✓` buttons, and (per BoB's
   §3) the FONT/COLOR set behind the cross-cutting font deviation #1.
2. Career content table (Sorties/Combats/Kills/Losses) — the remaining half of I4.
3. **RScrlBar (`505aee46`) still unhosted** though created 16× on the campaign-map path.
4. Route real mouse clicks to `ma_tabs_hit` (the hit-test is written and unused; only the
   scripted `SelectTab` path is exercised so far).
5. #12 debrief capture; cross-cutting font (#1) / chrome (#2).
