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

**Native captures:** `port/ref/native/*.png` (committed, S56). Repro: run `./wmig` from the
install dir with the env line given per row; 2D screens dump via `BOB_DUMP_FRAME=N
BOB_EXIT_AFTER_DUMP=1` → `/tmp/bobframe.ppm`; 3D via `MA_DUMP_BACK=N` → `/tmp/maback.ppm`.
Standard sequences: FLY=`BOB_CLICKSEQ="40,588,231;95,588,217"` (title→Single Player→Hot
Shot); CAMP=`BOB_CLICKSEQ="30,588,263;65,40,108;100,68,565"` (title→Load Game→Auto
Save→Load) + `MA_DISABLE_3D=1 MA_IGNORE_SAVE_DATE=1`.

Verdict scale (BoB S123): **MATCH** / **CLOSE** (minor named deviations) / **PARTIAL**
(renders, major named deviations) / **GAP** (missing/wrong) / **not yet captured**.

## Inventory & verdicts (S56 pass)

| # | Gold (17-…) | Screen | Native capture / repro | Verdict | Deviations (named) |
|---|---|---|---|---|---|
| 1 | 00-06 | Title screen (menu incl. "BDG version 0.85F" row) | *not yet captured* (title renders natively since S4 — capture with `BOB_DUMP_FRAME=15`) | — | Known: no BDG menu row (resource delta); menu font is engine art vs native GDI font |
| 2 | 00-45 | Preferences → 3d | `prefs_3d.png` (`BOB_DUMP_FRAME=…`, click Preferences) | CLOSE | Tab bar plain GDI text vs gold's art typeface (+ no BDG tab, resource delta); label font white serif vs gold blue sans; combo boxes black-filled vs translucent; values differ (settings state, not render) |
| 3 | 00-52 | Preferences → 3d II | `prefs_3d2.png` | CLOSE | As #2; label typo "Texture Qualtiy" is in the source resources (gold BDG fixed it: "Texture Quality") |
| 4 | 00-58 | Preferences → Flight | `prefs_flight.png` | CLOSE | As #2; layout/rows/rects correct |
| 5 | 01-04 | Preferences → Game | `prefs_game.png` | CLOSE | As #2 |
| 6 | 01-13 | Preferences → Views | `prefs_views.png` | CLOSE | As #2; "Camera Colour" vs gold "Camera Color" (resource delta) |
| 7 | 01-23 | Preferences → Controls | `prefs_controls.png` | PARTIAL → **fix landed S57, re-capture GL-blocked** | As #2, plus: `Calibrate` button missing; Enable/Use-for-FF checkbox glyphs missing; "Dead Zone:"/"Airframe"/"Input Devices:" labels missing; joystick axis names empty ("active joystick : &" vs "Axis 0 & Axis 1") — DI axis-name enumeration not filled. **S57 root causes fixed headless-verified against miglang.dll (IDD 958):** missing labels = template statics 2027/2078/2080/2083 never DDX-bound → now hosted (`ma_host_template_statics`); "Input Device:"→"Input Devices:" = stale DLGINIT literal → IDS_JOYDEV now resolved via BDG string table; tickboxes 2358/2360 = FIL_ICON_TICKBOX1 art (0x6a81) + glyph "3" now applied; Calibrate = un-hosted REdtBt OCX (0x461a1fe3) → new `ma_oleredtbt.cpp` host + runtime SetCaption path; axis names = DI `tszName` never filled → "Axis %d"/"X-Axis" in `DIDEV_EnumObjects` |
| 8 | 01-32 | Preferences → Others | `prefs_others.png` | PARTIAL → **fix landed S57, re-capture GL-blocked** | As #2, plus ~6 row labels missing (Control SFX / Ambient SFX / G Effects / Injury Effects / White Outs / Auto Vectoring) — combos render, labels don't. **S57 headless-verified (IDD 271):** exactly those 6 = template statics 2023–2028, none DDX-bound by CSSound (binds only 2020/2021/2022/2151, which did render) → hosted via `ma_host_template_statics`; labels parse+IDS-resolve correctly in the production TUs (harness) |
| 9 | 01-45 | Quick Mission setup | `quickmission.png` | PARTIAL | Rows/combos/mission text/Back-Variants-Fly all render; deviations: mission text not word-wrapped to the panel (runs off right edge); stray combo artifact ~(590,165) — **S57 membership filter (`ma_dlg_in_template`, draw+click paths) should kill it if it is a source-only control; re-capture GL-blocked**; Scenario/UN radio row missing; "I.D." label vs gold none (resource delta?) |
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
   A/B-verified byte-identical to S56 in the headless harness). GL-blocked: in-game
   re-capture for the verdict flip.
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
