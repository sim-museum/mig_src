# Sprint 111 — "A frame's worth of hardware" (PO-12 phase 1) — ✅ CLOSED 2026-08-15 (goal MET)

**Planned 2026-08-15 (PO pre-approved ceremonies; continuous-sprint directive). Autonomous. ~8 pts.**
**Sprint Goal:** climb S110's first rung — make the DX5/6 execute-buffer machinery real enough that
the game drives a complete `BeginScene → Execute → EndScene` cycle.

| Story | Pts | Result |
|---|---|---|
| S111-1 execute buffer with real memory | 3 | ✅ `IDirect3DExecuteBuffer` owns an allocation; Lock/Unlock/Set/GetExecuteData |
| S111-2 a device the game can obtain | 3 | ✅ the back surface's `QueryInterface` hands one back |
| S111-3 measure the next rung | 2 | ✅ textures — `PrepTexture` ← `CreateTexture` ← `FlushPTDraw` |

## What now happens

The census went from **1** method to **14**, in the game's own order:

```
IDirect3D::EnumDevices → IDirect3DDevice::EnumTextureFormats
IDirect3D::CreateViewport → IDirect3DDevice::AddViewport → IDirect3DViewport::SetViewport
IDirect3DDevice::CreateExecuteBuffer → Initialize → Lock → Unlock → SetExecuteData
IDirect3DDevice::BeginScene → Execute → EndScene
IDirect3D::CreateMaterial
```

**The game is submitting execute buffers.** Nothing renders yet — `Execute` still returns `D3D_OK`
without walking the opcode stream — but the whole submission path in front of it is live.

## The two things that had to become real

1. **`IDirect3DExecuteBuffer` owns memory.** The game asks for a buffer of a given size, `Lock`s it,
   writes its entire instruction stream and vertex array into `lpData`, `Unlock`s and records the
   extents with `SetExecuteData`. With `lpData` NULL it wrote through a null pointer inside
   `SetInitialRenderStatesLand` (S110's measured crash). `Lock` returns the same pointer every time
   and `Unlock` does not free — the game re-locks the same buffer every frame.
2. **The device comes from the BACK SURFACE.** `direct_3d::CreateDevice` does
   `lpDDSBack->QueryInterface(Driver[n].Guid, &lpD3DDevice)`, so the DirectDraw surface has to hand
   one back or `BeginScene` stops the game with *"3D Hardware acceleration is not enabled"*. The
   surface's `QueryInterface` lives in a header included **before** the legacy D3D types exist, so
   it calls `ma_d3d_device()` (new TU `compat/ma_d3d_device.cpp`) rather than naming the type.
   **The new TU is registered in BOTH builders** — `CMakeLists.txt` and `port/rebuild.sh`, the one
   the ASan build uses (the S88 lesson, which cost a sprint the first time).

## Next rung, measured not guessed

```
direct_3d::PrepTexture ← CreateTexture ← RegisterTextureUse ← FlushPTDraw ← EndScene
```

Textures are next: `CreateSurface` for texture surfaces, `QueryInterface(IID_IDirect3DTexture)`,
`GetHandle`, `Load`. After that, phase 3 — the `Execute` opcode walk (`PROCESSVERTICES` is a copy,
since the game submits pre-transformed `D3DTLVERTEX`) — is what finally puts pixels on screen.

## Gates

The hardware path is still opt-in (`MA_TRY_HARDWARE`), so the shipped risk is limited to
`ddraw_legacy.h` (included widely) and one new TU. Verified: **parity 5/5 byte-identical, stress
6/6**. The full set runs with the next sprint that changes a render path.

## Result

Two rungs of S110's ladder in one sprint, and the third named by a backtrace rather than by reading.
The game now builds and submits a frame's worth of hardware commands; making them draw is the next
piece.
