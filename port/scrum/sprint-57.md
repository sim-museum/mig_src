# Sprint 57 — "PE resource path adopted" (autonomous; GL verification HARD-BLOCKED)

**Goal:** adopt BoB's S124 parity-oracle resource design (cross-port note 14 / lessons
§8f) so the Sprint-56 PARTIAL screens driven by missing labels (#7 prefs Controls,
#8 prefs Others) go label-correct vs the BDG 0.85F gold shots.
**Committed:** ~8 pts (main story ~6, close-out ~2; I4 explicitly not pulled — see blocker).

## Environment gate (checked FIRST, per sprint contract)

`DISPLAY=:0 glxinfo -B` fails machine-wide with `X_GLXCreateNewContext BadValue`
(same wedge as last session; re-checked mid-sprint under the display lock — still
broken). **No GL app can create a context, so every run-the-game verification is
blocked**: re-captures for the parity verdicts, `port/asan_all.sh`,
`port/stress_launch.sh`, front-end render regression. Per the contract we did NOT
crash-loop launches; the story was developed and verified **headless against the
resource module directly** (production TUs compiled into a standalone harness).

## Story 1 — adopt the PE resource path (BoB note 14) — ✅ code + headless DoD; ◐ GL re-capture blocked

**Resource-module finding:** `English/TEXT/miglang.dll` (487 KB, dated 2005-04-29 = the
BDG 0.85F patch, exactly note 14's prediction) + `Mig.exe` (2005-05-02, also BDG). Both
parse: enumerators count 122 dialogs / 1041 dialog items / 930 DLGINIT records across the
two modules (dedup: language DLL wins, matching `bob_res_get` precedence). IDD 271
(CSSound = prefs "Others") and IDD 958 (SController = prefs "Controls") both live in
miglang.dll.

**Key discovery — MA was already PE-first.** `ma_dlgtmpl.cpp` has read RT_DIALOG/RT_DLGINIT
from the installed modules via `bob_res_get` since Phase 4; there was never a source-`.rc`
path to demote (unlike BoB). Per the reuse rule the story became: copy the port-only piece
(the two enumerators), apply the *technique* (BoB's four §8f lessons) to MA's own files.
The baseline harness proved the missing labels were **not** a parse bug: all label text
parses fine; the gaps were creation/consumption-side, exactly BoB's lessons.

| Piece | File(s) | Status / evidence |
|---|---|---|
| Enumerators (`bob_res_enum_dialog_items`/`_dlginit`) | `SRC/compat/bob_resources.cpp` | ✅ copied from BoB S124 (port-only diff) + MA adaptation: dual-module walk with per-dlgId dedup |
| Parser correctness: classic creation-data advance (count INCLUDES its size WORD; old `2+cd` was the EX semantics = §8f desync trap) + DLGTEMPLATEEX support + class capture → kind | `SRC/compat/ma_dlgtmpl.cpp` | ✅; harness A/B: current resources have cd==0 everywhere on these dialogs → rects byte-identical, fix is future-proofing |
| **Template-driven static hosting** (§8f lesson 3 — the actual #7/#8 root cause) | `afxwin.h` `ma_host_template_statics` (after OnInitDialog, when all DDX registrations exist), synthetic CWnd clients through the normal `ma_ole_create` path, DDX-registered for GetDlgItem/idempotency | ✅ headless: IDD 271 statics 2023–2028 (the exact 6 missing "Others" labels) and IDD 958's 2027 "Dead Zone:"/2078 "Airframe"/2080 "Stick"/2083 "Rudder" are precisely the unbound set |
| **IDS→string-table caption resolution** (§8f lesson 4) | `ma_dlgtmpl.cpp`: DLGINIT keeps the `IDS_*` name; `ma_dlg_label` resolves via runtime-parsed RESOURCE.H (5444 defines) → `bob_load_string` (BDG string table); literal fallback; `IDS_NONE` sentinel skipped | ✅ headless: 958 id 2024 "Input Device:" (stale literal) → **"Input Devices:" = the gold wording** |
| **Membership draw+click filter** (§8f inverse lesson) | `ma_dlg_in_template` (1/0/−1); applied in `ma_ole_draw_all` + `ma_ole_click` panel paths only (toolbar path unfiltered, as BoB) | ✅ API verified; expected to kill #9's stray combo (capture blocked) |
| Button DLGINIT properties: FIL_* art + design-time String | `ma_dlg_artnum` (F_GRAFIX.G runtime equate table, 371 entries) + `ma_ole_set_artnum`/`ma_button_set_string`; applied from DDX_Control | ✅ headless: tickboxes 2358/2360 → FIL_ICON_TICKBOX1=0x6a81 + glyph "3" (the Enable/Use-for-FF checkboxes) |
| **REdtBt OCX hosted** (Calibrate was invisible because 0x461a1fe3 was an unhosted type) | new `SRC/compat/ma_oleredtbt.cpp` + `CT_EDTBT` routing (setprop/getprop/draw/click→Clicked) + `SRC/REDTBT/REDTBTC.CPP` compiled (new `oleredtbt` mode in rebuild.sh + CMake); 2 sound lines gated `#ifndef MA_LINUX` (same gate as RBUTTONC.CPP) | ✅ compiles+links; caption arrives at runtime via `SController::OnInitDialog` `SetCaption(RESSTRING(CALIBRATE))` → stock DISPID_CAPTION → `SetText` |
| DI axis names (#7 "active joystick : &") | `bob_video.cpp` `DIDEV_EnumObjects`: `tszName` was never filled → "Axis %d" (joystick), "X-Axis"/"Y-Axis" (mouse), buttons/hats too | ✅ code; gold shows exactly "Axis 0 & Axis 1"/"X-Axis & Y-Axis" |
| Escape hatch | `MA_NO_PE_RSRC=1` reverts the whole S57 layer (old parse semantics; no kind/IDS/art; hosting/filter/button-caption inert via `ma_pe_layer_on`) | ✅ harness A/B: hatch output **byte-identical** to the unmodified-S56-TU baseline (rects+labels) |
| `-DMA_SRC_DIR` (RESOURCE.H / F_GRAFIX.G location; `$MA_RC_DIR` overrides) | rebuild.sh COMMON + CMake `ma_flags` | ✅ both builders |

**Headless DoD evidence:** standalone harness (scratchpad `rsrc_harness/`) compiles the
PRODUCTION `bob_resources.cpp` + `ma_dlgtmpl.cpp` unmodified and drives them against the
installed miglang.dll/Mig.exe. Baseline-vs-S57 outputs captured for IDD 271/958;
`MA_NO_PE_RSRC=1` A/B identical to baseline. Full build: CMake+Ninja incremental, `wmig`
links, 32-bit ELF, 0 undefined symbols.

**No .rc fallback (decision):** BoB demoted its source-`.rc` parse to fallback; MA never
had one and does not need one — Mig.exe (BDG-patched, same install) already backfills any
dialog miglang.dll lacks, and a source-derived fallback would contradict the SM oracle
ruling (gold shots as-is = BDG 0.85F data). Recorded here per §8f's process note.

## Story 2 — I4 Playerlog completion — ⬜ NOT PULLED (GL-gated)

Every named gap (dialog frame/title bar, CRTabs tab bar, stats table + Name edit,
centring) is render work whose only acceptance is a native capture vs gold shot #15 —
impossible under the GLX wedge. Deliberately left in the backlog rather than landing
blind render code.

## Regression gates

- `port/asan_all.sh` / `port/stress_launch.sh` / front-end render check: **SKIPPED — GL
  blocked** (contract note). Compile+link gates pass; the parser layer is A/B-proven
  inert under its hatch; the biggest un-run risk is the new hosting/draw integration
  (synthetic statics, button String application on non-prefs screens, REdtBt draw) —
  first GL session must run `asan_all.sh` + re-capture #2–#9 before any verdict flips.

## Carry-over / next (S58)

1. GL restored → re-capture #7/#8/#9 (+#2–#6 for incidental deltas), flip verdicts,
   run `asan_all.sh` + stress gate; `MA_NO_PE_RSRC=1` in-game A/B if anything regresses.
2. I4 proper (8 pts, unchanged).
3. Residual #7 item to watch: checkbox CHECK-STATE (Pressed) wiring — art+glyph now
   present, state comes from game runtime (`SetPressed`, dispid 14, already routed).
