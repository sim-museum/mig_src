# Sprint 121 — "The front end had no keyboard" (PO-16) — ✅ CLOSED 2026-08-15 (goal MET)

**Planned 2026-08-15 (PO directed continuous sprints on the campaign-GUI list). Autonomous. ~8 pts.**
**Sprint Goal:** let the player type a name into the campaign profile.

| Story | Pts | Result |
|---|---|---|
| S121-1 route keystrokes to the focused control | 3 | ✅ focus tracking + SDL text input |
| S121-2 make editing work in a windowless host | 3 | ✅ host-side editing, two compat nulls fixed |
| S121-3 a way to drive typing from a script | 2 | ✅ `MA_TYPESEQ` |

## Evidence

`port/ref/native/career_typed.png` — the Player Log → Career page with **TESTPILOT** in the Name
field, typed through the same path a player's keystrokes take.

## What was wrong

**The front end had no keyboard route at all.** Every hosted OCX control was click-only: the port
hosts RListBox, RStatic, RButton, RCombo, REdit and friends, and all of them were reached through
`ma_ole_click`. `CWnd::SetFocus()` was `{ return NULL; }`, so nothing even recorded *which* control
had the keyboard. `CAREER.CPP` does exactly the right thing —

```c
editbox->SetCaption(MMC.PlayerName);
editbox->SetEnabled(true);
editbox->SetFocus();          // <- a no-op
```

— and the keystrokes had nowhere to go. Selection had always been enough; entry had never been
needed, so the gap was invisible.

## What shipped

- `CWnd::SetFocus()` records the focused hosted control (edits only).
- `ma_ole_char` / `ma_ole_key` deliver to it; the SDL pump feeds them from **`SDL_TEXTINPUT`**
  (already layout- and modifier-aware, so we do not reimplement shift/AltGr over scancodes) plus
  the editing keys that event does not carry. Both are ignored while the sim owns the keyboard
  (`g_diKbAcquired`), so flight controls are untouched.
- `SDL_StartTextInput()` at window creation — without it the event never arrives.
- Focus is cleared when a panel's controls are removed, so it can never point at a freed control.

## Editing lives in the host, and why

The first cut called the game's own `CREditCtrl::OnChar` — it already inserts, handles backspace,
tracks the caret and fires TextChanged. It **segfaulted**, because that method runs in an MFC
message context this port does not provide: it measures through `CWnd::GetDC()`, invalidates, and
drives a caret timer. Each is a separate null in a windowless host, and chasing them one at a time
is unbounded.

So the host supplies the editing behaviour — exactly as it already does elsewhere: `ma_ole_click`
*cycles* a combo rather than invoking `CRComboCtrl::OnLButtonDown`. The text still lives in the
game's control, so its own `OnDraw` renders it and the dialog reads it back normally.

**Two compat nulls found on the way, both kept — they are real defects with wider reach:**

- `CDC::GetTextExtent` reached `strlen(NULL)`: an **empty** `CString` converts to a NULL `LPCSTR`.
  Nothing hit it while the front end was click-only; the first caller was a control measuring its
  own initially-empty text. Measuring nothing is a legitimate question with the answer 0×0.
- `ma_gdi_get_text_extent` dereferenced a DC it does not own (`CWnd::GetDC()` hands back a
  placeholder handle). An unknown DC still has a sensible answer — the current font's metrics.

## `MA_TYPESEQ` — the missing injector

Typing was the one front-end interaction with **no synthetic driver**, which is why PO-16 could
only be reproduced by hand. `MA_TYPESEQ="<pump>,<text>"` types into the focused control, matching
`BOB_CLICKSEQ` / `MA_UISCR_KEY`. A count of **0** means "as soon as an edit has focus" — because
the count is in *pumps*, which run far slower than frames, and a frame-shaped number silently never
fires (booked in S113/PO-13, and it caught me again here on the first attempt).

*A defect you cannot drive from a script cannot have a gate.*

## Gates

parity 5/5 byte-identical · sweep 9 OPEN/0 CRASH · map click · map drag · sysbox · help click ·
stress 12/12.

## Next

PO-17 (campaign dialogs piling up), PO-18 (map zoom tiling), PO-19 (recon zoom keys) — and the
recon view's terrain, which S120 should have fixed, still needs confirming in that view.
