# EPIC I — Wine-parity screen inventory & verdicts (I1)

**Gold oracle:** 14 PO-supplied captures of the Windows build under Wine, taken 2026-06-24,
at `/run/media/admin/BEA6-BBCE/ma/` (immutable; referenced by timestamp suffix below).
Plus gold shot #15 (I4, PO-added): `/home/admin/Pictures/Screenshots/Screenshot From
2026-07-19 20-33-27.png` (Player Log over the campaign map).

**⚠ Oracle provenance (BoB S123 lesson, cross-port note 13):** the gold title screen shows
**"BDG version 0.85F"** — the oracle is the *BDG-patched* Windows build, NOT the 2000 source
checkout this port compiles. Known resource-version deltas (NOT render bugs): the extra
**BDG** tab in the Preferences tab bar (gold has `3d, 3d II, Flight, Game, Views, Controls,
Others, BDG, Back`; source resources have no BDG tab); possibly label wording ("Camera
Color" gold vs "Camera Colour" native — both real, resource delta). Judge data-level vs
render-level deviations separately. Gold prefs shots are ~1280-res; native captures below
are 800×600/640×480 — layout is resolution-relative, so verdicts judge layout/art/content,
not pixel dimensions.

**Native captures:** `port/ref/native/*.png` (committed S56; 2D set re-captured S58).
**S58 capture path:** 2D screens now capture GL-free via `MA_SHOT=N [MA_SHOT_PATH=…]`
under `SDL_VIDEODRIVER=dummy` (canvas→PPM at front-end idle N, exits) — **proven
byte-identical to a GL-run canvas at the same idle**, so 2D parity evidence no longer
needs the display. Legacy: `BOB_DUMP_FRAME=N BOB_EXIT_AFTER_DUMP=1` → `/tmp/bobframe.ppm`
(GL framebuffer); 3D via `MA_DUMP_BACK=N` → `/tmp/maback.ppm` (still GL-run only for
real scenes). Prefs-tab clicks (800×600 canvas): tab bar y=18, x: 3d≈15 / 3d II≈43 /
Flight≈88 / Game≈134 / Views≈179 / Controls≈237 / Others≈299 / Back≈345; Preferences
menu row (588,215); Single Player submenu: Hot Shot (585,215) / Quick Mission (585,231) /
Campaign (585,247).
**S63 — recipes are now FONT-INDEPENDENT.** Fixed pixel rows silently broke every recipe
when the reader changed the menu pitch (~16px → ~28px): the `quickmission` capture came
back showing *Preferences* and the campaign recipe never reached the map. `BOB_CLICKSEQ`
now accepts `f,rN` (menu ROW N, resolved at click time from the listbox's own metric) and
`f,#ID[:COL]` (hosted control by dialog id; COL indexes a horizontal listbox's items via
its own `GetColFromX`). Absolute `f,x,y` still works. Current recipes:
FLY=`"40,r1;95,r0"` · CAMP=`"30,r3;65,#1055;100,#2063:1"` (+ `MA_DISABLE_3D=1
MA_IGNORE_SAVE_DATE=1`) · PREFS=`"40,r0"` · QUICKMISSION=`"40,r1;60,r1"`.
Validation: with the reader OFF the row form reproduces the old hand-derived constants
(row1 → y=233 vs the hardcoded 231; row0 → y=217 vs 217).
**★ S80 — the gate is now ONE COMMAND: `port/parity_2d.sh`.** The recipes below had been
re-derived by hand from this prose every sprint; they now live in the script, which captures each
screen GL-free and pixel-compares it against `port/ref/native/`. Default set (all **0 px** at S80):
`title` · `prefs_3d` · `prefs_others` · `quickmission`. **Two traps it caught on its first run:**
(1) the `Others` tab x≈299 recorded above is **STALE** — a later font change moved the tab bar, so
that click silently captured the **Game** tab instead; the script uses the font-independent
`#2063:6` (`IDC_RLISTBOX` column 6) form. Treat every pixel coordinate in this file as historical.
(2) **`campaign_map` — retired at S80, RESTORED at S81, now 0 px and back in the default set.**
It renders live campaign **save state**, which this repo's own `MA_CAMP_FLY`/`MA_CAMP_LOOP` runs
advance on disk; at S80 it measured 8095 px different and the S60 A/B proved that was state drift
(the pre-S80 binary produced a byte-identical capture), so it was excluded. S81 found *why* the
state drifted — the autosave was being written and read under the truncated name `Auto Save.sa`
(`namenumberedfilelessfail` missing the long-name branch) — fixed it, and made the gate **pin** a
committed reference save (`port/ref/save/campaign_pristine.sav`) around the capture, restoring the
player's own save afterwards. Result: **0 px against the committed reference, which was never
wrong.** *Lesson worth keeping: pinning the state beats excluding the screen — an excluded screen
silently stops testing, a pinned one re-proves its reference every run.*

**⚠ #7 prefs_controls is NOT a stable oracle (S62 finding, S64 confirmed).** That capture
embeds LIVE JOYSTICK STATE: S62 saw it read "NOT CONNECTED / 0 axes" because no stick was
attached, and S64's runs show `[joy] opened 'Logitech Extreme 3D' axes=4 buttons=12 hats=1`
again. The reference therefore only matches when the same hardware is present, and a
mismatch is an environment difference, **not** a code regression. Until it is captured
against a synthetic device, treat #7 as environment-dependent and exclude it from the
byte-identical sweep — the other five screens carry the gate.

**Gold oracle location:** the `BEA6-BBCE` USB was NOT mounted this sprint; all 14 gold
shots are mirrored locally at `/home/admin/gold standard/ma/` and that mirror was used.

Verdict scale (BoB S123): **MATCH** / **CLOSE** (minor named deviations) / **PARTIAL**
(renders, major named deviations) / **GAP** (missing/wrong) / **not yet captured**.


## S82 note — the OOB dialogs now accept clicks (2026-08-08)

Captures of OOB dialogs no longer need the `MA_OOB_PLAYERLOG_TAB` scaffold hook to reach a
non-default tab: a click at the tab's own drawn rect switches it (`ma_tabs_click` → the control's
`SelectTab`), and a click on the title bar's tick dismisses the dialog through the owning dialog's
derived `OnOK`. Useful for parity work: a capture recipe can now drive these screens the way a
player does, with `f,x,y` on the tab row, instead of an env var that only the harness understands.
The scaffold hooks still exist and still work.

## Inventory & verdicts (S56 pass)

| # | Gold (17-…) | Screen | Native capture / repro | Verdict | Deviations (named) |
|---|---|---|---|---|---|
| 1 | 00-06 | Title screen (menu incl. "BDG version 0.85F" row) | `title.png` (S58 first capture: `MA_SHOT=30`) | CLOSE | No BDG menu row (resource delta); menu font GDI vs gold art typeface (cross-cutting #1). **S58:** the doubled/black-band menu rows seen in early S58 shots were the uninit-PX garbage (see below), NOT a font-path delta — menu now renders single centred captions |
| 2 | 00-45 | Preferences → 3d | `prefs_3d.png` (`BOB_DUMP_FRAME=…`, click Preferences) | CLOSE | Tab bar plain GDI text vs gold's art typeface (+ no BDG tab, resource delta); label font white serif vs gold blue sans; combo boxes black-filled vs translucent; values differ (settings state, not render) |
| 3 | 00-52 | Preferences → 3d II | `prefs_3d2.png` | CLOSE | As #2; label typo "Texture Qualtiy" is in the source resources (gold BDG fixed it: "Texture Quality") |
| 4 | 00-58 | Preferences → Flight | `prefs_flight.png` | CLOSE | As #2; layout/rows/rects correct |
| 5 | 01-04 | Preferences → Game | `prefs_game.png` | CLOSE | As #2 |
| 6 | 01-13 | Preferences → Views | `prefs_views.png` | CLOSE | As #2; "Camera Colour" vs gold "Camera Color" (resource delta) |
| 7 | 01-23 | Preferences → Controls | `prefs_controls.png` (S58 re-capture) | **CLOSE** (S58 verdict flip — every S57 fix capture-verified) | ✔ in-capture: "Input Devices:" (gold IDS wording), Calibrate (REdtBt), "Stick"/"Throttle"/"Rudder"/"Dead Zone:"/"Airframe" labels, axis names "Axis 0 & Axis 1"/"Axis 3"/"Axis 2" (live Logitech Extreme 3D enumerated), tickbox art + glyph on Enable/Use-for-FF. Residual (named): tick glyph renders literal "3" (Marlett-family glyph through the GDI fallback font — cross-cutting #1); combo chrome (#2). S59: the "View pan"-value-row large font FIXED (uninit `m_FontNum` PX default — R* ctor audit); "3d Pointer" row now reads "active mouse : X-Axis & Y-Axis" = gold wording (DI mouse now enumerated unconditionally — was gated on the SDL window, absent headless) |
| 8 | 01-32 | Preferences → Others | `prefs_others.png` (S58 re-capture) | **CLOSE** (S58 verdict flip) | ✔ in-capture: all 6 previously-missing labels render (Control SFX Volume / Ambient SFX Volume / G Effects / Injury Effects / White Outs / Auto Vectoring) via `ma_host_template_statics` (S57 headless verdict confirmed on IDD 271 statics 2023–2028). Residual: font/chrome cross-cutting #1/#2 only |
| 9 | 01-45 | Quick Mission setup | `quickmission.png` (S59 re-capture) | PARTIAL → **CLOSE-minus** (S59: 3 of 4 named deviations fixed) | ✔ S59: (a) **stray combo cluster ~(590,165) GONE — root cause found**: ids 2069/2246/2025/1118 (dead-coded Cloud/Weather combos + static + button) are parked at dlu x=367–389 on a **335-dlu-wide dialog** — Windows clips children to the parent rect so they can never paint; host now routes it (`ma_dlg_never_visible`, draw+click filters). (b) **"I.D." label GONE — was NOT a resource delta**: id=2023 is `!WS_VISIBLE` in the installed template (style 40010000); the host never read the per-control style dword — now routed as the initial show state (`ma_dlg_template_visible`). (c) **mission text word-wraps** in-panel (compat `CDC::DrawText` now implements `DT_WORDBREAK`; CRStaticCtrl always asked for it). Remaining (named): Scenario/UN/CH radio row missing (RRadio OCX `{5363BA22}` not hosted — backlog); font face cross-cutting #1 (native wraps to 3 lines vs gold 4, same template width, wider gold art font) |
| 10 | 02-21 | In-flight cockpit (F-86) | `flight_cockpit.png` (FLY + `MA_DUMP_BACK=220`) | **CLOSE** (S73 — cockpit-black FIXED) | **S73: the cockpit renders FULLY TEXTURED = gold** — metallic canopy arch, instrument panel, gunsight range drum (10/20/30/40), side knob, gunsight glass, ADI padlock inset now shows attitude content (was a black rectangle). **Root cause (S72's 3 hypotheses all REFUTED with gl-lock data — lighting: cockpitAmbient=landAmbient=(255,255,255); imagemap-load: 236 LBM bodies load, none black; palette-not-populated: XX_PalChange runs, palette_table populated for world polys):** on the software raster path the active 8→16bpp LUT (nasm `palette_table`) is left stale/empty by the preceding non-cockpit draw, and the cockpit's own `createpoly→SelectPalette(0)` no-ops because `polygon::selectedPalette` cache still reads 0 — so every cockpit imagemap/flat texel indexes an EMPTY LUT and resolves to a near-0 (black) 565 value. Terrain is immune (uses `LandFadeData`, not `palette_table`), which is why only the cockpit showed it. **Fix (`BTREE.CPP:580`, `MA_LINUX`):** re-enable the engine's original per-object palette reset (disabled there as `//dead POLYGON.SelectPalette(0)`), forcing a real LUT reload (`POLYGON.selectedPalette=-1; POLYGON.SelectPalette(0)`) before `drw_cockpit`. Verified: diagnostic forcing the same select turned the flat-black cockpit textured, then the standalone fix reproduced it. Remaining (not the bug): scene-state/bank/time-of-day differs (same-view recapture caveat); ADI inset + enemy-disk are PORT ADDITIONS (default-on since S25). |
| 11 | 02-45 | External/chase view (F-86 over terrain) | `flight_external.png` (FLY + `BOB_KEYSEQ='12,0x40'` F6 + `MA_DUMP_BACK=320`) | **CLOSE** (S73) | Aircraft renders FULLY TEXTURED — silver/white F-86 skin, yellow ID bands, "FU-908" markings, drop tanks; terrain/river/horizon render; ADI inset now shows attitude content (no longer a black rectangle). The S72-era "aircraft skins near-silhouette dark" is NOT present in the S73 capture (aircraft/`MOBILE_OBJECT` was already fine; the palette-reset bug prominently hit only the cockpit). Remaining: scene-state differs from gold (same-view recapture caveat); ADI inset + enemy-disk are port additions. |
| 12 | 03-02 | Debrief (Claims table) | `flight_debrief.png` — FLY + `BOB_AUTOEXIT=60 MA_SHOT=220` under `SDL_VIDEODRIVER=dummy` (S75, GL-free) | **CLOSE** (S75) | **S75: the post-mission debrief is CAPTURED and matches gold** — identical layout, the **same pilot briefing photo**, mission header (Mission/Objective/Status, yellow labels+values), the Claims table (Player/UN/Red × target rows), and yellow small-caps BACK/AC STATS/GROUND STATS/REPLAY chrome (S69 fonts). Captured **headless** via the existing `BOB_AUTOEXIT=N` hook (→`ma_request_flight_exit`→`OverLay.quit3d`→`CloseWindow(IDOK)`→`OnFlyingClosed`→debrief) under `SDL_VIDEODRIVER=dummy` — **no code change, no display lock**. **The only differences are mission-type DATA, not render deviations:** native flew Hot Shot (air-to-air) → the default Claims view is aircraft (F51/F80/F84/F86/B26/B29/MiG 15/Yak 9) and the 2nd header field is "Objective: Pyongyang Main Airfield"; gold's ground-attack mission → ground-target Claims (Supply Point/Vehicle/Train, Marshalling Yard, Bridge, Airfields, Artillery, Troops, Tank) and "Name: Kimpo Airfield" — the AC-Stats-vs-Ground-Stats default follows the mission type (both views selectable via the buttons). **#12 → CLOSE; the I1 gold-shot inventory (all shots have native captures) is now COMPLETE.** _(S74's `campaign_overview.png` = the Ac-Stats sub-view, kept as a bonus capture.)_ **Legacy S74 note:** gold #12 is the **post-mission DEBRIEF** (full-screen: pilot-in-cockpit photo, "Mission: Landing/Takeoff practice / Name: Kimpo Airfield / Status: Operational", a **ground-target Claims** table — Supply Point/Vehicle/Train, Marshalling Yard, Bridge, Airfields, Artillery, Troops, Tank × Player/UN/Red — and BACK/AC STATS/GROUND STATS/REPLAY). Reached only via the mission-end path (`FULLPANE.CPP:2674` `MMC.indebrief=TRUE`+`MMC.NextMission()`), so a real mission→debrief run is required (no clean headless trigger). **The gold's "Ac Stats" SUB-VIEW = the campaign-map Overview panel** (new `MA_OOB_OVERVIEW` hook, `MainToolbar::OnClickedOverview`→`CAC_view`): captured natively and renders correctly — "OVERVIEW" title chrome + star roundel + ?/✓, Ac Stats/Ground Stats tabs, Kills + Losses tables (MiG 15/Yak 9/AAA × F86 1/F86 2/F80/F84/F51/B26/B29/All), yellow sans headers (S69), translucent briefing photo (S71). Minor residual: two small black rectangles on the right of the panel (likely an un-palette'd inset — cf. the cockpit-black class, low priority). **Main #12 capture scoped to a post-mission run.** |
| 13 | 03-14 | Campaign select (5 phases) | `campaign_select.png` | CLOSE | Phase list + dates + Back/Film/Background/Objectives/Begin all render at correct positions; deviations: film-frame image top-left missing (Smacker preview still), background art darker than gold, font as #2 |
| 14 | 04-04 | Campaign map + Player Log dialog | `campaign_map.png` (map alone, CAMP + `BOB_DUMP_FRAME=200`) + `map_playerlog.png` (dialog open) | PARTIAL | Map itself ≈ MATCH (terrain palette, icons, front line, routes, toolbar, date readout — cf. S45–S47). Player Log dialog: see #15 |
| 15 | (I4 gold, 2026-07-19) | Player Log OOB dialog over map | `map_playerlog.png` (Career) + `map_playerlog_tab1.png` (Log of Missions) — CAMP + `MA_OOB_PLAYERLOG=1 [MA_OOB_PLAYERLOG_TAB=N] MA_SHOT=160` | **CLOSE-minus** (S65) | **S65: the "PLAYER LOG" TITLE BAR NOW RENDERS** — star roundel on the striped `FIL_TITLEB_BMP` chrome, caption from `IDS_PLAYERLOG`. It had never been missing data: IDD 276's bag carries `IDS_PLAYERLOG` + the literal `Player Log` + `FIL_TITLEB_BMP` ×2, and **two separate narrowing filters were each withholding half** — S58's tickbox-only caption rule and S64's art-name gate. Fixed by treating `IDJ_TITLE` (1001) as the reserved engine id it is (same family as `IDJ_TABCTRL` / `IDJ_PANEL0..9`), whose caption and art are design-time by definition. Earlier (S61) the tab bar, centring and tab switching landed; (S63) authored colours; (S67) the title trimmed to the dialog; (S68) the `?`/`✓` title buttons; (S69) the sans "Name" label. **S70: the Career CONTENT TABLE now renders** — the per-type Sorties/Combats/Kills/Losses table (F86 1/F86 2/F80/F84/F51/All, populated from `MMC.debrief.playertotals`), the last open half of I4. Root cause was engine-level: the OOB dialog draw path (`ma_ole_draw_toolbar`) had **no `CT_LISTBOX` case**, so the table's RListBox (`IDC_RLISTBOXCTRL1` in `IDD_CAREER`) was populated but never drawn; adding the case also lit up the **Log of Missions** tab's Date/IP/Kills log. **#15 → CLOSE.** **S71: the table is now TRANSLUCENT** — a context flag (`ma_oob_lb_draw`, set only on the OOB draw path) skips the listbox black fill so the pilot photo shows through = gold, without erasing the front-end menu (the S70 blocker). **Residual (named, minor):** a doubled "F86 1" in the header corner (`CAREER.CPP:173-177` adds `IDS_L_SQ_BF_F86A` twice — a source-vs-BDG data delta, PO-waivable). |

> **⚠ `prefs_controls.png` is NOT a stable reference (S62).** That screen enumerates
> LIVE hardware, so the committed capture embeds the machine's state at capture time: it
> was taken with a Logitech Extreme 3D attached ("4 axes, 1 hat(s), 12 buttons", axis
> names populated). On a box with no joystick (`/dev/input/js*` absent) the same build
> renders "NOT CONNECTED / 0 axes" and the byte-compare fails for a purely environmental
> reason. Check for a joystick before treating a `prefs_controls` diff as a regression.
> This is the S59 device-presence lesson one level out — there it was the port's own
> enumeration that varied by video backend; here it is the ORACLE that varies by hardware.

## Cross-cutting deviations (fix once, moves many rows)

1. **Front-end font/typeface** — ✅ **SOLVED (colour S63, FACE S66).**
   - *Colour (S63):* the persisted design-time property reader gives the front end its
     authored colours — VALUES yellow exactly as gold, tab bar yellow, labels in gold's
     blue family, title menu yellow with its black backing box gone.
   - *Face (S66):* the game ships its own typeface —
     `drive_c/windows/Fonts/Intel.ttf`, *"Copyright (c) Rowan Software, 1998"* — and the
     port was never loading it. Two reasons: `ma_gdi_font_create` ignores the requested
     face outright, and the single global TTF load was **rejecting Intel.ttf** because
     `stbtt_InitFont` accepts only platform-3 cmap encodings 1/10 while Intel.ttf ships a
     **(3,0) SYMBOL** cmap (characters addressed at `0xF000+c`). Fixed by accepting symbol
     cmaps and offsetting lookups. Verified against gold: the title menu is
     `PREFERENCES / SINGLE PLAYER / …` in yellow **small caps**, the same face gold uses;
     Preferences shows a yellow small-caps tab bar, blue small-caps labels and yellow
     values, as gold does.
   - *Per-face FACE (S69):* S66 loaded Intel.ttf globally, but the port drew **every** face
     in it — matching gold "by luck" only where the front-end already used Intel. Two fixes:
     (a) `ma_gdi_font_create` now resolves the requested face through a **cached face
     registry** (ART=Intel.ttf, SANS=LiberationSans≈Arial, SERIF=Liberation/DejaVu Serif;
     unknown→ART); (b) the port was running as a **Japanese system** — compat
     `EnumFontFamiliesA` always reported a face present, so `MIG.CPP`'s localization probe
     took the Japanese branch and asked for MS Mincho everywhere (an unshipped CJK name that
     collapsed to the art face), so it **never requested Arial**. Fixed the stub to report a
     face present only for a pure-ASCII name (no CJK ships), so the English branch runs and
     the runtime faces are now `Intel` (ART) + `Arial` (SANS), exactly as on gold's box.
     **Gold-verified:** Preferences #2/#8 and Quick Mission #9 now render **blue sans labels +
     yellow sans values** = gold; campaign #13 phase list in yellow sans = gold; Intel bars
     unchanged (byte-identical). Cross-cutting #1 is now **fully closed** (colour + face).
   - **Residual on these rows is now only the BDG tab (a resource delta — gold is the
     BDG-patched build).** (Combo chrome, cross-cutting #2, resolved S69 — see below.)
   - ⚠ Retires the "GDI DejaVu fallback" phrasing used across this doc since S56: accurate
     as a symptom, but it read as the design and nobody asked why the fallback was taken.
   - ⚠ The S64 resolution caveat still stands: gold is ~1280×1024 and native front-end
     captures are 800×600 because the game selects its panel ART SET by resolution, so **no
     verdict may rest on relative size, spacing or density.**

2. **Combo/control chrome** — ✅ **SOLVED (S69).** Native combos filled their value box
   **opaque black**; gold's are **transparent** — the panel/photo shows straight through a
   thin bordered outline (verified by cropping gold #2). Root cause: `CRComboCtrl::OnDraw`
   fills black (`RCOMBOC.CPP:355`) when `WM_GETARTWORK` returns 0, and the port deliberately
   returns 0 (the panel's OnPaint already composited its background; hosted controls draw
   transparently over it). So the combo was the one control still *filling* its box. Fixed by
   skipping the black `FillRect` on the `MA_LINUX` path — the border pens + transparent
   `FIL_COMBO_BUTTON` still draw the chrome; native combos are now translucent = gold.
   Residual (named, minor): gold's border is a fainter rounded blue vs native's rectangular
   light edge (a pen-colour/style delta, not the opaque-fill deviation).
3. **Missing static labels on some panels** (#7, #8) — ~~likely the (dlgId, ctrlId)-scoped
   lookup keystone~~ **root-caused S57 (BoB note 14 / §8f lesson #3): NOT a scoping bug**
   (MA's per-instance template keying already scopes correctly) — the labels sit on
   template statics the dialog classes never DDX-bind, so DDX-driven creation never made
   them. Fixed: template-driven static hosting after OnInitDialog + IDS→string-table
   caption resolution + membership draw/click filter (`MA_NO_PE_RSRC=1` reverts the layer,
   A/B-verified byte-identical to S56 in the headless harness). **✔ CLOSED S58:
   capture-verified in-game (#7, #8 flipped to CLOSE).**
3b. **Uninitialised `DoPropExchange`-only members (S58 root cause, fixed)** — compat
   `PX_*` are no-ops that don't write defaults, so every persisted member the R* ctors /
   `OnResetState` don't cover held heap garbage, and the garbage was
   **environment-dependent** (SDL-dummy vs GL-window heap layout): the prefs tab bar drew
   a black band + clipped rows *headless only* (`m_bLockTopRow`/`m_bBlackboard`
   garbage-TRUE), the title menu drew doubled captions + black row fills. Fixed by
   ctor-initialising all persisted `CRListBoxCtrl` members to PX defaults
   (`RLISTBXC.CPP`); dummy-run canvas now **byte-identical** to GL-run canvas (the new
   MA_SHOT acceptance bar). Watch for the same class on the other R* controls if a
   screen renders differently headless vs windowed. Cross-port: MA note 16 / §8f
   addendum.
4. ~~**3D overlay black boxes** (#10, #11) — padlock-ADI inset renders black; cockpit POV art
   black.~~ **CLOSED S73.** NOT a palette/texture-upload bug: the software raster path's active
   `palette_table` LUT was left stale/empty at cockpit-draw time and the cockpit's cached
   `SelectPalette(0)` no-op'd, so cockpit imagemap/flat texels indexed an empty LUT → near-0
   black; terrain was immune (uses `LandFadeData`). Fixed by re-enabling the engine's original
   per-object palette reset at `BTREE.CPP:580` (`//dead POLYGON.SelectPalette(0)`), forced.
   Cockpit + ADI inset now textured = gold; external F-86 confirmed already textured.
5. **Oracle is BDG 0.85F** — decide with PO whether parity targets BDG or source resources
   (BoB has the same open PO question for 0.99).

## S56 A/B evidence — LBM bounds fix (IMAGEMAP.CPP), kept

Controlled A/B (2 runs each way, FLY view, `MA_TRACE_LBM=1`): traces fully deterministic
per mode (both bounded runs byte-identical: `e5e0aae9…`; both unbounded: `43774cd2…`);
**exactly one decode differs** — a 2×2 imagemap whose ByteRun1 row data ends at the buffer
edge: unbounded consumes **24655/24636** bytes (reads 19 bytes past the heap fileblock —
the exact ASan heap-buffer-overflow the fix targets), bounded stops at the edge. Default
build behaviour is bit-identical to pre-instrumentation (env-gated). `MA_LBM_NOBOUND=1`
re-runs the losing side; `MA_TRACE_LBM=1` prints per-file decode traces.

## S58 re-capture note (2026-07-27)

All 2D captures above (#1–#9, #13) were re-taken in S58 via the new GL-free `MA_SHOT`
path (SDL-dummy, byte-identical to GL runs — verified by `cmp` on #2). #1 is the first
title capture; #7/#8 flipped PARTIAL→CLOSE on the S57 fixes; #2–#6/#13 verdicts
unchanged (CLOSE). Gates at capture time: `port/asan_all.sh` + `port/stress_launch.sh`
(see sprint-58 board for results). #10–#12/#14–#15 (3D scenes, debrief, Player Log)
unchanged this sprint.

## S59 re-capture note (2026-07-27)

Full 2D set re-captured post-fix; refs refreshed for #3/#4/#5/#7/#9 (the rest byte-
identical to S58). Changes are all fixes: #9 stray-cluster/"I.D."/word-wrap (see row),
and the uninit-PX ctor audit widened to RStatic/RButton/RCombo/REdtBt (S58 3b class) —
the large-font value rows on #3/#4/#5/#7 (garbage `m_FontNum`) now match sibling rows,
and the Controls tickbox glyph sits inside its box art. Template visibility is now
routed: per-control `WS_VISIBLE` = initial show state; controls parked outside the
dialog's own rect are Windows-clipped and never drawn/clicked (`ma_dlg_template_visible`
/ `ma_dlg_never_visible`, style dword parsed from RT_DIALOG). Acceptance: dummy==GL
`cmp` **byte-identical** re-verified on #7 and #9 — after fixing a new catch by the bar
itself: the DI mouse device was enumerated only when the SDL window existed, so #7's
"3d Pointer" row read "Keyboard" headless vs "active mouse : X-Axis & Y-Axis" on GL
(gold agrees with GL); device presence must not depend on the video backend.

## S69 re-capture note (2026-08-02) — font FACE + combo chrome REBASE

Deliberate **rebase** (not byte-identical), as S63/S66: the per-face font fix and the combo
transparency change every label/combo screen by design. Re-captured and gold-verified, then
rebased 10 native refs (`title` unchanged/byte-identical; `prefs_3d/3d2/flight/game/views/
controls/others`, `quickmission`, `campaign_select`, `map_playerlog`).

- **Cross-cutting #1 font FACE — CLOSED (see the deviations section).** Two silent-fallback
  bugs: `ma_gdi_font_create` ignored the face arg (now a cached ART/SANS/SERIF registry), and
  the port was running as a **Japanese system** (compat `EnumFontFamiliesA` always reported a
  face present → `MIG.CPP` localization probe took the CJK branch and never requested Arial;
  fixed to report only ASCII faces present). Runtime faces are now `Intel`(ART)+`Arial`(SANS)
  as on gold's box. **#2/#8/#9/#13 verified against gold: blue sans labels + yellow sans
  values + sans briefing/phase-list = gold; Intel bars byte-identical.**
- **Cross-cutting #2 combo chrome — CLOSED.** Native combos were opaque black; gold's are
  transparent (panel shows through a thin border). Skipped the black `FillRect` on the
  `MA_LINUX` path (`RCOMBOC.CPP`). Combos now translucent = gold.
- **Verdicts:** #2 Prefs-3d and #13 Campaign-select move to **CLOSE** (font+combo now match
  gold; residual = BDG tab resource delta only). #7/#8 stay **CLOSE** (Controls tick-glyph and
  the `prefs_controls` live-joystick caveat unchanged). #9 stays **CLOSE-minus** (Scenario/UN
  RRadio row still unhosted). #15 map_playerlog rebased for the sans "Name" label; its Career
  content table remains the only open half of I4.
- **Not yet rebased (font touches them, deferred to S70):** `map_playerlog_tab1` (Log of
  Missions tab) and `campaign_map` (map date readout) — both change to sans and must be
  re-captured before the S70 byte-identical sweep, or they will false-flag.
- `prefs_controls` remains the environment-dependent oracle (captured with a Logitech
  attached this run; `/dev/input/js0` present).

## S70 note (2026-08-02) — Player Log Career table renders; #15 → CLOSE

The last open half of I4 is closed: the Career tab's Sorties/Combats/Kills/Losses table now
renders (see #15). Root cause was a **missing `CT_LISTBOX` case in the OOB dialog draw path**
(`ma_ole_draw_toolbar`) — the table's RListBox was populated but never drawn; the front-end
draws listboxes via a different path (`ma_ole_draw_all`), which masked the gap. The same fix
also renders the Log of Missions tab's log listbox. Two named residuals remain (opaque listbox
box vs gold's translucent — the fill is load-bearing for the front-end menu so it can't be
skipped globally; and a doubled "F86 1" header cell = source-vs-BDG data delta).

`campaign_map` confirmed **byte-identical** across S69→S70 (the map date readout uses
`g_AllFonts[1]="Intel"` = the art face, untouched by the S69 font change). `map_playerlog` +
`map_playerlog_tab1` rebased for the now-rendering tables. Byte-identical sweep RESUMED and
passes on title/prefs_3d/prefs_others/quickmission/campaign_map.
