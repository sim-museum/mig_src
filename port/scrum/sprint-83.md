# Sprint 83 — "Check every site" — ✅ CLOSED 2026-08-08 — ⭐ a one-line uninit fix that had two dialogs deferred for 30 sprints

**Planned 2026-08-08 (PO pre-approved ceremonies). Autonomous. Committed ~8 pts.**
**Sprint Goal:** act on BoB's note-19 ask (sweep every `SendMessage`-result deref individually, since
one guarded site says nothing about the next), and clear the two OOB dialogs the click path still
defers.

| Story | Pts | Result |
|---|---|---|
| S83-1 sweep unchecked `SendMessage`-result derefs | 3 | ✅ 4 sites hardened + the gap made visible |
| S83-2 unblock Authorise (2023) / Directives (2074) | 3 | ✅ SEGV root-caused + **fixed**; a second blocker named |
| S83-3 gates + cross-port | 2 | ✅ |

## Execution log

### S83-1 — the sweep — DONE
**Root of the class, found by reading the dispatcher rather than the call sites.** The port's
`RDialog::OnRowanMessage` implements **8 of the 14** routes in the engine's real message map and
everything else falls to `default: return 0` — which callers then treat as a valid pointer. The six
unrouted: `WM_GETHINTBOX`, `WM_GETCOMBODIALOG`, `WM_GETCOMBOLISTBOX`, `WM_ACTIVEXSCROLL`,
`WM_GETSTRING`, `WM_COMMANDHELP`. Each is now listed in-code with *why* it is still unrouted, and
`MA_TRACE_MSG=1` prints any unrouted message with its class, so a silent 0 is diagnosable.

**Measured, not assumed:** on the campaign + OOB path the only route actually exercised is
`WM_GETSTRING` (WM_USER+16) — 4 classes hit it (`CSystemBox`, `CPlyr_log`, `CCareer`, `CLoad`),
which confirms S63's zero-the-locals fix is still load-bearing.

**Four sites hardened** with the codebase's own safe spelling:
- `CRButtonCtrl::OnLButtonUp` — `phintbox->ShowWindow` (the one S82 had to route around).
- `CRButtonCtrl::OnMouseMove` — the hover tooltip: five derefs plus a deref of the DC it returns;
  the whole block is skipped when there is no hint box.
- `CRComboCtrl::OnLButtonDown` — `WM_GETCOMBODIALOG` + `WM_GETCOMBOLISTBOX`, both dereferenced.
- `CRComboCtrl::OnTextChanged` — survives today only because the enclosing `if (… && m_hWnd)` is
  false in the port. **An accidental guard, not an intentional one.**

**BoB's warning about case-variant twins does NOT transfer as-is.** They report `rbuttonc.cpp` and
`RBUTTONC.CPP` as distinct stale files. Tested it here empirically (appended a probe to the
lowercase name and grepped the uppercase): in MA's tree they are the **same** file — writing either
name changes the compiled one. Note MA's `CLAUDE.md` says other twins *have* diverged
(`VIEWSEL.CPP` vs `Viewsel.cpp`), so this is per-file: **verify, don't generalise, in either
direction.** (The probe was removed; `git diff` confirmed only the intended 14 lines remained.)

### S83-2 — the deferred dialogs — SEGV FIXED
Since S82 routes real clicks, ids 2023/2074 were user-reachable buttons that silently did nothing.
Added `MA_OOB_NO_DEFER=1` to lift the guard deliberately, then reproduced.

**The recorded cause was wrong.** The in-code note blamed `CComit_e -> DirControl::AllocateAc`. A
symbolized backtrace says:
```
CSupply::AddSupplyMission(int, SupplyNode*)  SUPPLY.CPP:131
CSupply::SortSupplyNodes()                   SUPPLY.CPP:162
CSupply::SortIntell()                        SUPPLY.CPP:337
CSupply::OnInitDialog()                      SUPPLY.CPP:454
```
**Root cause — a half-applied for-scope hoist (one line).**
```c
int i;  // Linux/GCC port: for-scope hoist
for (int i = (MAX_TARGETS-1); i > j; i--)   // <-- re-declares i, SHADOWING the hoisted one
    target[i] = target[i-1];
target[i].activity = …;                      // <-- reads the OUTER i, which nothing ever wrote
```
Under MSVC's for-scope leak the loop variable *was* that variable and left the loop holding `j`,
which is the index the inserts are addressed by. The port's mechanical hoist added the declaration
without removing the inner one, so `target[i]` indexed on uninitialised stack → wild write → SEGV.
Dropping the inner `int` restores the original semantics exactly (the loop exits with `i == j`).
Textbook member of the port's own uninit-read class, and of its own tooling's failure mode.

**Swept the tree for the same tooling bug:** 15 matches across 7 unique files (the rest are
case-variant listings). Checked each for a use of the variable *after* the loop —
`AddSupplyMission` is the only one where the shadowed variable is read, i.e. the only harmful
instance. The others are inert.

**Result:** both dialogs now **build and paint** — the Authorise OOB tree comes up 501×407 with all
five tab pages (Supply / Chokepoints / Traffic / Airfields / Army).

**They stay deferred for a SECOND, freshly-named reason.** Opening Authorise now trips
`[SysError] Opened file block (6a78) again without closing!` → `SayAndQuit`. That is the same
double-open family S79 fixed for `0x6a63` in the debrief preload (`fileman::MA_IsFileOpen` + a skip
guard), so the recipe exists. Booked as the top S84 item; the defer comment in
`ma_ole_toolbar_click` now records the corrected cause and the new blocker.

## Gates — all under `gl-lock`
- **2D parity: 5/5 byte-identical.**  **Stress: 20/20 PASS.**
- **ASan `asan_all.sh 2 80`: PASS — 0 reports, all 4 paths reached 2/2.**
- Note the diff touches four R\* control TUs and the shared `OnRowanMessage`, so the byte-identical
  parity sweep is again the load-bearing gate: the added guards are all on paths that return early
  only when a pointer is already NULL, and the sweep shows nothing that used to render changed.

## Cross-port
**MA note 32**: the sweep results (MA has 2 hint-box sites, not 4 — different control revision), the
dispatcher-subset framing which is the *root* of the class on both sides, the twin-file
counter-finding, and the shadowed-hoist bug — which BoB should sweep for, since the same hoist
tooling was applied to their tree.

## Result
BoB's "check each site individually" ask produced four hardened derefs *and* the framing that makes
them tractable: the port's message dispatcher silently answers 0 for six routes it never implemented.
Separately, a crash that had kept two dialogs deferred since S52 turned out to be one shadowed loop
variable.
