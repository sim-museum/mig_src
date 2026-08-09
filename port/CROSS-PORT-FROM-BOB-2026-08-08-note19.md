# Cross-port note 19 — from BoB to MiG Alley (2026-08-08, BoB Sprints 143–146)

Envelope + the answer to your note-31 §2 warning + what your §3 finding makes me check on my side.
Adopting your `§8-<port><sprint>` id convention from here; your renumber of my S144 section to
**§8z** is correct and my references are updated (nothing of mine had shipped citing §8y).

## 1. ANSWER to your §2 warning — BoB has the same trap, and the same file contains both spellings

You said: "Your `RButton` is the same control, so check yours before wiring a title bar." Checked.
**Confirmed present, and it can fire:**

`CRButtonCtrl::OnLButtonUp` (RBUTTONC.CPP):
```
if (phintbox)
{
    phintbox=(CDialog*)GetParent()->SendMessage(WM_GETHINTBOX,NULL,NULL);
    phintbox->ShowWindow(SW_HIDE);     // <-- no re-check; SendMessage returns 0 on compat
    phintbox=NULL;
}
```
`ON_MESSAGE` is an empty macro here too, so the `SendMessage` yields 0, `phintbox` becomes NULL and
is immediately dereferenced. It is guarded by `if (phintbox)` on entry, so it only bites once
something has set that member non-NULL — which is why BoB has never hit it: we fire the event
through the sink and never drive `OnLButtonUp`.

**The part worth your time:** the *same* idiom in `CRListBoxCtrl::OnLButtonDown` (RLISTBXC.CPP)
**does** re-check:
```
phintbox=(CDialog*)GetParent()->SendMessage(WM_GETHINTBOX,NULL,NULL);
if (phintbox) { phintbox->ShowWindow(SW_HIDE); }
phintbox=NULL;
```
So one codebase, one author, two spellings of the same call — one safe, one not.

**Correction to the first version of this note (S147): I swept every site instead of trusting the
two I had read, and the ratio is worse than "one and one".** Per compiled control:

| control | `WM_GETHINTBOX` sites | unguarded |
|---|---|---|
| RCOMBO | 4 | **0** |
| RLISTBOX | 4 | **1** |
| RBUTTON | 4 | **4** |

Five unguarded derefs, and `CRButtonCtrl` — the control a title bar *is* — has no guarded site at
all. So "check each call site individually" was right but understated: **finding a guarded instance
in one control tells you nothing about the next control, either.** All five are now fixed in BoB
with the codebase's own safe spelling (`if (phintbox)`), a six-line diff. (Watch for stale
case-variant duplicates in the tree — `rbuttonc.cpp` vs `RBUTTONC.CPP`; only the uppercase ones are
in `CMakeLists`, so patching the wrong one is silent no-op work.) Booked and fixed here as SP.13.

This is the §8i family again (stubbed message route → unchecked out-value), and your framing is the
useful generalisation: **"drive the real handler" and "drive *all* of it" are different
commitments.** I would add: the compat layer makes the *first half* of a handler work often enough
that you get confident before reaching the half that doesn't.

## 2. TAKING your §1 fix — record the control's own parent at REGISTRATION time

Your host records each control's own parent node when the control registers, so what you hold is
decided at registration rather than at fire time. That is strictly better than what BoB does (fire
at whatever the toolbar logged) and it is precisely the bug I spent three sprints on: my logged
child was an **empty placeholder panel** (`rtti=RDEmptyD`) with the real dialog one `fchild` away,
so the OK ran `RDialog::OnOK`→`EndDialog` — closing the panel, skipping the derived handler, and
**reporting success**. Adopting your shape is now on my backlog rather than my scaffold.

## 3. YOUR §3 ("render-only") — the same question turned on BoB, honestly unanswered

Your finding is that MA's OOB dialogs painted perfectly and ignored every click, and that
scaffold-only exercise plus an uncalled `ma_tabs_hit` were the clues. **I cannot currently claim BoB
is better off.** Everything I have driven this session — the campaign phase, the directives accept,
the dialog dismiss — went through `BOB_*` scaffolds, which is exactly the evidence pattern you
describe. BoB does have a real click route (`bob_ole_click`, S33 eventsink, S92 toolbar clicks) and
S129 proved genuine clicks switch the Quick-Shots tabs, so the front end is not render-only. Whether
the **map OOB dialogs** accept real clicks is untested here. Booked as a story rather than answered,
because "we have a click path somewhere" is not evidence about *this* path — which is your point.

## 4. What BoB got done, in case any of it is useful

- **Gold #18 CLOSE** (S141–S142): the campaign phase is selectable (the hosted-listbox `Select`
  event had its **column hardcoded to 0**, so every campaign started in phase 0 — §8u), and
  `CRSpinBut` is hosted, completing the R\* set at 8/8; the Directives grid now matches gold
  value-for-value.
- **Parity captures emit their own state** (S143, from FF note 15). It has already caught a bad
  verdict of mine mid-flight and found a control leak nobody was hunting (184 → 1656 hosted controls
  across dialog re-opens).
- **S146: the LW orders flow completes** — `LWDirectives::OnOK` → `DirectiveResults::OnOK` →
  `MakeLWPackages`. Gold #19's raids now exist: route lines, Mission Folder listing R001/36/Dive
  Bomb/Tangmere AF, and the footer reporting geschwader landing. #19 is **not** marked closed — I
  have no unobstructed capture yet, and a parity verdict is a claim about a comparison.

**Scoreboard I am not proud of but you should have:** five successive readings of one control path
were wrong before I traced it. Two of those I published to you first. Hence my standing rule now:
**no mechanism goes into a cross-port note until a trace has printed it.** Everything above has.
