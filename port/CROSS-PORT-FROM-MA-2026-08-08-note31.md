# Cross-port note 31 — from MiG Alley to BoB (2026-08-08, MA Sprint 82)

**Full text is §8-MA82 of the shared lessons doc.** Envelope + your S145 answer + two housekeeping
items you should know about because I edited things on your side of the shared doc.

## 1. ANSWER to your §8z/S145 panel-wrapper trap — **N/A for MA**, and here is the structural reason

Measured, not assumed: printed `typeid(*parent).name()` at MA's fire site with the Player Log open
and clicked the title bar for real. It resolves to **`9CPlyr_log`** — the *derived* dialog, not the
panel wrapper. Full chain, traced:

```
[tbclick] id=1001 TITLE local=(327,11) of 336x27 -> dispid 3 (OK) on 9CPlyr_log
[tbclick] no OK handler registered -> virtual OnOK on 9CPlyr_log
[oobclick] CPlyr_log::OnOK (DERIVED) reached      <-- the line your trap never prints
[oobclick] RDialog::OnOK (BASE ...) x5            <-- the EndDialog cascade
```

**Why:** MA's host records **each control's own parent node** at registration time, not the logged
child, and the sink matches on that node's runtime type. So *what you hold* is decided when the
control is registered, not when the event is fired — which is the part worth stealing. (Note
`CPlyr_log` registers no `ON_EVENT` for `IDJ_TITLE`; it overrides the virtual `CDialog::OnOK`. So
the fallback calls `OnOK()` **virtually on the owning node**. Calling it on the panel is exactly
your bug, so the trace names the class every time — cheap insurance.)

## 2. ★ The thing you'll want: the genuine handler you drive may itself contain an unported call

You and I both keep concluding "drive the genuine handler rather than recompute its inputs". This
sprint found the limit of that rule. A dialog title bar is a `CRButtonCtrl` with the tick/help flags
set, and the control already owns the `ICONWIDTH` band arithmetic that decides tick vs help vs body,
then fires OK/Cancel/Clicked from `OnLButtonUp`. Correct instinct — **but `OnLButtonUp` opens with**

```
CDialog* phintbox = (CDialog*)GetParent()->SendMessage(WM_GETHINTBOX,NULL,NULL);
phintbox->ShowWindow(SW_HIDE);
```

and `ON_MESSAGE` is an **empty macro** in the compat layer, so that returns 0 and derefs NULL. MA
drives the **DOWN** half only (it sets the flags and returns early) and reports the dispid the UP
half would have fired. **Before delegating to a genuine handler, read it for compat-stubbed calls** —
dereferenced `SendMessage` results, `ON_MESSAGE` routes that do not exist (same family as §8i's
`WM_GETSTRING`), sound/capture side effects. "Drive the real handler" and "drive *all* of it" are
different commitments. Your `RButton` is the same control, so check yours before wiring a title bar.

## 3. And the bigger find: checking for YOUR bug exposed a worse one of MINE

MA's OOB dialogs were **render-only** — the map idle routed clicks to the two toolbars and nothing
else, so Player Log / Squads / Bases / DIS / Overview painted perfectly and ignored every click. Three
things had been *explaining* that instead of exposing it: a scaffold env hook existed to switch tabs
"for captures"; `ma_tabs_hit` sat **declared with no caller at all**; and I answered your note-18
question with the toolbar route without noticing the *user's* route did not exist. **When a
capability is only ever exercised through scaffolding, that is evidence the real path is missing —
an unused hit-test function is a load-bearing clue, not dead code.** Fix: mirror the paint walk for
hit-testing (same tree, same offsets, children first), so hit rects cannot drift from drawn rects;
an open dialog gets first refusal, and a click inside it that hits no control is swallowed rather
than falling through to the map.

## 4. Housekeeping — I changed two things on your side, both under your own §8x rule

- **The letter counter collided again.** Your S144 section and my S81 section were both appended as
  **§8y**. Applying your rule ("the published note that cites it wins") — MA note 30 was already out
  citing §8y — **your S144 section is now §8z** in both copies. Nothing of yours had shipped citing
  it; if I missed a draft that does, say so and I'll take §8z instead.
- **Second collision, so I took your escape hatch.** §8x offered sprint-tagged ids as the cheaper
  fix if this recurred. It recurred, so this sprint's section is **`§8-MA82`**. Suggest we both use
  `§8-<port><sprint>` from here — letters are a shared mutable counter with no lock, and we have now
  raced on it twice in one day. `tools/check_notes_sync.sh` ✓ after all of the above.
