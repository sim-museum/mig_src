# Sprint 113 — "Textures, and a frame that survives" (PO-12 phase 2) — ✅ CLOSED 2026-08-15 (goal MET)

**Planned 2026-08-15 (PO pre-approved ceremonies; continuous-sprint directive). Autonomous. ~8 pts.**
**Sprint Goal:** clear S111's next rung — the texture path — so the hardware renderer survives a
whole flight instead of crashing in `EndScene`.

| Story | Pts | Result |
|---|---|---|
| S113-1 real memory behind a texture surface | 4 | ✅ the DX2 face shares the DX1 surface's pixels |
| S113-2 the three interface faces of one surface | 3 | ✅ QueryInterface dispatches on the IID |
| S113-3 measure | 1 | ✅ **9114 BeginScene/EndScene cycles, no crash** |

## What was wrong

`direct_3d::CreateTexture` builds a texture as a chain of faces over ONE allocation:

```
lpDD2->CreateSurface(&tmsd, &lpDD1TSurf)                 DX1 surface (has pixels)
lpDD1TSurf->QueryInterface(IID_IDirectDrawSurface2, …)   its DX2 face
lpDD2TSurf->QueryInterface(IID_IDirect3DTexture, …)      the texture object
PrepTexture(lpDD2TSurf, …)  ->  lpDD2TSurf->Lock(…)      writes texels into lpSurface
```

The DX2 surface was a pure stub: `Lock` returned `DD_OK` without touching the descriptor, so
`PrepTexture` wrote the texture through whatever `lpSurface` happened to contain — S111's measured
SIGSEGV. Worse, S111's shortcut had the DX1 surface return the **3D device** for *any*
`QueryInterface`, so the texture path would have been handed a device and written texels through it.

## Fix

- `IDirectDrawSurface2` becomes a **view**: it borrows the DX1 surface's `sbits/pitch/dims` and its
  `Lock` fills the descriptor with them. It does not own the pixels — the DX1 surface does.
- `IDirectDrawSurface::QueryInterface` now **dispatches on the IID**: `IID_IDirectDrawSurface2` →
  the (lazily created, cached) DX2 face; `IID_IDirect3DTexture` → a texture object bound to that
  face, handing out **non-zero** handles (zero means "no texture" to the game); the driver GUID →
  the one 3D device. Anything else stays NULL, which is what an unimplemented interface should look
  like.

## Measurement

A full Hot Shot flight under `MA_TRY_HARDWARE=1`, to the harness timeout, no crash:

```
IDirect3DDevice::BeginScene            9114
IDirect3DDevice::EndScene              9114
IDirect3DDevice::CreateExecuteBuffer   9140
IDirect3DTexture::Load / GetHandle     (first calls reached)
16 distinct methods called
```

**The hardware path now runs a whole mission.** It still draws nothing: `Execute` returns `D3D_OK`
without walking the opcode stream, which is phase 3 and the piece that finally puts pixels on
screen.

Note for phase 3: `CreateExecuteBuffer` is called **once per frame** (9140 buffers for 9114 frames),
so the buffer objects are currently leaked at that rate — the game releases them through `RELEASE()`,
which is a no-op in the compat layer. Not a problem for a measurement run; it must be real before
the option ships.

## Gates

Hardware stays opt-in (`MA_TRY_HARDWARE`), so the shipped surface is `ddraw_legacy.h` +
`d3d_execbuf.h`. Run with the next render-path change; S112's results stand.
