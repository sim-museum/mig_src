# Sprint 158 — "A new gold standard, and the class of target it asks for" (EPIC K opened) — ✅ CLOSED 2026-08-21 (goal MET, 8/8)

**Planned 2026-08-21** (PO ceremonies pre-approved). The Product Owner added a **new gold standard**
to `~/gold standard/ma/` — `wonju_attack.mp4` plus a written walkthrough `wonju_script.txt` — with the
stated intent *"as a test of campaign I will try to create and run this mission in linux MA."*

**Sprint Goal:** the new gold is usable as an oracle, and the port can be pointed at the *class* of
target the walkthrough starts from.

| Story | Pts | Result |
|---|---|---|
| S158-1 K0: `gold_video.sh` learns the new recording | 2 | ✅ `wonju` alias; geometry measured (1280×1024 at desktop 320,28) |
| S158-2 K0: inventory the script against the video | 3 | ✅ `port/scrum/wonju-walkthrough.md` — 14 timeline rows, and **the recording stops at the frag screen** |
| S158-3 K1: can the port reach a supply dump at all? | 3 | ✅ 20 supply items on the map; the dossier opens with the right fields — and **its art overflows the dialog by 281 px** |

## What the new gold actually contains

344 s, 60 fps, the game windowed at 1280×1024 inside a 1920×1080 desktop capture. It covers script
steps **4–14 — building the mission — and stops at the frag screen (~t=333).** Steps 15–18 (fly it)
are **script-only: there is no video oracle for K10–K13.** Recording that is worth a note to the PO,
because the obvious assumption ("the video shows the mission being flown") is wrong and a verdict
written against it would be fiction.

Full timeline in `port/scrum/wonju-walkthrough.md`. The load-bearing frames:

- **t≈155** — three dialogs at once: **TASKS** (tabs Flak Supp./AAA Cover/Air Cover; Squadron,
  Attack Method, Attack Pattern, Group Formation, Escort Position; then Flight 1–4 rows each with a
  stores combo and a Target combo), **PAYLOAD** (`8 Rockets (140 lb) / 250 gall External Fuel`, three
  options), and the **WONJU SUPPLY DUMP** mission folder (`Wave / ToT / Main Duty / AAA Cover / Air
  Cover`, buttons `Route  Task  Save  Ins Wave  Del Wave`).
- **t≈265** — the route as a white polyline over a zoomed map, being dragged.
- **t≈305** — the frag: a wave combo, four `F84 1 (08:12) Bomb` rows, callsign combos, pilot names.

**The cheapest end-to-end assertion in the whole epic is a single number**: the COMBAT ORDER dialog's
`Flights` field, which reads **6** at t≈90 and **8** at t≈265. It only moves if the flight the player
added actually reached the mission. Prefer it to any screenshot diff for K5/K7.

## S158-3 — the K1 measurement

The existing `MA_MAP_ITEM_SCAN` hook asks the map's own hit-test where its items are. It printed
`band=9472`, which is a number until you open `UNIQUEID.H` and work out that 9472 = 0x2500 =
`AmberSupplyBAND`, and it clicked **whichever item came first** — a bridge on the pinned save. The
walkthrough starts at a *supply dump*, so the scan now:

- prints the band **by name** (`band=0x2500 AmberSupply`),
- **tallies** the classes present — this is the measurement:

```
[mapband] 0x2500 AmberSupply        20
[mapband] 0x2700 AmberBridge        22
[mapband] 0x1200 AmberAirfield       5
[mapband] 0x1a00 AmberCivilian       3
[mapband] 0x0100 WayPoint            6
```

- and takes `MA_MAP_CLICK_BAND=AmberSupply` (name or number) to pick its click target by class.
  An unsatisfiable request **clicks nothing and says so** — silently clicking another class reads
  exactly like "the dossier shows the wrong target" (S85's ambiguous-`#ID` lesson).

Result: `[mapitem] band AmberSupply selected id=9798(0x2646) at (396,156)` → the **DOSSIER opens on a
supply target** — *Objective: Sukchon Warehouses, MSR West, Threat AAA Medium / MiG 15 Low, Activity
Very Low, Repairs Operational, Last Sortie (never)* — with the `Details / Damage / Notes` tabs and the
`Center / Zoom / Photo / Authorize` buttons. **Photo is K2's entry point and Authorize is K4's**, so
K1's own path is present and the next two steps have somewhere to start.

## The defect the measurement found

The dossier's **backdrop art is painted at its natural size, not the dialog's**:

| | |
|---|---|
| dialog node rect (`MA_TRACE_OOB`) | **330 × 320** at (679, 0) |
| art actually painted (measured off the capture) | **≈394 × 575** — dark pixels run to y=601 at x=900 |
| overhang | **281 px below the dialog, ~64 px to its right** |

It is not specific to supply targets: the same run's bridge dossier (`Taeryong Road Bridge`) has the
same ~330 px skirt hanging below its `Center/Zoom/Photo/Authorize` row. This is the **PO-47 shape**
— *"the dialog is not oversized, the ART is"* — one screen further on. S156 fixed that case in
`RMdlDlg::DoModal` with `ma_gdi_set_clip` around the art blit; the dossier is painted by the map's
OOB walk instead, so the fix does not transfer for free.

⚠ **And the neighbouring clip has already been tried and reverted**: S155 clipped the OOB *node* rect
for PO-43 and it removed the tab row and the combo border, because the node rect is smaller than the
dialog's visible content. So S159 must clip **the art blit specifically**, at the size the dialog
reports — not the node's whole paint — or the S155 revert repeats.

Logged as **PO-49** and carried into Sprint 159.

## Definition of Done

- [x] `port/tools/gold_video.sh` lists and serves `wonju`; geometry measured, not assumed
- [x] Walkthrough inventory committed with per-step port items and the "no flight footage" warning
- [x] Ninja build clean; `wmig` links
- [x] `port/map_icon_click.sh` (PO-3's gate) still **PASS** — dossier painted, 50 passes
- [x] The new hook is env-gated and default-off; with no `MA_MAP_CLICK_BAND` the old behaviour stands
- [x] EPIC K added to `scrum.md` (K0–K13, 75 pts) with the PO's own words recorded
