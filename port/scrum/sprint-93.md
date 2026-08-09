# Sprint 93 — "Make the key arrive" — ✅ CLOSED 2026-08-09 — ⭐ headless key injection was dead in exactly the mode it exists for

**Planned 2026-08-09 (PO pre-approved ceremonies; continuous-sprint directive). Autonomous. ~7 pts.**
**Sprint Goal:** fix the blocker S92 named — `BOB_KEYSEQ` taps not reaching the game — because both
C4 verification and B7 steering depend on it.

| Story | Pts | Result |
|---|---|---|
| S93-1 fix key injection | 5 | ✅ fixed and proven end to end |
| S93-2 gates | 2 | ✅ |

## Execution log

### The bug — one line, and it invalidated an earlier conclusion
`pump_events()` began with:
```c
static void pump_events(void)
{
    if (!g_win) return;          // <-- bails out before the synthetic-input hooks
    ... BOB_AUTOFLY ...
    ... BOB_KEYSEQ ...
    SDL_Event e; while (SDL_PollEvent(&e)) ...
```
Under `SDL_VIDEODRIVER=dummy`, `SDL_CreateWindow` **fails** (no GL in the dummy driver — the boot
log has said so all along: *"[vid] SDL_CreateWindow failed (won't retry)"*), so `g_win` stays NULL
and the function returned immediately. **`BOB_KEYSEQ` and `BOB_AUTOFLY` were therefore dead in
headless mode — precisely the mode they exist to serve.** Nothing errored; the taps simply never
happened.

**Fix:** the synthetic-input hooks run *before* the window check; only real SDL event polling needs
a window. Both hooks merely push to the DIK queue and never touch the window, so this is safe.

**Proven end to end**, which the old code could not show at all:
```
[keyseq] tap dik=0x3b at kidle=250
[key] DOWN scancode=0x3b shift=0 -> action index=132     <- engine dispatched it (ENEMYVIEW)
```

### ⚠ Correction to Sprint 91 — B7's "third negative" was not evidence
S91 recorded a B7 attempt in which 60 `ELEVATOR_FORWARD` taps were injected to dive the aircraft
and bring close ground objects into the radar cone, and concluded from the unchanged result that
*"the problem is what is near the aircraft, not how it is flown"*. **That dive never happened** —
the taps were discarded by this bug. The conclusion was drawn from a test that did not run, and it
is withdrawn: B7's scenario question is **re-opened**, not settled. S92's failed padlock
verification has the same cause.

*The tell was there and was misread: no `[keyseq]` trace appeared in either run. I read the absence
as "the tap had no effect" rather than "the tap never fired" — the same "no output means the code
never runs" trap this project has now booked three times (§8-MA83, S64→S65, and here). The
difference between a silent no-op and an absent one is the whole diagnosis.*

### Still open, honestly
Even with the key arriving, the padlock did not engage: action 132 dispatches but `trackeditem2`
stays NULL, because `CheckPadlock(currentenemyitem)` needs an enemy actually selected. So C4c/C4d
remain unverified — but the blocker has moved from "the harness is broken" to "the scenario has no
enemy in view", which is the same wall B7 is at and now clearly one problem, not two.

## Gates — all under `gl-lock`
- **2D parity: 5/5 byte-identical. Stress: 20/20 PASS. ASan: 0 reports, 4/4 paths 2/2.**

## Result
A one-line ordering bug had silently disabled every headless input hook. Two sprints' worth of
in-flight conclusions rested on tests that never ran; one of them (S91's) is now withdrawn. The
capability that fix restores — driving the game's own keys headlessly — is what C4, B7 and any
future in-flight verification all depend on.
