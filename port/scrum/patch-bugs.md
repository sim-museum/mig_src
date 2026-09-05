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

---

## M5 — is PO-61 a patch-level mismatch?  ⚠️ **PARTIAL: no evidence for it so far**

The patch readme's *"Applying the patch will invalidate all existing savegames and recorded videos"*
made this cheap to test, and the test does **not** support the hypothesis.

**Compared a `.cam` WE wrote (the PO's saved flight, preserved in `scratchpad/po65/`) against four
shipped `Ian*.cam`:**

| file | size | 1st block magic (`0x12345678`) | first 4 bytes |
|---|---|---|---|
| ours | 38,145 | 18,952 | `78 56 34 72` |
| IanAce Kill | 56,442 | 19,064 | `78 56 34 72` |
| IanHead-On Kill | 47,036 | 21,852 | `78 56 34 72` |
| IanLo Alt 1 v 1 Quick | 63,014 | 21,852 | `78 56 34 72` |
| IanLooper Hero | 209,695 | 21,852 | `78 56 34 72` |

**Same super-header ID (`0x72345678`) in ours and theirs, and the same overall structure.** The
differing block-magic offsets are *expected*: `SuperHeaderSize` is **computed by parsing**, not
stored, and the super header holds variable-length lists — three shipped files share 21,852 and one
has 19,064, which is content variation, not versioning. **So no format-version difference is
visible, and MA-P4 should not be assumed to be a format story.**

### What the failure actually is — narrowed

From the PO's session: `GetShapePtr(8036) OUT OF RANGE [0,1023)` ×6 → `LoadItemAnims FAILED`.
Reading `LoadItemAnims`: it reads a **`UWord` uid** from the file, resolves it with
`Persons2::ConvertPtrUID`, and then works on **that object's** anim/shape data. So **8036 is not a
shape number read from the file** — it is the `shape` field of a **wrongly-resolved object**. The
defect is in the **uid → object** step, one level up from where S183's backtrace pointed and where
the guard sits.

**S183's `GetShapePtr` bounds guard is therefore treating a symptom**, and — per `§8-BoB210`, which
landed the same day — it is exactly the kind of guard that can outlive its cause and hide the real
one. It should be re-examined once the uid step is understood, not before.

### Blocked on, and why

Pinning the uid values needs the parse to actually run, and the parse only runs once playback enters
**3D** — which needs a real display. Confirmed headlessly that everything *up to* that point works:
`30,r4;70,#1055:r0;110,#2063:1` reaches `[replay] ReplayLoad: valid file -> Playback=TRUE`.
*(That also settles, positively this time, that the Replay screen's LOAD click routes correctly —
the S203 carried claim that it "does not register" was wrong twice over.)*

**Next:** a 3D run tracing the uid read in `LoadItemAnims` against the objects the super header
reconstructed. If the uids are sane and the objects are missing, the world reconstruction is at
fault; if the uids are garbage, the stream is misaligned earlier than `LoadItemAnims`.

---

# ⚠️ S432 (2026-09-05) — **M1's ANSWER IS OVERTURNED. The source is NOT pre-patch.**

EPIC M is rank 1 on the PO's 2026-09-05 priority ruling, so this file was picked up again. The first
thing the new evidence does is **contradict this document's own headline finding**, so it goes at the
top rather than at the bottom.

## What S212 concluded, and why it was wrong

S212 answered M1 *"our source is PRE-PATCH"* from two negatives: no version string anywhere in
`SRC/`, and no `BDG` reference outside our own compat commentary. Both observations are still true.
**The conclusion drawn from them is not.** The search looked for *version markers* and found none —
but this codebase does not carry version markers. It carries **author-and-date comment tags**, in
their thousands, and those date the tree directly.

## The evidence, three independent strands

**1. A patch fix that names itself as one.** `SRC/H/KEYMAPS.H:390`:

```c
KeyName(281,NEWFLAPSUP)   //New Flap controls for US version and Patch  //CSB 24/08/99
KeyName(282,NEWFLAPSMID)  //New Flap controls for US version and Patch  //CSB 24/08/99
KeyName(283,NEWFLAPSDOWN) //New Flap controls for US version and Patch  //CSB 24/08/99
```

with the bindings at `KEYMAPS.H:910–966` (`shift-f` mid, `shift-r` up, `shift-v` down) and the
implementation at `KEYFLY.CPP:1243–1251`, all tagged `CSB 24/08/99`. **That is the v1.03 changelist
item, verbatim** — *"Flaps - now have Full up mid and down … shift-f half on … shift-r Full on …
shift-v Full off"* — dated three days before v1.03 was compiled (27 August 1999). The comment says
the word **"Patch"** outright.

**2. The tree contains work eight months past the last patch.** Date-tag census over the source:

| year in tag | `//XXX DDMonYY` | `//XXX DD/MM/YY` |
|---|---|---|
| 95 | 156 | — |
| 96 | 4,472 | — |
| 97 | 1,890 | — |
| 98 | 3,403 | 92 |
| 99 | 3,483 | 888 |
| **00** | **37** | **4** |

The 2000 tags run from `AMM 12Jan00` to **`RJS 4Dec00` / `RJS 05Dec00`** (`3D/IMAGEMAP.CPP:677`,
`3D/OVERLAY.CPP:1148`, `H/3DDEFS.H:75`, `H/IMAGEMAP.H:76`, `H/OVERLAY.H:386`) and
`DAW 27Sep00` (`MATH/MATH.CPP:1079`). **V1.23 was compiled 13 April 2000.** So the tree holds
development from at least eight months *after* the final patch.

**3. It is not stale code.** Every file cited above was checked against the real build, not assumed:
`3D/IMAGEMAP.CPP`, `3D/OVERLAY.CPP`, `MATH/MATH.CPP`, `3D/3DCODE.CPP`, `AI/USERMSG.CPP`,
`COMMS/WINMOVE.CPP`, `MOVECODE/KEYFLY.CPP`, `MODEL/GEAR.CPP` — **all compiled**. That check was not
optional: `compile_commands.json` lists only 288 translation units, and a naive lookup says
`USERMSG.CPP`, `GEAR.CPP`, `ENGINE.CPP`, `MODEL.CPP` and `ACMMAN.CPP` are "not compiled". They are.
**MA builds several directories as unity TUs** — `SRC/AI/_AI.CPP` `#include`s `usermsg.cpp`,
`SRC/MODEL/_MODE.CPP` `#include`s `Engine.cpp`, `Gear.cpp`, `Model.cpp`, `Acmman.cpp` and six more —
so the real figure is **382 source files reaching the compiler**, and membership must be resolved
through the unity includes (which themselves go through the lowercase case-variant symlinks).

## ⭐ What this does to EPIC M — it inverts the epic's default

The epic was built on this sentence, which appears in `scrum.md` and at the top of this file:

> *"Every bug those patches fixed in the EXE is, by default, still live in our port — we build the
> pre-patch sources."*

**Delete "by default".** The correct statement is:

> **The source drop post-dates v1.23. A patch fix may or may not be present, and each one must be
> checked individually. Absence is now the surprising outcome, not the expected one.**

Consequences, in order of cost:

1. **M2's triage direction reverses.** The prior was "live unless shown fixed"; it is now "fixed
   unless shown live". Same evidence rule, opposite starting point — and it makes M2 *cheaper*,
   because a positive find (the fix is in the tree, dated) is a grep, not a run.
2. **M0's two celebrated "direct hits" need re-reading.** MA-P4 (*"Random Crashes in the replay"*,
   v1.02) was called *"⭐ a direct hit on PO-61"*. If v1.02's fix is in this tree, PO-61 is a
   **different** replay defect and that link is a coincidence of category, not of cause. Not yet
   checked — filed as the first M2 row below. MA-P7 (Intel font) is unaffected: our font failure was
   stb_truetype rejecting a (3,0) SYMBOL cmap, which is a port-side defect either way.
3. **M4 gets bigger, not smaller.** Parity deviations can no longer be waved at "patch difference"
   as an explanation. The BDG 0.85F oracle is still a *community* patch on top of v1.23, so the
   source-vs-oracle split is real — but it is now a narrower gap than S212 described.

## What is NOT established

* **Not** that the tree is at v1.23 or any specific level. Dated work after a release date does not
  make a tree a superset of that release: this is a source drop off Rowan's own line, not the branch
  the patch installers were built from. Individual v1.2x fixes may still be absent.
* **Not** that any specific patch item is present, except the v1.03 flaps item, which is proven by
  the code above.
* The `//XXX DDMonYY` census counts comment tags, not changes: one edit can add ten tags, and files
  the port has rewritten carry our own tags too. It bounds the tree's **latest** date, which is all
  it is used for here.

## Method note worth keeping

The cheap general test for "is patch item X in this tree" is now: **find the changelist's compile
date, then look for author-dated comments at or just before it in the file the item implies.** The
flaps item took one grep. That is the shape M2 should use.

---

# S432 — M0 completed, and one live defect found

## M0: the readme corpus is now FULLY inventoried, and it is smaller than the "still to inventory" list said

`INSTALL/Mig-Alley_Patch_Win_EN_Patch-123/readme.txt` (872 lines) is **the same English note repeated
in four languages**. The English block is lines 28–246, and its CONTENTS is complete:

| § | content | inventoried |
|---|---|---|
| 1 | Logitech Wingman Force spring setting | env, N/A |
| 2 | Keys / the Controls program | env, N/A |
| 3 | **V1.02 fixes** (9 items) | ✅ S212 — MA-P1…MA-P9 |
| 4 | **Workarounds (V1.1)** (6 items) | ✅ S212 |
| 5 | **V1.03** | ✅ S212 |
| 6 | **V1.1** (Sept–Dec 1999) | ✅ S212 |
| 7 | Multiplayer restricted views | feature note |
| 8 | **V1.23** | ⛔ **was never inventoried — done below** |

⭐ **There are no v1.2, v1.21 or v1.22 changelists at all.** This file's "Still to inventory" line
asked for them; they do not exist in the readme, which jumps from V1.1 straight to V1.23. That line
is now answered and should not be re-opened.

### v1.23 (13 April 2000) — the richest section, and it is nearly all crash classes

| # | Patch text | First-pass implication | Verdict |
|---|---|---|---|
| MA-P10 | Target Lock stutter fixed | Graphics/lock path. | 🔨 triage |
| MA-P11 | TnT+VooDoo2 graphics selection: illegal-mode D3D error | Multi-adapter mode selection. Touches **PO-12** (hardware graphics choice). | 🔨 triage |
| MA-P12 | F51 Speed Indicator accurate to ~500 kt | A concrete numeric claim, checkable in the F51 instrument tables. Cheapest row here. | 🔨 triage |
| MA-P13 | Multiplayer: match/team internet stability; **warping bug**; *"many initialisation problems fixed by making the comms packages smaller"* | ⭐ **MP-2 is rank 6 on the PO's list.** "Initialisation problems" + packet size is exactly the class MP-2 lives in. | 🔨 **triage with MP-2** |
| MA-P14 | Joysticks can use the Z axis as a rudder | Input mapping. Adjacent to PO-53 (axis order), fixed here by another route. | 🔨 triage |
| MA-P15 | ⚠️ *"If you update your graphics hardware with MA installed you must delete `savegame\settings.mig`"* | **Live-looking.** The game persists a graphics selection that survives a hardware change and then breaks. We have a preferences history: **S103** (`InitPreferences` never called), **S206/S209** (layout size tracks neither window nor canvas), and **PO-12**. | 🔨 **triage early** |
| MA-P16 | 'Auto Frame Rate' gains a "fast" option for Ground Stutter | Feature + a named performance symptom. | 🔨 triage |
| MA-P17 | Crack and Burn bug | Unknown symptom; needs the term resolved before it can be triaged. | 🔨 triage |
| MA-P18 | **Crash when selecting trees as targets** | A target-selection crash on a specific object class. | 🔨 triage |
| MA-P19 | **Crash when pressing tab/fire/pause on take-off** | Input during a specific phase. **K10 is "start on the runway and take off"** and is half-done. | 🔨 triage |
| MA-P20 | **Too many radio messages crash** | ⭐ See the defect found below. | 🔨 **in progress** |
| MA-P21 | **Music / audio thread crash** | A threaded-audio crash class; we replaced Miles with OpenAL, so likely N/A — but *likely* is not a verdict. | 🔨 triage |
| MA-P22 | Photo bug generated from DX7 | DX7-specific. Probably N/A. | 🔨 triage |
| MA-P23 | Fixed attacking bug in comms | Comms + attack logic. | 🔨 triage |
| MA-P24 | **Fixed a memory leak in the 3D** | Second 3D leak in the chain (MA-P9 was the 3D→Preferences leak). | 🔨 triage |
| MA-P25 | Fixed bad fuel reporting | Instrument/telemetry correctness. | 🔨 triage |

### A corpus source nobody had listed: `SRC/CHANGES.TXT`

Eleven paths, and nothing else — the **last-changed file list of the source drop itself**:

    AI\USERMSG.CPP   AIRCRAFT\Ai_f86e.cpp   H\Lnchrdat.h   MFC\RESOURCE.H
    MODEL\Acmai.cpp  MODEL\ACMMAN.CPP  MODEL\ACMSIMPL.CPP  MODEL\ENGINE.CPP
    MODEL\GEAR.CPP   MODEL\MODEL.CPP   MOVECODE\Automove.cpp

All eleven exist and **all reach the compiler** (two only via case-variant spellings —
`AIRCRAFT/AI_F86E.CPP` and `H/LNCHRDAT.H`; the `Ai_f86e.cpp` / `Lnchrdat.h` forms in the list are
Windows-case and resolve through the symlink set). ACM + ENGINE + GEAR + MODEL is the **flight-model
and air-combat cluster**, which is what v1.1's "Game Play Issues" (MiGs more aggressive, improved
CAS/Armed Reconn logic) and its "front gear suspension on the F80" item both touch. **Suggestive,
not proof** — it is a file list with no dates and no descriptions, and it is recorded as a lead.

---

## M2 — first row worked: MA-P20 *"Too many radio messages crash"*

**Two real defects in the radio decision table. Neither is proven to be the patch's bug; both are
defects on their own evidence.**

### The table and its constructor

`SRC/H/AI.H:41–45`:

```c
enum { OPTIONTABLEMAX = 100 };
static DecisionAI* optiontable[OPTIONTABLEMAX];
static int optiontablemax;
DecisionAI() { optiontable[optionnumber = ++optiontablemax] = this; }
```

**Defect A — pre-increment with no bound.** The counter is incremented *before* it is used as an
index, so the first object lands at `[1]` and **`optiontable[0]` is NULL for the life of the
process**. There is no bounds check at all: the 100th construction writes `optiontable[100]`, one
past the end of a 100-element array, and the 101st and beyond keep going.

**How close is it? MEASURED, not guessed.** Objects are created one per `INSTANCEAI(...)` macro:

| file | live `INSTANCEAI` | commented out |
|---|---|---|
| `SRC/AI/USERMSG.CPP` | **50** | 6 |
| `SRC/AI/SPOTTED.CPP` | **21** | 1 |

**71 of 100 slots used.** So Defect A is **LATENT** — 29 spare, and index 100 is never reached
today. It is a trip-wire for anyone adding radio options, which is exactly what EPIC J/PO-7 work
would do. Recording the number rather than the adjective: *71*, not *"close to the limit"*.

### ⭐ Defect B — the multiplayer path indexes that table from the wire, unchecked

`SRC/COMMS/WINMOVE.CPP:8365–8372`, inside **`DPlay::ProcessWingmanCommand`** — a DirectPlay packet
handler:

```c
decision = id2 & 0x7f;          /* 0..127, straight off the network */
option   = id2 >> 7;            /* unbounded */
DecisionAI* dec = DecisionAI::optiontable[decision];
DecisionAI::OptionRef* opt = dec->GetMsgOptions();
opt += option;                  /* unbounded pointer add */
```

Against a 100-entry table holding 71 live pointers at indices 1..71:

| `decision` | what happens |
|---|---|
| 0 | **guaranteed NULL** (Defect A never fills slot 0) → virtual call on NULL |
| 1..71 | valid |
| 72..99 | NULL (static init) → virtual call on NULL |
| 100..127 | **out of bounds read**, then a virtual call through whatever was there |

and `option` is added to the options pointer with no bound in every one of those cases.

**The sender is the same file** (`DPlay::NewWingmanCommand`): `id2 = (option<<7) + decision`, where
`decision` is a `UByte`. So a sender with `decision >= 128` silently corrupts the option field too —
also unchecked, though unreachable today at 71 options.

### Honest limits on this finding

* **NOT PROVEN to be MA-P20.** *"Too many radio messages crash"* could equally be a message-queue
  overflow elsewhere. What is proven is that this path has an unchecked network-controlled index
  into a fixed table followed by a virtual call.
* **NOT REACHED in a run here.** Between two matched builds every `decision` sent is a real
  `optionnumber` in 1..71, so a healthy pair never trips it. It needs a corrupted packet or
  **mismatched peers** — and mismatched peers is precisely the situation MP-2 is about.
* Per this file's own rule 1, the verdict comes from the code read, and the *reachability* claim is
  marked open rather than rounded up.

**Next, and it is cheap:** bound both sites (`decision > optiontablemax || optiontable[decision]==NULL`
→ drop the packet; `option` against the decision's own option count), then re-run the MP connect
gate. It is additive and cannot change single-player behaviour. Do it **with MP-2** (rank 6), not
before it, so the two share one multiplayer run.
