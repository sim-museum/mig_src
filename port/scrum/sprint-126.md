# Sprint 126 — "Not a seam: the map ran out" (B6) — ⚠️ CLOSED PARTIAL 2026-08-15

**Planned 2026-08-15 (PO: proceed). Autonomous. ~8 pts.**
**Sprint Goal:** close the black band and flip the full-resolution canvas on.

| Story | Pts | Result |
|---|---|---|
| S126-1 what is the black band? | 3 | ✅ **the map ends** — not a seam, not a missing tile |
| S126-2 make the map cover the client | 3 | ✅ band gone; map fills 1920×1080 |
| S126-3 flip the default | 2 | ⚠️ flipped, **reverted again** — panning still not lossless |

## ⭐ The band was the map running out

One trace settled three sprints of guessing:

```
[maptile] client=1728x888 zoom=1 tile=256 areax=4 startx=0 endx=4 -> tiles reach x=913
```

The campaign map is **4 tile columns of 256px = 1024px wide** at the startup zoom. The client at
1920×1080 is **1728px**. The tiles legitimately run out at x=913 and nothing paints the rest. Every
tile loads; there is no seam and no missing asset. I had called it "a tile column that isn't drawn"
and then "a ~107px seam" — both wrong, and both would have sent the fix to the wrong place.

## And the engine already had the answer

`CMIGView::Zoom()` contains a block commented *"min zoom for full screen map"* that raises the zoom
until the map covers the window — guarded by `rect.bottom > m_size.cy`. **It only ever checks
height.** The map is 4 tiles across and 7 down, so on the 4:3 displays this shipped for, height was
always the binding constraint and width came free. On 16:9 that reverses: 1787 tall (fine) against
1019 wide (not fine).

Fixed by taking the larger of the two required zooms — identical to the original whenever height
binds, so 4:3 behaviour is unchanged. Two details the measurement forced:

- `+5` on the width requirement, because `m_size.cx = 256*4*zoom - 5` otherwise lands exactly 5px
  short and leaves a hairline down the edge;
- re-clamp `m_scrollpoint` after rescaling, or a scroll from the old zoom leaves the map short of
  the right edge (measured: tiles reached 1541 of 1728 with scroll 187).

**And it had to be called from the right place.** `Zoom()` only runs when the *player* zooms, so the
startup zoom stood however large the window was. I hooked `CMIGView::OnDraw` first — the natural
home — and watched the trace never fire: **this port paints the map from its idle loop and never
calls OnDraw.** The hook belongs in `UpdateBitmaps`, which is what actually draws the tiles.

Result: `tiles reach x=1732` for a 1728 client, scroll clamped to 0, no black runs anywhere, and
the map fills the screen (`port/ref/native/b6_fullres_map.png`).

## ⚠️ The flip is still off, and this is the second veto

With the canvas flipped on, `port/map_drag.sh` still fails: a round-trip pan differs from the
baseline by 820211 px. **The band was not the cause** — S125 measured 108000 px differing with the
flip on and *before* any zoom change, so the full-resolution canvas breaks lossless panning on its
own. The band fix removed one symptom, not the fault.

Standing hypothesis for the next sprint: the canvas is not fully repainted between frames, so
content outside the old 800×600 region goes stale — consistent with S96's blit-overhang clipping,
which was written when the canvas was always art-sized.

The work stays behind `MA_CANVAS_FULLRES=1`. Turning it on would give a campaign map that looks
right and corrupts when you drag it.

## Gates

parity 5/5 byte-identical · map drag PASS (default path) · sweep 9 OPEN/0 CRASH.
