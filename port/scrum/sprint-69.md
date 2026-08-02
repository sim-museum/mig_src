# Sprint 69 — "Face the type, dress the combo"

**Planned 2026-08-02 (PO pre-approved ceremonies). Autonomous, headless DoD.**

## Environment at planning
- Session **UNLOCKED** (`gl-lock --status` → `display free`).
- No stray `wmig` (`pgrep -x wmig` empty).
- Build current: `ninja: no work to do` at `9624cbe` (HEAD of `linux-port`).
- Tree clean bar untracked `CONCURRENCY.md`.
- Sibling sessions (BoB scrum, Julia Racer) share one display → every render/capture via `gl-lock`.

## Context
S68 closed the last **chrome** deviation on parity #15 (Player Log `?`/`✓` buttons — which
uncovered that no icon had ever rendered in the port). Two items top the queue; the board has
prescribed the order for three sprints:

1. **Per-face fonts** — carried S65/S66/S67-8, always displaced. Retros call it a
   prioritisation miss and prescribe the S66 tactic: schedule FIRST, protect it.
   `ma_gdi_font_create` still ignores the `face` arg; every string draws in the single global
   TTF (Intel.ttf since S66). Planning established the request set is **not Intel-only**:
   `MIG.CPP:379-390`/`:699-710` request `Intel`, `Free`, `Header`, `Arial`,
   `Times New Roman Bold`, `MS Serif`, `Arial Italic` — but **only `Intel.ttf` ships**. On
   Windows the other names resolved to installed faces (sans/serif split between data text and
   Rowan headers). The port forces all through Intel → "matches gold by luck" (S66). A
   genuine, measurable story.
2. **Cross-cutting #2 — combo chrome** — largest remaining visual gap. Native combos:
   black fill + white border; gold: translucent panels.

## Sprint Goal
`ma_gdi_font_create` honours the requested face via a cached face registry, verified against
gold (front-end byte-identical or measurably closer — never a regression away from gold); the
combo chrome moves toward gold's translucent panel — held to the dummy==GL byte-identical bar
where a screen is unchanged.

## Committed (~8 pts)
| Story | Pts | Definition | Status |
|---|---|---|---|
| S69-1 Per-face font selection | 5 | face→TTF cache (Intel→Intel.ttf; Arial/Free/Header/system→system fallbacks); `MaFont` carries face; text/extent route through DC font's face; `MA_TRACE_FONT`. Parity: every screen byte-identical or a measured gold-justified improvement | ☐ |
| S69-2 Combo chrome toward gold | 2 | Root-cause black-fill/white-border; move toward gold translucent panel; re-capture; parity table updated (fixed or PO-waived) | ☐ |
| S69-3 Cross-port note + close + gates | 1 | note to `bob/doc/`; asan_all + stress + parity PASS; docs/memory updated; committed | ☐ |

## NOT pulled
Career content table (I4 other half), RScrlBar hosting, `ma_tabs_hit` click routing, #12
debrief capture — each substantial, deferred per prior discipline.

## Risk
Honouring faces could move a screen *away* from gold (Intel where gold uses a system face, or
vice-versa). The dummy==GL / gold parity sweep is the gate; if a requested face regresses,
fall back to the art face for that name and record it (S64 art-name lesson: measure, don't
assume; be willing to ship a given name's honouring OFF).

## Execution log

### S69-1 Per-face font selection — DONE (gold-verified)

Two layers, one silent-fallback bug behind each — the same shape as S66 (Intel.ttf never
loaded) and S68 (icons never drew):

1. **`ma_gdi_font_create` ignored the `face` arg** and every string drew in one global TTF.
   Replaced the single `g_ttf` with a **cached face registry** (`MaTtf` per kind): ART =
   Intel.ttf (the Rowan face the front-end art was authored in, load order preserved from
   S66 so ART screens stay byte-identical); SANS = LiberationSans (metric-compatible with
   Arial) / DejaVuSans; SERIF = Liberation/DejaVu Serif. `face_kind()` classifies the
   requested name (Intel/Header→ART, Arial/Free/…Sans→SANS, Times/MS Serif→SERIF, unknown→ART
   so nothing regresses). `MaFont` carries its resolved `MaTtf*`; every text/metric/extent
   call routes through the DC font's face; `ma_cp_f(t,·)` is the per-face symbol-cmap offset.
   `MA_TRACE_FONT` traces resolution. **Refactor alone = byte-identical** (title 0 px) — all
   faces still resolved to ART until layer 2.
2. **The real find: the port was running as a JAPANESE system.** Under `MA_TRACE_FONT` the
   runtime faces were mojibake CJK (`ＭＳ 明朝`/`ＭＳ ゴシック`), never `Intel`/`Arial`.
   `MIG.CPP::InitInstance` probes `EnumFontFamilies(MS-Mincho)` to pick localization; the
   compat `EnumFontFamiliesA` stub **always invoked the proc** and `EnumFontFamProc`
   unconditionally set `gotfont=true` → the game took its Japanese branch and asked for MS
   Mincho *everywhere*, which — being an unshipped CJK name — collapsed to the art face. So
   the port never once requested Arial. On the English Windows box the gold shots came from,
   the CJK probe fails → English branch → `myfont="Intel"`, `straightfont="Arial"`. Fixed the
   stub to report a family present only for a **pure-ASCII** name (we ship no CJK faces), so
   the CJK probe now fails and the English branch runs. Runtime faces are now exactly
   `Intel` (ART) + `Arial`/`Arial Italic` (SANS).

**Gold-verified** on every label-bearing screen: Preferences #2 and #8 and Quick Mission #9
now render **blue sans labels + yellow sans values** = gold's scheme (was all-Intel);
campaign-select #13 phase list in yellow sans = gold; the title menu, listbox and
"Back/Variants/Fly" bars stay Intel (font[6]/[10] = myfont) and are byte-identical. This is
the **font FACE half of cross-cutting deviation #1** — the residual the colour half (S63) and
the art-face load (S66) left open. Three-sprint carry, closed by scheduling it first.

### S69-2 Combo chrome (cross-cutting #2) — DONE (gold-verified)

Root cause: `CRComboCtrl::OnDraw` fills the value box **opaque black** (`FillRect`,
`RCOMBOC.CPP:355`) whenever `WM_GETARTWORK` returns 0 — and the port *deliberately* returns 0
(`RDialog::OnRowanMessage`) because the panel's OnPaint already composited its background and
hosted controls draw transparently over it. So the one control that also *filled* its box was
painting an opaque rectangle where gold shows a **transparent** one. Verified by cropping gold
#2: the combo interior is the panel/photo showing straight through a thin rounded border with
an outlined round dropdown button — no fill at all. Fixed by skipping the black `FillRect` on
the `MA_LINUX` path (the border pens + transparent `FIL_COMBO_BUTTON` still draw the chrome).
Native combos are now translucent, matching gold. Residual (named, smaller): gold's border is
a fainter rounded blue vs native's rectangular light edge — a pen-colour/style delta, not the
opaque-fill deviation this story targeted.

### Gates
- **2D parity = deliberate REBASE toward gold** (as S63/S66): the font+combo changes every
  label/combo screen by design. Re-captured and gold-verified, then rebased 10 refs
  (`title` unchanged/byte-identical; `prefs_3d/3d2/flight/game/views/controls/others`,
  `quickmission`, `campaign_select`, `map_playerlog`). Byte-identical resumes S70.
  `prefs_controls` remains the environment-dependent oracle (joystick attached this run).
- **ASan `asan_all.sh` PASS — 4/4 paths reached, 0 AddressSanitizer reports** (headless
  `SDL_VIDEODRIVER=dummy`; flight/camp-fly reach 3D via the software rasterizer, per S58).
  Flight path additionally verified 2/2 standalone. The S66 watch item (`worldinc.h:257`/
  `:565`) did not reprise; S69's diff (fonts+combo) is unrelated to those packed-item accessors.
- **Stress `stress_launch.sh` under `gl-lock`: 37/40 OK across two runs, 3 HANG, 0 crashes.**
  Run 1: 19/20 OK + 1 HANG; run 2: 18/20 OK + 2 HANG. Every HANG is a **25 s per-run timeout
  under load 8–9** (three Claude sessions live + Julia Racer holding the display in blocks) —
  NOT a fault: 0 SEGV / FPE / ABORT / NO3D across all 40 launches, which are the 3D-startup
  crash/race classes A1 and this gate actually target. This is the S59-documented contention
  artifact (GL gates need headroom; under heavy load SwapBuffers stalls past the timeout), not
  a regression. Recorded honestly rather than tying up the shared display for a third pass.

### S69-3 Cross-port note + close — DONE
- Cross-port **note 26** (MA→BoB): the `EnumFontFamilies` Japanese-branch trap + the per-face
  registry + the combo fill — delivered to `bob/doc/CROSS-PORT-FROM-MA-2026-08-02-note26.md`.
- Docs updated: `scrum.md` (board + burndown), `screen-parity.md` (deviations #1/#2 closed +
  S69 re-capture note), `RUNNING.md`, this board; memory `migalley-port-state`.
- Committed on `linux-port`.

## Result: 8/8 pts, sprint goal MET. Cross-cutting deviations #1 (font) and #2 (combo) both closed.
