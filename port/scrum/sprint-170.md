# Sprint 170 — "The last unhosted control, and the two doors in front of it" (K5) — ✅ CLOSED 2026-08-22 (goal MET, 8/8)

**Planned 2026-08-22** (PO ceremonies pre-approved). The PO: *"fix the spin button host in ma so
step 8 works."*

| Story | Pts | Result |
|---|---|---|
| S170-1 host RSpinBut | 3 | ✅ `SRC/compat/ma_olespin.cpp`, dispids 1–12 |
| S170-2 reach the dialog that owns it | 2 | ✅ `CT_EDTBT` clicks route — the duty field opens ChooseSquad |
| S170-3 address the right cell of the wave table | 1 | ✅ new `:rN.C` recipe form |
| S170-4 commit the dialog like a player | 1 | ✅ new `:-3` / `:-4` title-bar OK / Cancel bands |
| S170-5 gate step 8 end to end | 1 | ✅ `port/add_flight.sh` — Flights **2 → 3** |

## The finding

**RSpinBut was the last R\* type the port never hosted.** Nothing ever failed to say so: the wrapper
`SRC/MFC/RSPINBUT.CPP` has compiled since bring-up, `ma_ole_create` had no branch for CLSID
`c3270e66`, so every `InvokeHelper` on a spin button went to a control that did not exist. Silent,
for 170 sprints — the same shape as S168's discarded eventsink maps: *the port's failure mode of
choice is a no-op that returns success.*

`ma_olespin.cpp` hosts it against the **wrapper's own dispid map** (1 RepeatDelay, 2 Index,
3 FontNum, 4 CurrentValue, 5 AddString, 6 DeleteString, 7 Clear, 8–12 the range setters) rather than
against a guess at map order. Its header keeps the dispatch block `protected:` where RCombo and
RListBox leave it public — `BEGIN_OLEFACTORY` reopens `public:` and those two headers never
re-specify, RSPINBTC.H does. A thin derived accessor republishes exactly the members the host needs;
no game source is edited for access.

## Two doors were shut in front of the spinner, and both looked like working code

**1. `CT_EDTBT` was drawn and inert.** `IDC_ACTYPE` — the `F84 (2)` duty field on the TASKS dialog —
is an RedtBt, and `CREdtBtCtrl::OnLButtonUp` fires `Clicked` for any press-and-release that did not
become a drag. It is the **only** door into `ChooseSquad`, the dialog that owns the Flights spin-box.
Hosting the spinner without this reaches nothing.

This is the fourth control type found missing from `ma_ole_toolbar_click`'s filter: S87 listbox rows,
S140 scroll bars, S163 combos, S170 edit-buttons. **The filter is an allowlist, and an allowlist
fails silently by construction.** Every remaining type should be checked deliberately rather than
discovered one epic at a time.

**2. `:rN` addresses a row, and a row's centre is a cell.** The Profile wave table is
`Wave / ToT / Main Duty / AAA Cover / Air Cover`. `:r1` clicks the row's horizontal centre, which is
**column 3** — so `CProfile::OnClickedTask`, which reads `currcol`, opened the **flak** tab, and the
gate would have measured the port editing a duty the walkthrough never touched. The recipe read
correctly and addressed the wrong cell: S85 (an ambiguous id) and S162 (a row read as a control),
one dimension further out.

New form **`#ID@Class:rN.C`** names the cell, resolved through the control's own `GetRowFromY` **and**
`GetColFromX` — the two resolvers already existed and had simply never been usable together, because
they shared one `col` parameter. The row and column now travel in one int, encoded in exactly one
place and decoded through `MA_RC_ROW` / `MA_RC_COL`.

## Committing the dialog

A gate that can only click controls can prove a control moved; it cannot prove the change reached the
mission. `ChooseSquad::OnOK` recalculates the route and refreshes the parent, and a player reaches it
by pressing the tick on the title bar. **`:-3` / `:-4` address the OK / Cancel bands**, generalising
S98's `:?` for Help through the same `MaButtonHit` scan. No parser change — the generic `:%d` form
already carries a negative column.

## The spinner refuses correctly, and the first assertion would have missed it

`CRSpinButCtrl::OnTimer` goes UP only while `m_index <= m_list.GetCount() - 2`. A spinner at its limit
**takes the click and does nothing** — and the first run of this gate looked exactly like a broken
host: click delivered, arrows hit, index unchanged. It was a 2-entry list (`"0"`, `"1"`) at index 1,
i.e. the AAA-cover slot reached through the wrong cell, correctly at maximum.

So the gate asserts **the index changed**, not that the click was delivered. BoB S197 read the same
control the same wrong way; the note is now in both ports' ledgers.

## Result

`port/add_flight.sh`, headless, from the pinned campaign save:

```
  wave table cell selected: row=1 col=2 (Main Duty)
  duty field opened the squadron dialog: yes
  click ... local(41,6) rect=49x24 index 1 -> 2 value 0 -> 0 (list has 3 entries)
  spinner index moved: 1 -> 2
  ChooseSquad::OnTextChangedRspinbutctrl1 ran: yes
  Mission Folder Flights for "Wonju": 2 2 3 3
  flight count 2 -> 3: yes
```

The **Mission Folder** now lists `Wonju Supply Dump  Bomb  08:30  3` — the walkthrough's own
"cheapest end-to-end assertion in the epic", read from the game's `AddString` trace rather than from
pixels, so it cannot pass on a redraw artefact.

## Two compat gaps found on the way in

- **`SRC/RSPINBUT/RSpinBut.h` did not exist.** Every sibling OCX project carries a case-exact symlink
  to its shouty on-disk twin (`RScrlBar.h -> RSCRLBAR.H`); the spin project had one for `RSpinBtC.h`
  and not for `RSpinBut.h`, which is where `extern const WORD _wVerMajor` is declared. It had never
  mattered because nothing compiled that TU.
- **`CWnd::ReleaseCapture` did not exist.** `CRSpinButCtrl` calls it `this->`-qualified, forcing
  member lookup; the sibling controls call it unqualified and resolve to the global
  `::ReleaseCapture` in `compat_winuser.h`. Added as a no-op member beside `SetCapture`.

## Residual

- The **Off-Duty 3rd-flight-slot route** to the same outcome (the walkthrough's "or") is not
  asserted; nor is *persists into the frag* — both belong to **K7/K9**.
- `WPDETAIL`'s ETA spinner is now hosted too but has never been driven — it is on the **K8** route
  (waypoint detail), so it will get its first exercise there.
- The remaining unhosted control types are still an allowlist. **Worth a deliberate sweep rather than
  a fifth discovery.**
