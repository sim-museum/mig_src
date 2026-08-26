# MiG Alley — Linux port status

Last updated: 2026-08-26 (sprint 304)

## What works

- **3D flight, replay record and playback.** Quick missions and campaign missions fly; `.cam`
  replays record, save under a chosen name, and play back with multiple aircraft. PO-verified.
- **Tacview `.acmi` export** alongside every `.cam` (EPIC L). Emits real Lon/Lat, per-aircraft type
  names, player identification, IAS. PO-verified in Tacview: a 31 s one-on-one shows the F-86
  holding heading while the MiG-15 manoeuvres behind it — confirmed against the data (MiG behind in
  95% of samples; F-86 heading swing 16°, MiG's 360°).
- **Front end**: menus, preferences, quick mission, campaign map, Player Log, replay dialog.
- Gates: `parity_2d` (2D screens vs gold-derived references), `map_drag`, ASan, stress.

## Open, with the next concrete step

| id | what | next step |
|---|---|---|
| PO-67 | Front end laid out at 800x600 on a 1920x1080 display; ~77% black | Fix DIALOG placement under a widened container. Root cause found (below); `MA_MAXIMIZE=1` fixes the menu screens and breaks the dialogs. |
| PO-72 | Campaign instruction / next-mission text missing after 3D exit | **Blocked on the PO** — a screenshot decides whether the text is ABSENT or drawn BLANK/ELSEWHERE. Those need opposite fixes. |
| S248 | `parity_2d` campaign_map is RED (5184 px) | **Blocked on the PO** — how many toolbar icons the campaign map shows *before* flying. Either the refs are stale (rebase) or the guard is too broad (narrow). |
| — | `.acmi` theatre placement is arbitrary | Dump a named airfield's runtime World.X/Z, pair with Kimpo 37.558N 126.791E and Sinuiju 40.100N 124.400E, solve offset+scale. |

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
