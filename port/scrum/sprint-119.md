# Sprint 119 — "What the PO found in ten minutes" (PO play-test of S118) — ✅ CLOSED 2026-08-15 (goal MET)

**Unplanned, PO-driven: the product owner ran the shipped hardware option under a debugger and it
failed immediately. Autonomous. ~8 pts.**
**Sprint Goal:** fix what play-testing found, and understand why four green sprints missed it.

| Story | Pts | Result |
|---|---|---|
| S119-1 SIGSEGV entering 3D flight on hardware | 3 | ✅ the flip chain was a stub that reported success |
| S119-2 campaign screens stale, Fly leaves a blank window | 3 | ✅ hardware had silently gone fullscreen |
| S119-3 1920×1080 renders into a corner | 2 | ✅ viewport sized from a value the 2D canvas overwrites |

## ⭐ Why the gates were green and the game was broken

S118 made the hardware driver *properly visible* — `fNoHardwareAtAll=false`,
`fFirstHardIsPrimary=true`, `dddriver=-1`. That is exactly the condition at `Win3d.cpp:1826`, so
the engine started **requesting fullscreen**, which selects the two-surface flip-chain path
(`Hardwin.cpp` case 2). **Every hardware sprint before this ran windowed.** S118 shipped the option
and, in the same change, moved the renderer onto code no test had ever executed.

The gates could not have caught it: they pin `MA_NO_HARDWARE=1`, which withdraws the device
entirely — **a configuration no player will ever have**. A gate that pins away the feature under
test is not testing the feature. That is the sprint's real lesson, and it is mine, not the PO's.

## The three faults

**1. `GetAttachedSurface` — a stub that returned `DD_OK` and wrote NULL.**

```c
HRESULT GetAttachedSurface(LPDDSCAPS, LPDIRECTDRAWSURFACE* s) { if(s)*s=0; return DD_OK; }
```

The fullscreen path asks the primary for its back buffer and Locks the answer unconditionally
(`Hardwin.cpp:1110`), so a successful NULL is a SIGSEGV one call later. Now a real flip chain.
**Only a primary owns one**: the first cut handed a back buffer to every surface, and callers WALK
the chain (`while (lpDDS) { …; lpDDS->GetAttachedSurface(&caps,&lpDDS); }`), so it allocated until
`calloc` failed — one crash traded for another. The terminator is the fix.

**2. The port is windowed; it must stay windowed.** A single SDL window, sized to the chosen mode,
with GL scaling it — a DirectDraw fullscreen mode buys nothing and costs the whole half-built
flip-chain path. `isFullScreen()` now returns false under `MA_LINUX`
(`MA_ALLOW_FULLSCREEN=1` restores the engine's answer). That fixed the campaign screens not
repainting *and* Fly leaving a blank window: the stacks showed no 3D draw thread at all, with the
main thread parked in the front-end pump.

**3. The 3D scene was sized from `g_scrW/g_scrH`**, which track whichever caller last touched
`ma_ddraw_ensure_window` — and the 2D canvas keeps calling it with its own 800×600 during a
1920×1080 flight (`MA_TRACE_RES`: 640×480, 800×600, 1920×1080, 800×600, 1920×1080). Whenever
800×600 landed last, the frame rendered into a corner of the window. Now sized from the real
drawable.

**And the frame dump had the same bug**, which is why I reported 1920×1080 as working: it read
`g_scrW/g_scrH` too, so it captured the wrong rectangle and hid exactly what the PO could see on
screen. **A capture that shares a bug with the code under test is not evidence.**

## Also fixed here

`IDirect3DTexture::Load` now carries the **palette** across, not just the texels. 8-bit texels are
indices and mean nothing without one; the engine sets the palette on the system surface it writes
through and the renderer binds the destination. (This did *not* fix the terrain — see S120 — but it
is a real defect on its own.)

## Corrections issued to the PO during the sprint

- "Terrain is fixed" — it was not. I measured a Hot Shot start at 17,000 ft where the ground is
  haze, and read *not black* as *fixed*. The PO's own repro (quick mission, low altitude) showed
  software brown and hardware black side by side.
- "1920×1080 renders correctly" — the capture was reading the wrong buffer size.
- An earlier "restored settings to software" silently did nothing: the file is
  `SaveGame/settings.mig`, I `cp`'d a path that does not exist, and swallowed the error with
  `2>/dev/null`.

## Gates

parity 5/5 byte-identical · sweep 9 OPEN/0 CRASH · map click · map drag · sysbox · help click ·
stress 20/20 software **and** hardware.
