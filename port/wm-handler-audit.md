# MA — duplicated `WM_*` protocol handlers (backlog N3, MA half)

**S267, 2026-08-25.** Companion to BoB's `doc/wm-route-audit.md`.

## The question this answers

Three MA bugs this month came from one class — a handler that exists but is not reached, or one of
several copies that drifted:

- **S243** — a fix placed in `OnShowWindow`, a `WM_SHOWWINDOW` handler the compat never delivers, so
  it silently did nothing; the *same* dead hook was also the only caller of `SetDisabled(false)`.
- **S248** — `CRToolBar::OnGetFile`'s guard admitted only directories 104..113 and **blanked 732 of
  1347 art fetches**, while its twin `RDialog::OnGetFile` had been widened long before.
- **S249** — after fixing S248 I *left two more copies narrow*: `CMIGView` and `RMdlDlg`.

## The structural finding

**MA implements each `WM_*` protocol handler in up to FOUR separate classes** — `RDialog`,
`CMIGView`, `CRToolBar`, `RMdlDlg` — each a **hand-written copy**:

| handler | distinct classes |
|---|---|
| `OnGetFile` | 4 |
| `OnReleaseLastFile` | 4 |
| `OnGetGlobalFont` | 4 (+`RFullPanelDial`, `TitleBar`) |
| `OnGetXYOffset` | 4 (+`CCampBack`) |
| `OnGetString` | 3 (+`Rtestsh1`) |
| `OnGetOffScreenDC` | 3 |
| `OnGetHintBox` | 3 |
| `OnPlaySound` | 3 (+`CListBx`) |

⭐ **BoB has ONE.** `DIALCLASS::OnGetFile` in `RDIALMSG.CPP`, `#include`d three times with `DIALCLASS`
redefined — all classes get identical code **by construction**, and there is no twin to forget
(S249). **That is the shape worth adopting**: a single implementation instantiated per class makes
divergence impossible rather than merely unlikely.

⚠️ **And every count above DOUBLES on disk** — MA tracks case-colliding twins (`RDIALOG.CPP` *and*
`RDialog.cpp`, `MIGVIEW.CPP` *and* `MIGView.cpp`) which have diverged before. A fix applied to the
wrong twin is silently inert; that trap has been hit at least four times this month (S240, S249,
S258, S260), each caught only by checking what the unity `#include`s.

## What this audit does NOT claim

**It does not claim the copies currently disagree.** The scan captured the first hex constants
following each definition, and that window bleeds into neighbouring code and into S249's own
comments — so its "copies disagree" flags are partly an artefact of a crude instrument. *Reporting
them as findings would be the same mistake as S250*, which audited a dispatcher by reading a file
that does not enumerate what it dispatches.

**What is established** is the *duplication*, which is a fact about the code and is the precondition
for divergence. S248/S249 already proved that divergence real once, for `OnGetFile`.

## The next step, precisely

For each handler above, diff the four copies **by body**, not by scanning for constants — the
divergence that mattered was one integer in one of four otherwise-identical functions. Where they
agree, collapse them onto BoB's macro-include pattern; where they differ, decide which is right
before unifying.


---

## S271 — widening the body-diff: one real find, ~101 false positives

After S269 found a real defect by diffing bodies, the obvious move was to widen the same scan to
**every** method with 3+ compiled copies and flag the "N agree, one differs" signature.

**Result: 102 candidates, of which one was already known (`OnReleaseLastFile`, fixed in S269) and
essentially all the rest are noise.** Two representative false positives:

- **`CRListBoxCtrl` is flagged as "odd one out" for ~60 accessors** (`GetBorder`, `SetLineColor`, …).
  The three "copies" are `CRListBoxCtrl`, `CRComboCtrl` and `CREditCtrl` — *different controls that
  happen to share a method name*. Two have trivial identical accessors and the listbox's does more.
  Nothing is wrong.
- **`WindowProc`: 4 copies, 3 agree, `CMainFrame` differs by 2 characters.** The difference is
  `CFrameWnd::WindowProc` vs `CDialog::WindowProc` — **the base class name**. Correct in both.

⭐ **THE REFINED CRITERION, which is the actual deliverable:** body-diffing finds real divergence only
when the copies **implement ONE protocol across classes** — the `WM_*` handlers, where every class
answers the same message and must answer it the same way. It finds nothing but noise when the copies
merely **share a name**. S269 worked because `OnReleaseLastFile` is the former; S271's widening
swept in the latter and drowned the signal 100:1.

**Stated plainly because the temptation is real:** 102 candidates *looks* like a productive audit.
Reporting them as findings would have manufactured a hundred investigations out of a filter that
cannot tell a protocol from a naming coincidence. **The scan is kept, narrowed to the `WM_*` family
where it has actually paid.**
