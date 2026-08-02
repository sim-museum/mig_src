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

**Sprint outcome: 5 of 8 pts. The reader is built, correct and measured — but it ships
OPT-IN (`MA_DLGINIT_PROPS=1`), not on by default, so the sprint goal is only half met.**
The Player Log title bar was NOT verified. Reasoning for landing it switched off is
below; it is a deliberate call, not an oversight.

### S62-1 Adopt the property-stream reader (5 pts) — ✅ built and correct, ◐ not enabled
- **Adopted, not re-derived.** BoB's `CPropExchange` lifted from `bob/SRC/compat/afxwin.h`
  and their bag storage ported onto MA's existing RT_DLGINIT walk. Split exactly as MFC
  does so a control's own PX_* land on its own field region: `ExchangeVersion` consumes
  the version DWORD, `COleControl::DoPropExchange` consumes ExchangeExtent +
  ExchangeStockProps, then the control's fields in source order. Real
  `PX_Bool/Short/Long/Color/String` replace the `{ return TRUE; }` stubs (plus the
  `short&` PX_Bool overload some R* controls need).
- **It works.** On the boot path **all 58 bags parse clean** — `ok=1` on every one,
  persisted version read (`ver=00010001`), and ≤8 bytes left over, which is exactly the
  editor slop BoB documents. Zero parse failures, matching their 1280-bag validation.
- **Two MA-specific divergences from BoB's reader, both measured rather than assumed:**
  - **Stock Caption is consumed but NOT applied.** MA's persisted captions are `IDS_*`
    SYMBOL NAMES (`"IDS_MIGALLEY"`, `"IDS_NONE"`), which S57 already resolves the way the
    control's own WM_GETSTRING does — IDS_ name → RESOURCE.H → the BDG string table, i.e.
    the *shipped* wording. Applying the raw value would overwrite a correct caption with a
    symbol name. Found by tracing the captions, not by reasoning.
  - **Stock BackColor is consumed but NOT applied.** MA's hosts composite over the panel
    artwork and treat control backgrounds as transparent; honouring a persisted opaque
    backcolour would paint boxes gold does not have.
- BoB's trap 2 applied (button art indices snapshotted and restored around the replay —
  the persisted Normal/PressedFileNum are authoring-install indices). Trap 1 deliberately
  *not* applied: MA's `OLE_COLOR` is already 0x00BBGGRR end to end, so converting would be
  the "twice" BoB warns about. Trap 3 is a draw-path concern and no MA screen shows the
  symptom yet.

### S62-2 Payoff: title bar + parity re-verdicts (2 pts) — ◐ payoff proven, title bar NOT
- **The FONT/COLOR payoff is real and gold-verified.** With the reader on, Preferences
  goes from the "white bold serif" of cross-cutting deviation #1 to **blue labels and
  yellow values** — sampled against the original gold PNG
  (`BEA6-BBCE/ma/Screenshot from 2026-06-24 17-00-45.png`), that is gold's scheme. The
  title menu turns yellow, likewise matching. This is BoB's "13 of 14 screens snapped
  toward gold" reproducing on MA. **The colour half of cross-cutting #1 is solved**; the
  font *face* and size half remains.
- **The title bar was not reached**, because enabling the reader breaks the capture
  recipes (below).

### Why it ships OFF — two blockers, both found by measurement
1. **An uninitialised read surfaces as garbage text at the title screen's top-left.** It
   **varies between runs** (`cÂôÿ"Ÿ:` then `c«¶ÿ"Ÿ:`) — the tell for an uninit read rather
   than a bad persisted value — and it is absent from the S61 reference even at 6×
   contrast, so it is new. Bisected: it is NOT the stock caption (still present with
   caption application removed) and NOT a persisted string (still present with
   `PX_String` forced to defaults). Not yet root-caused.
2. **The persisted FontNum changes the title menu's row pitch (~16px → ~28px), so every
   fixed-coordinate `BOB_CLICKSEQ` recipe lands on the wrong row.** The `quickmission`
   capture came back showing *Preferences*; the campaign recipe never reaches the map.
   This invalidates the parity capture recipes **and** `asan_all.sh`'s drive recipes
   together — re-deriving them is a body of work in itself, and doing it half-way would
   leave the regression gate untrustworthy exactly when the diff is largest.

Landing it opt-in keeps the default path byte-identical to S61 (verified below), keeps the
gate trustworthy, and leaves a complete, exercisable component for S63 to switch on.

### Gates
- **2D parity sweep — 6/6 unregressed on the default path.** `title`, `prefs_3d`,
  `quickmission`, `campaign_map`, `map_playerlog` **byte-identical**. `prefs_controls`
  differs **for an environmental reason, not a code one**: the reference was captured with
  a Logitech Extreme 3D attached and this box now has no joystick
  (`/dev/input/js*` absent), so the panel reads "NOT CONNECTED / 0 axes" instead of
  "4 axes, 1 hat(s), 12 buttons". Recorded in the parity doc — **that reference embeds
  live hardware state and is not a stable oracle**, which is the S59 device-presence
  lesson one level further out.
- `port/asan_all.sh` — PASS 4/4 modes, 0 reports (default path).
- `port/stress_launch.sh` — PASS 20/20.

### Carry-over to S63 (this is now a well-defined, self-contained sprint)
1. **Root-cause the uninit garbage** — run-to-run variance says uninit read; the S61
   precedent (unchecked `RegQueryValueEx` + uninit locals) is the pattern to look for
   first, and `MA_DLGINIT_PROPS=1` reproduces it on the plain title screen in ~2s.
2. **Re-derive the capture + ASan drive recipes** against the new metrics — ideally
   replacing fixed pixel coordinates with a click-by-menu-row-index helper so the recipes
   stop being font-dependent at all. That is the durable fix and it pays back every future
   font change.
3. Then switch the reader on by default and re-verdict the whole parity set — the
   Preferences colour change alone should move several rows.
4. Player Log title bar + `?`/`✓` (the original S62-2 target, unblocked once 1–3 land).
