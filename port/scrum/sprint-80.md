# Sprint 80 — "Fly the loop" (G2) — ✅ CLOSED 2026-08-08 — ⭐ the flyable multi-mission campaign loop runs

**Planned 2026-08-08 (PO pre-approved ceremonies). Autonomous. Committed ~8 pts.**
**Sprint Goal:** close G2's remaining item 1 — drive the campaign debrief's *Next Period* and prove
the full **fly-M1 → debrief → next period → fly-M2** loop, the thing S77/S78/S79 converged on.

| Story | Pts | Result |
|---|---|---|
| S80-1 `MA_CAMP_LOOP` drives Next Period out of the debrief | 3 | ✅ |
| S80-2 fly-M1 → fly-M2 verified headless | 3 | ✅ (went further — the campaign ran to its END state) |
| S80-3 cross-port: lessons sync + note 18 processed, note 29 sent | 2 | ✅ |

## Execution log

### S80-1 — the loop drive — DONE
`OnFlyingClosed`'s CAMP branch (`FULLPANE.CPP:2674`) already set `MMC.indebrief`, called
`NextMission` and returned to the map with the **debrief** toolbar up (S79). The missing link was
that toolbar's Next Period button. Added, all `MA_LINUX` and env-gated (default off):
- `ma_camp_indebrief()` / `ma_camp_next_period()` / `ma_camp_state()` (`MAINTBAR.CPP`, where `MMC`
  and the toolbar accessors are already in scope). `ma_camp_next_period` calls the **genuine**
  `CDebriefToolbar::OnClickedNextPeriod` (`DBRFTLBR.CPP:226` → `MMC.EndDebrief` → `ChkEndCampaign`
  → `UpdateToolbars` → the new period) rather than reimplementing `EndDebrief`.
- `MA_CAMP_LOOP=N` in the map idle (`MIG.CPP`): when `indebrief` and the map has settled, drive
  Next Period, then **re-arm** the frag/Fly drives, N missions total.

### S80-2 — the loop, and what it actually did — DONE
**The blocker was in the harness, not the game.** `if (++n == N)` on a function-local static fires
exactly **once per process**, and this path had *three* of them — the frag drive (`_fragn == 40`),
the Fly click (`_flyn == 30`) and `BOB_AUTOEXIT` (`_aef == atoi(ae)`). Mission 2 fragged, launched
into 3D and then **flew forever**, because the exit counter had been spent on mission 1. Two of the
three had to be hoisted out of their own blocks before they could even be reset. `BOB_AUTOEXIT` is
now **per flight**, re-armed on every 3D→front-end edge, so each flight gets its own N frames.

**Verified** (`MA_CAMP_FLY=1 MA_CAMP_LOOP=2 BOB_AUTOEXIT=40`, SDL dummy, under `gl-lock`), with the
campaign's **own date/period readout** logged each step so the proof is campaign progression and not
"the buttons got pressed":

```
[map]  driving frag -> singlefrag        [campaign: 7/8/50:  Morning, planning]
[3d]   BOB_AUTOEXIT=40 -> flight exit
[debrief] CAMP branch: indebrief=TRUE, calling NextMission
[loop] mission 1 debriefed -> Next Period [campaign: 7/8/50:  Morning, debrief]
[loop] after Next Period                  [campaign: 7/19/50: Morning, planning]
[loop] re-armed frag/Fly drives for mission 2
[map]  driving frag -> singlefrag        [campaign: 7/19/50: Morning, planning]
[3d]   BOB_AUTOEXIT=40 -> flight exit
[loop] mission 2 debriefed -> Next Period [campaign: 7/19/50: Morning, debrief]
[loop] after Next Period                  [campaign: 7/20/50: Morning, planning]
```

**More than the goal: the campaign reached its END state.** The post-loop capture
(`port/ref/native/campaign_loop_endcamp.png`, armed by the drive itself — see below) is not the map
but the **end-of-campaign screen** — the UN-defeat narrative with Quit/Load, i.e.
`OnClickedNextPeriod` took its `campend` branch (`DBRFTLBR.CPP:264` `LaunchFullPane(endcamp)`)
because `ChkEndCampaign()` returned true. That is correct behaviour, not a fault: `BOB_AUTOEXIT`
abandons each mission after 40 frames, so both scored **Failure** ("Osan-Pyongtaek Highway /
Reconn / Failure" in the results table), and the strategic sim ran the UN to defeat. **So the whole
campaign lifecycle — mission → flight → debrief → next period → … → end-of-campaign — now runs
end-to-end in the native port.**

**Capture recipes must not contain magic numbers either.** `MA_SHOT=N` fires at an absolute idle
number, which cannot be aimed at the end of a two-mission loop (the count depends on how long the
flights took). The drive now **arms its own capture** (`MA_CAMP_LOOP_SHOT` → countdown → dump+exit)
— the same rule S62/S63 applied to click coordinates.

### S80-3 — cross-port — DONE
- Shared lessons doc **byte-identical** in both repos; inbound **BoB note 18** processed:
  - §1 `Select(row, COLUMN)` hardcoded-column bug → **checked, N/A for MA**, and the structural
    reason recorded: MA's host never reimplements the hit-test (`MaMouse` drives the control's real
    `OnLButtonDown/Up` and reads back `m_iRowSel`/`m_iColSel`), plus `OnSelectRlistbox` uses
    `max(row,column)`, so a hardcoded 0 could never have gone latent here.
  - §3 their open question (dismiss a logged dialog headlessly) → **answered**: `OpenXxx` is
    *ensure-open*, `OnClickedXxx` is the real toggle, and a scaffold should call
    `CloseLoggedChild(idx)` directly.
  - **§8s (nested `DialList` screens)** — BoB's section says it applies to MA "verbatim";
    **assessed: it does not, for the map OOB dialogs.** MA reached the same place independently
    (S53→S70) with `ma_oob_paint_tree_rec` (`MIG.CPP:847`), which recurses `fchild`/`sibling` and
    honours `m_maVisible` (the engine's own `SW_SHOW`/`SW_HIDE` on tab pages) instead of
    synthesizing row geometry. MA's Career/Log tables had a *different* root cause — the OOB draw
    path had no `CT_LISTBOX` case (S70). BoB's synthesized-`rowStep` recipe is **banked** for
    MA's order-of-battle `DialList` screens, which are not hosted yet. Verdict: "checked,
    different root cause, no current symptom".
- **MA note 29 sent** (+ §8v of the shared doc): the one-shot-static harness trap, the
  stateful-oracle rule, the §8s verdict, and the possible shared-`fileman` truncated-save bug below.

## Gates — all green (every run wrapped in `gl-lock`; 3 sibling sessions were active)
- **Build:** clean, links the 32-bit ELF, 0 undefined symbols.
- **2D parity:** `title` / `prefs_3d` / `prefs_others` / `quickmission` — **4/4 at 0 px,
  byte-identical**, via the new `port/parity_2d.sh`.
- **Stress `stress_launch.sh` 20 runs: PASS 20/20** (reached & sustained 100 3D frames).
- **ASan `asan_all.sh 2 80`: PASS — 0 reports across all 4 paths**, each reached 2/2
  (flight + campaign map/fly/nextday; the campaign modes exercise the new drive).
- *Honesty note:* stress and ASan were first run just before a whitespace-only indentation fix in
  `MIG.CPP`. Rather than argue that codegen was unchanged, the **parity and stress gates were
  re-run on the final binary** (results above are the re-runs); the ASan build predates the
  whitespace fix and was not rebuilt for it.

### New: `port/parity_2d.sh` — the parity gate is now one command
The gate's recipes had been re-derived by hand from prose in `screen-parity.md` every sprint, which
is the same fragility S62/S63 fixed for click coordinates. They now live in one script that captures
and pixel-compares against `port/ref/native/`. Two things it caught immediately:
- The doc's `Others` tab x≈299 is **stale** (a later font change moved the tab bar) — my first run
  silently captured the **Game** tab instead. Replaced with the font-independent
  `#2063:6` (`IDC_RLISTBOX` column 6). *The documented trap, re-sprung by trusting documented pixels.*
- **`campaign_map` is not a valid byte-identical oracle** and is now excluded by default: it renders
  live campaign save state, which this repo's own `MA_CAMP_FLY`/`MA_CAMP_LOOP` runs advance. It came
  back 8095 px different; **A/B settled it in one step** — the pre-S80 binary produced a
  *byte-identical* capture to the S80 binary, so the delta is state drift, not code. (Same class as
  `#7 prefs_controls`, which embeds live joystick state.)

## Discovered — top of the G2 backlog for S81
**The campaign autosave writes a truncated filename.** The port writes
`SaveGame/Auto Save.sa` (30113 bytes, written by every campaign run) — one character short of the
`Auto Save.sav` that `CFiling::SaveGame` (`FILING.CPP:135`) is asked to write; the canonical
`Auto Save.sav` has not been touched since 2026-07-19. The name goes through
`File_Man.fakefile(FIL_SAVEGAMEDIR, fname)` → `namenumberedfile(..., buffer[150])`
(`Fileman.cpp:256`, `strncpy(namedirdir+128, filename, 80)`), i.e. fixed-width name buffers.
Evidence it is load-bearing: parking `Auto Save.sa` makes the standing campaign nav recipe fail to
reach the map at all. This is **G2 item 2 (state persistence)** with a named, evidence-backed
mechanism to chase — a good S81 opener, and shared to BoB since `fileman` is shared engine code.

## Result
G2's flyable multi-mission loop **works**: two campaign missions flown back-to-back in one process,
each debriefed, the period advanced between them, and the campaign carried through to its own
end-of-campaign screen. The blocker turned out to be three one-shot counters in the *test harness*
that had been reading as a game limitation for the port's whole life.
