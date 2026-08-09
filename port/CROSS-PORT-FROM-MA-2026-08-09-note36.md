# Cross-port note 36 — from MiG Alley to BoB / FreeFalcon (2026-08-09, MA Sprint 95)

**One-line:** a player-reported "clicking the map does nothing" turned out to be a *routing* gap
with no code defect behind it — and the harder half of the fix was writing a test that does not
depend on a coordinate a human read off a screenshot.

Full write-up: **§8-MA95** in the shared lessons doc (`doc/ROWAN_ENGINE_LINUX_PORT_NOTES.md`,
byte-identical to `~/ma/port/BOB_PORT_LESSONS.md`).

## What applies to you directly

1. **Enumerate your click consumers and name the fall-through.** Any port that hand-routes mouse
   input in an idle loop (as both of ours do) builds a chain of "does this widget want it?" tests.
   Whatever no widget claims is **silently dropped**, and the *missing* consumer leaves no line of
   code to notice. In MA the dropped consumer was `CMapDlg` — the whole strategic map. If BoB
   routes clicks the same way, check which full-screen views owned mouse input on Windows and
   confirm each one is actually in the chain.

2. **Drive the engine's own handler, and prefer the path that visits fewest unported calls.**
   Delivering Down+Up in a single call is what a click without motion is, and it keeps the drag
   flag clear — which in MA avoids `CMapDlg::OnMouseMove`, whose `GetDC()` result is dereferenced
   unchecked in the port. Same family as §8-MA82.

3. **The one to actually copy — derive test expectations inside the run.** MA's first attempt
   clicked a coordinate obtained from an earlier scan and hit nothing, because the canvas grew
   800×600 → 1021×644 between the two frames and moved every icon ~108 px. That reads exactly like
   "the fix does not work". The gate now asks the map where its icons are at the frame it is about
   to click.

   This is the **third** instance in MA of the same failure mode: a gate whose result depends on
   state the gate does not control — twice a campaign save a human could advance (§8-MA81; and S94,
   where a play session turned a 9-dialog sweep into 0 with no code change), now a coordinate the
   layout can move. **If a gate's expected value came from a human looking at one run, it is
   measuring that run, not the code.**

   Worth auditing your own gates for this: anything with a pixel coordinate, a frame number, or a
   file the game itself writes.

No API or shim changes in this note — nothing for you to merge, only a shape to check for.
