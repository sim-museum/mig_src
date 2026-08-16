# Sprint 139 — "Naming the ghost" (PO-33) — ⚠️ CLOSED 2026-08-16, goal NOT met (located, not fixed)

**Planned 2026-08-16 (PO: continuous sprints on the campaign GUI).**
**Sprint Goal:** no stale panel painted over the title screen after quitting a campaign.

| Story | Pts | Result |
|---|---|---|
| S139-1 which dialog is the ghost? | 3 | ✅ `MA_TRACE_GHOST` — a `CLoad` panel, 4 controls |
| S139-2 close it when it closes | 5 | ❌ two hypotheses tried, both refuted |

**The goal was not met.** The ghost is still there. What this sprint produced is a name, a
mechanism, and two dead ends closed off — which is worth more than another guess.

## What is on screen

`MA_TRACE_GHOST` prints the distinct OWNERS the global draw pass will paint, with their runtime
class — the registry is keyed by control, not by owner, so a screenshot cannot tell a ghost from a
legitimate panel. On the title screen after quitting a campaign:

```
[ghost] pass 401 owner=0xb379f00 class=5CLoad visible=1 controls=4
```

A `CLoad` panel — the Load Campaign dialog used to start the campaign — still visible, four
controls, hundreds of passes after the campaign it launched was quit. It is invisible on the
campaign map only because that screen draws parent-scoped chrome and nothing else.

## Where the close path stops

`RDialog::EndDialog` is a full teardown: `DialExitFix` → `ChildDialClosed` → `DestroyWindow`. It
is never reached, because `RDialog::OnOK` begins:

```c
if (edges.l & EDGE::ACTIONS_ARTCHILD) return;
```

and `MA_TRACE_OOBCLICK` reports this node as **`artchild=1`**:

```
[evt_fire] id=1055 dispid=1 type=5CLoad -> HANDLER CALLED
[oobclick] RDialog::OnOK (BASE, this=0x977deb0 artchild=1)
```

So the panel's OK is swallowed by design — the flag means "this node is art, the OK belongs to an
ancestor" — and nothing closes it. The open question is whether this node should carry that flag
at all, or whether the port is firing the OK at the wrong node of the dial tree. That is where the
next attempt starts, and it is a question about the dial tree, not about drawing.

## Two hypotheses tried and refuted

Recorded so no one spends the time again:

1. **Remove the controls in `CDialog::EndDialog`.** Refuted — the ghost survived unchanged, which
   is itself the evidence that `EndDialog` never runs for this panel.
2. **Remove them in `CWnd::DestroyWindow`.** Also refuted for this defect, for the same reason:
   nothing calls it here.

The second change was **kept** even though it is inert for PO-33, and that deserves saying
plainly. `DestroyWindow` was `{ return TRUE; }`, and on Windows a destroyed window takes its child
windows with it — the port's hosted OCX controls *are* those children. It is the same
stub-shaped gap this port keeps finding, and it will matter the moment the ARTCHILD question is
answered. It is env-revertible (`MA_NO_DESTROY_REMOVE=1`) and the full suite is green with it,
including `panel_click` — the canary that caught S128-S130's dead front end.

## Gates

parity 5/5 byte-identical · sweep 9 OPEN/0 CRASH · map icon click · sysbox exit (83.0% of the map
area changes on quit) · help click · panel click · map filter.
