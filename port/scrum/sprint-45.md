# Sprint 45 — "Map colour fidelity" — RESOLVED as a frame-dump bug (not a render bug)

**Context:** "Compare campaign with `~/bob`; finish pinning the fidelity desaturation and fix it." BoB's map
is colour (Wine-match); MA's map *looked* grey/speckled in every headless capture (STATUS S7/S14/S20).

## Root cause — pinned
The strategic map **always rendered in full colour.** The grey/speckle was entirely an artifact of the
`BOB_DUMP_FRAME` diagnostic path:
- Proof chain: (1) a direct dump of the `ma_gdi` screen canvas (`g_canvas`, system memory, BGRA) →
  **vivid colour** (green terrain, blue rivers, red front-lines, sea, highlands). (2) An in-`ma_gl_blit_bgra`
  `glReadPixels` of the framebuffer centre at the map present → **colour** (`fb RGB=(86,94,69)`), matching the
  canvas. (3) All presents during the 2D map are the `gdi-canvas` path (no 3D-fb interleave). So the display
  is colour.
- The bug: `present_dbg` (`bob_video.cpp`) does `glReadPixels(0,0,w,h,GL_RGB,GL_UNSIGNED_BYTE,buf)` then writes
  a PPM with `w*3` bytes/row. Default `GL_PACK_ALIGNMENT = 4` → GL pads each row to a 4-byte multiple. The
  campaign map is **1021 wide**; `1021*3 = 3063` is **not** divisible by 4, so GL emits 3064-byte rows while
  the writer consumes 3063 → **1-byte row drift**, cumulatively rotating R/G/B → channel-shift noise that
  reads as grey-speckle. The front-end (800) and flight (640/800) frames are 4-divisible → always clean,
  which masked the bug and made the map look uniquely broken.

## Fix
`bob_video.cpp` `present_dbg`: `glPixelStorei(GL_PACK_ALIGNMENT, 1)` before the `glReadPixels` calls (the
full-frame `BOB_DUMP_FRAME` read + the `BOB_TRACE_PRESENT` centre read). **2-line change.**

## Validation (headless DoD)
- Re-captured the map via `BOB_DUMP_FRAME` at 1021×644 → **clean full-colour Korean peninsula**, identical to
  the authoritative `g_canvas` dump (chroma 64.4). Before: channel-shift noise.
- No regression: 3D flight capture (`MA_DUMP_BACK`, 640/800-wide, already 4-divisible) still renders.

## Consequence
- **MA's campaign map is at colour parity with BoB** — it was never behind on fidelity; the docs' "greyish
  map" (S7/S14/S20) were all this artifact. Comparison table + roadmap (`campaign-epic.md`) updated.
- **Every future headless visual verification is now pixel-accurate at any width** — this also de-risks the
  remaining campaign UI sprints (date readout, toolbar buttons), which rely on frame capture as their DoD.

**Next (campaign roadmap):** the real remaining gaps are functional — date/period readout, unit-icon verify,
map toolbar buttons (host `CMainToolbar`/`MSCTLBR` `CRButtonCtrl` like BoB S88–92), mission-folder interaction.
