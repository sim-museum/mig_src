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
Standard sequences: FLY=`BOB_CLICKSEQ="40,588,231;95,588,217"` (title→Single Player→Hot
Shot); CAMP=`BOB_CLICKSEQ="30,588,263;65,40,108;100,68,565"` (title→Load Game→Auto
Save→Load) + `MA_DISABLE_3D=1 MA_IGNORE_SAVE_DATE=1`.

Verdict scale (BoB S123): **MATCH** / **CLOSE** (minor named deviations) / **PARTIAL**
(renders, major named deviations) / **GAP** (missing/wrong) / **not yet captured**.

## Inventory & verdicts (S56 pass)

| # | Gold (17-…) | Screen | Native capture / repro | Verdict | Deviations (named) |
|---|---|---|---|---|---|
| 1 | 00-06 | Title screen (menu incl. "BDG version 0.85F" row) | `title.png` (S58 first capture: `MA_SHOT=30`) | CLOSE | No BDG menu row (resource delta); menu font GDI vs gold art typeface (cross-cutting #1). **S58:** the doubled/black-band menu rows seen in early S58 shots were the uninit-PX garbage (see below), NOT a font-path delta — menu now renders single centred captions |
| 2 | 00-45 | Preferences → 3d | `prefs_3d.png` (`BOB_DUMP_FRAME=…`, click Preferences) | CLOSE | Tab bar plain GDI text vs gold's art typeface (+ no BDG tab, resource delta); label font white serif vs gold blue sans; combo boxes black-filled vs translucent; values differ (settings state, not render) |
| 3 | 00-52 | Preferences → 3d II | `prefs_3d2.png` | CLOSE | As #2; label typo "Texture Qualtiy" is in the source resources (gold BDG fixed it: "Texture Quality") |
| 4 | 00-58 | Preferences → Flight | `prefs_flight.png` | CLOSE | As #2; layout/rows/rects correct |
| 5 | 01-04 | Preferences → Game | `prefs_game.png` | CLOSE | As #2 |
| 6 | 01-13 | Preferences → Views | `prefs_views.png` | CLOSE | As #2; "Camera Colour" vs gold "Camera Color" (resource delta) |
| 7 | 01-23 | Preferences → Controls | `prefs_controls.png` (S58 re-capture) | **CLOSE** (S58 verdict flip — every S57 fix capture-verified) | ✔ in-capture: "Input Devices:" (gold IDS wording), Calibrate (REdtBt), "Stick"/"Throttle"/"Rudder"/"Dead Zone:"/"Airframe" labels, axis names "Axis 0 & Axis 1"/"Axis 3"/"Axis 2" (live Logitech Extreme 3D enumerated), tickbox art + glyph on Enable/Use-for-FF. Residual (named): tick glyph renders literal "3" (Marlett-family glyph through the GDI fallback font — cross-cutting #1); combo chrome (#2); "View pan" value row draws in a larger font than sibling rows |
| 8 | 01-32 | Preferences → Others | `prefs_others.png` (S58 re-capture) | **CLOSE** (S58 verdict flip) | ✔ in-capture: all 6 previously-missing labels render (Control SFX Volume / Ambient SFX Volume / G Effects / Injury Effects / White Outs / Auto Vectoring) via `ma_host_template_statics` (S57 headless verdict confirmed on IDD 271 statics 2023–2028). Residual: font/chrome cross-cutting #1/#2 only |
| 9 | 01-45 | Quick Mission setup | `quickmission.png` (S58 re-capture) | PARTIAL | Rows/combos/mission text/Back-Variants-Fly all render; deviations: mission text not word-wrapped to the panel (runs off right edge); stray combo artifact ~(590,165) — **S58 finding: the S57 filter hypothesis is DISPROVEN — zero `[filter-skip]` on this screen, the control IS in the installed template**, so Windows must hide/overlay it at runtime by a mechanism the host doesn't route (`m_maVisible=1`; not the `RFullPanelDial::incomms` branch) — still open; Scenario/UN radio row missing; "I.D." label vs gold none (resource delta?) |
| 10 | 02-21 | In-flight cockpit (F-86) | `flight_cockpit.png` (FLY + `MA_DUMP_BACK=100`) | PARTIAL | Scene/terrain/sky/gunsight-glass render; deviations: cockpit frame + instrument panel render flat black (POV cockpit art untextured); padlock-ADI inset black rectangle (top-right); scene state differs (above cloud deck vs gold over terrain) — needs same-view recapture |
| 11 | 02-45 | External/chase view (F-86 over terrain) | `flight_external.png` | PARTIAL | Aircraft + contrails + cloud deck + horizon render; deviations: aircraft skins near-silhouette dark (cf. BoB note-8 depth-sort/washout fix — check), ADI inset black rectangle, bottom strip dithered black; scene state differs from gold (needs same-view recapture over terrain) |
| 12 | 03-02 | Debrief (Claims table) | *not yet captured* (renders natively since S21–35 — capture after a flight exit) | — | Known-good from play-tests; Claims "Player" header fixed S35 |
| 13 | 03-14 | Campaign select (5 phases) | `campaign_select.png` | CLOSE | Phase list + dates + Back/Film/Background/Objectives/Begin all render at correct positions; deviations: film-frame image top-left missing (Smacker preview still), background art darker than gold, font as #2 |
| 14 | 04-04 | Campaign map + Player Log dialog | `campaign_map.png` (map alone, CAMP + `BOB_DUMP_FRAME=200`) + `map_playerlog.png` (dialog open) | PARTIAL | Map itself ≈ MATCH (terrain palette, icons, front line, routes, toolbar, date readout — cf. S45–S47). Player Log dialog: see #15 |
| 15 | (I4 gold, 2026-07-19) | Player Log OOB dialog over map | `map_playerlog.png` — CAMP + `MA_OOB_PLAYERLOG=1 BOB_DUMP_FRAME=280` (S56 hook: `MIG.CPP` map idle → `CMainToolbar::OpenPlayerlog()` after 40 map idles) | PARTIAL | Career-tab pilot-photo background art renders over the map (S53/S54 OOB path, headless-reproducible). Deviations: dialog draws at top-left, not centred (self-position/offset not honoured for this dialog); no dialog frame/title bar ("PLAYER LOG" + ?/✓ buttons); Career / Log of Missions / Last Mission tab bar missing (CRTabs not hosted); Name edit + Sorties/Combats/Kills/Losses table not drawn (content dialogs of the tab not rendering their controls). I4 remains open |

## Cross-cutting deviations (fix once, moves many rows)

1. **Front-end font/typeface** — native draws labels/menus with the GDI DejaVu fallback
   (white bold serif); gold uses the game's art typefaces (gold-embossed menu font, blue
   sans labels, yellow values). Affects #1–#9, #13. Biggest single visual gap.
2. **Combo/control chrome** — native combos are black-filled with white border; gold's are
   translucent panels. One draw-path fix in `ma_olecombo`/`ma_gdi`.
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
4. **3D overlay black boxes** (#10, #11) — padlock-ADI inset renders black; cockpit POV art
   black. Separate render-path bugs, likely palette/texture upload for overlay imagemaps.
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
