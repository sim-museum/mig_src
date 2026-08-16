# Sprint 134 — "Every dialog showed the same page" (PO-26) — ✅ CLOSED 2026-08-15 (goal MET)

**Planned 2026-08-15 (PO: continuous sprints, campaign dialogs first, gold videos as reference).**
**Sprint Goal:** the "?" on a campaign dialog opens THAT dialog's documentation, legibly.

| Story | Pts | Result |
|---|---|---|
| S134-1 why is the topic always the Introduction? | 3 | ✅ the dialog never carried a help context |
| S134-2 wrap the body by measuring, not guessing | 3 | ✅ real font metrics; column capped |
| S134-3 verify against a known topic | 2 | ✅ Player Log resolves to topic 30 |

## What was wrong

The PO: *"clicking ? … poorly formatted text, also not the right text"*, on the DIS dialog and
again on the mission-result dialog. Two independent faults behind one symptom.

**Wrong text.** Real MFC's `CDialog` constructor records the template id as the dialog's help
context (`m_nIDHelp = nIDTemplate`), and `OnCommandHelp` uses it. The port's compat ctor was
`CDialog(UINT, CWnd* = NULL) {}` — the id was accepted and dropped. So every dialog arrived at
help with **no identity**, the chain fell through to `CMainFrame::OnCommandHelp`, and that method
— in the game's own source — hardcodes:

```c
AfxGetApp()->WinHelp(HID_BASE_RESOURCE+IDD_INTRODUCTION);
```

That is the shipped fallback for a dialog with no context of its own, and in this port *every*
dialog was that dialog. One assignment in the ctor plus a real `CDialog::OnCommandHelp` gives the
whole campaign UI its own topics, because `RDialog` already passes its IID up
(`CDialog(IID,pParent)`) and the data is already there: the extracted help carries 186 context ids
and 43 topics, including `HIDD_DIS` → *Daily Intelligence Summary*, `HIDD_RESULTS` → *Mission
Results*, `HIDD_THEMAP`, `HIDD_WEATHER`, `HIDD_BASES_TITLE`.

**Poor formatting.** The panel wrapped by assuming *"~7px per char"* and a 13px line height. That
is true of the built-in 8×8 face and of nothing else; with the real TTF selected the lines
overlapped and overran the panel. It now asks the font: line height from `ma_gdi_get_text_extent`,
and the break point found by binary-searching the measured width. Blank source lines became
paragraph gaps, and the column is capped at 1040px — at 1920 the body had been running the full
screen width, which is wrapped in the technical sense only.

## Evidence

`port/ref/native/help_playerlog.png` — the Player Log's own topic, correctly titled, no overlap.
Log: `[help] context 0x20114 -> HIDD_PLAYERLOG -> topic 30`.

## The gold video is a better spec than the bug list

Pulled `g12.png` from `260814_mig_complete_campaign.mp4` (t=12s): the campaign map with the Player
Log open. It shows, at a glance, several things no report had named precisely — the **distance
ruler down the left edge** (PO-22, next sprint), the toolbar strip layout, and the map's default
zoom. Worth returning to before guessing at any campaign-screen layout.

## Gates

parity 5/5 byte-identical · sweep 9 OPEN/0 CRASH · help click PASS (now reporting the resolved
topic, not just "handled").
