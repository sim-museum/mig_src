# Sprint 33 — "Play-test prep" (fixes + a script for the PO-driven session)

_2026-06-29 · Product Owner pre-approved planning + review. PO chose an interactive play-test
session to unblock the repro-gated backlog._

## Sprint Goal
Make the upcoming PO-driven play-test session productive in one pass: implement the play-test
fixes that can be nailed from code, add diagnostics for the rest, and hand the PO an exact
script (steps + trace flags + what to capture) for each backlog item.

## Implemented this sprint (ready for live validation)
- **Replay hang — graceful-degrade (`STUB3D.CPP:1438`).** Root cause confirmed in code: during
  replay playback the 3D loop's exit-key test is gated off (`!_Replay.Playback && KeyPress3d(EXITKEY)`)
  because playback is meant to end when the replay data runs out — but the replay-playback subsystem
  is effectively unimplemented, so the end never fires and the loop spins forever. Fix (`MA_LINUX`):
  honor `EXITKEY` during playback too (strict superset — normal flight unaffected), so the user can
  always escape back to the menu. Same philosophy as the S21 Quit-hang fix. `MA_TRACE_REPLAY` logs
  the launch path (`FULLPANE.CPP` `ReplayLoad`).
  - DoD demo in-session: debrief → **Replay** → press the exit key (F12) → returns to the menu, **no hang**.
  - Verified headless: normal 3D flight unchanged (boots, renders frame 200, exit 0).

## Diagnose-in-session (need the rendered/runtime state — can't fix blind)
- **Debrief "Claims" table — missing player-column header (`SAIRCLMS.CPP`).** The dialog has many
  static labels (RSTATICCTRL*/SCLAIMS_*). The gap is either an RT_DLGINIT label the parser misses or a
  dynamically-set header dropped in the compat path — distinguishable only by seeing it render.
- **Radar gunsight doesn't range/expand.** The F-86 radar-ranging reticle (`DOGUNSIGHT` shape opcode,
  scaled by target range) stays fixed size; range/lock input likely not fed on the software path.
- **Campaign-map wheel-zoom** resizes the window + patchworks tiles (present canvas tied to `m_size`).

## Play-test session script (for the PO)

Run bare (no env vars), adding only the trace flags you want:
```
cd <drive_c>/rowan/mig
MA_TRACE_REPLAY=1 MA_TRACE_KEY=1 ./wmig            # for the Replay-hang check
```
Capture a frame any time with `MA_DUMP_BACK=<n>` (→ `/tmp/maback.ppm`) or `BOB_DUMP_FRAME=<n>`
(→ `/tmp/bobframe.ppm`). Keep `stderr` (redirect `2> /tmp/play.log`) and attach it + any PPMs.

| # | Item | Steps | Flags | Capture / expected |
|---|---|---|---|---|
| 1 | **Replay hang** (fix applied) | Fly a Quick Mission → end → **debrief** → click **Replay**. If the view freezes, press **F12** (exit key). | `MA_TRACE_REPLAY=1 MA_TRACE_KEY=1` | Expect: returns to the menu, no hang. Attach `play.log` (look for `[replay] … Playback=TRUE`). |
| 2 | **Claims header** | At the debrief, open the **Claims** table. | `MA_TRACE_DLGINIT=1 MA_TRACE_STATIC=1` | Screenshot the table + `play.log`. Note which column has no header text. I'll fix from that. |
| 3 | **Gunsight ranging** | In flight, get a target in the **F-86 radar gunsight**; close to ~range. Watch whether the reticle scales. | `MA_TRACE_3D=1` | Note fixed-vs-scaling; capture a frame at lock. |
| 4 | **Campaign-map wheel-zoom** | Open the **campaign map**; scroll the mouse wheel. | — | Note the window-resize/tile-patchwork; a frame before/after. |

Bring the logs/PPMs back and I'll implement + validate #2–#4 with real data (and confirm #1).

## Definition of Done (this sprint)
- ✅ Replay-hang graceful-degrade implemented; compiles; normal flight regression-checked headless.
- ✅ `MA_TRACE_REPLAY` diagnostic added.
- ✅ Play-test script delivered for the PO session.
- ◻ Live validation of #1 and diagnosis of #2–#4 — **carried to the PO session** (the unblock).

## Commit
`Sprint 33: Replay-hang graceful-degrade + MA_TRACE_REPLAY; play-test session script`.

## PO play-test result (2026-06-29)
PO drove: Preferences → gun-camera ON (enables recording) → Quick Mission / Turkey Shoot →
shot down the MiG → Alt+X → Replay → View → picked a **pre-stored** replay.
- **Replay hang → RESOLVED.** The replay came up as a live, responsive 3D view; eject/back/quit
  all worked; process exited 0 — **no stuck state** (the S33 goal). ✅
- **Replay playback → unimplemented (new finding).** The replay loaded only the start state
  (aircraft static, gear down); the recorded flight never advanced and the VCR transport
  (keyboard or mouse) was dead — even on a *valid pre-stored* replay, so it's the **playback path**,
  not the recording, that's missing. Reclassified as a **future epic** (E2), out of this train.
- Diagnostic note: `MA_TRACE_REPLAY` didn't fire — the View→list path goes through
  `ReplayView`/`ReplaySaveBack`, not the `ReplayLoad` I traced. If E2 is ever pursued, trace the
  playback-loop advance in `STUB3D`, not the launch functions.
- PO overall: "very impressive."

## Retro
- Two of the four items were code-diagnosable (Replay hang had a clear exit-gate root cause); the
  other two genuinely need eyes on the render. Splitting "fix now" vs "diagnose in-session" keeps the
  live session focused on exactly the data I can't get headlessly.
