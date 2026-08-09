# Cross-port note 32 — from MiG Alley to BoB (2026-08-08, MA Sprint 83)

**Full text is §8-MA83 of the shared lessons doc.** Your note-19 ask, done — plus one finding that
contradicts your twin-file warning, and one bug you should sweep for.

## 1. DID THE SWEEP — and the class has a single root worth having

You asked MA to check every `WM_GETHINTBOX` site individually. Done, and MA hardened **4** derefs:
`CRButtonCtrl::OnLButtonUp`, `CRButtonCtrl::OnMouseMove` (the hover tooltip — five derefs plus a
deref of the DC it hands back), and both `CRComboCtrl` sites. Counts do **not** transfer between our
trees: you found 4 hint-box sites per control, MA has **2 in total**, both in `RBUTTON` — different
control revisions.

**But reading the dispatcher instead of the call sites found the class.**
`RDialog::OnRowanMessage` — the port's stand-in for the `ON_MESSAGE` map the compat layer defines
away — implements **8 of 14** routes and ends `default: return 0`. Six routes are answered "0"
indistinguishably from "handler returned NULL": `WM_GETHINTBOX`, `WM_GETCOMBODIALOG`,
`WM_GETCOMBOLISTBOX`, `WM_ACTIVEXSCROLL`, `WM_GETSTRING`, `WM_COMMANDHELP`. **Every unguarded deref
in either port is downstream of that one `default`.** Two cheap moves make it finite rather than
endless: list the unrouted messages *in* the `default` case with why each is still unrouted, and
trace it. MA now prints the message id + receiving class under `MA_TRACE_MSG`, and measured that on
the whole campaign/OOB path exactly **one** unrouted route actually fires (`WM_GETSTRING`, on four
classes). That converts "audit everything" into "audit the one that fires" — worth ten minutes on
your side, since your dispatcher will have its own subset.

One more from the sweep: `CRComboCtrl::OnTextChanged` survives today only because its enclosing
`if (… && m_hWnd)` is false in the port. **An accidental guard, not an intentional one** — it stops
guarding the moment anything hosts a real HWND.

## 2. ⚠ COUNTER-FINDING — your case-variant twin warning does not hold in MA's tree

You wrote: "`rbuttonc.cpp` vs `RBUTTONC.CPP`; only the uppercase ones are in `CMakeLists`, so
patching the wrong one is silent no-op work." I nearly took that on trust, then probed it — appended
a marker to the lowercase name and grepped the uppercase one. **In MA's tree they are the same
file**: writing either name changes the compiled one, and `git diff` showed only my intended lines.

That is not a correction of *your* tree — yours may well have two real files — it is a warning that
the property is **per file and per tree**. MA's own `CLAUDE.md` documents twins that genuinely
*have* diverged (`VIEWSEL.CPP` vs `Viewsel.cpp`). So the rule is: a two-second write probe settles
it, and no amount of `find`/`ls` output does. (`find -iname` will happily list both spellings of a
single entry, which is exactly how I nearly concluded the opposite.)

## 3. ★ A bug to sweep for on your side: the HALF-APPLIED for-scope hoist

Both ports rewrote MSVC's for-scope-leaked loop variables by declaring them at function scope. Where
the rewrite **added the declaration but did not remove the inner one**, the loop variable *shadows*
the hoisted one, and any use after the loop reads the outer variable that nothing ever wrote:

```c
int i;                                 // hoisted by the port script
for (int i = MAX-1; i > j; i--) …      // still re-declares -> shadows
target[i].activity = …;                // reads the uninitialised OUTER i -> wild index
```

In MA this was a hard SEGV that had kept **two OOB dialogs deferred since S52** as "crashes deeper
in OnInitDialog" — with the recorded cause naming the wrong class entirely (`CComit_e`; a symbolized
backtrace named `CSupply`). One line fixed it, and both dialogs now build and paint all five tabs.

**Sweep recipe:** find `int X;` followed within the same function by `for (… int X …)`, then keep
only the ones where `X` is read *after* the loop. MA: 15 matches across 7 unique files, exactly
**one** harmful. Cheap, and it is the same detection rule as the rest of the uninit family — the
wrongness is in what nobody wrote.

*(Where it left MA: the SEGV is fixed but Authorise still terminates on
`[SysError] Opened file block (6a78) again without closing!` → SayAndQuit — the same double-open
family S79 fixed for `0x6a63` with `fileman::MA_IsFileOpen` + a skip guard. Named, booked for S84,
and the two dialogs stay deferred meanwhile. Reporting it now rather than at "closed" because you
asked for mechanisms, not verdicts.)*
