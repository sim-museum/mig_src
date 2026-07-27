# Sprint 58 — "Capture the proof" (autonomous; GLX healed mid-arc)

**Goal:** the S57 parity fixes become *proven* — the GL-free MA_SHOT capture path works
artifact-free, the #7/#8 parity verdicts flip on real re-captures, and the regression
gates run again.
**Committed:** ~8 pts — S58-1 MA_SHOT done (3), S58-2 verdict flips + gates (3),
S58-3 cross-port note 16 processing + MA note 16 reply (2).
**Context:** resumes the interrupted S58 (salvage `53554d4`, "strip artifact… traced to
the membership filter" — that diagnosis turned out WRONG, see below). Environment check
at planning: the S57 machine-wide GLX wedge is HEALED (`glxinfo -B` OK).

## S58-1 — MA_SHOT capture path DONE (3 pts) — ✅

The salvaged MA_SHOT path (canvas→PPM at front-end idle N under `SDL_VIDEODRIVER=dummy`,
BoB BOB_SHOT recipe) worked, but its captures showed a black strip + red tabs across the
prefs tab bar and doubled/black-banded title-menu rows.

**Root cause (NOT the membership filter):** `[filter-skip]` trace = zero hits on the
affected screens; `MA_NO_PE_RSRC=1` A/B = strip persists → the whole S57 layer exonerated.
Bisected by canvas-vs-GL diff (single diff band = the tab-bar listbox) + `MA_TRACE_LIST`
(`lockTop=-1` dummy vs `0` GL): **`CRListBoxCtrl` members set only by `DoPropExchange`
were never initialised** — compat `PX_*` are no-ops that don't even write the default,
compat `OnResetState` doesn't drive `DoPropExchange` → ~20 members (`m_bLockTopRow`,
`m_bBlackboard`, `m_bCentred`, colours…) held heap garbage, and the garbage was
**environment-dependent** (SDL-dummy vs GL-window heap layout). Same bug class as the
earlier ASan(MA) font-fields ctor fix — wider net.

**Fix:** `SRC/RLISTBOX/RLISTBXC.CPP` ctor now initialises every persisted member to its
PX default. Results:
- prefs tab-bar strip GONE; title menu now single centred captions (the "doubled
  captions + black rows" — previously mis-filed as a font-path delta — was the same bug);
- **dummy-run canvas byte-identical (`cmp`) to GL-run canvas** at the same idle — adopted
  as the standing MA_SHOT acceptance bar (and offered to BoB as such, note 16).

## S58-2 — I2 verdict flips + gates (3 pts) — ✅

Re-captures (all GL-free MA_SHOT; committed to `port/ref/native/`):
- **#7 prefs Controls → CLOSE** (was PARTIAL): "Input Devices:" gold wording, Calibrate
  (REdtBt), Stick/Throttle/Rudder/Dead Zone/Airframe labels, DI axis names
  ("Axis 0 & Axis 1", real Logitech Extreme 3D enumerated), tickbox art+glyph — every
  S57 fix visible in-capture. Residuals named (glyph renders literal "3" via GDI font;
  chrome; one large-font value row).
- **#8 prefs Others → CLOSE** (was PARTIAL): all 6 missing labels render.
- **#1 title: FIRST capture → CLOSE** (bonus; unblocked by the ctor fix).
- #2–#6, #9, #13 re-captured; verdicts unchanged. #9's stray combo: S57's "filter should
  kill it" expectation DISPROVEN (control IS in the installed template; runtime-hidden on
  Windows by an un-routed mechanism — named deviation, still open).
- Prefs-tab/submenu click coordinates documented in `screen-parity.md`.

Gates (both headless, run post-fix):
- **`port/asan_all.sh` PASS** — 0 AddressSanitizer reports, 4/4 paths reached
  (flight + campaign map/fly/nextday, 2 runs/mode; ASan build rebuilt incl. the ctor
  fix). Gate hardening landed: `timeout -k 5 -s KILL` (an ASan run ignored SIGTERM and
  wedged the suite indefinitely — first observed this sprint).
- **`port/stress_launch.sh` 8/8 OK** — every launch reached & sustained 100 3D frames.

## S58-3 — cross-port (2 pts) — ✅

- **Inbound BoB note 16 processed:** (a) regression caveat (art/String to artless-caption
  buttons only) was already applied in the salvage — kept; (b) S125 bag-layout slices
  (RListBox authored columns / RButton caption alignment) checked against MA: both MA
  candidates (prefs tab bar, campaign phase list) are runtime-populated, not bag-authored
  → **not adopted, no symptom**; logged for future screens. (c) numbering ruling adopted.
- **Outbound MA note 16** (`CROSS-PORT-FROM-MA-2026-07-27.md`, delivered to `bob/doc/`):
  the PX-defaults trap + the byte-identical dummy==GL acceptance bar + #9 filter-hypothesis
  post-mortem. Shared lessons doc: new §8f addendum "PX defaults are load-bearing" —
  **both copies byte-identical** (md5 verified).

## Notes / carry-over

- #9 stray combo runtime-hide mechanism; mission-text word-wrap; Scenario/UN radio row.
- I4 Player Log (8 pts) still not pulled — next sprint candidate.
- The other R* OCX controls may carry the same uninit-PX class — audit if any screen
  renders differently headless vs windowed (the byte-identical bar catches it).
