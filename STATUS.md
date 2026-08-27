# MiG Alley — Linux port status

Last updated: 2026-08-27 (sprint 305)

## What works

- **3D flight, replay record and playback.** Quick missions and campaign missions fly; `.cam`
  replays record, save under a chosen name, and play back with multiple aircraft. PO-verified.
- **Tacview `.acmi` export** alongside every `.cam` (EPIC L). Emits real Lon/Lat, per-aircraft type
  names, player identification, IAS. PO-verified in Tacview: a 31 s one-on-one shows the F-86
  holding heading while the MiG-15 manoeuvres behind it — confirmed against the data (MiG behind in
  95% of samples; F-86 heading swing 16°, MiG's 360°).
  **Georeferenced (S305)**: the theatre is pinned to real Korea, so a track now overlays the map
  instead of floating ~900 km NE in the Sea of Japan. Gate: `port/acmi_georef.sh` (+ `CONTROL=1`).
  `port/acmi_regeoref.py` repairs recordings made before the fix without re-flying.
- **Front end**: menus, preferences, quick mission, campaign map, Player Log, replay dialog.
- Gates: `parity_2d` (2D screens vs gold-derived references), `map_drag`, ASan, stress.

## Open, with the next concrete step

| id | what | next step |
|---|---|---|
| PO-67 | Front end laid out at 800x600 on a 1920x1080 display; ~77% black | **Dialog blocker FIXED (2026-08-27)** — the doubling was stale *pixels*: the canvas is cleared on map→panel and 3D→panel but never **panel→panel**. Now cleared on panel teardown. `MA_MAXIMIZE=1` no longer breaks the dialogs. Flipping the default is still a gate event (all refs are 800x600). |
| PO-72 | Campaign instruction / next-mission text missing after 3D exit | **Blocked on the PO** — a screenshot decides whether the text is ABSENT or drawn BLANK/ELSEWHERE. Those need opposite fixes. |
| S248 | `parity_2d` campaign_map is RED (5184 px) | **Blocked on the PO** — how many toolbar icons the campaign map shows *before* flying. Either the refs are stale (rebase) or the guard is too broad (narrow). |


## PO-67 root cause (found, not yet fixed)

The compat `ShowWindow` records visibility only:

```c
BOOL ShowWindow(int nCmdShow) { m_maVisible = (nCmdShow != 0); return TRUE; }
```

`SW_SHOWMAXIMIZED` is therefore indistinguishable from "show", so `CMainFrame::OnGoBig()` — which
`LaunchFullPane` calls *specifically* to reach full size before building the panel — changes no
size. Everything downstream then measures a stale window: layout index 1 (800) chosen at
`window=800x600`, container rect 800x600, on a 1920x1080 canvas.

`MA_MAXIMIZE=1` (OFF by default) moves the frame **and** the view, since this port has no `WM_SIZE`
propagation. Title screen becomes correct — 1280x1024 art centred exactly, coverage 23% → 62.9%.
It is off because the **dialog** screens break at that size (labels overlap and double up). At
800x600 it is a proven no-op.

## Instruments

`MA_TRACE_PRESENT` (canvas coverage + painted bbox + centre pixel) · `MA_FORCE_RESINDEX` (force the
front-end layout) · `MA_TRACE_RES` (layout choice + container rect) · `MA_TRACE_REVPAD` ·
`MA_FORCE_PADLOCK` · `MA_SHOT` (2D canvas capture) · `BOB_KEYSEQ` (inject keys with modifiers) ·
`BOB_CLICKSEQ` (font-independent click recipes).

Reverts for recent changes: `MA_NO_REPLAY_SLOT_FIX`, `MA_NARROW_TBART`, `MA_NO_READ_OVERFLOW_FIX`,
`MA_NO_SMOKE_BOUND`, `MA_NO_REVPADLOCK`, `MA_CANVAS_ARTSIZE`.

## Cautions for whoever picks this up

- **Every 2D reference is 800x600.** No existing gate can judge a display-resolution layout change.
  `PARITY_RES=1080` exists but `port/ref/native1080` is intentionally empty — see its README.
- **A 1080 reference set would be a REGRESSION oracle, not a parity one.** `ref/native` came from
  the real game; there is no gold capture at 1920x1080 and there never will be.
- **MFC sources are case-colliding twins.** `Replay.cpp` vs `REPLAY.CPP` etc. Check `build.ninja`
  for which copy is compiled before editing, or the edit will compile clean and do nothing.
- **Treat every null result as a claim about the instrument** until a positive control says
  otherwise. Several wrong conclusions this session came from probes that were unreachable, not
  switched on, or measuring the wrong instant.


## PO-67 — dialog blocker fixed (2026-08-27)

The recorded next step was "fix DIALOG placement under a widened container". Reproducing it says
otherwise.

**Maximized, `prefs_3d` renders correctly** — tab bar, nine labels, nine combos, all cleanly placed.
Only `prefs_others` breaks, and the extra labels are identifiable: *Software Driver, Gamma
Correction, Lowest Frame Rate, Ground Shading, Reflections, Weather Effects* — the **3D tab's**
controls, still drawn under the Others tab's audio labels. So the defect is **stale controls
surviving a tab switch**, not layout scaling, and "widen the container" would not have touched it.

**But it is not a removal failure either.** With `MA_TRACE_SIZE=1`, both arms trace *identically*:

```
off  [hosted.remove] parent=0xb2e8850 removed=21 remaining=63
on   [hosted.remove] parent=0x99659f0 removed=21 remaining=63
```

One call, 21 removed, 63 left — the same either way. The stale controls exist at 800x600 too;
maximizing only makes them **visible**. Next step is therefore the DRAW pass, which composes
"parent-dialog screen origin + control client-relative pos": a changed parent origin is the suspect
for why the leftovers land somewhere visible only when the container grows.

| coverage | unmaximized | maximized |
|---|---|---|
| `prefs_3d` | 22.9 % | 63.7 % (clean) |
| `prefs_others` | 20.2 % | 51.0 % (doubled) |

### ⚠️ `MA_MAXIMIZE` was a presence test — fixed

`if (getenv("MA_MAXIMIZE"))` treated **`MA_MAXIMIZE=0` as ON**. That is the opposite of what "=0"
means to anyone reading it, and it silently invalidated the first A/B run here: both arms maximized,
0.00 % difference — which reads exactly like "the flag does nothing". Every other `MA_*` revert in
this port is spelled "=1 to enable / unset to disable", so the gate now honours `0`/`off`/`no`/
`false` as OFF. Verified:

| `MA_MAXIMIZE` | coverage | `[maximize]` fired |
|---|---|---|
| unset | 22.9 % | no |
| `0` | 22.9 % | no |
| `1` | 63.7 % | yes |


### The fix

`ma_gdi_clear_screen()` was called on **map→panel** (`_wasMap`) and **3D→panel** (`_was3d`) — the S155
comment gives the reasoning: *"wherever the panel does not cover, the stale frame shows through"*.
**Panel→panel was never covered.** At 800x600 each prefs tab's art lands on the same rect, so the
previous tab is overwritten and nobody saw it; maximized, the art lands differently and the previous
tab's TEXT survives underneath.

The leftovers are stale **pixels**, not stale controls — `MA_TRACE_GHOST` shows the same three owners
in both arms (`CMIGView`, `CSSound`, `RFullPanelDial`) with no ghost panel drawing, and
`ma_ole_remove_by_parent` removes an identical 21 controls either way. A panel teardown *is* a screen
transition, so the clear now happens there. `MA_NO_PANEL_CLEAR=1` reverts.

**Verified:** the Others tab maximized shows only its own ten labels. `parity_2d`: title, prefs_3d,
prefs_others, quickmission all **byte-identical**; `campaign_map` differs by **5184 px — exactly the
pre-existing S248 figure**, unchanged.

Also added: `MA_TRACE_GHOST_EVERY=<n>`. The ghost trace fired 1-in-200 passes, so a capture at frame
110 only ever showed pass 1 — the state *before* the dialogs are built, which is not the state under
investigation.


## S305 — the `.acmi` theatre is pinned (2026-08-27)

S296 shipped the Lon/Lat as an admitted guess and recorded the next step as *"dump a named
airfield's runtime World.X/Z"*. The dump was never needed. S296's reasoning was:

> the node tables name real places (Seoul, Pyongyang, Kimpo, Sinuiju) but carry UIDs, not
> coordinates -- the positions live in the world item data, reachable only at runtime.

True of the *runtime* positions, false of the *placements*. The world is built from
`SRC/BFIELDS/*.BFI`, checked-in text where every item carries an absolute `Posn { Abs { X, Z } }`
in world centimetres -- the same frame `Replay.cpp` samples as `_ac->World.X / 100`. The
coordinates were in the repo the whole time. **The blocker was an assumption about where data
lives, not missing data**, and it survived because "reachable only at runtime" was never tested.

Two landmarks from `MAINMIG.BFI`, two unknowns per axis (X is east, Z is north):

| | world X, Z (cm) | real |
|---|---|---|
| `UID_AfBlKimpo` | 61615953, 61326673 | 37.558 N, 126.791 E |
| `UID_AfRdSinuiju` | 41409080, 89304918 | 40.100 N, 124.400 E |

**Solution:** origin (0,0) = **31.986085 N, 119.500224 E**; **110063.9 m/°lat, 84512.2 m/°lon**.

The scale is the *map's*, not the Earth's, so it no longer derives from the reference latitude --
the old `111320 * cos(refLat)` form meant moving `MA_ACMI_REF` silently rescaled the theatre.

### Why this is a measurement and not two points joined by a line

Two points always fit a two-parameter model, so the fit itself proves nothing. What does:

- The solved latitude scale is **110064 m/° against a true 111132** -- within 1%. Nothing in the
  solve forced that; a mispaired landmark would have produced no such agreement.
- **Four airfields held out of the fit** all land within 6 km on an ~800 km theatre: Suwon 0.6,
  Pyongyang 2.6, Antung 3.7, Taegu 6.0. The residuals are about the size of the pairing ambiguity
  (Pyongyang and Taegu each have more than one field), so they bound the *game's* placement error.
- The pre-existing `ma-1v1-long.acmi` opens at U=412525 V=892952 -- Sinuiju's placement to 1.5 km.
  The quick mission does start over MiG Alley, confirming U/V share the BFI frame. That shared
  frame is the one assumption the whole solve rests on, and it is now checked rather than assumed.

### Gate

`port/acmi_georef.sh` drives the **real** `ma_acmi_object()` with the six airfield positions and
reads the Lon/Lat it actually writes -- not a re-implementation of the formula, which would pass
regardless of what the shipped code did. No GL, no flight. `CONTROL=1` restores the discarded
37.5N/127.0E guess and must turn all six rows red; it does, by ~900 km.

Existing recordings keep correct U/V, so `port/acmi_regeoref.py` recomputes their Lon/Lat in
place. `ma-1v1-long.acmi`: 131.671E 45.535N -> **124.381E 40.099N**, Sea of Japan -> Sinuiju.
