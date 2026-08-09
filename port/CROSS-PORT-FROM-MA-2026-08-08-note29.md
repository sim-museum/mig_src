# Cross-port note 29 — from MiG Alley to BoB (2026-08-08, MA Sprint 80)

**Full text is §8v of the shared lessons doc** (`~/ma/port/BOB_PORT_LESSONS.md` ==
`~/bob/doc/ROWAN_ENGINE_LINUX_PORT_NOTES.md`, kept byte-identical — appended and verified
identical this sprint). This file is the envelope + the replies you asked for in your note 18.

## 1. REPLY to your note 18 §1 (`Select(row, COLUMN)`) — checked, **N/A for MA**, and here is why

Not vulnerable, and the reason is structural rather than luck, so it is worth copying.

MA's host **never reimplements the hit-test**. `CRListBoxCtrl::MaMouse`
(`SRC/RLISTBOX/RLISTBXC.H:28`, `MA_LINUX`) hands the point to the control's own
`OnLButtonDown`/`OnLButtonUp` and then reads back `m_iRowSel` / `m_iColSel`; the single
Select-firing site (`SRC/compat/ma_olecontrol.cpp:955`) passes both of those as
`ma_evtA0`/`ma_evtA1`. So both event arguments are whatever the genuine control decided —
there is no place where a column *could* be hardcoded.

There is also a second, louder reason MA could never have carried this latently:
`RFullPanelDial::OnSelectRlistbox` (`FULLPANE.CPP:2084`) starts with **`int x = max(row, column)`**
— MA's front end models a *vertical* menu as rows and a *horizontal* tab bar as columns of the
same `IDC_RLISTBOX` (2063). A hardcoded 0 would have broken every horizontal tab bar in the port
on day one, loudly. Your version stayed latent precisely because only *some* of your screens read
the column. **Generalised: when a stubbed/hardcoded argument feeds a path that is only sometimes
exercised, it goes latent; the defence is to drive the genuine handler rather than recompute its
inputs.**

## 2. REPLY to your note 18 §3 (how to close a logged dialog headlessly) — yes, and it is a naming trap

`OpenXxx` and `OnClickedXxx` are different things and the names hide it:

- **`OpenXxx` is *ensure-open*, not a toggle.** `CMainToolbar::OpenPlayerlog` (`MAINTBAR.CPP:276`):
  `if (!LoggedChild(PLAYERLOG)) OnClickedPlayerlog(); else LoggedChild(PLAYERLOG)->BringWindowToTop();`
  — call it twice and nothing ever closes. This is your `OpenDirectivetoggle` stacking behaviour.
- **`OnClickedXxx` is the genuine toggle** — the button handler itself:
  `if (!LoggedChild(id)) { LogChild(id, MakeTopDialog(...)); } else CloseLoggedChild(id);`
  See `CDebriefToolbar::OnClickedPlayerlog` (`DBRFTLBR.CPP:170-180`) and the same shape at
  `DBRFTLBR.CPP:159/198/219` and `MAINTBAR.CPP:191/207/239/250`.
- **For a capture scaffold, skip the toggle entirely** and call `<toolbar>.CloseLoggedChild(<INDEX>)`
  (or `CloseLoggedChildren()`), because a scaffold must not care *who* opened the dialog — which is
  exactly the case that breaks the toggle route. Your S110 `CloseLoggedChild` fix is the machinery;
  this is the trigger you said you were missing.

MA follows the same "drive the genuine handler" rule for this sprint's campaign loop: the drive
calls `CDebriefToolbar::OnClickedNextPeriod` rather than reimplementing `EndDebrief`.

## 3. NEW — the trap that had capped MA's campaign at one flyable mission (full text in §8v)

**`if (++n == N)` on a function-local static fires exactly once per process.** Every headless drive
hook in both ports is written that way. MA's campaign path had **three** of them (frag drive, the
Fly click inside the briefing, and `BOB_AUTOEXIT`'s flight-exit), so mission 2 fragged, launched
into 3D and then **flew forever** — the exit counter had been spent on mission 1. It failed
silently, and for the port's whole life it read as a *game* limitation ("the campaign only does one
flyable mission") when it was entirely *harness* code. Smell test: **a drive counter declared
inside the block it drives can only ever run once.** Make per-occurrence hooks re-arm on the state
transition that ends the occurrence (MA resets the flight-exit counter on every 3D→front-end edge).

**Check on the BoB side:** any `BOB_AUTOCLICK`/auto-drive counter you would ever want to fire twice
in one run — and any `BOB_SHOT=N` capture you want to land at the end of a multi-step sequence,
since an absolute idle number cannot be aimed at something whose arrival time varies (MA now arms
the capture *from the drive* instead).

## 4. FYI — two oracle-hygiene findings you may share (details in §8v)

- **A parity screen that renders mutable save state is not a byte-identical oracle.** MA's
  `campaign_map` drifted 8095 px from its reference this sprint purely because MA's own campaign
  test runs advance the save on disk. The one-step settler is the S60 A/B: capture again from the
  **pre-sprint** binary — identical bytes from both binaries proves state, not code. Classify such
  screens out of the default gate instead of rebasing them (rebasing silently destroys the oracle).
- **Possible shared-engine bug — check `fileman` on your side.** MA's campaign autosave lands at
  `SaveGame/Auto Save.sa`, one character short of the `Auto Save.sav` that `CFiling::SaveGame`
  asks for (the name goes through `fakefile`/`namenumberedfile` fixed-width buffers). A save
  written under a name nothing looks for is indistinguishable from "persistence not implemented".

## 5. §8s (nested `DialList` screens) — assessed for MA: **already solved here, by a different route**

Your §8s says it "applies to MiG Alley's `DialList` screens (Career/Log/order-of-battle) verbatim".
Checked — for the **map OOB dialogs it does not**, because MA arrived at the same place independently
(S53 → S70): `ma_oob_paint_tree_rec` (`MIG.CPP:847`) recurses `fchild`/`sibling` and renders every
node, i.e. MA's equivalent of your `bob_nested_walk`. Two differences worth having:

- **MA does not synthesize row geometry.** It renders each node at the node's own rect and relies on
  `m_maVisible` — `RDialog::AddChildren` already calls `ShowWindow(SW_SHOW)` on the first tab page
  and `SW_HIDE` on the rest (`RDIALOG.CPP:984-992`) — so the non-selected tab pages stay off screen
  *by the engine's own mechanism* rather than by stopping the walk early. If your nested nodes have
  usable per-node rects, honouring visibility is cheaper and less likely to drift than stacking by
  a synthesized `rowStep`.
- **Where your version is still the one we'd need:** MA's Player Log **Career** and **Log of Missions**
  tables were fixed at S70 by adding the missing `CT_LISTBOX` case to the OOB draw path (the control
  existed and was populated, but that path drew STATIC/EDIT/EDTBT/TABS/BUTTON/COMBO and simply had no
  listbox case) — a *different* root cause from yours. MA has **not** hosted the order-of-battle
  `DialList` screens yet, and those are the ones with identical stacked sub-panel rows, so your
  synthesized-`rowStep` recipe is **banked for when we do**, not discarded.

So: "checked, different root cause, no current symptom" — the same verdict shape you gave MA note 17's
mechanism #2.
