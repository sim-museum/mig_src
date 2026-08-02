# Sprint 63 — "Switch the properties on" (autonomous)

**Goal:** finish S62. The persisted-property reader is built, parses all 58 boot-path bags
clean and has a gold-verified payoff — it just ships OFF because of two blockers. Clear
both, switch it on by default, and re-verdict the parity set.

**Committed (~8 pts):**
| Story | Pts | Definition |
|---|---|---|
| S63-1 Root-cause the uninit garbage | 3 | The varying garbage text at the title screen's top-left under `MA_DLGINIT_PROPS=1` is root-caused and fixed; title capture clean at 6× contrast |
| S63-2 Font-independent click recipes | 3 | The parity + `asan_all.sh` drive recipes stop depending on fixed pixel rows: a click-by-menu-row-index helper resolves the row at runtime, so a font/pitch change cannot invalidate the gate again |
| S63-3 Reader ON by default + re-verdict | 1 | Reader default-on (`MA_NO_DLGINIT_PROPS=1` escape retained); full parity set re-captured; every moved verdict re-stated in `screen-parity.md` |
| S63-4 Cross-port note 20 + close | 1 | MA note 20 to `bob/doc/` — **owed from S62**, whose close story did not land — carrying the adoption report, MA's two divergences, and S63's findings; lessons doc md5-identical; board/burndown/parity/`RUNNING.md`/rollup; gates |

**Not pulled:** Player Log title bar + `?`/`✓` (the original S62-2 target — it becomes
reachable once the reader is on, but it is a verification job of its own and S62 already
showed it is not a one-liner); Career content table; RScrlBar hosting; `ma_tabs_hit`
click routing; #12 debrief capture.

**Planning notes (2026-08-01):**
- **Environment:** session UNLOCKED, no stray `wmig`, build current.
- **Tree state at planning needed sorting out first.** The working tree held an
  **uncommitted full revert of Sprint 62** (−475 lines, plus deletion of
  `port/scrum/sprint-62.md` and the S62 board entry) that no session in this
  conversation's history made. Verified it was byte-identical to the pre-S62 commit
  `f40a9ee` — i.e. it contained *zero* unique information and was fully reproducible from
  history — so restoring HEAD destroyed nothing. Preserved it in a stash regardless
  (`S62 revert found in working tree at S63 start`) rather than discarding it outright,
  then confirmed HEAD builds and its **default path is still byte-identical** to the
  `title` reference before building on it.
- **Blocker 1 (uninit garbage)** already has its search narrowed by S62: run-to-run
  variance says uninit read; the stock caption and `PX_String` are exonerated. The S61
  precedent — a local passed as an out-param to a **stubbed Win32 API** and then read
  without checking the return — is the first pattern to test, per
  `[[rowan-port-uninit-and-stub-traps]]`. Repro is ~2s: `MA_DLGINIT_PROPS=1` + the plain
  title screen.
- **Blocker 2 is the more valuable fix and should be done as the durable one.** The
  recipes break because they encode *pixel rows* for menu items. Resolving the row at
  runtime removes a whole class of future breakage — every subsequent font, DPI or
  layout change would otherwise re-break the gate. Doing it half-way (re-deriving the
  constants for the new pitch) buys one sprint and re-breaks on the next change.
- **Order matters:** S63-2 before S63-3, because the gate must be trustworthy *before*
  the default flips. That was exactly S62's reasoning for shipping opt-in.

## Results

**Sprint outcome: 8 of 8 pts — goal MET. The persisted-property reader is ON by default,
both S62 blockers are cleared, and the colour half of cross-cutting deviation #1 is
solved.**

### S63-1 Root-cause the uninit garbage (3 pts) — ✅
Found by trapping the draw rather than reasoning about the reader: a gated hook in
`ma_gdi_text_out` fires on any non-ASCII text draw and (opt-in) aborts, so gdb gave the
caller directly — `CRButtonCtrl::OnDraw` rendering a `CString` whose bytes were
`63 c0 ff ff 6c 5a 3a 08 01`, i.e. containing the code address `0x083a5a6c`.

> **Root cause: `WM_GETSTRING` is an IN/OUT convention and three call sites ignore the OUT
> half.** `CRButtonCtrl::GetParentWndInfo` (×2 — caption and hint) and
> `CRStaticCtrl::GetParentWndInfo` (×1) do:
> ```c
> char workspace[100];
> workspace[0]=99;                       // IN: buffer capacity
> int strsize = parent->SendMessage(WM_GETSTRING, m_ResourceNumber, (int)workspace);
> m_string = workspace;                  // strsize NEVER checked
> ```
> On Windows every parent in a dialog tree handles `WM_GETSTRING`, so the buffer is always
> written. Here a parent whose message map does not route it makes `SendMessage` return 0
> having touched nothing — leaving literal `99` (`'c'` = **0x63, exactly the first garbage
> byte**) followed by uninitialised stack, adopted verbatim as the control's caption.

It stayed latent for the entire life of the port because `m_ResourceNumber` was always the
ctor default 0, so the `else m_string=""` branch ran. Giving it genuine design-time values
is what first exercised the path. Fixed at all three sites: zero the buffer so any
residual read is deterministic, and adopt it only when the handler reported a length.
**Verified: zero non-ASCII text draws, and two consecutive runs now byte-identical**
(they previously differed in a 47×12 box at the title's top-left).

This is `[[rowan-port-uninit-and-stub-traps]]` scoring again, and it extends the pattern:
S61 was *a local passed to a stubbed API*; this is *a local passed to a **message handler
that may not exist***. Same shape, different Win32 mechanism.

### S63-2 Font-independent click recipes (3 pts) — ✅
Two resolvers, both delegating to the control's **own** metrics so they track any font,
DPI or layout change automatically:
- `ma_ole_menu_row_point(row)` — row band from `GetListHeight()/GetCount()`. Note
  `GetRowFromY` is **not** usable as the oracle despite being the obvious choice: it ends
  `if (row > m_playerList.GetCount()) row = -1`, and the front-end menu leaves
  `m_playerList` empty, so it answers −1 for every row past the first. Found by measuring,
  not by reading.
- `ma_ole_control_point(id, col)` — a hosted control by dialog id; `col` indexes a
  horizontal listbox's items through its own `GetColFromX`. Needed because the Load Game
  dialog's "Back Load" bar is **one listbox (id 2063), not two buttons** — the old fixed
  point `(68,565)` landed on "Back" once the font grew, which is why the campaign recipe
  still failed after the menu rows were fixed.

`BOB_CLICKSEQ` gained `f,rN` and `f,#ID[:COL]`; absolute `f,x,y` still works, so migration
was incremental. **Validation that matters: with the reader OFF the row form reproduces
the old hand-derived constants** (row1 → y=233 vs the hardcoded 231; row0 → y=217 vs 217),
and with it ON `rowH` moves 16→28 by itself. `asan_all.sh` and `stress_launch.sh` migrated.

### S63-3 Reader ON by default + re-verdict (1 pt) — ✅
Default flipped; `MA_NO_DLGINIT_PROPS=1` is the escape hatch. Re-captured the whole parity
set through the new recipes and re-verdicted against the gold mirror.

**Cross-cutting deviation #1 — the colour half is solved** (verified against the original
gold PNGs, per BoB trap 1's methodology warning about composites):
- setting **VALUES are yellow, matching gold exactly**;
- the Preferences **tab bar is yellow** where it was white;
- **labels moved from white serif into gold's blue family** — gold's own
  `(103,132,198)` now appears in the native capture;
- the title menu is yellow with a drop shadow and its black backing box is gone.

**Renamed, narrower residual: font FACE and SIZE.** Gold uses the art typefaces (small-caps
tab bar, compact labels); native still uses the DejaVu fallback and the persisted FontNum
renders it **larger than gold**, which loosens row density on every settings screen, and
native labels read brighter cyan `(100,224,255)` than gold's `(103,132,198)`. Named
honestly rather than folded into "solved".

**Oracle note:** the `BEA6-BBCE` gold USB was **not mounted** this sprint. All 14 gold
shots are mirrored at `/home/admin/gold standard/ma/` and that mirror was used — recorded
in the parity doc so the provenance of these verdicts is not ambiguous later.

### Gates
- **2D parity sweep — all 6 screens re-captured and re-based deliberately.** Unlike the
  last three sprints this is NOT a byte-identical result and must not be read as one: the
  reader changes fonts and colours by design, so every reference was regenerated. The
  byte-identical check resumes from S64 against these new baselines.
- `port/asan_all.sh` — see gate log (run on the default = reader-ON path, through the
  migrated recipes).
- `port/stress_launch.sh` — see gate log.

### Carry-over to S64
1. **Font face + size** — the remaining half of cross-cutting #1. Needs the game's art
   typefaces rather than the DejaVu fallback; the size mismatch may be a FontNum→point-size
   mapping question rather than a face question, and that is worth measuring first.
2. Player Log title bar + `?`/`✓` — the reader is now on, so `IDJ_TITLE`'s bag is being
   replayed; S62's original target is finally testable.
3. Career content table (the remaining half of I4); RScrlBar hosting; `ma_tabs_hit` click
   routing; #12 debrief capture.
4. `prefs_controls` embeds live joystick state and is not a stable oracle (S62 finding) —
   either capture it with a synthetic device or mark the row environment-dependent.
