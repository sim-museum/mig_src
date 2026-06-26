# Sprint 21 — M4: in-map navigation (campaign operational map is now interactive)

**Goal:** the campaign operational map RENDERS (Sprint a1b5da7) but had NO input — the player
reached the Korea map and was stuck (only Ctrl+Esc/window-close could leave). Wire real
navigation so the map is usable.

**PO:** standing pre-approval. Run autonomously.

## Outcome: DONE. Pan / zoom / exit / fly all work natively, validated, crash-free.

The port has no WM_*/scrollbar/toolbar message flow reaching `CMIGView`, and the 5 `CMainFrame`
toolbars (Back/Fly/Zoom bitmap-button dialogs) aren't rendered. Rather than stand up the full
toolbar UI (large, high-regression), I wired the map's existing view/toolbar **handlers** directly
to SDL input in the idle-loop map branch — the highest-value, lowest-risk increment.

### What was added
- **`bob_video.cpp` — front-end nav input capture** (active only while the 3D flight doesn't own
  the keyboard, `!g_diKbAcquired`, so in-flight DI controls are untouched):
  - held-direction bitmask (arrows / WASD) → `ma_map_nav_held()`
  - one-shot action ring (`+`/`-`/PageUp/Dn = zoom, Esc = exit, F/Enter = fly) → `ma_map_nav_take()`
  - `SDL_MOUSEWHEEL` accumulator → `ma_mouse_wheel()`
- **`MIG.CPP` — idle-loop map branch drives the view each frame:**
  - **pan:** held arrows/WASD (`m_scrollpoint += STEP`) **and** left-button drag (`ma_mouse_pos`
    delta), clamped to `[0, m_size − canvas]` (the same bounds `UpdateScrollbars` uses).
  - **zoom:** wheel + `+`/`-` keys via `ma_map_apply_zoom()` — a local replica of
    `CMIGView::Zoom()`'s math (MIGVIEW.CPP:888-925) **minus** its `m_pScaleBar->RedrawWindow()` /
    scrollbar / `m_mapdlg` side-effects (`m_pScaleBar` can be NULL in the port → the real `Zoom`
    would crash; `CMainFrame::OnCreate` toolbar setup isn't run).
  - **exit:** Esc → `CMIGView::OnChangeToTitle()` → `LaunchFullPane(&introsmack)` (the proven
    Phase-4 title path). Fixes "stuck on the map".
  - **fly:** F/Enter → `CMainToolbar::OnClickedFrag2()` → singlefrag briefing → (its Fly →
    StartFlying, the existing M1 flight path). Reuses the gated MA_CAMP_FLY chain, now key-driven.

### Validation (all from the live build, campaign → Begin → map)
| Action | Test | Result |
|---|---|---|
| per-idle nav code (pan/drag/clamp every frame, no input) | boot to map, idle 210 frames | exit 0, map renders 1021×644, **no crash** |
| **Esc → exit** | `BOB_NAVSEQ=60,3` | `[map] Esc -> OnChangeToTitle`; branch flips map→panel (title, currentpage=1); no crash |
| **zoom** | `BOB_NAVSEQ=30,1;45,1;60,1` (3× zoom-in) | rendered map DIFFERS from baseline (93% of sampled px); map re-scaled; no crash |
| **pan** | `BOB_NAVPAN=2` (right) vs `=8` (down) | both differ from baseline and each other → map moves; no crash |
| **fly** | `BOB_NAVSEQ=60,4` | `[map] Fly -> OnClickedFrag2`; branch→panel (singlefrag briefing); no crash |
| **3D-launch regression** | `port/stress_launch.sh 5` | **5/5** reached & sustained 100 3D frames |

### Controls (campaign map)
`Arrows`/`WASD` or `left-drag` = pan · `mouse-wheel` / `+` / `-` = zoom · `Esc` = exit to title ·
`F` / `Enter` = fly the current mission.

### Test hooks added (gated, default off)
`BOB_NAVSEQ="idle,act;…"` (act 1=zoomin 2=zoomout 3=exit 4=fly), `BOB_NAVPAN=<bits>` (force held
pan: 1L 2R 4U 8D). Trace: `MA_TRACE_3D` logs `[map] Esc/Fly` actions.

## Not in scope (next increments, intentionally deferred — higher risk / more UI)
- **Map-item click → dossier / mission folder** (`CMapDlg::OnClickItem` opens dialogs that aren't
  rendered yet). Clicks are currently used only for drag-pan.
- **Rendering the 5 toolbars** as visible bitmap-button bars (so actions are discoverable on-screen
  rather than via keys/wheel) + the `CMapFilters` layer toggles.
- **Day-advance / debrief** toolbar (`CDebriefToolbar::OnClickedNextPeriod`).
