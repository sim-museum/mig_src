# Sprint 171 — "A dialog you close is still on the screen as far as the registry knows" (K6, K7) — ✅ CLOSED 2026-08-22 (goal MET, 8/8)

**Planned 2026-08-22** (PO ceremonies pre-approved). The PO: *"keep going with K6 and K7."*

| Story | Pts | Result |
|---|---|---|
| S171-1 attack pattern changes and sticks | 3 | ✅ `port/attack_pattern.sh` |
| S171-2 flak suppression: squadron + stores | 3 | ✅ `port/flak_suppression.sh` |
| S171-3 whatever is blocking them | 2 | ✅ the hosted-registry leak, and three things behind it |

## The finding

**Closing a campaign dialog leaked its entire control set into the hosted registry, still flagged
visible.** `RDialog::EndDialog` tears down a *subtree* of dialogs; compat's `CWnd::DestroyWindow`
deregisters exactly *one* window's hosted controls. So after closing the Profile dialog and
reopening it there were **two live `CProfile`s and two live `CFlt_Task`s**, both visible, and
`ma_ole_control_point_p` took whichever sorted first **by pointer** — the dead one.

The symptom was absurd and perfectly reproducible: `#2149@CFlt_Task` opened the attack-pattern
dropdown, and the very next entry, `#2149@CFlt_Task:r1`, replied *"needs its dropdown OPEN first"*.
Two clicks naming one control were reaching two different controls.

Fixed with the walk **S169** already built for `ma_ole_set_parent_scoped` — `fchild`, `dchild` **and**
`sibling`, deliberately wider than the paint recursion, for the same reason: a node the paint walk
never visits is still hosted. `MA_NO_SUBTREE_REMOVE=1` reverts.

## Three things were hiding behind it

**1. `@Class` stops disambiguating when a dialog is ambiguous with itself.** S85 added the qualifier
*and* an ambiguity warning, but the warning only ran when **no** class was given — on the assumption
that a class name settles it. A dialog closed and reopened is ambiguous with its own corpse. The
count now runs after the *same* filters the resolver uses, class included, and prints each
candidate's parent pointer. The `UNRESOLVED` dump names the parent class too: it used to list N
identical anonymous candidates while telling you to add a qualifier you had no way to choose.

**2. The separate `Load` click never did anything, in any recipe, ever.**
`CLoad::OnSelectRlistboxfile` calls `OnOK()` when the clicked row is *already* the current one, and
`currrow` starts at **0** — so `:r0` selects "Minimum Strike" **and loads it**, destroying the
chooser in the same click. Every recipe since S162 then clicked `#1056@CLoad`, a button on a
destroyed dialog. It resolved because the dead dialog's controls were still registered.

`authorize_mission.sh` even carried a comment rationalising the silence: *"the Load button's own
'-> fire' trace line is not reliably flushed before the capture, so the mission folder appearing is
what proves Load ran."* The trace was not unflushed. **It was never emitted.** The gate's instinct —
assert on the outcome — was right, and it is what kept the gate honest for nine sprints; the
explanation attached to it was wrong. *A missing trace line deserves an explanation you have
checked, not one that merely accounts for it.*

**3. A recipe entry that can never resolve holds every entry behind it.** Fixing the leak turned
that silent no-op into a hang, which is how it was found: the run stalled forever on a `#1056@CLoad`
whose dialog no longer existed, and the log filled with one identical `UNRESOLVED` line per idle —
indistinguishable from the legitimate "the control is not up yet" wait the hold exists for.
`[clickseq] STALLED` now says so once, loudly, naming the entry, after 240 idles.

## ⭐ The gate reported PASS on a run that segfaulted

`flak_suppression.sh` printed every assertion green and `PASS`, on a run that died with
`SIGSEGV`. Every assertion was **true** — the evidence was all in the log before the crash — and the
gate never looked at how the run ended. Auditing the others: only `oob_sweep.sh` checked exit
status, and that is the one gate whose entire job is counting crashes. Three more (`help_click`,
`map_icon_click`, `sysbox_exit`) checked `$?`, which a crash on a worker thread need not disturb.

New `port/gate_lib.sh` — `assert_no_crash` (reads the binary's own `=== CRASH: signal` banner, which
is authoritative where the exit status is not, and **symbolises the top frames**, because an address
list is not a diagnosis) and `assert_recipe_ran` (STALLED / AMBIGUOUS). Wired into nine gates.

**A gate that cannot fail on a crash is not a gate; it is a log grep.**

## The crash was S170's, latent since it shipped

```
=== CRASH: signal 11 fault_addr=0x8 ===
  CRSpinButCtrl::GetCurrentText(char*)
  CRSpinButCtrl::OnDraw(CDC*, CRect const&, CRect const&)
  ma_spin_draw
```

`GetCurrentText` does `m_list.GetAt(m_list.FindIndex(m_index))`. `FindIndex` on an **empty** list
returns NULL and `GetAt` dereferences it. The line directly above it is
`ASSERT(m_list.GetCount()); // have at least one entry!` — the authors knew, and `NDEBUG` compiles
the assert out.

On Windows this was unreachable: a real OCX is not drawn before its container fills it. Here the
global pass paints **every** hosted control **every** idle, so a dialog that creates a spinner and
populates it a moment later gets one fatal frame. `ma_spin_draw` now honours the precondition the
control documents and does not check — this is not a workaround for a port bug, it is the contract.

The culprit is **`WPDetail`**'s ETA spinner — named as a residual in S170 (*"hosted too but never
driven; it is on the K8 route"*) and reached here by accident. **Hosting a control type makes every
instance of it live at once, including the ones no story has driven yet.**

## Results

`port/attack_pattern.sh` (K6):

```
  attack pattern, each time the dialog filled it:
    1:Individual targets      <- default
    3:Spaced target selection <- changed, then closed and reopened
    1:Individual targets      <- changed back, closed and reopened
  attack method, each time: Dive Bomb Dive Bomb Dive Bomb
```

⚠ **Named divergence:** the port's pattern is **already** "Individual targets" when the dialog first
opens (the Minimum Strike profile sets `attpattern=2`), so step 9 has no distance to travel here.
Gold only ever shows the post-change state, so the default is **not** claimed wrong. The gate proves
the mechanism instead, and ends in the state the script asks for.

`port/flak_suppression.sh` (K7):

```
  wave table cell selected: row=1 col=3 (AAA Cover)
  ChooseSquad's own Available column for F84: 0
  the flak slot started Off Duty: yes
  the flak slot now holds: F80 (1/1)
  the unavailable squadron (F84) was refused: yes
  stores now read: Rockets & Fuel tanks
  Mission Folder Flights for "Wonju": 2 2 3 3
```

⚠ The script says pick **F84**. ChooseSquad's own **Available** column reads `F84: 0` on this save's
date, and `OnSelectRlistboxctrl1` refuses any squadron with `numavail < 4`. So the gate asserts the
**refusal** — a squadron the game says is unavailable must not become assignable — rather than
quietly assigning something else and calling it step 11. Same divergence class as **K4**'s recorded
F84/F80 note.

## Residual

- *"the suppression flight appears in the frag"* is **K9**, not asserted here.
- The port's default attack pattern vs gold's is **unresolved, not dismissed**: it needs a gold frame
  of the TASKS dialog *before* step 9, which the recording does not contain.
- `assert_no_crash` is wired into nine gates. `dialog_scroll`, `map_filter`, `panel_click` and
  `parity_2d` structure their runs differently and were left alone — **they are still gates that
  cannot fail on a crash**, and that should be finished rather than forgotten.
