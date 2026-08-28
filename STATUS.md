# MiG Alley — Linux port status

Last updated: 2026-08-28 (sprint 314)

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
| PO-67 | Front end laid out at 800x600 on a 1920x1080 display; ~77% black | **S312 flipped `MA_MAXIMIZE` on by default and S314 REVERTED it — the item is OPEN again.** The maximised front end draws correctly and **does not respond to clicks**: `panel_click` traces `listbox id=2063 rect=(810,370,105,100) vs (1100,470) miss` — the menu is painted at (1100,470) while its hit rect is at (810,370) and is 105x100, far too small for a seven-item menu, so the listbox's rect comes from a different layout than the one that drew it. Pre-existing; the flip only exposed it. **Next: make the menu listbox's rect follow the chosen layout, then re-flip.** `MA_MAXIMIZE=1` still gives the correct-looking 1080 layout for development. |
| PO-72 | Campaign instruction / next-mission text missing after 3D exit | **Blocked on the PO** — a screenshot decides whether the text is ABSENT or drawn BLANK/ELSEWHERE. Those need opposite fixes. |
| S248 | `parity_2d` campaign_map is RED (5184 px) | **NO LONGER BLOCKED ON THE PO (S313).** The 5184 px is exactly **three whole icon cells** — two 48x48 toolbar buttons (x 286..333, x 748..795, y 52..99) and one 24x24 filter button (x 624..647, y 28..51); 2x2304 + 576 = 5184, with no partial or shifted pixels anywhere. In the gold-derived reference all three cells show **bare map**; the port draws **buttons** in them. So of the two options this row listed, it is "**the guard is too broad**", not stale refs. Suspect: `CRToolBar::OnGetFile` (SRC/MFC/RTOOLBAR.CPP:~627), widened from `0x6800..0x7100` to `0..0xFFFF`. Next: trace those three buttons' FileNums and find what distinguishes them. |
| PO-74 | Campaign buttons wrong | **PO 2026-08-27**, from `/home/admin/Videos/260827_ma_campaign1.mp4` — a full campaign-1 run recorded on the port. Correct the campaign screen's buttons against it. Related to S248 (`campaign_map` differs by exactly three whole icon cells, two 48x48 toolbar buttons + one 24x24 filter button, where the gold-derived reference shows bare map) — check whether these are the same defect before treating them separately. |
| PO-75 | Pre- and post-mission campaign messages wrong | **PO 2026-08-27**, same video. The briefing/debrief text around a campaign mission does not match. Distinct from PO-72 (campaign instruction text missing *after 3D exit*), which is about text being absent; this is about the wrong text being shown. Check whether fixing one resolves the other. |


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


## S313 — S248 answered by measuring the diff instead of asking

The row said: *"how many toolbar icons the campaign map shows before flying. Either the refs are
stale (rebase) or the guard is too broad (narrow)."* That is a question about a picture, and the
picture was already committed — the difference has been sitting in `parity_2d`'s output every run
since S248.

Clustering the 5184 differing pixels gives **exactly three connected components**, each a complete
icon cell:

| | bbox | size | px |
|---|---|---|---|
| toolbar button | x 286..333, y 52..99 | 48x48 | 2304 |
| toolbar button | x 748..795, y 52..99 | 48x48 | 2304 |
| filter button | x 624..647, y 28..51 | 24x24 | 576 |

2x2304 + 576 = 5184, and **nothing else differs** — no partial glyphs, no one-pixel shift, no
antialiasing fringe. Whatever this is, it is whole buttons appearing or not appearing.

Cropping the three cells from both images settles the direction: the **reference shows bare map**
(terrain, sea, terrain) and the **port shows toolbar buttons** (a star, a slashed rectangle, part of
a star). `ref/native` came from the real game, so the real game does not draw these three at this
pinned campaign state and the port does. **The guard is too broad.** No PO screenshot needed; the
choice between the two options was decidable from data already on disk.

**Leading suspect, not a conclusion.** `CRToolBar::OnGetFile` was widened from the old
`0x6800..0x7100` range to `filenum <= 0 || filenum > 0xFFFF` so that both WM_GETFILE handlers would
answer alike — a change made for good reasons (the narrow range was skipping 732 icons in one
session). Widening it supplies art to buttons that previously came back empty, and three of them
are ones gold leaves blank. That fits, but it is a hypothesis until the three FileNums are read.

**Next step:** capture campaign_map with the toolbar art trace on, read the FileNums for those three
slots, and find what separates them from the 615 that legitimately draw. `MA_NARROW_TBART=1` is the
A/B — if it removes exactly these three and nothing else, the mechanism is confirmed; if it removes
hundreds, the old range was never the right discriminator and the answer lies in the buttons' own
display condition, not in the art lookup at all.


## S314 — the S312 flip is reverted, and so is the refactor I tried to rescue it with

**S312 was wrong to ship, and the gates I chose are why I did not know.** parity_2d passed 5/5 at
1080 and 4-identical at 800; map_drag was lossless. All true, and all blind to the defect:
`port/panel_click.sh` is the gate that clicks what is drawn, and it reported

```
menu located at pixel (1100,470)
the menu was drawn at (1100,470) but clicking there did nothing
```

A front end that looks finished and ignores every click is the worst shape a UI regression can
take, and it shipped because I ran the gates that covered the change I had in mind rather than the
suite. I flagged the missing suite verdict in the S312 commit; flagging it is not the same as
having it.

Isolated by A/B on panel_click, since two of my changes were live at once:

| arm | result |
|---|---|
| `MA_MAXIMIZE=0` + panel offset **on** (S311 alone) | **PASS** |
| maximise **on** + panel offset off | **FAIL** |

So it is S312's flip, not S311's control placement. Traced: `listbox id=2063 rect=(810,370,105,100)`
against a click at (1100,470). The rect is 105x100 — far too small for a seven-item menu — so the
listbox's rect is computed from a different layout than the one used to draw it. **Pre-existing in
the maximise path; the flip only exposed it.** Default back to opt-in until it is fixed.

### The refactor that was supposed to help, and made it worse

S311 added the panel-art offset to the draw pass. Six sites compute that same origin, so I factored
them into one `hosted_origin()` — draw, hit-test, listbox click, scrollbar, and the `#id` resolver.
Structurally right, and it broke the 1080 gate: `prefs_others` and `campaign_map` went to 1.3M and
2.0M differing pixels, deterministically across three runs. The capture showed the **3D tab** where
the recipe asks for **Others** — the recipes navigate by clicking, and changing hit-testing changed
where their clicks land.

I then found a real asymmetry underneath (`ma_ole_click` omits the `h.type != CT_LISTBOX` term that
the draw pass and the `#id` resolver both carry, so a game-positioned listbox's hit rect sits
`parent->m_maX` from where it is painted) and aligned it. **That did not fix the 1080 arm either** —
recorded because it is a real inconsistency worth fixing later, and because "I found a plausible
asymmetry" is not the same as "I found the cause".

Reverted to the S311 state the 1080 references were blessed against. Re-blessing was not an option:
the new captures are the *wrong screens*, so committing them would enshrine broken navigation as
the expected result — the exact failure `ref/native1080`'s own README was written to prevent.

**Verified after the revert:** parity_2d 1080 **5/5 byte-identical**, parity_2d 800 four identical
with campaign_map at its unchanged 5184 px, panel_click **PASS**.
