# Sprint 62 — "Design-time properties arrive" (autonomous)

**Goal:** adopt BoB's S126 persisted-property-stream reader so every hosted R* control
boots with its **genuine design-time properties** instead of an empty exchange. That is
the single component blocking the Player Log's title bar (S61's named residual) and, per
BoB's note 17 §3, the FONT/COLOR set behind cross-cutting deviation #1.

**Committed (~8 pts):**
| Story | Pts | Definition |
|---|---|---|
| S62-1 Adopt the property-stream reader | 5 | Raw DLGINIT bag storage keyed by (dialog, control); a real `CPropExchange` (licence prefix → version → extents → stockPropMask → the control's own PX fields in source order); real `PX_Bool/Short/Long/Color/String`; every hosted-control creation path replays its bag through `DoPropExchange`. BoB's three traps applied. `MA_NO_DLGINIT_PROPS=1` reverts. |
| S62-2 Payoff: title bar + parity re-verdicts | 2 | The Player Log's "PLAYER LOG" title bar renders (its art + caption come from `IDJ_TITLE`'s bag); parity set re-captured and every moved verdict re-stated |
| S62-3 Cross-port note 20 + close | 1 | MA note 20 to `bob/doc/` (adoption report + MA-specific traps); lessons doc md5-identical; board/burndown/parity/`RUNNING.md`/rollup; gates |

**Not pulled:** Career content table (the remaining half of I4); RScrlBar hosting; routing
real mouse clicks to `ma_tabs_hit`; #12 debrief capture.

**Planning notes (2026-08-01):**
- Environment: session **UNLOCKED**, no stray `wmig`, build current, tree clean at
  `f40a9ee`.
- **This is an adoption, not a derivation.** BoB landed the reader in their S126 and
  validated the layout against **all 1280 R\*-class RT240 bags in boblang.dll with zero
  parse failures** (note 17 §3; layout written up in shared lessons §8f). MA note 19 asked
  whether it could be lifted directly — reading their source, it can: the
  `CPropExchange` class in `bob/SRC/compat/afxwin.h` is self-contained (~70 lines) and
  their `bob_dlgtemplate.cpp` bag storage is a straight port to `ma_dlgtmpl.cpp`, which
  already parses the same RT_DLGINIT records for captions.
- MA's current state is exactly BoB's pre-S126 one: `CPropExchange` is a stub and every
  `PX_*` in `afxctl.h` is `{ return TRUE; }`.
- **S58/S59 interaction, checked:** MA fixed the uninit-PX class with shape **(a)**
  (ctor-init every persisted member to its PX default). BoB used shape **(b)** (a
  default-writing exchange). Lessons §8f states (b) composes with a real reader with no
  garbage window; **(a) composes too and is strictly safer here** — the ctor default is
  the fallback and the reader simply overwrites it with the genuine persisted value, so
  there is no window in which a member is unwritten. Keep the ctor inits.
- BoB's three traps to apply verbatim:
  1. persisted colours are **COLORREF order (0x00BBGGRR)** — convert exactly once at the
     seam where the framebuffer text draw expects RGB;
  2. persisted `Normal/PressedFileNum` art indices are **authoring-install** file-table
     indices, meaningless at runtime — restore to boot defaults after the replay and
     resolve art by name;
  3. settled-state emulation for statics fully covered by an interactive listbox.
- Acceptance bar unchanged: the dummy==GL byte-identical `cmp`, plus the 5-screen parity
  sweep before commit (it has caught a scoping regression in each of the last two
  sprints).

## Results

*(filled in as stories land)*
