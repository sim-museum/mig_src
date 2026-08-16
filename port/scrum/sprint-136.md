# Sprint 136 — "The blank bars were a control nobody hosted" (PO-28) — ✅ CLOSED 2026-08-16 (goal MET)

**Planned 2026-08-15 (PO: continuous sprints, campaign dialogs, gold videos as reference).**
**Sprint Goal:** the D.I.S. dialog shows its buttons, its filters and its briefing.

| Story | Pts | Result |
|---|---|---|
| S136-1 what are the three blank bars? | 3 | ✅ RRadio — an unhosted control type |
| S136-2 why have the buttons no captions? | 3 | ✅ the caption policy admitted only tickboxes |
| S136-3 why is the briefing empty? | 2 | ✅ a consequence of 1 — the filter was inert |

The PO: *"DIS dialog — no briefing text, no text on dialog buttons"*, and more generally *"a lot of
map dialogs lack button text, and in the case of the situation dialog that is open by default,
body text as well."* Three independent causes, each measured rather than guessed.

## 1. RRadio was not a hosted control type

A CLSID census of the dialog (`MA_TRACE_OLE`) showed **2 × 0x5363ba22** — `CRRadioCtrl` — and the
router's table had no entry for it. So `CDIS::OnInitDialog` did this, into the void:

```c
pradio=GETDLGITEM(IDC_RRADIO_INTELLTYPE);
string.LoadString(IDS_TARGET);  pradio->AddButton(string);
string.LoadString(IDS_GENERAL); pradio->AddButton(string);
```

Now hosted (`SRC/compat/ma_oleradio.cpp`, the eighth control to follow this recipe), drawn, and
clickable. **The click walk uses the geometry the last PAINT recorded** — column stride
`m_ColumnWidth*tm.tmAveCharWidth`, row stride `tm.tmHeight+2`, both dependent on the font the
parent hands the control — rather than recomputing from the control rect, which would be a second
and divergent source of truth (S82's rule, applied in advance for once).

Two build notes: `RRADIOC.CPP` calls `LoadImage` on the **OCX project's own** .ICO resources,
which are not in the game module this port links; the handles are never read, because
`DrawTransparentBitmap` draws the radio glyph from the game's own `FIL_RADIO_BUTTON_UP/DOWN` art.
And its selection-sound line is guarded exactly as `RLISTBXC.CPP` already guards its own.

## 2. The caption policy was arguing from the wrong signal

The three bottom buttons drew as empty plates. Their captions are design-time, and the port's
policy (S57 broad → S58 narrowed to tickboxes after a regression) refused them.

`MA_TRACE_DLGBAG` — added to print the evidence rather than argue about it again — shows what
every control actually carries. **Nearly all of them have an `IDS_` name**, so "has a string
resource" cannot be the test: the system box carries `IDS_THUMBNAILMAP` / `IDS_ZOOMIN` /
`IDS_LOADSAVE` and the map filters carry `IDS_AIRFIELD` / `IDS_SUPPLY`, and those are **tooltips**.
Drawing them is precisely the S57 regression ("Quit"/"Size" materialising on icon buttons).

What separates them is the **art**:

| control | art | `IDS_` | means |
|---|---|---|---|
| system box, map filters | `FIL_ICON_*` | tooltip | the picture *is* the label |
| D.I.S. buttons | `FIL_MAP_DIS_BUTTON` | caption | a plate, with nothing to show but text |

So `ma_dlg_art_isplate()`: art present, not `FIL_ICON_*`, not `FIL_NULL`. In a full campaign-map
run that admits exactly three controls — the three that were blank — and they now read **Notes /
Footage / Intelligence**, yellow when enabled and grey when the game disables them.

## 3. The empty briefing was a consequence, not a cause

With the filter radios inert, nothing ever selected an intelligence category, so the list stayed
empty. Clicking **Target** now fires `Selected(0)` into `CDIS::OnSelectedRradioIntelltype` and the
body fills: *Kalmal — Enemy supplies getting through*, *Pyongtaek Supply Depot*, *Wonju Supply
Dump*, *Kangnung — Supply to enemy front restricted*. Nothing was wrong with the body at all.

## Evidence

`port/ref/native/dis_dialog.png` — the complete dialog: both radio groups with their captions and
selection ticks, the intelligence list with target names and messages, and three captioned buttons.

## Gates

parity 5/5 byte-identical · sweep 9 OPEN/0 CRASH · map icon click · sysbox exit · help click ·
stress 12/12. Both build systems updated (`port/rebuild.sh` + `CMakeLists.txt`).

## Next

The same census named **8 × 0x505aee46 — RScrlBar — also unhosted**, which is why scrollable
dialogs have no scrollbars. That is the next control, and PO-30's dead filter buttons are the next
campaign-map defect.
