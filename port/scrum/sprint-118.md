# Sprint 118 — "The player can choose it" (PO-12 phase 4) — ✅ CLOSED 2026-08-15 (goal MET) — ⭐ PO-12 delivered

**Planned 2026-08-15 (PO pre-approved ceremonies; continuous-sprint directive). Autonomous. ~8 pts.**
**Sprint Goal:** turn the hardware renderer from a developer env var into the Preferences option the
PO asked for.

| Story | Pts | Result |
|---|---|---|
| S118-1 does hardware mode break the front end? | 2 | ✅ measured — one real defect found |
| S118-2 offer the driver in Preferences | 4 | ✅ "Primary Display Driver" is selectable and persists |
| S118-3 keep the gates meaningful | 2 | ✅ every gate now pins its renderer |

## ⭐ The PO's ask, delivered

Preferences → 3D → **Display Driver** now offers:

```
Software Driver
Primary Display Driver     <- the hardware path (S115-S117)
```

Choosing it writes `Save_Data.fSoftware=false` to `settings.mig` through the game's own
`OPTIONS`/`SG2C_WRITEBACK` path, and the next launch flies on the hardware renderer with no
environment variable anywhere: `port/ref/native/hw_selected_in_prefs.png` is a Hot Shot flight
whose only instruction was the menu click.

## What the measurement found

**Start by asking whether hardware mode breaks anything outside the cockpit.** Running the standing
parity gate with hardware forced on is a one-line experiment and it found a real defect:

```
title          OK byte-identical
prefs_3d       DIFF  (704 px differ)      <- the Resolutions combo was EMPTY
prefs_others   OK byte-identical
quickmission   OK byte-identical
campaign_map   OK byte-identical
```

704 pixels, and they were exactly the `640 X 480` readout. A player selecting hardware would have
found no resolutions at all. Three things all derive from `Save_Data.dddriver` and all had to
agree:

- **the mode tag** — SDETAIL filters on `driverModes[x].driverNo - 1 == driver_index`, and the port
  hardcoded `driverNo = 0` (software) for every mode;
- **the width table** — `IsValidMode(modeFlags, w, h)` reads `soft_modes` for software but
  `hard_modes[dddriver + 1]` for hardware. Registering into `hard_modes[dddriver]` left the combo
  empty *with the tag already correct*: the filter passed and the width check then rejected every
  mode. The index was taken from SDETAIL's own expression rather than guessed — both of its
  branches land on `dddriver + 1`.
- **the driver number itself** — the port offers exactly one hardware driver and it is the primary,
  which `dddriver` names as **-1**. A saved `0` makes SDETAIL compute combo index 2, which does not
  exist (`driverCount` is 0). Normalised on load, so a settings file written by an earlier build
  cannot carry an unreachable driver number in.

**And the option has to be visible to be chosen.** SDETAIL adds the hardware entry only when
`!fNoHardwareAtAll && sd.fFirstHardIsPrimary`; without those the combo has one entry and the option
is unreachable no matter what the renderer can do. The engine sets them in `CONFIG.CPP` after
probing a real device for texture formats and free RAM; the port's device is synthetic, so
`ma_populate_software_modes` states the same conclusion for it.

## The three places that forced software

S110 measured them and warned that "a hardware choice has to survive all of them". They are now one
predicate, `ma_hardware_available()`:

| where | was | now |
|---|---|---|
| `STUB3D::MakePassive` | `fSoftware = true` unless `MA_TRY_HARDWARE` | honours the player's choice |
| `ma_populate_software_modes` | pinned software + `dddriver = -1` | pins only when no driver is offered |
| `MIG.CPP` before display init | forced by env | reads the loaded preference, normalises `dddriver` |
| `MATRIX.CPP body2screen` | gated on the env var | uses the engine's own `DoingHardware3D()` — which tests `DD.lpDirect3D != NULL`, the definitive runtime answer |

`MA_TRY_HARDWARE=1` survives as a developer override that forces hardware on; `MA_NO_HARDWARE=1`
withdraws the offer entirely, for a machine whose GL cannot cope and for the gates.

## Keeping the gates meaningful

With the renderer now a *player setting*, an unpinned gate tests whichever renderer `settings.mig`
happens to hold — and this repo's own runs write that file. Every gate script now pins
`MA_NO_HARDWARE=1` (the software path its references were captured from), and `MA_NO_HARDWARE=0`
runs the same gate on hardware. The predicate is deliberately value-sensitive, so `=0` means what it
says.

Worth stating plainly: **the standing gates pass on BOTH renderers.**

| gate | software | hardware |
|---|---|---|
| parity 2D (5 screens) | 5/5 byte-identical | 5/5 byte-identical |
| stress launch | 20/20 | 20/20 |
| sweep / map click / map drag / sysbox / help / overlay | pass | — |
| ASan (flight + campaign map/fly/nextday) | 0 reports | — |

**A harness error worth booking, not hiding.** The overlay-text gate first came back FAIL, and the
cause was mine: I ran it concurrently with the ASan gate, so two runs were driving the display at
once. Only one of them was inside `gl-lock`. Run alone it passes 3/3. *A gate result obtained
outside the display lock is not a result* — the same discipline that already applies to launching
the game, applied to gates.

## Result

**PO-12 is delivered.** The player chooses hardware graphics in Preferences, as in BoB; the choice
persists; the flight renders through the DX5/6 execute-buffer path on the GPU; and software remains
selectable and bit-identical.

Still open on the renderer itself, as quality work rather than a blocker: fog and specular (states
28/29 are read but not applied), `IDirect3DViewport::Clear`, and a sweep of the other views
(external, padlock, map) against the software oracle the way S117 did the cockpit.
