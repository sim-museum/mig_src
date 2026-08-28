# POPULATED 2026-08-27 (S311). The history below is kept — read it before re-blessing.

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

---

## S311 — the placement defect is fixed, and this directory is now populated

The two origins S310 measured have been made to agree. The panel background art is centred by the
game (`[setdib] dst=(320,28) 1280x1024` in a 1920x1080 window); `ma_gdi.cpp` now records that origin
whenever a blit covers at least half the canvas in both axes, and `ma_ole_draw_all` adds it to every
hosted control's position. `MA_NO_PANEL_ORIGIN=1` reverts.

**The safety property is that at 800x600 the art blits at `(0,0)`, so the offset is exactly zero.**
That is not an argument, it is checked: `parity_2d` at 800x600 reports title, prefs_3d, prefs_others
and quickmission **byte-identical**, and campaign_map differing by **5184 px — the pre-existing S248
figure, unchanged**.

It took two attempts, and the first was the instructive one. Offsetting only *client-relative*
controls moved the labels and combos onto the artwork and left the **tab bar** stranded on the black
margin, because `rel` deliberately excludes `CT_LISTBOX` and the tab bar **is** the panel's listbox
(`IDC_RLISTBOX` 2063). Half-fixed looked plausible in the numbers — the label column measured
correctly placed — and was obviously wrong in the picture. The offset now applies to every hosted
control.

Measured, `prefs_others` first label relative to the art's left edge:

| | before | after | 800x600 reference |
|---|---|---|---|
| prefs_others | −230 (outside) | **+90 (inside)** | +50 inside |
| prefs_3d | −226 | **+95** | — |
| quickmission | −156 | **+152** | — |

### What these references are

**PORT-captured REGRESSION references, not gold-parity ones.** `ref/native` came from the real game;
there is no 1920x1080 gold capture of these screens and there never will be. These answer "did this
change?" and cannot answer "is this right?". They were blessed after looking at all five: title
(1280x1024 art centred, menu on the art), prefs_3d, prefs_others, quickmission (labels, combos,
mission text and the Back/Variants/Fly menu all on the art) and campaign_map (drawn to fill,
unaffected by the offset). The gate passes 5/5 byte-identical against them.

### What is now unblocked, and what is not

Flipping `MA_MAXIMIZE` on by default is no longer blocked on a missing reference set. It is still a
change in its own right: the 800x600 recipes pass no `MA_MAXIMIZE`, so a flipped default would
maximise them too and invalidate `ref/native`. That gate must pin `MA_MAXIMIZE=0` first.
