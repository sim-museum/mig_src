# Sprint 114 — "The help source was in the tree" (PO-10) — ✅ CLOSED 2026-08-15 (goal MET) — ⭐ the "?" shows the real documentation

**Planned 2026-08-15 (PO pre-approved ceremonies; continuous-sprint directive). Autonomous. ~8 pts.**
**Sprint Goal:** show the actual help TEXT, after two sprints failed to decode the compiled file.

| Story | Pts | Result |
|---|---|---|
| S114-1 follow the PO's two hints | 2 | ✅ the Wine tree named the format; BoB's tree showed where help LIVES |
| S114-2 read the documentation from its source | 4 | ✅ `port/tools/rtf_help.py`, 43 topics + 186 context ids |
| S114-3 show it for the screen the player asked about | 2 | ✅ context id → symbol → topic, with the index as fallback |

## ⭐ The finding: the compiled file never had to be decoded

Three sprints had been trying to read `English/TEXT/MIG.HLP` — S98 (routing), S99 (four of five
decode stages), S112 (an 800-candidate search of the fifth, best 2/39). The PO pointed at two places
to look, and the second one settled it:

- **the Wine tree** — `WP/drive_c/windows/winhlp32.exe`, a full WinHelp viewer, which would have
  given ground truth to check a decoder against;
- **BoB** — whose help ships as `SRC/<LANG>/HELP/*.CHM` **with its sources beside it**, which is the
  clue that mattered: *this engine keeps help sources in the source tree.*

And MiG Alley does too. `SRC/ENGLISH/HELP/` holds `MIG.RTF` (75 KB — the WinHelp RTF the .HLP was
compiled FROM), `MIG.HPJ` (which even records `COMPRESS=12 Hall Zeck`, naming the compression that
cost two sprints) and `MIG.HM` (the `HID_*`/`HIDD_*` → context-number map the game passes to
WinHelp). **The documentation never needed decompressing; it needed reading.**

## What shipped

- `port/tools/rtf_help.py` — RTF → a flat, inspectable data file: `#TOPIC <ctx>|<title>` blocks plus
  `#MAP <symbol> <id>` lines. 43 topics, 186 context ids, committed as `port/data/mig_help.txt` and
  installed into the game dir by `packaging/install.sh`.
  Two things the extractor has to get right, both found by reading the output: RTF formatting is
  **scoped to its group**, so a `\v` (hidden) jump target inside `{…}` ends at the closing brace —
  without saving/restoring that, one hotspot hides the rest of the topic (the Introduction stopped
  mid-sentence at item 5). And bare newlines in an RTF file are layout, not text, so they must be
  dropped or they land mid-word ("Yo\nu are supporting").
- `SRC/compat/ma_help.cpp` — loads that file, resolves the context id the game passed
  (`HID_BASE_RESOURCE+IDD_INTRODUCTION` → `HIDD_INTRODUCTION` → the Introduction topic), and serves
  the topic's lines.
- The panel now renders the topic's **real text**, word-wrapped, with the topic index as the
  fallback when a screen's context id is not in the map — and says which it is showing.

## Evidence

Clicking "?" on the campaign map (`port/help_click.sh`, capture `port/ref/native/help_panel.png`):

> **MiG Alley - Introduction**
> On-line help is only available for the map screen
> 1. You are in command of a force of 112 aircraft arranged into seven squadrons.
> 2. In pursuit of your campaign objectives, you will design missions using this map screen.
> 3. The day is organized into three sessions: Morning, Midday and Afternoon. Missions should be
>    arranged for each session. Up to 96 aircraft can be used in each session.
> 4. Some sessions will be lost due to bad weather.
> 5. The full range of Mission Types can be designed. You can either design a mission from scratch
>    personally or just set the overall parameters and let your staff complete the details.
> 6. You are cleared to fly missions in the following aircraft: F86 Sabre, F80C Shooting Star,
>    F84E Thunder Jet and the F51 Mustang.

That is the topic WinHelp itself would have shown for that context id.

**A side benefit worth noting:** the Map Screen topic says the screen has *"five dockable toolbars:
Title Toolbar, Main Toolbar, Utility Toolbar, Scale Toolbar, Filters Toolbar"* — the game's own
documentation confirming PO-11's inventory, including the Title and Scale toolbars still to do.

## Gates

parity 5/5 · sweep 9 OPEN/0 CRASH · map click · map drag · sysbox exit · **help click (panel + the
resolved topic)** · overlay text 3/3 · stress 20/20 · ASan 0.

## Result

PO-10 closed properly — real documentation, from the game's own source, for the screen the player
asked about. The lesson is the PO's: **look in the tree before decoding a binary.** Three sprints of
decompression work were aimed at a file whose source was sitting two directories away.
