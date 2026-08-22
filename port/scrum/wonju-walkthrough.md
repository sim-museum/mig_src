# The Wonju supply-depot attack — gold walkthrough (EPIC K)

_Inventoried S158 (2026-08-21) from the PO's new gold standard:_

| Asset | Path |
|---|---|
| Video | `~/gold standard/ma/wonju_attack.mp4` — 1920×1080 desktop capture, 60 fps, **344.1 s** |
| Script | `~/gold standard/ma/wonju_script.txt` — the PO's own step list, steps **4–18** |
| Frames | `port/tools/gold_video.sh {frame,crop,sheet,geom} wonju <t>` (the `wonju` alias was added this sprint) |

**Geometry.** The game runs **windowed at 1280×1024, letterboxed at desktop (320, 28)**, exactly like
the two 260814 recordings — so `crop=1280:1024:320:28` yields the game's own pixels at its own scale.
`geom` reports the full 1920×1080 desktop for a few instants (a bright desktop element enters the
frame); crop by the fixed rect rather than trusting `geom` per frame.

> ⚠ **The recording stops at the frag screen (~t=333).** It covers script steps **4–14 — building the
> mission — and nothing of steps 15–18.** K10–K13 (fly it) therefore have **no video oracle of their
> own**; their nearest oracle is the older `full` video (260814), which shows the runway start, the
> cockpit map, the radio menus and the debrief for a *pre-built* campaign mission. Do not record a
> K10–K13 verdict as "matches gold" against this video.

## Timeline

| t (s) | Screen / dialog | Script step | Port item |
|---|---|---|---|
| 0–8 | Title screen: Hot Shot / Quick Mission / **Campaign** / Entire War / Back | (pre-1) | — works |
| 8–18 | Campaign selection: `Back  Film  Background  Objectives  Begin` | (pre-1) | — |
| 18–23 | Campaign map + **Player Log** | (pre-1) | I4/#15 CLOSED |
| 24–38 | **DIRECTIVES**: Auto Generation / Air Superiority / Alpha Strike tickboxes, Category row (Strike / Fighters / Targets / Monitor), and the seven effort sliders (Ground, Supply, Airfields, Fleet, Power, Army, Reserve) with % readouts | (pre-4) | S85 opens it; the **sliders and their percentages are new** |
| 40–62 | Map browsing with **Front Line + Red Supply filters on** — the front line is the orange diagonal, the supply icons are the red squares; the Wonju dump sits north of the Central Front Line marker | 4 | **K1** |
| 63–72 | **Photo → 3D recon**: the toolbar reads `S1 Morning …`; terrain, then zoomed in on a **row of warehouse buildings** | 5 | **K2** |
| 75–82 | Back on the map, zoomed in far enough for the **sub-target icons**; Damage tab → top combo lists them | 6 | **K3** |
| ~80 | Authorize → **Minimum Strike** (the auto-fill "Fighter Bomber Strike" is explicitly *not* taken) | 7 | **K4** |
| 84 → end | **WONJU SUPPLY DUMP** mission folder — table `Wave / ToT / Main Duty / AAA Cover / Air Cover`, rows `1.Bomb 08:30 F84 (2)` and later `2.Flak Supp. 08:20 F86 1 (4)`; buttons `Route  Task  Save  Ins Wave  Del Wave` | 8–13 | **K5** |
| 84 → end | **COMBAT ORDER** (bottom-left, mostly off-screen): `ToT 08:30`, `Flights 6` → **8** by t=265 — the flight count is the readout that proves K5/K7 landed | 8, 11 | **K5/K7** |
| 90–250 | **TASKS** dialog: tabs `Flak Supp. / AAA Cover / Air Cover`; fields Squadron (`F86 1 (4/4)`), Attack Method (`Dive Bomb`), Attack Pattern (`Individual…`), Group Formation (`Flat V`), Escort Position (`Lead G…`); then `Flight 1..4` rows, each a stores combo + a `Target` combo (`Main Target` / `Leader's Target`) | 8–12 | **K5/K6/K7** |
| ~155 | **PAYLOAD** dialog: `Current Selection: 8 Rockets (140 lb) / 250 gall External Fuel`; `Options: Fuel tanks / Rockets & Fuel tanks / No external stores` | 10–11 | **K5/K7** |
| 255–285 | Map zoomed right in; the route is the **white polyline** with square waypoint markers, dragged against the red/blue icon field | 13 | **K8** |
| 288–333 | **Frag / pilot roster**: `Squadrons ○ / Mission ☑` radio pair, a wave combo (`1: Wonju Supply Dump, Wave1 / Wave2`), then four `F84  1 (08:12)  Bomb` rows, each with a callsign combo (`Viper`) and **four pilot-name fields** | 14 | **K9** |
| 333–344 | recording ends on the desktop | — | — |

## What the timeline tells us about the port

1. **The build half is nine dialogs deep and every one of them is a *combo box*.** TASKS alone drives
   five, PAYLOAD one, the frag two. Combos are hosted (S69 dressed them) but the port has never had to
   *change a selection and have the change stick into game state* on this path — that is what K5/K6/K7
   actually test.
2. **`Flights 6 → 8` in COMBAT ORDER is the cheapest end-to-end assertion in the epic.** It is one
   number, on screen throughout, and it only moves if the flight the PO added reached the mission.
   Prefer it to a screenshot diff when judging K5/K7.
3. **K8 (route drag) is the one genuinely new *interaction*.** Every click the port has learned so far
   is press-and-release in one place; a waypoint drag is press → move → release with the map redrawing
   under the cursor. Nothing in the tree does that today.
4. **The DIRECTIVES sliders (t=24–38) are not on the script but are on the way to it** — the PO passed
   through that dialog to reach the map. S85 proved it *opens*; the sliders' behaviour is unmeasured.
