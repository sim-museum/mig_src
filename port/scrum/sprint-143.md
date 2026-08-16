# Sprint 143 — "Our references were wrong on four screens" (PO-35) — ✅ CLOSED 2026-08-16 (goal MET) — ⭐ the front end matches gold

**Planned 2026-08-16, continuing S142's gold-comparison method.**
**Sprint Goal:** remove the black box the port paints behind front-end lists, if gold says so.

| Story | Pts | Result |
|---|---|---|
| S143-1 re-test S70's "it erases the menu" | 2 | ✅ no longer true |
| S143-2 check every front-end screen against gold | 3 | ⭐ four screens were wrong |
| S143-3 rebase the references | 3 | ✅ parity 5/5, suite green |

## The defect

`CRListBoxCtrl::OnDraw` fills its box when `!artnum` — *"my parent gave me no artwork, so I must
paint my own background"*. On Windows `artnum` is the panel's art and non-zero, so the branch
never runs. The port's `RDialog::OnRowanMessage` answers `WM_GETARTWORK` with **0** deliberately
(the real artnum sends hosted controls down an offscreen-compositing path that renders all black),
so the branch ran on **every front-end listbox**, painting an opaque black box the original never
paints. One workaround becoming another screen's defect.

## What gold says — on four screens, not one

| screen | gold | ours (before) |
|---|---|---|
| title menu | yellow text directly on the artwork | a black rectangle behind part of it |
| **Preferences tab strip** | sky and a blue gradient show through the tabs | a **solid black band** across the top |
| Preferences (Others) | same | same band |
| Quick Mission | `BACK VARIANTS FLY` over the cockpit photo | a black band behind it |

The prefs strip is the one that matters most: it is 800×23 of solid black across the top of the
screen the PO has been using all week to switch renderers, and it has been there since the
Preferences UI was first brought up.

S70 recorded that skipping this fill *"erased the title menu"*. Re-tested this sprint: **no longer
true.** The menu renders correctly without it — something between then and now fixed whatever made
it vanish, and the workaround outlived its cause. Worth remembering: a note that records an
empirical finding should be re-tested before it is treated as a constraint, because the code it
described has moved.

## The references had to be rebased, and that is the real lesson

Four of the five 2D parity references were captured from this port **with the black box in them**,
so the gate has been asserting the defect is present, byte for byte, for over eighty sprints. It
could never have found this. Only a comparison against the **gold captures** could, which is
S141's finding put to work:

> *a capture that shares a bug with the code under test is not evidence.*

`port/ref/gold/prefs_tab_gold.png` and `port/ref/gold/title_menu_gold.png` are now committed
beside our own, so the next person can see what the target actually is.

## Gates

parity 5/5 byte-identical **against rebased references** · sweep 9 OPEN/0 CRASH · map icon click ·
help click · dialog scroll · panel click.
