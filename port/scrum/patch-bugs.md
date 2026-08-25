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
