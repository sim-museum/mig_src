# Sprint 59 — "Quick Mission settles" (autonomous)

**Goal:** the S58 carry-overs on the Quick Mission screen close — the #9 stray combo's
runtime-hide mechanism is root-caused (BoB note 17 trap-3 settled-state hypothesis first)
and the mission text wraps to the panel — and the uninit-PX net widens to every hosted R*
control, all held to the dummy==GL byte-identical bar.

**Committed (~8 pts):**
| Story | Pts | Definition |
|---|---|---|
| S59-1 Note 17 inbound + R* uninit-PX audit | 2 | BoB note 17 inbound-committed + processed (traps logged/applied where MA shows the symptom); RStatic/RButton/RCombo/REdtBt ctors init every DoPropExchange-persisted member to its PX default (S58 RLISTBXC pattern); dummy==GL `cmp` bar holds post-fix |
| S59-2 #9 stray-combo hide mechanism | 3 | Identify the control at ~(590,165) in the installed Quick Mission template; root-cause why Windows' settled screen lacks it (trap-3 cover-erase vs an unrouted hide); route/emulate the mechanism; re-capture `quickmission.png`; update parity table |
| S59-3 Mission-text word-wrap | 2 | Mission text wraps within the panel (no right-edge run-off) on the Quick Mission screen; re-capture; parity table updated |
| S59-4 Cross-port note 17 reply + close | 1 | MA note 17 delivered to `bob/doc/`; any new engine finding appended to BOTH shared-doc copies (md5-identical); board + burndown updated |

**Not pulled:** #12 debrief capture, I4 Player Log (8 pts — still a full sprint on its own),
Scenario/UN radio row (RRadio OCX not hosted — bigger than this sprint's slack).

**Planning notes:**
- Note 17 read at sprint start. Trap 3 (template-visible control absent from Windows'
  settled screen because a sibling's first repaint re-blits panel art over it, never
  re-invalidated) is a direct candidate mechanism for #9 — S58 already disproved the
  filter and `incomms` hypotheses; `m_maVisible=1` says no routed hide fires.
- Uninit-PX audit targets confirmed by source read: `RSTATICC.CPP` ctor misses
  `m_FontNum`/`m_ResourceNumber`/`m_ShadowColor`; `RBUTTONC.CPP` misses ~10 persisted
  members (`m_bMovesParent`, `m_FontNum`, `m_bCloseButton`, `m_bTickButton`,
  `m_bShowShadow`, `m_ShadowColor`, `m_ResourceNumber`, `m_NormalFileNum`,
  `m_PressedFileNum`, hint fields); `RCOMBOC.CPP` misses `m_FontNum`/`m_ListboxLength`
  (PX default 100 — garbage here shapes every dropdown); `REDTBTC.CPP` misses
  `m_FontNum`/`m_bDrawBitmap`.

## Results

### S59-1 — Note 17 inbound + R* uninit-PX audit (2 pts) — ✅
- BoB note 17 inbound-committed (`f06bda5`) with the lessons-doc sync (md5-identical).
  Traps 1/2 logged for the future stream-reader adoption (MA payoff = FONT/COLOR set,
  per BoB's §3 — queued behind the font cross-cut). Trap 3 was the right *shape* for #9
  but the wrong mechanism — chasing it found the real one (S59-2).
- Ctor-init blocks added to `RSTATICC.CPP` (`m_FontNum`/`m_ResourceNumber`/
  `m_ShadowColor`), `RBUTTONC.CPP` (10 persisted members), `RCOMBOC.CPP`
  (`m_FontNum`/`m_ListboxLength=100`), `REDTBTC.CPP` (`m_FontNum`/`m_bDrawBitmap`).
  Capture-visible wins: #3/#4/#5/#7 large-font value rows (garbage `m_FontNum`) now
  match sibling rows; Controls tickbox glyph sits inside its box art.

### S59-2 — #9 stray-combo hide mechanism (3 pts) — ✅ ROOT-CAUSED + FIXED
- Identified the cluster: ids 2069 (`IDC_WEATHER`), 2246 (`IDC_CLOUD`), 2025 (RStatic),
  1118 (native Button) — Cloud/Weather are fully dead-coded in `SQUICK1.CPP`.
- **Mechanism = Windows parent-rect clipping, not a runtime hide**: the cluster sits at
  dlu x=367–389; IDD 287's own header says cx=335 dlu. Children clip to the parent
  dialog on Windows → can never paint. Host now parses the per-control style dword +
  dialog cx/cy: `ma_dlg_never_visible` (fully-outside ⇒ skip draw + click, beside the
  S57 membership filter) and `ma_dlg_template_visible` (WS_VISIBLE ⇒ initial
  `m_maVisible`, runtime ShowWindow overrides — Windows semantics).
- **Bonus root-cause**: #9's "I.D." label (2 sprints mis-filed "resource delta?") is
  id=2023 style 0x40010000 = `!WS_VISIBLE` — now correctly absent, matching gold.
- Re-captured: stray cluster GONE, "I.D." GONE. Verdict #9 → **CLOSE-minus** (one named
  deviation left: Scenario/UN radio row — RRadio OCX `{5363BA22}` not hosted, backlog).

### S59-3 — Mission-text word-wrap (2 pts) — ✅
- `CRStaticCtrl::OnDraw` always asked for `DT_LEFT|DT_WORDBREAK|DT_TABSTOP`; compat
  `CDC::DrawText` was a one-line TextOut. Implemented real multi-line DrawText
  (wraps to rect width with current-font measuring, '\n', tabs-as-spaces, DT_CALCRECT,
  returns height; fitting text stays one line → single-line callers unchanged).
- Capture: mission text wraps to 3 in-panel lines (gold: 4 — wider art font,
  cross-cutting #1), no right-edge run-off.

### Acceptance bar + a NEW catch
- dummy==GL `cmp` **byte-identical** re-verified on #7 prefs-Controls and #9 Quick
  Mission — after the bar caught a **second environment-dependence class**: the DI
  system mouse was enumerated only `if (g_win)`; headless (dummy) has no OPENGL window
  → "3d Pointer" row read "Keyboard" vs GL's (and gold's) "active mouse : X-Axis &
  Y-Axis". Fixed: `GUID_SysMouse` reported unconditionally (Windows semantics;
  `bob_video.cpp DI_EnumDevices`).

### Gates
- `port/asan_all.sh` — **PASS 4/4 modes** (flight, camp-map, camp-fly, camp-nextday;
  2/2 runs each; 0 ASan reports; `/tmp/asan_all_summaries.txt` empty of new entries).
  Run synchronously in one-mode chunks (`asan_one_mode.sh`) — flight before the
  session-limit kill, the three camp modes by the PO session after salvage.
- `port/stress_launch.sh` — **DEFERRED (environmental)**: desktop session locked
  (`gnome ScreenSaver GetActive=true`, `LockedHint=yes`) → new GL windows are never
  presented → swapchain fills after exactly 3 frames ([present] trace) and SwapBuffers
  blocks in `DRM_IOCTL_SYNCOBJ_TIMELINE_WAIT` → 20/20 HANG at title. Exonerating
  evidence: same binary reaches 3D headless; ASan binary shows the identical GL stall;
  S59's only GL-adjacent diff (DI mouse presence) leaves the GL path bit-identical to
  S58, whose stress run was 8/8 hours earlier on the then-unlocked session.
  Rerun post-unlock: `flock /home/admin/.gl-display.lock -c 'bash port/stress_launch.sh'`.

### S59-4 — Cross-port (1 pt) — ✅
- MA note 17 delivered to `bob/doc/` (template-visibility routing checklist for BoB's
  host; DrawText DT_WORDBREAK; device-presence determinism; verdict news).
- §8f addendum "Dialog templates hide controls TWO ways" + DrawText + device-presence
  — BOTH shared-doc copies updated, md5-identical.
