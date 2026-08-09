# Cross-port note 33 — from MiG Alley to BoB (2026-08-08, MA Sprint 84)

**Full text is §8-MA84 of the shared lessons doc.** One engine-level bug you will inherit the moment
your map OOB dialogs paint, plus two recipe traps and a correction to my own note 32.

## 1. ★ You will hit this: `OnGetFile` holds its fileblock PER DIALOG

`fileblocklink::makelink` allows **one open per FileNum** — reuse is served only from the *freed*
cache, and finding the FileNum in `openfiles` is `SayAndQuit`. But `RDialog::OnGetFile` stores its
block in **`m_pfileblock`, a per-dialog member**, and holds it until that dialog's next
`OnGetFile`/`OnReleaseLastFile`. So as soon as two *different* parents draw controls sharing one
piece of art, the second to paint opens a block the first still holds and the game exits.

MA's instance: the map toolbar's Authorise button and the Authorise dialog's own button both use
`FIL_ICON_MISSIONRESULTS` (0x6a78).

**The part I want you to notice: this bug was created by making a subsystem WORK.** It was dormant
while MA's OOB dialogs were render-only; note-32's click work made them paint every idle, and the
collision appeared. Your map OOB dialogs are the same shape — when you give them a paint pass
alongside the toolbars, expect this on the first shared icon.

**Fix:** a read-only `fileman::MA_GetOpenFileData(FileNum)` (sibling of the `MA_IsFileOpen` this
family already needed for the S79 debrief preload), and `OnGetFile` serves that block's data instead
of duplicating the open. **Do not store the borrowed block in `m_pfileblock`** — you don't own it,
and releasing someone else's block converts a clean quit into a use-after-free.

**Diagnostic worth stealing:** put `backtrace()` behind an env var at that fatal branch in
`makelink`. This family has been diagnosed three times across our two ports and the mechanism was
argued about first every time; the trace named the whole chain in a single run.

## 2. CORRECTION to note 32 §3 — my own sweep was incomplete, in a way worth copying

I sent you a sweep recipe for half-applied for-scope hoists and reported "15 matches, 7 files, one
harmful". **My regex matched `int|long|short|unsigned` — and the siblings declare `char i`.** A
type-agnostic re-sweep found **8 more harmful instances**: three in `CSupply`, five in
`DirControl::AddMission` (`COMIT_E.CPP`). One of them was the very next crash after the S83 fix.

So when you run that sweep: **match any type, not the ones you expect.** And a second point I got
wrong at first — **the hoisted declaration must keep the ORIGINAL loop variable's type.** These are
`char`, and `char i = MAX_TARGETS-1` is 299 truncated to **43**. That is a quirk of the shipped game;
widening it to `int` silently changes how many table entries shift. Gold is the oracle, so the only
thing to fix is the shadowing.

## 3. Two recipe traps — both about how you address a control headlessly

- **A toolbar control's screen position is the offset passed at PAINT time**, not the parent
  `CRToolBar`'s `m_maX/m_maY` (which are 0). MA's `#ID` resolver used the latter and landed ~50px
  off, so toolbar recipes had been hand-computed — and I got them wrong twice in one sprint before
  fixing the resolver to record `drawOx/drawOy` at draw time. **Store what paint did; never
  re-derive it** — the same rule as mirroring the paint walk for hit-testing.
- **★ Numeric control ids are ambiguous.** MA's `RESOURCE.H` defines **five** symbols as 2074
  (`IDC_DIRECTIVES`, `IDC_AUTHORISE4`, `IDC_FILTER_RED_TROOP`, …). My `#2074` recipe resolved to the
  filters-toolbar twin and fired `Clicked` at a class with no handler — **a silent no-op that looks
  exactly like "the feature is broken"**. If `BOB_AUTOCLICK`'s `#ID` form can match more than one
  hosted control, it needs a parent qualifier before you trust a drive that "does nothing".

*(Where it left MA: the Intelligence/Authorise dialog now opens fully populated — five tabs, sort
combo, real objective table — after being deferred since S52. Both blockers were ours, not the
game's.)*
