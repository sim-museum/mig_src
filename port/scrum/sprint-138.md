# Sprint 138 — "The game always asked; nobody was listening" (PO-29) — ✅ CLOSED 2026-08-16 (goal MET)

**Planned 2026-08-16 (PO: continuous sprints on the campaign GUI).**
**Sprint Goal:** leaving the campaign asks before it throws the player out.

| Story | Pts | Result |
|---|---|---|
| S138-1 why does X quit without asking? | 3 | ✅ `DoModal` was `{ return -1; }` |
| S138-2 give the port a modal loop | 5 | ✅ `RMdlDlg::DoModal`, input + paint + present |
| S138-3 the gate asserted the wrong behaviour | 2 | ✅ rewritten to walk the real path |

The PO: *"clicking on X from map drops you to the landing page (with some stale text on it), not
to a save/quit/cancel dialog as it should."*

## The game does ask

`CMainFrame::OnBye` — the handler behind the map's X — is:

```c
int rv = RDialog::RMessageBox(IDS_QUITGAME, IDS_AREYOUSURE, IDS_SAVE, IDS_L_YESNO_YES, IDS_CANCEL);
if (rv==0)      { MiscToolBar().OpenSaveOnly(true); return; }   // Save
else if (rv<2)  { ... LaunchFullPane(&RFullPanelDial::title); } // Yes, quit
                                                                // rv==2 -> Cancel, stay
```

and `RMessageBox` ends in `m_pMessageBox->DoModal()`. **The port had no modal loop at all**:
`CDialog::DoModal` was `{ return -1; }` and `EndDialog` was `{}`. `-1 < 2`, so every confirmation
in the game returned "yes, quit" — and was never shown, because nothing drew it. Not a broken
dialog: a dialog that never existed, with a stub answer that happened to select the most
destructive branch.

## What the port needed

`RMdlDlg::DoModal` (MA_LINUX) now runs the loop the port never had:

1. `Create(IDD)` — template, `OnInitDialog` (which sets the captions from the string ids the
   caller passed), hosted controls;
2. centre it on the canvas from its own control extent;
3. loop: pump input → let the dialog paint **its own art** (`OnPaint`, under a viewport origin,
   because it blits at `OnGetXYOffset()` = 0 and the port has one canvas) → draw its hosted
   controls → present;
4. until a button calls `EndDialog` — 0 = OK/Save, 1 = Cancel, 2 = Retry (`RMDLDLG.CPP:194-207`);
5. remove its controls, and return the recorded result.

While it runs, input goes to that dialog alone — which is what modal means, and the game has
already disabled every toolbar around the call. `BOB_CLICKSEQ` injection comes through the same
path, so a modal is scriptable.

Two deliberate limits. It is **scoped to `RMdlDlg`** rather than `CDialog`: the other `DoModal`
call sites are the About box and two forwarding overrides, and a nested loop under a dialog the
port drives differently would hang rather than fail visibly. And the loop is **bounded** — a modal
that can never be dismissed returns Cancel rather than freezing the game, because Cancel is the
safe answer for every caller.

## The stale text was the dialog itself

With the modal gone, its hosted controls stayed in the draw registry and kept painting over the
title screen — precisely the "some stale text on it" in the report. `RDialog::DestroyPanel`
already does `ma_ole_remove_by_parent` for panels; a modal created and abandoned on every
invocation needs it just as much.

**A second ghost survives and is now PO-33:** a "Load Campaign / Auto Save" panel is still painted
over the title after quitting. It is invisible on the campaign map (which draws only parent-scoped
chrome) and reappears on the title screen (global draw pass), so it is a panel-lifecycle gap, not
a draw-order one.

## The gate had encoded the wrong behaviour

`sysbox_exit.sh` asserted that clicking X leaves the campaign **immediately** — which is what the
stub made it do. It failed the moment the confirmation appeared, correctly. It now walks the real
path: click X, require the confirmation to open, **locate "Yes"** among the dialog's yellow
captions, answer, and require the player to have left.

Three gate-writing faults, all mine, worth recording because each produced a confident wrong
verdict:
- the button locator searched the **whole canvas** for the strongest yellow row and found the
  map's chrome — the modal is drawn over a live map. The dialog now reports its rect when it
  opens (uncondtionally, not behind a trace flag: a test that must find the dialog needs it).
- `rc=$?` after a helper whose last statement was `pkill` captured **pkill's** status, and pkill
  exits 1 when nothing matched → "RESULT: CRASH" on a healthy run.
- the exit check compared against the committed 800×600 reference and **short-circuited on the
  size mismatch** ("different size, so we must have left"), which would have passed with the map
  still on screen. It now compares against the map as captured *in the same run*, outside the
  dialog's rect: 82.9% of the map area changes.

`MA_MODAL_SHOT=<path>` was added for the same reason — `MA_SHOT` counts idles, and the idle loop
is exactly what a modal suspends, so the standard capture hook can never photograph one.

## Evidence

`port/ref/native/quit_confirm.png` — **QUIT GAME / Are you sure? / SAVE  YES  CANCEL** over the
campaign map, on the game's own army artwork. Cancel returns to the map (non-black pixel count
identical to the map baseline); Yes reaches the title screen.

## Gates

parity 5/5 byte-identical · sweep 9 OPEN/0 CRASH · map icon click · map drag · help click ·
map filter · **sysbox exit (rewritten, now covers the confirmation)**.
