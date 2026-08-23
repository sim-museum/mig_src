# MA scaffold audit — which capabilities have only ever worked under a test harness?

*Started S192 (2026-08-23), after PO-55 and BoB's S205.*

## Why this file exists

Two defects in one day, in two different ports, with the same shape:

- **PO-55 (MA).** The PO could not drag a route waypoint. No waypoint had *ever* been draggable:
  the map received clicks only, as press+release fused into a single tick, and any release more
  than 4px from its press was discarded. `route_drag.sh` was green throughout, because it calls
  `CMapDlg::MaDriveDrag` — a **test-only entry point** that invokes `OnMouseMove` directly.
- **S205 (BoB).** The PO had no German missions. The campaign day never began, because the map
  clock was driven only while un-paused and the map starts paused. Every gate was green because
  **every gate sets `BOB_MAP_TIMER`**, a scaffold that drives the clock itself.

> **A capability that only works when a test harness is present is not a capability.**

The question this file asks of every capability is not "is it gated?" but **"does the gate drive
the path a player uses, or a shortcut into the middle of it?"**

## Finding 1 — 17 of 19 gates never pump an SDL event

Gates run under `SDL_VIDEODRIVER=dummy`, where `SDL_CreateWindow` fails, `g_win` stays NULL and
`pump_events` returns early. **No SDL event is processed at all.** Input arrives instead through
`BOB_CLICKSEQ`, which injects a resolved click directly into the front-end tick.

| exercises the real SDL input layer | does not |
|---|---|
| `panel_click`, `route_drag_real` | the other 17 |

That is not an argument for converting them: headless gates are fast, deterministic, and catch
plenty. It *is* an argument that **every distinct kind of player input needs at least one real-SDL
gate**, because the layers between SDL and the game — event decode, `win_to_canvas` scaling, button
state, the press/move/release edges — are invisible to all the others. Today that means:

| input kind | real-SDL coverage |
|---|---|
| a click on a front-end menu | ✅ `panel_click` |
| a drag on the map | ✅ `route_drag_real` (S190) |
| a click on a hosted OCX control (button, combo, listbox, spin) | ❌ **none** |
| a keypress in flight | ❌ **none** — `BOB_KEYSEQ` pushes to the DIK queue directly |
| the joystick | ❌ **none** — PO-verified by hand only |

The keypress row is not hypothetical: **PO-60** was exactly that defect (SDL delivers keys only to
a focused window; the resize-for-3D handed focus away), it survived two sprints being mistaken for
a brake bug, and no gate could have seen it.

## Finding 2 — test-only entry points in game code

`grep MaDrive SRC/` — each one is a door into the middle of a flow:

| entry point | what it bypasses | real-path gate? |
|---|---|---|
| `MaDriveClick` | SDL, the click edge | ✅ `panel_click` covers the equivalent |
| `MaDriveDrag` | SDL, `pump_events`, the drag stream, the map tick's dispatch | ✅ `route_drag_real` (S190) |
| `MaDriveDown/Move/Up` | *(these ARE the real path — driven from the map tick)* | ✅ |
| `MaDriveLaunch` | the Fly button flow | ❌ **not audited yet** |

## How to use this

When adding a gate, write down which layer it enters at. When a PO reports something a gate
"covers", **check the entry point before checking the code** — the gate may be proving a
different sentence than the one you need.
