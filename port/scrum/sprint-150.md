# Sprint 150 — "The refresh flag was cleared before it was read" (PO-31) — ✅ CLOSED 2026-08-16 (goal MET) — ⭐ the ADI works in hardware

**Planned 2026-08-16 (PO: *"the ADI at upper right has never worked with hardware graphics"*).**
**Sprint Goal:** the attitude indicator follows the aircraft on the hardware renderer.

| Story | Pts | Result |
|---|---|---|
| S150-1 why is fRefresh never true? | 3 | ⭐ the port clears `lastoffset` before the draw reads it |
| S150-2 roll must refresh too | 2 | ✅ `offset` is pitch only |
| S150-3 prove it on the screen | 3 | ✅ 19,696 px change; the ball is at a different attitude |

## The finding

`COverlay::DoArtHoriz` draws the ball in hardware with:

```c
pw->DoPutC(pball, dp, offset != lastoffset ? true : false);   // 3rd arg = fRefresh
```

`fRefresh` is what makes `direct_3d::CreateTexture` take its `RemakeTexture` branch instead of
returning the cached texture. The port's own `MA_LINUX` block — which bakes roll and pitch into the
ball's pixels, because the ported texturer tiles a roll-rotated quad — ended with:

```c
lastoffset = offset; lastroll = rollkey;
```

**`lastoffset` is exactly what the draw tests twelve lines later.** Setting it there made
`offset != lastoffset` **always false**, so the freshly re-baked pixels were never sent to the
video texture and the instrument showed the same image for the whole flight. The engine updates
`lastoffset` *after* the draw (line ~7379) — that is the contract this block broke.

Measured before the fix, at the start of the night: `direct_3d::PutC` called **512 times**,
`PutC with fRefresh` **zero times**, `RemakeTexture` **zero times**. After: `PutC with fRefresh`
fires.

**A second fault in the same expression.** `offset` is *pitch only* — pitch quantised to 1/64 of
90°, about 1.4° per step. So even with the premature update fixed, **banking** the aircraft would
re-bake the ball and still not re-upload it. Roll is precisely what the PO was watching ("does not
change with aircraft orientation"). The draw now refreshes when the ball was re-baked for either.

## Proving it took longer than fixing it

Three of my measurement attempts were wrong before one was right, and each looked plausible:

- `BOB_AUTOFLY=sweep` — CLAUDE.md records it trips a SEGV; the flight never sustained.
- `MA_DUMP_BACK` — captures the **software back surface**, which in hardware holds the loading
  blueprint, not the flight. Two identical loading screens compare as "0 pixels differ", which is
  exactly the answer I had been getting.
- `BOB_DUMP_FRAME` on a recipe that reached the campaign map instead of a flight.

The right instrument is `BOB_DUMP_FRAME` (the GL drawable) on the click recipe `stress_launch`
uses, checked against `[3d] Launch3d returned` in the same log. *The original "the ADI region is
byte-identical" measurement may have been reading a loading screen too — the fix is confirmed by
the fRefresh census and by the two flight frames, not by that.*

## Evidence

`port/ref/native/adi_hardware.png` — the instrument at frames 1500 and 4500 of one hardware
flight, side by side: the horizon ball sits at visibly different attitudes. **19,696 of 25,500
pixels** in the ADI region differ between them.

## Gates

parity 5/5 byte-identical · hardware stress 4/4 sustained 100 3D frames.
