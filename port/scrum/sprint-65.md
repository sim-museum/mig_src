# Sprint 65 — "Bags and faces" (autonomous)

**Goal:** finish the Player Log title bar — but scoped as an **investigation with a visual
stretch**, per S64's retro. The last three attempts were planned as rendering stories and
the blocker was a layer underneath every time.

**Committed (~8 pts):**
| Story | Pts | Definition |
|---|---|---|
| S65-1 Why does the title bar not draw? | 3 | Root cause documented. Stretch: it renders |
| S65-2 Narrowing criterion for persisted art names | 2 | A defensible rule for when design-bag caption/art may be applied, so `MA_BTN_ART_NAMES` need not stay a blanket flag |
| S65-3 Font FACE | 2 | Identify what typeface gold uses and whether the game ships it; deliverable is a **decision**, not necessarily an implementation |
| S65-4 Cross-port note 22 + close | 1 | Note 22; lessons doc md5-identical; board/burndown/parity/`RUNNING.md`/rollup; gates |

**Planning notes:** environment UNLOCKED, tree clean at `2697a3a`, build current.

## Results

**Sprint outcome: 6 of 8 pts. The title bar renders — and S64's stated blocker turned out
to be a trace artefact, which is the more useful finding.**

### S65-1 The Player Log title bar (3 pts) — ✅ root-caused AND rendering

**First: S64's conclusion was wrong, and the reason matters.** S64 reported that
`ma_px_replay` "never fires for id 1001". It does. The `[px]` trace was capped at a fixed
60 lines and the boot path alone replays 58+ bags, so the Player Log's controls fell off
the end of the trace. **An absence of trace output was read as an absence of behaviour.**
The cap is now `MA_TRACE_PX_MAX`-tunable, and with it raised: `[px] id=1001 type=3 len=178
ver=00010001 ok=1 consumed=175/178` — a clean parse all along.

With that cleared, dumping the bag settled it. IDD 276's record for `IDJ_TITLE` carries
everything needed: **`IDS_PLAYERLOG`**, the literal **`Player Log`**, and
**`FIL_TITLEB_BMP`** (the title-bar art, twice — normal and pressed). Nothing was missing;
**two separate narrowing filters were each withholding half of it**:
- the **caption** was blocked by S58's tickbox-only rule in `ma_ole_set_label`
  (`ma_dlg_artnum` only answers for `FIL_ICON_TICKBOX*` art);
- the **art** was blocked by S64's own `MA_BTN_ART_NAMES` gate.

Fix: treat `IDJ_TITLE` (1001) as what it is — a **reserved engine id**, the same family as
`IDJ_TABCTRL` and `IDJ_PANEL0..9` that S61 already special-cases — and exempt it from both.
Its caption and art are design-time *by definition*, so the rules that protect
runtime-owned captions do not apply.

**Result: the title bar renders** — "Player Log" with the star roundel on the striped
`FIL_TITLEB_BMP` chrome, matching gold's title-bar art. Parity sweep stayed 4/4
byte-identical, so the widening is genuinely contained.

Residual, named: the title bar draws **wider than the dialog** (it spans to x≈883 against a
336px dialog) because `RDialog::UpdateTitle` sizes it from `viewsize.right`; and the
`?`/`✓` buttons are still absent — they are separate controls, not part of this one.

### S65-2 Narrowing criterion (2 pts) — ◐ tested and REJECTED, which is a result
The obvious candidate was **template membership** — apply design-bag properties only to
controls the dialog's own template declares. **Measured and rejected:** the system-box
"Quit"/"Size" buttons that S58 documented (and that S64 accidentally resurrected) are
`inTmpl=1` themselves, so membership does not separate them from legitimate controls.
No general criterion is established, so `MA_BTN_ART_NAMES` stays a blanket opt-in flag and
the only widening shipped is the single reserved id above. Recording the rejected candidate
so it is not re-tried.

### S65-3 Font FACE (2 pts) — ⬜ not started
Ran out of sprint. Untouched, carried whole.

### S65-4 Cross-port note 22 + close (1 pt) — ✅

### Gates
- 2D parity sweep — 4/4 byte-identical (`title`, `prefs_3d`, `quickmission`,
  `campaign_map`); `map_playerlog` re-based deliberately for the new title bar;
  `prefs_controls` excluded as environment-dependent (S64).
- `port/asan_all.sh` — see gate log.
- `port/stress_launch.sh` — see gate log.

### Carry-over to S66
1. **Font FACE** — the correctly-scoped remainder of cross-cutting #1 (S65-3, untouched).
2. Title bar **width** (`UpdateTitle` sizes from `viewsize.right`) and the `?`/`✓` buttons.
3. A general narrowing criterion for design-bag caption/art (template membership rejected).
4. Career content table (other half of I4); RScrlBar hosting; `ma_tabs_hit` click routing;
   #12 debrief capture.
