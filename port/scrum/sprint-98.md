# Sprint 98 — "The '?' reaches the help system" (PO-4) — ⚠️ CLOSED PARTIAL 2026-08-09 — routing fixed in four places; no viewer, so the player still sees nothing

**Planned 2026-08-09 (PO pre-approved ceremonies; continuous-sprint directive). Autonomous. ~8 pts.**
**Sprint Goal:** PO-4 — *"dialogs in campaign come up, but clicking on '?' yields no documentation
screen."*

| Story | Pts | Result |
|---|---|---|
| S98-1 route the help click end to end | 5 | ✅ four separate breakages fixed |
| S98-2 symbolic help-glyph recipe + gate | 2 | ✅ `port/help_click.sh` |
| S98-3 scope the help *content* honestly | 1 | ✅ reconnaissance done, epic logged |

## ⚠ Read the result correctly
**The click now reaches the help system. There is still no documentation screen**, because the port
has no WinHelp viewer — `CWinApp::WinHelp` remains a stub. PO-4 is therefore **half closed**, and
the gate says so in its own output rather than reporting a green "help works".

## The click was being dropped in four different places, all in the port
1. **The title-bar hit router returned early** for the help band — literally
   `if (disp == 0) return 1; /* help: nothing to route to yet */`.
2. **`WM_COMMANDHELP` did not exist.** It is MFC's own private message (`afxpriv.h`, `0x0365`) and
   the compat headers had never defined it. The reason is §8-MA83 exactly: while `ON_MESSAGE`
   expanded to nothing it **never evaluated its message argument**, so the symbol was never
   required to exist. Adding the route is what made it required.
3. **`CWnd::SendMessage` only dispatched `WM_USER+`** (`>= 0x400`), and `WM_COMMANDHELP` is
   `0x0365` — *below* it. Named explicitly rather than widening the range, so everything else under
   `WM_USER` keeps returning 0 untouched.
4. **`CWnd::OnCommandHelp` was a non-virtual stub returning 0** — and `CDialog` overrode it back to
   0 as well. MFC routes this message *up* the window chain until something handles it, so
   `CMainFrame::OnCommandHelp` — the thing that actually opens help — was unreachable, and being
   non-virtual it could not have dispatched through a `CWnd*` even if it had been called.

With all four fixed the engine's own chain runs: `RDialog::OnCommandHelp` → parent chain →
`CMainFrame::OnCommandHelp` → `WinHelp(HID_BASE_RESOURCE+IDD_INTRODUCTION)`. Measured: the send
returned **0** after fixes 1–3 (nothing handled it) and **1** after fix 4 (the frame did) — which is
how the last one was found rather than assumed.

## The gate clicks a symbol, not a pixel
New recipe form **`#ID@Class:?`** — "the help glyph of this title bar" — resolved by asking the
control's **own** hit-test where its help band is (scanning right-to-left for the first point it
reports as Help). The glyphs come from the button's art and move with the dialog's width and font.
This is S95's rule applied again, and S96 moved the screen edge twice inside one sprint.

**Trap hit while adding it:** `sscanf`'s return value counts **assignments, not literals**, so
`"%d,#%d@%63[^:;]:?"` returned 3 for entries that had no `:?` at all — the new branch stole
`#2064@CMainToolbar`. The token is now checked explicitly. Same family as the `;`-scanset trap
already recorded in that parser.

## The help content — reconnaissance, not a guess
`port/tools/hlp_probe.py` (new) reads the internal structure of `English/TEXT/MIG.HLP`:
- **11 internal files**, WinHelp 4 (`|PhrImage` + `|PhrIndex` → Hall compression)
- **44 topics**, titled **Map Screen, Main Toolbar, Dossier, Bases, Weather, Squadron Information,
  Flight Details, Target List, Daily Intelligence Summary, Frag, Filter Toolbar…** — documentation
  for precisely the screens the play-tester was pressing "?" on
- **35 `|CTXOMAP` entries** mapping context ids straight to topic offsets — the game passes exactly
  such an id, so the lookup the viewer needs is already there

Remaining: `|TOPIC` is LZ77 + phrase compressed and needs decompressing before it reads as English,
then an in-game viewer. Logged as its own item — it is a real piece of work and half-building it
inside this sprint would have produced something that neither renders help nor can be trusted.

## Gates — all under `gl-lock`
- **parity 5/5** · **sweep 9 OPEN / 0 CRASH** · **map click PASS** · **map drag PASS** ·
  **sysbox exit PASS** · **new help click PASS (routing only)** · **stress 20/20** ·
  **ASan 0 reports**

## Result
Four dead links in one chain, each of which independently made the "?" a no-op, and each invisible
until the one before it was fixed. The button now does what it is supposed to do right up to the
point where the port genuinely lacks a feature — and that boundary is written into the gate's
output so nobody later reads this as "help works".
