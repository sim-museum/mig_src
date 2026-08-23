# Sprint 177 — "A gate that cannot tell its preconditions from its subject" — ✅ CLOSED 2026-08-22 (goal MET, 8/8)

**Planned 2026-08-22** (PO ceremonies pre-approved). The PO: *"run the gates and fix
damage_elements."*

| Story | Pts | Result |
|---|---|---|
| S177-1 reproduce the failure | 1 | ⭐ it does not reproduce — **PASSES standalone** |
| S177-2 find what actually happened | 2 | ✅ a stray `wmig` from a SIGKILLed suite |
| S177-3 make it impossible to misreport | 3 | ✅ `assert_clean_start`, and the runner cleans between gates |
| S177-4 full suite, clean | 2 | ✅ see below |

## It was never a regression

`damage_elements` reported:

```
  the tab bar never took a click — FAIL
  the combo never opened — FAIL
  dropdown row 0 was never selected — FAIL
```

in a partial suite run, and **passed standalone** minutes later, unchanged:

```
  Damage tab took the click: yes
  combo id=2398 open dropdown (2 items)
  dropdown row 0 selected: yes
  PASS
```

The suite had been `kill -9`'d **twice** that session to hand the display to the PO for joystick
testing. That leaves a **stray `wmig`** — the gate wrapper dies, the game it launched does not. The
next gate then started against a run directory another process was already driving, its clicks went
nowhere, and it reported that as *"the tab bar is broken"*.

**An environment problem reported as a content failure.** That is the same family as S171's *PASS on
a crashed run*: a gate that cannot tell its own preconditions apart from its subject. S171 taught
these gates to assert how a run **ended**; this one teaches them to assert how it **began**.

## The guard refuses; it does not clean up

`assert_clean_start` exits if any `wmig` is alive, and deliberately **does not kill it**:

> A stray wmig may be the user's own game on the display, and a gate is never entitled to close it.

That matters concretely today — the PO was flying while gates were queued. A tidy-up-and-proceed
guard would have killed their session mid-flight to run a regression test.

It exits **2**, not 1, so a suite can distinguish *could not run* from *failed*. The old output had
no way to say the difference, which is precisely why a blocked gate looked like a broken feature.

The suite runner additionally clears strays **between** gates, so one killed gate can no longer
poison its successor.

## The bit I should have done first

The failure appeared one step after a plausible culprit — S176 had just changed DirectInput axis
enumeration — and my first instinct was to look for how a joystick change could break a 2D dossier
screen. It cannot, and the check that says so is **one command**: run the gate on its own.

**Reproduce in isolation before reading any diff.** Proximity to a change is not evidence, and a
recent diff is the most available explanation rather than the most likely one. Cheap, and it would
have saved the detour.

## Full suite, clean start

Run from a clean start with the guard in place: **16/16, every gate exit 0, no failures.**

```
parity_2d          PASS  5 screens byte-identical to the committed references
oob_sweep          OPEN=9  NONE=0  CRASH=0
authorize_mission  PASS   damage_elements  PASS   dialog_scroll  PASS
map_filter         PASS   help_click       PASS   sysbox_exit    PASS
map_icon_click     PASS   recon_photo      PASS   map_drag       PASS
add_flight         PASS (K step 8)         attack_pattern PASS (K step 9)
flak_suppression   PASS (K step 11)        route_drag     PASS (K step 13)
frag_review        PASS (K step 14)
```

`parity_2d` byte-identical matters here specifically: **prefs-Controls is one of the reference
screens**, and S176 changed DirectInput axis enumeration. If the canonical reordering had shifted
what that screen draws, this is where it would have shown up. It did not.
