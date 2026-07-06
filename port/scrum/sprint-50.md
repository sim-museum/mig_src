# Sprint 50 — "CRToolBar Phase-3: toolbar buttons clickable" (autonomous, headless DoD)

**Goal:** make the campaign map toolbar buttons functional (click → fire the button's `ON_EVENT` handler).

## Implementation
- `ma_ole_toolbar_click(dialog, ox, oy, sx, sy)` (`ma_olecontrol.cpp`): hit-tests a screen click against the
  toolbar's hosted buttons at the SAME origin `ma_ole_draw_toolbar` used, then fires
  `ma_evt_fire(toolbar, &typeid(*toolbar), ctrlId, 1/*Clicked*/)` → the registered `ON_EVENT` handler
  (`OnClickedFrag2`/`OnClickedDis`/...). The eventsink + `CMainToolbar` `BEGIN_EVENTSINK_MAP` were already
  present.
- Wired in the `MIG.CPP` map branch via `ma_mouse_take_click` (the map otherwise uses only nav-take/drag).

## Validation (headless DoD)
- Click Frag2 (rect 485,52,48,48): `[tbclick] id=1905 -> fire` → `OnClickedFrag2` → `[LaunchFullPane]`
  (the mission briefing launches). The toolbar is functional.
- Categorized all 10 main-toolbar buttons: **7 safe** (Frag2/Dis/Bases/Weather/Overview/Playerlog/Packages)
  fire cleanly; **3 crash** (Authorise/Squads/Directives) — their `OnClicked` derefs an unbuilt `fchild`
  tree (`fault_addr=0xd0`), the OOB-info dialogs (Squadrons/…) don't build in the port yet (the OOB-render
  epic, cf. BoB S99-101). **Blacklisted** those 3 (consume the click, don't fire) → no crash.
- **`port/asan_campaign.sh` PASS** + a `load→map→click-Frag2` ASan run **0 reports**.

## Increment
The campaign map's main toolbar is now **interactive** — clicking Fly (Frag2) launches the mission briefing,
and the other action buttons fire their handlers; the not-yet-rendered OOB-info dialogs are safely deferred.

**Next:** the OOB-info dialog render epic (unblacklist Authorise/Squads/Directives once their
`MakeTopDialog`/`HTabBox` trees build — §8c `Edges` idiom + BoB S99-101 fileblock/positioning); filter-toolbar
icons; the latent `Curve` static-teardown `new[]`/`delete[]`.
