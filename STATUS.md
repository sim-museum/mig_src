# Mig Alley — native Linux (SDL2) port: STATUS

_Last updated: 2026-08-25 (**S206 — the replay plays; the save screen writes over the game's own replays**)._

- _**✅ PO-64 CLOSED, PO-VERIFIED:** "yes! replay moves!" S205's `SetEndOfFile` fix confirmed in
  play; the reader now walks the whole recording and ends on a clean EOF._
- _**⚠️⚠️ PO-65 NEW, with DATA LOSS.** The replay Save screen is drawn shifted off the LEFT edge and
  the save lands on an **existing shipped `.cam`**: two of the game's own replays were overwritten
  (98,568 → 38,145 and 173,131 → 38,145 bytes). **Both restored** from `~/sgl/TUE/afterGameReport/`,
  the only known pristine copy — treat it as an oracle, and never let a gate write to `Videos/`._
- _**Measured, real, but not the whole story:** `GetCurrentRes` sizes the front end from
  `GetWindowRect()`, which answers **800x600 whatever the canvas is** (`canvas 1920x1080` vs
  `window=800x600`), so every panel is laid out for a screen that is not the one being drawn — the
  fifth "two code paths disagreeing about one fact" here. It does **not** explain PO-65: the Replay
  screen's ink bbox is x 0..798, nothing clipped left. The PO's panel was at NEGATIVE x, so the
  post-flight path has a further cause (S105's `WINSH_MID` centre-origin is the suspect, unmeasured)._

- _**⭐⭐ S205 (PO-61/PO-64): `SetEndOfFile` was `{ (void)h; return TRUE; }`** — a compat stub
  reporting success and doing nothing. `Replay::OpenRecordLog` opens `replay.dat` with `OPEN_ALWAYS`
  and empties it through that call, so **the file was never truncated and every flight ever flown
  was appended to it** (measured 2,427,259 → 2,551,847 bytes across the PO's sessions). Playback
  starts at the FIRST block — stale, count never back-patched — read `numframes=0`, and could never
  advance: the PO's *"VCR controls don't work, no motion"*. **The transport was innocent all along.**
  Real `ftruncate` implemented; `MA_NO_TRUNCATE=1` reverts. Verified to the byte: one flight now
  yields **20,641 = 18952 + 963 + 66×11 exactly**, magic `78 56 34 12` at 18952, counts (66,0,65) at
  19905. New gate `port/replay_record.sh` with a negative control that catches a regression by
  reporting *"2 block headers ... playback reads the FIRST"*. **Awaiting PO retest of Replay/View.**_

- _**⭐ S204 (PO-61/PO-64): the replay VCR transport is FINE; the recorded block says it holds zero
  frames.** PO-verified from play: the replay no longer crashes, and pressing play activates the
  control but nothing moves. Measured interleaving settles the direction — `SEL_4 -> PLAY:
  PlaybackPaused=0` → `LoadHeaderID at 19915 → MAGIC MISMATCH` → `paused=1`: playback un-pauses,
  fails the block read, and **re-pauses itself**. `LoadFrameCounts` reports `numframes=0
  emptyblock=1`, so the block is treated as empty, no frames are consumed, and the reader looks for
  the next header where it stands. **The reader is right; the recorder wrote an empty-looking
  block.** `FRAMESINBLOCK=1024` means a short flight never fills one, leaving `Replay::StopRecord()`
  as the only writer of the real count — S205 asks whether it runs on ALT+X. **Not fixed**; new
  `MA_TRACE_REPLAY` instrumentation only, gates 5/5 + replay_screen PASS._

- _**⭐ S203 (PO-63): the title menu's lower rows were painted and unclickable.** The menu listbox
  is **105x100 and draws seven 28px rows = 199px** (ink measured at y=215/238/266/294/322/350/378
  against a control at y=210 h=100). Nothing clips it, and the **gold title screen shows the whole
  list too**, so drawing all seven is right — but every listbox hit test bounded the click by
  `m_maH`, so **rows 4–6 could not be clicked by any route**. **Row 4 is REPLAY**, which is why the
  whole `_Replay` subsystem had no gate and PO-61 no headless repro. Fixed by hit-testing the
  height paint covered (`Hosted::drawH`, from the control's own `GetListHeight()`); `MA_NO_DRAWH=1`
  reverts. **`parity_2d` 5/5 byte-identical** — the change can only widen what takes a click.
  New gate `port/replay_screen.sh` with a built-in negative control. ⭐ **The fourth paint-walk vs
  click-walk disagreement in this port** — after collection (S165), control type (S164) and row
  count (S166), now **extent**._

- _**⭐ S185 (PO-60): the window loses INPUT FOCUS on the resize-for-3D, so no key ever
  arrives.** SDL delivers key events only to a focused window and the port never handled a
  `SDL_WINDOWEVENT`; the 2D→3 transition changes size, border and position at once and the WM
  takes focus. Measured `focus=NO`. **This is why "tapping the brakes does nothing" survived two
  sprints as a brake bug** — the brake chain was provably correct and the keys were never
  arriving. Fixed with `SDL_RaiseWindow`; `MA_NO_RAISE=1` reverts._
- _**⭐ S181 (PO-57): `GetCaption()` was never implemented on ANY hosted control** — set only,
  never get, since bring-up — so the campaign name dialog read back an empty string and
  overwrote the player's name with it. PO-verified fixed._
- _**S182 (PO-59):** `DestroyPanel` deregistered one window, not the subtree, so the frag
  panel's `CFragPilot` rows were drawn over the title screen. PO-verified fixed._
- _**S183 (PO-61):** `GetShapePtr` had no bounds check — a `.cam` replay's shape number read an
  uninitialised pointer. **S184 (PO-62):** `SDL_WINDOWPOS_CENTERED` straddled a dual-monitor
  desktop._

- _**⭐ S176 (PO-53): the port enumerated joystick axes in SDL order; DirectInput enumerates
  canonically.** `SController::RemakeAxes` assigns roles FIRST-COME, so the third-enumerated
  axis becomes the throttle — the TWIST in SDL order — which pushed the SLIDER onto RUDDER.
  The slider rests at minimum, so the game read a **permanent full-left rudder**. Reported from
  play as *"it pulls to the left"*. Fixed (canonical order, instance still carries the SDL axis;
  `MA_JOY_SDL_ORDER=1` reverts); `GUID_Slider` defined at last. **PO confirmed calibration
  correct.** ⭐ **This was also PO-52:** full-left rudder ground-loops the aircraft, which is why
  every runway test sat at 20 kt at full thrust; it now runs **0 → 143 Kts**. Three causes had
  been published for PO-52 and all three were wrong — the PO, who had watched it spin, had the
  answer in one sentence._

- _**S174: the Wonju strike this epic built now FLIES.** Frag → Fly → 3D puts the player on the
  runway at **0 Kts / 4 ft**. ⚠ **It will not take off (PO-52):** at 100% thrust it accelerates
  **0 → 20 kt and plateaus**. Isolated by measurement, not argument — the throttle command
  lands (`thrustpercent=100`), the player has manual control (`controlmode=MANUAL`,
  `movecode=AUTO_FOLLOWWP`, so NOT on the AI takeoff rail, which was the first hypothesis and
  was wrong), the brakes are hold-to-brake so they are off, and the **flight model is fine**:
  the airborne Hot Shot start flies at **503 kt / Mach 0.84 / 15,966 ft** under the same build.
  The defect is the **ground roll**. New tooling for K11–K13: `MA_TRACE_HUD=<n>` (the flight
  model's own speed/alt/mach) and `BOB_AUTOFLY=takeoff` (throttle held, counting from `g_ma_in3d`
  rather than from process start — the old `throttle` mode spent all its taps in the front end
  on the campaign path)._

- _**S173: the frag screen hosts THREE `CFragPilot` sub-dialogs with identical control ids**, so
  `@CFragPilot` is ambiguous with itself — caught on the first run by **S171's** ambiguity
  warning, which was written for a different bug. New `@Class#N` names the Nth instance **by
  screen position**, not map order (which is by pointer). **K9 CLOSED** (`port/frag_review.sh`):
  `FlyableAircraftAvailable=1`, a 12-name roster, the callsign reaching the package
  (`callname 1 → 5 " Red "`) and the seat the player flies (`MMC.playeracnum → 4`, matching
  flight 1 slot 0). Correction: the callsign control is a **combo**, not an edit — K9 never
  depended on PO-16._

- _**⭐ S172: the port had never dragged anything, and that was deliberate.** S95 drove map
  clicks down+up in ONE tick specifically to keep `m_bDragging` FALSE, because
  `CMapDlg::OnMouseMove` was believed to deref `GetDC()` unchecked — true when written, false
  since compat grew a real static `CDC`, so the engine's whole press-move-release chain sat
  intact and unreachable. `CMapDlg::MaDriveDrag` issues the moves; `MA_MAP_DRAG` names
  waypoints and resolves them through the map's own `FindMapItem`. **K8 CLOSED**
  (`port/route_drag.sh`): the IP dragged onto the target lands **3.06 miles** away (the
  script asks ≤ 4, measured in the game's own centimetres), Egress moves, both report
  `dragging=1`, the map redraws them 4–24px from the drop, and **the target itself refuses to
  drag**. The instrumentation lied first — reading the after-position through `m_buttonid`,
  which the drop path rewrites, made two waypoints report byte-identical coordinates._

- _**⭐ S171: closing a campaign dialog leaked its whole control set into the hosted registry,
  still flagged visible.** `RDialog::EndDialog` tears down a SUBTREE; `CWnd::DestroyWindow`
  deregisters ONE window. After a close/reopen there were two live `CProfile`s and two live
  `CFlt_Task`s, and the id resolver picked **by pointer** — so two clicks naming one control
  reached two different controls. Fixed with S169's fchild/dchild/sibling walk. Behind it:
  `@Class` cannot disambiguate a dialog from its own corpse (the ambiguity warning now counts
  after the class filter), and **the separate `Load` click never did anything in any recipe** —
  `CLoad::OnSelectRlistboxfile` calls `OnOK()` when the row is already current, so `:r0` selects
  AND loads; the `#1056@CLoad` step had been clicking a destroyed dialog since S162.
  **⭐ And a gate reported PASS on a run that SEGFAULTED** — every assertion true, none of them
  about how the run ended. New `port/gate_lib.sh` (`assert_no_crash` + symbolised frames,
  `assert_recipe_ran`) wired into nine gates. The crash was S170's, latent: a spinner with an
  EMPTY list derefs NULL in its own `OnDraw`, under an `ASSERT` that `NDEBUG` compiles out —
  **and BoB's copy of the same engine file already carries the fix** (`RDH 29/10/99`).
  **K6 and K7 CLOSED**: `port/attack_pattern.sh`, `port/flak_suppression.sh`._

- _**⭐ S170: RSpinBut was the LAST R\* control type the port never hosted** — no CLSID branch, so
  every `InvokeHelper` on a spin button was a silent no-op and the control was never created,
  drawn or clickable. `SRC/compat/ma_olespin.cpp` hosts it. Two more doors were shut in front of
  it and both read as working code: **`CT_EDTBT` was drawn but inert** (the `F84 (2)` duty field,
  the only route into the squadron dialog — the **fourth** control type found missing from the
  click filter after S87/S140/S163), and **a `:rN` recipe addresses a row's CENTRE**, which on the
  five-column wave table is the AAA-cover column, so the gate was driving the flak tab while
  looking correct. New recipe forms `:rN.C` (cell) and `:-3`/`:-4` (title-bar OK/Cancel).
  **EPIC K step 8 now runs end to end** — `port/add_flight.sh`: Main Duty cell → duty field →
  ChooseSquad → spinner `1→2` → `SetFlights(3)` → **Mission Folder `Wonju Supply Dump  Bomb
  08:30  3`**. K5 closed._

- _**⭐ S168: `-Wl,--allow-multiple-definition` had been silently deleting four entire eventsink maps.**
  `BEGIN_EVENTSINK_MAP` named its auto-registrar by `__LINE__` and defined the constructor **out of
  line**, so the symbol had external linkage; two TUs whose sink map sat on the same line emitted the
  same symbol and the linker kept the first. **68 sink maps, four colliding pairs** — `SQDNLBUT/WPBUT`
  (waypoint buttons), `LISTBX/WAVETABS` (the wave tabs), `MAPFLTRS/MISSFLDR` (the Mission Folder,
  including **Frag**), `SERVICE/SESSION`. The losing class's every button drew, highlighted and did
  nothing; the winner registered twice. Fixed by keying the registrar on the **class** with an
  in-class constructor. **`--allow-multiple-definition` converts an ODR violation from a link error
  into a silent behavioural bug** — audit any macro that builds a symbol name from `__LINE__` or
  `__COUNTER__`. Cross-ported as §8-MA114 (BoB uses `__COUNTER__`, same property)._
- _**`[frag] FlyableAircraftAvailable=1`** — the Wonju mission is flyable, `LaunchFullPane(singlefrag)`
  runs, and the **pilot roster renders with `Map Fly Preferences`** (the gold's t≈305 frame). New
  instruments: `[evt_fire] NO HANDLER …` reports an unmatched dispatch and lists what IS registered,
  and `MA_TRACE_EVTREG=<class>` traces registration, filtered rather than capped._
- _New **PO-51**: the campaign map's OOB dialogs paint **over** the frag panel — the map idle keeps
  its paint walk running after a full-screen pane takes over. **PO-37** (front end does not fill 1920)
  confirmed on that screen too._

_Previously updated: 2026-08-22 (**S158–S166 — the Wonju mission builder is reachable through step 8's Task button**)._

- _**S166: `CRListBoxCtrl::GetRowFromY` clamped against the wrong list.** `m_playerList` is filled only
  by `AddPlayerNum` (multiplayer / player log) while rows come from `AddString` into `m_list`, so on
  every other listbox it answered **-1 for every row past the first**. The control's own
  `OnLButtonDown` clamps against `m_list` and is correct — **two opinions about "how many rows" inside
  one control.** Safe to correct because `GetRowFromY` has **no caller in the game tree**: its only
  consumer is the port's `#ID:rN` recipe resolver (S162), which was therefore silently limited to
  player lists from the day it was written. `MA_LB_PLAYERCLAMP=1` reverts._
- _K5 progress: the real `1.Bomb` wave row selects and the **Task button fires**; it does not yet open
  the TASKS dialog. S167 prints `currrow`/`currcol` in `OnClickedTask` to settle which of two
  candidates it is — measured, not reasoned, which is how S164 went wrong._

_Previously updated: 2026-08-22 (**S158–S165 — the Wonju mission can be found, reconnoitred, created and inspected; its wave folder now takes clicks**)._

- _**⭐ PO-50 CLOSED (S165): campaign dialogs logged on OTHER DIALOGS were painted and unclickable.**
  `ma_map_paint_oob` descends a second level of logged children (the wave folder is a child of the
  Mission Folder, not of `m_toolbar2`); `ma_map_click_oob` had only the first level, so every click on
  such a dialog fell through to the main toolbar underneath — clicking a row of the mission you are
  editing fired `IDC_OVERVIEW`. The click walk now mirrors the paint walk, grandchildren first
  (they are painted on top). `MA_NO_OOB_GRANDCHILD=1` reverts._
- _⚠ **S164's published cause for this was WRONG** ("the walk paints 3 of the 5 dialogs on screen") —
  a misreading of a per-frame counter. **A summary number was used to infer a set difference.** S165
  printed the two walks' node sets (`[oobrender]`, `[oobvisit]`) and diffed them, which settled it in
  two lines. Also third booking of **"filter, don't cap"**: the `[oobpaint]` trace was budgeted
  `if (_r++<40)`, spent it all on the first dialog tree, and that is what hid the answer from S164._

- _**⭐ K3 CLOSED (S163), and the blocker was much bigger than K3: combos inside every campaign-map
  dialog were DRAWN AND INERT.** `CT_COMBO` was missing from `ma_ole_toolbar_click`'s type filter —
  S87 (listbox rows) and S140 (scroll bars) one control type later, and the widest yet: the
  walkthrough's TASKS dialog alone drives **five** combos, PAYLOAD one, the frag two. **K5, K6 and K7
  were all sitting behind it.** Three parts: the click (open the dropdown), the draw (the open list
  is painted after the WHOLE OOB tree — per-dialog it is covered by the next dialog in the walk), and
  the dismiss (an open list gets first refusal on the next click and consumes it either way). Row
  arithmetic is shared with the front-end path, not reimplemented._
- _**Recipes: `:rN` now means "the Nth item of this control"** — a listbox row (`GetRowFromY`), a tab
  (`CRTabsCtrl::m_rectList`), or a row of a combo's **open** dropdown (the geometry paint recorded).
  Never a pixel. The combo form takes two entries on purpose — open, then pick — so the recipe walks
  the route a player walks._
- _Step 6 now shows what the dump is made of: warehouse groups and `SB Flak Site` rows, `Fully / functional`.
  ⚠ **The list overflows its dialog — PO-43 again, on a second dialog**, so that defect is not
  Intelligence-specific. New gates: `damage_elements.sh`, `authorize_mission.sh`._

- _**⭐ K4 CLOSED (S162): the Wonju mission exists in the campaign.** Authorize on the dossier opens
  the profile chooser (**Minimum Strike / Napalm Strike / Fighter Bomber Strike**) and Load creates
  the mission — the **MISSION FOLDER** then lists `Wonju Supply Dump  Bomb  08:30  2`. Gate
  `port/authorize_mission.sh`. **The recipe had been clicking the wrong row**: naming a listbox
  resolves to its centre, which on a three-row list is "Fighter Bomber Strike", the one profile the
  script says not to pick — and the mission was created anyway, so it looked right. New recipe form
  `#ID@Class:rN` resolves a row through the control's own `GetRowFromY`. On this save both profiles
  produce the same wave, so this corrects what the recipe **addresses**, not what it produces._
- _Two corrections to the S158 walkthrough, both from seeing the real dialog: the bottom-left dialog
  is the **MISSION FOLDER** (S158 named it from a gold frame cut off at `…DER`), and gold's `F84`
  against the port's `F80` is the campaign date, not a defect._

- _**⭐ S161: `§8-BoB181` described S159's PO-49 exactly and had been sitting in MA's own tree.** The
  shared notes are byte-identical in both ports and a guard proves it every sprint — but **syncing was
  being mistaken for processing**, so three BoB notes sat unanswered from S157 while MA rediscovered
  one of them from a play-test defect. The fix is structural: **`§8-LEDGER`**, one row per note with a
  per-port verdict (applied / N/A + reason / open + blocker), including MA's own unassessed rows.
  Verdicts shipped for BoB182 (N/A — MA implemented *neither* half of the enumerate/apply pair, and
  declining the desktop mode switch is correct here) and BoB183 (N/A — PO-1/S97). Notes MA 107–110
  sent._
- _**`port/ref/native/` is 5 oracles and 50 pictures**, and now says so per file. Only `parity_2d.sh`
  reads this directory. The rest are undated sprint snapshots — several showing defects since fixed;
  the `1021×644` captures predate the canvas-overhang fix and **that size is the bug**. Not refreshed
  wholesale: their recipes were never recorded. If you need a reference to be true, gate it._

- _**⭐ K2 CLOSED (S160): the 3D recon of a target renders.** The dossier's **Photo** button hung the
  game. Run under gdb (`ptrace_scope=1` blocks attaching), **thread 11 had already taken SIGSEGV in
  `Inst3d::moveloop` while thread 1 was still inside `Inst3d::Inst3d(bool)`**, down in
  `Three_Dee.InitialiseCache()` building the landscape cache the worker reads. The map-view ctor
  starts its sim thread ~40 lines before the members that thread reads exist. **S69 fixed this exact
  race in the no-argument `Inst3d` twin and the fix never crossed the 100 lines between them** — when
  a fix is a reordering inside a constructor, look for the constructor's twins. Every headless gate
  we own sets `MA_DISABLE_3D=1`, and with 3D off the photo dialog never launches 3D, so nothing could
  see it. New gate `port/recon_photo.sh` (negative control checked); `stress_launch` 20/20._
- _**K1 CLOSED (S160): map items are addressable by NAME.** `MA_MAP_CLICK_NAME=Wonju` finds the dump
  through the game's own `GetTargName` — a band cannot pick one of twenty `AmberSupply` items. The
  dossier matches the PO's script **on content**: it predicts "no MiGs expected, but a large AAA
  presence" and the port reads **Threat AAA High / MiG 15 Low**, MSR Central._

- _**⭐ PO-49 CLOSED (S159): the campaign dialogs' backdrop art is clipped to the dialog.**
  `RDialog::OnPaint` handed `SetDIBitsToDevice` the **bitmap's** width and height, never the dialog's
  — Windows clips painting to the window and this port has none. The target dossier's backdrop was a
  **540×602 bitmap in a 327×316 dialog**, hanging a 286 px skirt over the map the player uses to pick
  a target. Reported against one dialog; an A/B over the OOB sweep shows it was **9 of 9** (bases
  172,230 px of map reclaimed, intelligence 113,635, overview 43,623, weather 39,198 …). `MA_NO_ART_CLIP=1`
  reverts; `MA_TRACE_OOB` prints one `[artclip]` line per clipped node. **PO-43 is untouched and still
  open** — that one is a `ResizeToFit` listbox, not art._
- _**A gate was reporting on itself.** `asan_campaign.sh` returned "NO-MAP / INCONCLUSIVE", which reads
  like the campaign regressing. An A/B with the fix disabled failed identically; the cause was the
  gate's **hardcoded pixel** navigation (the S62/S63 trap). Switched to the symbolic `f,rN` / `f,#ID`
  recipe its siblings use → MAP-OK 2/2, 0 ASan reports. `ab.sh`, `asan_flight.sh` and `hw_gate.sh`
  still navigate by pixel and are logged for the same treatment._

- _**New gold: the Wonju supply-depot attack.** `~/gold standard/ma/wonju_attack.mp4` (344 s) plus the
  PO's own written walkthrough `wonju_script.txt`, added with the intent *"as a test of campaign I will
  try to create and run this mission in linux MA."* This is the port's first oracle for a **whole
  workflow** rather than one widget on one screen: nine dialogs, four combo boxes, a drag-editable
  route. **EPIC K (K0–K13, 75 pts)** in `scrum.md`; timeline in `port/scrum/wonju-walkthrough.md`;
  frames via `port/tools/gold_video.sh … wonju`. ⚠ **The recording stops at the frag screen — it covers
  building the mission (steps 4–14) and none of flying it (15–18).**_
- _**Map items can now be addressed by CLASS.** `MA_MAP_ITEM_SCAN` prints each item's UID band by name
  and tallies the classes present (**20 AmberSupply**, 22 AmberBridge, 5 AmberAirfield on the pinned
  save), and `MA_MAP_CLICK_BAND=AmberSupply` picks the click target by class — the walkthrough starts
  at a supply dump, and the old "click the first item" hook lands on a bridge. The supply dossier opens
  correctly (*Sukchon Warehouses*, Details/Damage/Notes, Center/Zoom/**Photo**/**Authorize**)._
- _**PO-49, found by measurement:** every dossier's backdrop art paints ≈394×575 for a dialog that
  reports **330×320** — a 281 px skirt below the button row. PO-47's shape (*the dialog is not
  oversized, the ART is*) one screen further on. Note that S155 already tried clipping the OOB **node**
  rect and reverted it, so the fix must clip the art blit itself._

_Previously updated: 2026-08-17 (**S157 — a cross-port MEASUREMENT, deliberately not a cross-port fix**).
The sister Battle of Britain port spent a night on Product-Owner defects and three of its findings
were checked against this tree:_

- _**Not applied: the D3D→GL viewport-origin flip.** BoB S173v flips it (DirectDraw measures `dwY`
  from the top, `glViewport` from the bottom) and MA's `DEV_SetViewport` is the same function,
  unflipped — a one-line cross-port on the face of it. It was **not** applied, because the flip is
  **inert for a full-screen viewport** and only matters if the game sets a sub-viewport, and MA's
  front end needs real mouse clicks to reach 3D (`MA_CAMP_FLY` alone does not navigate there from
  the title), so the measurement that answers "does MA ever set one?" could not be taken. Shipping
  it blind would risk a working renderer for a defect not shown to exist here. Landed instead:
  **`MA_TRACE_VIEWPORT=1`**, one line per distinct viewport rect, labelled `[full-height: flip
  inert]` or `[SUB-VIEWPORT: flip MATTERS]`. The next session that reaches 3D answers it in one run._
- _**MA is the reference design for BoB's worst bug this session** (§8-MA104). BoB froze whenever
  the game moved to a full-screen page: its idle tick chose a renderer from a **port-owned** flag
  ANDed with the **game's** page state, and when the two disagreed neither branch painted. MA's
  dispatch — `fp ? paint the pane : page==0 ? paint the map` — branches on the object that actually
  exists, so it cannot express that bug. Rule recorded: derive "which subsystem owns the screen"
  from the game's own state, never from a port-side mirror._
- _**BoB's box-derived font bug cannot occur here** (§8-MA106). BoB's `CDC::SelectObject(CFont*)`
  discards `m_height` and lets each drawing site invent a size (a 14px font rendered at 36px, and
  the controls' own shrink logic truncated captions in response). MA passes the real font through
  to a GDI object model — `ma_gdi_set_font(m_hDC, f->m_hObject)` — so the size the game chose is the
  size drawn. When one port has a class of bug the other cannot express, the difference is usually a
  missing abstraction rather than a missing fix._

_Shared lessons doc resynced (`port/BOB_PORT_LESSONS.md` == BoB's
`doc/ROWAN_ENGINE_LINUX_PORT_NOTES.md`, guard green). No MA game-code or renderer behaviour changed
this session; the only new code is the env-gated trace._

_Previously updated: 2026-08-16 (**S134–S151, one continuous night of campaign-GUI work**). The
campaign UI is now largely correct against the gold captures. Delivered this session:
context-sensitive help (S134); the map **distance ruler** and a **53-site `sprintf("%s",CString)`
varargs bug class** whose loss of text explains most "missing text" reports (S135); **RRadio
hosted** + plate-button captions, which filled the D.I.S. dialog (S136); the **map filter buttons**,
dead from a range-registrar cap plus a missing button toggle (S137); a **real modal loop**, so
quitting asks Save/Yes/Cancel instead of throwing the player out (S138); **RScrlBar hosted**, so
campaign lists scroll (S140); the **black box behind every front-end list removed** — it was in
four of our five parity references, which had to be rebased against gold (S143); the **filter rows
moved off the system box**, which is what made the upper-right controls unclickable (S144); the
campaign map **filling the screen** at last (S145); panel teardown, ending the stale-dialog ghosts
(S146); the missing **D.I.S. briefing window** (S142); and the **ADI working in hardware** (S150).
Verified: ASan clean on the campaign path, hardware flights 4/4, full gate suite green with two
new gates (`map_filter.sh`, `dialog_scroll.sh`). Open: PO-37 (front end does not scale at 1920 —
measured, no fix chosen), PO-23 (runway smear), PO-25 (white objects, not reproduced)._

_Previously updated: 2026-07-27 (S59 — **the Quick Mission carry-overs fell to the installed
template itself**: the #9 "stray combo" is the dead-coded Cloud/Weather cluster parked at
dlu x=367–389 on a **335-dlu-wide** dialog — Windows clips children to the parent rect,
the host now routes it (`ma_dlg_never_visible` draw/click filter); the phantom "I.D."
label was NOT a resource delta but a **`!WS_VISIBLE` template control** (style dword now
parsed and routed as the initial show state, runtime `ShowWindow` still overrides);
mission text **word-wraps** (compat `CDC::DrawText` now implements `DT_WORDBREAK` —
CRStaticCtrl always asked for it). Uninit-PX ctor audit widened to
RSTATICC/RBUTTONC/RCOMBOC/REDTBTC (S58 class): prefs large-font value rows fixed, tickbox
glyph in its box. The dummy==GL `cmp` bar caught a SECOND class: the DI system mouse was
enumerated only when the SDL window existed → prefs-Controls "3d Pointer" read "Keyboard"
headless vs gold's "active mouse : X-Axis & Y-Axis" — device presence now unconditional
(Windows semantics). #9 → CLOSE-minus (one named deviation left: RRadio row, OCX not
hosted). Refs refreshed #3/#4/#5/#7/#9. Cross-port: BoB note 17 processed; **MA note 17**
sent (template-visibility routing + DrawText + device-presence finds); §8f addendum
synced byte-identical both sides.) Prior: S58 — **the S57 parity fixes are capture-proven and the 2D
oracle went display-independent**: `MA_SHOT=N` dumps the GDI canvas headless
(`SDL_VIDEODRIVER=dummy`, no GL), and after the sprint's root-cause fix the dummy-run
canvas is **byte-identical to a GL-run canvas**. The S58-salvage "strip artifact" was NOT
the membership filter: compat `PX_*` are no-ops, so `CRListBoxCtrl` members set only by
`DoPropExchange` (`m_bLockTopRow`/`m_bBlackboard`/`m_bCentred`/colours…) held
**environment-dependent heap garbage** — ctor now inits all persisted members to PX
defaults (`RLISTBXC.CPP`; also fixed the title menu's doubled captions, long mis-filed as
a font delta). Parity verdicts flipped on real captures: **#7 prefs Controls → CLOSE,
#8 prefs Others → CLOSE, #1 title first-captured → CLOSE** (labels/Calibrate/axis
names/tickboxes all verified in-capture; `port/ref/native/` refreshed). #9's stray combo:
S57 filter hypothesis disproven — the control IS in the installed template (runtime-hide
mechanism still open). Gates re-run post-wedge: `asan_all.sh` (+ new `-k`/KILL timeout
hardening) + stress launch. Cross-port: BoB note 16 processed (bag-layout slices checked
— no MA symptom, not adopted); **MA note 16** sent (PX-defaults trap + byte-identical
acceptance bar); §8f addendum synced byte-identical both sides.) Prior: S57 — **BoB's PE parity-oracle resource layer adopted** (note 14 / §8f): dialog labels/captions/art now come fully from the installed BDG 0.85F modules — template-driven static hosting (fixes the #7/#8 missing prefs labels at root), IDS→BDG-string-table caption resolution, template-membership draw/click filter, tickbox art+glyph, **REdtBt OCX newly hosted** (prefs-Controls Calibrate), DI axis names; all headless-verified against miglang.dll (production-TU harness), `MA_NO_PE_RSRC=1` reverts (A/B byte-identical). **GLX wedged machine-wide → in-game re-captures + asan/stress gates deferred to the next GL session; parity verdicts #7/#8/#9 not yet flipped.** Prior: S56 — **EPIC I Wine-parity oracle stood up**: all 14 PO gold shots + I4 Player Log gold inventoried with per-shot verdicts in `port/scrum/screen-parity.md`; 13 native captures in `port/ref/native/`; oracle provenance = **BDG 0.85F patched build** (resource deltas ≠ render bugs); IMAGEMAP.CPP LBM bounds fix A/B-proven (kept, `MA_LBM_NOBOUND`/`MA_TRACE_LBM` gated); new `MA_OOB_PLAYERLOG=1` headless hook opens the Player Log OOB dialog on the campaign map for capture — photo art renders, frame/tabs/stats table still missing (I4 open)). Prior: 2026-07-06 S45–S54 campaign UI; 2026-07-05 cross-port BoB S63→S82; 2026-06-29 BoB S46→S62 ASan arc._

> **⚠ Tooling note (2026-07-06 session):** the Bash tool was returning exit 1 for every command (confirmed
> environment-wide, incl. a subagent) — so this STATUS write could not be `git` committed/pushed in-session,
> and the S54 `AddMission` fix + note-8 (`BOB_ZDEPTH`) adoption await a working shell. Commit/deliver when Bash
> is restored. All *validated* work through S53 is already committed on `linux-port`.

Native **32-bit i386 ELF** port of the 1999 Rowan engine (OpenWatcom / Win32 / DirectX / MFC)
to Linux + SDL2/OpenGL. Branch `linux-port`. Game data: the Wine install at
`/home/admin/sgl/TUE/MigAlley/WP/drive_c/rowan/mig`.

> **Sister port:** Battle of Britain (`~/bob`), the same Rowan framework one renderer-generation
> later (D3D7/Lib3D vs MiG's software rasterizer). Shared cross-port field notes live in
> `/home/admin/bob/doc/ROWAN_ENGINE_LINUX_PORT_NOTES.md` (read its "MiG Alley specifics" box first).
> The two ports are at **near-parity**; knowledge flows both ways (see "Cross-port" below).

## One-line state

The game **boots to the native title screen, navigates the full single-player front-end, flies a
software-rasterized 3D mission and returns to the menu in one process, with OpenAL audio and
keyboard+joystick flight input.** Campaign reaches the operational Korea map; save/load round-trips.
Mouse drives the in-flight UI cursor. **Hot Shot air combat is playable end-to-end** (multiple
bogies, padlock view, kills register, debrief screen). Recent (S21–S28): live play-test hardening —
the campaign map is navigable, the F1-padlock crash is fixed, HUD instruments + ADI render, and the
quick-mission dropdown selects missions (Turkey Shoot / One on One now fly with their enemy).
**Front-end typography and chrome now match the Wine gold shots** (EPIC I): the game's own
faces (Intel for headers, Arial for data text), gold's blue-sans-label / yellow-sans-value
scheme, and translucent combo boxes all render natively (S63/S66/S69 — cross-cutting
deviations #1 font + #2 combo closed). **Parity #15 Player Log is CLOSED** — tab bar, title
bar, `?`/`✓` buttons, and the Career content table (Sorties/Combats/Kills/Losses per aircraft
type) all render (S70 added the missing OOB `CT_LISTBOX` draw case).

```
cd <drive_c>/rowan/mig && ./wmig          # bare launch — no env vars (S30/H1)
```
The data path is derived from the cwd's `/drive_c` ancestor and `InitInstance` auto-runs.
Overrides: `BOB_DRIVE_C=<dir>` to point elsewhere; `BOB_NO_RUN` for a link-only run.
(3D flight is default-on; `MA_DISABLE_3D=1` keeps it 2D-only for front-end debugging.)

## Subsystem state

| Subsystem | State | Where / sprint |
|-----------|-------|----------------|
| Compile (15/15 game unities) | ✅ | Phase 1 |
| Link (`wmig`, 0 undef) | ✅ | Phase 2 — 7.8 MB i386 ELF |
| SDL2 runtime + DirectDraw→GL present | ✅ | Phase 3 (`ddraw_legacy.h` bridge) |
| 2D front-end (title + OCX hosting + Prefs) | ✅ | Phase 4 / S2–S4 |
| Full single-player nav (QuickMission/Campaign/HotShot) | ✅ | S4 |
| 3D flight (software rasterizer) | ✅ | Phase 5 / S5 — first frame + menu↔flight round-trip |
| Keyboard flight (DirectInput→SDL) | ✅ | S3 |
| Joystick (SDL_Joystick→DirectInput) | ✅ | S10 — live fly-validated, axis-map fixed |
| Audio digital path (Miles AIL→OpenAL) | ✅ | S6 — `ma_openal.cpp` (SFX/UI/engine/radio) |
| Campaign → operational Korea map | ✅ | S7 — `StretchDIBits`; **renders full colour** (S45: the "greyish map" was a `BOB_DUMP_FRAME` `glReadPixels` pack-alignment bug at the 1021-wide map, not the render — fixed) |
| 3D/map colour fidelity | ◐ | S8/S20 — terrain matches Wine; **sky renders correct blue** (S8/S9 "brown" was stale, fixed by M2 `1a70d2d`); residual = ~75-unit brightness gap vs Wine's D3D-material sky (fidelity-target choice, low pri) |
| Campaign map toolbar (CRToolBar hosting) | ✅ | S48–S50 — icon toolbar renders (Bases/Squads/Weather/Dis/Frag/…) + clickable (fires `ON_EVENT` handlers). Fixed `CRToolBar` not inheriting `OnRowanMessage` (WM_GETFILE art) + control-id→icon table |
| Campaign OOB-info dialogs (Squadrons/Bases/…) | ◐ | S52–S54 — **7 of 10 render** over the map with real data (Squads: photo + Available Aircraft/Rotate Flights/Bingo Fuel). Fixed `CDialog::Create` dropping its parent (GetParent()==NULL in OnInitDialog) + unit-conversion FPE. Deferred: Authorise/Directives 2nd crash sites; selected-tab (CRTabs) |
| Save/load (click-driven loadgame) | ✅ | S11–S14 — "Auto Save" → Load → campaign map |
| ASan heap-bug oracle + flight-path grind | ✅ | S15–S38 — **flight path ASan-clean** (0 reports across 5 flights): per-frame corruptors (S15/16), mid-freq set (S17), base-item type-confusion pair (S37), lifetime UAF (S38). Residual = deliberately-benign `FixLbmImageMap` (BoB-guarded) |
| In-flight mouse (DInput rel→`AU_UI_X/Y`) | ✅ | S18 — DInput mouse device wired (mirror S10 joystick); motion reaches native `AU_UI_X/Y` cursor axis (verified `theaxis=4`) |
| MIDI/XMIDI music | ✅ | `SRC/compat/ma_music.cpp` — XMI→SMF in-memory (`parse_xmi`) → FluidSynth + the game's shipped `MUSIC/fieldsnr.sf2` |
| Smacker intro video | ⬜ | stubbed |
| DirectPlay multiplayer | ⬜ | out of scope (scrum.md §8) |

## Phase progress

| Phase | State |
|-------|-------|
| 1 — compile | ✅ all 15/15 game module unities compile clean |
| 2 — first link | ✅ `wmig` links, 0 undefined symbols (7.8 MB i386 ELF) |
| 3 — SDL2 runtime | ✅ boots into `CMIGApp::Run()`; SDL2 window + DirectDraw→GL present bridge |
| 4 — 2D front-end | ✅ title + interactive Preferences (OCX hosting, RLE8 BMPs, TTF fonts, tabs, write-back) |
| 5 — 3D flight | ✅ software rasterizer renders the cockpit (S73: **cockpit fully textured** = gold #10 — the stale-`palette_table` cockpit-black fixed at `BTREE.CPP:580`); menu↔flight round-trip; ◐ colour fidelity |
| 6 — input | ✅ keyboard (S3) + joystick (S10) + in-flight mouse (S18) |
| 7 — audio | ✅ digital path on OpenAL (S6); ✅ XMIDI music via FluidSynth (`ma_music.cpp`) |
| 8 — campaign/mission | ✅ reaches + renders operational map (S7); ✅ save/load (S14) |
| 9 — video | ⬜ Smacker → libsmacker |
| 10 — multiplayer | ⬜ DirectPlay → sockets (out of scope) |

## Live play-test hardening (Sprints 21–28, 2026-06-25)

Driven by interactive play sessions; each fix is committed + (where possible) validated headlessly.

| # | Fix | Commit | Notes |
|---|-----|--------|-------|
| S21 | **In-map navigation** | `88287a6` | campaign map: arrows/WASD/drag pan, wheel/`+`/`-` zoom, `Esc` exit, `F` fly. (Wheel-zoom window-resize is a known rough edge.) |
| S22 | **Turkey Shoot spawn measured** | `e65636d` | not a bug — bogie spawns dead-centre at FT_5000; drift is lawful dynamics. Added `MA_QUICKMISS`/`MA_TRACE_BOGIE`. |
| S23 | **F1-padlock crash, part 1** | `676eb14` | unclamped **horizontal** span → image filler OOB write. `polygon::ASM_Call_clamp` clamps span X for the 0-based image converters. |
| S24 | **F1-padlock crash, part 2** | `2ed87e6` | the real one: **vertical** OOB — a poly projected far below screen gave an off-surface `scradr`. `drawpoly` now clips `miny/maxy` to `[0,height)`. Crash handler upgraded to dump `fault_addr`+registers (`SA_SIGINFO`) — self-diagnosing now. |
| S25 | **HUD instruments default-on** | `716729c` | enemy-indicator disk + ADI were gated behind `GD_HUDINSTACTIVE` (the `h` key); default it on per flight in `MakePassive`. |
| S26 | **Lone-MiG no-spawn root-caused** | `0f0729f` | disproved the spawn-path theory; the bug was the mission combo (see S28). `MA_QUICKMISS` shipped as the interim workaround. |
| S27 | **ADI speckle glitch** | `f3a7393` | `DoArtHoriz` read the ball image out of bounds at pitch beyond ±90° → garbage texels. Wrap `offset` into `[0,h)`. |
| S28 | **Quick-mission combo selection** | `7f50acb` | **the lone-MiG fix.** Combo `TextChanged` went through the stubbed OCX connection point and never reached the dialog → mission selection did nothing (stuck on the no-enemy default). `ma_ole_click` now fires `ma_evt_fire` after any combo select, like buttons. Fixes **every** combo selection game-wide. |

Plus earlier same-session live fixes: `21ff9ec` 4× flight speed + Quit hang, `219a11c` flight-exit
crash (move-thread UAF + heap corruptors), `a1b5da7` Campaign-Begin map-render hang.

## Campaign UI arc (Sprints 45–54, 2026-07-05→06)

Autonomous headless-DoD sprints building out the campaign map's interface. Each committed + ASan-verified.

| # | Work | Commit | Notes |
|---|------|--------|-------|
| S45 | **Colour-fidelity fix** | — | the "greyish map" was a `BOB_DUMP_FRAME` `glReadPixels` `GL_PACK_ALIGNMENT` bug at the 1021-wide map, not the render. Adopted by BoB (unblocked their S101). |
| S46/S47 | **Map unit icons + date readout** | — | `CDC::GetBoundsRect` returned garbage → 0 icons drew; `GetClientRect` fix. Date/period header. |
| S48–S50 | **CRToolBar hosting** | `14d38a8`/`3f711f4`/`cbd240d` | parent-scoped toolbar draw (no bleed) → per-button icons (fixed `CRToolBar`≠`RDialog::OnRowanMessage` so `WM_GETFILE` art routed) → clickable (`ma_ole_toolbar_click`→`ma_evt_fire`→handler; Fly launches briefing). |
| S51 | **Cross-port: `CloseLoggedChild` guard** | `59aa042` | adopted BoB's S109 per-slot re-entrancy guard (Linux `OnCancel` no-op → infinite recursion) on both `CRToolBar`+`RDialog` variants. |
| S52 | **OOB dialog build crash fixed** | `cf3feed` | 2 general bugs: `CDialog::Create` dropped its parent (`GetParent()`==NULL in every `OnInitDialog`) + unit-conversion FPE (`mass.gm==0`). Squads OOB dialog builds clean. |
| S53 | **OOB Squads dialog RENDERS** | `48cf159` | `ma_map_paint_oob` walks the open logged-child tree each map idle → `MaOnPaint` art + `ma_ole_draw_toolbar` controls. Shows the squadron photo + real data. Mirrors BoB S113/S114. |
| S54 | **OOB render generalized + Directives diagnosed** | `10e6813` | verified 7/10 OOB dialogs render (Bases/Weather/Playerlog/Squads). Diagnosed the Directives crash (fnhoist var-shadow OOB-write in `COMIT_E.CPP AddMission`); fix reverted **unvalidated** (session SDL/GL/X11 wedge blocked the ASan gate) → deferred to S55 with the exact one-line fix. |

**S55 backlog:** apply+ASan-validate the `AddMission` fix; gdb the Directives 2nd site + Authorise; un-blacklist
the last 2 buttons; selected-tab render (CRTabs host); adopt BoB note-8 `BOB_ZDEPTH` for the 3D chase view.

## Cross-port with `~/bob` (sister Rowan port) — and now `~/free-falcon`

**New correspondent (2026-07-19): the FreeFalcon 6 port** (`~/free-falcon`, branch `develop`) has
joined the exchange — inbound **note 12**, `port/CROSS-PORT-FROM-FF-2026-07-19.md`. It is **not** a
Rowan-engine port (Falcon 4 lineage, 64-bit, no MFC/OCX, its own UI toolkit), so nothing `[ENGINE]`-
tagged transfers in either direction; treat it as a **class-level-only** correspondent. What came
across:
- Its **8-bug-class triage taxonomy**, folded into the shared lessons doc as **§7b**, annotated for
  how much each class actually bites a `-m32` Rowan port (two of the eight — 64-bit pointer
  truncation and `long`-in-binary-formats — do **not** apply to us; they are the mirror image of our
  own #1 recurring bug, the pack-struct ABI boundary, which FreeFalcon does not have).
- Two classes it flags hard for us: **`RAND_MAX` 32767 assumptions** (degrades silently — cost it
  months of misdiagnosed "broken AI"), and **silently default-returning compat stubs**. We have live
  instances of the latter (registry stubs, no-op `WritePrivateProfileString`); they are deliberate,
  but should be *listed as known-degenerate* rather than assumed harmless.
- Its **packaging** (`install.sh` + a self-verifying relocatable AppDir recipe) — the model for our
  `packaging/` (H1-pkg), adapted to i386.

**Given to FreeFalcon:** our Wine pixel-oracle discipline (`port/ref/wine/` + `ab.sh`), BoB's
objective band-statistics validation, and the `glReadPixels` `GL_PACK_ALIGNMENT` finding — its
"cannot capture 3D frames" impediment, open for months, turned out to be **stale**; it captured a
frame on the first attempt once it looked. Reinforces the rule from BoB's S101: *a diagnostic that
lies is worse than no diagnostic.*

## Cross-port with `~/bob` (sister Rowan port)

**Adopted from BoB:** refcount-UAF insurance (real `int ref` on `bob_video.cpp` D3D7 surfaces);
`INT3`-guards-are-data-bugs (fix the state, not the guard — our F3); menu↔flight one-process recipe
(`F12→CloseWindow→OnCancel→OnFlyingClosed`); CString-in-varargs Itanium-ABI fix (`FormatV`); the
ASan `new`/`delete` form-mismatch bug family (S16 cites BoB R1.3d/e, R3.9; S17 backlog cites R1.3b).

**Given to BoB:** general `ma_eventsink.cpp` (BoB adopting in its S33 to retire targeted bridges);
`ma_populate_software_modes` F3 pattern (BoB taking the approach for its resolution-combo crash);
the further-along campaign map view (BoB candidate for its R4.2 icon-culling fix).

**Watch (shared bug families):** the `fakefile` save-path family — MA has the same 3 sites BoB
flagged (`FILING.CPP` SaveGame:124 / LoadGame:138, `LOAD.CPP` MakeFileList:271) but reaches working
save/load **without** a `MA_LINUX` path bypass (the engine path + case-insensitive `fopen` resolve
it). If a save-path corruption ever surfaces, it's this known family.

**Cross-port sync 2026-06-29 (BoB S46→S62 ASan arc):** verified BoB's gameplay-loop ASan sweep against
MA's tree; three were **confirmed shared engine bugs** (see `port/BOB_PORT_LESSONS.md` §5 table):
- **`MathLib::rnd()` `rndlookup[55]` over-read** (BoB S55) — **FIXED** in MA `MATH.CPP:1722/1730`
  (`% table-size`; engine-wide PRNG, was latent for any mission).
- **compat `BITSET/BITTEST` dword-granular → byte-granular** (BoB S59) — **FIXED** in
  `SRC/H/mathasm_linux.h` (latent global-buffer-overflow for every sub-4-byte `MakeField` bitfield).
- **`LBMCPP.H` IFF unpack reads one control byte past the file buffer** (BoB S47) — **FIXED**: adopted
  BoB's `LBM_INBOUNDS`/`cend` macro (`LBMCPP.H` now byte-identical to BoB; real `cend` in
  `FixLbmImageMap`, inert sentinel in the uncalled generic `UnpackRow`). Rebuild + headless boot clean.
Not shared: BoB's `DrawSubShape`/`dodigitdial` shape-opcode `new[]/delete` (absent from MA), and its
`g_devTex` UAF / `~View3d` teardown race (DX7/Lib3D-specific — MA's software path differs).

**Cross-port sync 2026-07-05 (BoB S63→S66 campaign-serialiser + S71/S72→S82 arc):** triaged both against MA
(details in `port/CROSS-PORT-FROM-MA-2026-07-05.md`; shared-doc §5 addenda updated). Two real shared bugs fixed:
- **`PackageList::LoadGame` `new char[]`/scalar-`delete` mismatch** (BoB S65a) — **FIXED** in MA
  `MAPCODE.CPP:307/326` (`delete`→`delete[]`; `new char[SIZ=20000]` campaign-reload buffer). Compiled via
  `_BFIE.CPP`→`Mapcode.cpp` (symlink to `MAPCODE.CPP`, one file). BFIELDS unity recompiles clean.
- **`MIGLAND.CPP` two-strip terrain-index `pNorth/pEast[index+1]` OOB** (BoB S71) — **FIXED** on the flight
  terrain path: 4 active seek sites now wrap via `_seekNextIndex(index)=(index+1)%5120` (MA_LINUX). BoB's
  `& 0xFFF` mask does **not** port — MA's `north.ind`/`east.ind` are 5120 `SInfo` entries (not 0x1000), so a
  copied mask would corrupt Korea terrain; re-derived from MA's own data files. `_3D` unity recompiles clean.
- **Not shared:** BoB S64 `SaveBin` `char packstr[5]` overflow (MA's `SaveBin` uses a *different*
  `CSprintf`/`BOStream` serialiser, no such buffer); S65b `CDC::SelectObject(CPen*)` SUAR (MA's software-GDI
  applies the pen immediately + returns NULL, never caches); S78 `Formation_xyz` `wingpos[16]` (no such method
  in MA; scramble/formation model differs, cf. S54); S72 `Grid_Base::getWorld` clamp (no such symbol; software
  terrain path differs). S81 cockpit/cloud z-fighting = BoB GL-depth path — **watch** only if MA grows a
  hardware/GL 3D path.
- **Given to BoB:** MA's hosted-OCX front-end path (`ma_olecontrol.cpp`/`ma_ole_draw_all`/`ma_dlgtmpl.cpp`
  RT_DLGINIT/`ma_eventsink.cpp`) as the reference for BoB's stuck campaign Phase-1 (its campaign screens are
  OCX-hosted, not `textlists[]`-driven — the same control model MA already renders + navigates).
The three candidates were verified 2026-06-29:
- **`CRListBoxCtrl` cell-string `delete`** (BoB S58) — **shared, FIXED + ASan-validated**: `DeleteRow`
  (`RLISTBXC.CPP:2145`) did scalar `delete` on a `new char[]` cell → now `delete[]`. (MA's
  `ReplaceString:1746` was already correct, unlike BoB's.) Confirmed under ASan via a differential test
  (`MA_ASAN_LISTBOX_SELFTEST=1`, drives the real `DeleteRow` since no game code calls it): scalar `delete`
  → `alloc-dealloc-mismatch (new[] vs delete) at DeleteRow:2149`; `delete[]` → zero ASan errors.
- **`FindNextBf` `GR_Scram_*[8]` >8 groups** (BoB S54) — **NOT shared**: MA has no `glind`/unbounded
  scramble loop; the `[8]` arrays are touched only by a bounded `for(i=0;i<8)` clear + 8 fixed named refs
  (`refto8`). MiG's quick-mission scramble structure differs.
- **`LaunchScreen resolutions[m_currentres==-1]`** (BoB S57) — **already fixed in MA** independently
  (`FULLPANE.CPP:2037` `if(m_currentres==-1) m_currentres=GetCurrentRes();`, an "ASan(MA)" guard);
  `GetCurrentRes` returns `[0,5]` into the properly-sized `FullScreen::resolutions[6]`.

## Build & run

```
cmake -S . -B build -G Ninja && ninja -C build   # INCREMENTAL → build/wmig  (day-to-day)
bash port/rebuild.sh                             # from scratch → /tmp/wmig  (fallback)
```
`CMakeLists.txt` (2026-07-19) reproduces rebuild.sh exactly — same 270 TUs, same five
object groups, same six OCX modes, same `port/lists/*` build sets, same link line — but with
Ninja header-dependency tracking: one-`.cpp` edit ≈ 1 s, `ddraw_legacy.h` edit = the 6 TUs
that actually include it (`_HARD`/`SMKDLG`/`STUB3D`/`bob_video`/`bob_stubs`/`ddraw_stubs`, i.e.
Ninja derives the hand-maintained rule below automatically), full build ≈ 84 s, same as
rebuild.sh. `-DMA_ASAN=ON` in a separate tree mirrors `ASAN=1`. `ninja install-wmig` copies to
`/tmp/wmig` for the `port/*.sh` harnesses. **`port/rebuild.sh` is untouched and remains the
canonical/fallback builder.**
Link: `g++ -m32 -no-pie port/build/{obj,obj2,objmfc,objmfc2,objole}/*.o -Wl,--allow-multiple-definition -lSDL2 -lGL -lpthread -lm -o wmig`.
**Rebuild `_HARD`+`SMKDLG`+`STUB3D` on surface-layout changes; `ddraw_legacy.h` is inlined into
many TUs → full rebuild when editing it (`--allow-multiple-definition` picks one copy).**

## Diagnostics (gated, default off)

`MA_DISABLE_3D`, `MA_TRACE_3D`, `MA_TRACE_DD` (Blt src size/bpp/nonzero), `MA_TRACE_FILL`,
`MA_DUMP_BACK=N` (N-th back→primary Blt → PPM), `MA_TRACE_SKY` (fog/horizon colour), `MA_TRACE_KEY`,
`MA_TRACE_JOY`, `MA_TRACE_MOUSE`/`BOB_AUTOMOUSE`/`MA_NO_MOUSE_GRAB`, `MA_NO_AUDIO`/`BOB_AUTOFLY`,
`MA_TRACE_FPS` (per-second present fps + running average; B3 regression gate),
`MA_TRACE_REPLAY` (debrief replay-launch path; S33 Replay-hang fix),
`MA_TRACE_RES` (window resize / SetDisplayMode), `MA_TRACE_ADI` (attitude-ball draw geometry),
`MA_TRACE_PATHCACHE` (case-insensitive path resolver: per-lookup HIT/MISS/BYPASS/FLUSH + a
500-lookup summary with cumulative resolver walk time) / `MA_NO_PATHCACHE=1` (bypass the cache,
A/B escape hatch; still traced/timed so the two runs are directly comparable).
S21–S28: `MA_DISABLE_MAP`, `MA_QUICKMISS=<idx>` (2=Turkey Shoot, 3=One on One), `MA_TRACE_BOGIE`,
`MA_TRACE_SPAWN`, `MA_FORCE_PADLOCK=<frame>` (headless padlock repro), `MA_NO_HUDINST`, `MA_TRACE_CLIP`.
ASan oracle: see `port/scrum/asan-findings.md`. `MA_ASAN_LISTBOX_SELFTEST=1` drives the otherwise-
unreached `CRListBoxCtrl::DeleteRow` once (regression check for the BoB S58 `new[]/delete[]` fix).
Standing ASan gates: **`port/asan_all.sh`** (full suite: flight + campaign map/fly/nextday, one command),
`port/asan_flight.sh` (flight), `port/asan_campaign.sh` (save-load→map). Campaign auto-drives:
`MA_CAMP_FLY` (mission-gen→fly), `MA_CAMP_NEXTDAY` (day-advance strategic sim).
Save-date guard is **default-skip** (saves load across rebuilds; format is packing-stable, matches BoB); `MA_ENFORCE_SAVE_DATE` re-enables it.

## Known issues / next steps

- **3D colour fidelity (S20):** sky now confirmed correct blue (zenith ~(152,180,216)); the old
  "brown" defect was stale (fixed by M2 `1a70d2d`). Residual = Wine's near-horizon sky is ~75 units
  brighter (its D3D background-material brightening, stubbed in the software port). Fidelity-target
  choice (match D3D vs faithful software look), low priority — see `port/scrum/sprint-20.md`.
- **ASan flight-path grind — ✅ COMPLETE (S18 sub-epic closed).** The base-item type-confusion pair
  (`LauncherToWorld`, `InitROL`, S37) and the lifetime UAF (`PersonalThreat`/`mobileitem … T_nationality`,
  S38) are fixed; an instrumented Hot Shot flight now yields **0 ASan reports across 5 runs**. The full
  S15→S38 arc eliminated every flight-path heap error the oracle surfaced. Only known-remaining: the
  deliberately-benign `FixLbmImageMap` RLE over-read (bounded by the adopted BoB `LBM_INBOUNDS` guard).
- **ASan campaign save-load path — ✅ CLEAN (S40).** A new headless campaign-drive (`port/asan_campaign.sh`:
  title → Load Game → "Auto Save" → Load → Korea strategic map) sweeps `CFiling::LoadGame` →
  `Todays_Packages.LoadGame` → `PackageList::LoadGame` (the S65a site) → map render with **0 ASan reports** —
  so the cross-port **S65a** fix is now validated on a live load. Enabled by **`MA_IGNORE_SAVE_DATE=1`**
  (skips the build-date guard that otherwise voids every save on recompile; the save format is stable).
  Standing gates: `port/asan_flight.sh` + `port/asan_campaign.sh`.
- **ASan campaign mission-gen + fly path — ✅ CLEAN (S41), 2 real bugs fixed.** Driving a loaded campaign
  to fly (`MA_CAMP_FLY=1`) surfaced two campaign-only heap errors the flight/load sweeps never hit:
  `make_airgrp` (`Persons3.cpp:836`) `GR_Pack_TakeTime[w][gotgrpnum==-1]` **global-buffer-overflow**
  (negative group index → guarded), and `AddChildren` (`RDIALOG.CPP:537`) **stack-use-after-scope** (a
  named-local `DialBox`'s `edges` pointed at a dead `EDGES_` macro temporary → function-scope lifetime).
  Both fixed + re-verified 0.
- **ASan campaign day-advance / strategic sim — ✅ CLEAN (S42).** `MA_CAMP_NEXTDAY=1` (new hook forcing
  `OnClickedFrag2`'s no-flyable branch) drives `Campaign::NextMission`→`NextDay` (date advance / MiG
  rotation / stock replenish) →`ProcessAirFields`→`OnClickedNextPeriod` from the map idle — **0 ASan
  reports across 3 runs**. The `SaveBin`/`SaveGame` writeback was already swept by S41's frag2 else-branch.
  **The campaign path is now ASan-swept end to end** (load→map→mission-gen→fly→writeback→day-advance);
  only multi-day rollover over several days remains (low priority).
- **Higher-leverage next moves:** finish S8 sky-colour fidelity, or the deferred S17 item-type/lifetime
  ASan family — rather than grind the low-frequency ASan singleton tail.
- **Play-test backlog (queued, from S21–S28 sessions):**
  - **Radar gunsight doesn't range/expand** — the F-86 radar-ranging reticle (`DOGUNSIGHT` shape
    opcode, scaled by target range) stays fixed size; range/lock input likely not fed (software path).
  - **Debrief "Claims" table** — the first (player) column has no header label (`sairclms.cpp`).
  - **Replay hang — ✅ RESOLVED** (S33, validated in the 2026-06-29 PO play-test). The 3D loop's
    exit-key test was gated off during playback (`STUB3D.CPP:1438`); the unimplemented replay
    subsystem never ends playback → it could spin. Fix re-enables `EXITKEY` during playback
    (`MA_LINUX`, strict superset). PO session: replay loaded a live, responsive 3D view and exited
    cleanly (eject/back/quit, exit 0) — no stuck state.
  - **Replay *playback* unimplemented** — ⬜ **future epic** (separate from the hang). PO session:
    a (valid, pre-stored) replay loads only the start state (aircraft static, gear down); the
    recorded flight never advances and the VCR transport (kbd/mouse) is dead. Recording works
    (gun-camera ON writes a file). Full playback = driving the 3D view frame-by-frame from the
    replay data + wiring the transport — a substantial subsystem, same out-of-this-train tier as
    Smacker video / multiplayer. Recommend deferring; document as a known limitation.
  - **High-resolution support (S34, 2026-06-29):** the resolution combo now offers up to **1920×1080**
    (added higher 4:3 + 16:9 modes to `ma_populate_software_modes` + both DD enumerators; relaxed the
    4:3-only `IsValidMode`; windowed flight applies the selected mode via `Save_Data.displayW/H`). The
    window re-centers on resize and goes borderless-fill at desktop size. **3D flight is gorgeous at
    1080p** (software rasterizer); the **ADI kaleidoscope-on-bank is fixed** at all resolutions
    by baking roll (and pitch) into the ball image and drawing the quad axis-aligned
    (`OVERLAY.CPP DoArtHoriz`) — the ported `ma_xasm` texturer tiled any roll-rotated quad;
    resolving roll in a resample avoids that path. **C4 padlock target box + telemetry** (the
    Wine two-patch feature): **box ✅ PO-verified** — SHIFT+D (and `d`) toggle a red diamond that
    **scales with range to enclose the bogey** (`R_WORLD` projected extent, `OVERLAY.CPP DoCheatBox`);
    keys handled in the SDL layer (`g_adi_box`/`g_adi_telem`, bob_video.cpp) because the engine
    keymap binds `BOXTARGET` to `d`+no-modifier and rejects shift+d. **ALT+D telemetry 🔨 WIP** —
    toggle works and the beside-box `Rng`/`Alt` lines are computed from world coords (`SetViewVals`
    is never called, so `targRange`/`RelAlt` stay 0; World.Z is altitude), but the engine text
    path (`PrintAt2`→`PutC3`) renders a **white blob, no glyphs** in this overlay. **Next:** replace
    with a custom glyph renderer — **large fixed-size text (range-independent), no background box,
    black/white by background contrast** (PO spec) — then add **bogey-speed[closure] + own-speed**
    (C4d) and finalize the **adaptive color** (C4c). Pixel access: `currscreen->PlotPixel` (read/write
    565) or `DoClippedLine` strokes. **Remaining high-res 2D-layer issues** (the 2D overlays aren't
    resolution-independent like the 3D world): the **campaign map** tiles / scales with the wheel / icons
    not visible, and the **kneeboard** page renders blank. These need per-layer scaling work (deferred).
    **Ground truth now exists:** `port/ref/wine/07_campaign_map_planning.png` (added 2026-07-19) is a
    Wine capture of the campaign map at **1917×1077** — the map renders correctly at that size under
    the original, so these are port bugs in the 2D layer, **not** engine or resolution limits. Use it
    as the target picture (see `port/ref/wine/README.md` for the feature-by-feature checklist).
  - **Campaign-map wheel-zoom** resizes the window + patchworks tiles (present canvas tied to `m_size`).
  - ~~**Window title**~~ — ✅ fixed S31 ("Mig Alley (Linux native port)").
  - **Replay hang** and the items above are **interactive-repro-gated** — batch for a PO-driven
    play-test session (can't be DoD-demonstrated headlessly).

See `scrum.md` + `port/scrum/` for the sprint boards, `port/ROADMAP.md` for the completion plan, and
the `migalley-port-state` memory note for detailed per-blocker history.
