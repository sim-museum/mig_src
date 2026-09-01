# `port/ref/native/` — what these captures are, and are not

_Added S161 (2026-08-21), after S159 clipped every campaign dialog's backdrop art (PO-49) and it
became clear that nothing in this directory would have noticed._

**Five of these files are oracles. The rest are undated documentation.**

`port/parity_2d.sh` is the only thing in the tree that compares against this directory. It uses
exactly five screens and requires them **byte-identical**; they are marked ORACLE below, they carry
their recipes inside the gate (not in prose — the doc's tab pixels went stale once and silently
captured the wrong tab, S-lesson), and `campaign_map` is captured around a pinned save fixture
(`port/ref/save/campaign_pristine.sav`) so mutable campaign state cannot drift the reference.

Everything else is a **snapshot taken to illustrate one sprint's result**. Snapshots have no gate,
no recorded recipe, and no expiry. They were true on their date and several are not true now:

- **The `1021x644` and `1021x777` captures predate the canvas-overhang fix.** That size is itself
  the bug — a blit that overhung the screen used to GROW the canvas instead of being clipped
  (`port/map_drag.sh` documents it), so any capture at 1021 wide is showing an inflated screen.
- **Every campaign-dialog snapshot taken before 2026-08-21 shows the UNCLIPPED backdrop art**
  that S159 removed — a skirt of up to 286 px hanging below the dialog over the map. Nine of nine
  campaign dialogs were affected.

They are kept because a picture of what a sprint delivered is worth having, and deliberately **not**
refreshed wholesale: their recipes were never recorded, so a "refresh" would be a new capture under a
guessed recipe wearing an old file's name, which is worse than a snapshot with a date on it.

**If you need a reference to be true, put it behind a gate.** An oracle that nothing runs is a
picture. (BoB's §8-BoB181 makes the sharper version of this point: a gate whose coordinates were
fitted to a defect can only pass while the defect survives.)

| file | role | size | last written | sprint |
|---|---|---|---|---|
| `adi_hardware.png` | snapshot | 700x300 | 2026-08-16 | S150 (PO-31) |
| `b6_fullres_map.png` | snapshot | 960x540 | 2026-08-15 | S127 (B6 SHIPPED) |
| `camp_flight.png` | snapshot | 640x360 | 2026-08-15 | S123 |
| `campaign_loop_endcamp.png` | snapshot | 1021x644 | 2026-08-08 | Sprint 80 'Fly the loop' |
| `campaign_map.png` | **ORACLE** | 800x600 | 2026-08-16 | S145 (PO-19) |
| `campaign_map_chrome.png` | snapshot | 800x600 | 2026-08-15 | S109 (PO-14 -> PO-11) |
| `campaign_mission2_brief.png` | snapshot | 1021x644 | 2026-08-03 | Sprint 76 'Scope the campaign' |
| `campaign_overview.png` | snapshot | 1021x644 | 2026-08-02 | Sprint 74 'Face the debrief' |
| `campaign_postdebrief.png` | snapshot | 1021x644 | 2026-08-03 | Sprint 79 'Land the loop fix' |
| `campaign_select.png` | snapshot | 800x600 | 2026-08-02 | Sprint 69 'Face the type, dress th |
| `career_typed.png` | snapshot | 800x600 | 2026-08-15 | S121 (PO-16) |
| `dialog_scrollbar.png` | snapshot | 500x350 | 2026-08-16 | S140 (PO-34) |
| `dis_briefing.png` | snapshot | 700x220 | 2026-08-16 | S142 (PO-36) |
| `dis_dialog.png` | snapshot | 470x400 | 2026-08-16 | S136 (PO-28) |
| `flight_cockpit.png` | snapshot | 640x480 | 2026-07-26 | Sprint 56 |
| `flight_debrief.png` | snapshot | 800x600 | 2026-08-02 | Sprint 75 'Capture the debrief' |
| `flight_external.png` | snapshot | 640x480 | 2026-07-26 | Sprint 56 |
| `flight_profile.png` | snapshot | 470x200 | 2026-08-16 | S149 (PO-38) |
| `help_panel.png` | snapshot | 800x600 | 2026-08-15 | S114 (PO-10) |
| `help_playerlog.png` | snapshot | 1840x450 | 2026-08-15 | S134 (PO-26) |
| `hud_1920.png` | snapshot | 960x540 | 2026-08-15 | S122 (PO-20) |
| `hw_cockpit.png` | snapshot | 640x480 | 2026-08-15 | S115 (PO-12 phase 3) |
| `hw_cockpit_full.png` | snapshot | 640x480 | 2026-08-15 | S117 (PO-12 phase 3c) |
| `hw_cockpit_textured.png` | snapshot | 640x480 | 2026-08-15 | S116 (PO-12 phase 3b) |
| `hw_selected_in_prefs.png` | snapshot | 640x480 | 2026-08-15 | S118 (PO-12 phase 4) |
| `hw_terrain.png` | snapshot | 640x480 | 2026-08-15 | S119+S120 |
| `map_chrome.png` | snapshot | 1920x110 | 2026-08-16 | S144 (PO-21) |
| `map_filters.png` | snapshot | 800x550 | 2026-08-16 | S137 (PO-30) |
| `map_fullscreen.png` | snapshot | 960x540 | 2026-08-16 | S145 (PO-19) |
| `map_playerlog.png` | snapshot | 1021x644 | 2026-08-02 | Sprint 71 'Polish the chrome' |
| `map_playerlog_tab1.png` | snapshot | 1021x644 | 2026-08-02 | Sprint 71 'Polish the chrome' |
| `map_ruler.png` | snapshot | 700x840 | 2026-08-15 | S135 (PO-22) |
| `map_vs_gold.png` | snapshot | 760x870 | 2026-08-16 | S152 |
| `map_waypoints.png` | snapshot | 640x480 | 2026-08-15 | S107 (PO-13) |
| `map_window.png` | snapshot | 640x480 | 2026-08-15 | S105 (PO-6) |
| `mission_folder.png` | snapshot | 540x220 | 2026-08-16 | S149 (PO-38) |
| `mission_results.png` | snapshot | 800x600 | 2026-08-15 | S106 (PO-9) |
| `oob_directives.png` | snapshot | 1021x644 | 2026-08-09 | Sprint 85 'Say which one' |
| `oob_intelligence.png` | snapshot | 1021x777 | 2026-08-08 | Sprint 84 'Open it once' |
| `prefs_3d.png` | **ORACLE** | 800x600 | 2026-08-16 | S143 (PO-35) |
| `prefs_3d2.png` | snapshot | 800x600 | 2026-08-02 | Sprint 69 'Face the type, dress th |
| `prefs_controls.png` | snapshot | 800x600 | 2026-08-02 | Sprint 69 'Face the type, dress th |
| `prefs_flight.png` | snapshot | 800x600 | 2026-08-02 | Sprint 69 'Face the type, dress th |
| `prefs_game.png` | snapshot | 800x600 | 2026-08-02 | Sprint 69 'Face the type, dress th |
| `prefs_others.png` | **ORACLE** | 800x600 | 2026-08-16 | S143 (PO-35) |
| `prefs_tab_ours.png` | snapshot | 800x40 | 2026-08-16 | S143 (PO-35) |
| `prefs_views.png` | snapshot | 800x600 | 2026-08-02 | Sprint 69 'Face the type, dress th |
| `quickmission.png` | **ORACLE** | 800x600 | **2026-09-01 (re-seeded, S414)** | S143 (PO-35) → S414 (PO-89) |
| `quit_confirm.png` | snapshot | 420x260 | 2026-08-16 | S156 (PO-47) |
| `radio_menu.png` | snapshot | 640x480 | 2026-08-15 | S104 (PO-7) |
| `sw_cockpit_ref.png` | snapshot | 640x480 | 2026-08-15 | S116 (PO-12 phase 3b) |
| `sw_terrain_ref.png` | snapshot | 640x480 | 2026-08-15 | S119+S120 |
| `title.png` | **ORACLE** | 800x600 | 2026-08-16 | S143 (PO-35) |
| `title_after_quit.png` | snapshot | 960x540 | 2026-08-16 | S146 (PO-33) |
| `title_menu_ours.png` | snapshot | 660x420 | 2026-08-16 | S143 (PO-35) |

## An oracle can go stale in the "we got better" direction (S414, PO-89)

`quickmission.png` was captured **2026-08-16**. The `CT_RADIO` branch in `ma_ole_draw_all` — the
code that lets a radio control paint at all — landed **2026-08-29** (`8cf158a`, PO-83). So for
thirteen days the oracle recorded a screen with a control **missing**, and from S329 onwards the
gate reported the port's *improvement* as a 1497 px regression.

It was re-seeded only after two things were established, and neither is optional:

1. **An independent authority agreed with the new pixels, not the old ones.**
   `port/reference/wine-gold/09-quickmission.png` — the real game under Wine, already in this
   tree — shows the radio row (`✓Scenario ○UN ○…`) exactly where the port now draws it.
   S354 parked this row as *"needs the PO to confirm gold"*; **the confirmation had been sitting
   in the repository the whole time.** Nobody had to be asked.
2. **Every pixel of the change was accounted for.** The re-seed script asserts that the differing
   pixels lie *entirely* inside the radio band (x 33–412, y 166–187) and refuses to write if even
   one pixel outside it differs. It measured 1497 in, **0 out**.

Re-seeding without both is how a gate is quietly turned into a picture of whatever the build
happens to do — the failure this directory's own header warns about.
