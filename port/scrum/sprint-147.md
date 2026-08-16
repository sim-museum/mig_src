# Sprint 147 — "Nothing corrupted" (verification) — ✅ CLOSED 2026-08-16

**Planned 2026-08-16, after thirteen sprints of change in one night.**
**Sprint Goal:** prove the night's work did not corrupt memory, and record what PO-37 needs.

| Story | Pts | Result |
|---|---|---|
| S147-1 ASan over the campaign path | 3 | ✅ zero heap errors |
| S147-2 ASan over a flight | 2 | ❌ NOT ACHIEVED — the ASan build never reaches 3D in time |
| S147-3 what PO-37 actually needs measured | 3 | ✅ recorded; two opposite candidate fixes |

## Why this sprint exists

S134–S146 changed a lot of lifetime-sensitive code in one night: a nested modal loop, a control
registry gaining two new control types, and — most dangerous of all — **S146 started destroying
objects that had previously been leaked**. Freeing something the rest of the program still points
at is the classic way to turn a cosmetic fix into a crash three screens later, and it would not
necessarily show up in a gate.

So: the AddressSanitizer oracle over the whole campaign path — map → D.I.S. → Intelligence →
system box → quit confirmation → title.

## Result

```
76 odr-violation
 0 heap-buffer-overflow
 0 use-after-free
 0 double-free
 0 stack-buffer-overflow
```

The ODR violations are inherent to this port's link arrangement — unity builds plus
`--allow-multiple-definition` — and are not memory errors. **No heap corruption of any kind on the
campaign path**, including where it now tears down panels that used to leak.

That is the result I most wanted from tonight, because S146's change is the one whose failure mode
is silent and delayed.

## What this does NOT cover — corrected

I first recorded this sprint as also clearing **a flight**. It does not, and the claim was wrong
when I wrote it. Checked afterwards: the "flight" run produced one `[3d]` line —
`EnumDisplayModes`, which the front end does — and never launched. The ASan build is roughly an
order of magnitude slower, so the flight had not started before the run was cut off; a second
attempt at 1500s got no further. **The 3D path is unverified under ASan**, and S145 (the map view
now sized from the canvas) is the change that most deserves that check.

Recorded rather than quietly dropped, because "we ran ASan" is exactly the kind of claim that
gets believed later. The next session should run the ASan flight with no wall-clock ceiling.

## PO-37 (title screen at 1920) — what to measure first

Two candidate fixes, and they are opposites:

- **(a)** switch the CANVAS to the panel's native size while a full panel is up, and back to the
  display size on the map — the GL present already scales a canvas to the window;
- **(b)** STRETCH the panel background art to the canvas and leave the controls where they are.

(b) rests on S125's note that the full-res canvas is *"correct in layout (dialogs land at gold's
size and position)"*. **That claim needs checking before either fix is written:** at 1920 our
title menu sits at (629,285) while gold's is around x≈1050–1310. They do not obviously agree, and
choosing the wrong one means moving every control on every front-end screen in the wrong
direction. Measure our control positions against a gold frame at the same resolution first.

## Gates

Full suite green as of S146: parity 5/5 byte-identical · sweep 9 OPEN/0 CRASH · map icon click ·
map drag · map filter · sysbox exit · help click · dialog scroll · panel click · stress 10/10.
