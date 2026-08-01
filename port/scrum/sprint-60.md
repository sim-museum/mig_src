# Sprint 60 — "The Player Log opens" (autonomous)

**Goal:** I4's structural blocker falls — the **RTabs OCX gets hosted**, so the Player Log
stops being a bare pilot-photo blit and becomes a real tabbed dialog: Career / Log of
Missions / Last Mission tab bar, a proper frame + "PLAYER LOG" title bar, positioned where
the gold shot puts it. Held to the standing dummy==GL byte-identical capture bar.

**Committed (~8 pts):**
| Story | Pts | Definition |
|---|---|---|
| S60-1 Host the RTabs OCX (CRTabs) | 3 | CLSID `0x4a1e1986` routed in `ma_ole_create` → new `SRC/compat/ma_oletabs.cpp` reusing the real `CRTabsCtrl::OnDraw`; build mode `rtabs` (`-ISRC/RTABS -include afxctl.h`); `GetDlgItem(IDJ_TABCTRL)` returns a real `CRTabs` so `RDialog::AddChildren`'s `SetHorzAlign` + `AttachTabToTabControl` run for real; the 3-tab bar renders in the `map_playerlog` capture |
| S60-2 Player Log frame + title bar | 2 | `CPlyr_log` (IDD 276) draws its frame and the "PLAYER LOG" title bar with the ?/✓ buttons, as in gold #15 |
| S60-3 Dialog placement honoured | 2 | The dialog stops drawing pinned at top-left — `MakeTopDialog(Place(x,y),…)` / `Edges` routed so it lands where gold has it over the map |
| S60-4 Cross-port note 18 + close | 1 | MA note 18 delivered to `bob/doc/` (BoB hosts the same R* control family — tab hosting is directly reusable); both shared-doc copies md5-identical; board + burndown + parity table updated; `stress_launch.sh` default `WMIG` pointed at the ninja artifact |

**Not pulled:** the Career tab's *content* — Name edit box + the per-type
Sorties/Combats/Kills/Losses table (F86 1 / F86 2 / F80 / F84 / F51 / All). That is the
other half of I4 and is a sprint of its own → S61. Also still out: RRadio OCX hosting
(#9's last named deviation), #12 debrief capture, cross-cutting font (#1) / chrome (#2).

**Planning notes (evidence gathered at planning, 2026-08-01):**
- **S59's deferred stress gate was cleared first, before planning** — 20/20 OK on the
  now-unlocked session, same commit/binary that scored 0/20 while locked. S59 therefore
  closes with all gates green and carries **nothing** into S60. See `sprint-59.md`.
- The Player Log tree is built in `MAINTBAR.CPP:315` `CMainToolbar::LaunchPlayerLog`:
  `MakeTopDialog(Place(x,y), DialList(DialBox(FIL_NULL, new CPlyr_log, Edges(...)),
  HTabBox(FIL_NULL, IdList(IDS_CAREER, IDS_MISSIONLOG, IDS_LASTMISSION), Edges(...),
  DialBox(FIL_MAP_PLAYER_LOG, new CCareer), DialBox(…, new CMisn_log),
  DialBox(…, new CLastMissionLog))))`. So all four gold-shot deviations trace to ONE
  structural gap plus its knock-ons: the `HTabBox` (`childtype=TABT`) arm.
- `RDialog::AddChildren(diallist, childtype, titles)` (`RDIALOG.CPP:612`) is real,
  compiled game code and already does the tab work — but it opens with
  `CRTabs* tabControl = (CRTabs*)GetDlgItem(IDJ_TABCTRL); tabControl->SetHorzAlign(TRUE);`
  and ends each child with `dial->AttachTabToTabControl(titles->list[i])`. `IDJ_TABCTRL`
  = 1002. With RTabs unhosted this is the prime suspect for why only the innermost
  `CCareer` art survives to the canvas. **Verify the actual runtime behaviour with
  `MA_TRACE_OLE`/`MA_TRACE_DLG` before writing code** — S58's "membership filter"
  post-mortem is the standing lesson against inheriting a hypothesis.
- Hosting RTabs is *precedented, not novel*: `SRC/RTABS/` is a complete OCX tree with a
  real `CRTabsCtrl::OnDraw` (`RTABSCTL.CPP:269`), exactly like RSTATIC/RBUTTON/RCOMBO/
  REDIT/REDTBT. Follow `ma_olestatic.cpp` and the `port/rebuild.sh` per-control mode.
- **Apply the S59 uninit-PX lesson up front:** `CRTabsCtrl`'s ctor must init every
  `DoPropExchange`-persisted member to its PX default before first draw, or we buy the
  same environment-dependent heap-garbage class that cost S58 a sprint.

## Results

**Sprint outcome: 5 of 8 pts. S60-1 and S60-3 landed the mechanisms and are demonstrably
working; S60-2 partially renders; the tab bar is populated and drawn but not yet
composited at the right screen offset, so I4's acceptance is NOT met and #15 stays
PARTIAL. Everything below is capture- or gate-verified; nothing is claimed on inspection.**

### The planning hypothesis was wrong in a useful way
Planning said "RTabs is unhosted → GetDlgItem(IDJ_TABCTRL) returns NULL". The trace said
otherwise on the first run: **RTabs was never even CREATED** (`MA_TRACE_OLE` over the whole
Player Log path shows RButton ×57, RScrlBar ×16, RListBox ×5, RStatic ×3, REdit ×2 — and
zero `4a1e1986`). Hosting the CLSID alone would have changed nothing. Root cause instead:

> **Template-declared OCX controls that no dialog class `DDX_Control`-binds were never
> instantiated.** On Windows the dialog manager creates *every* template item; this port
> only ever created what the game explicitly bound. `RDEmptyP::DoDataExchange` is empty —
> it binds nothing — so IDJ_TABCTRL never existed.

This is the same class S57 found for the prefs row labels (`ma_host_template_statics`),
which had been fixed *for RStatic only*. S60 generalizes it to a kind table.

### S60-1 Host the RTabs OCX (3 pts) — ✅ mechanism done, ◐ not yet visible
- `ma_dlgkind.h` (NEW): the template-control kind taxonomy, now shared by the parser and
  its consumers instead of duplicated. Added `MA_K_RTABS` (`4a1e1986`) and
  `MA_K_RSCRLBAR` (`505aee46` — classified for audit only; **RScrlBar is created 16× on
  this path and is still unhosted**, logged as a backlog find).
- `ma_oletabs.cpp` (NEW) + `rtabs` build mode (CMake **and** `port/rebuild.sh` — the ASan
  build goes through rebuild.sh and caught the omission at link time).
- `ma_host_template_statics` → **`ma_host_template_controls`**, driven by a kind table
  (RSTATIC / RBUTTON / RTABS) with a per-kind `needsLabel` rule — a tab bar legitimately
  starts empty, a caption-less static does not.
- **Verified working end to end:** `SetHorzAlign`/`SetFirstTab` dispatch, and
  `RDialog::AttachTabToTabControl` now adds all three tabs with the gold captions —
  `[tabs] AddTab "Career" / "Log of Missions" / "Last Mission"`.
- **Tab art from the OCX's own PE:** `CRTabsCtrl::OnDraw` loads IDB_TABUP/IDB_TABDOWN from
  *RTabs.ocx*, not Mig.exe, so the compat `CBitmap::LoadBitmap` no-op could never serve
  them. `RTabs.ocx` ships in the install dir → preloaded through the existing PE resource
  layer into memory DCs, `m_bInit` cleared so OnDraw skips its own load. Verified:
  `[tabs] art up=… (297x31) down=… (297x31)`. Degrades to text-only tabs if absent.
- S59 uninit-PX audit applied and **checked, not assumed**: `CRTabsCtrl`'s ctor already
  inits its only PX-persisted member (`m_FontNum`). Its four HICONs, however, are never
  assigned in the shipped source (every `LoadImage` is commented out), so `DrawIconEx` was
  handed heap garbage on Windows too — NULLed under `MA_LINUX` so the no-op is
  deterministic. `DrawIconEx`/`DI_NORMAL` added to the compat as a faithful no-op.
- **Not met:** the tab bar does not appear in the capture. It is created, populated, sized
  (420×258 host) and drawn — `ma_tabs_draw` runs — but lands off the visible region. The
  remaining unknown is `RDialog::OnGetXYOffset`, whose parent-walk only accumulates an
  offset when `newparent->parent->artnum == artnum`; every node in this tree has
  `artnum == 0` except the tab pages. That is the S61 starting point.

### S60-2 Player Log frame + title bar (2 pts) — ◐ partial
IDD 276 declares `id=1001 IDJ_TITLE` as an **RBUTTON** (kind 4 — a hosted kind all along),
so the generalized template hosting picks it up and it now renders. But it draws at the
wrong offset with a truncated caption (the capture shows a title strip with "on" visible),
i.e. it shares S60-1's compositing defect. The ?/✓ buttons are not yet identified in the
template. **Bonus, unplanned:** the same change made the Career tab's **"Name" label and
Name edit box** render — content that #15 listed as missing and that S60 had explicitly
NOT pulled.

### S60-3 Dialog placement honoured (2 pts) — ✅ root-caused + fixed (systemic)
The deepest find of the sprint, and the reason the tab bar could never have worked:

> **No RDialog in a MakeTopDialog/AddChildren tree ever learned its own size.** The ctor
> zeroes `homesize`/`viewsize` and the one line that would refresh them from the client
> rect is commented out (`RDIALOG.CPP:147`). Windows sizes the dialog window from the
> template, so `GetClientRect` is meaningful there; here every RDialog answered 0×0.

Consequences, all observed: `MakeParentDialog`'s "if dialsize is meaningless" branch got a
0×0 client rect; `AddChildren` sized each child from `dial->homesize.Width() == 0`;
`RDialog::OnSize` then gave IDJ_TABCTRL a **zero-width** `MoveWindow`, and the draw loop
skipped it on its `w <= 0` guard. Fix: `RDialog::MaSeedTemplateSize()` +
`ma_dlg_own_size()` (the parser already read the template's cx/cy for S59's clip test but
never exported it), called from the three tree-builder `Create` sites.
Measured: tab host 0×0 → **420×258**, CPlyr_log → **336×396**, tab pages → **420×228**.

**Scoping note — a regression I caused and backed out.** The first version applied the
template size in `CDialog::Create`, i.e. to every dialog. That visibly broke the
front-end: canvas 644 → 600 with Load-panel art bleeding into the map. `Create` is shared
with the full-screen panels, which establish their size by other means. Re-scoped to the
tree builders only, which is the sole consumer of a dialog's own size. The comment at the
fix records this so it is not "cleaned up" into Create later.

Still open in S60-3: the top node (`RDEmptyD`) carries a garbage viewsize
(`978990,978859 -1957003x-1956942`) from the `Place()` centring math reading an
uninitialised `m_pView` window rect. It does not affect the OOB paint (which uses
`MaXYOffset`), but it is almost certainly the same defect family as the compositing
offset above — S61.

### S60-4 Cross-port note 18 + close (1 pt) — ✅
- `stress_launch.sh` now defaults `WMIG` to the ninja artifact (`build/wmig`) with the
  `/tmp/wmig` rebuild.sh path as fallback — the S59 gate had to be run with an explicit
  override.
- `rtabs` mode added to **both** builders (see S60-1).
- MA note 18 delivered to `bob/doc/`; both shared lessons-doc copies md5-identical.

### Gates
- **2D parity regression sweep — CLEAN.** Re-captured `title`, `prefs_3d`,
  `prefs_controls`, `quickmission` and `cmp`'d against the committed
  `port/ref/native/*.png`: **all four byte-identical, 0 changed pixels.** This is the check
  that mattered, given the diff touches `afxwin.h`, `RDIALOG.CPP` and `MIG.CPP`.
- **`port/stress_launch.sh` — PASS 20/20** (100 sustained 3D frames per run, unlocked
  session, under the display lock).
- **`port/asan_all.sh` — PASS 4/4 modes** (flight, camp-map, camp-fly, camp-nextday;
  2 runs each; 0 ASan reports). ASan build relinked with the new `rtabs` objects.

### Carry-over to S61
1. **Composite the tab bar and title bar at the right offset** — `OnGetXYOffset`'s
   `artnum == artnum` parent-walk vs. the all-artnum-0 Player Log tree. Fixes S60-1's
   acceptance and S60-2 together.
2. The `RDEmptyD` garbage viewsize / `Place()` centring against an uninitialised
   `m_pView` rect.
3. Career-tab content table (Sorties/Combats/Kills/Losses) — the half of I4 never pulled.
4. **RScrlBar (`505aee46`) is unhosted** though created 16× on the campaign-map path.
5. `?`/`✓` title buttons: identify them in the IDD 276 template.
