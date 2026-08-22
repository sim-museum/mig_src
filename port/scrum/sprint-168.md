# Sprint 168 — "Four eventsink maps were silently thrown away by the linker" (K9) — ✅ CLOSED 2026-08-22 (goal MET, 8/8) — ⭐ FRAG reaches the frag screen; the Wonju mission reports FLYABLE

**Planned 2026-08-22** (PO ceremonies pre-approved). The PO wants to run the mission, so this sprint
followed the shortest path to flying it: **Frag**.

| Story | Pts | Result |
|---|---|---|
| S168-1 which branch does Frag take? | 2 | ✅ `MA_TRACE_FRAG` — and the answer was "neither, it never got there" |
| S168-2 why does the handler not run? | 3 | ⭐ **the linker discarded four entire eventsink maps** |
| S168-3 fix, and see the frag screen | 3 | ✅ `FlyableAircraftAvailable=1`; the pilot roster and `Map Fly Preferences` render |

## The chain, and every step of it was a measurement

`Frag` printed `[tbclick] id=2126 … -> fire` and nothing happened. That trace is printed **before**
`ma_evt_fire`, so it means "about to try", not "handled" — a log line that actively reads like
success. First fix: **an unmatched dispatch now says so**, and lists what *is* registered for that id.

```
[evt_fire] NO HANDLER for id=2126 dispid=1 on type=14CMissionFolder
[evt_fire]   registered: id=1..9999 dispid=1 type=11CMapFilters
[evt_fire]   registered: id=1..9999 dispid=1 type=11CMapFilters     <- the same class, TWICE
[evt_fire]   registry holds 0 entr(y|ies) for 14CMissionFolder, 260 in total
```

Zero for `CMissionFolder`. `MA_TRACE_EVTREG=<class>` (filtered, not capped) confirmed it never
registers, with `CProfile` as the control — 12 entries, so the instrument works.

`CMissionFolder::MaRegEvents()` **is in the binary** and its TU's initialiser **is in
`.init_array`**. Disassembling that initialiser gave the answer:

```
call 8334172 <_ZN13MaEvtAuto_159C1Ev>
  → 8334178:  call 83341a2 <_ZN11CMapFilters11MaRegEventsEv>
```

**`MaEvtAuto_159`'s constructor calls `CMapFilters::MaRegEvents`.** The macro named its registrar by
`__LINE__` and defined the constructor **out of line**, so the symbol had external linkage — and
`MAPFLTRS.CPP` and `MISSFLDR.CPP` both have `BEGIN_EVENTSINK_MAP` on **line 159**. This port links
with `-Wl,--allow-multiple-definition`, so the linker kept the first and threw the second away, in
silence. The loser's entire sink map never registered; the winner registered twice — which is what
that duplicated `CMapFilters` line was telling me in the very first dump, before I read past it.

## How wide: measured, not estimated

68 translation units carry an eventsink map. **Four pairs collide:**

| line | classes | what dies |
|---|---|---|
| 126 | `SQDNLBUT` / `WPBUT` | **waypoint buttons — script step 13** |
| 130 | `LISTBX` / `WAVETABS` | **the wave tabs — script steps 8–12** |
| 159 | `MAPFLTRS` / `MISSFLDR` | **the Mission Folder: Intelligence, Profile, Delete, Frag** |
| 162 | `SERVICE` / `SESSION` | |

One macro fault, and it takes out most of the PO's walkthrough from step 8 onwards.

**The fix is the key, not the collision.** Name the registrar after the **class** — a class has
exactly one sink map, so the class name is the correct unique key — and define its constructor
**inside** the struct so it never reaches the external symbol table at all. Two belts, because this
failure was completely silent for the port's whole life.

## What it bought immediately

```
[evt_register] id=2121/2126/2124/2129/2018 type=14CMissionFolder     all five handlers
[frag] FlyableAircraftAvailable=1  pack[0][0][0].uid=4096  incomms=0
```

**The Wonju mission is flyable**, `OnClickedFrag2` takes the `LaunchFullPane(singlefrag)` branch, and
the capture shows the **frag screen**: the pilot roster (E. B. Best, Arnold Eagleston, John Fox,
Stanton G. Preston …), `F80 1 (08:12) Bomb`, and `Map Fly Preferences` along the bottom — the gold's
t≈305 frame. **Fly is on that bar.**

## Two defects the frag screen exposes, logged not fudged

- **PO-37 confirmed on a new screen** — the panel draws in a ~800×600 corner of the 1920×1080 canvas
  instead of filling it. Already on the board.
- **PO-51 (new)** — the campaign map's OOB dialogs are still painted **on top of** the frag panel.
  The map idle keeps running its paint walk after a full-screen pane has taken over. Sibling of
  S167: the walk needs to know when it does not own the screen — and note MA's own §8-MA104 told BoB
  to derive that from the game's state, never a port-side mirror.

## Gates

`parity_2d` 5/5 byte-identical · `oob_sweep` OPEN=9 NONE=0 CRASH=0 · `authorize_mission` PASS ·
`damage_elements` PASS · `dialog_scroll` PASS · `map_filter` PASS · `help_click` PASS ·
`sysbox_exit` PASS (99.1 %) · `map_icon_click` PASS · `recon_photo` PASS.

Ten for ten, and this is the highest-blast-radius change of the run: **every** eventsink map in the
game changed how it registers, four classes gained their handlers, and four stopped registering
twice. `map_filter` is the one to watch — `CMapFilters` was a *winner* of a collision and had been
registering its range handler twice; it still filters the map.
