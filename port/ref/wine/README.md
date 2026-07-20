# Wine pixel-oracle reference frames

Ground-truth captures of **MiG Alley running under Wine** (the original Windows binary), used as the
reference set for the native Linux port. `port/ab.sh` diffs native captures against these per view and
writes `<view>_native.png` / `_wine.png` / `_diff.png` / `_sidebyside.png` into `port/out/ab/`.

The rule these encode: **when native and Wine disagree, Wine is right.** A fidelity port has a ground
truth available; use it rather than judging frames by impression.

| File | What it shows | Consumed by |
|---|---|---|
| `01_cockpit_fwd_gunsight.png` | Forward cockpit, gunsight | `ab.sh cockpit` (default launch view) |
| `02_cockpit_fwd_hud.png` | Forward cockpit, HUD | — |
| `03_cockpit_panel_down.png` | Panel-down view | — |
| `04_ext_chase_high.png` | External chase, high | `ab.sh external` (F6, DIK 0x40) |
| `05_ext_chase_low_airfield.png` | External chase, low over airfield | `ab.sh chase` (F9, DIK 0x43) |
| `06_ext_flyby_terrain.png` | Flyby over terrain | `ab.sh satellite` (F10, DIK 0x44) |
| `07_campaign_map_planning.png` | **Campaign strategic map, 2D front-end** (added 2026-07-19) | not yet automated — see below |

---

## 07_campaign_map_planning.png — the first 2D/front-end reference

**1917×1077, 10,712 distinct colours.** Campaign map, `6/25/50: Morning, planning`, with the **Player Log**
dialog open on its Career tab.

### Why this one matters more than its subject suggests

Every other reference here is a 3D flight view. This is the first capture of the **2D front-end**, and it
is at **~1920×1080** — precisely the resolution class where the native port is known to break. `STATUS.md`
records these as open:

- the campaign map **tiles / patchworks** at high resolution
- its **unit icons vanish** at high resolution
- the **kneeboard page renders blank**
- campaign-map **wheel-zoom resizes the window** (present canvas tied to `m_size`)

This frame is the proof that **none of those are engine or resolution limits** — the original renders the
full map correctly at this size. They are port bugs in the 2D layer (`ma_gdi.cpp` blit/stretch path,
`ma_populate_software_modes`, and the `SetDIBitsToDevice`/`StretchDIBits` viewport handling). That
converts four open items from "maybe it just can't do this" into "we have the target picture".

### What to check a native capture against, feature by feature

- **Title block** (top-left): `MIG ALLEY` + `6/25/50: Morning, planning` — the live campaign clock.
- **Two toolbar rows of sheet icons** top-centre, plus the right-hand toolbar groups. These are hosted
  `CRButtonCtrl` OCX buttons drawing sprite-sheet regions, i.e. the §8b `ICON_PAGE_*` path in the shared
  lessons doc — icon *presence and distinctness* is the thing to diff, since a broken sheet lookup makes
  them all render as the same icon or as blanks.
- **Nm ruler** down the left edge, 0–350, tick-labelled.
- **Terrain**: Korean peninsula with blue rivers/sea, red road and border network, correct coastline. This
  is the layer that patchworks natively.
- **Unit icons**: red (North) concentrated north and along the front, green and blue (friendly) in the
  south — these are what vanish natively.
- **Mission routes**: white leg lines from the southern airfields north, with one orange leg (selected
  package). Route rendering exercises `SetHiLightInfo`.
- **Player Log dialog**: tabs `Career` / `Log of Missions` / `Last Mission`; a `Name` edit field with the
  focus caret visible; a stats grid `Sorties / Combats / Kills / Losses` over rows `F86 1, F86 2, F80,
  F84, F51, All`; pilot photograph as the panel background. This is an OCX-hosted dialog — RStatic labels,
  the edit control, and the tab strip — so it doubles as a reference for the OOB/dialog work (the
  selected-tab rendering that is still missing natively is visible here as the raised `Career` tab).

### Capturing the native counterpart

`ab.sh` only drives 3D flight views, so this reference is **not yet wired into it**. To capture the
native equivalent, drive the front-end instead of flight — `MA_CAMP_FLY` / `MA_QUICKMISS` / the
`BOB_CLICKSEQ` frame-indexed click hooks to reach the map, then `MA_DUMP_BACK=<n>` +
`BOB_EXIT_AFTER_DUMP=1`. Note the source frame here is 1917×1077 rather than a round 1920×1080, so a
strict pixel diff needs the native capture at the same mode — or a size-tolerant comparison.

**Worth borrowing when you automate it:** the sibling BoB port's `tools/bob_validate.sh` prints objective
per-band statistics (distinct colour count, per-band average RGB, non-black %) instead of requiring a
human to look. For this frame the tell-tales are cheap: a patchworked map collapses the distinct-colour
count, and vanished icons show up as a large drop in distinct colours in the middle band specifically.
