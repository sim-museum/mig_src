# Sprint 52 — "OOB-info dialog epic: build-crash fixed" (autonomous, headless DoD)

**Goal:** un-blacklist the 3 toolbar buttons (Squads/Authorise/Directives) that SEGV on click by fixing the
OOB-info dialog build. (Follow-on to S50's blacklist; roadmap = BoB S113.)

## Diagnosis (gdb, not assumption)
I'd assumed a NULL `fchild` tree; gdb showed the crash is **inside `CSqdnlist::Make()`** — the
`MakeParentDialog` tree build itself, in a child dialog's `OnInitDialog`:
```
#0 RDialog::InDialAncestor()  #1 RDialog::SetMaxSize()  #2 CSquads::OnInitDialog()
#3 CDialog::Create()  #4 MakeParentDialog()  #6 CSqdnlist::Make()  #7 OnClickedSquads()
```

## Two GENERAL root causes fixed
1. **`CDialog::Create(UINT idd, CWnd* = NULL)` discarded its parent** (`afxwin.h`): the param was unnamed, so
   `m_maParent` stayed NULL → **`GetParent()` returned NULL in every dialog's `OnInitDialog`**.
   `CSquads::OnInitDialog` does `((RDialog*)GetParent())->SetMaxSize(...)`→`InDialAncestor()` NULL-derefs
   (`fault_addr=0xd0`). Fix: `if (pParent) m_maParent = pParent;` before `OnInitDialog`. Real MFC sets the
   parent in Create; this is correct + general (fixes GetParent() for ALL dialogs).
2. **Unit-conversion SIGFPE** (S3 HUD-FPE family): `CSqdnlistBut::OnInitDialog` computes
   `bingofuel/(100*Save_Data.mass.gm)` with `mass.gm==0` (units unset on the campaign path). Fix: the MIG.CPP
   map branch calls `Save_Data.SetUnits()` when `!mass.gm` (mirrors STUB3D `MakePassive`), before any OOB open.

Plus a defensive null-guard in `OnClickedSquads` (MA_LINUX): `GetDlgItem(IDJ_TABCTRL)` returns NULL (CRTabs
not hosted yet) → guard `if (tab) tab->SelectTab(entry)` so it degrades instead of SEGV.

## Result
- **Squads OOB dialog tree BUILDS cleanly** (`top`/`fchild`/`fchild2` all non-NULL, no crash) — un-blacklisted.
- Regression clean: front-end renders (100% non-black), `asan_campaign` + `asan_flight` gates PASS (0 reports).
- **Still deferred: Authorise(2023) + Directives(2074)** — deeper dialog-specific `OnInitDialog` crashes
  (Directives = `CComit_e::OnInitDialog`→`DirControl::AllocateAc`→`ListSupplyNodes`→`AddMission` NULL deref).

## Next (S53)
The built OOB dialog **doesn't render** — the map branch drives only the toolbar draw, not the logged-child
RDialog paint. Drive the OOB dialog's paint in the map idle (mine BoB S113); host the CRTabs tab control; then
the Authorise/Directives deeper crashes.
