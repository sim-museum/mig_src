# Sprint 159 — "The dossier is the size it says it is" (PO-49) — ✅ CLOSED 2026-08-21 (goal MET, 8/8)

**Planned 2026-08-21** (PO ceremonies pre-approved), straight out of S158's K1 measurement.
**Sprint Goal:** a target dossier's backdrop art stops at the dialog edge, so the map stays readable
while the player is choosing a target — step 4 of the Wonju walkthrough.

| Story | Pts | Result |
|---|---|---|
| S159-1 find where the backdrop size is decided | 3 | ⭐ `RDialog::OnPaint` hands `SetDIBitsToDevice` the **bitmap's** width/height |
| S159-2 clip it without repeating S155's reverted node clip | 3 | ✅ clip the ART BLIT to the dialog's own rect, viewport-origin aware |
| S159-3 prove it, and prove nothing else moved | 2 | ✅ 9/9 OOB dialogs reclaim map area; parity 5/5 byte-identical |

## The finding

`RDialog::OnPaint` (`SRC/MFC/RDIALOG.CPP:1296`) ends its art path with:

```c
long offsets=OnGetXYOffset();
SetDIBitsToDevice(pDC->m_hDC, x, y, pInfo->bmiHeader.biWidth, pInfo->bmiHeader.biHeight, …);
```

The size passed is **the bitmap's**, never the dialog's. On Windows that is harmless — painting is
clipped to the window. This port has no window, so the art paints wherever it reaches:

```
[artclip] node=… art=26641 540x602 -> dialog 327x316 at (679,0)     the target dossier
[artclip] node=… art=26625 496x632 -> dialog 397x417 at (0,0)
[artclip] node=… art=26629 440x635 -> dialog 438x373 at (568,391)
[artclip] node=… art=26647 513x378 -> dialog 393x209 at (0,555)
      … eleven such nodes across the nine campaign-map dialogs
```

The dossier's `FIL_MAP_SUPPLY` backdrop is **540×602 for a 327×316 dialog** — a 286 px skirt below
its own Center/Zoom/Photo/Authorize row, over the map the player is using to pick a target.

This is **PO-47's shape one screen further on**: *"the dialog is not oversized, the ART is"*. S156
fixed that case by clipping around `RMdlDlg::DoModal`'s call; the fix now lives one level down, in
the paint itself, so every caller gets it.

## Why this is not S155's reverted clip

S155 clipped **the OOB node's rect around the CONTROLS** for PO-43 and had to revert: the node rect
is smaller than the content its controls draw, so the clip ate the tab row and the combo border.

A **backdrop** is different in kind. It has no content of its own to overflow for; art larger than
its dialog is always wrong. So the clip goes around the DIB blit only, and the controls draw exactly
as before — which the 5/5 byte-identical parity run confirms.

Three details that would each have made it subtly wrong:

1. **It clips to `m_maW/m_maH`, and only when those are set.** They are 0 before a node is placed,
   and clipping to nothing would erase art that is currently correct.
2. **It clips only when the art is actually bigger.** A no-op clip on every panel in the game is a
   regression waiting for a rounding error.
3. **The clip is in absolute canvas coordinates** — `ma_gdi.cpp`'s `putpx` adds the viewport origin
   *before* testing the clip — and this `OnPaint` can be reached with an origin already set
   (`RMdlDlg::DoModal` sets one). The origin is read back and added rather than assumed to be 0.
   Assuming it was 0 is exactly the S105 centre-origin trap.

`MA_NO_ART_CLIP=1` reverts it for an A/B; `MA_TRACE_OOB=1` prints one `[artclip]` line per clipped
node.

## What it actually bought

A/B over the whole OOB sweep — same run, clip on vs `MA_NO_ART_CLIP=1` — counting pixels the map
gets back:

| dialog | px reclaimed | | dialog | px reclaimed |
|---|---|---|---|---|
| bases | **172,230** | | overview | 43,623 |
| intelligence | **113,635** | | weather | 39,198 |
| dis | 35,080 | | directives | 31,132 |
| playerlog | 30,387 | | missionfolder | 27,132 |
| squads | 1,489 | | | |

**Nine of nine.** The defect was reported against one dialog and was in all of them — which is what
"fix it where the size is decided" is worth.

⚠ **PO-43 is NOT fixed by this** and the capture says so plainly: the Intelligence dialog's supply
table still runs past the bottom of its dialog over the map. That list is a `CRListBoxCtrl` grown by
`ResizeToFit`, not art — S155 named it correctly and it stays open.

## Gates

| Gate | Result |
|---|---|
| `port/parity_2d.sh` | **5/5 byte-identical** |
| `port/oob_sweep.sh` | OPEN=9 NONE=0 CRASH=0 |
| `port/sysbox_exit.sh` | quit confirmation still 279×142, "Yes" clicked, 99.0 % of the map changed |
| `port/map_icon_click.sh` | PASS — dossier painted |
| `port/map_filter.sh` | PASS — 64,119 map px change on the filter |
| `port/dialog_scroll.sh` | PASS — list scrolls |
| `port/map_drag.sh` | PASS — one-way 901,804 px, round trip 0 px, release not a click |
| `port/help_click.sh` | PASS |
| `port/panel_click.sh` | PASS — real GL window @1920×1080 |
| `port/asan_campaign.sh` | **MAP-OK 2/2, 0 AddressSanitizer reports** — after the gate itself was repaired, below |

**Harness note worth keeping:** `panel_click.sh` takes `gl-lock` **itself**. Running it inside an
outer `gl-lock` deadlocks until the timeout, and it presents as an **empty log and a FAIL that looks
like a regression** — the same trap `map_filter.sh` documents in its header. It passed immediately
when run unnested. A gate that fails with a zero-byte log is reporting on the harness, not the code.

## The gate that was reporting on itself

`asan_campaign.sh` came back **"NO-MAP / INCONCLUSIVE: map never rendered (load/nav regression?)"**
— which reads exactly like this sprint's change breaking the campaign. It was not:

1. An **A/B with `MA_NO_ART_CLIP=1` failed identically**, so the clip was not the cause.
2. The gate navigated by **hardcoded pixels** (`30,588,263;65,40,108;100,68,565`) while every sibling
   gate uses the symbolic form S62/S63 mandated — `f,rN` (menu row), `f,#ID` (control by dialog id),
   resolved from the controls' own metrics. Re-running the same gate, same binary, with
   `BOB_CLICKSEQ="30,r3;65,#1055;100,#2063:1"`: **MAP-OK, ASan-clean.**

Switched to the symbolic recipe; default timeout 80 s → 120 s (the ASan build did not reach the map
in 80 s on this box). At defaults it now reports **MAP-OK 2/2, 0 ASan reports**.

**Not claimed: why the pixels stopped working.** `port/hw_gate.sh` still passes with the *same three
pixels*, so whatever moved is conditional — a resolution left in `settings.mig` by another gate is
the leading suspect since S103 made preferences actually load, but that was not measured and is not
recorded as fact. The three remaining pixel recipes (`ab.sh`, `asan_flight.sh`, `hw_gate.sh`) are
logged for the same treatment rather than changed on a hunch.

**The lesson is the one this project keeps re-learning:** an INCONCLUSIVE gate is a claim about the
harness that is very easy to write down as a claim about the code. The A/B took two minutes and it
is what kept "the campaign map regressed" out of this record.
