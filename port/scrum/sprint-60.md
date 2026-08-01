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

*(filled in as stories land)*
