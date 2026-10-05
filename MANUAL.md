# MiG Alley — Pilot's Manual (Linux port)

*New manual, 2026-10-01. Written for the native Linux port of Rowan's MiG Alley (v1.23 game data).*

**Sources.**
- The 1999 Empire/Rowan manual (`DOC/Mig-Alley_Manual_Win_EN.pdf`), the reference card, the
  v1.02–1.23 patch notes, `MigAlleyTips.pdf` and the community tutorials under `~/sgl/TUE/MigAlley/DOC`.
- The port's own notes (`RUNNING.md`, the 2026-09-28 multiplayer guide).
- For every key: the game's own binding table (see [Keys](#16-keyboard-reference)).

Where the 1999 manual and the program disagree, the program wins; [section 17](#17-corrections-to-the-1999-manual)
lists every correction.

---

## Contents

1. [The game](#1-the-game)
2. [Installing and starting](#2-installing-and-starting)
3. [Menus and game modes](#3-menus-and-game-modes)
4. [Campaigns](#4-campaigns)
5. [The Map screen and mission planning](#5-the-map-screen-and-mission-planning)
6. [Two worked missions](#6-two-worked-missions)
7. [Flying the aircraft](#7-flying-the-aircraft)
8. [Weapons and the gunsight](#8-weapons-and-the-gunsight)
9. [Views](#9-views)
10. [Radio](#10-radio)
11. [Cockpits and instruments](#11-cockpits-and-instruments)
12. [Air combat: MiG versus Sabre](#12-air-combat-mig-versus-sabre)
13. [Ground attack](#13-ground-attack)
14. [Gun camera and replay](#14-gun-camera-and-replay)
15. [Multiplayer](#15-multiplayer)
16. [Keyboard reference](#16-keyboard-reference)
17. [Corrections to the 1999 manual](#17-corrections-to-the-1999-manual)
18. [Preferences](#18-preferences)
19. [Troubleshooting](#19-troubleshooting)

---

## 1. The game

MiG Alley covers the air war over Korea from June 1950 to the Spring Offensive of 1951. You fly for
the United Nations air forces (or, in Quick Missions and multiplayer, for the Communist side). It is a
gun-and-bomb war with no guided missiles: a kill means getting close. Expect big fights, sometimes
more than 150 aircraft in the sky at once.

**Flyable aircraft**

| Aircraft | Side | Role in the game |
|---|---|---|
| F-86A/E/F Sabre | UN | The only UN fighter that can meet the MiG on equal terms. Six .50 cal guns (1,800 rounds). |
| F-80C Shooting Star | UN | Straight-wing jet; outclassed by the MiG, used for strike. |
| F-84E Thunderjet | UN | Straight-wing jet; the better of the two strike jets, can fly long-range escort. |
| F-51D Mustang | UN | WWII piston fighter for close air support near the front. 1,880 rounds; can carry rockets and bombs together. |
| MiG-15 / MiG-15bis | Communist | Out-climbs and out-accelerates the Sabre. Two 23 mm cannons (160 rounds) and one 37 mm (40 rounds). |

The war is a ground war first. Your job is mostly support: protect UN troops, cut Communist supply
routes, and keep the MiGs off the bombers.

---

## 2. Installing and starting

**The AppImage.** The packaged game is a single file, for example `MigAlley-x86_64-260926b.AppImage`.
Make it executable and run it:

```bash
chmod +x MigAlley-x86_64-260926b.AppImage
./MigAlley-x86_64-260926b.AppImage
```

**From an installed tree** (developer layout):

```bash
cd ~/sgl/TUE/MigAlley/WP/drive_c/rowan/mig && ./wmig
```

A bare launch goes to the title screen. **Single Player → Hot Shot** puts you in the air in two clicks.

**Data folder.** `MA_HOME=<dir>` gives a copy of the game its own save games, settings and logs.
You need this when running two copies on one PC; the first run copies about 730 MB.

**Quitting.** Use **Exit** on the title screen, or close the window. **Ctrl+Esc** is a port-added hard
exit that saves preferences first. Esc on its own never quits the program.

**Settings file.** Graphics, sound and gameplay settings live in `savegame/settings.mig`. The 1.23
notes say to delete it after changing graphics hardware.

**Rebinding keys.** See [section 16](#16-keyboard-reference): `MA_DUMP_BINDINGS=1` writes the live key
table to `controls.cfg`, you edit a line, and the game applies it at the next start.

---

## 3. Menus and game modes

Click the intro animation to reach the **Main Menu**. Everything is mouse-driven. In MiG Alley,
clicking the current value of a drop-down list steps it to the next value.

| Mode | What it is |
|---|---|
| **Hot Shot** | One fixed mission, no options: a Sabre escorting fighter-bombers into MiG country. The fastest way into a fight. Press **F1** to padlock the nearest MiG. |
| **Quick Mission** | Short missions with many settings. The first two (including the default Landing/Take-off) have no MiGs, which makes them good for learning to fly. |
| **Campaign** | One of the five campaigns, played on its own. |
| **Entire War** | All five campaigns in order. A campaign that goes badly can lose the war. |
| **Multi-Player** | Death Match, Team Play and Quick Missions with other players ([section 15](#15-multiplayer)). |
| **Preferences** | Graphics, flight model, game rules, views, controls and sound ([section 18](#18-preferences)). |
| **Replay** | Gun-camera recordings ([section 14](#14-gun-camera-and-replay)). |

**Quick Mission settings**

| Setting | Effect |
|---|---|
| **Flight** | UN or Red flight, and your seat in it (Lead 1 or 2, Wing 1 or 2). |
| **Target zone** | Civilian, bridges, airfields, supply points, roads or rail, then a specific target from the next list. |
| **Scenario** (tick box) | A short brief of the mission. |
| **UN / Red** (tick boxes) | Per-squadron pilots, duty (the aircraft type, with the number of flights), and AI skill (affects air combat only). On the Red side you can pick MiG-15 or MiG-15bis. |

---

## 4. Campaigns

You play an ambitious USAF officer who wants combat time on every fighter type, and then the post
of Air Commander.

| # | Dates | Aircraft | Character |
|---|---|---|---|
| 1 | 25 Jun – 1 Aug 1950 | F-80C | Pre-planned missions with a set objective in a limited number of missions |
| 2 | 2 Aug – 15 Sep 1950 | F-51D | **Pusan Perimeter** (the CAS walk-through in section 6); as above |
| 3 | 18 Sep – 1 Nov 1950 | F-84E | as above |
| 4 | 2 Nov 1950 – 1 Jan 1951 | F-86A | as above |
| 5 | Spring 1951 | all four | **Spring Offensive**: you are the Front Line Air Commander |

In campaigns 1–4 the missions come pre-planned. You can still edit or replace them on the Map screen.

**The Spring Offensive brief**
- You command 112 aircraft in seven squadrons: two of Sabres, one F-84E, one F-80C, one F-51, and
  bombers based in Japan (B-29 heavies, or B-26s on days the B-29s are withheld for losses).
- The goal is to push the Communists back to the Chinese border.
- Each day has three sessions (Morning, Midday, Afternoon), with up to 96 aircraft per session.
  Bad weather can cancel a session.
- Squadrons have six flights of four, but at most 16 aircraft (four flights) can launch per session,
  which lets pilots rest.

**The battle.** The peninsula runs north–south, so the front runs east–west. Communist strength
depends on supplies moving along three Main Supply Routes (west coast, central, east coast), with
many cross-links around broken bridges. Close Air Support saves a front that is about to break, but
**interdiction**, cutting supplies, is what actually moves the line north.

---

## 5. The Map screen and mission planning

Missions are planned on the **Map screen**. It has a scrollable, zoomable map, a fixed set of icons
and five dockable toolbars: **Title, Main, Utility, Scale, Filters**.

- **F1** or the **?** on a dialogue's title bar opens on-line help.
- The mouse wheel zooms the map. Dragging the Scale toolbar also zooms, and you can drop the Scale
  toolbar onto the map to measure ranges.
- The map shows bearing and range from your aircraft to the current waypoint, and from the
  previous waypoint for the others (added in v1.1).
- After a mission, the Title toolbar reads **Debrief** and the Main toolbar changes to debrief icons.

**Making missions**
- **Directives** fill the Mission Folder automatically for the session.
- **Authorize** on a target's dialogue or Dossier creates a mission framework from a profile. "Fighter
  Bomber Strike" uses the Dossier's threat estimate; "Minimum Strike" gives a bare mission for you to
  build up.
- **Redo** on the Mission Results dialogue carries a mission over to the next day. Its aircraft are
  taken out of the pool before Directives run.

**Mission structure**
- A session can hold up to **10 missions**. Each mission has up to **6 Waves**, and each Wave has its
  own Time over Target.
- A Wave is a **Main Duty Group** (strike, or BARCAP when there is no target), plus an optional **Air
  Cover (escort) Group** and an optional **AAA (flak suppression) Group**.
- Aircraft are allocated by flight (four aircraft, two elements of two). Duties can be given to single
  elements. A Group is up to four flights of one aircraft type.
- Edit with the **Profile** dialogue (insert or delete Waves), the **Task** dialogue (types, numbers,
  duties, payloads) and the **Route** dialogue.

**Routes.** Every route starts with the standard waypoints: Take-off → Rendezvous → Ingress →
Initial Point → (target zone) → Regroup → Egress → Disperse → Landing. You can drag waypoints and
insert new ones, but you cannot delete the standard set. Groups fly together from the Rendezvous to
the IP, then each Group follows its own target-zone waypoints. Escorts of a different type fly just
below the contrail layer. You can give individual elements their own target-zone routes, and each
element can have a main and a secondary target.

**Attack methods and patterns**

| Method | Meaning |
|---|---|
| High | Bomb from the altitude of the current waypoint |
| Low | Straight and level at 100 m, then release |
| Dive | Dive on the target, releasing at the right point |

| Pattern | Meaning |
|---|---|
| Wide | Stay in formation; everyone releases on the leader's call |
| Single File | Everyone attacks the same target in quick succession (bridges, runways) |
| Individual Targets | Everyone attacks at once, each a different target |
| Element Targets | As above, one target per element |
| Spaced Target Selection | One aircraft attacks while the rest circle; move on when the target dies. Only one aircraft is exposed to a flak barrage at a time. |
| Spaced Elements / Spaced Flights | The same, two aircraft or one flight at a time |

The game enforces these limits: B-29s fly High + Wide. B-26s fly High + Wide or Low + Wide. Rockets
and guns use Dive with any pattern except Wide. Bombs and napalm use High + Wide, Low with any
pattern, or Dive with any pattern except Wide.

**The Frag.** The Frag (Fragmentary Order) screen lists every mission of the session. Here you pick
your callsign and your aircraft (click a pilot name; by default you lead), then press **Fly**.

---

## 6. Two worked missions

### Close Air Support — Pusan Perimeter (F-51D)
1. **Single Player → Campaign → 2. Pusan Perimeter**, read Background and Objectives, then **Begin**.
2. Enter your name in the Player Log and close it with the tick. Read the Daily Intelligence
   Summary (D.I.S.) and close it.
3. Click the **Taegu Front Line** icon (dark yellow), then **Dossier**, then **Zoom** to centre the
   route. **Details** in the Army Report shows the front-line commanders' intelligence.
4. **Frag → Fly.** You start on the runway with the flight behind you.
5. Set full power (**0** = 100 %) and release the wheel brakes. To take off in the Mustang:
   - Either hold back pressure until it flies itself off, or
   - Push forward gently to lift the tail and hold right rudder against the swing.
   Then raise the gear (**G**) and throttle back a little so the flight can form up.
6. Accelerate to the IP: **M** (cockpit map) → **1** Accel → **4** Initial Point. You return to the
   cockpit at the IP.
7. Find the FAC (a T-6 "Mosquito"). **F2** steps through friendly padlocks until it reaches him, and
   **D** puts the red marker on the padlocked aircraft (see the Linux note in
   [section 9](#9-views)). If you lose him: **R** → **6** FAC → **2** Lost FAC.
8. The FAC marks the target with smoke rockets. **R → 6 → 1 Begin your run** commits the flight.
   Ask for **3 Re-mark** if you missed the smoke, or **5 More Targets** when this target is done.
9. Go home with **R → 6 → 6 Leave area** and accelerate home with **M → 1 → 5 Home**. **Left Alt+X**
   ends the flight at any time and opens the debrief.

The FAC flies; the TAC (Tactical Air Controller) is on the ground and marks targets with smoke mortars.

### Strike — Wonju Supply Dump (Spring Offensive, F-84E)
1. **Campaign → 5. Spring Offensive → Begin.** In **Directives**, set the Fighter (32) and Strike (64)
   allocations to zero: clicking the maximum value wraps it round to 0. Close with the tick.
2. Turn on the **Front Line** and **Red Supply** filters. Find the **Wonju Supply Dump** north of the
   central front and open its **Dossier**. It shows no MiGs expected but heavy AAA.
3. Use **Photo** for a 3-D reconnaissance of the target, and the **Damage** tab for its list of
   warehouses.
4. **Authorize → Minimum Strike.** In **Task**, add a third F-84 flight with 1,000 lb bombs. Keep
   **Dive**, and set the pattern to **Individual Targets**.
5. In the **AAA cover** tab, give one F-84 flight flak suppression with rockets and guns.
6. On the map, drag the **Egress** waypoint inland and the **IP** to within four miles of the target,
   so the target is visible as soon as you leave accelerated time. Put the AAA waypoints over the
   target.
7. **Frag → Fly.** Rotate at about 100 kt and keep the tail off the runway. Accelerate to the IP
   (**M → 1 → 4**).
8. Lead the attack with **R → 6 → 1 Begin your run**. "Cannot identify target" means you need to get
   closer. Select bombs (**N**) before you press the trigger. **F11** (impact view) shows the bombs
   landing. Secondary explosions mean a building held supplies.
9. Leave with **R → 6 → 6** before the group starts strafing on its own.

*Crack and burn* is two Waves: heavy bombs open the warehouses, then napalm burns the stores.

---

## 7. Flying the aircraft

A joystick is strongly recommended. The keyboard flies the aircraft, but crudely.

**Primary controls**

| Control | Keys |
|---|---|
| Pitch | **Up** (nose down) / **Down** (nose up) |
| Roll | **Left** / **Right** |
| Rudder | **Num0** (left) / **Num.** (right) |
| Keyboard sensitivity | **K** more / **Shift+K** less. Use low for bombing, high for dogfights. |
| Throttle | **1**–**9** = 10–90 %, **0** = 100 %, **`** or **\\** = 0 %. **=** / **-** = ±1 %. |
| Elevator trim | **O** nose down, **L** nose up, **Shift+L** reset |

The throttle keys are disabled while an analogue throttle is connected. Since v1.23 a joystick's Z
axis can be the rudder.

**Secondary controls**

| Control | Keys |
|---|---|
| Landing gear | **G** |
| Flaps | **F** toggles fully up/down. **Shift+R** up, **Shift+F** half, **Shift+V** full down. |
| Air brake | **B** |
| Wheel brakes | **,** left, **.** right (also used for steering on the ground) |
| Engine restart | Throttle below 25 %, then **E** |
| Mustang propeller (power / cruise) | **Shift+0** |
| Drop external fuel tanks | **Ctrl+F** |
| Eject | **Ctrl+E** |

**Time**

| Action | Keys |
|---|---|
| Pause | **P** (this also disables the view-pan controls) |
| Accelerated time, staying in the cockpit | **Tab** |
| Accelerated time on the map | **Shift+Tab** |
| Accelerate to a point | **M** (cockpit map) → **1 Accel** → **4** Initial Point / **5** Home / … |
| End the flight | **Left Alt+X** |

**Accel off** in Preferences sets when accelerated time drops back to real time. *Engage* waits until
you are under direct threat. *Tactical* returns earlier, which leaves time to climb and manoeuvre.

**Learning to fly**
1. **Straight and level.** Check full thrust. **I** cycles the info line until it shows % thrust,
   or you can read the thrust gauge on the panel. Push forward to hold the climb that builds with
   speed, and watch the rate-of-climb gauge. Trim takes the load off the stick for long legs.
2. **Gentle turn.** Bank 45°, then hold zero climb with aileron and a little back pressure. Keep
   under 3 g.
3. **Break turn.** Turn on G effects (Preferences → Others). From a fast run, bank to about 90° and
   pull until the screen starts to grey and you hear breathing (about 7 g). Ride that edge as the
   speed bleeds off. Buffet means the stall is close. **U** adds 1,000 ft, which helps when
   practising.
4. **Spin recovery.** Centre the stick, apply opposite rudder, and when the rotation stops, ease out
   of the dive gently. Pulling too hard re-enters the spin. **Left Shift+S** is the "spin recovery"
   cheat.

**Engines.** With *Realistic spool-up*, 1950s jets take time to reach full thrust. Slamming the
throttle burns fuel in the jet pipe: exhaust temperature jumps (normal is about 650°), the engine is
damaged, and spool-up gets even slower. Flame-outs can happen at very high yaw angles.

**Landing.** Taxi to the holding area, set 10 % and brake; there's no need to go to 0 %.

**Cheats**

| Cheat | Keys |
|---|---|
| Reload weapons | **Ctrl+R** |
| +1,000 ft | **U** |
| Spin recovery | **Left Shift+S** |

---

## 8. Weapons and the gunsight

| Action | Keys |
|---|---|
| Fire | **Space** or the joystick trigger |
| Next / previous weapon | **]** / **[** |
| Cycle weapons | **N** |
| Dump stores | **Ctrl+W** |

**The cockpit weapons switch** shows guns when down, and heavy ordnance (bombs, rockets or napalm)
when up. Only the F-51 can carry two kinds of heavy ordnance; its second switch picks rockets (down)
or bombs/napalm (up).

**Ammunition**

| Aircraft | Guns | Rounds | Firing time |
|---|---|---|---|
| F-86, F-80, F-84 | six .50 cal at 1,100 rpm | 1,800 | about 16–17 s of fire |
| F-51D | six .50 cal at 1,100 rpm | 1,880 | about 16–17 s of fire |
| MiG-15 | two 23 mm at 560 rpm, plus one 37 mm at 400 rpm | 160 + 40 | 8–9 s of fire. One hit usually kills a Sabre. |

**The gunsight** computes deflection for guns, and is fixed for bombs and rockets. Pick the ranging
mode in Preferences → Game:

- **Manual.** Dial the target's wingspan with **Q** (up) and **W** (down), then frame the target by
  sizing the sight with **Y** (more range) and **T** (less range). The range dial reads in tens of
  yards (20 = 200 yd). Track for a second before you fire.
- **Perfect radar / Realistic radar.** The range light comes on inside 1,800 yd and the dial winds
  down by itself. With *Realistic*, pointing at the ground can break the lock. Dialling in the
  wingspan is still good practice in case the radar fails.

| Aircraft | Wingspan |
|---|---|
| MiG-15 | 33′ 1″ |
| F-86 | 37′ 1½″ |
| F-84 | 33′ 7¼″ |
| F-80 | 38′ 10½″ |
| F-51 | 37′ 0″ |

**Target size** (Preferences → Game) changes how forgiving the guns are; smaller is more realistic.

---

## 9. Views

**Cameras**

| View | Key |
|---|---|
| Cockpit | **F7** |
| Outside / track | **F6** (**Shift+F6** switches between a fixed and a floating camera) |
| Forward view, no cockpit | **F8** |
| Chase / fly-by | **F9** |
| Satellite | **F10** |
| Impact (follow your bombs) | **F11** |
| Inside / outside toggle | **Backspace** |
| Preferences | **F12** |

**Padlocks**

| Target | Next | Previous | Reset to nearest |
|---|---|---|---|
| Enemy aircraft | **F1** | **Shift+F1** | **Ctrl+F1** |
| Friendly aircraft | **F2** | **Shift+F2** | **Ctrl+F2** |
| Ground target | **F3** | **Shift+F3** | **Ctrl+F3** |
| Waypoint | **F4** | **Shift+F4** | **Ctrl+F4** |

- **F5** padlocks the subject of the current radio message. **Alt+F1** shows the AI enemy's view;
  **Alt+F2** shows your escortee (on a wing, your element leader).
- **Enter** turns the padlock on and off. **Esc** resets the view.
- **D** boxes the padlocked item with a red diamond. *Linux port:* in flight, D is handled by the port
  itself: D toggles its padlock box and Alt+D a telemetry readout.

**Panning the view (number pad).** **NumLock** switches between *panning* (a smooth pan) and *fixed*
(a jump to set views).
- **Num8 / 2 / 4 / 6** move the view up/forward, down/back, left and right; **Num7 / 9 / 1 / 3** are
  the diagonals.
- **Shift** with a number-pad key pans fast.
- **Num5** resets the view. **Num/** and **Num*** step one view left or right.
- **Num-** zooms in and **Num+** zooms out (Shift for fast). **Ctrl+Num+** / **Ctrl+Num-** change the
  field of view.
- On the main keyboard, **Alt+1…9** repeat the panning keys and **Ctrl+1…9** the fixed views, so both
  sets are available at once.
- **Ctrl+Num5** or **NumEnter** glances at the instruments.

**Sticky looks** last only while you hold the key, then the view snaps back. They are good for a
quick check of your six.

| Key | Look |
|---|---|
| **Home** | Forward |
| **Insert** / **PgUp** | Forward-left / forward-right |
| **Alt+Num4** / **Alt+Num6** | Left / right |
| **Delete** / **PgDn** | Back-left / back-right |
| **End** | Back |
| **Ctrl+Num1…9** | The same looks, raised to look up |
| **ScrollLock** | Turns "look up" on or off for every sticky key |

**Situational awareness**
- **H** shows the *virtual instruments*: a threat indicator, where a smoked-glass disc is your wing
  plane and lines show other aircraft (red Communist, blue UN), and a compressed artificial horizon.
- **Peripheral vision** (Preferences → Views) shows red and blue blobs at the screen edge.
- **I** cycles the info line through flight data and view data; the last radio message also
  appears there.
- **Shift+M** turns radio-message text on and off. **Ctrl+M** shows MiGs on the 3-D map.
- **Ctrl+P** or **PrintScreen** takes a screenshot into the `stills` folder; the numbering restarts
  each run.
- **Ctrl+D** / **Shift+D** raise or lower 3-D detail in the original game; on Linux they are lost to
  the D intercept in flight, so use Preferences.

---

## 10. Radio

Press **R** to open the radio menu, then choose by number key or mouse:

| # | Menu | Shortcut |
|---|---|---|
| 1 | Group Info: status from your group, e.g. a fuel check | **Shift+1** |
| 2 | Precombat | **Shift+2** |
| 3 | Combat | **Shift+3** |
| 4 | Postcombat | **Shift+4** |
| 5 | Tower: MayDay, Home Tower or Nearest Tower vector, surface wind, wind at 35,000 ft, Land at home/nearest | **Shift+5** |
| 6 | FAC: Begin your run, Lost FAC, Re-mark, Missed, More targets, Leave area | **Shift+6** |

In multiplayer the menu also has **Comms Player** and **Comms Msg** ([section 15](#15-multiplayer)).

**One-key calls**

| Key | Call |
|---|---|
| **A** | "Any bandits?": asks where the nearest enemy is |
| **C** | As leader, "Am I clear?"; as wingman, "You're clear" |
| **Z** | "Break!": tells a flight member to evade |
| **Ctrl+V** | Turns the player's voice on and off |

Who answers:
- The **Tower** gives weather, vectors and landing clearance, and answers MayDay calls.
- **Dentist**, the forward radar controller on a UN-held island off the North Korean coast, reports
  MiG activity, sometimes unasked.
- The **FAC** runs close air support.

"Begin your Run" and "Leave Area" work on strike missions even when there is no FAC. With **Auto
Vectoring** off (Preferences → Others), a group leader is offered tactical choices when combat
starts. Pick one in a few seconds, or the pilots decide for themselves.

---

## 11. Cockpits and instruments

Every cockpit is modelled. Use the number pad or the joystick hat to look round the panel. In
combat, watch four things: **speed, altitude, fuel and rounds**.

**US jets (F-86, F-80, F-84)**

| Instrument | What it tells you |
|---|---|
| Accelerometer | With G effects on: blackout at high g, red-out at negative g; the wings fail at +13 g |
| Thrust | The small needle reads 0–50 % (subtract 50 from the dial); the large needle reads 50–100 % |
| Exhaust gas temperature | About 650° normally; a jump means a too-fast throttle movement |
| Altimeter | Height above the home field (small needle 10,000 ft, large needle 1,000 ft) |
| VSI | ±6,000 ft/min |
| Fuel | Total and internal fuel in pounds; the external-fuel light shows tanks aboard |
| Fire lights, low hydraulic/fuel/oil pressure | Engine damage |
| Gear handle/light, flaps light, gun-camera light, rounds counter, Mach, slip and turn, gyro compass, artificial horizon | as named |
| Gun-sight range dial, wingspan wheel, range light | See [section 8](#8-weapons-and-the-gunsight) |

The **F-51D** adds piston-engine gauges: manifold pressure, tachometer, coolant and carburettor-air
temperature, a combined fuel/oil gauge, suction, and remote and standby compasses. It also has the
two weapon switches.

The **MiG-15** panel is Russian-pattern: oxygen and cabin pressure, airspeed, altimeter and radar
altimeter, horizon, turn indicator, rate of climb, fuel, radio compass, gyromagnetic compass, engine
speed, temperatures, gear and flaps lights, and rounds counters.

---

## 12. Air combat: MiG versus Sabre

| | MiG-15 | F-86 Sabre |
|---|---|---|
| Climb and acceleration | **Better at every altitude** (lighter, higher thrust-to-weight) | |
| Ceiling | **Higher**; MiGs choose when to fight | |
| Top speed at low level and in the dive | | **Higher**; the Sabre can go supersonic in a dive and the MiG cannot |
| Sustained turn | **Better** (10.0 vs 8.2°/s at 20,000 ft) | |
| Roll rate | Half to two-thirds of the Sabre's, and worse at high speed | **About 180°/s** at all speeds |
| Handling | Poor yaw stability; anhedral reverses rudder roll; wing drop at high speed and low level; vicious stall | Refined; no tendency to yaw |
| Guns | Heavy, slow shells that need more lead; one hit usually kills | Fast-firing .50s; easier solutions, many hits needed |

Both types pitch up at high angles of attack as the wing tips stall, and both lose control power
near Mach 1.

**Energy.** Specific power is Ps = v(T − D)/W. Ps = 0 means you can hold your state. A positive Ps
can buy climb (100 ft/s = 6,000 ft/min) or acceleration. The game's figures are worked out from its
own flight model:
- Peak instantaneous turn at 20,000 ft: about 17°/s at Mach 0.72 (F-86E), 18.4°/s at Mach 0.67
  (MiG-15).
- Sustained turn at 30,000 ft: 5.2°/s (F-86E), 6.2°/s (MiG-15).
- Minimum turn radius for the F-86E at 20,000 ft: about 2,000–2,500 ft.

**How each side fought.** MiGs fight in the vertical: slash from above, then zoom back up. Sabres want
turning and diving fights low down, where the MiG's advantage shrinks. In MiG Alley, fights that end
at ground level are the rule, not the exception.

**Communist tactics to expect**
- **Defensive split:** half the group breaks left and half right, one high and one low. The half you
  don't chase comes back onto your tail.
- **End run:** a decoy group draws the Sabre patrol away so a larger force slips through to the
  fighter-bombers.
- **Zoom and sun:** the MiGs orbit at 48–50,000 ft in the sun and make a single diving pass.
- **Roundabout / yo-yo:** a high orbit that drifts toward you, sending MiGs down one at a time.
- **Waiting game:** the MiGs arrive as the Sabres' 20 minutes on station run out. Counter it by
  sending patrols in Waves.
- **Pre-emptive move:** feint attacks that make fighter-bombers jettison their stores early.
- **Upper cut:** ground-camouflaged MiGs wait low and behind two decoys.
- **Pincer:** MiG forces go down both coasts, then turn north and catch tired, low-fuel aircraft
  heading home.

**USAF combat rules**
- Be fast when you sight the enemy.
- Drop your tanks early. If a tank won't come off, go home with your wingman.
- Watch the sun.
- Never take your eyes off a target you are engaging, but see the whole picture before you commit.
- Turn into an attacker just before he reaches gun range (2,500–3,500 ft); climbing in the turn can
  tighten it.
- Stay at 100 % and leave the air brakes alone.
- Don't fire without positive identification. A swept wing may be a MiG or a Sabre. A straight wing
  may be an F-84, F-80, F-51 or Yak-9.
- Check fuel (**R → 1**) and leave when the lowest aircraft nears bingo.
- Break off when your wingman tells you to.
- Don't panic.

**Wingman.** The element (leader plus wingman) is the smallest fighting unit, and a wingman must never
lose his leader. **Alt+F2** locks your view on him.

---

## 13. Ground attack

- AAA caused more UN losses than any other factor. Communist guns moved, and flak traps were baited.
  On heavily defended targets, send a flak-suppression Wave in first.
- Supplies were hidden: dispersed, under trees, even buried among crops. Destroy as many of the
  candidate buildings as you can.
- Put the IP within four miles of the target so it is in sight when accelerated time ends; padlock
  (**F3**) works at once.
- Stores drag and weight (Preferences → Flight) slow a loaded aircraft. Drop your tanks (**Ctrl+F**)
  before a fight.
- Use **F11** to watch your bombs land, and **V** to start the gun camera.

---

## 14. Gun camera and replay

The gun camera records your flight for replay, or to save part of it to disk.

- **Preferences → Views → Gun Camera** sets *Off*, *Trigger* or *On*. In Trigger mode the camera
  keeps running for a while after you release, longer after a bomb release than after gunfire.
- **V** toggles the camera and **X** resets it, overriding the preference.
- **Preferences → Views → Camera Colour** gives monochrome (realistic) or colour film.

**On the Replay screen** the icons work left to right, and number keys select them (**1** = the
first icon):
1. Rewind to the previous marker
2. Rewind one block
3. Back one frame (paused only)
4. Play / pause
5. Forward one frame (paused only)
6. Forward one block
7. Forward to the next marker
8. Save the marked block
9. Reset the markers
10. Exit
11. Set the start marker
12. Set the end marker

The function-key views work in replay; cockpit views don't, because instruments are not recorded.
Applying a patch invalidates saved recordings.

---

## 15. Multiplayer

**Game types**
- **Death Match:** people only, every aircraft available, eight starting scenarios.
- **Team Play:** people only, UN against Red, eight scenarios.
- **Quick Missions:** players plus AI aircraft in the Quick Mission scenarios.

- **Campaign:** the co-op campaign. Rowan switched it off days before release in 1999; the port restores it.

**On Linux** the port replaces DirectPlay with its own UDP transport. The four connection services of
1999 (IPX, TCP/IP, modem, serial) are not used.

**Before you start**
- Use the **same AppImage** on both PCs.
- The host opens UDP **47624** (`sudo ufw allow 47624/udp`).
- The game has **no address box.** The joiner names the host with an environment variable.
- **Two players** have been tested, on a LAN only.

**Host**
1. `./MigAlley-x86_64-<ver>.AppImage`
2. Title → **Multi-Player** → **Create Game**.
3. Enter your Name and a Session name, and pick the Game Type. **Select Side** appears only for Team
   Play and Quick Missions.
4. Press **Continue**.
   - Death Match / Team Play: you are now in the **Ready Room** (the 1999 manual's *Flight Line*),
     and the game is open to the joiner.
   - Quick Missions: set up the mission first, then press **Ready Room**.
5. Press **Fly**. In Quick Missions: **Frag** → click your seat → **Fly**.

**Joiner**
1. `MA_DPLAY_HOST=<host's LAN IP> ./MigAlley-x86_64-<ver>.AppImage`
2. Title → **Multi-Player** → **Join Game**.
3. Click the host's session row, then **Select**.
4. Enter your name, pick a side if one is offered, then **Continue**.
5. **Fly** (or **Frag** → a free seat → **Fly**).

`MA_DPLAY_PORT=<n>` changes the port; it must be the same on both PCs.

**In the Ready Room**
- **Chat:** type and press Return. In Team Play and Quick Missions you can send to your side or to
  everyone; click a name to send to that player only.
- **Name colours:** your name is white, other players green, AI pilots yellow, and the dead grey
  with K.I.A.
- **Paintshop:** pick your aircraft and nose art (MiGs have none).
- **Radio:** edit your multiplayer messages; they are kept between games.
- **Brief:** the host edits the Quick Mission. Anyone already on the Frag screen goes back to the
  Ready Room to choose a seat again.
- **Prefs:** the host alone controls the difficulty settings.
- **Visitors' Book:** a player without the password clicks *Add my name*. The host opens Visitors'
  Book and changes *Excluded* to *Accepted*.

**In the air**
- **R** (or the cockpit map) gives **Comms Msg Recipient** (all players or your team) and **Comms
  Message** (send a numbered message).
- **Death Match / Team Play:** a dead player is resurrected and climbs on autopilot. **J** takes
  control at once; otherwise control returns at 30,000 ft or 2,000 ft above the highest aircraft.
  **Left Alt+S** cuts the death sequence short after a few seconds.
- **Quick Missions:** a dead player returns to the Ready Room and may take over a free AI seat from
  the Frag.
- **Restricted Views** (Preferences → Views) keeps every player in the cockpit, with no padlock or
  panning.
- Death Match and Team Play show scores in the Ready Room. Quick Missions end with a debrief of
  everything destroyed.
- Players who leave can rejoin while the host keeps the game running.

**Campaign (co-op)**
1. **Host:** in the Locker Room choose **Campaign**, a side (UN or Red) and the campaign, then **Continue**. You
   plan on the campaign map as in single player.
2. **Opening the session:** frag a flight. The session opens at your campaign Ready Room, and guests receive your
   campaign state as they join.
3. **Guests:** join, then take a seat on the Frag screen (MiG seats for Red players), and fly. After the debrief,
   guests return to their Ready Room and the host to the map.

Tested on one PC with up to three players, on both sides.

**Finding games: squeak (the Serious Games Week matchmaker)**
- Once per PC, choose the matchmaker: `sgw url http://<matchmaker-host>:8090`. The AppImage carries its own `sgw`;
  `pipx install git+https://github.com/sim-museum/squeak` puts one on your PATH.
- Hosting lists your session while it is open, and withdraws it when it closes. A campaign host's session opens at
  the campaign Ready Room.
- Join's session list includes the sessions the matchmaker lists, so a joiner needs no host address.
- A game is listed only on its day of the week (Tuesday for MiG Alley and Battle of Britain, in your own time
  zone). On other days the game still hosts normally; the log says why it isn't listed.

**Internet play**
- Lobby, chat, seat and launch messages are sent reliably: retransmitted until acknowledged, and delivered in
  order. Two players stay in step through 10% packet loss, which was measured with the loss simulator
  (`MA_NET_LOSS=10`, a percentage). `MA_NO_RELIABLE=1` turns this off.
- The host must accept UDP 47624 from outside (router port-forward). There is no NAT traversal.

**State of the port (2026-10-04, AppImage 261004)**
- **Working:** Death Match, Team Play, Quick Missions and the co-op campaign: joining in flight (not the campaign), respawn, chat,
  kill credit, reliable delivery and the squeak matchmaker.
- **Still open:** in Team Play, a player joining in flight on the side nobody started on is not seen by the host.
  Join the host's side, or start together. Two PCs and real internet play are untested since 19 September.

**Two copies on one PC.** Start the host first. Give the second copy `MA_HOME=~/ma2`, and put the
windows on different monitors: a fully covered window drops to 1 fps under XWayland and stalls sync.

**For a bug report**
- Run both sides with `MA_TRACE_DPLAY=1 MA_TRACE_AGG=1 MA_TRACE_UIDBAND=1 MA_TRACE_MPPOS=1`.
- `EnumSessions: probing` with no `found` line is a network problem (IP, firewall or port).
- After a crash, include `drive_c/MIG_ALLEY_LAST_ERROR*.LOG`.

---

## 16. Keyboard reference

The complete, machine-generated map is **[`KEYMAP.md`](KEYMAP.md)**: 174 actions and 529
key-and-modifier bindings, read from the game's own table (game file 0x7501), with descriptions
from `KEYMAPS.H`. Regenerate it with:

```bash
cd <install dir> && MA_DUMP_BINDINGS=1 MA_CONTROLS=/tmp/dump.cfg ./wmig    # enter a flight once
python3 ~/ma/tools/ma_keymap.py /tmp/dump.cfg ~/ma/SRC/H/KEYMAPS.H > KEYMAP.md
```

**Modifiers.** The game knows one modifier at a time: CapsLock, Left Alt, AltGr, Left/Right Ctrl, or
Left/Right Shift. Where a table says *Left Alt+X*, only the left Alt key works.

**Rebinding.**
1. `MA_DUMP_BINDINGS=1 ./wmig` writes `controls.cfg`, in the format `ACTION = <DirectInput
   scancode>[, <shift state>]`.
2. Edit a line.
3. Start the game; the file is applied after the game loads its own table.

`MA_CONTROLS=<path>` uses a different file, and `MA_TRACE_KEY=1` logs each binding as it is applied.
Note that dumping writes `controls.cfg`, so point `MA_CONTROLS` elsewhere if you have edits to keep.

**The keys you will use most**

| Flight | | Views | | Fighting | |
|---|---|---|---|---|---|
| Throttle | 1–0, `, =/- | Cockpit / outside | F7 / F6 | Fire | Space |
| Gear / flaps / air brake | G / F / B | Chase / satellite | F9 / F10 | Weapon next/prev/cycle | ] / [ / N |
| Trim | O / L / Shift+L | Padlock enemy / friend | F1 / F2 | Gunsight span / range | Q W / Y T |
| Rudder | Num0 / Num. | Padlock on/off | Enter | Radio | R |
| Wheel brakes | , / . | Look back | End | Any bandits / clear / break | A / C / Z |
| Pause / accel | P / Tab | Reset view | Esc / Num5 | Map | M |
| End flight | Left Alt+X | Virtual instruments | H | Eject | Ctrl+E |

---

## 17. Corrections to the 1999 manual

These come from the binding table the program actually loads.

| Function | 1999 manual | Program |
|---|---|---|
| Rudder | Number pad Insert / Delete | **Num0 / Num.**. Insert and Delete are sticky looks. |
| Fine throttle ±1 % | + / _ | **=** / **-** on the main keyboard. 0 % throttle is **`** or **\\**. |
| Mustang propeller | Shift+F6 | **Shift+0**. Shift+F6 switches the outside camera between fixed and floating. |
| Spin-recovery cheat | Ctrl+S (controls chapter), Shift+S (flying chapter) | **Left Shift+S** |
| Shorten the death sequence (multiplayer) | S | **Left Alt+S** |
| Exit | Alt+X | **Left Alt+X** (AltGr+X does nothing) |
| Glance at instruments (v1.1 notes) | Ctrl+5 or NumEnter | **Ctrl+Num5** or **NumEnter**. Ctrl+5 on the main row is *look up*. |
| 3-D detail | Ctrl+D up, Shift+D down | As stated, but the Linux port swallows D in flight; use Preferences |
| Zoom | "Number pad +/−" | **Num-** zooms **in**, **Num+** zooms **out** |
| Inside/outside toggle | (unlabelled in the table) | **Backspace** |

Keys the 1999 manual never mentions:
- **Ctrl+M** shows MiGs on the 3-D map.
- **Shift+M** turns radio text on and off.
- **Ctrl+V** turns the player's voice on and off.
- **Shift+Tab** accelerates time on the map.
- **Alt+F1** gives the AI enemy's view.
- **Left Ctrl+F6** is bound to `OUTREVLOCKTOG`, which the game leaves unlabelled; it appears to
  be an outside reverse-lock camera.

---

## 18. Preferences

Press **F12** in flight, or use the Main Menu. Settings that affect difficulty are the host's
alone in multiplayer.

**3D**
- **Display driver, resolution, gamma.**
- **Lowest frame rate** and **Auto frame rate.** Auto turns detail off to hold the frame rate, and
  overrides the detail settings. Its **Fast** option (v1.23) cures ground stutter on fast machines.
- **Ground and item shading, reflections, weather effects** (rain, snow, mist).

**3D II**
- **Filtering** (none, bilinear, trilinear, all), **transparency** (contrails, smoke), **texture
  quality** (a big effect on speed and on cockpit look).
- **Trees, routes** (small roads, rivers, rail), **aircraft and item shadows**.
- **Horizon fade and distance** (near fade is more realistic but slower), **contour detail**.

**Flight.** There are presets from Minimum to Maximum, and changing any single item sets Custom.
- **Flame-outs.**
- **Auto throttle:** holds position behind an opponent, only when you are in a good position.
- **Power boost:** about twice the real thrust.
- **Wind and gusts.**
- **Flight model:** Easy, or Realistic (realistic spins and quirks).
- **Airframe stress:** wings fail when over-g'd.
- **Stores drag and weight.**
- **Torque and slipstream** (propeller aircraft).
- **Spool-up:** Realistic or Fast.

**Game**
- **Weapons:** Realistic, or Unlimited (stations reload; **Ctrl+R** reloads by hand).
- **Vulnerable to fire, ground collisions, mid-air collisions.**
- **Complex AI pilots:** off flies only you and your opponent on the full flight model.
- **Accel off:** Tactical or Engage.
- **Target size.**
- **Autopilot skill** for the UN and Red AI pilots in campaigns.
- **Gun-sight ranging:** Manual, Perfect radar or Realistic radar.

**Views**
- **Restricted views:** cockpit only, no padlock or pan; meant for fair multiplayer.
- **Peripheral vision.**
- **Auto padlock:** with the inside padlock cockpit selected, it switches inside or outside as the
  target moves.
- **View mode:** panning or fixed.
- **Padlock when visible:** if on, padlock only targets in view; if off, any target within visual
  range.
- **Camera colour, info line, units** (Imperial or metric), **gun camera, head-up display**.

**Controls**
- The detected controllers, with calibrate and enable.
- What each axis does: stick, throttle, rudder, view pan, view pitch, range (zoom), 3-D pointer.
- Force-feedback effects (gunfire, buffet, aerodynamic stiffening, airframe) and the stick and rudder
  dead zones.
- With a Logitech WingMan Force, set *Spring Effect Strength* to 35 %.

**Others**
- **Volumes:** music, 3-D effects (the master for radio and engine), interface, ambient, radio
  chatter, engine.
- **G effects, injury effects, white-outs** (from the sun).
- **Auto vectoring.**

---

## 19. Troubleshooting

| Symptom | What to do |
|---|---|
| Joiner never sees the host's session | Check the host is in the Ready Room (Quick Missions too). Run the joiner with `MA_TRACE_DPLAY=1`: a `probing` line with no `found` means the IP, firewall or port is wrong. |
| Crash entering the 3-D world in multiplayer | Both sides need the same AppImage. Attach `drive_c/MIG_ALLEY_LAST_ERROR*.LOG` and both logs. |
| A key does nothing | Check it in [`KEYMAP.md`](KEYMAP.md): only one modifier counts, and some keys need the *left* modifier. Throttle keys are disabled while an analogue throttle is connected. |
| Ctrl+D / Shift+D do nothing in flight | Known port issue (MA-KEYD-1). Change detail in Preferences. |
| Two copies on one PC stutter or lose sync | Put them on separate monitors; a covered window runs at 1 fps. |
| Strange behaviour after changing graphics hardware | Delete `savegame/settings.mig`. |
| Keyboard dead in flight | Close any key-remapper or joystick-profiler program that grabs the keyboard (the 1999 advice still applies). |
