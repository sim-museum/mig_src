# Cross-port note 39 — from MiG Alley to BoB / FreeFalcon (2026-08-09, MA Sprint 98)

**One symptom, four dead links — and the trick that found the last one.**

Full write-up: **§8-MA98** in the shared lessons doc.

MA's "?" button on dialog title bars did nothing. Four independent breakages, all in the port, each
invisible until the previous one was fixed:

1. the title-bar hit router **returned early** for the help band;
2. **`WM_COMMANDHELP` was not defined at all** — MFC's private message (`afxpriv.h`, `0x0365`).
   This is §8-MA83 at its purest: while `ON_MESSAGE` expanded to nothing it never evaluated its
   message argument, so the symbol had never been *required to exist*;
3. `CWnd::SendMessage` dispatched only `WM_USER+` (`>= 0x400`) — this one is *below* that;
4. `CWnd::OnCommandHelp` was a **non-virtual** stub returning 0, and `CDialog` overrode it back to
   0, so the frame's override — the thing that actually opens help — was unreachable *and*
   undispatchable through a `CWnd*`.

## The two things worth taking

**Read the route's return value, not the fact that you sent it.** The send returned **0** after
fixes 1–3 and **1** after fix 4. That single number identified the last dead link instead of
guessing. *"Delivered" and "handled" are different claims, and a chain of stubs returns a plausible
0 at every step.* If you are wiring the message routes described in note 34/§8-MA83, log what each
handler returned — otherwise a restored route that lands on a stub looks exactly like success.

**Check your compat headers for non-virtual "overrides".** A non-virtual `CWnd` stub that a derived
class appears to override compiles cleanly and silently never runs. Any `afx_msg`/`virtual`
mismatch is a route that looks wired and is not — worth a grep in BoB's `afxwin.h` today.

## Two smaller ones

- **Recipes should name symbols, not pixels.** MA added `#ID@Class:?` = "the help glyph of this
  title bar", resolved by asking the control's *own* hit-test where its help band is. Glyph
  positions move with dialog width and font.
- **`sscanf` returns the number of ASSIGNMENTS, not literals.** A format ending in a literal `:?`
  matches happily when `:?` is absent — MA's new branch silently stole an unrelated recipe entry.
  Verify literal tokens yourself.

## Scope note (may apply to BoB's help too)
Routing was a real fix; there is still **no WinHelp viewer**, so nothing is displayed. MA recorded
the defect as **half closed** and made the gate print that boundary in its own output. Recon first:
`port/tools/hlp_probe.py` (portable, takes any .hlp) reports MIG.HLP is WinHelp 4 with **44 topics**
and **35 context→topic mappings** — so building a viewer is now a decision with facts behind it.
If BoB ships a .hlp, the same tool will tell you what is in it.
