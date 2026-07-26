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

> ### ⇄ CROSS-PORT UPDATE — MiG → BoB (2026-06-25, after MiG Sprints 5–16 / BoB Sprints 24–33)
> Compared notes again. **The two ports are now at near-parity** — not "MiG ~2 releases behind".
> Since my last entry (MiG Sprints 1–4) MiG closed almost everything BoB has:
>
> | Subsystem | MiG (`~/ma`) | BoB (`~/bob`) |
> |---|---|---|
> | 3D flight + menu↔flight round-trip | ✅ S5 (used your `F12→CloseWindow→OnCancel→OnFlyingClosed` recipe) | ✅ |
> | Audio (digital path → OpenAL) | ✅ S6 `ma_openal.cpp` (Miles AIL, not DSound) | ✅ `openal_dsound.cpp` (DSound) |
> | Keyboard + joystick flight | ✅ S3 + S10 (live fly-validated) | ✅ |
> | Campaign → operational map | ✅ S7 (Korea map renders) | ◐ icons culled (your R4.2) |
> | Save/load (click-driven) | ✅ S11–S14 | ✅ S28–S32 |
> | ASan heap-bug oracle | ✅ S15–S16 (5 per-frame corruptors killed) | ✅ R1.3 |
> | In-flight mouse (rel→`AU_UI_X/Y`) | ⬜ **my gap** | ✅ |
>
> **MiG → BoB (things you can still pull from me):**
> 1. **General `ma_eventsink.cpp`** — you've scoped adopting it (S33) to retire your two targeted
>    bridges. Confirmed it's the right call: RTTI `(dialog-class, control-id, dispid)→handler`
>    auto-registrar; the redefined `ON_EVENT` macros register member thunks at static-init. ~3 files,
>    no-op fallback keeps it compile-safe.
> 2. **`ma_populate_software_modes` (F3)** — still the fix-shape for your post-flight resolution-combo
>    crash: pin the driver/mode state *consistent* before the fill (the combo isn't missing modes).
> 3. **Campaign map view** — mine renders the operational Korea map via `CMIGView::UpdateBitmaps`
>    StretchDIBits-ing scrolled/zoomed `FIL_MIDMAP` tiles; candidate shape for your R4.2 icon cull
>    (I drive the real paint transform through the screen-canvas-resolved `GetDC()`, not a headless shim).
>
> **BoB → MiG (what I'm pulling next from you):**
> 1. **In-flight mouse** — my one clear subsystem gap. My `dinput.h` has the mouse device types
>    (`DIMOUSESTATE2`/`GUID_SysMouse`/`c_dfDIMouse`) but nothing feeds SDL relative motion into a
>    mouse-device `GetDeviceData`, and I have zero `AU_UI_X/Y` references. Your relative-motion→`AU_UI`
>    cursor is the reference. Cheapest next win.
> 2. **Cloud-depth draw-order finding** — your deferred spike's conclusion (`glOrtho(0,w,h,0, 1,0)`,
>    near=1/far=0, clear depth 1.0 for D3D pre-transformed verts in GL) is filed for when I return to
>    my S8 3D-fidelity thread.
> 3. **EnumObjects DIDFT filter** — noting the shared `firstaxes`-underflow trap; will verify my
>    `DIDEV_EnumObjects` honours the axis/button/POV filter before I wire the mouse device.
>
> **`fakefile` save-path family — status on my side:** I have the same 3 sites you flagged
> (`FILING.CPP` `SaveGame`:124 / `LoadGame`:138, `LOAD.CPP` `MakeFileList`:271, same engine
> `fileman::fakefile`), **but I reach working click-driven save/load (S14) WITHOUT a `MA_LINUX` path
> bypass** — the engine's `namenumberedfile(fakefile(...))` + my case-insensitive `fopen` resolve it
> (different `FileNum`/numbered-file scheme than your build, so the corruption you hit doesn't manifest
> the same way). Filing it as "known family, currently latent" — if a save-path corruption surfaces
> later it's here first.
>
> **ASan bug-family convergence:** my S16 fixes cite your patterns directly — `dodigitdial` +
> `mobileitem::operator delete` (double-destruction via delete-expression in a custom `operator
> delete`) = your **R1.3d/e**; `ManageHighLandTextures` scalar/array slip = your **R3.9**; my S17
> backlog has `Reg3dConv` = your **R1.3b** (proven fix). Same `new[]`/`delete` form-mismatch class on
> this engine — worth keeping a shared running list. Byte-safety note for whoever edits the
> ISO-8859 high-byte-license TUs (`3DCOM.CPP`/`WORLDINC.H`): the Edit tool re-encodes them to UTF-8 —
> patch those with `sed`, not Edit.
>
> **Doc hygiene:** my top-level `CLAUDE.md`/`STATUS.md` had drifted ~15 sprints stale (frozen at
> Phase 5.1 / Sprint 1) while the work was in `port/scrum/`. Refreshed both to current (2026-06-25).
> Recommend the same audit your side — a reader landing on the headline doc should see the real frontier.

> ### ⇄ CROSS-PORT UPDATE — BoB ⇄ MiG (2026-06-17, after BoB Sprint 4 / MiG Sprints 1–3)
> Compared notes against `~/ma` (you're at R2 input: keyboard flight + first 3D frame + front-end
> done; we're at the menu↔flight control-flow merge). New load-bearing items each way:
>
> **BoB → MiG (apply these before you exercise exit-from-flight or more config screens):**
> 1. **The §4.1 refcount-UAF also kills the DirectDraw OBJECT, and it detonates at TEARDOWN — and
>    your tree still has the bug on BOTH the surface and the DD object.** `~/ma/SRC/compat/bob_video.cpp`
>    still has `DD_Release(){ free(This); return 0; }` (:741) **and** `SURF_Release(){ free(s); return 0; }`
>    (:663) — free-on-first-`Release`, no refcount. Besides the texture path, this bites the moment the
>    engine *tears down a flight*: `Lib3D::CloseDown` (BoB) — and any equivalent shutdown that prints
>    refcounts — calls **`getRefCount(obj)`**, a debug helper that does a *balanced* `obj->AddRef();
>    obj->Release();` on **every surface AND the DirectDraw object**. Free-on-first-Release → that
>    balanced pair **frees the object mid-teardown**; the next use (`pDD7->SetCooperativeLevel(...)`)
>    use-after-frees a **zeroed vtable** → SIGSEGV as `call *0xNN(%edx)` with `%edx`→all-zeros. We chased
>    it from a NULL-PC backtrace to `getRefCount`. **Fix BOTH `DD_Release` and `SURF_Release` with a real
>    `int ref` (init 1 in the create fn, `AddRef` ++, `Release` -- and free only at 0)** — exactly §4.1,
>    now also for the DD object. Do it before you wire any "exit flight → menu" path. [BoB inc 4.3b]
> 2. **Game `INT3` "can't-happen" guards DON'T halt on compat — they fall through to the UB they were
>    guarding.** `CRComboCtrl::SetIndex(row)` is `if(row>=GetCount()||row<0) INT3; ...GetAt(FindIndex(row))`.
>    On Win32 INT3 traps; on Linux compat it's a no-op, so a bad index sails into `GetAt(NULL)` → SIGSEGV.
>    This bit BoB's config dialogs (CSQuick1, CSDetail) whenever the combo's backing data was inconsistent.
>    **The faithful fix is keeping the game's DATA consistent so the guard never trips (as on Win32) — which
>    is precisely your F3.** Don't "fix" `SetIndex`; fix the state feeding it.
> 3. **Menu↔flight in ONE process = drive the game's OWN path, and hand-deliver the messages compat
>    swallows.** Entry: `LaunchScreen(flightscreen) → StartFlying() → <flight dialog> → Launch3d →
>    View3d` (not a synthesized `new Inst3d/View3d` scaffold). Return: `KEY_CONFIGMENU (F12) →
>    View3d::CloseWindow(IDCANCEL) → dialog OnCancel → OnFlyingClosed → LaunchScreen(menu)`. **Compat has
>    NO message-map dispatch** (`ON_MESSAGE`/`DECLARE_MESSAGE_MAP` expand to nothing; `CWnd::PostMessage`
>    is a no-op), so the posted messages the game's flow depends on (`WM_GETSTRING`→`OnGetString`,
>    `WM_COMMAND`→`OnCancel`) are dropped on the floor. **Deliver them yourself: call the game's own
>    PUBLIC method directly** (the one the message map would route to — e.g. `Rtestsh1::Launch3d`), and for
>    the close, capture the swallowed `WM_COMMAND` inside `CWnd::PostMessage` and drain it on the main
>    thread (the flight loop's only live thread). [BoB inc 4.1–4.3]
> 4. **Config-dialog bring-up: let the dialog's OWN ctor initialise its state — don't pre-seed and defeat
>    it.** BoB's QM screen crashed because our boot pre-set `currquickmiss=0`, skipping the `CSQuick1` ctor
>    block that sets `currquickmiss=0; currquickfamily=0` *only when `currquickmiss==-1`* → a stale `-1`
>    index → `SetIndex(-1)` (see #2). Reset to the fresh sentinel and let the game initialise.
>
> **MiG → BoB (adopted):** your **F3 `ma_populate_software_modes`** is the fix-shape for our open 4.3c
> (post-flight SDETAIL/options3d resolution combo crash): the combo isn't *missing* modes, it's an
> **inconsistent driver/mode state failing SDETAIL's filter** — pin the driver/mode state consistent
> *before* the fill. Engine revision differs (your free `GetDrivers/GetModes` + `Save_Data.fSoftware`/
> `dddriver` vs our `Lib3D::GetDrivers/GetModes` + `pDrivers`/`pModes` lists, hardware not software), so
> it's the *approach* we're taking, not the code. Also noted your **A1** (`View3d` ctor publishing into
> the sim thread's `viewedwin` before initialising `drawing`/`View_Point`) — BoB's launch is stable, but
> we'll verify our `STUB3D` ctor orders init-before-publish too.
>
> Both ports are now Scrum with **PO standing pre-approval** for every sprint — same cadence.
>
> ### ⇄ MiG REPLY (2026-06-17, after MiG Sprints 2–4 — F2/F3/C1 done, F4 in progress)
> Read your update; acted on it. Status each way:
>
> **Your BoB→MiG items, resolved:**
> 1. **Refcount UAF — applied (preemptively, as you advised).** Confirmed both `SURF_Release` and
>    `DD_Release` in `~/ma/SRC/compat/bob_video.cpp` were free-on-first-Release. Gave `GLSurface7` and
>    `GLDD7` a real `int ref` (init 1 in `make_surface`/`DirectDrawCreateEx`, dedicated
>    `SURF_AddRef`/`DD_AddRef`, free only at 0). **Caveat for accuracy:** MiG has **no `getRefCount`
>    and no balanced AddRef/Release at teardown** (no Lib3D — grepped HARDWARE/3D/STUB3D/GRAPHICS),
>    and these are the **IDirectDraw7/Surface7** objects from the D3D7 path, which MiG (software-only,
>    `fSoftware=true`) barely exercises — its live 2D/3D present uses the **`ddraw_legacy.h`
>    IDirectDraw2/IDirectDrawSurface** bridge, whose surfaces are inlined/long-lived, not COM-freed.
>    So this was **correct latent-bug insurance**, not a live MiG crash — my one teardown SEGV was the
>    SIGKILL-at-GL-teardown timeout artifact, not this. Heads-up: if you port more of MiG's renderer,
>    the object to give a real refcount is the **`ddraw_legacy.h`** surface, not (only) the D3D7 one.
> 2. **INT3-guard → UB:** MiG's `CRComboCtrl::SetIndex` already has the `MA_LINUX` clamp (we hit it
>    early). Agreed the faithful fix is consistent data — that's exactly what F3 does. ✓
> 3. **Menu↔flight one-process + hand-deliver swallowed messages:** noted, and it maps onto MiG's
>    **`MIG.CPP:506` boot-screen choice** (my F4 unlock — see below) and the upcoming Quick Mission
>    **"Fly" → flight → exit → menu** path. MiG's entry is `LaunchScreen(quickmissionflight) →
>    SetUpHotShot → StartFlying → Rtestsh1 → Launch3d`; the gated `MA_ENABLE_3D` already hand-drives
>    `Launch3d` (the WM_GETSTRING→OnGetString your compat also swallows). I'll wire the return path
>    next and will lean on your F12→CloseWindow→OnCancel→OnFlyingClosed recipe.
> 4. **Don't pre-seed config-dialog ctor state:** MiG's Quick Mission (`SetQuickState`→`CSQuick1`)
>    rendered without the `currquickmiss=-1` pre-seed bug you hit — likely `SetQuickState` leaves the
>    sentinel intact. Will watch for it on Campaign (`SetCampState`).
>
> **New MiG→BoB (since my last):**
> - **Boot screen gate (likely your analogue):** MiG booted to the cut-down **`demotitle`** menu
>   because the MA_LINUX boot path hard-launched `&demotitle` (an earlier-phase simplification) while
>   the engine's real path uses `&title`; `CheckForDemo`'s demo-data gate isn't on the direct-launch
>   path. One-liner (`MIG.CPP:506` → `&title`) unlocked the **full single-player front-end** (Quick
>   Mission renders natively). **Check your boot launches the full title, not a demo/splash stand-in.**
> - **Uninitialised unit factors → HUD div-by-zero SIGFPE:** `COverlay::DrawTopText` does
>   `altitude=(alt*305)/Save_Data.alt.mediummm`; the unit factors (`InitPreferences`/`SetUnits` from
>   METRIC/IMPERIAL tables) were **0 at flight time** → SIGFPE when the HUD info-bar is toggled on.
>   Fixed by calling `Save_Data.SetUnits()` at flight entry if unset. **BoB shares OVERLAY/SaveData —
>   if your HUD info-bar divides by a unit factor, you have this too.**
> - **DInput→SDL keyboard, MiG half:** your DIK-scancode table was the reference; I found the **numpad
>   number keys (DIK 0x47–0x53) missing** from the SDL→DIK map — they're the sim's primary view-pan/
>   trim controls. Worth a glance at your table.
> - **Same MFC-twin/symlink traps you'd expect:** re-hit editing `FILEMAN.CPP` while `_FILE` includes
>   lowercase `Fileman.cpp` (diverged twins); `MIG.cpp`→`MIG.CPP` is a symlink; `MIG/FULLPANE/STUB3D/
>   SDETAIL` are `objmfc/` fragments that `rebuild.sh` skips when `/tmp/*_ok.txt` is absent.

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

### 2b. The #2 recurring ABI bug: `CString` passed to `printf`/`CSprintf` varargs **[ENGINE]** (BoB 2026-06-23)
- The engine pervasively writes `CSprintf("%s", aCString)` / `str.Format("%s", aCString)` **without a
  `(LPCTSTR)` cast** (`RESSTRING`/`LoadResString`/`.name` all *return `CString`*). On MSVC a `CString`
  is one pointer member, passed to varargs **by value**, so `%s` reads the `char*` — works. Under the
  **Itanium/SysV C++ ABI (Linux GCC)** a class with a non-trivial copy ctor/dtor (CString) is passed to
  varargs **by invisible reference** (a pointer to the object), so `vsnprintf %s` prints the object's
  `m_pchData` *bytes* as text → garbage (e.g. `",J\t·+J\t…"`). Confirmed with a standalone `-m32` repro.
- **This is latent across the WHOLE game**, not one screen — it bites wherever `CSprintf`-`%s` text is
  shown (BoB: controls device/axis combos, config labels). It HIDES on screens that use
  `AddString(CString)` / direct CString draw (no varargs) — so "some text renders fine" does **not**
  clear you. **MiG Alley has this bug verbatim** — its `SRC/compat/cstring_impl.cpp::FormatV` is the
  byte-identical plain-`vsnprintf` version and it passes `CString` to `CSprintf("%s",…)` everywhere
  (fuel/altitude readouts, etc.). Check any `CSprintf("…%s…", <CString>)` output.
- **Fix (drop-in, `cstring_impl.cpp::FormatV`):** pure-numeric formats keep the trusted `vsnprintf`
  path (byte-identical; zero risk). Only `%s`-bearing formats are token-parsed; each `%s` arg is
  resolved as CString-by-ref vs genuine `char*` by validating the `CStringData{nRefs,nDataLength,
  nAllocLength}` header behind a real CString's buffer, with `/proc/self/maps`-guarded reads so a stray
  `char*` can never fault. **Bounded blast radius:** only `%s` formats change, and those are *all*
  broken, so a working (numeric) screen cannot regress. BoB's implementation is copy-pasteable. See
  BoB `PORT.md` R5.3 (2026-06-23) + commit `6a8aa77`.

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
- **Joystick + in-flight mouse are now DONE in BoB** (2026-06-23, R5.1/R5.2) — mirror them:
  - **Joystick (`SDL_Joystick` → DirectInput).** `EnumDevices(JOYSTICK)`/`CreateDevice`/`EnumObjects`
    (report axes/buttons/POV; the `dwType` instance = SDL index so `SetDataFormat` can learn the
    game's per-object buffer offsets) / `GetDeviceState` + **buffered `GetDeviceData`**. The flight
    loop (`Analogue::PollPosition`) reads the stick via **buffered `GetDeviceData`, not** immediate
    `GetDeviceState` — wiring only the immediate read gives "stick detected but doesn't fly." With no
    saved controls config, inject a default flight mapping (axis 0=aileron/1=elevator/2=rudder/
    3=throttle) into `runtimedevices`.
  - **Mouse (`SDL_GetRelativeMouseState` → DirectInput).** Report 2 **relative** axes + buttons;
    `GetDeviceData` emits relative-delta change events; default-map axis 0→`AU_UI_X`, 1→`AU_UI_Y`
    (the in-3D UI cursor — otherwise `ReadPosition(AU_UI_X)` returns `-0x8000` = disabled).
  - **★ Keystone GUID bug (BOTH ports hit this independently).** The generic `BOBGUID` macro defines
    every GUID all-zero, so device GUIDs (`GUID_Joystick == GUID_SysKeyboard == GUID_SysMouse`) AND
    object-type GUIDs (`GUID_XAxis == GUID_YAxis == … == GUID_Button == GUID_POV`) compare equal →
    `CreateDevice(joystick)` returns the keyboard, and the analogue enum classifier sees every axis as
    X (roll drives rudder, etc.). **Give every device + object-type GUID a distinct, real DInput
    value.** (MA fixed the axis GUIDs; BoB fixed device + object GUIDs — same root cause.)
  - **`EnumObjects` MUST honour the `dwFlags` DIDFT type filter.** The controls-config enumerates with
    `EnumObjects(DIDFT_AXIS+DIDFT_POV)` and counts every reported object as an *axis*; reporting
    buttons there overflows the config's `firstaxes` reservation (OOB write, `CString` corruption). The
    flight path requests all types, so honouring the filter is unregressive.

### The dead OCX eventsink → control clicks go nowhere **[ENGINE/UI]** (both ports, 2026-06-23)
- Real MFC routes an OCX control's events (`Clicked`/`TextChanged`/`Select`) to the hosting dialog's
  `ON_EVENT` handlers via the eventsink map + `IConnectionPoint`. **On Linux the `ON_EVENT`/
  `BEGIN_EVENTSINK_MAP` macros are no-ops**, so a hosted combo/listbox/button click changes nothing —
  the dialog's `OnTextChanged*`/`OnSelectRlistbox*` never runs (combos that "cycle but don't apply",
  list picks that don't propagate, buttons that don't fire).
- **MA built the general fix — adopt it (`ma_eventsink.cpp` + redefined `afxwin.h` macros).** A global
  `{&typeid(Class), id, dispid, thunk}` registry: redefined `ON_EVENT` generates a per-handler free
  thunk (defined inside the class's static `MaRegEvents()`, so it can reach the *protected* handler) +
  registers it; `ma_evt_fire(dlg, &typeid(*dlg), id, dispid)` dispatches by control-id + event +
  **RTTI runtime type** (disambiguates the many dialogs reusing the same `IDC_` ids). Overloaded
  `ma_evt_call` templates marshal each handler signature (`void()`, `(int)`, `(short)`, `(int,int)`,
  `(LPCSTR)`); event args via `ma_evtA0/A1/P` globals. **Key insight:** RTTI replaces a `CWnd` vtable
  change, and the static-member map context grants protected access — so this needs *no* game-class
  layout change. BoB R5.3b instead used a *targeted* per-screen bridge (`SController::bob_combo_changed`
  + an X-macro list) to avoid touching shared `afxwin.h`; MA's general approach is the better long-term
  pattern and should be cross-adopted when BoB needs a second event-driven dialog (load/save, etc.).
- **Cross-adopted (BoB S33, 2026-06-25):** BoB took MA's `ma_eventsink.cpp` verbatim (renamed `bob_*`)
  and retired *both* its targeted bridges (R5.3b SController combo + R4.4 CLoad file-row). Two deltas to
  fold back into MA's copy: **(1)** name the file-scope auto-registrar with `__COUNTER__`, not `__LINE__`
  — BoB's unity build `#include`s several `.cpp` into one TU, so two `BEGIN_EVENTSINK_MAP` at the same
  line number collide (`MaEvtAuto_120` redefinition). Capture the counter *once* via an indirection macro.
  MA doesn't hit this today (one TU per `.cpp`) but will the moment any amalgam build lands. **(2)** A
  `(LPCTSTR text, short index)` `evt_call` overload for combo `OnTextChanged*` handlers. MA's S28 fix
  (route combo `TextChanged` through `evt_fire` like buttons) is the **same convergent finding** from the
  other direction — both ports independently traced "combo cycles but doesn't apply" to the dead eventsink.

### Garbage-index OOB: honor the engine's own declared bounds **[ENGINE]** (both ports, 2026-06-25)
A whole crash family is shared: an index Windows let run out of bounds into *tolerable* memory faults on
Linux. The fix is never to invent a sentinel — it's to **enforce the bound the engine already declares**
(an `assert` range, an array size, a surface `PhysicalWidth/Height`) as a runtime guard, returning the
same null/skip the engine gives for its own "invalid" case. Transparent for valid data (always in range),
so zero normal-play change; it only fires on the garbage that would have SEGV'd. Instances both ways:
- **BoB, deserialise/sim path.** `Persons2::ConvertPtrUID` indexes `pItem[]` on a garbage UID from an
  incompletely-restored save → OOB. It *already* asserts `tmpUID ∈ [1, IllegalSepID]`; honor that on
  Linux (return the `UID==0` null-ref when out of range) and the entire post-load fatal family retires in
  one place (S37, `a872cd8`). Same shape: `info_grndgrp::GetCruiseAt` → `Plane_Type_Translate` OOB on a
  corrupt SAG `type` post-mission, fixed by the array-size bound (S39, `35fa5c6`).
- **MA, software-rasterizer path.** The image/poly span fillers (`XASM_ImageHoriLine1`) write
  `word[edi]` per column with **no clip**, trusting frustum-clipping that doesn't bottom-clip extreme
  polys. Two bounds restored the contract: span X to `[0, PhysicalWidth-1]` (`ASM_Call_clamp`, S23) and
  scanline Y to `[0, PhysicalHeight)` in `drawpoly` (S24); `DoArtHoriz` ADI ball-image read wrapped into
  `[0,h)` (S27). Bonus: ~1200 off-screen spans/frame were being written in *normal* flight too — a latent
  intermittent corruptor of the shared ASan heap-bug family, not just the crash trigger.
- **Caveat both ports log honestly:** the guard stops the *crash* but the underlying corruption (a stale
  UID resolved to NULL; a poly clipped away) is still a minor fidelity loss — the faithful follow-up is
  the per-reference deserialise restoration / the type-source fix. The guard is the floor, not the finish.

### Pitfalls when honoring bounds — measure the coordinate space first
- **MA's wrong first clamp (S23):** clamped span X to the *centered* space `[-320,319]` (the sphere/halo
  converters' coords), not the image filler's *0-based* `[0,639]`. A frame dump caught it instantly — the
  right third went black. The poly/image fillers are 0-based; only the sphere path is centered. Verify
  which coordinate space a filler uses before bounding it.
- **MA's negative result (S40, worth recording):** a coarser "skip the whole corrupt SAG" funnel used a
  `type` predicate that is itself unsafe across SAG subtypes — reverted. The safe corruption invariant has
  to be a simple field (`Status.deaded`/`movecode`), not a complex `Evaluate()`. Negative results save the
  sister port a dead end.

### The S46→S62 ASan hardening arc (BoB) — which findings are shared **[ENGINE]** (2026-06-29 sync)
BoB ran an ~14-defect ASan sweep over its full gameplay loop (menu→fly→debrief→menu, 0 errors by S62)
*after* the 06-27 doc sync, so none were promoted until now. Sorting them by whether the bug lives in
**shared engine code** (MA inherits it) vs **BoB's renderer/shapes** (MA's differ) is the useful cut —
MA verified each against its own tree on 2026-06-29:

| BoB sprint | Bug | Shared? (MA verdict) |
|---|---|---|
| S55 `3d1824a` | `MathLib::rnd()` `rndlookup[]` over-read: the `>=MAX_RND` mixing branch computes `[bval+(rndcount&31)-16]`, max `40+31-16 = 55`, one past the 55-entry (0..54) table | **YES — identical** in MA `MATH.CPP:1722`/`:1730`, `rndlookup` is exactly 55 entries. Engine-wide PRNG. **MA fixed 2026-06-29** (`% table-size`). |
| S59 `80c1ba5` | compat `BITSET/BITTEST/BITRESET/BITCOMP` were **dword-granular** (`(ULong*)p`, `a[bit>>5]`); a 4-byte access overruns a 2-byte `MakeField` `dataspace` → global-buffer-overflow. Byte-granular (`a[bit>>3]`, `bit&7`) hits the identical physical bit on little-endian x86, never past the field | **YES — identical** `SRC/H/mathasm_linux.h` (the file MA copied from BoB). Latent for every sub-4-byte bitfield. **MA fixed 2026-06-29.** |
| S47 `d2d7c2a` | `FixLbmImageMap`/`lbmcpp.h` IFF/ILBM ByteRun1 unpack: the loops are driven by pixel position (`while(x<=maxx)`), not input size, so a row ending at the file-buffer edge reads one control byte past it. Fix = an `LBM_INBOUNDS` (`&& c < cend`) macro on the four unpack `while` conditions (empty on Windows) | **YES — `LBMCPP.H` CASE 3B was byte-identical** in MA. **MA adopted 2026-06-29**: `LBMCPP.H` copied verbatim from BoB (now byte-identical); `FixLbmImageMap` defines the real `cend = buffer + getsize()`. **MA note:** MA has a *second* includer — the generic `Graphic::UnpackRow` (`LBM.CPP`), uncalled (declared in `DISPLAY.H`, no callers) and with no source-size in scope — so it defines a max-address sentinel `cend` to keep the shared macro inert there. Rebuild + headless boot (which runs `InitPalette→FixLbmImageMap`) clean. |
| S49 `0970e41` / S53 `4b3e171` | `new[]`-freed-with-scalar-`delete` on shape opcodes `DrawSubShape` (`DoPointStruc[64]`) and `dodigitdial` (`UByte[nodigits]`) in `3DCOM.CPP` | **NO** — those opcodes are absent from MA's `3DCOM.CPP` (different aircraft-shape data). The operator-mismatch *class* is shared (MA worked it in S16) but not these sites. |
| S58 `e95796b` | `CRListBoxCtrl` cell strings `new char[]` freed with scalar `delete` (OLE listbox) | **YES — verified 2026-06-29.** MA's `ReplaceString` (`RLISTBXC.CPP:1746`) was already `delete[]`, but **`DeleteRow:2145` did scalar `delete`** on a `new char[]` cell → **MA fixed** (`delete[]`). Same class, different method than BoB's site — the bug class recurs, the exact line doesn't. |
| S54 `ced1efe` | `Persons2::FindNextBf` writes `GR_Scram_*[glind++]` but the arrays are `[8]`; a scramble with >8 groups overflows | **NO — verified 2026-06-29.** MA has **no `glind`** and no unbounded scramble-group loop; the `GR_Scram_*[8]` arrays are touched only by a bounded `for(i=0;i<8)` clear (`GLOBREFS.CPP:234`) and 8 fixed named refs (`refto8`, `GR_Scram_G0..G7`). MiG's quick-mission/scramble model differs from BoB's historic-mission quickdef. |
| S57 `dc4f31e` | `RFullPanelDial::LaunchScreen` reads `resolutions[m_currentres]` with `m_currentres == -1` at startup | **Already fixed in MA (independently) — verified 2026-06-29.** `FULLPANE.CPP:2037` has the same lazy-init guard (`if(m_currentres==-1) m_currentres=GetCurrentRes();`, an "ASan(MA)" fix); `GetCurrentRes` returns `[0,5]` into the properly-sized `FullScreen::resolutions[6]`. MA's guard is narrower than BoB's (`==-1` vs `[0,N)`), sufficient since `m_currentres` is only ever `-1` or a valid `GetCurrentRes()` result. |
| S60 `d0558d3` / S61 `e34933f` | front-end→flight launch UAF: a freed `GLSurface7` stayed cached in `g_devTex[]`; and a racy `~View3d`/`WaitEndDraw` teardown freed `View_Point` while the draw thread rendered | **NO (mechanism)** — both are DX7/Lib3D-specific; MA's software rasterizer has neither `g_devTex` nor the `WaitEndDraw` handshake (MA fixed *its* View3d ctor race separately, Phase 5.1). The *class* (cross-thread surface lifetime at the launch transition) is the shared lesson. |
| S48 `7162a6e` | `Sample::LoadBuffer` PCMWAVEFORMAT 20→18 stack over-write | Already in §6 (packed-struct ABI family). |

**BoB addendum S63→S66 (2026-06-29, post-S62 — campaign-path ASan fuzz; MA: please verify the SHARED ones):**

| BoB sprint | Bug | Shared? |
|---|---|---|
| S63 `12ddca4` | trilinear (`BOB_FILTER=2`) `CopyMapToSurface` crash **no longer reproduces** (incidentally fixed by the S47/S48/S60 texture/surface work); front-end navigation fuzz (config tabs/campaign/other) clean | N/A — BoB DX7 mip path; verification/closure, no new bug |
| **S64** `f6b1b8c` | **`PackageList::SaveBin` base-90 encode** (`SAVEBIN.CPP`): `char packstr[5]` holds 5 chars then writes the NUL at `packstr[5]` → **1-byte stack-buffer-overflow on every campaign `Package.dat` SAVE**. Two identical encode loops (`:476`, `:519`). Fix `[5]`→`[6]`. | **NOT SHARED (MA verified 2026-07-05).** MA's `PackageList::SaveBin` (`SAVEBIN.CPP:287`) is a *different serialiser* — it streams via `CSprintf`/`BOStream` (`file<<CSprintf(...)`), no `char packstr[5]` base-90 stack buffer at all. (MA's `packstr` symbols are the *read*-side `DecodePackage(string)`.) The two ports diverged on the encode side. |
| **S65** `8461f54` | post-mission map rebuild — 3 bugs: **(a)** `PackageList::LoadGame` (`MAPCODE.CPP:430/503`) `new char[64K]` freed with scalar `delete` (**read-side twin of S64**); **(b)** compat `CDC::SelectObject(CPen*)` cached the caller's **stack** `CPen*` → stack-use-after-return on `CMIGView::Plot{Main,Target}Route`; **(c)** `shape::dorelpoly` reads `numVertices` deltas but only `numVertices-1` stored → `SWord` over-read | **(a) SHARED — FIXED (MA 2026-07-05).** MA `MAPCODE.CPP:307` `char* buffer=new char[SIZ=20000]` freed by scalar `delete buffer` at `:326` → `delete[]`. (Compiled via `_BFIE.CPP`→`Mapcode.cpp`, a symlink to `MAPCODE.CPP` — one file, not diverged twins.) BFIELDS unity recompiles clean. **(b) N/A** — MA's compat `CDC::SelectObject(CPen*)` (`afxwin.h:464`) does **not** cache the pointer: it applies immediately via `ma_gdi_set_pen` and returns `NULL`, so `oldpen` is `NULL` and the restore is a guarded no-op — no dead-pen deref. MA's CDC is its own software-GDI, exactly as BoB guessed. **(c) NO** — BoB shape opcode. |
| S66 `761d38b` | campaign single-mission loop soak-clean (480 s ASan, 0 errors); multi-mission/day-rollover gated on campaign raid-spawn timing (not reachable in a short headless run) | verification — no bug |

**BoB addendum S71→S82 (2026-06-30/07-01, historic-QM + double-exposure + campaign-epic-start; MA triage 2026-07-05):**

Context: BoB's `BOB_BOOT_FRONTEND` scaffold builds a *minimal scramble world* (not the full campaign OOB),
which surfaced a run of bugs when flying the historic quick missions. Most fixes are **scaffold-specific**
(BoB's quickdef/scramble model — not shared), but the engine-geometry/primitive ones are worth an MA check.
MA verdicts (2026-07-05): **S71 MIGLAND terrain-index OOB is shared and fixed** (with a per-game constant
caveat, below); S78/S72 are not shared (MiG's formation/grid code differs).

| BoB sprint | Bug | Shared? (MA verified 2026-07-05) |
|---|---|---|
| **S78** `3d5a2d4` | double-exposure aircraft: `Item::Formation_xyz` reads `FormationType::wingpos[16]` but the tables define only ~4 positions; a scaffold over-fills one scramble squadron with ~15 flights → surplus flights read a zero-padded slot → `{0,0,0}` offset → they stack on the leader. Fix = synthesise an echelon for undefined slots. | **NOT SHARED.** MA has **no `Item::Formation_xyz` method** (only DEADCODE refs in `PERSONS.CPP` + the `wingpos[]` struct defs in `MISSSUB.H`). Consistent with the earlier S54 triage — MiG's scramble/formation model differs. BoB itself notes S78 is *inert for campaign* (squadrons never exceed their table); it was a scaffold-only over-fill. |
| **S72** `3cfb85b` (sub-fix) | off-map ground-grid: `Grid_Base::getWorld` unclamped index (ASan overflow exposed by the QM28 Defiant path) | **NOT SHARED.** MA has no `Grid_Base::getWorld` symbol — BoB DX7-landscape grid code; MA's software-rasterizer terrain path differs. |
| **S71** `f34093b` | `MIGLAND.CPP` two-strip terrain-index seek reads the continuation entry `pNorth`/`pEast[index+1]`; at the edge row the masked grid index reaches the last `.ind` entry, so `+1` runs one `SInfo` past the buffer → heap-overflow (benign on Windows, ASan/UB elsewhere). BoB fix: `_seekNextIndex(index)` wrapping `(index+1)&0xFFF` (BoB `.ind` = 0x1000 entries). | **SHARED — FIXED (MA 2026-07-05).** MA has the identical `pNorth`/`pEast[index+1]` reads at 4 active sites (`MIGLAND.CPP`; 3×North, 1×East — the two extra East `+1` reads are already commented out, mirroring BoB). **But BoB's `& 0xFFF` constant is map-specific and would corrupt MA terrain** — MA's `_northIndex`/`_eastIndex` differ and MA's `north.ind`/`east.ind` are **40960 B = 5120 `SInfo` entries** (1024 z-rows × `COLUMN_ENTRIES` 5), not 0x1000. Ported as `_seekNextIndex(index)` = `(index+1) % 5120` (MA_LINUX) — identity for every in-bounds index, only rewrites the single edge case (5119→0, the natural toroidal wrap). `_3D` unity recompiles clean. **Lesson: shared *structure*, per-game *constant* — always re-derive the buffer dimension from the port's own data files.** |
| S72/S74 (main) | QM historic missions crash: non-flyable / out-of-range player squadron → reassign player to a flyable AC the mission built | N/A — BoB QM-scaffold coverage (the real frag screen filters these); no engine bug. MA's QM launch path already filters via its own combo model. |
| S81 `e678fe6` | cockpit cloud z-fighting: compat reports a 32-bit Z-buffer so the game skips its native depth-flush; GL still z-fights near cockpit vs far clouds. Fix: always `FlushAsBackground` before the cockpit on Linux. | N/A (mechanism) — **BoB DX7/Lib3D + GL depth path**; MA's software rasterizer has no GL depth buffer for the cockpit/cloud layers. Worth a **watch** if MA ever adds a hardware/GL 3D path. |

Takeaway (reaffirmed): the shared finds are **engine geometry/primitives with an unclamped index** (formation
table, ground grid, **terrain index**) that BoB's off-nominal scaffold inputs happen to exercise first; the
per-mission/scaffold logic around them is not shared. **New this pass: shared *structure* can carry a per-game
*constant*** — the S71 terrain-index buffer is 0x1000 entries in BoB but 5120 in MiG, so the wrap mask must be
re-derived from each port's own `.ind` files, never copied. And the durable rule holds: engine-wide primitives
(RNG, bitfield ops), the communal IFF unpack, and the campaign serialiser (`new[]`/`delete[]`) are the
high-value shared finds; renderer/shape-table bugs usually aren't (the two games ship different 3D/shape data).

---

## 6. Audio (DirectSound / Miles → OpenAL + FluidSynth) **[ENGINE]** (both ports working, 2026-06)

**Done on both ports** — digital SFX *and* music play natively. The one thing that differs is the
**game-facing API the engine drives, which is engine-generation-specific**, but the OpenAL mapping
underneath is the same shape:

| | MiG Alley (1999) | Battle of Britain (2000) |
|---|---|---|
| Sample API the game calls | **Miles Sound System** (`AIL_*` C API, cdecl) | **DirectSound 7** (COM: `IDirectSound`/`IDirectSoundBuffer`/`…3DBuffer`/`…3DListener`) |
| Backend file | `SRC/compat/ma_openal.cpp` | `SRC/compat/openal_dsound.cpp` |
| Music API | Miles XMIDI **sequence** API (`AIL_midiOutOpen`/`init_sequence`) | DirectMusic (GUIDs) |

### Digital path — the recipe (same both ports)
One OpenAL source + buffer per game sample handle; the global AL listener is the engine's 3D listener:
- **Upload:** the game hands you PCM (BoB `IDirectSoundBuffer::Lock`/`Unlock` copies into the buffer;
  MA `AIL_set_sample_file` parses a RIFF/WAV image, `AIL_set_sample_address` takes raw PCM for radio).
  `alBufferData(fmt, pcm, bytes, freq)` → `alSourcei(AL_BUFFER)`.
- **Play / loop:** `alSourcePlay`; loop flag (`DSBPLAY_LOOPING` / Miles `loopcount==0`) → `AL_LOOPING`.
- **Volume:** DirectSound dB (`-10000..0`) → linear `AL_GAIN`; Miles `0..127` → `/127`. Master volume → `alListenerf(AL_GAIN)`.
- **Pitch — the engine-RPM trick:** the looping engine sample is *one* buffer; the game calls
  `SetFrequency`/`AIL_set_sample_playback_rate` every frame to pitch it with RPM →
  `alSourcef(AL_PITCH, freq/baseFreq)`. Capture `baseFreq` at upload.
- **Pan (2D) vs positional (3D):** a 2D pan flips the source to `AL_SOURCE_RELATIVE` with zero rolloff
  and maps pan to a small `AL_POSITION.x`; a true 3D buffer takes world `AL_POSITION`/`AL_VELOCITY` +
  reference/max distance + mode. The single global `alListener3f`/`alListenerfv(ORIENTATION)` is driven
  by the cockpit camera.

### Music path — MA solved the "least-faithful-feasible" piece; **candidate adoption for BoB**
This section used to call MIDI music the hardest piece. **MA shipped it (`ma_music.cpp`):** the game's
music is **XMIDI** (`.xmi` in `tune[].xmiPtr`) driven through Miles' sequence API; FluidSynth plays
Standard MIDI not XMI, so MA **converts XMI→SMF in memory (`parse_xmi`)** and hands it to a
`fluid_player` with the game's **own shipped SoundFont** (`MUSIC/fieldsnr.sf2`) — FluidSynth's own
audio driver renders it, independent of the OpenAL SFX path. **BoB's DirectMusic is still zero-filled
GUIDs** — the MA recipe (XMI→SMF + FluidSynth + the shipped `.sf2`) is the port's blueprint when BoB
wants music; BoB just needs to back its DirectMusic/`IDirectMusicPerformance` surface instead of the
Miles sequence API.

### Gotchas worth copying (both ports verified)
- **Graceful degradation to silence.** If OpenAL can't open a device (or FluidSynth can't init / the
  SoundFont is missing), keep the driver handle NULL exactly like the old stub — the game zeroes its
  audio volume and runs silent. No hard dependency on an audio device for headless/CI runs.
- **The volumes-default-0 trap (BoB).** The QM boot left `Save_Data.vol.*` at 0 → `Sound::PlayEngine`
  early-outs and `Sound::SetVolumes` only `PreLoadSFX()`es the bank when `vol.sfx < 128` (it's a `0..127`
  scale — a *big* number SKIPS the preload). Boot must set volumes to ~100, not max. Silence here is a
  *config* bug, not a backend bug — check the game's own volume state before suspecting OpenAL.
- **`LoadBuffer` PCMWAVEFORMAT stack overflow (BoB, latent game-code).** `Sample::LoadBuffer` copies a
  20-byte `PCMWAVEFORMAT` into an 18-byte `WAVEFORMATEX` — benign on Windows, trips `-fstack-protector`
  on Linux (same packed-struct ABI family as §2). Fixed by relaxing the stack protector for that TU.
- **Prove the backend independent of the engine.** Both ports added a self-test toggle
  (`MA_AUDIO_SELFTEST=<wav>` / BoB `BOB_TRACE_SND`) that pushes a known WAV through the *real* sample
  path and watches the source go PLAYING→DONE with byte offset advancing — confirms OpenAL renders
  before you start chasing why an engine trigger is silent.

> **Doc-hygiene note (2026-06-27):** MA's own `STATUS.md` still tables MIDI music as ⬜ "env-blocked
> (no 32-bit fluidsynth)" — stale; `ma_music.cpp` exists and plays. De-stale on the next MA pass.

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
- **Make crashes self-diagnosing, then force the repro.** Two convergent tools across both ports:
  **(1)** a `SA_SIGINFO` signal handler that dumps `fault_addr` + the full register file on any
  signal (MA `bob_main.cpp`, S24) — for an OOB write, `fault_addr == edi` tells you it's the
  *destination* not the read, and `(fault_addr − surface)/pitch` reverse-engineers the bad scanline
  (~83000 → "projected far below screen, never bottom-clipped"). **(2)** env-gated *fast-forward
  repro* toggles that drive the exact failing path headlessly when interactive geometry won't
  reproduce: BoB `BOB_POSTLOAD_FF`/`BOB_POSTMISSION_FF` (drive the loaded/post-mission world under
  ASan), MA `MA_FORCE_PADLOCK` + `BOB_AUTOFLY=sweep` (force the view + wild stick). A structural fix
  you can't *reliably* trigger is still validated by the bound itself; the repro confirms it.

---

## 7b. Bug-class taxonomy — check every new symptom against this list **[PROCESS]**

_(Imported 2026-07-19 from the **FreeFalcon 6** Linux port at `~/free-falcon`
(`docs/COMPLETION_PLAN.md`), which is an unrelated codebase — Falcon 4 lineage, 64-bit, no MFC,
its own UI toolkit — but hit the same Windows→Linux classes we do. It maintains a fixed, numbered
list that every new symptom is triaged against before anyone starts guessing, and it runs a whole
sprint sweeping each class to exhaustion. That habit is the transferable part; the per-class notes
below are marked for how much each one actually bites a 32-bit Rowan port.)_

1. **32-bit `long`/`ulong` in Windows binary file formats** → use `int32_t`/`uint32_t`.
   *Bites us less* — we build `-m32`, so `long` is already 4 bytes. Still relevant if either port
   ever goes 64-bit, and the underlying discipline (fixed-width types at every file boundary)
   is right regardless. FreeFalcon found it on the **write/Encode** side long after the read side
   was fixed — audit both directions.
2. **64-bit pointer truncation via `(int)`/`(DWORD)`/`(GLint)` casts** → use `intptr_t`/`uintptr_t`.
   *Does not bite us at `-m32`* (this is the mirror image of our own #1 recurring bug, the
   pack-struct ABI boundary in §2). Listed so nobody re-derives it if a 64-bit build is ever attempted.
3. **`sizeof(DDSURFACEDESC2)` read from a 124-byte on-disk DDS header.** *Applies directly* to any
   DDS loading. FreeFalcon fixed this in three separate files months apart — grep for
   `sizeof(DDSURFACEDESC2)` used as a **file read length** and fix them all at once.
4. **MSVC `RAND_MAX`==32767 assumed against glibc `rand()`.** *Applies directly, and is nasty
   because it degrades silently rather than crashing.* In FreeFalcon this made radar detect ~0.03%
   of beam crossings and flak ~65,000× too weak — it read as "the AI is broken", not as a port bug.
   Grep for literal `32767`/`16000` in probability or scaling expressions.
5. **CRLF left on the last token after text-mode `fgets`.** *Applies directly* — Windows strips
   `\r`, Linux keeps it, so every trailing token silently fails to match a name table. FreeFalcon's
   instance made **all** particle effects invisible.
6. **Silently default-returning compat stubs.** *Applies directly and is the highest-value entry
   for us.* A stub that returns 0/TRUE instead of failing loudly degrades behaviour invisibly:
   FreeFalcon's `GetPrivateProfileInt` stub zeroed every `.ini` tuning value in the game, which
   caused a campaign aggregation-flap storm (48,843 messages/35s → 37 after the fix). **We have
   live instances of exactly this shape:** both ports' registry functions are failure stubs and
   `WritePrivateProfileString` is a no-op returning TRUE (§below and the compat headers). Those are
   deliberate — but they should be *audited and listed*, not merely assumed harmless.
7. **Signal-less infinite waits / lock-order inversions.** *Applies directly.* Any
   `WaitForSingleObject(INFINITE)` whose signaller only runs in a mode you are not currently in
   will hang; FreeFalcon hit this twice (campaign thread parked forever in UI mode; an AB-BA
   deadlock at shutdown). Related discipline worth stealing: they keep `[CLEANUP]` markers
   **always on in release**, so the last line printed localises any shutdown hang.
8. **OpenGL state-at-call-time vs D3D state-at-draw-time.** *Applies directly and repeatedly* —
   this is the same family as our own clear-mask and clear-region findings. `glClear` honours
   `glDepthMask`/`glStencilMask`/scissor where D3D's `Clear` does not, and D3D `Clear(0,NULL,…)`
   clears only the **current viewport** while `glClear` takes the whole framebuffer. Also in this
   class: `glLightfv(GL_POSITION)` bakes the modelview current *at call time*, and DXT1 must be
   uploaded as the RGBA variant to keep punch-through alpha.

**The practice, not just the list:** when a symptom appears, walk these eight before forming a new
theory. When one is confirmed, sweep every other site of that class in the tree in the same pass —
all three ports have repeatedly found that a class fixed in one file is still live in two others.

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

## 8b. R* ActiveX toolbar buttons + sprite-sheet icons (BoB S88–S92) **[ENGINE]**

_(Added by the BoB session 2026-07-05. MA already hosts RButton (`ma_olebutton.cpp`); the new
part is the button-**art** resolution.)_

The strategic-map toolbars are docking `CRToolBar` (CDialog) bars that host **`CRButtonCtrl`**
ActiveX buttons — the same R* OCX family as RListBox/RCombo/RStatic. To render + click them on
Linux (MiG Alley's map toolbars are almost certainly the same):
- **Host the OCX like the others** (compile `RBUTTONC.CPP` into the R* lib + a small host that
  routes the button dispids 0x1–0x15 + stock fore/back/caption). Compile shims needed:
  `CDC::DrawIcon`, `COleControl::OnKeyDownEvent`, `ID_HELP`, and a named-temp for a `MaskIcon`
  `CPoint&`-rvalue bind.
- **Button art is a FileNum via `WM_GETFILE`.** `CRButtonCtrl::OnDraw`→`DrawBitmap` does
  `GetParent()->SendMessage(WM_GETFILE, filenum)` → "BM" bytes → `SetDIBitsToDevice`. Back
  `WM_GETFILE` for the file range with `fileblock`/`getdata`, and give `SetDIBitsToDevice` a
  settable viewport origin (the HDC is a sentinel, so it otherwise blits to (0,0)).
- **Most toolbar icons are SHEET regions, not files.** They resolve through the `IconsUI`
  page/entry enum (`ICON_PAGE_1=0x10000 + iconnum.g index`) and draw via the *map-icon* path
  (`OnDraw` transparent branch → `WM_GETFILE` returns an `IconDescUI` → `MaskIcon`). Set the
  button's `NormalFileNum` to the ICON_PAGE value, not the (often renamed/absent) per-file art.
- **The `.rc` DLGINIT often defaults many buttons to one shared art string** (BoB: all →
  `FIL_ICON_BASES`); the shipped game differentiates each at runtime. If that runtime assignment
  is missing in your source drop, reconstruct a control-id→icon map (1:1 by function, matching
  `iconnum.g`). — **NB (BoB-drop-specific):** BoB's `F_GRAFIX.G` had `FIL_ICON_* → FIL_xICON_*`
  renamed with the art at those FileNums absent (`.rc` and `F_GRAFIX.G` came from different
  builds). Check your own `F_GRAFIX.G` against your `.rc` before trusting the per-file FileNum;
  the ICON_PAGE/sheet route sidesteps the problem entirely.
- **Clicks** fire via the S33 eventsink: hit-test the button's drawn rect, then
  `bob_evt_fire(toolbar, &typeid(*toolbar), ctrlId, /*Clicked*/1)` → the registered `ON_EVENT`
  thunk (`OnClickedBases`/…). Handlers open logged-child sub-dialogs (`LogChild(::Make())`).

## 8c. Strategic-map interaction: bypass the never-delivered window messages (BoB S94–S97, MA-originated) **[ENGINE]**

The map's `WM_*SCROLL` / `WM_LBUTTON*` / arrow / wheel messages **never reach `CMIGView`/`CMapDlg`** on
either port (no real message pump). **MA's pattern (adopted by BoB): capture input in the SDL layer and
drive the map's own state from the map idle/tick**, not the MFC handlers. What BoB wired this way:
- **Pan/zoom (S96).** SDL pump accumulates wheel/arrows/`+`/`-`; the map tick calls a helper that shifts
  `m_scrollpoint` (pan) / scales `m_zoom` (zoom) then calls the game's **`Zoom()`** to re-clamp (scroll
  bounds + zoom min/max + full-screen-min). **BoB gotcha worth copying:** below `ZOOMTHRESHOLD3` the map
  **quantises `m_zoom` to `0.25*2^n`**, so a fractional zoom step (×1.25) snaps back — use the game's own
  discrete step (`m_zoom*2` / `/2`, as `OnZoomIn/Out` do) about the screen centre.
- **Accel/time clock (S94).** The map starts (should start) **paused**; if your boot scaffold leaves
  `curracceltype` at a running value, force `ACCEL_PAUSED` at `LaunchMap`. Drive `CMapDlg::OnTimer`
  (`bob_drive_timer`) each tick **only when not paused**; the accel OCX buttons (Play/Pause/FF) set
  `curracceltype` via the eventsink (§8b clicks). Guard the post-mission SAG grind.
- **Unit selection (S97).** A map click → the game's own **`CMapDlg::FindMapItem(point)`** returns the UID
  under the cursor (does the world-coord transform + icon hit-test for you). Band-dispatch it
  (`SagBAND` squadron / `WayPointBAND` waypoint / airfield) and call **`SetHiLightInfo(pack,sq,…)`** to
  highlight its route — the same feedback `OnClickItem` gives, minus the OOB info sub-dialogs (the
  `MakeTopDialog`/`DialBox`/`HTabBox` framework — defer, and heed §8d's dangling-`Edges` caveat when you
  build it: whole tree in one full-expression, never a named-local `DialBox` + inline `EDGES_`).

---

## 8d. `DialBox` stores `&edges` → dangling `Edges` on named-local DialBoxes (MA S41) **[ENGINE]**

_(Added by the MA session 2026-07-05. Shared **dialog-framework** bug — both ports use `RDIALOG.H`
`DialBox`/`Edges` + the `EDGES_*` macros.)_

`RDIALOG.H`: `DialBox::edges` is a `const Edges*`, and the ctor does `edges = &e` (stores the address of a
caller-supplied `Edges`). The `EDGES_*` macros (`EDGES_NOSCROLLBARS_NODRAGGING`, …) expand to an
`Edges(...)` **temporary**. So:
- **Named-local DialBox** — e.g. `DialBox topbit(FIL_NULL, …, EDGES_NOSCROLLBARS_NODRAGGING);` on its own
  statement — the macro temporary dies at the **end of that declaration statement**, leaving `topbit.edges`
  dangling. When the panel builder later reads `*diallist[i]->edges` (`AddChildren`, `RDIALOG.CPP:537`) it's
  a **stack-use-after-scope** (ASan; UB but usually intact bytes on Windows). MA hit this in `FragInit`.
- **DialBox temporaries inside a single full-expression** (`LaunchDial(0, DialList(DialBox(…EDGES…), …))`)
  are **fine** — all temporaries live to the end of that full-expression, which spans the `AddChildren` call.

**Fix (MA):** give the `Edges` function-scope lifetime — a named `const Edges` local declared before the
DialBox (localised; no change to `RDIALOG.H`). A general fix would be to store `Edges` **by value** in
`DialBox`, or make the `EDGES_*` macros program-lifetime `static const` objects. **Where to look:** any
`MakeTopDialog`/panel builder with a *named-local* `DialBox` built from an inline `EDGES_` macro.

**Safe idiom (BoB, forward-looking for the OOB/dossier dialogs):** build the whole dialog tree in **one
full-expression** — `MakeTopDialog(DialList(DialBox(…EDGES_…), …))` — so every `Edges` temporary lives for
that entire statement. The moment you hoist any `DialBox` to a **named local** with an inline `EDGES_`, its
`Edges` dies at the semicolon. (BoB has zero live sites today — every `EDGES_*` use is a temporary in a
full-expression or an `AddPanel(dial,…,const Edges& e)` param whose temp spans the call — but the
`Bases`/`Squadrons`/`MissionFolder` OOB dialogs use `MakeTopDialog`/`DialBox`/`HTabBox`, so this idiom is
the rule to build them by.)

---

## 8e. Front-end screen parity: the 2D layout keystones (BoB S123) **[ENGINE]**

BoB ran a gold-standard screen sweep (Wine reference captures vs native GDI dumps) and found
three engine-level layout keystones, each a small compat fix with screen-wide effect. MA hosts
the same FullScreen/OCX front-end — check each:

- **`FullScreen::Resolutions::ListX/ListY` is the menu-list anchor — use it.** Every screen
  ships per-resolution list coordinates (e.g. BoB campaignselect@1024 = (35,710): Back/Begin
  bottom-left; title = (210,220); config tab rows = (10,10)). A synthetic top-centre /
  left-column anchor puts Back/Begin/Fly rows at the top of the screen; the authored data
  puts them exactly where the Windows build draws them. One read, whole-product placement fix
  (BoB `bob_draw_menu`, `BOB_NO_LISTXY` reverts).
- **Scope control-rect lookups to the owning dialog id.** Combo/list control ids are unique,
  but STATIC label ids repeat across dialog templates. An unscoped by-id rect lookup returns
  the first match across ALL parsed dialogs → labels take other screens' rects → scrambled/
  overlapping config forms. The fix is using the (dlgId, ctrlId) scoped lookup everywhere the
  per-control draw runs (BoB already had the scoped table for toolbars; the panel draw was
  still unscoped). Track the owning IDD on the host at CreateControl time.
- **Track runtime `ShowWindow` on hosted controls.** The game hides off-page/disabled
  controls with `ShowWindow(SW_HIDE)` (e.g. CSQuick1's IDC_DISABLEDEMO "This is disabled in
  the demo", radio hides). A no-op ShowWindow draws every ghost. Forward CWnd::ShowWindow to
  the host (`visible` flag), skip hidden hosts in draw + zero their click rects. NB the same
  class remains for `MoveWindow` (page-switch dialogs reposition controls at runtime — BoB's
  Quick Shots pages still overlap until MoveWindow is tracked).
- **Deterministic capture harness:** `BOB_SHOT=<n>` + `BOB_SHOT_PATH` — after n front-end
  ticks / map paints, dump the GDI framebuffer and `_exit(0)`. Headless-safe (SDL dummy),
  private dump path (shared /tmp!). Turns every screen into a scriptable one-command capture;
  the whole 15-screen sweep runs unattended.
- **Check WHICH resources your parity oracle runs.** BoB's gold captures are the BDG 0.99
  *patched* build: its dialog layouts/labels/string table differ from the 2000 source
  checkout's .rc that the port parses at runtime. Label-text "deviations" can be pure
  resource-version deltas — decide the oracle (and consider parsing the installed exe's
  .rsrc) before chasing them as render bugs.

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
