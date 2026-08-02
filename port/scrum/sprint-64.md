# Sprint 64 — "Face value" (autonomous)

**Goal:** close out cross-cutting deviation #1 by dealing with the font residual S63 left,
and finally land the Player Log title bar now that the property reader is on.

**Committed (~8 pts):**
| Story | Pts | Definition |
|---|---|---|
| S64-1 Font size — measure the FontNum→point-size mapping | 3 | Determine whether the reported "native renders larger than gold" is a mapping bug (cheap) or genuinely needs the art typefaces (big). **Measure before assuming.** |
| S64-2 Player Log title bar + `?`/`✓` | 3 | Now testable: the reader replays `IDJ_TITLE`'s bag, so its art/caption should be reachable. The original S62-2 target, deferred twice |
| S64-3 `prefs_controls` stable oracle | 1 | It embeds live joystick state (S62); capture against a synthetic device or mark the row environment-dependent |
| S64-4 Cross-port note 21 + close | 1 | Note 21 to `bob/doc/`; lessons doc md5-identical; board/burndown/parity/`RUNNING.md`/rollup; gates — including the **byte-identical parity sweep resuming** against S63's new baselines |

**Not pulled:** Career content table (other half of I4); RScrlBar hosting; `ma_tabs_hit`
click routing; #12 debrief capture.

**Planning notes (2026-08-01):**
- Environment: session UNLOCKED, build current, tree clean at `9c08ee0`.
- S64-1 is deliberately framed as a **measurement** first. S63 asserted the size residual
  without measuring it, and the parity doc's own header warns that gold and native are
  captured at different resolutions — so the first question is whether the defect is real
  at all, not how to fix it.
- The byte-identical parity gate **resumes this sprint** against S63's re-based
  references. S63 was a deliberate rebase (the reader changes fonts/colours by design);
  from here a diff is a regression again.

## Results

**Sprint outcome: 6 of 8 pts. S64-1's answer was "there is no bug" — a correction to S63 —
and S64-2 fixed two real defects but did NOT get the title bar rendering.**

### S64-1 Font size (3 pts) — ✅ measured; the reported defect does not exist
S63 recorded a residual: "native renders LARGER than gold". **Measured, it does not.**
Gold's label glyph band is **10 px** and native's **11 px**; gold's row pitch **52 px**,
native's **51 px** — the same absolute font size. S63 compared a **1280×1003** gold shot
with an **800×600** native capture and read the resulting 1.64× density difference as a
font-size defect. This parity doc's own header already warned about precisely that
("layout is resolution-relative … verdicts judge layout/art/content, not pixel
dimensions"); the warning was not applied.

Corrected in `screen-parity.md`, and the caveat is now explicit rather than a line in a
header: **the gold set was captured at ~1280×1024 and native front-end captures are
800×600 because the game selects its panel ART SET by resolution** — so capturing at gold's
resolution is a different art path, not a flag. Until that exists, **no verdict may rest on
relative size, spacing or density**. Cross-cutting #1's residual is re-scoped to **font
FACE only**.

Worth stating plainly: this story's value was deleting a wrong entry from the backlog. S65
would otherwise have spent a sprint hunting a font-scaling bug that isn't there.

### S64-2 Player Log title bar + `?`/`✓` (3 pts) — ◐ 1 of 3; two real bugs fixed, target NOT met
Chasing the title bar's art found two genuine defects:

1. **`GetFileNum(name)` was a stub returning 0.** It is the filename→FileNum resolver the
   R* controls' string-file setters (`SetNormalFileNumString`) depend on — i.e. the
   "resolve art by NAME" half of BoB's trap 2, the *sound* half, since the persisted
   numeric FileNums are authoring-install indices that `ma_px_replay` deliberately discards.
   With it stubbed, **every control whose art is named rather than numbered silently lost
   its artwork.** Now resolves against the `F_GRAFIX.G` `FIL_*` table that `ma_dlgtmpl.cpp`
   already parses (`ma_fil_lookup`). Verified live: `FIL_ICON_BASES`,
   `FIL_ICON_B_AIRFIELD_ON`, … all resolve.
2. **`CString(LPCWSTR)` was declared but never defined.** Reading any OCX getter's BSTR
   back (`CString s = c->GetNormalFileNumString();`) failed at **link** time only, so the
   gap stayed invisible until something actually did it. Defined to match this port's BSTR
   convention — `AllocSysString` returns a malloc'd **narrow** string — and that convention
   is now documented at the definition instead of being folklore.

**The art-name application is implemented but deliberately NOT enabled** (`MA_BTN_ART_NAMES=1`
to experiment). Applying it to every button **regressed the parity sweep**: the invisible
system-box buttons ("Quit"/"Size") materialised in the top-left 72×52 of every front-end
screen — *exactly* the failure S58 documented when it narrowed the design-bag **caption**
application to tickbox-class buttons. The art path needs the same class narrowing and the
criterion is not yet established, so shipping it off is the honest call. Caught by the
sweep, not by eye.

**The title bar still does not render, and the reason is now precisely located:**
`ma_px_replay` **never fires for id 1001** — no `[px]` and no `[btnartname]` line for it —
so IDD 276's bag for `IDJ_TITLE` is not reaching `bagmap`, even though the DLGINIT stream
for idd=276 demonstrably begins with `e9 03` (= 1001). That is a bag-storage/keying
question in the RT_DLGINIT walk, not an art question. S65 starts there.

This target has now resisted S60, S62 and S64. Each attempt removed a real obstacle, but it
is worth naming the pattern: it keeps being scoped as a rendering story when the blocker
has each time been a layer underneath.

### S64-3 `prefs_controls` oracle (1 pt) — ✅ documented
The reference **embeds live joystick state** — S62 saw "NOT CONNECTED / 0 axes" with no
stick attached; S64's runs show `[joy] opened 'Logitech Extreme 3D' axes=4 buttons=12` again.
A mismatch there is an environment difference, not a regression. Recorded in the parity
doc and **excluded from the byte-identical sweep**; the other five screens carry the gate.

### S64-4 Cross-port note 21 + close (1 pt) — ✅

### Gates
- **2D parity sweep — 5/5 byte-identical.** The byte-identical check **resumes this sprint**
  against S63's re-based references, as promised. It is also what caught and then confirmed
  the removal of the system-box regression above.
- `port/asan_all.sh` — PASS 4/4 modes, 0 reports.
- `port/stress_launch.sh` — PASS 20/20.

### Carry-over to S65
1. **Why does `ma_px_replay` never fire for IDD 276 id 1001?** Bag storage/keying in the
   RT_DLGINIT walk. Blocks the Player Log title bar and `?`/`✓`.
2. **Narrowing criterion for persisted art names** so `MA_BTN_ART_NAMES` can ship on
   without resurrecting the system-box buttons.
3. Font **FACE** (the correctly-scoped remainder of cross-cutting #1) — and, separately,
   whether a 1280-res capture path is worth building so gold comparisons stop being
   resolution-mismatched.
4. Career content table (other half of I4); RScrlBar hosting; `ma_tabs_hit` click routing;
   #12 debrief capture.
