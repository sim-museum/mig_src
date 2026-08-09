# Cross-port note 38 — from MiG Alley to BoB / FreeFalcon (2026-08-09, MA Sprint 97)

**Subject: the `CSystemBox` cluster — and two traps that will hit any port hosting OCX controls.**

Full write-up: **§8-MA97** in the shared lessons doc.

## 1. BoB hosts the same system box — check yours draws exactly once

MA's `CSystemBox` (minimise / resize / **exit**) rendered as three blank buttons, so a play-tester
could not leave the campaign at all. Two separate defects, both likely present in BoB:

**(a) The art named after a control is not necessarily the art *for* that control.** `F_GRAFIX.G`
has `FIL_ICON_THUMBNAIL`, `FIL_ICON_ZOOMIN`, `FIL_ICON_CLOSE1` — named after the three control ids,
and **two of the three are the wrong pictures** (they render as unrelated map glyphs). The gold shot
settles what a button looks like; a name in a header does not. Make the comparison cheap — MA added
`MA_BTN_ART="id=0xNNNN,…"` to override the id→art table at runtime, turning "which of these twelve
file numbers is it?" into minutes rather than a rebuild per candidate. Cross-check the answer
against *behaviour* (`IDC_ZOOMIN` drives `OnGoBig`/`OnGoNormal`, so `FIL_ICON_SCREENSIZE` is right).

**(b) Giving a control art can reveal a bug that was always there.** With art, a **second copy** of
the whole cluster appeared at the top-left and **outlived the campaign, sitting on the title
screen** — `ma_ole_draw_all` had always been drawing those controls at their raw template origin
*in addition to* the parent-scoped draw. With no art it painted nothing, so nobody saw it.

**This is the one I would check in BoB today.** Your map toolbars escape the global draw pass only
because their parent `CDialog` is created **hidden**. That is an accident, not a rule — any hosted
dialog with a *visible* parent that you also composite explicitly is drawing twice right now, and
you will only find out when its controls get art. MA now marks such dialogs explicitly
(`ma_ole_set_parent_scoped(dialog)`) instead of relying on show state.

## 2. A widget must not change the state of the screen it draws on

Drawing the box left a different **GDI font** selected in the screen DC; the campaign map's date
readout, drawn from the same DC later in the frame, inherited it and rendered in a plain sans. The
symptom appeared **top-left, nowhere near the box**. Save/restore the DC's selected objects around
any composite draw.

Also worth stealing, the verification: after the fix, the *only* pixels differing from the previous
reference were the box's own 72×48 rect at its own position. **"Parity still passes" is a much
weaker claim than "the diff is exactly the shape of what I added, and nothing else moved."**

## 3. Your per-screen parity suite has a systematic hole

None of MA's gates caught the ghost cluster. The parity `title` capture is a **clean boot** that
never enters the campaign, so it stayed byte-identical while the title screen was visibly wrong
*after an exit*. It was found by looking at the screenshot of the thing just built.

**Transition states — screen A arrived at from screen B — are untested in a per-screen suite.** If
your suite is built the same way (ours is), the same class of defect is invisible to it. MA has
logged adding the campaign→title exit as a parity screen.
