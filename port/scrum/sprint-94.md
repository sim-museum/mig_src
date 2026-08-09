# Sprint 94 — "What the PO found" — ⚠️ CLOSED PARTIAL 2026-08-09 — five play-test defects triaged, two root-caused

**Planned 2026-08-09 (PO pre-approved ceremonies; continuous-sprint directive). Autonomous.**
**Sprint Goal:** triage the defects the PO reported from a live play session under gdb — the first
human play-test this port has had in this session — and fix what can be fixed.

## The PO's report (all five logged as backlog items)
| # | Report | Status |
|---|---|---|
| PO-1 | No way to exit the campaign — no exit/resize widgets upper right | ◐ root-caused, half-fixed |
| PO-2 | Click-drag on the map corrupts the display | ⬜ logged |
| PO-3 | Clicking a map icon (e.g. an airfield) does nothing; should open the recon dialog | ⬜ logged |
| PO-4 | The "?" button on dialogs yields no documentation screen | ⬜ logged, cause known |
| PO-5 | Overlay TEXT does not print — padlock info, map menu, radio menu | ◐ deep chain, still open |

**The play-test itself ran clean:** ~12 minutes under gdb, exited normally on window close, no fault.
*(Note for future logs: the gdb `-ex` commands after `run` fire unconditionally, so the
"FAULT CAUGHT" banner in `/tmp/ma_gdb.log` is not evidence — `bt full` printing "No stack." is.
The runner script now says so; a banner that fires either way is the S93 trap in another costume.)*

## PO-1 — the exit widgets: root-caused, half-fixed
The upper-right cluster is **`CSystemBox`**, and it hosts exactly what the PO went looking for:
`IDC_FILES` → `OnBye()` (**the exit**), `IDC_ZOOMIN` → `OnGoNormal()` (resize), `IDC_THUMBNAIL` →
`OnGoSmall()`. `CMainFrame` creates it and `RDialog` enables/disables it — **but nothing ever drew
it**: the map idle drew `m_toolbar1` and `m_toolbar2` and nothing else.

Landed: the box is now drawn and click-routed, positioned at the canvas's top right from **its own
control extent** (new `ma_ole_dialog_extent`) rather than a hardcoded width — the same rule the
click recipes follow. Runtime confirms 3 controls, all visible, extent 72×48.

**Not finished:** it renders **blank**, because those three ids have no entry in the icon-art table,
so the buttons have no artwork. Positioned and clickable but invisible is not a fix.

## PO-5 — overlay text: a four-step chain, still open
1. `DrawInfoBar` returns early on `Save_Data.infoLineCount==0 && messageTimer==0`, and the PO's save
   has **`infoLineCount=0`** — so the HUD info line was never attempted. That part is a *setting*.
2. Forcing it on (`MA_FORCE_INFOLINES`) proves a **real defect underneath**: `DrawTopText` runs, the
   font image map resolves to a valid pointer, glyphs blit — and nothing appears.
3. Measured at the pixel level: glyphs draw through **palette slot 252**, and the engine does
   `SetPaletteEntry(252, GetPaletteEntry(fontColour))` — but **`WHITE == 252`** (`Palette.H:45`), so
   for normal white text that is a **self-copy no-op**. Slot 252 holds **0x0000**, and the glyph
   blit is masked (`IMAPPED_M`) where index 0 is the transparency key. **The text is rendered
   correctly and drawn fully transparent.** Same family as S73's cockpit-black.
4. **Tried and rejected:** writing a real white into slot 252 does *not* make text appear, so the
   glyph texels do not index 252 either. That change was **not shipped** — it alters a shared render
   path for no proven benefit. Remaining suspect: the font image map's texel/alpha data through the
   software blit.

## ⚠ The trap that cost this sprint the most time — and corrected one of my own notes
`SRC/GRAPHICS/POLYGON.CPP` (149 KB) and `SRC/GRAPHICS/Polygon.cpp` (159 KB) are **genuinely
different files with different inodes**, and the `_GRAP` unity compiles the **mixed-case** one
(`#include "../GRAPHICS/Polygon.cpp"`). The first full read, analysis and instrumentation of
`DoPutC` all went into the file that is **never built**. The only symptom was `ninja: no work to do`.

This was self-inflicted: **S83 probed RBUTTON's twins, found them to be the same file, and I
generalised.** That memory note is corrected — the property is **per file**, and the exact case must
come from the unity's own `#include` line (or the compiler's error path) every time.

## Gates — and two failures worth more than the sprint's code

The first full run came back **campaign_map DIFF (4605 px)** and **OOB sweep 9 OPEN → 0**. Two
different causes, and only one was mine:

1. **The system-box draw was a real regression.** The map diff sat at x 8-379, y 5-31 -- the *date
   readout*, top **left**, nowhere near the box. Drawing the box leaves a different GDI font
   selected, and the date text, drawn from the same DC, inherited it: the game's art typeface
   rendered as a plain sans. Since the box also still renders blank, shipping it on by default
   would trade a passing gate for no visible benefit. **Now opt-in behind `MA_SYSBOX=1`**, and
   parity returned to 0 px.
2. **The sweep failure was environmental, not code.** It reproduced at committed HEAD, which
   pointed away from this sprint's diff -- and the cause was the **PO's own play session advancing
   the campaign save**. Every dialog in the sweep is reached by navigating the campaign through the
   Load dialog, so the whole gate depends on campaign state. Restoring the pinned save returned it
   to OPEN immediately.

   **`oob_sweep.sh` now PINS `campaign_pristine.sav` instead of merely preserving whatever is
   there** -- the rule `parity_2d.sh` learned in S81, which this gate had never been given. A gate
   whose result depends on state a human can change reports "regression" when nothing regressed.

Final run: parity **5/5**, sweep **9 OPEN / 0 CRASH**, stress **20/20**, ASan **0 reports**.

## Result
The first human play-test produced more actionable defects in twelve minutes than the last four
autonomous sprints found between them — worth remembering when choosing between another headless
investigation and putting the game in front of someone. Two of the five are root-caused to a named
line; none is yet closed, and the record says so.
