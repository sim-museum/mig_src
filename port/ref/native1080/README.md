# Intentionally empty — do NOT populate from the current build

S302 created this directory for a 1920x1080 reference set (PARITY_RES=1080 in port/parity_2d.sh),
then captured the five screens and LOOKED AT THEM before blessing any as a reference. Two of them
are wrong, so committing these captures would enshrine a broken layout as the expected result.

- `title` at 1920x1080 with MA_MAXIMIZE=1 is CORRECT: 1280x1024 artwork centred, menu placed
  properly, dark margins. This is what PO-67 was aiming at.
- `prefs_others` is BROKEN: labels overlap and double up ("Music Volume" over "Master Volume",
  "Radio Chatter Volume" over "Gamma Correction"), the control column is squashed to the left and
  the art panel is offset.

So the S290 maximise fix is PARTIAL: it repairs the full-pane menu screens and breaks the dialog
screens, which position their controls with layout data that the widened container does not carry.

**Populate this directory only after the dialog layout is fixed at this resolution**, and say in the
commit that these are PORT-captured REGRESSION references, not gold-parity ones -- there is no
1920x1080 gold capture of these screens and there never will be.

---

## S310 (2026-08-27) — re-checked after the PO-67 dialog fix. Still do not populate.

S304 fixed the panel→panel canvas clear, and STATUS records "`MA_MAXIMIZE=1` no longer breaks the
dialogs". That satisfies half of what this file was waiting for, so the five screens were captured
again at 1920x1080 and looked at.

**The doubling is genuinely gone.** `prefs_others` now shows its own ten labels — Music/3d SFX/
Control SFX/Ambient SFX/Radio Chatter/Engine Volume, G Effects, Injury Effects, White Outs, Auto
Vectoring — with no 3D-tab leftovers underneath. That part of the S302 note is resolved.

**The placement is not.** S302 also said "the control column is squashed to the left and the art
panel is offset", and that is still true. Measured:

| | art panel | first label | label vs art edge |
|---|---|---|---|
| `prefs_others` @800x600 (gold-derived ref) | x 0..799, fills the frame | x 50 | **+50 inside** |
| `prefs_others` @1920x1080 | x 320..1558 | x 90 | **−230 OUTSIDE** |
| `prefs_3d` @1920x1080 | x 320..1599 | x 94 | −226 |
| `quickmission` @1920x1080 | x 320..1599 | x 164 | −156 |

The label column lands on the black margin, to the left of the artwork it belongs on.

### Where the two disagree

The art is centred correctly: every panel screen composites 1280-wide art at **x = 320**, which is
exactly `(1920 − 1280) / 2`. The controls do not follow it. `ma_ole_draw_all` positions each hosted
control at `px = rel ? parent->m_maX : 0` (`SRC/compat/ma_olecontrol.cpp`, the draw pass STATUS
already nominated as the suspect), and the labels imply `parent->m_maX ≈ 40` — the parent dialog's
own origin, set by the game's `MoveWindow`, not by where its art was composited.

So this is not "the container is too narrow". **The art panel and the dialog window are positioned
from different bases**, and at 800x600 the difference is invisible because the art fills the frame
and the centring offset is zero. That is why every 800x600 capture is byte-identical whatever this
code does, and why the defect only appears at a resolution where the art is smaller than the screen.

`campaign_map` is unaffected (97.6% coverage, drawn procedurally to fill) and `title` is correct
(62.8% coverage, matching STATUS's figure) — it has no hosted control column to misplace.

**Precondition still unmet.** Populating now would enshrine a layout with the labels off the
artwork, which is exactly what the original note above forbids.
