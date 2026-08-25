# Patch- and documentation-sourced bugs — do we still have them?

*Opened S212 (2026-08-25), EPIC M. PO: "check the ~/sgl/TUE patch changelists … and check whether
any bug fixes listed in these patch changelists are bugs that need to be fixed in the ma or bob
linux codebase" + "do the same with any bugs mentioned in … documentation, either that distributed
with the games or provided later by the user communities".*

---

## M1 — what patch level is our SOURCE?  ✅ **ANSWERED: PRE-PATCH**

This is the story everything else was gated on, because it is the difference between a long list and
an empty one.

**Evidence (MA):**
- No version marker anywhere in `SRC/` (`grep` for `1.2x` / `Version 1.0` in `MIG.CPP`: nothing).
- No `BDG` reference in any game source; every hit is in **our own** `SRC/compat/` commentary.
- The port already compensates for source-vs-installed differences case by case — the clearest
  statement being S57's oracle ruling, that the gold shots are the **BDG 0.85F patched build** and
  resources must be read from the *installed* `English/TEXT/miglang.dll`, not the source tree.

**Evidence (BoB, same answer — see `~/bob` Release P):**
- Every `BDG` mention in that tree is likewise our own comment.
- `SRC/RLISTBOX/bob_ole.cpp:265` names the split outright: source-only dialogs drawn under *"the BDG
  `IDD_SSOUND` layout"* taken from the installed PE.

### ⭐ What that means, and it cuts two ways

> **We compile the PRE-PATCH source. The parity oracle is a PATCHED binary.**

1. **Every bug the patches fixed in the EXE is, by default, still live in our port** — we have never
   had those fixes, and nobody has looked for them.
2. **Some recorded "parity deviations" may be patch differences, not port defects.** That would
   *revise* verdicts in `screen-parity.md` rather than add work — and a mis-attributed oracle is
   worse than no oracle.

---

## M0 — the fix list, verbatim, with a first-pass implication

Source: `~/sgl/TUE/MigAlley/INSTALL/Mig-Alley_Patch_Win_EN_Patch-123/readme.txt`
(official Rowan chain, v1.01 → v1.23; the installed build's own history block).

### v1.02 — "Fixes:-"

| # | Patch text | First-pass implication for the port | Verdict |
|---|---|---|---|
| MA-P1 | AWE64 Sound FX missing | Hardware-specific (SB AWE64). Almost certainly N/A — we map to OpenAL. | 🔨 triage |
| MA-P2 | Random Crashes in the 3D [Audio triggers] | **Live candidate.** A crash class in 3D triggered by audio. We have never audited for it. | 🔨 triage |
| MA-P3 | Smoke trails going jagged | Visual defect; would show as a parity/behaviour difference we might have blamed on ourselves. | 🔨 triage |
| MA-P4 | **Random Crashes in the replay [audio and accel]** | ⭐ **Direct hit on PO-61.** A *known* replay crash class, fixed in the patch, tied to audio and time-acceleration. Our `.cam` load fails at `LoadItemAnims` with `GetShapePtr(8036) OUT OF RANGE`. | 🔨 **triage first** |
| MA-P5 | Rcombo Crashes [mainly on the Preferences Screen] | R\* control crash class. We host `RCombo` ourselves; worth a targeted look. | 🔨 triage |
| MA-P6 | Missing dialogs if Windows Font is 110% or 200% | DPI-scaling dialog loss. **We have a live scaling story** (S206/S209 window vs canvas vs usable bounds). | 🔨 triage |
| MA-P7 | **Improved font if 'Intel' font not installed** | ⭐ **We rediscovered this from scratch in S66** — the game ships `Intel.ttf` and stb_truetype rejected it over a (3,0) SYMBOL cmap, so the port fell back to DejaVu for ten sprints. The patch had already addressed the same font's fragility. | 🔨 triage |
| MA-P8 | USB Joysticks cooperating with other USB devices | Input enumeration. Related in spirit to **PO-53** (axis order), already fixed here by a different route. | 🔨 triage |
| MA-P9 | Memory leak going from the 3D to the preferences screen | A leak on a transition we drive constantly in gates. | 🔨 triage |

### v1.03

Explosion colour in the software driver; Photo Zoom jitter; Target Diamond in software-driver mode;
`Ctrl-5`/Enter instrument glance; map work (waypoint info, mid-level and hi-detail map); *"removed
troops from inside the hill on CAS UN attacking mission"*; flaps get full/half/up positions.
→ Mostly **feature/data**, but the **software-driver** items matter: this port forces `fSoftware`
in places (S102 found text drawing rerouted by exactly that), so software-path fixes are relevant.

### v1.1 (Sept–Dec 1999)

*"Stuttering in Multiplayer squashed"*; ⭐ ***"The Sticky key problem has now been fixed"***;
navigation additions (mid-level zoom, bearing/range to waypoint, extra cockpit info line).
→ **Sticky keys** is a live candidate: this port has a documented history of keyboard-state defects
(**PO-60** focus loss, **S202** the sim never releasing the keyboard).

### Workarounds (v1.1) — environmental, but revealing

Voodoo2/TNT2 resolution advice; *"No 'stencil' style font on the front screens: Reinstall
Intel.ttf"* → the **same font fragility as MA-P7**, from the other direction.

---

## Still to inventory

- `DOC/MigAlleyTips.pdf`, `DOC/CampaignGraphicsWorkarounds.pdf` (**title suggests known graphics
  defects + workarounds** — high value), `DOC/MigAlleyLinks.html`, `DOC/communityDoc/`,
  `DOC/REFERENCE/`, `DOC/dogfightingSummary.md`.
- v1.2 / v1.21 / v1.22 / v1.23 sections of the readme.
- BoB's corpus is tracked in `~/bob` under Release P — `DEBUG/THU_graphics_glitches.txt` first.

## Rules for triage (M2)

1. **A verdict comes from evidence** — a grep, a run, a `git log -L` — never from the patch text.
2. **Data-only patch items are N/A to a source port** and must be marked so, not left ambiguous.
3. **A patch item that matches a bug we already fixed independently is still worth recording** — it
   tells us the list is predictive, which is the argument for working the rest of it.

---

## M0 (continued) — documentation-sourced items

### `DOC/CampaignGraphicsWorkarounds.pdf` — community, Linux/Wine

Written about running MA under **Wine**, not about this port — so nothing in it is automatically
ours. It is included because it records *game* behaviours observed by people who played it hard,
and two of them land squarely on open work here.

| # | Observed behaviour (community) | Why it matters to the native port | Verdict |
|---|---|---|---|
| MA-D1 | *"in the 3D world Mig Alley produces two displays, a 3D view and a black box"*, and with one monitor the black box covers the 3D view | The **game itself** wants a second surface in 3D. We have a live window/canvas story (S206 layout-vs-canvas, S209 usable bounds, S209b the click mapping) and have been treating every oddity as ours. This says part of the geometry weirdness is the game's own design. | 🔨 triage |
| MA-D2 | *"Mig Alley can change all these resolution settings without your knowledge"* — the game rewrites its own resolution preferences | Directly relevant to **S103** (`InitPreferences` was never called, so prefs never loaded) and **S206** (the layout picker reads a size that tracks neither window nor canvas). If the game mutates its own resolution state, a port that reads it must expect it to move. | 🔨 triage |
| MA-D3 | ⭐ *"When in the 2D view with 3D graphics settings, icons are likely to disappear"* — recovered by clicking **Size** (adjust the campaign canvas) or **Hide/Reveal Toolbars**, i.e. **by forcing a refresh** | **A strong match for our own history.** S109 found 30 campaign-map filter buttons *drawn blank*; PO-11 inventoried the same family. The community's workaround — *a canvas resize restores the icons* — says the icons are **drawn once and not re-drawn**, and that a resize forces the redraw. If our map has the same draw-once behaviour, this is an invalidation bug with a known trigger and a known cure. | 🔨 **triage first of the three** |

**Method note.** A Wine-workaround document is *not* a bug list for this port, and must not be
triaged as if it were: every row above needs the same evidence rule as the patch items. What makes
it worth reading is that the authors were describing the **game**, and the game is what we compile.
