# Sprint 95 — "The map was never told" (PO-3) — ✅ CLOSED 2026-08-09 — recon dossier opens from a map icon

**Planned 2026-08-09 (PO pre-approved ceremonies; continuous-sprint directive). Autonomous. ~8 pts.**
**Sprint Goal:** close PO-3 from the play-test — *"clicking an icon, e.g. an airfield on the map,
has no effect when it should bring up the recon dialog."*

| Story | Pts | Result |
|---|---|---|
| S95-1 route map clicks to `CMapDlg` | 5 | ✅ dossier opens, verified by capture |
| S95-2 make it a gate that cannot go stale | 1 | ✅ `port/map_icon_click.sh` |
| S95-3 gates | 2 | ✅ |

## What was actually wrong — nothing in the game code

The whole chain was already there and already correct:

```
CMapDlg::OnLButtonDown  -> FindMapItem(point) -> m_buttonid
CMapDlg::OnLButtonUp    -> OnClickItem(m_buttonid)
                        -> CMainToolbar::OpenDossier(uid)
                        -> CTargetDossier::MakeSheet(...)      <- the PO's "recon dialog"
```

`CMIGView::OnLButtonDown` is empty (`CView::OnLButtonDown` and nothing else) — on Windows the map
dialog got its clicks from the message queue, which this port does not have. The map idle loop
routed a click to the OOB dialogs, then the system box, then toolbar2, then toolbar1 — and if all
of those declined, **dropped it**. `m_mapdlg` was the last unrouted click consumer in the game.

**Fix:** when nothing above claims the click, it belongs to the map, so hand it to the map's own
handlers (`MA_NO_MAP_ITEM_CLICK` reverts). As with the listbox and button paths, the port drives
the *engine's* handlers rather than re-implementing the hit-test — `FindMapItem` knows about
bands, filters, scroll and zoom, and a reimplementation would be wrong the first time any of those
changed.

Down and Up are delivered in one call, which is what a click with no intervening motion is. That
is not just convenient: it keeps `m_bDragging` FALSE, so we take the `OnClickItem` path and
**never enter `CMapDlg::OnMouseMove`, which dereferences `GetDC()` unchecked** in this port — the
S82 rule that the genuine handler you drive may itself contain an unported call. (That same
`OnMouseMove` is where PO-2, the drag corruption, is going to be fought; this sprint deliberately
does not open that door.)

The handlers are `protected`, so the seam is one public method, `CMapDlg::MaDriveClick`, under
`MA_LINUX` — better than a `friend` or a cast, and it documents itself at the declaration.

**Verified:** clicking the icon at (582,378) resolves to item `0x264b` and the **DOSSIER** sheet
renders with real campaign data — *Yonchon Supply Dispersal*, MSR Central, Threat AAA Medium /
MiG 15 Low, Activity Very Low, Repairs Operational, Last Sortie (never) — over the recon photo,
with Details/Damage/Notes tabs. The clicked icon also redraws as selected, so `RedrawIcon`,
`ConvertPtrUID` and `ScreenXY` all survive the port unaided.

## ⚠ The lesson that shaped the gate: a coordinate is not a test

The first click, on a coordinate read off a scan, hit **nothing** — `hit id=0`. Same binary, same
pinned save; the canvas had grown **800×600 → 1021×644** between the scan frame and the click
frame, moving every icon by ~108 px. It would have been easy to read that as "the routing does not
work" and go looking for a bug that was not there.

So the gate never names a point. It asks the map's own hit-test where the icons are at the frame
it is about to click (`MA_MAP_ITEM_SCAN=<frame>`), then clicks the first one clear of the toolbars
(`MA_MAP_CLICK_FIRST=1`). `port/map_icon_click.sh` PASSes on *item hit + dialog painted +
survived*, and pins `campaign_pristine.sav` the way `oob_sweep.sh` learned to in S94 — icons come
from campaign state, so an advanced save changes what is clickable.

This is the third distinct form of the same project-wide failure: a check whose result depends on
state the test does not control (S81 parity/save, S94 sweep/save, S95 click/coordinate).

## Gates — all under `gl-lock`
- **2D parity 5/5 byte-identical** · **OOB sweep 9 OPEN / 0 CRASH** · **stress 20/20** ·
  **ASan 0 reports** · **new: map icon click PASS**

## Result
PO-3 closed. Three of the five play-test defects are now root-caused and one is fixed; the map's
icons went from inert to opening the dossier the PO expected, without a line of gameplay logic
being written — the defect was entirely in what the port failed to deliver.
