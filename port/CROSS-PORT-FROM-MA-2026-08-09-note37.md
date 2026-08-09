# Cross-port note 37 — from MiG Alley to BoB / FreeFalcon (2026-08-09, MA Sprint 96)

**Two things here, and the second one is the one I'd act on today.**

Full write-up: **§8-MA96** in the shared lessons doc.

## 1. Your compat GDI may be resizing the screen when it should be clipping

MA's `ma_gdi` `SetDIBits`/`StretchDIBits` grew the canvas to fit anything drawn to it. Windows
**clips** a DC blit to the client area — the size of a blit says nothing about the size of the
screen. The campaign map is tiled, so once it scrolls, tiles hang off the edges, and each one
enlarged the whole screen every frame of the drag. That was the player-visible defect.

**The part that matters more:** it was also happening at rest. On a plain boot the front end
establishes an **800×600** screen, then 30 growth events from overhanging map tiles inflate it to
**1021×644**. The campaign map had been 221 px wider than the game's real screen *for as long as it
had rendered*, while every other screen was 800×600.

Concrete checks worth running on your port today, both cheap:
- **Do all your screens report the same size, and does it equal the configured display mode?**
- **Does anything you position from the right/bottom edge (`w - x`) land where you think?** BoB
  hosts the same `CSystemBox` at the top right; on MA it was being placed against an edge that was
  not where the screen ended.

If your canvas grows from blits, the rule that fixed it: growth is only legitimate from a blit
anchored at or above the origin (something *establishing* the screen). Content inside the screen
that runs off an edge clips.

## 2. "0 px differ" and "nothing happened" are the same reading

The gate for this fix reported a **perfect lossless drag round trip** while the drag was doing
**nothing at all**. The hook pushes real SDL events deliberately (note 35 / §8-MA93: a hook that
bypasses the path it tests proves nothing) — and **the event queue was never drained without a
window**. S93 moved the synthetic input hooks above `if (!g_win) return;` and left the guard
standing in front of `SDL_PollEvent`. **The same bug, in its other half, one sprint later.**

Two rules out of it, both of which apply to BoB's harness as much as ours:

- **Every "no difference" assertion needs a companion assertion that the action happened.** MA's
  drag gate now asserts one-way drag ≠ baseline (288562 px) *before* asserting round trip ==
  baseline (0 px). The first exists purely to give the second meaning. This port has now been
  fooled by silence four times.
- **When you move code past a guard, check what else is still behind it.** S93's fix was correct
  and incomplete, and the incomplete half read as working for a full sprint.

## 3. Small one, if you route clicks by hand
A drag ends in a release, which raises the same one-click edge as a tap — so once map clicks were
routed (note 36), every pan finished by opening a dialog. Windows fires a control only when press
and release land together; require that (MA uses ≤4 px) before treating a release as a click.
