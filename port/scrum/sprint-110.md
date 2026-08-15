# Sprint 110 — "How far does hardware get?" (PO-12) — ⚠️ CLOSED PARTIAL 2026-08-15 — the ladder is measured, four rungs deep

**Planned 2026-08-15 (PO pre-approved ceremonies; continuous-sprint directive). Autonomous. ~8 pts.**
**Sprint Goal:** scope the hardware-graphics option the PO asked for, with measurements instead of a
reading of the headers.

| Story | Pts | Result |
|---|---|---|
| S110-1 make the hardware path reachable for measurement | 3 | ✅ `MA_TRY_HARDWARE=1` |
| S110-2 census: which D3D methods does the game actually drive? | 3 | ✅ five, in a strict order |
| S110-3 name the first real implementation task | 2 | ✅ the execute buffer's memory |

## What the port is starting from

MiG Alley drives **DirectX 5/6 execute buffers** (`IDirect3DDevice::CreateExecuteBuffer` / `Execute`
with `D3DINSTRUCTION` opcode streams) — not DX7 `DrawPrimitive`. The compat layer already recreates
that whole type surface (`SRC/compat/d3d_execbuf.h`, 434 lines) with every method a stub returning
`D3D_OK`. **BoB cannot be copied here:** it is D3D7 + Lib3D software T&L, so its *approach*
cross-ports but its device does not.

## The ladder, measured

`MA_TRACE_D3D=1` counts every compat D3D call; `MA_TRY_HARDWARE=1` makes the path reachable. Each
run named exactly one next requirement — a ladder, not a list:

| # | measurement | what it demanded |
|---|---|---|
| 1 | census showed **one** call: `IDirect3D::EnumDevices` | the stub never invokes the callback, so `bPrimaryDisplayDriverDoesHw3D` stays FALSE, `DD.lpDirect3D` stays NULL and `Display::HardPoly` returns FALSE on its first line |
| 2 | still one call, with a device enumerated | **three separate places force software**: `STUB3D::MakePassive`, the port's own `ma_populate_software_modes`, and — since S103 made preferences load — the persisted `settings.mig` itself. A real option must satisfy all three, and the decision has to be made **before display init** (DDRWINIT chooses whether to construct `Direct3D`) |
| 3 | `IDirect3DDevice::EnumTextureFormats`, then `[SysError] 3D Hardware acceleration is not enabled` | the game requires **two** formats and will not start without them: 8-bit **palettized** (opaque textures) and **16-bit with alpha**, preferring 4-bit alpha = ARGB4444 (transparent textures). Both are loop conditions in `direct_3d::EnumTextureFormats`, so this is a requirement, not a guess |
| 4 | with both formats reported: `CreateMaterial` → `CreateExecuteBuffer` → `IDirect3DExecuteBuffer::Lock` → **SIGSEGV** in `SetInitialRenderStatesLand` ← `CreateLandExecuteBuffer` ← `SelectDriver` | the first stub that needs to be **real**: `CreateExecuteBuffer` hands back a NULL buffer and the game locks it and writes its instruction stream into `lpData` |

## Phase plan (each phase ends at a measurable state)

1. **Execute buffer memory.** `IDirect3DExecuteBuffer` gets a real allocation, `Lock`/`Unlock`
   return it, `GetExecuteData`/`SetExecuteData` track the instruction extent. Ends when the game
   builds its land buffer and calls `Execute` without crashing (drawing nothing).
2. **Opcode walk → GL.** `Execute` interprets the stream: `D3DOP_PROCESSVERTICES` (the game submits
   pre-transformed `D3DTLVERTEX`, so this is a copy, not a transform), `D3DOP_TRIANGLE`,
   `D3DOP_STATERENDER`. Ends with untextured geometry on screen.
3. **Textures.** `RecordTextureUse`/`FlushPTDraw` handles → GL textures; 8-bit palettized expanded
   on upload, ARGB4444 direct. Ends with the terrain and cockpit textured.
4. **The option.** Preferences primary-graphics selector, written the way `SDETAIL` writes the
   resolution (into `Save_Data`), consumed before display init, with an automatic fall back to
   software when the device cannot be created — and the software path kept intact throughout.

## Deliberately not done

No GL device was written this sprint. The measurements above took five runs; guessing at the same
answers would have taken a sprint and produced a device built against assumptions rather than
against what the game demands, in the order it demands it.

## Gates

Not re-run: this sprint's only shipped changes are measurement hooks that are inert unless
`MA_TRY_HARDWARE` / `MA_TRACE_D3D` are set (plus the two software pins, which now read
`if (!getenv("MA_TRY_HARDWARE"))` and are otherwise unchanged). S109's results stand.

## Result

PO-12 goes from "implement hardware graphics" to a four-rung ladder with the first rung named
precisely, plus the two facts that would have cost a sprint each to discover late: **BoB's device
cannot be reused**, and **three separate places decide `fSoftware`**.
