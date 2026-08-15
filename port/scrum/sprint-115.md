# Sprint 115 — "The hardware path draws" (PO-12 phase 3) — ✅ CLOSED 2026-08-15 (goal MET) — ⭐ first hardware-rendered frame

**Planned 2026-08-15 (PO pre-approved ceremonies; continuous-sprint directive). Autonomous. ~8 pts.**
**Sprint Goal:** make `IDirect3DDevice::Execute` read the stream the game writes, and put its
geometry on the screen.

| Story | Pts | Result |
|---|---|---|
| S115-1 walk the execute-buffer opcode stream | 3 | ✅ 13028 streams, 0 unusable, every one EXIT-terminated |
| S115-2 submit its triangles to GL | 3 | ✅ **99.5% screen coverage — the cockpit renders** |
| S115-3 stop leaking one execute buffer per frame | 2 | ✅ real refcounting on `Release()` |

## ⭐ The frame

`port/ref/native/hw_cockpit.png` — canopy frame, windscreen bow, gunsight housing, instrument
coaming, sky and a hazy horizon, all rasterised on the GPU from the game's own execute buffers.
The white surfaces are the ones whose texture is not bound yet (S116).

## What the walk reads

The stream is exactly the DX5 shape (`Win3d.cpp`): a `D3DTLVERTEX` array (pre-transformed screen
space, so `D3DOP_PROCESSVERTICES` is always `_COPY`) followed by `STATERENDER` / `PROCESSVERTICES`
/ `TRIANGLE` / `EXIT`. Each instruction is a 4-byte header plus `wCount` operands of `bSize`, so
unknown opcodes are stepped over rather than aborting the walk — the census keeps working when the
game emits something we do not handle. Every read is bounds-checked against the allocation.

Census of one Hot Shot flight (`MA_D3D_EXEC=1`):

```
Execute calls 13028  (empty 0, unusable 0)
  TRIANGLE 289061   STATERENDER 156880   PROCESSVERTICES 165372   LINE 76224   POINT 5329
  EXIT 7609   <- equals the Execute count: every stream terminated properly
triangles 562909, vertices 1434919, textured 474864
zero-area 2796 (0.5%), wholly off-screen 0 (0.0%), mean area 1735 px
reached GL: 562909 triangles over 1406 scenes
```

## Three faults, each of which alone made the screen black

**1. The blend table was off by one (`bob_video.cpp: gl_blend`).** The game asks for
`SRCBLEND=D3DBLEND_SRCALPHA(5)`, `DESTBLEND=D3DBLEND_INVSRCALPHA(6)`. The table answered
`GL_ONE_MINUS_SRC_ALPHA` and `GL_DST_ALPHA`, so with the opaque alpha the engine actually writes
the source factor was 1−1 = **0**: every triangle was rasterised and then multiplied out of
existence. Entries 4–10 were all shifted; 8 and 11 were missing. **This mapper is shared with the
DX7 path, so `~/bob` inherits the same latent fault** — see the cross-port note.

**2. Texture handles were always 0 (`d3d_execbuf.h`).** S113 stored a non-zero handle on the
`MaD3DTexture` subclass, but none of these interface methods are virtual and the game holds an
`LPDIRECT3DTEXTURE`, so `GetHandle` dispatched statically to the base and returned 0. Zero means
"no texture" to the engine — the census counted 136433 `TEXTUREHANDLE` sets and not one textured
triangle. The handle now lives in the base class. Fixing it changed what the engine emits: textured
triangles went 0 → 474864.

**3. ⭐ `GetWindowRect` was a zero-fill stub (`compat_winuser.h`).** `direct_3d::SetViewParams`
computes the 3D view origin from it:

```c
viewdata.originy = screen_height - window_height/2.0;      // Win3d.cpp:3460
```

With `screen_height` 0 that is **−240**, so the entire hardware-rendered world was generated 240 to
480 pixels *above* the top of the screen. The measurement that named it: **58.1% of all triangles
wholly off-screen — 327172 above, 0 below, 0 left, 0 right.** A uniform offset in one direction is
not a clipping bug or a projection bug; it is a missing origin. After the fix: 0.0% off-screen,
vertex extent y 0..484, coverage 11.3% → **99.5%**.

This is the port's recurring bug class again, and the third time it has cost a sprint: *a stub
returns a plausible-looking zero and quietly reroutes real work somewhere the shipped game never
went.* Same shape as phase 5's back surface sized from a zeroed `GetWindowRect`.

## How the faults were separated

Three of the four diagnostic steps were controls, and two of my three predictions were wrong —
which is the point of running them:

- **whole-framebuffer count at `EndScene`**, not a pixel probe at a vertex. A vertex sits on a
  triangle's edge where the fill rule may exclude it, so the per-vertex readback said "black" for
  reasons that had nothing to do with the bug.
- **an immediate-mode control quad** drawn through the same projection at `EndScene`: it landed
  exactly 10000 px (100×100), which cleared the context, the projection, the thread binding and the
  readback in one run, and pointed the finger at per-draw state.
- **the same triangles replayed in immediate mode**: equally blank, which cleared the client-array
  submission.
- **predicted depth, measured blend.** Then, after the blend fix, predicted depth again for the
  remaining blackness — wrong again; `MA_EXEC_NODEPTH=1` left coverage at 11.3%. What actually
  answered it was the geometry census (mean area 1735 px yet 11.3% coverage → the big triangles
  must be off-screen), and the off-screen breakdown was unambiguous.

## Also fixed

- **The per-frame execute-buffer leak.** The game creates one buffer per frame (S113 measured 9140
  for 9114 frames) and releases it through `WIN3D.H`'s `RELEASE` macro; `Release()` was a no-op
  that returned 0, so a buffer and its allocation leaked every frame. Now refcounted, `delete this`
  at zero.
- **The hardware transform branch is live again.** `MATRIX.CPP body2screen` had `if (false)` from
  the software-only phase. It is now `ma_hardware3d() && mat_win && mat_win->DoingHardware3D()` —
  both original guards kept (mat_win is invalid until `Init3D`), and false in software mode, which
  the parity gate confirms.
- **`ma_ddraw_present` no longer uploads the software framebuffer over a hardware frame**
  (`MA_EXEC_KEEP2D=1` to override and see what 2D still arrives that way).

## Diagnostics left behind (all default-off)

`MA_D3D_EXEC=1` census + per-scene coverage sampling · `MA_D3D_NODRAW=1` walk without submitting ·
`MA_EXEC_NODEPTH=1` · `MA_EXEC_FALSECOLOUR=1|2` (colour by texture handle; =2 colours every batch,
which is how "no geometry" is told from "geometry drawn in the black the engine asked for") ·
`MA_D3D_CONTROL=1` the control quad · `MA_TRACE_GLBIND=1`.

## Gates

parity **5/5 byte-identical** (the `GetWindowRect` change is global — this is the gate that matters
here) · sweep 9 OPEN/0 CRASH · map click · map drag · sysbox exit · help click · overlay text 3/3 ·
stress **20/20** · ASan 0.

## Next (S116)

Bind the textures. The handle→surface map is already built (`ma_d3d_texture_surface`), the surfaces
have real pixels since S113, and the census says 84% of triangles want one. Then the fog/specular
states, `D3DOP_LINE`/`D3DOP_POINT` (76224 and 5329 instructions currently stepped over), and only
then phase 4: the Preferences option with automatic software fallback.
