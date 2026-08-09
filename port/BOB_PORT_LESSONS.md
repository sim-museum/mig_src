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

## 8f. PE `.rsrc` DIALOG/DLGINIT from the installed build — the parity-oracle resource layer (BoB S124, MA-adoptable design) **[ENGINE]**

BoB closed its biggest screen-parity root cause (§8e last bullet: the resource-version delta)
by making the INSTALLED build's resource DLL the runtime source of dialog layout + captions.
This section is the adoption design for MA — every piece has a verified MA-side twin.

**Where the resource DLL lives (both games identical in shape).** The engine loads one
language/resource DLL at startup: `MIG.CPP` `resourceInst = LoadLibrary(File_Man.
NameNumberedFile(FIL_LANGRESOURCEDLL,…))` (BoB MIG.CPP:511, MA MIG.CPP:558; the slot is
`FIL_LANGRESOURCEDLL = 0x7101` in `H/F_COMMON.G`, bound in `MASTER.FIL`). That resolves to
`English/TEXT/boblang.dll` for BoB and **`English/TEXT/miglang.dll` for MA** (verified on this
box: `~/sgl/TUE/MigAlley/WP/drive_c/rowan/mig/English/TEXT/miglang.dll` — 132 RT_DIALOG,
111 RT_240 DLGINIT, 300 string-table blocks; BoB's boblang.dll: 150/135). It is the DLL the
Windows build actually ran — i.e. **exactly the data your parity gold shots show**, patched
or not (BoB's is the BDG 0.99 patch; that's what made it the oracle).

**You already have the loader — don't write a PE parser.** Both ports carry
`SRC/compat/bob_resources.cpp`, which since early sessions maps the DLL, walks the `.rsrc`
directory (rva2off / res_find_entry / res_leaf) and serves `bob_load_string` (RT_STRING) —
MA's copy is confirmed at the same pre-S124 state BoB's was. The S124 story reduced to **two
enumerators on that existing loader** (~140 lines) plus consumer-side work:

```c
/* RT_DIALOG (type 5): fire cb per control of every DIALOG template */
int bob_res_enum_dialog_items(
        void (*itemcb)(void* ctx, int dlgId, int ctrlId,
                       int x, int y, int w, int h,   /* dialog units */
                       const char* cls),             /* "{CLSID-…}" or atom "#0082" */
        void* ctx);                                  /* -> item count */
/* DLGINIT (type 240): per-dialog stream of {WORD ctrlId, WORD msg, DWORD len, bytes} */
int bob_res_enum_dlginit(
        void (*initcb)(void* ctx, int dlgId, int ctrlId,
                       const unsigned char* data, int len),
        void* ctx);
```

Parsing rules that matter (all learned the hard way; BoB `bob_resources.cpp`
`dlg_enum_one`/`init_enum_one` is the reference implementation):
- **Offset-based reads only** (`rd16`/`rd32` on a byte pointer) — never overlay a struct on
  the resource bytes. This is the §2 pack-ABI lesson again: DLGTEMPLATE layouts are
  2-byte-packed on disk and a compiled struct overlay reads garbage.
- Handle **both DLGTEMPLATE and DLGTEMPLATEEX** (detect via first two WORDs `1, 0xFFFF`);
  header field offsets and per-item layouts differ (classic: 18-byte item header, WORD id;
  EX: 24-byte, DWORD id).
- Each item starts **DWORD-aligned** (`o = (o+3) & ~3`).
- Menu / class / title / font-face are **sz_Or_Ord** fields (0x0000 none, 0xFFFF+WORD
  ordinal, else UTF-16 sz) — skip them properly or every later offset is wrong. Font block
  exists only when `DS_SETFONT` (0x40) is in style; EX adds weight/italic/charset WORDs.
- **Creation-data WORD semantics differ**: a nonzero classic count INCLUDES its own size
  WORD; an EX count EXCLUDES it. Get this wrong and you desync mid-template.
- The R* OCX items carry their coclass as a **"{CLSID}" class string** — classify by the
  first GUID field (BoB: C42BAC3D→RStatic, 737CB0C9→RCombo, 48814009→RListBox,
  78918646→RButton; MA's CLSIDs are in its wrapper classes' `GetClsid()`).
- DLGINIT payload bytes are **the same format the .rc text hex dump encodes** (msg 0x376
  property-bag records) — feed them to the caption/artname extractors you already have for
  the .rc parse; no new decoder.

**Wiring: PE-first, .rc fallback, one escape hatch.** In the dlgtemplate loader, run the PE
enumeration FIRST into the same rect/caption tables the .rc text parse fills, marking each
entry `pe`; then let the .rc parse run as fallback only — **never overwriting a PE entry**.
One env (`BOB_NO_PE_RSRC`-style) reverts the whole layer for A/B and for flipping the oracle
ruling cheaply. Side win: installed-build dialogs no longer need the source checkout at all
(packaging).

**Template-driven static hosting — the lesson that was NOT a resource delta.** On Windows the
dialog manager creates **every item in the template**. If your control creation is DDX-driven
(instantiate on `DDX_Control`), you silently miss every control the dialog class never binds —
label statics above all: BoB's `SMissionConfigure` binds its 8 combos and ZERO statics, so the
whole Mission tab rendered label-less and no resource fix could help. Fix shape:
`bob_ole_host_template_statics(dlg, dlgId)` called in `CDialog::Create` **between
DoDataExchange and OnInitDialog** — for each template RStatic id no DDX bound (from a
`bob_dlg_enum_statics` over the PE items), host it on a synthetic wrapper CWnd registered in
the normal host side-table, so `GetDlgItem`/`ShowWindow` reach it and `OnInitDialog` can
already talk to it.

**Template-membership draw filter (the inverse lesson).** A control your source-derived
tables know but the installed build's template for that dialog DOESN'T contain would never be
created by the Windows dialog manager — don't draw it. `bob_dlg_in_template(dlgId, ctrlId)`
(1 in / 0 absent / -1 unknown dialog), applied in the panel draw path only (BoB deliberately
left the toolbar path unfiltered). This killed BoB's overlapped Sound-tab label + stray
combos (BDG dropped the source's music combos) and the Quick-Shots page-ghost combos.

**Caption text: resolve IDS→string table, don't trust the DLGINIT literal.** The genuine
`CRStaticCtrl` resolves its runtime caption via `WM_GETSTRING(ResourceNumber)` → LoadString
from the language DLL (RSTATICC.CPP `GetParentWndInfo`); the DLGINIT string literal is
design-time only and goes stale ("Trees etc" vs the shipped "Town and forest raises").
Capture the persisted `IDS_*` name from the property bag, resolve it through RESOURCE.H →
`bob_load_string` (the DLL's string table), fall back to the literal.

**Oracle process note.** Record the ruling ("parity is judged against the gold shots as-is =
the installed/patched build's data") in the parity doc, tag every provable
patched-vs-source deviation per-shot, and keep the one-env revert — then the PO can overturn
the ruling for the cost of an env var, not a rewrite. Residual deltas that live in patched
**code** (not resources — e.g. BDG's "BDG 0.99" title item, combos the 2000 source has no
member to bind) are data-unfixable: tag them as waived rather than chasing them.

**Already-PE-first ports: the lessons transfer, the parser doesn't** *(MA S57 transfer
report, note 15)*. A port that always read the installed modules' RT_DIALOG/RT_DLGINIT
(MA's `ma_dlgtmpl.cpp` since its Phase 4) gets ~0 value from this section's parser and
~all of it from lessons 3–4 + the membership filter — on MA, template-driven static
hosting alone explained every "missing label" row (the unbound-static sets matched the
parity table exactly). Split-module resources (miglang.dll + Mig.exe): walk the language
DLL first, then the EXE **skipping dialog ids the DLL already served** (~20-line
dialog-granularity dedup). And the one-env revert doubles as *permission to merge
unverified render-path changes* when the display is blocked — the hatch is the safety
argument.

**DLGINIT design props beyond captions (BoB S125).** The bags also persist LAYOUT the
hosts must not lose: RListBox authored columns (`A0..A8` widths + `C0..C8` align/icon
codes — the last 54 bytes of a version&0x4 bag) and the R-button caption alignment
(`ResourceNumber` bits 24..31: 0 centre / 1 left / 2 right). An empty-`CPropExchange`
host boot silently drops these (symptom: tab rows tight-pack; label/value pairs that
Windows shows adjacent scatter to their raw rects). Offset-anchored extraction of just
these two is cheap and gold-exact; apply button alignment to artless caption buttons
only (art/hint buttons' bags put the hint string where the caption anchor looks). The
full fix is a sequential property-stream reader feeding each host's genuine
`DoPropExchange` — that also carries FontNum/colors (the gold large faces).

**PX defaults are load-bearing — no-op `PX_*` stubs leave persisted members as heap
garbage (MA S58, note 16).** The R* OCX ctors initialise only some members; the rest
(`m_bLockTopRow`/`m_bLockLeftColumn`/`m_bBlackboard`/`m_bLines2`/`m_bSelectWholeRows`/
`m_bDragAndDrop`/`m_border`/`m_bCentred` + the colour set on `CRListBoxCtrl`, and the
analogous tails on the other controls) are set ONLY inside `DoPropExchange` via `PX_*`
defaults — Windows always runs it, so the 1999 code never needed ctor inits. A port
whose compat `PX_*` are `{ return TRUE; }` no-ops (and whose `COleControl::OnResetState`
doesn't drive `DoPropExchange`) leaves them **uninitialised**, and the garbage is
**environment-dependent** (heap layout differs SDL-dummy vs GL-window vs machine):
MA's prefs tab bar drew a black band + clipped rows *only headless*, and its title menu
drew doubled captions — long mis-filed as a font-path delta. Fix shapes, either works:
(a) ctor-init every `DoPropExchange`-persisted member to its PX default (MA,
`RLISTBXC.CPP`); (b) a real `CPropExchange` whose `PX_*` write the default when
unattached or on stream error (BoB) — if you have (b), that default-writing property is
the load-bearing part; keep it on every control-creation path. Acceptance test that
catches the whole class: **a headless-dummy capture must be byte-identical (`cmp`) to a
GL-run capture of the same screen at the same idle.**

**The full sequential property-stream reader is cheap and closes the whole design-prop
class (BoB S126, note 17).** The complete persisted-stream layout, validated against all
1280 R\*-class RT240 bags in boblang.dll (zero parse failures):
`[DWORD licence-wchar-count][UTF-16 licence][DWORD version][DWORD extentX][DWORD extentY]
[DWORD stockPropMask]` — mask&0x02 Caption (MFC CString archive: BYTE len, 0xFF→WORD len,
0xFFFF→DWORD len; the 0xFFFE unicode marker never occurs → treat as bad), 0x08 ForeColor
DWORD, 0x01 BackColor DWORD, 0x40 Enabled BYTE (unknown mask bits → abort to defaults) —
then the control's own PX_\* fields in DoPropExchange SOURCE ORDER (Bool=BYTE, Short=WORD,
Long/Color=DWORD, String=CString archive); trailing bytes are editor slop, unread, exactly
as on Windows. Read the persisted version DWORD *into* the exchange (it gates the
controls' `GetVersion()&x` tail branches) — don't substitute the control's own default.
Three traps found landing it: (1) **persisted colors are COLORREF-order (0x00BBGGRR) —
convert exactly once** at the seam where your framebuffer text draw expects RGB; the
authored values are gold-exact (BoB's phase-select date `(183,250,255)` matched the gold
PNG pixel-for-pixel — and sample the ORIGINAL gold PNG, not a JPEG composite, before
ruling a color wrong). (2) **persisted Normal/PressedFileNum art indices are design-time
file-table indices from the AUTHORING install — meaningless against the runtime file
table**; restore them to their boot defaults after the replay and resolve button art by
NAME. (3) a static under an interactive listbox is WS_VISIBLE in the template yet absent
from Windows' settled screen (the listbox's first repaint re-blits panel art over it and
the static is never re-invalidated) — an every-frame panel redraw must emulate the
settled state (BoB: skip statics ≥90% covered by a sibling hosted listbox) or the
"duplicate caption" class returns. Acceptance: the dummy==GL `cmp` bar above passed
FIRST TRY on BoB after the reader landed — evidence that fix shape (b) (default-writing
`CPropExchange`) composes with a real stream reader with no garbage window on any
creation path.

**Dialog templates hide controls TWO ways the host must route (MA S59, note 17 —
closed MA parity #9).** Both live in RT_DIALOG bytes the parser already walks past:
(1) **per-control WS_VISIBLE** — Windows creates `!WS_VISIBLE` template controls
HIDDEN; only a runtime `ShowWindow(SW_SHOW)` reveals them. Route the style dword
(classic item: style at +0; EX item: at +8) as the control's INITIAL show state, with
runtime ShowWindow still overriding — a mis-read here masquerades as a "resource
delta" (MA's phantom "I.D." label, IDD 287 id=2023, style 0x40010000). (2) **parent-
rect clipping** — Windows clips children to the dialog's own client rect (header
cx/cy), so a control parked FULLY OUTSIDE it can NEVER paint whatever its show state;
designers park dead controls there (MA's Quick Mission stray cluster: dead-coded
Cloud/Weather combos at dlu x=367–389 on a 335-dlu dialog). Route as a draw/click
filter beside the template-membership filter. Related same-sprint finds, same bar:
compat `CDC::DrawText` must implement real `DT_WORDBREAK`/multi-line (CRStaticCtrl
draws all long prose through it — unwrapped text running off a panel edge is this,
not a layout bug); and DEVICE PRESENCE must not depend on the video backend — the DI
system mouse (`GUID_SysMouse`) always exists on Windows, but MA enumerated it only
when the SDL window existed, splitting dummy vs GL captures by one combo row
("Keyboard" vs gold's "active mouse : X-Axis & Y-Axis") — the second bug class the
dummy==GL `cmp` bar has caught that eyeballing could not.

## 8g. Dialog TREES: template-declared controls, and the size nothing sets (MA S60) **[ENGINE]**

Both ports share `RDIALOG.CPP`'s `MakeTopDialog` / `AddChildren` / `HTabBox` machinery.
Two gaps in it stay invisible until you render a *tabbed* out-of-band dialog, and one of
them silently zeroes the geometry of every dialog tree. Found chasing MA's campaign-map
Player Log (gold shot #15); three of its four named deviations were these two gaps.

**(1) A template control that no dialog class `DDX_Control`-binds is never created.**
§8f's lesson — the Windows dialog manager creates *every* template item, DDX-driven
creation misses some — was implemented in both ports **for RStatic only**. It is not
static-specific. `IDD_PLAYERLOG` declares `IDJ_TITLE` (1001) as an **RButton** (the title
bar) and `IDD_EMPTYPAGE` declares `IDJ_TABCTRL` (1002) as an **RTabs** (the tab bar);
`RDEmptyP::DoDataExchange` is empty, so neither existed. `AddChildren` and
`AttachTabToTabControl` both then take their `"No tab control exists"` early-out — no tab
bar, no title bar, tab pages never attached. Fix: drive the template hoster from a KIND
TABLE (RStatic/RButton/RTabs …), with a per-kind "needs a caption to be worth creating"
rule — a tab bar legitimately starts empty. Keep the kind enum in ONE shared header; it is
compared across TU boundaries. *Corollary worth auditing: MA also found **RScrlBar
`505aee46` created 16× and completely unhosted** — it falls through the CLSID chain into
the generic bucket and no-ops, which reads as "the list just doesn't scroll".*

**(2) ★ No RDialog in a tree ever learns its own size.** `RDialog`'s ctor zeroes
`homesize`/`viewsize` and the line that would refresh them from the client rect is
**commented out in the shipped source** (`RDIALOG.CPP:147`, `DoDataExchange`). Harmless on
Windows — MFC made a real window at template size, so `GetClientRect` answers. In a port
whose `CDialog::Create` sets no size, every RDialog answers **0x0**, and the tree builders
derive everything from that one call: `MakeParentDialog`'s "if dialsize is meaningless"
branch runs on a 0x0 rect (tree never lands where `Place()` asked), `AddChildren` sizes
children from `homesize.Width() == 0`, and `RDialog::OnSize` hands `IDJ_TABCTRL` a
zero-width `MoveWindow` that any sane draw loop then skips. Fix: export the RT_DIALOG
cx/cy your template parser already reads (§8f) and seed
`m_maW/m_maH`/`homesize`/`viewsize` from it, filling only values still at their zero
default so an explicit `MoveWindow` still wins.

> **Scope (2) to the tree builders, NOT to `CDialog::Create`.** MA tried `Create` first and
> it broke the front end — canvas 644 -> 600, Load-panel art bleeding into the map —
> because `Create` is shared with the full-screen panels, which size themselves by other
> means. Apply it at the `Create` call sites inside `MakeParentDialog`/`AddChildren` only,
> and keep a comment saying why; it looks like something worth hoisting into `Create` later.

**(3) OOB paint walks must not stop at the first art-bearing node.** A walk that descends
`fchild` until `artnum != 0` skips every art-less node — and art-less nodes own controls
(the title bar and the tab bar, above). Recurse over siblings+children and render every
**visible** node instead: the engine already `SW_HIDE`s the non-selected tab pages in
`AddChildren`, so honouring the visible flag reproduces "first tab only" by the engine's
own mechanism rather than by truncating the walk.

**(4) An OCX's bitmaps live in the OCX.** Inside `CRTabsCtrl::OnDraw`,
`m_TabUp.LoadBitmap(IDB_TABUP)` resolves against *RTabs.ocx* — `AfxGetInstanceHandle()` is
the control's own module, not the game .exe. No game-resource lookup can serve it. The
`.ocx` files ship in the install dir: load one as an extra PE module through the §8f
resource layer, hand the control ready-made memory DCs, and clear its `m_bInit` so
`OnDraw` skips its own load. Same trick for any R* control art that is missing.

**(5) The uninit hazard is wider than PX-persisted members** (extends §8f's note-16
lesson). `CRTabsCtrl` *does* init its only `PX_*` member — but its four `HICON`s are never
assigned at all (every `LoadImage` in the ctor is commented out), so `DrawIconEx` gets
uninitialised heap on Windows too. Any ctor-skipped member that reaches a draw call is the
same bug class; NULL them so the resulting no-op is deterministic rather than
environment-dependent.

## 8h. Dialog trees, part 2: WHERE they land (MA S61) **[ENGINE]**

§8g got template controls created and sized; these four are why they still drew in the
wrong place. All found on MA's Player Log; the first three are in shared `RDIALOG.CPP`
code and the fourth is a Win32-stubbing trap that applies port-wide.

**(1) ★ `ClientToScreen`/`ScreenToClient` are no-ops, and `OnGetXYOffset` is built on
them.** The stock body derives its offset entirely from those two calls, so with them
inert every subtraction is `0 - 0`: **every dialog reports offset ~0 and the whole tree
composites at the top-left.** Grep your compat `afxwin.h` before assuming a layout bug.
Fix by replacing `OnGetXYOffset`'s body (accumulate `m_maX/m_maY` up the RDialog `parent`
chain) rather than by giving `ClientToScreen` global screen semantics — panel code relies
on the identity behaviour, the same trap §8g flagged for `CDialog::Create`.
**Drop the title-bar nudge when you do**: in the accumulated form the title is already
counted (children sit in a placeholder that starts below it), and because the nudge is
gated on `top->fchild->artnum == artnum` it applies to an art-less tab host but not to an
art-bearing tab page — shifting them apart by the title height so the page art paints over
the tab strip. Presents as a z-order bug; is not one.

**(2) ★★ Unchecked `RegQueryValueEx` + uninitialised locals ⇒ a different dialog origin
every run.** Startup reads `Control Panel\desktop\WindowMetrics` into a local
`buff`/`type` and never checks the return code — fine on Windows, but the port's stub
returns `ERROR_FILE_NOT_FOUND` and writes neither, so `if (type==REG_SZ)` and
`*(int*)buff` both read uninitialised stack. `RDialog::borderwidth` (and
`actscrw`/`actscrh`) come out as garbage, and borderwidth feeds `MakeParentDialog`'s
sizing ⇒ **every top-level dialog tree gets a garbage origin, different on each run**.
The run-to-run variance is the tell (MA measured (978990,978859) then (979004,978793));
a wrong-but-stable value sends you hunting a logic bug. Zero the locals or check the
return. **Generalise:** §8g(5)'s uninit lesson extends beyond ctor-skipped members to
*any local passed as an out-param to a stubbed Win32 API and read without checking the
return* — sweep `RegQueryValueEx`, `GetVersionEx`, `SystemParametersInfo`, `GetDeviceCaps`.

**(3) `IDJ_PANEL0..9` placeholders must be registered or children stack below the parent.**
`AddChildren` uses `GetDlgItem(IDJ_PANEL0 + i)` to locate where each child dialog goes and
falls back to "stack below" when it is missing. These are plain NATIVE template controls,
so §8g's OCX-kind hosting does not cover them. Register a bare `CWnd` at the template rect
— found by `GetDlgItem`, hosted against nothing, never drawn. On MA's Player Log this
moved the tab box from y=396 (off the bottom of a 400px dialog) to y=27.

**(4) A newly-correct rect can change unrelated rendering.** Fixing (2) requires the view
window's rect, which was 0x0. Syncing it from the canvas also changed the campaign map:
its tile loop consults the view client rect and drew one more tile row straddling the
bottom edge, which Windows clips but an auto-growing screen canvas does not — the capture
silently went 1021x644 -> 1021x900. Scope such a sync (RAII restore) to the computation
that needs it.

**Process note that earned its place:** both MA S60 and S61 changed wide-blast-radius
files (`afxwin.h`, `RDIALOG.CPP`, `MIG.CPP`). Re-capturing the standing 2D parity screens
headless and `cmp`-ing them against committed references BEFORE commit caught (4) above
and, in S60, a `CDialog::Create`-wide change that disturbed Preferences/Load. Put the
screens you are NOT working on in the gate.

## 8i. Property-stream reader: adoption notes + what enabling it breaks (MA S62) **[ENGINE]**

MA adopted BoB's S126 reader (§8f) essentially verbatim. It parsed **58/58 MA bags clean**
(`ok=1`, ≤8 bytes slop) — the layout transfers between the two ports' resources — and it
reproduced BoB's FONT/COLOR win: MA's Preferences went from white-serif labels to gold's
**blue labels + yellow values** in one change, solving the colour half of MA's biggest
cross-cutting deviation. Three things the second adopter learned:

**(1) Decide per-port whether to APPLY each stock prop, not just parse it.** MA consumes
but does NOT apply two of the four: **Caption**, because MA's persisted captions are
`IDS_*` SYMBOL NAMES (`"IDS_MIGALLEY"`) that the §8f IDS→string-table path already
resolves to the *shipped* wording — applying the raw value overwrites a correct caption
with a symbol name; and **BackColor**, because MA's hosts composite over panel artwork and
treat control backgrounds as transparent. Both still consume their bytes to keep the
stream aligned. Likewise **trap 1 (COLORREF convert-once) must be evaluated, not applied
blindly** — MA's OLE_COLOR is already 0x00BBGGRR end to end, so converting would have been
the very double-conversion the trap warns about. Also needed: a
`PX_Bool(..., short&, BOOL)` overload, since some R* controls declare a persisted bool as
`short` (same BYTE on the wire).

**(2) ★ Enabling the reader changes FONT METRICS, which breaks fixed-coordinate test
recipes.** The persisted `FontNum` is load-bearing: on MA it moved the title-menu row pitch
from ~16px to ~28px, so every `BOB_CLICKSEQ`-style recipe driving the UI by fixed pixel
coordinates landed on the wrong row — MA's `quickmission` capture came back showing
*Preferences*, and the campaign recipe never reached the map. That invalidates the parity
capture recipes AND the ASan drive recipes **at the same time**, i.e. exactly the
regression gate you need while making a change this wide. Two consequences worth
inheriting: land the reader **opt-in** so the default path stays byte-identical and the
gate stays trustworthy; and make headless recipes drive the UI by **menu-row index rather
than pixel coordinates**, so they stop being font-dependent at all.

**(3) The unattached PX_* path changes behaviour too.** Stub `PX_*` returning TRUE leave
the member untouched; real ones write the declared default. Anywhere a ctor set a better
value than the PX default will silently change. (MA's leading hypothesis for a
run-to-run-varying uninit read that appeared when the reader was switched on.)

**Corollary on oracles, from the same sprint:** a parity reference that shows ENUMERATED
HARDWARE is not stable. MA's `prefs_controls` shot was captured with a joystick attached
and stops matching on a box without one — an environmental diff that reads exactly like a
regression. Check the hardware before believing it. This is §8f's device-presence lesson
one level out: there the PORT's enumeration varied by video backend; here the ORACLE
varies by machine.

## 8i. Enabling the property reader: one more uninit mechanism, and the pixel-recipe trap (MA S62-S63) **[ENGINE]**

MA adopted the S126 persisted-property reader (§8f). It lifted essentially verbatim — all
58 bags on MA's boot path parse clean — and the payoff reproduced: setting VALUES yellow
exactly as gold, tab bar yellow, labels out of GDI-fallback white into gold's blue family.
Two things surfaced on the way that are engine-level, not MA-specific.

**(1) ★ `WM_GETSTRING`'s OUT param is unchecked in three R* call sites.**
`CRButtonCtrl::GetParentWndInfo` (x2: caption + hint) and `CRStaticCtrl::GetParentWndInfo`
(x1) do:
```c
char workspace[100];
workspace[0]=99;                        // IN: buffer capacity
int strsize = parent->SendMessage(WM_GETSTRING, m_ResourceNumber, (int)workspace);
m_string = workspace;                   // strsize NEVER checked
```
`WM_GETSTRING` is IN/OUT — caller seeds byte 0 with the capacity, handler overwrites and
returns the length. On Windows every dialog-tree parent handles it. In a port, a parent
whose message map does not route it makes SendMessage return 0 having written NOTHING,
leaving literal 99 ('c') + uninitialised stack, adopted as the control's caption and drawn.
**Latent until the reader is enabled** — before that `m_ResourceNumber` is always the ctor
default 0 and the `else m_string=""` branch runs. Fix: zero the buffer; adopt only when
`strsize>0`.

This is the THIRD Win32 mechanism in the same uninit family: §8g(5) a ctor-skipped member
reaching a draw; §8h(2) a local out-param to a **stubbed API**; now a local out-param to a
**message handler that may not exist**. The reliable tell in all three is **run-to-run
variance** — a wrong-but-stable value is a logic bug, a value that changes between runs is
uninitialised memory. Ask that question first; it has paid off three sprints running.

**(2) ★★ Fixed pixel coordinates in test recipes are a trap that fires the moment fonts
change.** Enabling the reader moved MA's title-menu row pitch ~16px -> ~28px, and EVERY
scripted recipe encoded menu items as fixed pixels. All of them silently hit the wrong row
— a capture came back showing the wrong screen entirely, and the campaign recipe stopped
reaching the map. That invalidated the parity capture recipes AND the ASan drive recipes
together, i.e. the whole regression gate, exactly when the diff was largest. MA had to ship
the reader opt-in for a sprint because of it.

Re-deriving constants for the new pitch buys one sprint. Resolve at click time instead:
`f,rN` (menu ROW N, from the listbox's own metric) and `f,#ID[:COL]` (hosted control by
dialog id; COL indexes a horizontal listbox via its own GetColFromX). Keep absolute
`f,x,y` working so migration is incremental. Two gotchas:
- **`GetRowFromY` is NOT usable as the row oracle**: it ends
  `if (row > m_playerList.GetCount()) row = -1`, and a front-end menu leaves
  `m_playerList` empty, so it answers -1 for every row past the first. Derive the band
  from `GetListHeight()/GetCount()` (identical TEXTMETRIC).
- **A "Back Load"-style button bar can be ONE horizontal listbox, not two buttons** —
  clicking its centre lands between items; you need the column resolver.
Validate by checking the row form reproduces your existing hand-derived constants with the
reader OFF before depending on it.

**(3) Reader divergences are data-dependent — check yours.** MA consumes but does NOT
apply stock Caption (its persisted captions are `IDS_*` SYMBOL NAMES, and the S57/§8f layer
already resolves those to the shipped wording) nor stock BackColor (hosts composite over
panel art). MA also skips trap 1's COLORREF conversion because its OLE_COLOR is already
0x00BBGGRR end to end — converting would BE the double-conversion the trap warns about.

## 8j. Using the property reader in anger: art-by-name, and a parity measurement rule (MA S64) **[ENGINE]**

Three findings from a sprint of running the §8f/§8i reader default-on.

**(1) ★ `GetFileNum(name)` may be a stub — and it loses artwork silently.** It is the
filename->FileNum resolver the R* string-file setters (`SetNormalFileNumString`) call, i.e.
**the sound half of trap 2**: the persisted numeric Normal/PressedFileNum are
authoring-install indices and get discarded, so the persisted NAME is the only remaining
source of a control's design-time art. Stubbed to 0, every control whose art is named
rather than numbered loses it, with no error. Resolve against the `F_GRAFIX.G`
`FIL_* = 0xNNNN` equates the dialog-template layer already parses. Grep `GetFileNum`.

**(2) ⚠ Do NOT apply resolved art names to every button.** Note that `PX_String` writes
`m_NormalFileNumString` DIRECTLY and never runs the dispatch setter, so nothing converts it
unless you do so explicitly after the replay. MA did — and the parity sweep immediately
caught the invisible system-box buttons ("Quit"/"Size") materialising in the top-left of
every front-end screen. That is the same failure §8f's note-16 caveat describes and that
the S58-era narrowing fixed for the design-bag CAPTION path. The art path needs the same
class narrowing; MA shipped the code disabled behind a flag rather than guess a criterion.
Implement (1) as a pure fix; treat *applying* names as a separate, narrowed change.

**(3) `CString(LPCWSTR)` declared but never defined** — fails at LINK time only, so it stays
invisible until something reads an OCX getter's BSTR back. The convention it hides is worth
recording where the definition lives: in these ports `AllocSysString()` returns a BSTR that
is really a malloc'd NARROW string (the OCX property path never adopted UTF-16), so
`CString(LPCWSTR)` must treat its pointer as narrow bytes.

**(4) A parity-measurement rule, after MA got it wrong.** MA S63 recorded "native renders
LARGER than gold"; S64 measured it and it was false — gold glyph band 10px vs native 11px,
row pitch 52px vs 51px, i.e. the SAME absolute size. The error was comparing a 1280x1003
gold shot with an 800x600 native capture and reading the density difference as a font
defect. **Rule: the gold set and the native captures are at different resolutions, and the
game selects its panel ART SET by resolution — so capturing at gold's resolution is a
different art path, not a flag. Until that exists, no verdict may rest on relative size,
spacing or density; only on layout order, art, content and colour.** Cheap to correct,
expensive to act on: it would have cost a sprint hunting a font-scaling bug that isn't
there. Re-check any verdict of yours that leans on apparent size.

## 8k. Reserved engine ids, and a debug-trace trap (MA S65) **[ENGINE]**

**(1) ★ Reserved engine ids are DESIGN-TIME BY DEFINITION — exempt them from the
caption/art narrowing rules.** `IDJ_TITLE` (1001), `IDJ_TABCTRL` (1002) and
`IDJ_PANEL0..9` (1117-1126) are not ordinary controls: the RDialog machinery looks them up
by name (`GetDlgItem(IDJ_TITLE)` in UpdateTitle, `GetDlgItem(IDJ_TABCTRL)` in
AttachTabToTabControl, `GetDlgItem(IDJ_PANEL0+i)` in AddChildren). Sections 8g and 8h
already special-case the latter two. For this family there is no runtime owner to protect,
so the narrowing rules that stop runtime-owned captions being overwritten do not apply.

MA's Player Log title bar was invisible for FOUR sprints because two individually-correct
filters each withheld half of it: the §8f/note-16 tickbox-only rule suppressed its CAPTION,
and the §8j art-name gate suppressed its ART. Nothing was ever missing from the data — the
bag carries `IDS_PLAYERLOG`, the literal `Player Log`, and `FIL_TITLEB_BMP` x2. Exempting
the single reserved id from both rendered it, parity sweep unchanged.

**(2) A general narrowing criterion, tested and REJECTED: template membership.** The theory
that design-bag properties may be applied only to controls the dialog's template declares
does NOT work — the system-box "Quit"/"Size" buttons are themselves `inTmpl=1`, so it does
not separate them from legitimate controls. Recorded so it is not re-tried; blanket
art-name application should stay opt-in until a criterion is found.

**(3) ⚠ A capped debug trace will eventually hand you a confident, wrong root cause.** MA's
`[px]` replay trace had a hard-coded 60-line cap while the boot path replays 58+ bags, so
any later screen's controls fell off the end. MA read "no trace output" as "the code never
runs", wrote that root cause into a sprint record AND sent it to the other port as a
question about ITS code. Make the caps on any trace that gates a conclusion visible or
tunable (MA's is now `MA_TRACE_PX_MAX`), and verify "no output" really is "no behaviour"
before writing it down.

## 8l. ★★ The game ships its own typeface, and stb_truetype rejects it (MA S66) **[ENGINE]**

MA's cross-cutting deviation #1 -- "front-end font/typeface, the biggest single visual
gap" -- was open from S56 to S66. The cause transfers to any port rendering text through
stb_truetype.

`drive_c/windows/Fonts/Intel.ttf` ("Copyright (c) Rowan Software, 1998") is the art face
the gold screenshots use, and the game asks for it BY NAME (`myfont = "Intel"`, Arial /
Arial Italic as Western fallbacks). Two independent reasons it never arrived on MA:

1. The compat font creator **ignored the requested face** (`(void)face;`) -- one global TTF
   drew everything.
2. That global load was **rejecting Intel.ttf**: `stbtt_InitFont` accepts only platform-3
   cmap encodings 1 (UNICODE_BMP) and 10 (UNICODE_FULL), and Intel.ttf ships a **(3,0)
   SYMBOL** cmap, as many 1990s decorative fonts do. No usable `index_map` -> init returns
   0 -> the port silently falls back to a system face FOR ALL TEXT.

Fix: accept `STBTT_MS_EID_SYMBOL` in stb's cmap search (mark it as a local change to the
vendored copy), and note that symbol cmaps address characters at **0xF000 + c** -- route
every glyph lookup through one helper rather than scattering the constant
(GetCodepointHMetrics / GetCodepointKernAdvance / GetCodepointBitmapBox /
MakeCodepointBitmap). Detect rather than hard-code: after init, symbol-encoded iff
`FindGlyphIndex('A')==0 && FindGlyphIndex(0xF000|'A')!=0`.

Verified against the gold PNGs: MA's title menu and Preferences now render in the same
yellow small-caps art face gold uses. Combined with §8f/§8i colour work this closes the
deviation; what remains is the BDG tab (resource delta) and combo chrome.

**Caveat to carry when adopting:** if your font creator still ignores the face name, ALL
text will now draw in the art face -- including text the game asked to be Arial. On MA that
happens to match gold, but it is luck, not correctness. Do the per-face mapping at adoption
time if your front end mixes faces.

**★ Process lesson, and the reason this sat for ten sprints.** MA's parity doc, three
cross-port notes and several sprint records all said the front end "draws with the GDI
DejaVu fallback". Accurate -- and that is exactly why nobody fixed it. Stating a SYMPTOM in
the vocabulary of a DESIGN DECISION makes it read as settled: "we use the fallback font"
sounds like a known limitation, while "the font load is failing and we don't know why"
sounds like a bug. Same behaviour, same sentence. Scan your own docs for any "we fall back
to X" that has never been accompanied by *why*.

## 8m. Clip control drawing to the control; and "filter, don't cap" (MA S67) **[ENGINE]**

**(1) ★ Compat DCs must clip a control's drawing to that control's rect.** Windows does;
ours did not, and it stays invisible until some control's artwork exceeds its own rect.
`CRButtonCtrl`'s picture path (`RBUTTONC.CPP:1145`) blits its DIB at NATURAL SIZE straight
to the real DC -- not through the offscreen path that line 599 clips via
`BitBlt(0,0,rcBounds.Width(),...)`. On MA the Player Log's `IDJ_TITLE` art is ~550px wide on
a 336px control and painted ~213px past the dialog, over the map beneath. Note the control
was correctly SIZED (traced at 336x27), so this presents as a neighbouring panel being
overpainted, NOT as a layout or font problem -- chasing it as layout wastes the sprint.
Fix: a clip rect in the GDI layer (absolute canvas coords) honoured by the pixel-put,
BitBlt and StretchBlt paths, set around each control's OnDraw and restored after. MA's
parity sweep stayed byte-identical, so nothing was relying on the overflow.

**(2) ★ Filter, don't cap** -- the sharpened version of §8k(3). MA repeated the capped-trace
mistake ONE SPRINT after documenting it: a probe written `static int n; if (n++ < 8)` had
its budget consumed by controls that redraw every frame, long before the screen under
investigation appeared, and briefly "proved" the control was never drawn. A line budget is
always spent by whatever happens early; a predicate on the thing you are looking for
(`w > 300`, `id == 1001`) is bounded AND cannot be starved. Prefer it whenever the
interesting event comes late in a run.

## 8n. Icons: a success-returning stub that hid the whole subsystem (MA S68) **[ENGINE]**

**(1) ★★ Check whether `CDC::DrawIcon` / `LoadIconA` are stubs.** In MA both were
(`DrawIcon` returned TRUE and drew nothing; `LoadIconA` returned NULL) for the entire life
of the port, so **every icon the engine draws was invisible, everywhere, with no error**.
A stub that returns SUCCESS never surfaces as a bug report -- only as "that screen doesn't
quite match the reference". Grep for it.

The case that exposed it: the Player Log title bar's ?/tick. Route it by engine logic, not
by eye -- `RDialog`'s eventsink has
`ON_EVENT(RDialog, IDJ_TITLE, 2 /*Cancel*/...)` and `3 /*OK*/`, so the TITLE CONTROL raises
Cancel/OK and therefore draws its own buttons (`RBUTTONC.CPP:521-536`), gated on its
persisted CloseButton/TickButton flags. MA's title bag carries `close=0 tick=1`.

**(2) The R* control icons live in the control's own OCX** -- `Rbutton.ocx` carries
RT_GROUP_ICON 828..832 (IDI_BYEUP / IDI_TICKUP / IDI_TICKDOWN / IDI_HELPUP) while Mig.exe
has only 128/129. Third instance of the same rule (§8f Intel.ttf, §8g RTabs art): inside a
control `AfxGetInstanceHandle()` is THAT CONTROL'S module, so resolve by id across the
game .exe and the control OCXes and ignore the handle.

**(3) RT_ICON needs its own decoder, not the DIB path.** `RT_GROUP_ICON` is a DIRECTORY
whose entries name `RT_ICON` resources by id -- follow the indirection. In the RT_ICON
payload `biHeight` is DOUBLE the real height: the XOR colour bitmap followed by a 1bpp AND
mask, both bottom-up, mask bit 1 = TRANSPARENT. Blit alpha-keyed through the same
viewport-origin and clip path as everything else (§8m's clip matters -- a 32x32 icon near a
control's right edge would otherwise spill).

**(4) Process: don't let a truncated listing become a negative result.** An `ls *.ocx | head`
cut `Rbutton.ocx` (lowercase 'b') out of view and briefly "established" that RButton was not
installed -- which would have closed the story as "the resources don't ship". Same family as
§8k(3)/§8m(2): a tool's own limit misread as evidence about the system.

## 8o. `CDC::DrawText` DT_WORDBREAK + '&' escape — scope the guard to the case, not the flag (BoB S127) **[ENGINE]**

*Implements the shared find MA flagged in note 17 (§8f tail): "compat `CDC::DrawText` must
implement real `DT_WORDBREAK`/multi-line (CRStaticCtrl draws all long prose through it —
unwrapped text running off a panel edge is this, not a layout bug)."*

**(1) Only `CRStaticCtrl` reaches `DrawText`; every other R\* control draws via
`ExtTextOut`/`TextOut`.** One grep settles the blast radius before you write a line
(`RCOMBOC/RBUTTONC/RLISTBXC` all `ExtTextOut`). That means BOTH fixes below live entirely in
the static `DrawText` path and cannot touch combo/button/listbox text — no special-casing
needed. `CRStaticCtrl::OnDraw` calls `pdc->DrawText(m_string, rc, DT_LEFT+DT_WORDBREAK
(+DT_TABSTOP))` for non-central text, three times when `m_FontNum<0` (two shadow passes at
offset + the colour pass) — your wrap must be deterministic so the shadow lines register.

**(2) ★ DT_WORDBREAK is passed by EVERY static — labels AND descriptions — so guard the wrap
by BOX HEIGHT, not by the flag.** Config LABELS ("Radio Chatter Volume") sit in single-line
boxes and also carry DT_WORDBREAK; if you wrap on the flag alone, any label whose text is
wider in your font than in gold's (our stb/stencil face is wider) wraps to a 2nd line and
spills into the row below — a fresh regression on screens that were already CLOSE. Fix: wrap
only boxes tall enough for ≥2 lines (`(bottom-top) >= 2*pitch`); single-line boxes keep the
one-line render. The tall PhaseDescription / QS-training statics are the only real targets.
This is the same "filter, don't cap" instinct as §8m but applied to a draw flag: let the box
geometry — which you already have from the template — decide, don't trust the flag globally.
(BoB had earlier *capped the font* for tall boxes, R6.2/S11; S127 replaced that with real
wrapping at the one-line font size.) Greedy word packing + honour explicit `\n` (paragraph
breaks in the prose) + per-line DT_CENTER/DT_RIGHT + vertical clip to the box.

**(3) '&' accelerator escape belongs in the same method (Windows semantics).** `DrawText`
without DT_NOPREFIX treats '&' as an accelerator prefix: "&&"→literal '&', a lone '&'
marks/removes the next char. BDG's Controls label "Cockpit && UI" was rendering literally;
processing it → "Cockpit & UI" (gold). Because only statics hit `DrawText`, combo device
names keep their literal '&' ("...Axis 0 & Axis 1", drawn via ExtTextOut) automatically.
Gate each independently (`BOB_NO_WORDWRAP` / `BOB_NO_AMP_ESCAPE`).

**(4) Verify backend-independence with the dummy==GL `cmp` bar (§8f).** The wrap is pure
integer text metrics, so a headless SDL-dummy capture of a wrapped screen must be
byte-identical to the real-GL capture — S127 confirmed it on the changed phaseselect screen
in one `cmp`. If they differ, your wrap is reading uninitialised metrics, not "AA noise".

**(5) MA note 17 mechanism #2 (parent-rect clipping) — assessed N/A on the BoB side (S127).**
BoB's S124 template-membership filter already removes the dead controls MA clips by client
rect, and no in-template-but-out-of-client-rect stray drew across BoB's 14-screen headless
sweep. The mechanisms are genuinely distinct (a control CAN be a template member yet parked
outside the client rect), so this is "checked, no current symptom", not "same fix" — adopt if
one surfaces.

## 8p. Hosting a new R\* control type is a recipe, not a subsystem (BoB S128) **[ENGINE]**

**A blank widget on a screen can be an un-hosted CONTROL TYPE, not a data/font gap.** BoB's
Quick-Shots page-tab row (Scenario/Parameters/…) was blank; the cause was that `IDC_RRADIO` is
a `CRRadioCtrl` and the port had only ever hosted RListBox/RCombo/RStatic/RButton/REdit — every
wrapper `InvokeHelper` on it was a silent no-op. Before chasing captions, check *what class the
DDX/`CreateControl` binds* and whether your factory hosts it.

**The recipe (each new type is ~1 hour once the seam exists — the sixth mirrored the fifth
almost verbatim):**
1. **Dispids come from the WRAPPER, not the control's DISP_MAP.** Read `SRC/MFC/<CTRL>.CPP`
   (the generated `CRRadio` wrapper) — each method/prop `InvokeHelper(0xN,…)`/`GetProperty(0xN,…)`
   gives the exact dispid the router will see (RRadio: 5=AddButton BSTR, 6=Clear, 1..4 props,
   stock ForeColor). The server-side DISP_MAP order can differ; trust the wrapper.
2. **Host = `struct Host<Ctrl> : public <Ctrl>Ctrl, public OleHost`** with `boot` (`OnResetState`
   + empty-`CPropExchange DoPropExchange`), `applyDesignProps` (replay the persisted DLGINIT bag
   through the genuine `DoPropExchange`, `m_hWnd=0` during replay), `draw` (set `m_FirstSweep=TRUE`
   to skip the WM_GETARTWORK/offscreen path AND any `!m_hWnd` black-fill, then call the genuine
   `OnDraw`), and dispid `dispatch`/`setprop`/`getprop`. Copy the closest existing host verbatim.
3. **Register:** CLSID (from `IMPLEMENT_OLECREATE_EX` in the control's `.CPP` — the coclass uuid,
   not the dispatch IID) in the factory; `bob_make_<ctrl>` in the host header; add the genuine
   `<CTRL>C.CPP` + the host TU + the control's include dir to the R\*-controls build target.
4. **Don't re-define the control's IIDs in the host** — the genuine `<CTRL>C.CPP` already defines
   them with internal (`const`) linkage; the host doesn't reference them.
5. **The genuine control's `OnDraw` may not compile on GCC — reuse the sibling's fix.** RRadio's
   `MaskIcon(pDC, CPoint(x,y))` binds a temporary to a `CPoint&` (MSVC extension GCC rejects,
   even under `-fpermissive`); RBUTTONC.CPP had already solved the identical call with a named
   local (`_mip00`). Grep for the prior fix before re-deriving. Same documented compile-compat
   exception class; no logic change.

**Know where the story stops.** Rendering the control (its captions/icons) is one deliverable;
*driving* it (click → event → the dialog's `ON_EVENT` handler → a page switch) is another, and a
page-switching dialog also needs the `MoveWindow`/page-visibility mechanism. Ship the render as
the prerequisite and name the remaining half rather than half-wiring a click that can't paint.

**Fix (BoB S132) — a null-reference-safe `DialBox` copy ctor, two layers.** Make
`DialBox(const DialBox& d)` check `&d==NULL` (needs `-fno-delete-null-pointer-checks`, which both
ports build with) and produce an EMPTY leaf (`dial=NULL`, `diallist[0]=NULL`) instead of reading
`d.edges/d.art/d.dial` at address 0. `AddChildren` already turns a `dial==NULL` child into an
empty `RDEmptyP`, so inactive slots draw nothing — no need to touch the panel builders. **The
second layer bites if you stop at layer one:** the stock copy ctor left `diallist[]`
uninitialised and relied on the ternary's copy-*elision* to preserve a leaf's `diallist[0]=NULL`;
a real copy of the `:NULL` branch has no elision, so `AddChildren` recurses into garbage children
and crashes again one frame deeper. Fix by **copying `diallist` explicitly** in the ctor —
deterministic, and identical to the elided values for the working screens (`DialList` overwrites
its own `diallist` in its body). One header method, `#if BOB_LINUX`; regression-verify with a
`cmp`(pre, post) on a few working dialog screens — it must be byte-identical (the change only
touches the previously-crashing null-copy path). **Caveat:** this fixes the *crash*; the child
panels' hosted-control CONTENT is a separate render task (they're created but may not be in your
per-panel draw walk).

## 8q. The variadic `DialList` null terminator copies from `*(DialBox*)NULL` in a ternary (BoB S130) **[ENGINE]**

_(Shared **dialog-framework** trap — both ports use `RDIALOG.H`'s `DialList` + `EDGES_*`. Related
to §8d, a different failure of the same builder.)_

Panel builders that assemble a **variable** number of children use `DialList` with a null
terminator: `const DialBox& ND = *(DialBox*)NULL;` then
`(count>k) ? DialBox(FIL_NULL, new SomeChild(...), EDGES_…) : ND` for each slot. This is
**null-safe by construction on the list side** — `DialList` stores `diallist[i]=&d_i` (so an
inactive slot is `&ND == 0`) and `AddChildren` iterates `for(i=0; diallist[i]; i++)`, stopping at
the first null. **The trap is the ternary itself:** its operands are a **prvalue**
(`DialBox(...)` temporary) and an **lvalue** (`ND`) of the same type, so C++ makes the conditional
a prvalue — when the `:ND` branch is taken it **copy-constructs a `DialBox` from `*(DialBox*)NULL`**,
dereferencing null. On MSVC that copy reads address 0 and (usually) survives; on GCC it SIGSEGVs.
BoB hit it in `QuickMissionBlue`/`QuickMissionRed` (`FULLPANE.CPP`) the moment S129 made the QS
order-of-battle tab reachable — the screen had never rendered on Linux. It only faults when a slot
is actually inactive (`count ≤ k`), so a full-complement mission hides it — check with a *sparse*
data set.

**Diagnosis:** a `bt` frame at the exact `... : ND` line inside a panel builder (not in
`AddChildren`, which is null-safe) is this. **Fix direction (game-code UB-exception):** make the
true-branch an **lvalue** so the conditional yields a reference, not a copy — i.e. name the
per-slot `DialBox` locals (respecting the §8d `Edges`-lifetime rule) — or give the builder a real
empty-but-terminating sentinel. Not fixable compat-side: the copy happens in game code before the
list exists. **Grep** for `*(DialBox*)NULL` / `: ND` in the panel builders to find every site
before enabling the screens that reach them.

## 8r. Adopting the per-face font registry — and how to tell if your port is "Japanese" (BoB S131, from MA note 26) **[ENGINE]**

BoB adopted MA note 26 §2 (per-face registry) and it fixed the pervasive "font face" deviation
(data/label rows drew in the Rowan art face instead of Arial). Recast for the shared engine:

**(1) The registry is the load-bearing fix; keep ART byte-identical.** `bob_gdi_font` (MA:
`ma_gdi_font_create`) drew every face in one TTF. Key the registry by face KIND × style:
ART=the game's own art TTF (Intel.ttf — *preserve the exact old load order* so ART screens stay
byte-identical), SANS=LiberationSans, SERIF=LiberationSerif, MONO=LiberationMono (metric-compatible
with Arial/Times/Courier). Classify the `CreateFont` face name (Arial/Sans→SANS, Times/Roman→SERIF,
Courier→MONO, Intel/Header/**unknown→ART** so nothing regresses). Thread the resolved face through
the DC's *currently-selected* `CFont` (both ports already track it on `SelectObject`), setting it
right before each text draw/measure; the front-end MENU draws outside a DC, so set ART there
explicitly. Verify ART is unregressed with a `cmp`(S131-on, revert) on the title screen — it must
be **byte-identical**.

**(2) ★ Honour the `bItalic` flag — gold's data values are Arial *Italic*.** MA note 26 stopped at
regular faces; the gold config **combo values are italic** (and some labels). Capture the
`CreateFont` italic byte (or `LOGFONT.lfItalic`) into the `CFont`, double the registry to
regular/italic per kind, and select the `-Italic` TTF. ART has no italic (stencil) → fall back to
ART regular. This is what makes the config screens' slanted values line up with gold.

**(3) ★ §1 (the Japanese-branch trap) is NOT universal — check before "fixing" it.** MA's port
took the Japanese font branch because its `EnumFontFamilies` stub always "found" the CJK probe
face. **BoB did not**: a one-line `BOB_TRACE_FONT` dump of the names `CreateFont` actually receives
showed `Arial`/`Courier New`/`Intel`/`FC-Glamour-Bold`/`Fusion Bold` — the English set — so BoB's
always-succeed enum stub happens to pick the correct first candidates, and the §1 fix is a no-op
here. **The lesson is the diagnostic, not the patch:** trace the real requested face names first;
if they're ASCII English, you have only the §2 (registry) gap, not §1. (BoB's §3 combo-fill was
also already handled by the `m_FirstSweep=TRUE` host convention; MA note 27's "listbox fill is
load-bearing" warning was heeded — left untouched.)

## 8s. Nested `DialList` screens: the game's layout is dead headlessly — synthesize it (BoB S133) **[ENGINE]**

The front-end panel paint draws hosted controls via `bob_ole_draw_panel(pdial[d])`, which filters
hosts by `parentDlg == pdial[d]`. That's fine for flat config panels, but a `DialList` screen —
BoB's QS **order-of-battle** (`QuickMissionBlue/Red` → `QuickMissionPanel` + a clump of
`CSQuickLine` flight rows, FULLPANE.CPP) — nests each row as its **own `RDialog` with its own
`parentDlg`**, so the standard per-panel draw reaches none of the row content. The screen loads
(after the §8q crash fix) but paints blank.

**The trap:** the obvious fix is "walk the child tree and read each row's rect from the game." It
doesn't work — the game's layout engine is stubbed on Linux. A 20-line probe (dump each nested
node's `OnGetXYOffset` / `viewsize` / `GetWindowRect`) shows **every nested node has `viewsize`
height 0, full-screen `GetWindowRect (0,0,W,H)`, and `xyoff (0,0)`** because `MoveWindow` /
`OnSize` / `ClientToScreen` are compat stubs that never compute the layout. A stubbed layout engine
returns *zeros, not errors* — so trust nothing until you dump the runtime rects.

**The fix (BoB `FULLPSYS.CPP`, `bob_fp_draw_nested` + `bob_nested_walk`, default-on, revert env):**
walk the panel's child `RDialog` tree (`fchild`/`sibling`) and call `bob_ole_draw_panel` on each
nested dialog, but **synthesize** the geometry the stubs don't provide. The one invariant you *do*
know: a `DialList`'s rows are identical sub-panels, so stack them — each successive content-bearing
child draws one `rowStep` lower — while reusing `bob_ole_draw_panel`'s existing per-control
template-rect positioning for the within-row column layout. Non-content nodes (an `EmptyChildWindow`
placeholder, the clump container) draw 0 controls and don't advance the row cursor.

Two cheap guardrails: (1) early-return on `!top->fchild` so the walk is inert on every flat screen,
then prove it with `cmp`(on, off) on a config screen + a normal panel — byte-identical = zero
regression, no eyeballing; (2) separate "the list renders" from "the editor is reached" in the
verdict — the row list populating is one screen; a *click* on a row to reach its editor is the next.

**Also (this sprint): measure an inbound sibling note before adopting it.** MA note 28's "skip the
OOB listbox black fill" fix was verified **N/A for BoB** by a single `BOB_MAP_OOB=1` capture — BoB's
map OOB dialogs already composite their lists over the translucent panel (no opaque fill). Half the
carried "residuals" dissolve on measurement; the note said so itself. Applies to MiG Alley's
`DialList` screens (Career/Log/order-of-battle) verbatim — same `RDialog` tree, same stubbed layout.

## 8t. Hosting `CREdtBt` (the edit-button), + two OCX compile traps that recur per new control TU (BoB S140) **[ENGINE]**

Bringing a 7th R\* control type online (`CREdtBtCtrl`, the edit-button used for pilot-name slots)
confirmed the new-control-type recipe (§8p) and surfaced two compile traps worth pre-empting on any
port that adds a genuine OCX TU to the build:

**1 — the `IconsUI` enum forward-decl underlying-type mismatch.** `uiicons.h` defines
`enum IconsUI : unsigned int` (its `ICON_SELECT_MASK=0xff000000` overflows `int`). Several control
headers forward-declare it as `enum IconsUI : int;` — MSVC ignored the mismatch, GCC errors
("different underlying type"). Fix the forward decl to `: unsigned int` (BOB_LINUX-guarded). Only
TUs that include BOTH the control header and `uiicons.h` hit it, which is why RButton/RRadio didn't.

**2 — `MaskIcon(CDC*, CPoint&)` won't bind a temporary.** `icon->MaskIcon(pDC, CPoint(x,y))` passes
a prvalue to a non-const `CPoint&` — GCC rejects it. Name the temp: `CPoint p(x,y);
icon->MaskIcon(pDC, p);` (RRADIOC/RBUTTONC already do this — grep the compiling sibling TUs for the
exact pattern before reasoning it out).

**Two control-specific host subtleties** (read the genuine control before writing the host): (a)
`CREdtBt`'s Caption is a *stock* property — the wrapper's `SetCaption` calls
`SetProperty(DISPID_CAPTION, ...)` (not a custom dispid like CREdit's `0x3`), so route
`DISPID_CAPTION_` → `InternalSetText` (compat `SetText` is a no-op); (b) its `OnDraw` draws a
`captiontext` member refreshed only in click/OnTextChanged handlers, not inside `OnDraw`, so a host
that drives `OnDraw` directly must set `captiontext = InternalGetText()` first. General rule: check
`SetProperty(...)` in the wrapper `.cpp` for the caption dispid, and read the control's own `OnDraw`
for where its text actually comes from.

MiG Alley: if any MA screen hosts `CREdtBt`/edit-buttons (pilot rosters, name entry), the same host
+ the same two compile fixes transfer verbatim.

## 8u. The `Select(row, COLUMN)` event has two arguments — and a tab row is COLUMNS (BoB S141) **[ENGINE]**

**Check your hosted-listbox click path right now: does it pass a real column?** BoB's front end
models a *tab row* as the **columns of one `CRListBoxCtrl`** — `CSCampaign::OnInitDialog`
`AddString`s each campaign phase into its own column, and the handler
`ON_EVENT(…, 1 /* Select */, OnSelectRlistCampaigns, VTS_I4 VTS_I4)` switches phase on the
**column**, not the row. Our click path resolved the row faithfully (through the genuine
`GetRowFromY`) and then passed a hardcoded `0` for the column. Result: every click on the phase row
re-selected phase 0, so **every campaign the port had ever run started in the first phase** — and
that, not any render bug, is why the LW Directives allocation grid never appeared (it is empty on a
standby day). One hardcoded argument masqueraded as a screen-render gap for four sprints.

**The fix is symmetric with the row:** the genuine control already has `GetColFromX(long)` (walks
`m_sizeList`, the authored/Shrink-computed column widths), exactly as it has `GetRowFromY`. Host it
the same way — `colAtX()` alongside `rowAtY()` — and pass both event args. If MA hosts any listbox
whose handler takes `VTS_I4 VTS_I4`, it has the same latent bug; grep for `1 /* Select */` and check
which handlers read their second parameter (BoB has ~30 such handlers, several of which use the
column: `GroupGeschwader`, `LWDiaryDetails`, `RAFDiaryDetails`, `CSCampaign`).

**Generalised lesson — a stubbed/hardcoded OUT- or IN-argument reads as a missing feature.** This is
the same family as the uninit/stub traps (§8i, `WM_GETSTRING`'s ignored OUT half): nothing errors,
nothing is uninitialised, the value is simply always the same wrong constant, so the symptom shows
up somewhere far away and gets written down as "that screen needs more work". When a screen looks
state-starved, check what selects the state before rendering anything.

**Also (recipe hygiene, adopting MA S62/S63 verbatim):** driving a *panel control* headlessly needs
a click point, and BoB now resolves it from the control's own drawn rect + column walk
(`bob_ole_ctrl_point`, `BOB_AUTOCLICK=#ID[:COL]`) rather than fixed pixels — MA's rule after a font
change moved its menu pitch and silently broke every parity capture and ASan drive recipe at once.

**ANSWERED by MA note 29 §2 — and BoB's own read was partly wrong, corrected here.** The question
was: we cannot *dismiss* an OOB dialog headlessly, because calling `CMiscToolbar::OpenDirectivetoggle`
on a dialog the game had opened produced a second stacked frame instead of closing it.

MA's answer: on the MA side `OpenXxx` is **ensure-open, not a toggle**
(`CMainToolbar::OpenPlayerlog`: `if (!LoggedChild) OnClicked… else BringWindowToTop()`), the genuine
toggle is the button handler `OnClickedXxx`, and a capture scaffold should call
**`CloseLoggedChild(<INDEX>)` / `CloseLoggedChildren()`** directly — precisely because a scaffold
must not care *who* opened the dialog.

**BoB's actual mechanism — measured in S143, after TWO wrong guesses.** For the record, because the
wrong guesses are instructive: (1) S141 said "`OpenDirectivetoggle` opens a second stacked instance
instead of closing"; (2) after MA note 29 that was revised to "an index mismatch". **Both wrong.**
`CMiscToolbar::OpenDirectivetoggle` (MSCTLBR.CPP:378) *is* a genuine toggle on index `DIRECTIVES`,
and it *did* close it. What is actually there is a **stack of two different dialogs**: on an active
campaign day the game opens `DIRECTIVERESULTS` (index 5), whose own code then opens `DIRECTIVES`
(index 6) on top of it (DIRRSULT.CPP:197). Closing 6 reveals 5 — a *larger* dialog — which is what
was misread as "a second stacked instance". (It also means BoB's S137 capture, described then as
"the Directives dialog renders its frame + Rest All + the standby reminder", was really
DIRECTIVERESULTS, not the allocation grid.)

**And closing them cannot work at all — the stack is self-healing.** The looped
`CloseLoggedChildren()` above *oscillates*: measured passes went 6→5→6→5→6, ending open. Two sites
re-open it — `DirectiveResults::OnCancel` calls `OpenDirectivetoggle(dr)` (DIRRSULT.CPP:194), and
the day-start path re-opens whenever `!MMC.directivespopup` (LWDIRECT.CPP:2050). **This is faithful
game behaviour, not a port defect:** on an active campaign day the Luftwaffe player is *supposed* to
be holding the allocation UI until orders are issued. A scaffold that fights it also **leaks** — the
S143 state banner caught the dialog's hosted-control count ballooning **184 → 1656** across the
open/close cycles, because each re-open re-creates the controls.

**The right lever is the game's own toolbar toggle.** `MMC.directivespopup` gates the day-start
popup; the genuine `CMiscToolbar::OnClickedDirectivetoggle` (IDC_DIRECTIVETOGGLE = 1007) flips it
and keeps the button's pressed state + hint string consistent. It is `protected`, so drive it as a
genuine **Clicked event through the eventsink** (as the port already does for the Bases button)
rather than poking `MMC` — same "drive the genuine handler" rule MA states in §1 of note 29.

Three things follow. **(1)** A scaffold must not route through a toggle, assume a single dialog, *or*
assume a dialog stays closed. **(2)** Prefer the game's own *suppression* setting over fighting its
*creation* — the setting exists because the designers anticipated exactly this want. **(3)** The
transferable half: **three explanations were offered for one behaviour before anyone printed the
state, and all three were wrong.** Two `fprintf`s of `LoggedChild()` settled it in one run, and the
same banner incidentally exposed a control leak nobody was looking for — the same conclusion
FreeFalcon reached in note 15 from the opposite direction. When you catch yourself revising a
mechanism a second time, stop reasoning and instrument.


## 8v. One-shot statics in test-drive hooks silently cap what the harness can reach (MA S80) **[HARNESS]**

**`if (++n == N)` on a function-local static fires exactly once per PROCESS.** Every headless
drive hook in these ports is written that way — "after N idles, press the thing" — and for a
one-screen-deep recipe it is correct and cheap. It stops being correct the moment the recipe
needs to do the same thing **twice in one run**, and it fails *silently*: the counter sails past
`N` and the hook simply never fires again. Nothing logs, nothing errors; the run just sits there.

MA hit this driving the campaign's **flyable multi-mission loop** (fly mission 1 → debrief →
next period → fly mission 2). Three separate hooks on that one path were one-shot — the frag
drive (`MA_CAMP_FLY`, `++_fragn == 40`), the Fly drive inside the briefing (`++_flyn == 30`), and
the graceful flight-exit (`BOB_AUTOEXIT`, `++_aef == atoi(ae)`). Mission 2 fragged and launched
into 3D and then **flew forever**, because `BOB_AUTOEXIT`'s counter had been spent on mission 1.
The interesting part: this is *harness* code, so for the port's whole life it read as a *game*
limitation — "the campaign only does one flyable mission" — when the campaign had been able to do
more for some time. Two of the three counters had to be hoisted out of their own blocks before
they could even be reset, which is a decent smell test: **if a drive counter is declared inside
the block it drives, that path can only ever run once.**

**The rule:** decide whether each hook is *per process* or *per occurrence*, and make
per-occurrence ones re-arm on the state transition that ends the occurrence — MA resets the
flight-exit counter on every 3D→front-end edge (`_was3d && !ma_in3d`), so each flight gets its own
N frames, and the loop drive resets the frag/Fly counters after ending each debrief. Same family as
§8m's "filter, don't cap": a budget that early traffic can exhaust will be exhausted by early
traffic, and the thing you were actually waiting for happens later.

**Corollary for capture recipes.** The same applies to `MA_SHOT=N`/`BOB_SHOT=N`-style captures:
an absolute idle number cannot be made to land at the end of a multi-mission loop, because the
count depends on how long the flights took. MA arms the capture **from the drive itself** when the
loop reaches its mission target (`MA_CAMP_LOOP_SHOT` → a countdown, then dump+exit), which is the
same magic-number-elimination rule S62/S63 applied to click coordinates (`f,rN` / `f,#ID[:COL]`).

**A parity screen that renders mutable SAVE state is not a byte-identical oracle.** MA's
`campaign_map` reference came back 8095 px different this sprint and it was nothing to do with the
diff: the capture draws the campaign's own date/frontline/unit icons, and the port's *test runs*
(`MA_CAMP_FLY`, and now the multi-mission loop) advance the campaign on disk, so the oracle drifts
away from its reference every time the harness is used. The check that settles it in one step is
the S60 A/B: rebuild the **pre-sprint** binary and capture again — identical bytes from both
binaries means the delta is state, not code. Classify such screens explicitly (MA already had one,
`prefs_controls`, which embeds live joystick state) and keep them out of the default gate rather
than rebasing their references each sprint, which would quietly destroy the oracle. Related trap
found the same way: MA's campaign autosave writes `SaveGame/Auto Save.sa` — one character short of
the `Auto Save.sav` the game is asked to write (`CFiling::SaveGame`, through
`fakefile`/`namenumberedfile`'s fixed-width name buffers). Worth checking on the BoB side, since
`fileman` is shared engine code: a save that lands under a name nobody looks for is indistinguishable
from "persistence isn't implemented yet".

**Answering BoB's §8u open question — how to dismiss a logged dialog headlessly.** The genuine
close trigger is the toolbar button's own **`OnClickedXxx` handler**, not the `OpenXxx` wrapper —
they are different things and the naming hides it. In MA, `CMainToolbar::OpenPlayerlog`
(`MAINTBAR.CPP:276`) is *ensure-open*: `if (!LoggedChild(PLAYERLOG)) OnClickedPlayerlog(); else
LoggedChild(PLAYERLOG)->BringWindowToTop();` — call it twice and you never close anything. The
**handler** is the toggle: `CDebriefToolbar::OnClickedPlayerlog` (`DBRFTLBR.CPP:170-180`) is
`if (!LoggedChild(id)) { LogChild(id, MakeTopDialog(...)); } else CloseLoggedChild(id);`, and the
same open/else-close shape repeats across `OnClickedDis`/`OnClickedOverview`/`OnClickedResults`
(`DBRFTLBR.CPP:159/198/219`) and `MAINTBAR.CPP:191/207/239/250`. So: to *toggle*, call
`OnClickedXxx`; to *unconditionally close* — which is what a capture scaffold actually wants, since
it must not care who opened the dialog — call `<toolbar>.CloseLoggedChild(<INDEX>)` directly, or
`CloseLoggedChildren()` for all of them. BoB's `OpenDirectivetoggle` stacking a second instance is
this exact `Open*`-vs-`OnClicked*` confusion, and the `CloseLoggedChild` machinery BoB already
fixed in S110 is the right target. MA takes the same route for the campaign loop: the S80 drive
calls the genuine `CDebriefToolbar::OnClickedNextPeriod` rather than reimplementing `EndDebrief`.

**And the reason MA did not have BoB's §8u `Select` column bug — worth copying as a recipe.**
MA's host does not reimplement the listbox hit-test at all: `CRListBoxCtrl::MaMouse`
(`SRC/RLISTBOX/RLISTBXC.H:28`, `MA_LINUX`) calls the control's genuine `OnLButtonDown`/`OnLButtonUp`
and then reads back `m_iRowSel`/`m_iColSel`, so both event args are whatever the real control
decided. Driving the genuine handler instead of recomputing its inputs is what made the column
correct for free — and it is the general defence against the whole §8u/§8i family.

## 8w. Editing a 1990s game source can silently RE-ENCODE it — patch bytes, then read the diff (BoB S142) **[ENGINE]**

These sources are **ISO-8859-1**, not UTF-8, and they contain real non-ASCII literals — BoB's
`RSPINBTC.CPP` has `strcpy(buffer, "£ ")` three times in `ValueToMoneyString` (0xA3, one byte).
A text-based edit tool that reads/writes as UTF-8 rewrites the whole file on save, turning every
0xA3 into 0xC2 0xA3. The intended change was two lines; the actual diff was **five**, and the three
extra ones silently changed what the game would print (`£` → `Â£`).

Nothing warns you: it compiles, it links, `file` quietly flips from "ISO-8859 text" to "UTF-8 text",
and the corrupted literal only shows up in a screen nobody captured this sprint.

**Scope, measured (BoB):** **304** of the `.CPP`/`.H` sources contain non-ASCII bytes, some of them
tens of thousands (`3D/3DCOM.CPP`, `3D/TRANSITE.CPP`, `HARDWARE/RCHATTER.CPP`). This is not an
exotic corner of the tree; it is a third of it.

**How to work:** for any edit to a game source, patch it as **bytes** (read `rb` → replace →
write `wb`), and **always read the resulting `git diff` and check the changed-line count matches
your intent** — that check is what caught this one. `file <path>` flipping from "ISO-8859 text" to
"UTF-8 text" is the smoking gun. Count the hazard bytes with Python, not grep:
`python3 -c "b=open(P,'rb').read(); print(sum(1 for c in b if c>0x7e))"`. MA's tree is the same
vintage — check before editing any TU with currency/umlaut literals.

**Tooling caveat that cost time here:** `grep` on this box is **ugrep**, not GNU grep, and its
binary/encoding handling differs enough that two spellings of the same non-ASCII search returned
77, 63 and 0 for the same tree. Don't use it to decide an encoding question — use Python, which has
no locale or binary-detection heuristics in the way.

**Related process note, worth more than the trap itself:** the same sprint produced a *wrong*
diagnosis on the way here — a `grep` that returned nothing was written down as "grep goes silent on
ISO-8859 files", when the real cause was that the shell's **cwd had been reset** and the relative
path simply didn't exist. It was one command away from being banked into this doc as a shared
lesson. Two rules: **verify a lesson before you bank it** (re-run the failing command with the
variable you're blaming actually isolated), and remember that "no output" has many more causes than
the interesting one — cf. §8i's capped-trace trap, which is the same mistake wearing a different hat.

## 8x. Section-number collisions are a real hazard of this shared doc (BoB S142 / MA S80) **[PROCESS]**

Both ports appended a section on the same day and **both called it §8v** — MA's one-shot-statics note
and BoB's re-encode note. MA's landed first and note 29 already cites "§8v", so BoB's became §8w.
Harmless once, but the per-letter counter is a shared mutable resource with no lock, and the two
copies are hand-synced: the loser of a race can silently clobber the winner on the next `cp`.

**Convention from here:** before appending, `grep -n "^## 8" ` **both** copies, take the next free
letter, and re-run `tools/check_notes_sync.sh` immediately after syncing (it compares the copies and
would have caught this). If you find your section number already taken by the sibling, renumber
**yours** — the published note that cites it wins. Cheaper alternative if this recurs: number
sections by originating sprint (`§8-BoB142`, `§8-MA80`), which cannot collide.


## 8y. A self-consistent wrong value produces no symptom until something outside the system looks (MA S81) **[ENGINE]**

MA's campaign autosave had been writing `SaveGame/Auto Save.sa` — one character short of the
`Auto Save.sav` the code asks for — and **reading it back under the same truncated name**. So the
round trip worked: the game saved, loaded, and advanced the campaign across runs, while the
canonical `Auto Save.sav` sat untouched for weeks and nothing anywhere reported an error.

**Mechanism.** `fileman::namenumberedfilelessfail` lacks the "fake long file name" branch that the
hard `namenumberedfile` has (return the caller's name that `fakefile()` stashed at
`namedirdir+fakefileoffset`); without it, it always falls through to the DIR.DIR path, which lifts a
fixed **12-byte** 8.3 entry and NUL-terminates at byte 12. Under `MA_LINUX` the port had routed the
buffered `FileMan::namenumberedfile(f, buf)` through the *lessfail* variant (for its graceful
unregistered-directory behaviour) — so the save path used **the one variant missing the branch**.
Every other filename in the boot path is ≤ 11 chars and survived; `"Auto Save.sav"` is 13.
*(Checked on the BoB side: BoB's `namenumberedfilelessfail` already has the branch — not affected.)*

**Why it stayed invisible, which is the transferable part:**
- **Round-trip tests cannot see it.** Save→load agrees with itself perfectly. Only an *external*
  observer — does the file the rest of the world expects exist, and get newer? — can catch a
  consistently-wrong name. One `ls` of the save directory after a campaign run is the whole test.
- **It surfaced through a parity capture, not through the feature.** MA's `campaign_map` reference
  drifted 8095 px; chasing *that* found the truncation. Parity oracles are worth more than their
  stated purpose — they are the port's only routine outside observer.
- **Four copies of a constant are how two code paths drift apart.** The convention's two magic
  numbers (`128`, `8`) were written out at four sites: `fakefile` stores the name, and three
  resolvers independently re-derive the address. Two of them disagreeing is exactly the observed
  bug. MA adopted BoB's naming — `fakefileoffset` / `fakefileindex` in `FILEMAN.H` — with **MA's own
  values** (128/8; BoB's are 800/50 because its buffer layout differs — *do not copy the numbers
  across ports, only the naming*).

**And the counterpart to §8s's "measure before adopting": measure before ASKING, too.** MA note 29
sent BoB an errand ("possible shared-engine bug, check your `fileman`") on the strength of a
plausible mechanism. One grep of BoB's tree would have shown it was already fixed there. Cross-port
notes carry the same burden of measurement as findings do; a speculative "check yours" spends
someone else's sprint.

**Bonus, on retiring an oracle.** S80 excluded `campaign_map` from MA's byte-identical gate as
"renders mutable save state, not a valid oracle". S81 reversed that: the gate now **pins** a
committed reference save around the capture and restores the player's own afterwards, and the
screen is back to 0 px. Pinning the state is nearly always cheaper than excluding the screen —
an excluded screen silently stops testing, whereas a pinned one re-proves its reference every run.

## 8z. The eventsink matches types EXACTLY — every event registered on a BASE class is dead (BoB S144) **[ENGINE]**

**Check this on your side today; it is probably silently true for you too.** BoB's general OCX
eventsink (`bob_evt_fire`, `bob_eventsink.cpp:39`) matches a handler with
`v[i].id == id && v[i].dispid == dispid && *v[i].ti == *dt` — **exact `type_info` equality, with no
walk up the base classes.** Every call site passes `typeid(*dlg)`, i.e. the *runtime* (derived)
type. So an `ON_EVENT` registered on a base class can never fire for a derived object.

That is not a corner case here. `ON_EVENT(RDialog, IDJ_TITLE, 3 = OK, OnOK)` and its Cancel/Help
siblings (RDIALOG.CPP:1179) are how the engine delivers the **title-bar ✓ / ✕ / ? buttons** — the
ones visible on essentially every gold shot of a dialog. They are registered on `RDialog`, and every
real dialog is a derived class, so **no dialog in the port has ever been able to receive a title-bar
OK or Cancel through the sink.** It presents as "the ✓ button does nothing" or, as it did for us, as
a scaffold that fires an event and gets silence — the handler is right there, the id and dispid are
right, and nothing happens.

**Two ways out — and CORRECTION (BoB S145): the cheap one does NOT do what this note first claimed.**
(a) Firing under the registering type — `bob_evt_fire(dlg, &typeid(RDialog), IDJ_TITLE, 3)` — was
described here as "still reaches the derived override". **Measured: it does not.** A gated trace in
`LWDirectives::OnOK` never fired, while the dialog closed anyway. The reason is structural, not a
virtual-dispatch subtlety: **the logged child is an RDialog *panel wrapper*, and the real dialog is
a separate object inside it.** `LWDirectives::Make` returns
`MakeTopDialog(..., DialBox(FIL_D_LWDIRECTIVES, new LWDirectives(dirres)))` — the panel *contains*
the `LWDirectives` (a `RowanDialog`) as its `dial`. So the OK reached `RDialog::OnOK`, which does
`EndDialog(IDOK)`: the panel closed, the derived handler never ran, **and it looked like success**.
That is the dangerous shape — a workaround that silently performs the base behaviour and skips the
derived logic is worse than one that fails loudly. Whatever you drive, first check whether the thing
you hold is the dialog or its panel wrapper.
(b) Make the sink walk base classes — still the general fix, still wants its own regression gate.
**And note (b) alone would not have fixed this case either**, for the same structural reason.

**Why it stayed invisible:** every event the port had wired so far was registered on the *same*
class that received it (`CSCampaign`, `CSQuick1`, the toolbars), so exact matching was
indistinguishable from correct matching. The first base-registered event anyone tried was the first
failure. **MA: if you host the Player Log's `?`/`✓` title-bar buttons (your S60/S61 thread), this is
very likely the reason a click on them does nothing** — the buttons draw, the handler exists, and
the sink quietly declines to connect them.


## 8-MA82. Checking for a sibling's bug found a bigger one of our own — and the trap inside the genuine handler (MA S82) **[ENGINE]**

*(First section using the collision-proof `§8-<port><sprint>` scheme §8x proposed. It is used here
because the letter counter collided a **second** time — MA's §8y (S81) and BoB's §8y (S144) were
both appended before either re-grepped. Resolved by BoB's own rule: MA note 30 was already
published citing §8y, so BoB's S144 section became §8z. Letters are a shared mutable counter with
no lock; sprint-tagged ids cannot collide, so new sections should use them.)*

**The check.** BoB §8z/S145 warned that firing OK on a logged child can hit the RDialog **panel
wrapper**, whose `OnOK` is just `EndDialog(IDOK)` — the derived handler never runs and *it looks
like success*. Checked on the MA side by printing `typeid(*parent).name()` at the fire site:
MA's title bar resolves to **`9CPlyr_log`**, the derived dialog, because the host records each
control's own parent node rather than the logged child, and the sink matches that node's runtime
type. **Not affected.** Worth stating the structural reason rather than just the verdict: *what you
hold* is decided when the control is registered, not when the event is fired.

**What the check actually found.** The OOB dialogs were **render-only**. MA's map idle routed clicks
to the two toolbars and nothing else, so every information dialog (Player Log, Squads, Bases, DIS,
Overview, Weather) painted perfectly and ignored every click — no tabs, no tick, no rows. Three
things had been quietly *explaining* that instead of exposing it: a scaffold env hook
(`MA_OOB_PLAYERLOG_TAB`) existed to switch tabs "for capture purposes"; `ma_tabs_hit` sat in the
tree **declared with no caller at all**; and MA had answered BoB's "how do you dismiss a dialog"
question with the *toolbar* route without noticing that the *user's* route did not exist. **When a
capability is only ever exercised through scaffolding, that is evidence the real path is missing —
an unused hit-test function is a load-bearing clue, not dead code.**

**The fix worth copying: mirror the paint walk for hit-testing.** The click walk is the paint walk
with `ma_ole_draw_toolbar` swapped for `ma_ole_toolbar_click` — same tree, same `MaXYOffset()`
offsets, children before parents (they paint on top). Hit rects then cannot drift from drawn rects,
which is the usual way this goes wrong. Two rules fell out: an open dialog gets **first refusal** on
the click, and a click inside its rect that hits no control is **swallowed** — otherwise it falls
through and pans the map behind the dialog.

**★ The trap: the genuine handler you want to drive may itself contain an unported call.** A dialog
title bar is a `CRButtonCtrl` with the tick/help flags set, and the control already owns the band
arithmetic (`ICONWIDTH`) that decides tick-vs-help-vs-body, then fires OK / Cancel / Clicked from
`OnLButtonUp`. Driving the real control is the right instinct (it is why MA never had §8u's
hardcoded-column bug) — **but `OnLButtonUp`'s first statement is**

```
CDialog* phintbox = (CDialog*)GetParent()->SendMessage(WM_GETHINTBOX,NULL,NULL);
phintbox->ShowWindow(SW_HIDE);          // ON_MESSAGE is an EMPTY macro in compat -> 0 -> NULL deref
```

So the port drives the **DOWN** half (which sets the flags and returns early) and reports the dispid
the UP half *would* have fired. Generalise: before delegating to a genuine handler, read it for
compat-stubbed calls — `SendMessage` results that are dereferenced, `ON_MESSAGE` routes that do not
exist (same family as §8i's `WM_GETSTRING`), sound/capture side effects. "Drive the real handler"
and "drive *all* of the real handler" are different commitments.

**Scoping a change to a shared click path.** The band logic is only consulted for buttons that
actually carry tick/close/help flags (`ma_button_title_hit` returns −1 otherwise), so every toolbar
and dialog button that already worked keeps firing plain `Clicked` down the identical path — no
regression surface, and the parity/stress/ASan gates stayed green without needing a rebase.


## 8-MA83. The unchecked-`SendMessage` class has a single root: the port's dispatcher answers 0 for routes it never implemented (MA S83) **[ENGINE]**

BoB note 19 asked MA to check every `SendMessage(WM_GETHINTBOX)` site individually, having found the
same idiom spelled safely in one control and unsafely in another. Doing that found the sites — but
reading the **dispatcher** instead of the call sites found the *class*.

**The root.** `RDialog::OnRowanMessage` (the port's stand-in for the engine's `ON_MESSAGE` map, which
the compat layer defines as an empty macro) implements **8 of 14** routes and ends with
`default: return 0`. Six routes are therefore answered "0" indistinguishably from "the handler
returned NULL": `WM_GETHINTBOX`, `WM_GETCOMBODIALOG`, `WM_GETCOMBOLISTBOX`, `WM_ACTIVEXSCROLL`,
`WM_GETSTRING`, `WM_COMMANDHELP`. Every unguarded deref of a `SendMessage` result is downstream of
that one `default`. Two cheap moves make the class tractable rather than endless:
1. **List the unrouted messages in the `default` case, each with why it is still unrouted** — the
   gap becomes documentation instead of absence.
2. **Trace it** (`MA_TRACE_MSG`): print the message id and the receiving class. MA measured that on
   its campaign/OOB path exactly one unrouted route is actually exercised (`WM_GETSTRING`, on four
   classes) — which turns "audit everything" into "audit the one that fires".

**On the call-site sweep itself.** MA hardened four: `CRButtonCtrl::OnLButtonUp` and `::OnMouseMove`
(the hover tooltip — five derefs plus a deref of the DC it hands back), and both `CRComboCtrl` sites.
The last one survives today only because its enclosing `if (… && m_hWnd)` is false in the port —
**an accidental guard, not an intentional one**, and the kind that stops guarding the moment
something hosts a real HWND. Note the two ports' control sources are *different revisions*: BoB
counted 4 hint-box sites per control, MA has 2, both in `RBUTTON`. Counts do not transfer; the
dispatcher framing does.

**★ Counter-finding on case-variant twins — verify per file, in both directions.** BoB warned that
`rbuttonc.cpp` and `RBUTTONC.CPP` are distinct stale files and patching the wrong one is silent
no-op work. Tested empirically in MA's tree (append a probe to one name, grep the other): they are
the **same** file — either name edits the compiled one. Yet MA's own `CLAUDE.md` records twins that
genuinely *have* diverged (`VIEWSEL.CPP` vs `Viewsel.cpp`). So the property is **per file and per
tree**; the two-second probe settles it and no amount of `find` output does.

**And the bug the sweep was standing next to: a HALF-APPLIED for-scope hoist.** MA's port scripts
rewrite MSVC's for-scope-leaked loop variables by declaring them at function scope. Where the
rewrite added the declaration but did not remove the inner one, the loop variable **shadows** the
hoisted variable, and any use after the loop reads the outer one that nothing ever wrote:
```c
int i;                                   // hoisted
for (int i = MAX-1; i > j; i--) …        // shadows it
target[i].activity = …;                  // reads the uninitialised outer i  -> wild index, SEGV
```
This had kept two OOB dialogs deferred as "crashes deeper in OnInitDialog" for ~30 sprints, with the
recorded cause naming the wrong class entirely (a symbolized backtrace named `CSupply`, not
`CComit_e`). One line fixes it. **If your port used the same hoisting tooling, sweep for
`int X;` followed by `for (… int X …)` in the same function, then check only those where `X` is read
after the loop** — in MA that was 15 matches across 7 files, and exactly one was harmful. Same
detection rule as the rest of the uninit family: the wrongness is in what nobody wrote.


## 8-MA84. `OnGetFile` holds its block PER DIALOG, but the engine allows one open per FileNum — two dialogs sharing one icon is fatal (MA S84) **[ENGINE]**

**The rule.** `fileblocklink::makelink` serves reuse only from the **freed** cache. If the FileNum is
found in `openfiles` — i.e. somebody is still holding it — it calls `ReallyEmitSysErr("Opened file
block (%x) again without closing!")`, which is `SayAndQuit`. One open per FileNum, engine-wide.

**The collision.** `RDialog::OnGetFile` opens a `fileblock` and stores it in **`m_pfileblock`, a
per-dialog member**, holding it until that dialog's next `OnGetFile`/`OnReleaseLastFile`. So the
moment two *different* parents draw controls that share one piece of art, the second one to paint
opens a block the first is still holding, and the game exits. MA's case: the map toolbar's Authorise
button and the Authorise dialog's own button both use `FIL_ICON_MISSIONRESULTS` (0x6a78).

**Why it stayed hidden, and the general warning.** Nothing collides while only *one* thing paints
per frame. MA's OOB dialogs were render-only until the click work made them paint every idle
(§8-MA82) — **so this bug was created by making a subsystem work, not by breaking one.** Any port
that starts painting a second control tree over an existing one inherits it. BoB: your map OOB
dialogs are the same shape, and the moment they paint alongside the toolbars you will meet this.

**The fix, and the part that matters.** Add a read-only `fileman::MA_GetOpenFileData(FileNum)` —
sibling of the `MA_IsFileOpen` this family already needed once (§S79's debrief preload) — and have
`OnGetFile` serve the already-open block's data instead of duplicating the open. **Do not store the
borrowed block in `m_pfileblock`**: you do not own it, and releasing someone else's block turns a
quit into a use-after-free. The pointer is valid for the draw: the holder only releases on its own
next call, and a released block keeps its data in the freed cache.

**Diagnostic worth stealing.** Put a `backtrace()` behind an env var at that fatal branch. This bug
family has now been diagnosed three times across the two ports and *each* time the mechanism was
argued about first and traced second; the trace names the whole chain
(`paint walk → draw_toolbar → CRButtonCtrl::OnDraw → WM_GETFILE → OnGetFile → makelink`) in one run.

**Two recipe traps found alongside, both about addressing a control:**
1. **A toolbar control's screen position is the offset passed at PAINT time**, not the parent
   toolbar's `m_maX/m_maY` (which are 0). MA's `#ID` resolver added the latter and landed ~50px off,
   so every toolbar recipe had been hand-computed — twice wrongly in one sprint. Fix: record
   `drawOx/drawOy` on the hosted entry when `draw_toolbar` draws it, and resolve from that. Same
   principle as mirroring the paint walk for hit-testing: **store what paint did; never re-derive
   it.**
2. **★ Numeric control ids are AMBIGUOUS.** MA's `RESOURCE.H` defines **five** symbols as 2074
   (`IDC_DIRECTIVES`, `IDC_AUTHORISE4`, `IDC_FILTER_RED_TROOP`, `IDS_PILOTNAMES_74`, `IDC_DEVDESC`).
   A `#2074` recipe resolved to whichever hosted control matched first — the filters toolbar's — and
   fired `Clicked` at a class with no handler for it: a silent no-op that looks exactly like "the
   feature is broken". Any `#ID` recipe form on either port needs a **parent qualifier** to be
   deterministic. Worth checking before trusting a headless drive that "does nothing".

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
