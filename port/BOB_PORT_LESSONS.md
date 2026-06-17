# Porting a Rowan Engine Game to Native Linux — Field Notes

**From:** the *Battle of Britain* (Rowan, 2000) native-Linux port (a DX7/Lib3D title).
**To:** whoever is porting *MiG Alley* (Rowan, 1999) — same studio, related engine *family*
(shared Win32/MFC/DirectX framework + RDialog UI + ILBM assets; **different, earlier
renderer** — see the box below).

This is hard-won knowledge so you don't re-derive it. BoB and MiG Alley are **closely
related but NOT the same engine** (recon below): they share the Win32/MFC/DirectX **framework**,
the **RDialog** front-end UI toolkit, the **ILBM/IFF** asset format, the build/ABI model, and
the whole *porting methodology* — but MiG Alley (1999) is an **earlier Rowan renderer
generation** than BoB (2000). **Read the "MiG Alley specifics" box immediately below before
anything else — it tells you which parts of this doc transfer verbatim and which are
BoB-renderer-specific.** Game *logic* (flight model, Korea vs the Channel, campaign rules,
jets) is irrelevant to the port — you never touch it.

Everything below is labelled **[ENGINE]** (the Rowan engine layer) or **[GAME]** (BoB-
specific). Treat **[ENGINE]** as "shared *framework*"; the DX7/Lib3D *renderer* internals
(§3–4) are BoB-specific — see the box. When in doubt, the BoB repo's `PORT.md` is the full
dated log with evidence; this is the distilled version.

---

## ★ MiG Alley specifics — READ THIS FIRST (tailored from `~/ma` recon)

You're further along than this doc assumes: **Phase 1–2 done** (15/15 game unities compile,
`wmig` links with 0 undefined symbols, boots into `CMIGApp::Run()`), and you're in **Phase 3
first-frame bring-up** with the `ma_ddraw_present` software-framebuffer→GL bridge. Your
`CLAUDE.md` + `port/PORTING.md` lead this doc on the foundation — don't let me re-teach that.

**Engine-generation difference (the load-bearing correction).** `~/ma/SRC` has **no Lib3D**,
no `_CreateTextureMap`, no `RENDERTARGET_*`, no D3D7 `DrawPrimitive`. MiG Alley renders with a
**software rasterizer** — `SRC/GRAPHICS/{GRAFPRIM,POLYJIM,POLYGON}.CPP` + the `*.ASM`
primitives, and `SRC/HARDWARE/{HARD320 (VGA mode-X), HARDVBE1 (VESA), HARDPRIM}.CPP` — drawing
into a **palettized 8-bit / 16-bit-565 framebuffer**, with an *optional* **Direct3D 5/6
execute-buffer** hardware path in `SRC/HARDWARE/{WIN3D,HARDWIN}.CPP`. BoB, by contrast, is
**D3D7 hardware-only with Lib3D software-T&L**. So:

| This doc's section | For MiG Alley |
|---|---|
| §1 strategy, §2 pack boundary, §5 input, §6 audio, §7 validation, §8 effort map | **Transfers directly.** |
| **RDialog front-end** (§3 last para) | **Yours verbatim** — same `R*` toolkit (`SRC/{RBUTTON,RCOMBO,RTABS,RLISTBOX,RSPINBUT,RTICKBOX,RTITLE,RTREE,RANIBUT,RRADIO,RSCRLBAR,REDIT,RSTATIC}`), same bitmap fonts, paints through your DirectDraw-2D software-fb bridge, **not** GDI. You already found this (the `RFullPanelDial`/FullScreen title over DirectDraw 2D). |
| **ILBM assets** (§3) | Transfers — `SRC/GRAPHICS/LBM.CPP` is the same `FORM/BMHD/CMAP/BODY` IFF family as BoB `.shp` (palette-indexed, RLE). |
| **§3 Lib3D internals, §4 DX7→GL backend + bug catalogue** | **BoB-renderer-specific — do NOT grep for these.** No `_CreateTextureMap`/`UpdateMipMaps`/render-targets in your tree. Read them as *the class of bug to expect in your own renderer*, not shared code. |

**What still bites you, mapped to your tree:**
- **§2 pack boundary** — you build `-fpack-struct=1` (same as BoB). It bit BoB **3×** by
  leaking into libc/libstdc++ types. Wall it off systematically and add `sizeof`/offset
  asserts; suspect it on any non-local corruption.
- **§4.1 COM refcount use-after-free** — won't be `_CreateTextureMap`, but the *pattern*
  (`AddRef` no-op + `Release` frees on first call) will bite the moment you back a DirectDraw/
  D3D COM object (your `IDirectDrawSurface`/`IDirectDraw2` in `ddraw_legacy.h`) with a real
  allocation and the engine does a balanced AddRef/Release. **Give surfaces a real `int ref`
  now**, before you trust them.
- **§5/§6 input & audio** — liftable from `~/bob/SRC/compat`: DInput→SDL (keyboard DIK
  scancodes done in BoB), Miles/DSound→OpenAL (your `miles_ail_stub.cpp` is silent today),
  case-insensitive `fopen`, threads, PE resources.
- **§7 wine-oracle + pixel-truth** — *more* useful for you, because your renderer is software:
  dump the palettized/16-bit framebuffer and A/B against `mig.exe` under wine, screen by screen.

**Your "black world" risk is different from BoB's.** BoB's black ground is a **D3D
render-to-texture** gap (needs FBO). Yours is the **software-rasterizer ASM** you've stubbed
(`XASM_*`, `ASM_PlotPixel` in `GRAFPRIM`, plus `POLYJIM`/`HARD320`): those plot pixels
*directly* into the palettized framebuffer. The world stays black until those primitives (or C
equivalents) actually write to your software fb — wiring/porting the rasterizer, **not**
implementing FBO RTT. Same "black until pixels land" shape, completely different fix. Decide
early: port the x86 ASM primitives, or take the D3D5/6 execute-buffer hardware path
(`WIN3D`/`HARDWIN`) and back it with GL — the software path is likely the faster route to a
faithful first frame given your 2D bridge already exists.

---

---

## 0. The one-paragraph version

Build the **unmodified game source** 32-bit (`-m32`) and provide a **compatibility layer**
that maps Win32/MFC/DX7 → SDL2 + OpenGL + OpenAL under `#if FF_LINUX`/`BOB_LINUX`. Do **not**
edit game logic — implement the *Windows APIs* it calls. The two things that will eat weeks
if you don't know them up front: (1) a **`-fpack-struct=1` ABI boundary** bug class that
corrupts libc/libstdc++ types, and (2) a **DirectDraw surface reference-counting** bug that
frees live textures. Both are below with fixes. Use the **original `.exe` under wine as your
pixel oracle** for every screen.

---

## 1. Strategy & architecture (the approach that works)

- **[ENGINE] Faithful compat, not game edits.** Keep `SRC/*` byte-faithful. Implement
  Win32/DDraw/D3D7/DInput/DSound/MFC as SDL/GL/OpenAL/pthread-backed shims. This is the
  whole game: you are writing a tiny DX7 + Win32 + MFC implementation, not modifying Rowan's
  code. Gate every Linux-ism behind `#if BOB_LINUX` (pick your own macro).
- **[ENGINE] Drive the game's NATURAL flow; don't cold-start subsystems.** The engine's
  init is deeply ordered and interdependent (campaign → packages → squadrons → persons →
  nodes → battlefields). Cherry-picking init calls to "boot straight into a mission" hits an
  endless wall of uninitialised-state crashes. Get the **front-end menu** driving init in
  order instead. (We built a `BOB_BOOT_FRONTEND` quick-mission cold-start probe for 3D
  bring-up — useful as a *crutch*, but the real game must run through the menus.)
- **[ENGINE] Iterate against the real install**, fixing at the *first real failure*. Keep
  default `./game` clean (exit 0); gate all diagnostics/experiments behind env vars,
  default-off.
- **[ENGINE] Sources are ISO-8859 / mixed encoding** — always `grep -a` or grep barfs.
- **[ENGINE] Two DDraw versions coexist:** Lib3D uses **DDraw7/D3D7** (the live 3D path);
  legacy hardware-probe code may reference **DDraw2**. Don't confuse them.

### Compat layer file map (BoB — mirror this structure)
```
SRC/compat/
  bob_video.cpp      # THE big one: DDraw7/D3D7 -> SDL2/OpenGL (surfaces, device, present, RTT)
                     #              + DirectInput device objects
  bob_stubs.cpp      # DirectSoundCreate, file/path helpers (case-insensitive fopen!),
                     #   _findfirst/_findnext, AfxGetApp/MainWnd, misc Win32
  bob_main.cpp       # entry, WinMain bridge
  bob_threads.cpp    # CreateThread/events/critical-sections -> pthreads
  bob_resources.cpp  # PE resource loader (reads the game's own .dll/.exe resources, e.g. boblang.dll)
  cstring_impl.cpp   # MFC CString
  *.h                # huge set of Win32/MFC/DX header shims:
                     #   ddraw.h d3d.h d3dtypes.h dinput.h dsound.h openal_dsound.h
                     #   afxwin.h (47k lines!) compat_winuser.h compat_winbase.h wingdi.h
                     #   compat_types.h  (<- pack-boundary lives here; see §2)
```
Header shims are mostly type/decl scaffolding lifted from the DX7 SDK / MFC, trimmed to what
the game references. The *implementations* live in the `.cpp` files. MiG Alley references a
very similar API surface — you can lift most of these headers wholesale.

---

## 2. Toolchain & the #1 recurring bug: the pack-struct ABI boundary **[ENGINE]**

- Build **32-bit**: `-m32 -fno-pie` (+ `-no-pie` link). Result is `ELF 32-bit i386`. This
  matters: `unsigned long` is **32-bit** on Linux i386, **matching Win32 (LLP64)**. So game
  structs, pointer-as-`ULong` casts, and IFF/binary parsing behave identically to Windows.
  A 64-bit build would break pervasively. Keep it 32-bit.
- The game is compiled with **`-fpack-struct=1`** (equivalent of MSVC `/Zp1`) because Rowan's
  on-disk/in-memory structs assume 1-byte packing. **This pragma MUST NOT reach libc or
  libstdc++ types.** If a `std::ifstream`, `std::locale`, `struct stat`, etc. is compiled
  packed, its subobject layout mismatches the system library → the library scribbles memory
  → spooky non-local corruption (stack smashes, `EE E9` heap markers, crashes far from the
  cause).
- **This bug class recurred at least 3× in BoB** (Lib3D fstream, `struct stat`, a `BIStream`
  save-file stream) before being fixed *systematically*. The fix: wrap every system/std
  include in `#pragma pack(push,8) ... #pragma pack(pop)` so they keep the native ABI despite
  the global `-fpack-struct`. We centralised this in the shim headers (`iostream.h`/
  `fstream.h` shims + the raw `<sstream>/<fstream>/<iostream>` includes in
  `compat_types.h`/`wtypes.h`). **Do this once, up front, for ALL system includes** — don't
  patch reactively. Verify with `static_assert(sizeof(std::ifstream)==...)`-style offset
  checks at a couple of known types.
- **Symptom to watch for in MiG Alley:** any crash that looks like memory corruption with no
  obvious local cause, especially around file I/O, streams, or anything touching libc/libstdc++.
  Suspect a packed std/libc type first.

---

## 3. The Rowan engine internals you need to know **[ENGINE]**

### Lib3D — software transform & lighting
- The engine runs **software T&L** (`NO_HARD_TNL`): it transforms and lights vertices on the
  CPU and submits **pre-transformed `XYZRHW`** vertices to the device. The hardware
  lighting/T&L path is dead.
- **Lighting AND fog are baked into the vertex `DIFFUSE` colour** by Lib3D. The device just
  does `MODULATE(texture, diffuse)` at stage 0. Consequences:
  - There is **no GL lighting to implement** — just multiply texture × vertex colour.
  - "Dark cockpit", "hazy distance" etc. are *baked into diffuse* and usually correct — not a
    bug in your backend. Don't try to "fix lighting" with GL lights; you'll double it.
  - `SPECULAR` is always 0 (no specular carrier; fog isn't carried in specular either).
  - **[GAME]** BoB had a "night" bug: the cold-start boot probe used a pre-dawn mission time,
    so `SetLighting` baked night into every vertex. That's a *game-state* issue (wrong time),
    not a renderer issue. MiG Alley: if a scene is too dark/bright, check the sim's time/
    weather state before touching the renderer.
- Text/2D overlay uses **`COverlay`** (loader screen, cockpit instruments, HUD, menus draw
  through related 2D paths). The cockpit instrument panel, gunsight reticle, HUD, and compass
  are 2D/overlay + textured geometry — *not* render-to-texture.

### The THREE render targets (this is the big architectural gotcha)
Lib3D has exactly three RTT targets — check `HRENDERTARGET::getType()`:
1. **`RENDERTARGET_PRIMARY`** — the back buffer (the normal scene). Works fine.
2. **`RENDERTARGET_MIRROR`** — the rear-view mirror (`RenderMirror`).
3. **`RENDERTARGET_LANDSCAPE`** — **landscape detail-tile compositing** (`TILEMAKE` /
   `AllocateLandscapeTextures`). The terrain *base* imagemaps load from disk and render fine;
   the per-tile **detail** is *composited at runtime into texture tiles via render-to-texture*.

**Why this bites you:** a pure software/GL backend with no FBO can't render-to-texture. The
engine's designed fallback (`SetNoRenderToTexture`) points the land/mirror render target at
the **back buffer** (`pDDS7LandRT = pDDSB7`) and then **copies** the rendered region into the
tile texture (`UploadTexture` → `Blt`/`PerformSlowCopy`). But — see §4 — your 3D render goes
to the **GL framebuffer**, while that copy reads the back-buffer's **system-memory bits**,
which the 3D path never touched → the tiles come out **black**. In BoB this was the
"black airfield ground" bug.

**FIXED in BoB (2026-06-16) — real FBO render-to-texture.** Gated on `BOB_FBO_RTT`; A/B at QM
frame 80 took ground non-black coverage **51% → 99%** (black → green landscape). The fix, in
`bob_video.cpp`:
- `DD_CreateSurface`: **accept** TEXTURE+3DDEVICE surfaces (was rejected) → a `GLSurface7` with
  `isRTT`; lazily build a GL texture + FBO on first `SetRenderTarget`.
- `DEV_SetRenderTarget` (was a no-op): RTT surface → `glBindFramebuffer(fbo)` + viewport;
  back buffer → bind 0 + restore. `present` force-unbinds (safety).
- `SURF_Lock` on an RTT surface → `glReadPixels` the FBO into the system bits (row-flipped), so
  the game's existing `UploadTexture`→`PerformSlowCopy` (reads Lock'd bits) carries the real
  detail pixels into `landTextures[i]`. **No change to the game's copy path.**
- Correction to the earlier worry: the detail pass does **not** "submit no geometry" — on the
  real `F_TEXTURECANBERENDERTARGET` path the geometry submits and composites fine; the empty
  result was purely the back-buffer-fallback readback, which the FBO path replaces.
- **GOTCHA that blocked it — see §4's device-caps note:** enabling RTT hung setup on a latent
  game-code NULL `Release()`, exposed by an over-advertised `dwRasterCaps` bit. Same machinery
  also restores the rear-view mirror (RENDERTARGET_MIRROR). MiG Alley's 2D path is software-fb
  (no D3D7 RTT), but its later 3D-flight phase rides this same `bob_video.cpp` GL path —
  budget for FBO RTT there.

### Assets: `.shp` / IFF
- `.shp` files are **IFF/LBM**: `BMHD` (w/h + masked flag), `CMAP` (palette: an index if
  `blocksize<256`, else a full RGB palette), optional `ALFA` (RLE alpha), `BODY` (RLE
  palette-indexed 8-bit pixels). Parsed on the CPU; identical to Windows on a 32-bit LE build.
- Textures become **`MAPDESC`** structs: `{ UWord w,h; UByteP body, alpha, palette; UByte
  paletteindex:4, isMasked:1, blendType:3; HTEXTUREMAP hTextureMap; }`. `body` = 8-bit
  palette indices. The renderer builds a 256-entry **16-bit (565) LUT** from the palette
  (global `paletteTable[paletteindex]` or a custom per-map palette) and writes
  `dst = LUT[body[i]]`. All texture formats are **16bpp** in practice.

### The RDialog front-end UI toolkit
- **The entire front-end is in-engine rendered art**, NOT native Win32/GDI dialogs:
  full-screen painted backgrounds + the game's **bitmap fonts** + its own **`R*`/`CR*`
  widgets** (dropdowns with red-triangle markers, spinners, checkboxes, tabs, editable text
  fields). Confirmed in BoB across main menu, the config screens (GFX/Controls/Sound/Sim
  tabs), mission/training select, the strategic map with its Directives/Resources windows,
  the briefing, and the debrief — *zero* native window chrome anywhere.
- **Implication:** you likely do **not** need a full native-Windows dialog + GDI backend. The
  front-end should render through the **2D DDraw path you already build** (the same path that
  draws the "Initialising 3D" loader screen). The one residual unknown is whether the widgets
  paint via DDraw surface blits (free) or some GDI text/blit calls (a small, targeted GDI-2D
  need — not full dialogs). Resolve it empirically by booting the real front-end and watching
  what it calls. (MiG Alley uses the same toolkit; expect the same answer.)

---

## 4. The DX7 → OpenGL backend (`bob_video.cpp`) — design + bug catalogue

### Design that works **[ENGINE]**
- **COM emulation:** each DDraw/D3D object is a C struct whose first member is a pointer to a
  hand-built vtbl (`IDirectDrawSurface7Vtbl* lpVtbl; ...`). Cast the COM `this` to your struct.
  Singletons for the device/d3d/ddraw; per-instance for surfaces/VBs/palettes.
- **Present model (critical):** the 3D scene is rendered straight into the **GL default
  framebuffer**. Set a flag (`g_devRendered`) in `BeginScene`/`Clear`/`DrawPrimitive`. On
  present/`Flip`, if that flag is set, just **`SDL_GL_SwapWindow`** — do **not** upload the
  DDraw back-buffer's system-memory bits (the 3D path never wrote them). Pure-2D frames (the
  loader, menus) *do* write surface bits and present by uploading those bits as a textured
  fullscreen quad. **Consequence:** the DDraw back-buffer system bits are stale/empty during
  3D — anything that reads them back (RTT compositing, screenshots-via-Blt, the mirror) gets
  garbage/black. This is the root of the landscape-tile problem (§3).
- **Draw path:** `DrawPrimitive(VB)` → walk the FVF, push `glVertexPointer`/`glColorPointer`
  (`GL_BGRA` for D3DCOLOR=ARGB)/`glTexCoordPointer`, `glDrawArrays`. `XYZRHW` 2D verts use an
  ortho projection with **y-down** (DDraw screen coords) and depth test off (painter's order).
- **Texture upload:** convert `MAPDESC` palette-indexed body → 565 into a system surface
  (`_CreateTextureMap` does this in *game* code, via Lock + a palette LUT), then `BltFast`
  into the managed texture, then your `BltFast`/`Blt` shim must do a **real surface copy**
  (a no-op here makes every texture white/blank). Upload 565 via
  `glTexImage2D(..., GL_RGB, GL_UNSIGNED_SHORT_5_6_5, bits)`; 1555/4444 if the format has an
  alpha mask.
- **Device caps:** report **generous, honest** caps — but **over-advertising caps is a real
  hazard, not a harmless convenience.** Blanket `0xFFFFFFFF` doesn't just "claim more"; it
  claims *specific* bits whose meaning routes the game down different code paths. Two concrete
  recurrences (both bit us in BoB):
  - **`dwTextureCaps`:** `0xFFFFFFFF` falsely claims `SQUAREONLY (0x20)`, `POW2 (0x2)`,
    `NONPOW2CONDITIONAL (0x100)` — Lib3D then mangles/over-scales textures (e.g. squishes a
    256×64 panel to 64×64 → low-res cockpit). **Clear those restriction bits.** Set
    `dwMaxTextureWidth/Height = 4096`, min = 1.
  - **`dwRasterCaps` → `D3DPRASTERCAPS_ZBUFFERLESSHSR (0x8000)`** (the 2026-06-16 RTT-hang
    root cause). This bit claims the device does hidden-surface removal *without* a z-buffer
    (a deferred-renderer / PowerVR trait). With it set, `CheckIfTextureCanBeRenderTarget`
    (LIB3D.CPP) **skips creating `pDDS7MirrorZB`** (leaves it NULL) — then unconditionally
    does `pDDS7MirrorZB->Release()`, a call through a NULL `this` (`lpVtbl->Release(this)`
    derefs address 0) → hang/crash. The non-RTT path dodges it by returning early when the RT
    probe is rejected; only the RTT path reaches the NULL release. Real z-buffered DX7 cards —
    the engine's target — don't set this bit, so the unconditional release was safe on HW.
    **Fix:** clear `0x8000` from `dwRasterCaps` (we gate it on `BOB_FBO_RTT` so the default
    path keeps identical caps). Then `ZBUFFERLESSHSR==0` → the engine creates the z-buffers and
    the probe's z-buffer branch runs to a non-NULL surface.
  - **General principle:** when a blanket cap flips behaviour, the bug often surfaces as a
    NULL deref or a "wrong branch" deep in game code that *assumes a real-hardware cap
    combination*. Advertise caps you actually implement; mask the ones that change control flow
    you don't support.

### Bug catalogue — these WILL recur in MiG Alley **[ENGINE]**
Listed by how much pain they cost. Grep for the named functions to pre-empt them.

1. **Surface reference-counting use-after-free (THE big one).** DDraw surfaces are COM-
   refcounted. The naïve shim (`AddRef` = no-op returning 1; `Release` = `free()` on the
   first call) **frees live surfaces**. Specifically: `_CreateTextureMap` ends every non-dither
   texture with `UpdateMipMaps(tex)`, which does a balanced `tex->AddRef(); ... tex->Release();`.
   Real DDraw: refcount 1→2→1 (stays alive). Broken shim: AddRef(nothing) then Release(**free**)
   → the just-created texture is freed while `textureTable[hTextureMap]` still points at it →
   the freed block is reused, its header overwritten → **garbage/black texture binds**. In BoB
   this manifested as a corrupted cockpit (flat grey panels, no gauges, black gunsight) and
   contributed to other black textures. **Fix:** give your surface struct a real `int ref`
   (init 1), `AddRef` increments, `Release` decrements and frees only at 0. Also free the GL
   texture (`glDeleteTextures`) when the surface is finally freed (guard to the GL-owning
   thread). The game's `DeRefAndNULL` even reads the `Release()` return value — return the real
   count, not 0. **This is the single highest-value fix; do it before you trust any texture.**

2. **`D3DBLEND` → GL blend-factor mapping.** The `D3DBLEND` enum is **1-based**. An off-by-one
   or mis-ordered mapping turns standard `SRCALPHA`/`INVSRCALPHA` alpha blending into garbage
   that **washes the whole 3D world toward the clear colour** (everything looks hazy/invisible).
   In BoB this was *the* "nothing renders" bug. Get the 1-based mapping exactly right
   (`1=ZERO,2=ONE,3=SRCCOLOR,4=INVSRCCOLOR,5=SRCALPHA,6=INVSRCALPHA,7=DESTALPHA,...`).

3. **Masked-texture transparency.** Lib3D bakes a 1-bit colour-key mask into the texture's
   **alpha** (keyed/transparent texels get alpha 0) and **expects the device to alpha-test
   them out — but never sets the D3D alpha-test render state.** If you don't alpha-test, every
   keyed texel shows its raw key colour (magenta/cyan fringe). **Fix:** on the opaque
   (blend-off) path, `glEnable(GL_ALPHA_TEST); glAlphaFunc(GL_GREATER, 0.5)`. Use `GL_NEAREST`
   filtering on masked textures so LINEAR doesn't bleed the key colour into edges.

4. **Texture-stage state & state blocks are easy to no-op — and that loses real features.**
   The game sets per-stage `D3DTSS_*` and builds **`D3DSBT_PIXELSTATE` state blocks**
   (`CreateStateBlock`/`ApplyStateBlock`). A no-op shim silently drops:
   - **`D3DTSS_ADDRESS`** (texture addressing: land = `MIRROR`/`CLAMP`, single-texture =
     `WRAP`). No-op → everything `GL_REPEAT` → over-tiling artifacts on surfaces meant to clamp.
   - The **2-stage combiner** (`D3DTSS_COLOROP = D3DTOP_ADDSIGNED` on stage 1) that adds a
     **second detail texture** for terrain shading. No-op → flat single-textured terrain.
   To do these faithfully you must **emulate state blocks** (snapshot current pixel-stage
   state on `CreateStateBlock`, restore on `ApplyStateBlock`) and apply addressing per-draw
   (`GL_CLAMP_TO_EDGE`/`GL_REPEAT`/`GL_MIRRORED_REPEAT`) + a `GL_COMBINE`/shader for the
   detail blend. This is real work; scope it as a feature, not a quick patch.

5. **Defensive garbage-surface skip.** During setup the engine may transiently bind an
   uninitialised/garbage "surface" (out-of-range w/h/bpp). Detect and skip-as-untextured to
   avoid sampling garbage. But **note**: a *valid* surface that's been use-after-freed (bug #1)
   can ALSO look garbage — fix #1 first, then this guard handles only true transients. Tag your
   real surfaces with a magic value to tell "ours but corrupted" from "never ours".

6. **Mipmaps / `COMPLEX` surfaces.** `_CreateTextureMap` requests
   `DDSCAPS_COMPLEX|DDSCAPS_MIPMAP`. Your `GetAttachedSurface(MIPMAP)` must terminate the
   chain with `NULL` or the engine's mip-walk loops run wild. (We don't build mip chains;
   returning `NULL` makes `UpdateMipMaps` a safe no-op — fine.) GL auto-mipmaps are optional.

7. **Case-insensitive file paths.** The game asks for `DATA\Foo.SHP`; the install is
   lower/mixed case on a case-sensitive FS. Provide `fopen_nocase`/`open_nocase`/path-resolve
   helpers (and `\`→`/`). MiG Alley needs this identically.

8. **R\* control repaint → the offscreen-DC NULL deref.** The `CR*Ctrl::OnDraw` (RCombo/RStatic/
   RListBox) draw **direct to the passed `pdc` only on the FIRST sweep** (`m_FirstSweep==TRUE`).
   Every *subsequent* OnDraw switches to an offscreen-DC route —
   `parent->SendMessage(WM_GETOFFSCREENDC)` + `CreateCompatibleBitmap` + `SelectObject` + a
   BitBlt back — for transparent-bitmap compositing. A compat that doesn't implement
   `WM_GETOFFSCREENDC` returns NULL → the next `SelectObject`/blit derefs NULL. You won't see it
   until something repaints a control *twice* in one screen session (e.g. a combo cycles its
   value on click and you redraw in place). **Fix:** force `m_FirstSweep=TRUE` before each hosted
   `OnDraw` to keep the control on the direct-to-`pdc` path (the offscreen route is an
   optimisation the immediate-mode GDI compat doesn't need). Sites: `RCOMBOC.CPP` (public
   `m_FirstSweep`), `RSTATICC.CPP:~306`, `RLISTBXC.CPP:~597` (the listbox already NULL-guards).
   *Engine-usage note:* in BoB the front-end **buttons are NOT hosted OCX** — only RCombo/
   RListBox/RStatic come through `DDX_Control`; menu/tab/OK buttons render via a separate
   2D-menu path. MiG Alley's destination screens ARE built from many `CRButtonCtrl`, so the
   eventsink/RButton machinery matters there but not in BoB. Verify which controls your dialogs
   actually instantiate (trace `CreateControl`) before building host glue for all of them.

---

## 5. Input (DirectInput → SDL) **[ENGINE]**

- Implement `IDirectInputDevice` objects backed by SDL events. **Keyboard:** the game reads
  buffered events `{dwOfs = DIK scancode, dwData = 0x80 down / 0 up}` and indexes its keymap by
  `dwOfs`, so you must translate `SDL_Scancode` → the **PS/2 set-1 DIK** values DirectInput
  uses (not ASCII). Keyboard is done in BoB.
- **Mouse + joystick are stubbed** in BoB (vtbls exist, no SDL wiring). You'll need both:
  mouse for the entire menu/UI (every front-end screen is click-driven) and joystick for
  faithful flight. Mirror the keyboard approach: SDL mouse motion/buttons →
  `GetDeviceState`/`GetDeviceData` on the mouse device; SDL game controller → the joystick
  device.

---

## 6. Audio (DirectSound / Miles → OpenAL) **[ENGINE, unimplemented]**

- Currently a **silent stub**: `DirectSoundCreate` returns `E_FAIL`; DirectMusic GUIDs are
  zero-filled. There's an `openal_dsound.h` scaffold but **zero real OpenAL calls**.
- To do it: back `IDirectSound`/`IDirectSoundBuffer` (and any Miles AIL calls) with OpenAL —
  buffer creation, sample upload, play/stop/loop, volume/pan, and **positional 3D** (engine,
  guns, radio chatter, ambient). Music (MIDI/DirectMusic) is the least-faithful-feasible
  piece — a software synth (e.g. FluidSynth/TiMidity) or pre-rendered tracks. Self-contained;
  parallelisable with everything else.

---

## 7. Validation methodology — copy this discipline **[ENGINE/process]**

This engine's rendering is **subtle**, and impressions lie. The BoB log has multiple
"CORRECTION" entries where a confident diagnosis was wrong. Protect yourself:

- **The original `.exe` under wine is your pixel oracle.** Run MiG Alley under wine, screenshot
  the exact screen, and A/B against your native output. This single technique cracked the BoB
  cockpit (we'd otherwise have "fixed" things that were already correct). Capture the menus,
  map, briefing, cockpit, and debrief up front and keep them as reference assets.
- **Judge by pixels, not vibes.** Dump frames to PPM (a tiny `glReadPixels`/surface-dump under
  an env flag), convert with PIL, and compute objective stats — distinct colours, per-band
  averages, saturation, hue spread — to tell "rendered" from "black/sky/garbage" and to find
  the one bad texture among dozens.
- **Env-gated, default-off diagnostics.** Litter the backend with `getenv("XX_TRACE_…")`
  probes: frame/texture dumps, per-draw texture+UV traces, `SetTexture` garbage backtraces
  (use `backtrace_symbols_fd` against the unstripped binary + `addr2line -f -C`), a
  surface-integrity canary. These are how you root-cause; keep them in, default-off. The
  symbolized backtrace from a draw/bind site straight into the game's call stack
  (`shape::draw_shape → RenderPolyList → SetCurrentMaterial → SetTexture`) is the fastest way
  to find *which* asset/path is broken.
- **Diagnose to root cause; don't patch symptoms.** The cockpit "rainbow", "missing
  instruments", and "needs FBO" were all symptoms of the *one* refcount UAF (§4.1). Chasing the
  symptoms produced three wrong diagnoses before the root cause. When several artifacts appear
  together, suspect a common root.
- **Keep default `./game` clean; commit small with evidence.** One reproducible finding per
  commit; a running log (our `PORT.md`, newest-on-top) so the next session (or instance) inherits
  context instead of re-deriving it.

---

## 8. Rough completion map (where the effort is) **[ENGINE]**

The order BoB found least-painful, de-risking early:
1. **Foundation** — build/link 32-bit, the pack boundary (§2), file I/O, PE resource loader,
   threads, the present pipeline. (Largely mechanical once §2 is solved.)
2. **Renderer** — 2D path (blits, fonts, ortho) → 3D path (FVF draw, MODULATE, the bug
   catalogue §4). Gets you a flyable cockpit.
3. **Input** — keyboard (done), then mouse + joystick.
4. **Front-end flow** — mouse + the in-engine RDialog toolkit + bitmap fonts, then drive
   menu → setup → (campaign map) → briefing → loader → fly → debrief → progression. This is
   the spine that turns a tech demo into a game, and it fixes the campaign cold-start by
   init-in-order.
5. **Renderer fidelity finish** — landscape RTT compositing (FBO), the 2-stage combiner,
   texture addressing, the mirror.
6. **Audio** — DirectSound → OpenAL (parallelisable).
7. **Save/load, polish, optional** — DirectPlay→sockets multiplayer, intro video (Smacker),
   fullscreen/resolution, perf.

Effort is dominated by #4 (the whole UI/campaign flow) and #5/#6. Foundation + basic flight
is the *easy* half; the game-shaped subsystems are the long tail.

---

## 9. What's BoB-specific (verify for MiG Alley) **[GAME]**

- **Map/world & campaign rules** (Channel/1940 vs Korea/1950s), flight models (props vs jets),
  mission types, the strategic layer's specifics. None of this is your concern — it's game
  logic that runs unmodified.
- **Exact module/file names & some screens.** MiG Alley may name modules differently and have
  different front-end screens, possibly a **newer engine revision** (slightly different DX
  usage, more/larger textures, different UI). The *shapes* of everything above should hold;
  confirm specifics against its source.
- **The "night" lighting issue** was a BoB boot-probe artifact (wrong mission time), not an
  engine bug — but it illustrates the engine-level "lighting is baked into vertex diffuse" fact
  (§3), which does transfer.

---

*Questions this doc can't answer → read the BoB `PORT.md` (full dated evidence log) and grep
the `SRC/compat/` layer. Good luck — most of the cliffs are mapped now.*
