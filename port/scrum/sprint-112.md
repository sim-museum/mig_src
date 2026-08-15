# Sprint 112 — "Show what we can read" (PO-10) — ✅ CLOSED 2026-08-15 (goal MET) — the "?" opens a documentation window

**Planned 2026-08-15 (PO pre-approved ceremonies; continuous-sprint directive). Autonomous. ~8 pts.**
**Sprint Goal:** make the "?" do something real — the last of the PO's six play-test defects still open.

| Story | Pts | Result |
|---|---|---|
| S112-1 one more honest attempt at the topic-text decoder | 3 | ⬜ **failed, and says so** — 800 candidates, best 2/39 |
| S112-2 show the documentation the file DOES yield | 4 | ✅ the panel lists the game's own 30+ topics |
| S112-3 make the gate assert what the player sees | 1 | ✅ `help_click.sh` checks the capture, not just the log |

## The decoder: a negative result worth having

S99 left the fifth decode stage — the Hall opcode table for the phrase-compressed topic text —
unsolved, with the right oracle beside it: *a correctly decoded topic contains its own |TTLBTREE
title* (0/39 with the shipped expander, 1/39 with S99's guess). That oracle is what makes a **search**
legitimate rather than a fishing trip, so this sprint enumerated **800 candidate opcode layouts**
(escape range × 1-or-2-byte index × shift × byte order × offset × space-bit) and scored every one.

**Best: 2/39.** The layout is not in that family. That is a real finding — it rules out the obvious
space rather than merely failing to find the answer — and PO-10's text half stays open, honestly.

## What shipped: the topic index, and a panel that says what it is

`|TTLBTREE` is verified data: 44 entries naming exactly the screens the play-tester was pressing "?"
on — *Map Screen, Title Toolbar, Main Toolbar, Frag, Utility Toolbar, Debrief Toolbar, Scale
Toolbar, Filter Toolbar, Bases, Dossier, Squadron Information, Flight Details, Weather, Daily
Intelligence Summary, Target List, CAS Requests, Directives, Mission Folder, Mission Results, Player
Log, Thumbnail Map, Filing, Load Profile, Missions, Insert a Wave, Tasks, Pay_Load, Aircraft Select,
Routes, Debrief…*

New `SRC/compat/ma_help.cpp` parses that index out of `English/TEXT/MIG.HLP` at runtime (WinHelp
container directory → `|TTLBTREE` root page). `CWinApp::WinHelp` — the destination S98 spent a
sprint reaching, and which was still an empty stub — now raises the panel, and the campaign-map idle
draws it above everything else. A click anywhere dismisses it.

The panel's footer reads: *"Topic text is not readable yet: MIG.HLP's phrase compression is
undecoded (PO-10)."* **A "?" that shows the index is honest; a "?" that shows plausible nonsense is
worse than one that does nothing** (S99's rule, and the reason the decoder search's 2/39 result did
not get shipped as "close enough").

Reference capture: `port/ref/native/help_panel.png`.

## The gate now asserts what the player sees

`port/help_click.sh` used to end with *"this proves routing only"* and PASS — and that was exactly
the half-truth of PO-10: routing had been correct since S98 while the player saw nothing. It now
also checks the **capture** for the panel's title strip and reports
`RESULT: PASS (click -> documentation panel)`.

## Gates

parity 5/5 · sweep 9 OPEN/0 CRASH · map click · map drag · sysbox exit · **help click (panel on
screen)** · overlay text 3/3 · stress 20/20 · ASan 0.

## Result

All six of the PO's play-test defects now have a visible answer in the game. PO-10's remaining half —
readable topic TEXT — is a decoder problem with a named unsolved piece, a working oracle, and one
family already eliminated.
