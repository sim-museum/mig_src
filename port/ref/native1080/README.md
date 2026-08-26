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
