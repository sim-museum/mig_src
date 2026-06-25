# Wine gold-standard reference screenshots

MiG Alley running under **Wine** (the reference renderer) — the visual A/B target for the
native Linux port. Captured 2026-06-24 (build label "BDG version 0.85F"), full-desktop PNGs
with the game window inside. Preserved here because the source (a USB volume) is transient.

Use these for fidelity comparison: dump a native frame (`MA_DUMP_BACK=N` → `/tmp/maback.ppm`,
or `BOB_DUMP_FRAME=N`) at the matching screen and diff against the corresponding image.

| # | File | Screen | Port status | Fidelity notes |
|---|------|--------|-------------|----------------|
| 01 | `01-title.png` | Title (Preferences / Single Player / Multi-Player / Load Game / Replay / Credits / BDG version 0.85F / Quit) | ✅ renders (Sprint 4 launches `&title`) | menu list + title.bmp; A/B the yellow menu font |
| 02 | `02-prefs-3d.png` | Preferences **3D** tab (Display Driver, Resolutions, Gamma, Lowest Frame Rate, Auto Frame Rate, Ground/Item Shading, Reflections, Weather Effects) | ✅ end-to-end (Sprint 2) | Wine lists `1400 X 1050`; port pins software 640/800/1024 (by design) |
| 03 | `03-prefs-3d2.png` | Preferences **3D II** (Filtering, Transparency, Texture Quality, Trees, Routes, A/C + Item Shadows, Horizon Fade/Distance, Contour Detail) | ✅ | combo values + tab nav |
| 04 | `04-prefs-flight.png` | Preferences **Flight** (Flame Outs, Auto Throttle, Power Boost, Wind Effects/Gusts, Flight Model, Airframe Stress, Torque, Spool Up) | ✅ | |
| 05 | `05-prefs-game.png` | Preferences **Game** (Weapons, Vulnerable To Fire, Collisions, Complex AI, Accel, Target Size, Autopilot Skill UN/Reds, Gun Sight Ranging) | ✅ | |
| 06 | `06-prefs-views.png` | Preferences **Views** (Restricted Views, Peripheral Vision, Auto Padlock, View Mode, Camera Color, Info Line, Units, Gun Camera, HUD) | ✅ | |
| 07 | `07-prefs-controls.png` | Preferences **Controls** (Input Devices = "Logitech Extreme 3D", axis mappings, Dead Zone) | ✅ (Sprint 10 joystick) | device name "Logitech Extreme 3D" renders clean — the FormatV `%s`/CString fix target |
| 08 | `08-prefs-sound.png` | Preferences **Sound/Others** (Motor/SFX/Ambient/Radio/Engine Volume, G-FX, Injury FX, White Out, Auto Vectoring) | ✅ | |
| 09 | `09-quickmission.png` | Quick Mission setup (Mission / Life / Aircraft / airfield + Back · Variants · Fly) | ✅ (Sprint 4) | |
| 10 | `10-flight-cockpit.png` | ★ **In-cockpit 3D flight** — canopy frame, **cyan gunsight reticle**, instrument panel, terrain horizon, distant aircraft | ◐ first-frame renders (Phase 5) | THE software-rasterizer A/B target. Port shows sky/horizon/green terrain/cyan HUD; verify geometry + colours |
| 11 | `11-flight-external.png` | External/chase view — F-86 ("USAF" / "FU-941"), ground + cloud layer | ❌ not validated | external/padlock/fly-by views are a later 3D sub-phase |
| 12 | `12-debrief.png` | Debrief stats (Mission/Base/Status; Claims · Player · UN · Lost table; Back · Ac Stats · Ground Stats · Replay) | ◐ (Sprint 5 area) | text-table layout; FormatV fix relevant |
| 13 | `13-campaign-select.png` | Campaign select — Korean-war phases + dates (North Korea Invades … The Spring Offensive) + Back · Film · Background · Objectives · Begin | ✅ (Sprint 4) | |
| 14 | `14-operational-map.png` | ★ **Operational map of Korea** — full-colour strategic map, red/blue airfield+target icons, front line, pilot/squadron info window | ◐ renders but **greyish** (Sprint 14) | THE colour-fidelity A/B target (M4/M8 map-tile palette gap) |

## Highest-value A/B targets
- **#14 operational map** — the port reaches this (loadgame → map, Sprint 14) but the tiles
  render greyish; this is the exact reference to fix the M4/M8 map-tile **colour fidelity**.
- **#10 flight cockpit** — the software-rasterizer fidelity reference (geometry, palette, HUD).
- **#11 external view** — the next 3D view mode to bring up.
