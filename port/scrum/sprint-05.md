# Sprint 5 — "Fly the mission + come back" (R2)

**Goal:** connect the working Quick-Mission front-end to working 3D flight as a one-process
round-trip — click Fly/Hot Shot → 3D flight → exit → back to the front-end (debrief/menu) — so a
Quick Mission is actually *playable*. (PO standing pre-approval; dev runs the cadence autonomously.)

## Backlog (this sprint)
- **M1 Menu↔flight round-trip** (the R2 playable gate) — DONE
- (M2 3D fidelity vs Wine — next sprint)

## M1 — DONE

### What shipped
The full **menu → 3D flight → exit → front-end** round-trip runs in one process, no crash:
title → Single Player → Hot Shot (or Quick Mission → Fly) → real software-rasterized 3D flight →
exit (Alt+X / the engine's `OverLay.quit3d`) → game's own teardown + `OnFlyingClosed` →
`LaunchScreen` back to the front-end (Quick-Mission debrief). **3D flight is now default-on** (no
`MA_ENABLE_3D` needed); `MA_DISABLE_3D=1` keeps the flight path 2D-only for front-end debugging.

Validated: 3D-launch stress **6/6**; round-trip **4/4** clean (exit 0, `InThe3D=0`, the post-flight
debrief screen renders 100% non-black). No regression on the title/front-end path.

### The recipe (mirrors BoB's, adapted to MA)
On Win32, `View3d::CloseWindow` posts `WM_COMMAND(IDOK|IDCANCEL)` to the flight dialog and the pump
routes it to `Rtestsh1::OnOK/OnCancel` + the parent panel's `OnFlyingClosed`. Our compat
`PostMessage` is a no-op (the message is swallowed), and `CloseWindow` runs on the View3d **draw
thread** — tearing down the view/inst there would race the very thread doing it. So:
- **`View3d::CloseWindow`** (STUB3D.CPP, MA_LINUX): instead of the dead PostMessage, call
  `ma_capture_flight_close(id)` to stash the id.
- **`ma_process_flight_close()`** (MIG.CPP): drained each idle on the **main thread** —
  `Rtestsh1::THISTHIS->OnOK()/OnCancel()` runs the game's own teardown (`Paused`, delete
  View3d/Inst3d; `~View3d` joins the draw thread via `WaitEndDraw`, so it's race-free). Unlike BoB,
  MA's `RDialog::EndDialog → DialExitFix → ChildDialClosed → localnote` chain fires `OnFlyingClosed`
  **naturally** inside OnOK/OnCancel → `LaunchScreen` back to the menu, so we must NOT call
  `OnFlyingClosed` again (double-navigate). After teardown we NULL `Rtestsh1::THISTHIS` to disarm the
  idle `MaDriveLaunch` re-launch on the stale (un-destructed) dialog.
- **Test hooks:** `ma_request_flight_exit()` (raises `OverLay.quit3d` → the engine's own faithful
  exit path), driven by `BOB_AUTOEXIT=N` (request exit after N sustained 3D frames). `MA_TRACE_3D`
  traces the close.

### Root-cause bug fixed along the way (the real win)
Returning to the debrief crashed in `stbtt ttSHORT` (the GDI font) — a hardware watchpoint on
`g_ttfState` pinned the wild writer to **`TileMake::SetTexturePointers()`** (software-landscape
3D init). It builds a 256-entry 8→16bpp land LUT at `GetPaletteTable()-256` — i.e. the 256 UWords
*before* the returned pointer. `XASM_GetPaletteTable` returns `&palette_buffer`, the **first symbol**
in ma_xasm's `.bss`, so `-256` underflowed into the previous TU's `.bss` and stomped ma_gdi's
stb_truetype state. The 3D render itself looked fine (the LUT write+read both used the same `-256`
slot) — it just collided with the font, surfacing only when the front-end next drew text after
flight. **Fix:** reserve `landlut: resw 256` headroom before `palette_buffer` in `ma_xasm.nasm` so
both the write and read stay in our own `.bss`. (Latent corruption that was always happening in
flight; now contained.)

### Files
- `SRC/MFC/STUB3D.CPP` — `View3d::CloseWindow` capture hook + `ma_request_flight_exit`.
- `SRC/MFC/MIG.CPP` — `ma_capture_flight_close`/`ma_process_flight_close` + idle drain;
  `BOB_AUTOEXIT` test hook; 3D launch **default-on** (`!MA_DISABLE_3D`).
- `SRC/GRAPHICS/ma_xasm.nasm` — `landlut` headroom before `palette_buffer` (the `.bss` overrun fix).

## Increment / Review (PO standing-accept)
**Demoable (native, no env vars):** a real **playable Quick Mission** — click into 3D flight, fly,
exit, land back on the debrief screen, all in one native process. R2 (flyable + return) met.

## Retro
- **Went well:** the hardware-watchpoint-on-the-corrupted-static approach found the `SetTexturePointers`
  `.bss` overrun in two gdb runs — far faster than guessing. The round-trip mechanism worked first try
  once the thread ownership (capture on draw thread, drain on main) was right.
- **Learned:** MA's RDialog close machinery is MORE wired than BoB's — OnOK/OnCancel already drive
  OnFlyingClosed via DialExitFix, so the BoB "also call OnFlyingClosed" step double-navigates here.
- **Carry:** MFC-fragment edits (MIG/STUB3D) still need a manual `.o` into `objmfc/`; rebuild.sh
  skips them when `/tmp/*_ok.txt` is absent.

## ➡ Sprint 6 (next)
- **M2** 3D fidelity A/B vs Wine (terrain/models/cockpit/HUD/palette) — the dominant remaining chunk.
- Begin **M3** audio (Miles/DirectSound → OpenAL) in parallel.
