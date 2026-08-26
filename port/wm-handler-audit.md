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
