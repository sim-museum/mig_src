# Sprint 96 — "The screen was the wrong size" (PO-2) — ✅ CLOSED 2026-08-09 — ⭐ the campaign map had been 221 px too wide since it first rendered

**Planned 2026-08-09 (PO pre-approved ceremonies; continuous-sprint directive). Autonomous. ~8 pts.**
**Sprint Goal:** close PO-2 — *"click-drag on the map messes up the display."*

| Story | Pts | Result |
|---|---|---|
| S96-1 root-cause and fix the drag corruption | 5 | ✅ fixed in the compat GDI |
| S96-2 a drag is not a click (S95 regression) | 1 | ✅ fixed and proven |
| S96-3 gate it | 2 | ✅ `port/map_drag.sh` |

## Root cause: a blit that overhangs the screen was allowed to *enlarge* the screen

`ma_gdi`'s `SetDIBits`/`StretchDIBits` grew the canvas to fit whatever was drawn. Windows clips a
DC blit to the client area; nothing about a blit's size tells you the screen got bigger.

The campaign map is **tiled** (`CMIGView::UpdateBitmaps` draws 256×256 blocks). The moment it
scrolls, tiles hang off the edges — and each overhanging tile made the whole screen bigger, **every
frame of the drag**:

```
[canvas] stretch_dibits at(0,456) dest=256x256 ...
[canvas] grow -> 1024x712
[canvas] stretch_dibits at(0,460) dest=256x256 ...
[canvas] grow -> 1024x716        ... and so on, for as long as the drag continues
```

**Fix:** growth is only legitimate from a blit anchored at or above the origin — something
*establishing* the screen, not content spilling off it. Everything else clips
(`MA_CANVAS_GROW_ANY=1` restores the old behaviour for A/B).

### ⭐ The bigger finding: this was ALSO happening at rest

The trace on a plain boot, no drag, no input:

```
[canvas] set_dibits at(0,0) dib=800x600 dest=800x600     <- the front end establishes the screen
[canvas] grow -> 800x600
[canvas] stretch_dibits at(-111,388) dest=256x256        <- a map tile hanging off the bottom
[canvas] grow -> 800x644
[canvas] stretch_dibits at(657,-124) ... 30 growth events ... -> 1021x644
```

**The campaign map screen has been 1021×644 for as long as it has rendered — 221 px wider and
44 px taller than the game's actual 800×600 screen — because its own tiles inflated it.** Every
other screen in the port is 800×600; the map was the odd one out and nobody asked why. It is now
800×600 like the rest, and the map fills it correctly.

This also explains a PO-1 detail: anything positioned relative to the right edge (the system box,
`_cw - _bw - 4`) was being placed against a screen edge that was not where the screen ended.

**The `campaign_map` parity reference was re-baselined** — deliberately, and this is the only
screen that changed. The old reference (`1021×644`) encoded the bug; the other four screens stayed
byte-identical, which is the evidence that the clipping change did not disturb the front end. The
Wine gold (#7, 1917×1077) is a maximised-window capture and does not settle the native resolution;
the game's own mode list (640/800/1024, 4:3 only) and the other four screens do.

## S95 regression, found and fixed in the same area
A drag ends in a left-button release, which raised the same one-click edge as a tap — so since S95
routed map clicks to `CMapDlg`, **every pan finished by opening a dossier**. Windows only fires a
control when press and release land together; the port now requires that too (≤4 px). Synthetic
`BOB_CLICKSEQ` injection does not go through this path, so no recipe changes.

## ⚠ The test lied first, and only assertion #1 caught it
The first version of the drag gate reported a **perfect lossless round trip** — while the drag was
doing **nothing at all**. `BOB_DRAG` pushes real SDL events on purpose (a hook that bypasses the
path it tests proves nothing — §8-MA93), and **the event queue was never drained without a window**:
S93 moved the synthetic hooks above `if (!g_win) return;` but left the guard in front of
`SDL_PollEvent`. The same bug, in its other half, one sprint later.

`0 px differ` is indistinguishable from `nothing happened`. The gate therefore asserts **three**
things, and the first one exists purely to make the second one mean something:

1. one-way drag **≠** baseline — the drag really moves the map (**288562 px**)
2. round trip **==** baseline — panning is lossless (**0 px**)
3. the drag release is **suppressed** as a click — a pan does not open a dossier

## Gates — all under `gl-lock`
- **2D parity 5/5** (campaign_map re-baselined, 4 unchanged) · **OOB sweep 9 OPEN / 0 CRASH** ·
  **map icon click PASS** · **map drag PASS** · **stress 20/20** · **ASan 0 reports**

## Result
PO-2 closed, and the defect behind it was three years older than the drag: the campaign map had
never been the size of the screen it was drawn on. Four of the five play-test defects are now
root-caused, two closed. The drag hook also makes mouse-held interaction testable headlessly for
the first time — PO-1's remaining work and any future map interaction can now be gated.
