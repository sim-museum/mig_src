# Sprint 162 — "Authorize" (EPIC K, K4) — ✅ CLOSED 2026-08-21 (goal MET, 8/8) — ⭐ the Wonju mission exists in the campaign

**Planned 2026-08-21** (PO ceremonies pre-approved). Script step 7, the step everything after it edits.
**Sprint Goal:** Authorize on the Wonju dossier creates a Minimum Strike mission, and the campaign
lists it.

| Story | Pts | Result |
|---|---|---|
| S162-1 drive Authorize; report what opens | 3 | ✅ the profile chooser, with **Minimum Strike** preselected |
| S162-2 make the choice commit | 3 | ✅ Load creates the mission; the **Mission Folder** lists it |
| S162-3 *(swapped)* address a listbox ROW, not its centre | 2 | ⭐ the recipe had been picking the one profile the PO says not to |

> **Scope note.** S162-3 was planned as K3 (the dossier's Damage tab). It was replaced mid-sprint by
> the `:rN` recipe work below, which S162-2 turned up and which had to be fixed before this gate
> could honestly claim anything. K3 moves to S163 — not dropped, deferred, and said so here.

## What Authorize does

`DossierButtons::OnClickedAuthorise` → `MainToolBar().OpenLoadProfile(uid)` →
`CLoadProf::MakeSheet` — a three-tab chooser (`Standard / User Strike / User Patrol`) over a `CLoad`
file list. Driven headlessly it comes up correct and populated:

```
Minimum Strike            <- current selection
  Minimum Strike
  Napalm Strike
  Fighter Bomber Strike           [ Load ]
```

which is exactly the script's step 7 — *"Authorize → pick Minimum Strike (not 'Fighter Bomber
Strike' — that auto-fills everything)"*. Clicking Load then opens **two** dialogs:

- **WONJU SUPPLY DUMP** — `Wave / ToT / Main Duty / AAA Cover / Air Cover`, row `1.Bomb 08:30 F80 (2)`,
  buttons `Route  Task  Save  Ins Wave  Del Wave`;
- **MISSION FOLDER** — `Objective / Task / ToT / Flights`, listing
  `Munsan-Seoul Rail-line  Reconn  05:40  1` and **`Wonju Supply Dump  Bomb  08:30  2`**,
  buttons `Intelligence / Profile / Delete / Frag`.

**The mission exists in the campaign.** K4 delivered.

### Two corrections to the S158 walkthrough, both from seeing the real thing

1. **The bottom-left dialog is the MISSION FOLDER, not a "COMBAT ORDER".** S158 named it from a gold
   frame where it runs off the left edge of the game window and only `…DER` is legible. The port's
   own copy is fully on screen, and gold's `Delete` / `Frag` buttons match it. *A title read from a
   truncated capture is a guess — say so, or go and get the untruncated one.*
2. Gold's wave reads `F84 (2)` where the port reads `F80 (2)`. **Not a defect:** ToT, wave count,
   columns and buttons all match, and the aircraft type is the game's own choice from the squadrons
   available on the save's date — the pinned fixture is 25 June 1950, day one of the campaign, and
   the PO's recording is from a later date. Recorded rather than "fixed".

## The recipe was clicking the wrong row

`#1055@CLoad` resolved to the listbox's **centre**, which on a three-row list is **row 2 — "Fighter
Bomber Strike"**, the one option the walkthrough names as the wrong answer. The mission was still
created, the trace still looked right, and the gate would have passed while testing the profile the
PO told us not to use. That is S85's failure mode (*"first prove it addressed the control you
meant"*) arriving through a different control type.

`#ID@Class:rN` now names a row, resolved through the control's **own `GetRowFromY`** — the same
technique as the existing `:COL` form's `GetColFromX`, so it survives a font or row-height change and
never encodes a pixel. Parsed before the generic `:%d` form, because `%d` fails on `r0` and would
otherwise fall through to the unqualified match and silently drop the row — the same shape as the
`:?` bug already recorded in that parser.

> **Stated honestly: on this save both profiles produce the same wave** (`1.Bomb 08:30 F80 (2)`
> either way), which is *why* the wrong row was invisible. This fix corrects what the recipe
> **addresses**, not — today, here — what it produces. The value is that the gate now means what it
> says; claiming it fixed broken missions would be an overclaim.

## New gate

`port/authorize_mission.sh` — target found **by name**; Authorize fires; **row 0 (Minimum Strike)**
is the row selected; Load is addressed; the MISSION FOLDER appears; and its list body has real row
ink (a dialog can open empty, and steps 8–11 are judged by the `Flights` number in this very list).

One assertion was written on the log and moved to the outcome: the Load button's `-> fire` trace is
not reliably flushed before the capture, so **the mission folder appearing** is what proves Load ran.
An assertion on a log line that races the thing it describes is not an assertion.

## Gates

`authorize_mission.sh` **PASS**; `parity_2d` 5/5 byte-identical; `oob_sweep` OPEN=9 NONE=0 CRASH=0;
`map_filter` PASS (64,115 px); `dialog_scroll` PASS; `help_click` PASS. The click-path files
(`ma_olecontrol.cpp`, `bob_video.cpp`) are shared by every recipe in the tree, which is why the whole
click-driven gate set ran rather than just the new one.
