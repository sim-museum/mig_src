# Sprint 173 — "Three sub-dialogs, one set of control ids" (K9) — ✅ CLOSED 2026-08-22 (goal MET, 8/8)

**Planned 2026-08-22** (PO ceremonies pre-approved). Continuing EPIC K in script order; K9 was the
one item S168 *reached* but declined to give a verdict on.

| Story | Pts | Result |
|---|---|---|
| S173-1 address one of N identical sub-dialogs | 2 | ✅ `@Class#N`, ordered by screen position |
| S173-2 callsign and aircraft must reach game state | 2 | ✅ `MA_TRACE_FRAG` on both writes |
| S173-3 gate step 14 | 1 | ✅ `port/frag_review.sh` |

## The finding

The frag screen hosts **three `CFragPilot` sub-dialogs — one per package — with identical control
ids**. `@CFragPilot` is therefore ambiguous *with itself*, in exactly the way a closed-and-reopened
dialog was in S171.

**S171's ambiguity warning caught it on the very first run:**

```
[clickid] WARNING id=2356 is AMBIGUOUS (3 visible hosts matching @CFragPilot) — the recipe cannot say which:
[clickid]   candidate host=10CFragPilot(0xb76b490) type=4 rect(0,27 277x26)
[clickid]   candidate host=10CFragPilot(0xb773d90) type=4 rect(0,27 277x26)
[clickid]   candidate host=10CFragPilot(0xb87f150) type=4 rect(0,27 277x26)
```

Without it, the recipe would have driven whichever host sorted first **by pointer** and the gate
would have passed for whichever row that happened to be. That warning was written two sprints ago to
diagnose a different bug; it paid for itself here on a bug it was not written for.

`@Class#N` names the Nth instance **by screen position** — top-to-bottom, then left-to-right — not by
map order. Map order is by pointer, i.e. by whatever the allocator did; *"the second flight row"* has
to mean the one the player sees second, or the recipe is addressing luck again (the S95 rule, one
level up: ask the screen, not the heap). Verified:

```
@CFragPilot#0 of 3 -> client=0xb460b50 at (0,34)
@CFragPilot#1 of 3 -> client=0xada3c20 at (0,145)
@CFragPilot#2 of 3 -> client=0x9a63090 at (0,256)
```

Evenly spaced, the three rows. The instance travels **inside the class string**, so every existing
caller, recipe form and gate is untouched.

## Every clause reads game state, not pixels

- **Callsign** → `Todays_Packages.pack[p][w][g].callname`, traced where `FindCallName` writes it. A
  combo can repaint a new caption without the write landing, and that is precisely what a gate must
  be able to fail on.
- **Aircraft** → `MMC.playeracnum`, the seat the player flies, and the gate checks it equals
  `flight*4 + slot`. This matters more than it looks: `CFragPilot::OnClickedPlayer` **refuses** a
  dead pilot's slot and a slot already taken by another comms player. A click that legitimately does
  nothing is indistinguishable from a broken one unless the write itself is traced.
- **Review** → the pilot roster, from the game's own names.

```
  FlyableAircraftAvailable=1  pack[0][0][0].uid=4096  incomms=0
  pilot roster: 12 distinct name(s) — Allen McElroy,Arnold Eagleston,Charles A. Mitchell...
  callname pack[1][0][0] <- 1 (combo row 0) " Rattler "
  callname pack[1][0][0] <- 5 (combo row 4) " Red "
  the callsign changed in the package: yes (1 -> 5)
  player seat <- squadron=2 acnum=4 (flight 1, ac 0 in flight)
  the seat matches the slot clicked (flight 1, ac 0 -> acnum 4): yes
  the player is no longer in the default lead seat: yes
```

## A correction to the story, not to the code

K9's acceptance said *"Callsign edit accepts text (cf. PO-16)"*. The callsign control is a **combo**,
not an edit: `CFragPilot::FillComboBox` fills `IDC_FRAG_CALLNAME` from the game's callsign string
table (Amber, Blue, … Rattler, Red …), filtered by `UniqueCallNameOrThisGroups` so two groups cannot
hold the same one, and the player **picks**. There is no text entry on this path, so **PO-16 is not
what K9 depends on** and remains open on its own terms.

*The story was written from the PO's script — "change callsign" — and a written step does not say
which widget the game uses. Check the control before inheriting the assumption into an acceptance
criterion.*

## PO-37

Unchanged, and it affects no clause in this gate. S168 declined a K9 verdict while **PO-51** (map
dialogs painted over the frag panel — fixed S169) and **PO-37** (the panel occupies 800×600 of a
1920×1080 canvas) both stood. PO-51 was the one that actually obscured the evidence. PO-37 is a
layout decision that needs a target variant chosen and all five parity screens re-verified
afterwards; it does not stop the callsign, the seat or the roster from being correct, and this gate
asserts none of them by position.
