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

## 8-BoB155. Every dialog is a PANEL wrapping the real dialog — and it has cost two separate multi-sprint hunts **[ENGINE]**

**The fact:** in this engine a "dialog" you can get a pointer to is almost always an `RDEmptyD`
placeholder panel (or `RFullPanelDial`) whose `fchild` is the object that actually implements the
screen. `LWDirectives::Make` returns `MakeTopDialog(.., DialBox(art, new LWDirectives(dr)))`; the
logged child, the thing `DestroyWindow` is called on, and the thing you hold when you enumerate a
toolbar's children are all the **wrapper**, not the dialog.

**Measured, per dialog** (BoB S155, `BOB_TRACE_DESTROY`), showing wrapper → real dialog → hosted
control count:

    RDEmptyD -> LWDirectives    184 hosted controls
    RDEmptyD -> DirectiveResults  3
    RDEmptyD -> LWMissionFolder  23
    RDEmptyD -> TakeOverOffered   6   (x4 distinct instances in one campaign day)
    RFullPanelDial -> CampaignEnterName 5

**It has now caused two expensive, unrelated hunts in this port:**
1. **Driving a handler (S144-S146, 3 sprints).** Firing the title-bar OK at the logged child ran
   `RDialog::OnOK` on the *wrapper* -> `EndDialog` -> the panel closed and **reported success**
   while the derived `LWDirectives::OnOK` never ran, so `MakeLWPackages` never built the day's
   raids. Fixed by descending to `fchild`.
2. **Releasing resources (S153-S155, 3 sprints).** Teardown hooked at the wrapper freed **1**
   hosted control instead of 184, because the controls belong to the contained dialog. Fixed by
   walking `fchild`/`sibling`.

**The rule:** *before driving or destroying a dialog, ask whether you are holding the panel or the
dialog.* Print `typeid(*p).name()` — `RDEmptyD`/`RFullPanelDial` means you are holding the wrapper
and whatever you do will silently apply to the wrong object. Both failures above looked like
success: an OK that closed a window, a teardown that freed something. **This wrapper is the single
most expensive piece of implicit knowledge in the codebase**; MA has the same `DialBox`/`RDialog`
structure, so the same two traps are available there.


## 8-MA91. Audit the compat layer's EMPTY MACROS — every one silently discards a registration the game source makes (MA S87–S88) **[ENGINE]**

Three separate features on the MA side turned out to be dead for the same reason, found one broken
screen at a time over four sprints. They are not three bugs; they are one **class**, and it is worth
auditing deliberately rather than discovering it feature by feature.

The compat layer defines MFC's map macros as no-ops so the game's `BEGIN_MESSAGE_MAP` /
`BEGIN_EVENTSINK_MAP` blocks compile. Each no-op silently throws away a registration the game
source *makes*, and nothing errors — the control draws, the handler exists, the click does nothing:

| macro | MA status | live registrations |
|---|---|---|
| `ON_MESSAGE` | empty → the port's own `OnRowanMessage` dispatcher covers 8 of 14 routes (§8-MA83) | — |
| `ON_EVENT` on a **base class** | dead: the sink matches the runtime type exactly (§8z) | — |
| **`ON_EVENT_RANGE`** | **was empty — every range-registered handler dead** | **9 across 4 classes** |
| `ON_COMMAND` | empty; **checked: not worth implementing** — 29 registrations, all MFC framework menu ids (`ID_FILE_OPEN`, `ID_APP_ABOUT`, `ID_HELP`) with no port equivalent | 29 |
| `ON_BN_CLICKED` | empty; 14 registrations, 13 of them commented out upstream | 14 |

**`ON_EVENT_RANGE` was the expensive one.** It registers one handler for a span of ids, which is how
this engine wires *grids* of controls — MA's `CBases` binds 30 airfield buttons that way, and
`CMapFilters` its map-layer filters. Both dialogs' entire purpose is being clicked; both were inert.
Implementation is small: register the thunk per id in the span, and **pass the fired id as the
handler's first argument** (what MFC does — `void OnClickedAfButtonID(long id)`), which needs a flag
on the registration so the fire path knows to supply it.

**The audit is cheap and worth doing in one pass.** `grep -c` each empty macro's live registrations
before implementing anything: MA's count above took minutes and showed that `ON_EVENT_RANGE` was
load-bearing while `ON_COMMAND`'s 29 sites were framework menu commands not worth a line of work.
**Not every dead registration deserves reviving — but you want to decide that from counts, not from
whichever screen you happened to open.**

**Two things fall out of implementing one, both worth expecting:**
1. **Upstream bugs surface.** MA's `CSqdnlist` eventsink map registers *its own* handlers under
   **`CBases`** (`SQDNLIST.CPP:246-248`) — a copy-paste slip in the shipped source that was inert
   while the macro was empty and became a compile error the moment it was not. Expect one or two.
2. **The second build system.** MA has `CMakeLists.txt` (Ninja, primary) and `port/rebuild.sh`
   (fallback, **and what the ASan build uses**). A new TU added to one and not the other links fine
   in the primary build and fails only in the ASan gate. Add new files to both.

**Related, from the same pair of sprints (MA S89–S90), on judging a feature "broken":**
- **Check whether it is switched ON.** MA's radar-assisted gunsight looked unimplemented; it is
  gated entirely on two opt-in difficulty settings and works when they are set. Nothing was wrong.
- **Check whether the value you are watching is CLAMPED.** `RequiredRange = radarRange` is pinned to
  20 000…100 000, so locks at 1.2 M can never move it — a constant reading that means "out of range",
  not "not implemented".
- **Check your own trace before believing it.** A trace printed a suspiciously constant `X=2 Y=3`
  and nearly became a finding; the fields were `Float` and the trace cast them to `long`. **A
  constant value deserves the same suspicion as a surprising one.**

## 8-BoB156. A capability only ever exercised through SCAFFOLDING is unproven — BoB's map dialogs were render-only for 40+ sprints (BoB S156) **[PROCESS]**

MA note 31 §3 asked BoB to check whether its map OOB dialogs accept real clicks. They did not, and
had not since S113. The map click dispatch was:

```c
if (bob_map_click_toolbars(cx, cy)) { if (!g_bob_map_active) return; }
else bob_map_select(cx, cy);          /* unit selection */
```

Toolbar buttons, then unit selection — **no branch into an open dialog at all**. Dialogs opened,
painted, and rendered hosted controls correctly; they simply could not be clicked.

**Why it stayed invisible for so long is the transferable part** — and the useful form of the
lesson is sharper than "scaffolds are bad", because most scaffolds are fine. Sort yours into two
kinds:

| | what it substitutes | does it prove the real path? |
|---|---|---|
| **Shallow** | an **input** — synthesizes a coordinate/keystroke, then falls into the same dispatch a real event reaches | **Yes.** Everything downstream is production code. |
| **Deep** | a **call** — invokes a handler, fires an event, or pokes a control directly | **No.** It enters *below* one or more layers, and can never report that those layers are missing. |

BoB's `BOB_MAP_CLICK` and front-end `BOB_AUTOCLICK` are **shallow**: they compute a point and hand
it to the same `if (haveClick)` / `if (got)` block the SDL mouse feeds (`BOB_AUTOCLICK`'s own
comment: *"synthesize a click on item N's centre so the real hit-test path below runs"*). Those
were never the problem.

The OOB dialogs were driven only by `bob_oob_accept_directives` / `bob_oob_close_dialogs`, which
call **`bob_evt_fire` directly on the dialog** — deep. S144–S146 used it to drive the Directives OK
and got the entire LW orders flow to complete: raids built, flown, landed. That reads as
overwhelming evidence the dialog works, and it says nothing whatever about whether a click can
reach it. Extensive parity testing of those same dialogs (gold-shot value parity, host counts,
teardown, draw rects) also never touched the dispatch.

**The check to run on your own port** — cheap, and it produces a concrete list:

> For each capability you believe works, name the last time it was exercised by something that
> entered at the **same layer a player enters at**. If every driver is a *deep* scaffold, the
> capability is unproven, not working — however rich the evidence downstream of it looks.

The failure is silent by construction, and note the trap: the deeper the scaffold, the more
impressive the evidence it produces, because it drives the working part of the system directly.

**Shape of the fix (BoB's, reusable):** give an open dialog **first refusal** ahead of the existing
click consumers; walk each toolbar's logged children **and descendants** (§8-BoB155 — controls live
on the contained dialog, not the panel wrapper); and **swallow in-dialog misses**, so a click on
dialog background does not fall through and select whatever is underneath. Keep it revertible
(`BOB_NO_OOB_CLICK`).

**One thing BoB got for free that MA had to engineer:** hit rects are the hosts' *own last-drawn
screen rects*, recorded at paint time, so hit-testing cannot drift from what was painted. If your
port hit-tests against separately computed geometry, that drift is a live bug class; recording the
drawn rect and testing against it removes it by construction.

**Verifying a fix like this needs a noise floor.** "With-click and without-click frames differ" is
not evidence on its own — in a codebase whose signature bug is uninitialised-read variance, two
identical runs may differ too. Run the *same* recipe twice with no click first and measure that
delta; only the difference outside it is signal. In BoB's case the noise floor was a 16×8 clock
field, leaving 4,666 of 4,742 changed pixels as genuine — including the directive grid going to all
zeros, which is what `IDC_RBUTTONREST` is supposed to do.

## 8-BoB156b. These are UNITY builds: a wrong-linkage declaration links anyway, until it doesn't (BoB S156) **[ENGINE]**

Rowan's `_MFC.CPP` / `_FULL.CPP` / `_LW.CPP` are **unity translation units** — they `#include`
the `.CPP` files themselves (`_MFC.CPP` pulls in `MainFrm.cpp` at line 79 and `fullpsys.cpp` at
line 105). MiG Alley is laid out the same way. Two consequences that bit BoB in S156:

**1. A language-linkage mismatch can silently succeed.** I added a call in `FULLPSYS.CPP` to a
function defined `extern "C"` in `MAINFRM.CPP`, declaring it in-body as plain `extern int f(int,int);`
— C++ linkage against a C-linkage definition. That is normally an undefined reference at link
time. It built and ran. The reason is *not* that the mismatch is benign: because both files land
in the **same** TU and `MainFrm.cpp` is included **first**, a prior C-linkage declaration is
already visible, so per `[dcl.link]/6` the redeclaration inherits C linkage. It links **by
include order**, not by correctness.

So the failure mode is delayed and misattributed: reorder `_MFC.CPP`, or split the unity build
for parallel compilation, and you get an undefined reference to a symbol that is plainly defined
right there — with nothing in the diff of that commit to blame. **Declare cross-file entry points
at file scope with an explicit `extern "C"`, next to the existing ones.** (BoB's `FULLPSYS.CPP`
already had `extern "C" int bob_map_click_toolbars(int,int);` at file scope — matching the
neighbour would have avoided this outright. House style was right; I skipped it.)

**2. The corollary for locating anything.** `nm` on a per-file object will not find these — there
is no `FULLPSYS.CPP.o`. Symbols live in `SRC/MFC/CMakeFiles/bob_mfc.dir/_MFC.CPP.o`. Grepping the
build tree for a per-source object and finding none is evidence about the *build layout*, not
about the symbol. Also: an in-function `extern` declaration is the same shape as the
`extern "C"`-inside-a-function-body error that has now broken this build twice (BoB S146, S153).
Both are fixed the same way — put it at file scope.

## 8-BoB157. Your headless harness probably cannot pump SDL at all — and `SendMessage` is an allowlist (BoB S157) **[ENGINE]**

Two findings from auditing what actually drives each capability (the §8-BoB156 check). Both are
almost certainly true of MiG Alley too — same compat layer, same harness design.

### (a) Under `SDL_VIDEODRIVER=dummy` the event pump never runs

BoB added a driver that pushes a **real `SDL_MOUSEBUTTONDOWN`** rather than injecting past SDL, to
prove the one layer no test had ever executed: the event handler and its logical→drawable scaling.
Headless, it produced nothing. A trace on `pump_events` calls #0/#100/#10000 printed **nothing at
all** — `SDL_CreateWindow` fails under the dummy driver (*"OpenGL support is either not configured
in SDL or not available in current SDL video driver"*), so no window exists, none of the
present/`BeginScene`/`SwapWindow` paths run, and the pump is never called once.

**Consequences worth internalising before you write another capture recipe:**

- Every headless click/key driver *must* enter below the SDL layer. That is a constraint of the
  harness, not sloppiness — and it means **no headless test in your project's history has said
  anything about your SDL input layer**, in either direction.
- Anything you want to prove about layer (1) needs a **real GL display**. On real GL, BoB's chain
  ran end to end in one go: `pump_events #0` → `SDL event POLLED` → `bob_gdi_get_click CONSUMED` →
  hit-test → handler fired.
- Corollary for evidence hygiene: a *null* result from a headless run is not evidence about the
  code until you have confirmed the harness can reach the code. Ask "can this rig physically
  observe the thing?" before concluding anything about the thing.

Cheap strong check while you're there: drive the same outcome from **two independent entry points**
(a real event at layer 1, an injection at layer 3) and compare the resulting frame. BoB's two agreed
**byte-for-byte** on the changed region while differing from the no-click control. Neither alone
rules out a scaffold artifact; together they do.

### (b) `CWnd::SendMessageA` answers only three messages; the rest die reporting success

BoB's compat `SendMessageA` handles `WM_GETFILE`, `WM_GETGLOBALFONT` and `WM_GETSTRING`, and
**`return 0`** for everything else. `SendMessageToDescendants` is `{}`. **`ON_MESSAGE(msg, fn)`
expands to nothing**, so every `ON_MESSAGE` row in every `BEGIN_MESSAGE_MAP` in the game is
decorative. This is §8-MA83's class and §8-MA91's class at once.

The game sends **20 distinct `WM_*` types**. A deduped one-line-per-id trace (`BOB_TRACE_MSG` —
*do not* trace per call; §8's 70 MB starvation lesson) caught **four firing in a single ordinary
run**: `WM_GETARTWORK`, `WM_GETXYOFFSET`, `WM_RELEASELASTFILE`, `WM_GETX2FLAG`. Several dead routes
have **real implemented handlers** on the other side — `RDialog::OnGetXYOffset`,
`RDialog::OnReleaseLastFile`, and four separate `OnSelectTab` implementations for `WM_SELECTTAB`.

**The tell that this is a subsystem gap and not a curiosity:** the port had already hand-delivered
two of these routes at individual call sites — one calling `OnGetXYOffset()` directly, one
delivering a swallowed `WM_GETSTRING` with the comment *"compat has no message-map dispatch … we
deliver it"*. **Two local workarounds for the same missing subsystem, written sprints apart, neither
recognising the other.** If you find yourself hand-delivering a second message, stop and implement
the dispatch.

*Method warning:* the static send-counts came from `grep -o "WM_[A-Z_]*"`, whose character class
excludes digits — it silently truncated `WM_GETX2FLAG` to a perfectly plausible `WM_GETX`. A regex
that can produce a **believable wrong answer** is the §8k(3)/§8m(2) hazard; the runtime census is
what caught it.

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

## 8-BoB158. Restoring a dead dispatcher is UNBOUNDED work — a constant-returning stub deletes the evidence of its own gap (BoB S158) **[ENGINE]**

BoB implemented the message dispatch that §8-BoB157(b) found missing. It works — and every step of
enabling it uncovered something else that had been invisible for the port's entire life. MiG Alley
should expect the same shape.

**1. A stub that returns a compile-time constant suppresses link errors, not just behaviour.**
compat's `SendMessage` was an inline allowlist returning a literal `0`. Game code reads:

```c
phintbox = (CDialog*)GetParent()->SendMessage(WM_GETHINTBOX, NULL, NULL);
if (phintbox) { CString realhint = CString(' ') + str + ' '; ... }
```

The optimizer proved `phintbox` NULL, deleted the block, and never emitted the `operator+` call — so
four `CString` single-char `operator+` overloads were **declared but never implemented**, and nothing
ever failed to link. Giving `SendMessage` a real dispatch made the branch opaque and turned the gap
into a link error. Likewise, while `ON_MESSAGE(msg, fn)` expanded to nothing it never evaluated
`msg`, so `WM_COMMANDHELP` **had never needed to exist**. *An empty macro stops its arguments from
having to be valid.* **Estimate accordingly: each route restored makes more never-executed code
reachable. Treat each new error as confirmation the fix works.**

**2. Check the stub is not defined in two places.** BoB's `ON_MESSAGE` was stubbed in
`compat/afxwin.h` **and** in the game's `GLOBDEFS.H`, which `#undef`s it and re-stubs it under the
port's own `#if defined(BOB_LINUX)` branch. GLOBDEFS.H wins. The first implementation compiled,
defined 146 registration functions, and **registered nothing** — it would have run, dispatched
nothing, changed no pixel, and read as "those routes are dead for some other reason".

**3. Count the registrations before testing the behaviour.**

```
$ objdump -d --demangle bob | grep -c 'call.*bob_msgmap_chain'   # 296  <- BEGIN_MESSAGE_MAP worked
$ objdump -d --demangle bob | grep -c 'call.*bob_msgmap_add'     # 0    <- ON_MESSAGE did not
```

Seconds, and it says *which half* failed — a behavioural test can only say "no effect". **Check the
right symbol, though:** the per-class registrar is an empty struct whose constructor GCC inlines
into `_GLOBAL__sub_I`, so `nm | grep <registrar>` reads **zero even when it works**. Absence of a
symbol is not absence of the call.

**4. The declared map base is often NOT the real base — do not build the chain on it alone.**
`BEGIN_MESSAGE_MAP(LWDirectives, CDialog)` while `class LWDirectives : public RowanDialog`, with no
`ON_MESSAGE` rows of its own. A chain walk keyed on the declared base never reaches `RDialog`'s
handlers. Fix that is correct regardless of what the map declares: register a per-class probe
`bool(*)(void*)` doing `dynamic_cast<T*>(...)`, emitted from the same macro where `T` is known, and
scan probes **only on a miss**. That took BoB from partial coverage to `unhandled=0`.
(Do still walk the declared chain first — it is exercised and it works: BoB measured
`WM_GETXYOFFSET -> CRToolBar (depth 1)`, a base-registered handler reached from a derived object.
That is the §8z trap avoided *and demonstrated*.)

**5. Then you will hit §8-MA84 from the other side.** With every route live, BoB dies with
`FILEMAN.CPP: Opened file block (6d12) again without closing!` — the `WM_GETFILE` /
`WM_RELEASELASTFILE` open/release protocol has **never run** in this port, so its bookkeeping has
never had to balance. Suspect non-virtual handlers being resolved to a base class where a derived
override exists, which unbalances get against release. **Keep the dispatch behind a flag until this
is settled** — the mechanism landing and the protocol being correct are two different milestones.

---

## §8-MA95 — The last unrouted consumer, and why the *test* was the hard part

**MA Sprint 95.** A play-tester reported that clicking any icon on the campaign map did nothing.
The expected dialog never appeared. Nothing in the game code was wrong.

**The mechanism.** On Windows the map dialog receives clicks from the message queue. This port has
no queue, so every click is routed by hand in the idle loop: open OOB dialogs get first refusal,
then the system box, then each toolbar. If all of them declined, the click was **dropped** — and
`CMapDlg` was never in the list at all. The engine's chain behind it was complete and correct:

```
CMapDlg::OnLButtonDown -> FindMapItem(point) -> m_buttonid
CMapDlg::OnLButtonUp   -> OnClickItem(m_buttonid) -> CMainToolbar::OpenDossier -> MakeSheet
```

**If you hand-route clicks, enumerate the consumers and name the one that gets the fall-through.**
A router built as a chain of "does this widget want it?" tests silently discards whatever no widget
claims, and the missing consumer is invisible in the code — there is no line to notice. BoB's map
and any other full-screen view that owned mouse input on Windows will have the same hole.

**Drive the engine's handler, not your own hit-test.** `FindMapItem` already accounts for bands,
map filters, scroll and zoom. Where the handler is `protected`, add one narrow public seam under
the port's `#ifdef` rather than a `friend` or a cast — it documents itself at the declaration.

**Deliver Down and Up in the same call for a click.** That is what a click without intervening
motion *is*, and it keeps `m_bDragging` FALSE — so the click takes the item path and **never enters
`OnMouseMove`, which dereferences `GetDC()` unchecked in this port**. Same rule as §8-MA82: the
genuine handler you drive may itself contain an unported call, so prefer the path that visits
fewest of them.

### ⚠ The part worth copying: a hardcoded coordinate is not a test

The first click, aimed at a point read off a scan of the map, resolved to **no item**. Same binary,
same pinned save. The canvas had grown **800×600 → 1021×644** between the scan frame and the click
frame, moving every icon by ~108 px. Read naively, that is "the routing does not work" — a fix
being abandoned because the harness lied.

The gate therefore **names no coordinate**. It asks the map's own hit-test where the icons are at
the frame it is about to click, clicks the first one clear of the toolbars, and passes on *item hit
+ dialog painted + process survived*.

This is the third distinct instance in this port of one failure mode: **a check whose result
depends on state the check does not control** — a save file the player can advance (§8-MA81, and
again in S94 when a play session silently turned a 9-dialog sweep into 0), and now a coordinate the
layout can move. The general rule: *if a gate's expected value was obtained by a human looking at
one run, the gate is measuring that run, not the code.* Derive the expectation inside the run.

---

## §8-MA96 — A blit that overhangs the screen must clip, not resize the screen

**MA Sprint 96.** Reported as "click-drag on the map messes up the display". The compat GDI's
`SetDIBits`/`StretchDIBits` **grew the canvas to fit whatever was drawn**. Windows clips a DC blit
to the client area; the size of a blit tells you nothing about the size of the screen.

The campaign map is tiled (256×256 blocks). As soon as it scrolls, tiles hang off the edges — and
each overhanging tile enlarged the whole screen, **every frame of the drag**:

```
[canvas] stretch_dibits at(0,456) dest=256x256 -> grow 1024x712
[canvas] stretch_dibits at(0,460) dest=256x256 -> grow 1024x716   ... for the length of the drag
```

**Rule:** growth is only legitimate from a blit anchored at or above the origin — something
*establishing* the screen. Content placed inside the screen that runs off an edge clips.

### The part that matters more than the drag

The same trace on a plain boot, **no input at all**:

```
[canvas] set_dibits at(0,0) dib=800x600      <- the front end establishes the screen: 800x600
[canvas] stretch_dibits at(-111,388) 256x256 <- a map tile hanging off the bottom
[canvas] grow -> 800x644 ... 30 growth events ... -> 1021x644
```

**The campaign map screen had been 1021×644 for as long as it had rendered — 221 px wider than the
game's actual screen — because its own tiles inflated it.** Every other screen was 800×600. A
long-standing wrong value that nothing ever contradicted, because the parity reference was captured
*from the port* and faithfully encoded it.

**If you keep native-vs-native references, ask periodically what would have caught a wrong value at
the moment it was first captured.** A byte-identical self-comparison locks in whatever was true on
day one, including bugs. Here the check that would have caught it costs nothing: *every screen
should be the same size, and that size should be the configured display mode.*

Anything positioned relative to a screen edge (`_cw - w - 4`) was being placed against an edge that
was not where the screen ended — worth checking in BoB, which positions the same system box.

### ⚠ And the test lied first: "0 px differ" == "nothing happened"

The drag gate's first version reported a **perfect lossless round trip** while the drag did
**nothing at all**. The hook pushes real SDL events deliberately (§8-MA93: a hook that bypasses the
path it tests proves nothing) — and **the event queue was never drained without a window**. S93
moved the synthetic input hooks above `if (!g_win) return;` and left the guard standing in front of
`SDL_PollEvent`. *The same bug, in its other half, one sprint later.* **When you move code past a
guard, check what else is still behind it.**

The gate now asserts three things, and the first exists purely to give the second meaning:

1. one-way drag **≠** baseline — proves the drag moves the map
2. round trip **==** baseline — proves panning is lossless
3. the release is **suppressed** as a click — proves a pan is not also a click

**Any "no difference" assertion needs a companion assertion that the action happened.** This port
has now been fooled by silence four times (§8-MA83, S64→S65, §8-MA93, here); a negative result is
only evidence when something independent shows the code ran.

### Related: a drag is not a click
A drag ends in a release, which raised the same one-click edge as a tap — so once map clicks were
routed (§8-MA95), **every pan finished by opening a dialog**. Windows fires a control only when
press and release land together; require that (≤4 px) before treating a release as a click.

---

## §8-MA97 — Art that is *named* after a control is not necessarily the art *for* that control

**MA Sprint 97.** The `CSystemBox` cluster (minimise / resize / **exit**) drew as three blank
buttons, so the play-tester could not leave the campaign. `F_GRAFIX.G` has `FIL_ICON_THUMBNAIL`,
`FIL_ICON_ZOOMIN` and `FIL_ICON_CLOSE1`, named after the three control ids — **and two of the three
are the wrong pictures.** They render as unrelated map glyphs.

The gold shot settles what the buttons look like; a name in a header does not. Identify art by
**comparing renders against gold**, and make that cheap — a runtime override
(`MA_BTN_ART="id=0xNNNN,…"`) turns "which of these twelve file numbers is it?" into minutes instead
of a rebuild per candidate. The final mapping was confirmed self-consistent by *behaviour*
(`IDC_ZOOMIN` drives `OnGoBig`/`OnGoNormal`, so `FIL_ICON_SCREENSIZE` is right), which is the
cross-check worth insisting on.

### A widget must not change the state of the screen it draws on
Drawing the box left a different **GDI font** selected in the screen DC, and the campaign map's date
readout — drawn from the same DC later in the frame — inherited it and rendered in a plain sans.
The failure appeared **top left, nowhere near the box**. Save/restore the DC's font (and any other
selected object) around a composite draw.

**The check that makes this convincing:** after the fix, the *only* pixels differing from the
previous reference were the box's own 72×48 rect at its own position. "Parity still passes" is
weaker than "the diff is exactly the shape of what I added, and nothing else moved".

### ⚠ Giving a control art can *reveal* a second bug that was always there
With art, a **second copy** of the cluster appeared at the top-left and outlived the campaign, still
sitting on the title screen. `ma_ole_draw_all` had always been drawing those controls at their raw
template origin *in addition to* the parent-scoped draw — **with no art it painted nothing, so
nobody saw it**.

BoB will have the same shape: the map toolbars escape the global pass only because their parent
`CDialog` is created **hidden**. That is an accident, not a rule. If a dialog is composited by a
parent-scoped path, **say so explicitly** (MA added `ma_ole_set_parent_scoped(dialog)`) rather than
relying on its parent's show state.

**And note how it was found: no gate caught it.** The parity `title` capture is a clean boot that
never enters the campaign, so it stayed byte-identical while the title screen was visibly wrong
*after an exit*. It was found by looking at the screenshot of the thing just built. Transition
states — screen A after coming from screen B — are a systematic hole in a per-screen parity suite.

---

## §8-MA98 — Four dead links in one chain, each invisible until the one before it was fixed

**MA Sprint 98.** "Clicking the '?' on a dialog yields no documentation screen." One user-visible
symptom; **four independent breakages**, all in the port, and each one only became observable after
the previous one was repaired:

1. **The title-bar hit router returned early for the help band** —
   `if (disp == 0) return 1; /* nothing to route to yet */`.
2. **`WM_COMMANDHELP` was not defined at all.** It is MFC's own private message (`afxpriv.h`,
   `0x0365`). This is **§8-MA83 in its purest form**: while `ON_MESSAGE` expanded to nothing it
   never evaluated its message argument, so the symbol had never been *required to exist*. Adding
   the route is what made it required — expect a compile error per route you restore, and treat
   each as confirmation the fix is real.
3. **`CWnd::SendMessage` only dispatched `WM_USER+`** (`>= 0x400`) and `WM_COMMANDHELP` is `0x0365`,
   *below* it. Name such messages explicitly rather than widening the range — everything else under
   `WM_USER` should keep returning 0 untouched.
4. **`CWnd::OnCommandHelp` was a non-virtual stub returning 0**, and `CDialog` overrode it back to 0.
   MFC routes this message **up the window chain** until something handles it, so the frame's
   override — the thing that actually opens help — was unreachable; and being non-virtual it could
   not have dispatched through a `CWnd*` even if called.

**The generalisable part: give the chain a return value you can read.** The measured send returned
**0** after fixes 1–3 and **1** after fix 4. That single number is what identified the last dead
link instead of guessing at it. When wiring a message route through a port, log *what the handler
returned*, not merely that you sent it — "delivered" and "handled" are different claims, and a
chain of stubs returns a plausible 0 at every step.

**Check for the same shape in BoB:** a non-virtual `CWnd` stub that a derived class "overrides"
compiles fine and silently never runs. Any `afx_msg`/`virtual` mismatch in the compat headers is a
route that looks wired and is not.

### Test recipes should name symbols, not pixels — and beware sscanf's return value
MA added a recipe form `#ID@Class:?` meaning "the help glyph of this title bar", resolved by asking
the control's **own** hit-test where its help band is. Glyph positions come from the button art and
move with dialog width and font.

Adding it hit a trap worth knowing: **`sscanf` returns the number of ASSIGNMENTS, not literals**, so
a format ending in a literal `:?` matches happily when `:?` is absent — the new branch silently
stole an unrelated recipe entry. Verify literal tokens yourself.

### Scope honestly when the port genuinely lacks a feature
Routing the click was a real fix; there is still **no WinHelp viewer**, so nothing is displayed. MA
recorded PO-4 as **half closed** and made the *gate print that boundary in its own output*, so no
later reader mistakes a green result for "help works". Reconnaissance first (`hlp_probe.py`: 44
topics, 35 context mappings, Hall compression identified) turns "should we build a viewer?" into a
decision with facts behind it — and is far cheaper than half-building one.

---

## §8-MA99 — ⭐ An oracle the failure mode can satisfy is not an oracle

**MA Sprint 99.** Decoding the shipped `MIG.HLP` documentation so the "?" button can show it. The
sprint was set up carefully: *the output must read as English* was chosen as the oracle up front,
precisely because this port keeps being fooled by checks that cannot see the bug.

It was implemented as "fraction of words that are common English words". Successive decoder fixes
took it **0.016 → 0.140 → 0.282 → 0.484**, printed as **PLAUSIBLE**. Here is the 0.484 text:

> *"airfield , different a : Summary automatically a KHowever icon have four a make, Patrolcampaign
> for a OtherNose, mousecampaign icon Forces"*

Gibberish. **A wrong phrase-table decoder emits real dictionary words in the wrong order — exactly
what a word-frequency metric rewards.** The metric did not just miss the failure; the failure mode
*maximised* it. Three consecutive "improvements" were measured by a number that could not tell
success from the thing being measured.

**The rule: design the oracle by asking what the FAILURE MODE would score.** If a plausible wrong
answer satisfies it, it measures nothing. Prefer an independent reference the decoder does not feed:
here, `|TTLBTREE` stores every topic's real title, and correctly decoded topic text contains its own
title — that needs the right words *in the right place*, which a scrambled decode cannot fake. It
reports 0/39 today, correctly.

This is the fifth time this port has been fooled by a check that could not see the bug (§8-MA83,
S64→S65, §8-MA93, §8-MA96, here) and the first where the check was the one designed as the
safeguard. Consider it the general form of the other four.

### Two concrete WinHelp findings, if you ever read a .hlp
- **`|PhrIndex`'s bit reader is LSB-first over 32-bit DWORDs**, not MSB-first over bytes. The
  natural guess is *almost* right: the phrase image decodes to correct alphabetical fragments and
  only the **boundaries** land wrong (`aboutagainstaircraf` / `tair`). **Nearly-right output is the
  signature of a nearly-right bit order** — worth remembering for any packed format.
- **Topic links are addressed by `TopicPos` in a logical space of fixed `0x4000` blocks**, though
  each block decompresses to far less. Concatenating decompressed blocks and walking linearly
  desynchronises at the first boundary — and presents as **"only 6 of 44 topics exist"**, i.e. as
  missing data rather than as an addressing bug. When a count comes out far too low, suspect the
  addressing before suspecting the data.

### And on scoping a feature the port simply lacks
Four of five decode stages are solved and separately evidenced; the fifth (the Hall text opcode
table) is not, so **nothing was wired into the game** and the tool states its own status in its
header. Shipping a decoder that produces confident nonsense would have been worse than shipping
nothing — the "?" showing wrong documentation is harder to notice than the "?" showing none.

---

## §8-MA100 — ⭐ The 3D overlay font is RASTERISED AT RUNTIME, and the stub that broke it said so

**MA Sprint 100.** "Text doesn't print" for every 3D overlay readout — padlock info, in-flight map
menu, radio menu. Five sprints of investigation had gone past the cause.

`COverlay` does not load its font as artwork. It **builds a glyph atlas at runtime** by asking
Windows to rasterise each character: `ImageMap_Desc::MakeChar` → `GetGlyphOutline(...,
GGO_GRAY8_BITMAP, ...)`. The compat layer's stub:

```c
/* GetGlyphOutline glyph-rasterising API (OVERLAY renders overlay text via font
   glyphs). Stubbed for bring-up: returns 0 (no glyph bitmap) -> blank text now; */
static inline DWORD GetGlyphOutlineA(...) { return 0; }
```

Every glyph's alpha stayed zero, so overlay text was laid out, positioned and composited perfectly
and drawn **completely transparent**. **Check `GetGlyphOutline` in BoB before investigating any
"overlay text missing" symptom** — this engine's HUD font comes through it.

**Grep your stubs for the ones whose comments describe a user-visible consequence.** This one
announced the defect in its own text since bring-up. A stub that says "blank text now" is a bug
report nobody filed.

### Implementing GGO_GRAY8_BITMAP — the details that bite
Taken from what `MakeChar` actually consumes, not from the API docs:
- **levels are 0..64, not 0..255** (the caller masks `0x40404040` to split out the saturated bit)
- rows are `gmBlackBoxX` bytes **padded to a DWORD**
- `gmptGlyphOrigin.y` is height *above* the baseline (stb_truetype's `y0` is negative there)
- the `MAT2` is 16.16 fixed and this engine passes a **non-square** scale — scale the axes
  independently rather than assuming one factor

### ⚠ Two invalid instruments before one that works — and one would have concluded the sprint
1. **Screenshot.** Showed "10 20 30 40" on the altitude ladder after the fix. Convincing, and
   wrong: that is **cockpit art**, present with the fix and without it.
2. **Whole-frame A/B, glyphs on vs off:** 14187 px differ. Also worthless — **two IDENTICAL flight
   runs differ by ~2700 px.** A frame diff of a live simulation measures the simulation.
   *Establish that a comparison is repeatable BEFORE drawing a conclusion from it.* One run of the
   same config twice is the cheapest experiment in this project and it invalidated the method.
3. **What works:** count the ink in the atlas — deterministic, and the exact thing that was broken.
   `2666 of 16384` non-zero alpha bytes with the fix, **`0`** with the stub restored.

Keep the disable switch (`MA_NO_GLYPHS=1`). A switch that removes *exactly* the feature is a claim a
wrong fix cannot satisfy — §8-MA99's rule, applied on the first attempt rather than the third.

### It also retires an earlier conclusion of ours
§8-MA94 traced these glyphs to palette slot 252, found `WHITE == 252` made
`SetPaletteEntry(252, GetPaletteEntry(WHITE))` a self-copy no-op, and reported the text as "rendered
correctly and drawn transparent". Writing real white into 252 changed nothing — recorded at the time
as "so the texels don't index 252 either". Correct, and the reason is that **there were no texels**.
A true observation about the wrong layer will happily survive several sprints.

## §8-MA101 — ⭐ A blend-factor table that is off by one renders everything *correctly* and *invisibly* **[ENGINE]**

**Found in MiG Alley S115 (PO-12 phase 3, the DX5 execute-buffer path). BoB has the same code and
the same latent fault — `gl_blend()` in `SRC/compat/bob_video.cpp` is shared by both ports.**

The MiG Alley hardware renderer submitted 562909 triangles to GL over 1406 scenes with no GL error,
correct screen-space coordinates, non-degenerate areas, the right thread, the right context — and
produced a black screen. The whole-framebuffer count at `EndScene` was 0 non-black pixels while an
immediate-mode control quad drawn *at the same point through the same projection* landed exactly
its 10000 pixels.

The cause was one table:

```c
/* WRONG -- entries 4..10 shifted by one, 8 and 11 absent */
case 4: return GL_SRC_ALPHA;            /* D3DBLEND_INVSRCCOLOR ! */
case 5: return GL_ONE_MINUS_SRC_ALPHA;  /* D3DBLEND_SRCALPHA    ! */
case 6: return GL_DST_ALPHA;            /* D3DBLEND_INVSRCALPHA ! */
```

The engine asks for `SRCBLEND=D3DBLEND_SRCALPHA(5)`, `DESTBLEND=D3DBLEND_INVSRCALPHA(6)` — the
ordinary "draw this normally" pair. It received `GL_ONE_MINUS_SRC_ALPHA` for the source factor, so
with the opaque alpha the engine actually writes (0xff) the source was multiplied by 1−1 = **0**.
Every triangle rasterised perfectly and contributed nothing.

**Why this is worth a note beyond the one-line fix.** An off-by-one in an enum→enum table is not a
crash, not a warning, not a GL error, and not a visual artefact: it is *absence*. It looks exactly
like "the geometry never got submitted", which is where four sprints of scoping effort naturally
point. What separated them was a **control arm drawn through the identical state at the identical
moment** — when the control lands and the payload does not, the fault is per-draw state, and the
suspect list collapses from "everything" to "the handful of things this draw sets differently".

Two predictions about *which* state were wrong (depth, both times) before the measurement named
blend. Cheap to be wrong when the experiment is one env var and one run; expensive to be right by
reasoning alone.

**Action for BoB:** take the corrected table (D3DBLEND 1..11 → GL, `d3dtypes.h:274` is the enum).
BoB's DX7 path passes `D3DRENDERSTATE_SRCBLEND/DESTBLEND` through the same function, so any BoB
geometry using SRCALPHA/INVSRCALPHA blending is currently being multiplied out in the same way.

---

## §8-MA102 — `sprintf("%s", <CString>)`: an MFC idiom that works by accident on MSVC and prints pointer bytes under GCC

**Found in MA (S135, 53 sites); applied to BoB (S164, 126 sites). Applies to any Rowan MFC port
built with GCC.**

The engine builds display strings like this, everywhere:

```c
sprintf(buffer, "%i. %s", n, RESLIST(MAIN_WP_GAP, currmainwp));   // RESLIST returns CString
templatename = CSprintf("%s: %s", RESSTRING(PATROL), GetTargName(pk.packagetarget[0]));
```

MFC's `CString` is a single `char*` member, so on MSVC passing one through `...` happens to push
that pointer and `%s` prints the string. It is a well-known MFC idiom precisely because it works.

**Under GCC it does not.** `CString` has a user-defined copy constructor and destructor, so it is
not trivially copyable, and GCC passes such an object through `...` by **invisible reference** —
`%s` receives the *address of the object* and prints the raw bytes of the pointer stored there.

Proved in isolation rather than argued about, with a five-line program that replicates only the
ABI shape (one pointer member, user-defined copy ctor and dtor), built `-m32` with the port's
compiler:

```
varargs   -> "0 9\xef\xbf\xbd\xef\xbf\xbdNm"   (len 9)
with cast -> "0 Nm"                            (len 4)
```

**The failure mode is what makes it expensive.** It never crashes and never logs. It draws a few
bytes of rubbish where a word should be, which from across the room reads as *"the label is
missing"* or *"the dialog is empty"* — and that is exactly how it was reported in MA, repeatedly,
as several different UI bugs.

**Fix:** an explicit `(LPCTSTR)` on every CString-valued argument in the *variadic* part. Do NOT
blanket-wrap the whole line: a `CString` in the **format-string** position is a declared parameter
and converts implicitly, so it is already correct, and wrapping other operands can change the
expression (`RESSTRING(A) + CSprintf(...)` is CString concatenation, not an argument).

**Finding them:** the four `RES*`/`LoadResString` macros are not the whole set. Enumerate every
function *declared to return `CString`* in the headers, then look for those names in the argument
list of any `sprintf`/`wsprintf`/`CSprintf`/`Format` call. In BoB that added `GetTargName` and
`SubName`; MA's first pass missed the equivalents.

**Detector, worth having permanently:** trace what reaches the text rasteriser and report any
string containing bytes outside printable ASCII (`BOB_TRACE_GARBAGE=1` in BoB,
`MA_TRACE_GARBAGE=1` in MA). It catches this whole class at the point of damage, including sites a
grep would miss. Filter by substring rather than capping the print count — a fixed budget is spent
by whatever draws first, which is always the menu.

**Honest scope note (BoB, S164):** all 126 sites are corrected and the mechanism is proven, but no
*live* instance has yet been reproduced on screen — every affected call sits on a campaign dialog
(RAF Tasks, intercept offers, weather, waypoint lists) that no current recipe drives. The front-end
gate screens are clean with the detector armed, and the control arm confirms they were clean before
the fix too. So this is a latent class removed on evidence of mechanism, not a reproduced defect
repaired.

---

## §8-MA103 — `CDialog::DoModal` returning a stub answers every confirmation the game ever asks

**Found in MA (S138); same defect present in BoB, fixed there S165. Applies to both ports.**

`RDialog::RMessageBox` is the engine's confirmation box: it fills an `RMdlDlg` with a title,
message and up to three button captions, then returns `m_pMessageBox->DoModal()`. In both ports
`CDialog::DoModal` was `{ return -1; }` and `CDialog::EndDialog` was `{}`, so **every caller in the
game received -1** — a value none of them expect:

| caller | code | what -1 does |
|---|---|---|
| `CMainFrame::OnBye` | `if (rv==0) save; else if (rv<2) quit;` | **quits the campaign without asking** |
| BoB `LWDIRECT` bad weather | `if (RMessageBox(...)==1) badweather=false;` | never offers to fly; the period is always skipped |
| BoB `LWDIRECT` aircraft allocation | branches on `rv` | always the not-chosen branch |

The quit path is the one that matters: the player is thrown out of a campaign and the dialog the
game was written to show never appears. It is silent — nothing logs, nothing crashes.

**Fix:** a real nested loop on `RMdlDlg` only — create + `OnInitDialog`, then *pump input → let the
dialog paint its own art → draw its hosted controls → present*, until a button calls `EndDialog`
(0 = OK, 1 = Cancel, 2 = Retry). Route input to that dialog alone while it runs; `RMessageBox` has
already disabled the toolbars around the call. Keep it **scoped to `RMdlDlg`** — the other
`DoModal` call sites are forwarding overrides, and a nested loop under a dialog the port drives
differently would hang rather than fail visibly. **Bound the loop**: an undismissable modal must
return the safe answer rather than freeze the game.

**Choose the no-answer default deliberately.** Not 0, and not -1: pick the code every caller reads
as *do nothing*. In both games that is **2** — `OnBye` stays in the game, the weather prompt leaves
`badweather` set.

**Two port-specific traps, and they differ between the ports** — worth knowing before copying code:
- **Coordinates.** MA's hit-test needs dialog-local coordinates (its click walk mirrors its paint
  walk). **BoB's `bob_ole_click` takes SCREEN coordinates**, because BoB's hosts record their
  last-drawn screen rect at paint time (S156/S160). Subtracting the panel origin in BoB — the
  correct thing to do in MA — misses every button, and looks exactly like "the modal ignores
  clicks".
- **Capture.** Both ports' screenshot hooks count *idle-loop ticks*, and a modal is precisely what
  suspends the idle loop, so neither can photograph one. Add a loop-local hook
  (`BOB_MODAL_SHOT` / `MA_MODAL_SHOT`).

**Give it a trigger.** Neither port had a headless way to reach a modal (`OnBye` needs the system
box; the weather prompt needs a campaign day whose weather says so). BoB's `BOB_TEST_MODAL=<tick>`
fires the real `RMessageBox` from the idle loop and prints its return. *A defect you cannot drive
from a script cannot have a gate.* The gate then asserts the **answer, per button** — three
distinct codes is the only thing that proves a loop actually ran.

---

## §8-BoB167 — the other half of `WM_GETFILE`: a held file block, and why "the message map is broken" was the wrong diagnosis

**BoB S167. The MA counterpart is §8-MA84, whose *mechanism* does not apply — see below.**

BoB's message dispatch (S158) has been default-off since it was written, blocked on a fatal:

```
*** FATAL: Opened file block (6d12) again without closing!
```

S159 traced it properly and established what it was *not*: not `RDialog::OnGetFile`, and not MA's
per-dialog `m_pfileblock` collision (that fix was implemented and measured `borrows: 0`, inert).
The trace ended at *"something on the `bob_fp_repaint` path constructs a `fileblock` directly"*.

**It is the compat layer holding one open.** `bob_dlg_getfile` — the `WM_GETFILE` handler the R*
controls fetch their art through — does:

```c
static fileblock* s_lastfb = NULL;
delete s_lastfb; s_lastfb = new fileblock((FileNum)filenum);   /* held until the NEXT call */
```

and the matching `bob_dlg_releasefile()` **was defined and called by nothing**. The controls do
their part: `WM_RELEASELASTFILE` (`WM_USER+5` = 0x405) is sent from **23 call sites**, and the
compat `SendMessageA` ignored every one. So the block stayed open indefinitely.

That was harmless only while `WM_GETARTWORK` returned 0: with no artnum, `RDialog::DoPaint` never
reached `fileblock picture(artnum)`. Turn the message map on, `OnGetArt` answers for real, and the
second open collides with the one the compat is still holding.

**So the fatal was never the message map's bug.** It was the message map *exposing a
half-implemented protocol underneath* — the "one half of a pair implemented, the other silently
missing" shape BoB's own S156–S163 retro had already named. The fix is one line, and it must not
`return`: the game's own `OnReleaseLastFile` still has to run when dispatch is on.

**Verified with a control arm, which mattered twice here.** The first recipe tried (`mainmenu`,
120 ticks) showed no fatal *with or without* the fix — proving nothing. The reproducer is
`entername` (`BOB_AUTOCLICK=1,1,1`, shot 520): **without the fix exit=1 and the 6d12 fatal;
with it exit=0, zero fatals.**

**But do not flip the default yet — there is a second blocker.** With the fatal gone, an A/B of the
whole gate suite (the suite takes a baseline directory; the previous sessions had not run one)
gives **11/14 byte-identical** and three screens visibly regressed:

| screen | with dispatch on |
|---|---|
| `phaseselect` | phase tabs, date, description paragraph and Back/Begin **all gone**; artwork only |
| `entername` | "Commander Bob", "Luftwaffe Convoys", the date and Back/Begin **all gone** |
| `bobfrag` | differs over most of the screen |

The shape suggests background art now painting where it never did (`OnGetArt` answering for the
first time) and covering text drawn earlier. That is the next investigation, and it is a
*different* bug from the one this note closes.

**The generalisable lesson:** when enabling a subsystem trips a fatal, the fatal is usually not in
the subsystem — it is in something that was never exercised before and has therefore never had to
be correct. Look for the protocol the newly-live code now completes, and check whether the port
implemented both halves of it.

---

## §8-BoB169 — mip-mapping must be split by ALPHA KIND, not applied to every texture (BoB → MA)

**A reverse note: BoB solved this properly first; MA's S153 got it half right and is corrected in
MA S154.**

The engine's terrain and detail textures tile heavily and are viewed at grazing angles, so with
`GL_TEXTURE_MIN_FILTER = GL_LINEAR` and no mip chain they alias and smear at distance. Both ports
have that symptom (MA's Product Owner reported it as *"at distance the filtering does a low pass on
the corners of the leading end of the runway, and it disappears as you get closer"*). The fix is a
mip chain plus `GL_LINEAR_MIPMAP_LINEAR` — **but not for every texture.**

BoB's `upload_texture` already splits three ways, and the reasoning is worth copying exactly:

| texture kind | filter | why |
|---|---|---|
| opaque (terrain, detail tiles) | `GL_LINEAR_MIPMAP_LINEAR` + **anisotropy** | tiled and grazing-angle; isotropic mips alias into stripes |
| **1-bit masked / colour-keyed** (1555, or ckey set) | `GL_NEAREST`, **no mip chain** | LINEAR pulls the keyed mask colour into the alpha edges — a rainbow/magenta fringe |
| smooth alpha (4444, 32-bit) | `GL_LINEAR` | soft sprites (clouds, smoke); their dithered 4-bit alpha under NEAREST showed as a hard white **checkerboard** — a first-pilot report |

MA's S153 turned mipmapping on for **every** texture in both of its upload paths. That is right for
the terrain it was aimed at and wrong for masked art: MA's 8-bit path keys palette index 0 to
alpha 0, so minification averages fully-transparent texels into every sprite edge and leaves a dark
halo. MA S154 corrects it — hard-masked textures (8-bit palette, or 1555) keep plain `GL_LINEAR`
with no chain, exactly their pre-S153 behaviour, and only opaque/smooth-alpha textures get the
chain.

**The generalisable point:** "enable mipmapping" is not one decision. Minification averaging is
only valid where neighbouring texels are meant to be blended, and a colour key or 1-bit mask is
precisely a declaration that they are not. Any port turning on mip-mapping for a 1990s engine
should enumerate its alpha kinds first — and BoB's remaining anisotropy step is still available to
MA when it wants it.

---

## §8-BoB171 — `SetTextAlign` as a no-op clips the map ruler off the edge of the screen

**BoB S171; MA implemented the same thing in its S135 for the same reason. Both ports had the
stub.**

`CDC::SetTextAlign` returning 0 and doing nothing looks harmless — most controls ask for
`TA_LEFT|TA_TOP`, which is the default. But the map's **scale ruler** asks for something else
(`SCALEBAR.CPP`):

```c
pDC->SetTextAlign(TA_RIGHT | TA_BASELINE | TA_NOUPDATECP);
```

and its labels are placed by their **right edge** near the strip's inner border. Discard the flag
and every label is drawn left-aligned from that point, straight off the right-hand side of the
screen: "0 Nm" arrives as "0 N", "50" as "5", "100" as "10". The ruler still *looks* like a ruler —
ticks, spacing and background are all correct — so it reads as a font or margin quirk rather than
a dropped GDI attribute.

**Both alignment bits matter.** `TA_BASELINE` means the y passed in is a baseline, while both
ports' text primitives take a top-left y — so a baseline origin must be raised by roughly the
ascent, or the labels sit a line too low.

**How it was found, and the lesson:** a census of MA findings against BoB (S169) listed
`SetTextAlign` as *"present, but nothing here depends on it yet"* — decided by grepping for callers
and eyeballing the front end. Two sprints later a side-by-side against the **Wine gold capture**
showed the ruler's labels clipped, and the caller was `SCALEBAR.CPP` all along. *A grep tells you
who calls an API; only a comparison against the original tells you whether the result is right.*

Verify against gold, not against your own references: MA discovered (its S143) that four of its
five committed parity references had encoded a defect, because they were captured from the port
itself.

## §8-BoB173 — ⭐ A table parser that SKIPS entries must still COUNT them (BoB S173) **[ENGINE]**

**The bug, in one line.** Resolving a sprite name to its sheet index:

```c
while (fgets(line, sizeof line, f)) {
    const char* p = line; while (*p==' '||*p=='\t') p++;
    if (strncmp(p, "ICON_", 5) != 0) continue;   /* <-- skips, BEFORE the counter */
    ...
    s_icons[s_nicons].val = 0x10000 + (*idx);
    (*idx)++;
}
```

`iconnum.g` is a bare enumerator list — 94 members, of which only 62 begin `ICON_`. The other 32
are `B_ICON_CITY`, `B_ICON_TOWN`, … map markers. They are **real enum members that consume real
values**. Skipping them without counting shifted every icon after the first one down by up to 32,
so `ICON_PAUSE` (true index 79) resolved to sheet page 47.

**Why it survived so long.** The shift starts at index 35. Everything the port had used up to that
point — the whole strategic-map toolbar set (`ICON_THUMB`=0 … `ICON_MISSIONS`=14), `ICON_TICK`=33,
`ICON_CROSS`=34 — is below the boundary and rendered **correctly**. A numbering error that begins
one third of the way into a table does not read as "the resolver is broken". It reads as "some
art is wrong", which invites per-symptom patching: BoB's S94 had already responded by writing a
hand-maintained id→icon reconstruction table, which is a workaround built on top of the real bug
and which quietly encodes the wrong assumption that these faces were *missing* rather than
*mis-addressed*.

**The general rule.** When you parse a positional table — enum members, resource entries, columns,
struct fields — **filtering by name and assigning by position are different jobs.** Filter what you
*record*; never filter what you *count*. If a parser's loop can `continue` before its index
increments, the index is only correct until the first entry the filter rejects.

Check for it by **validating the resolver against a known-far entry**, not a near one. Any test
using an early symbol passes on a broken parser. Take the last symbol in the file, compute its
index by hand, and assert.

**Sister-port check (MA): done, NOT APPLICABLE.** MA has no `iconnum*.g` equate files and no
page resolver — its `GETFILE.CPP` carries only the `F_GRAFIX.G` name→number lookup, which reads an
explicit `NAME =0xHHHH` value per line and so never derives anything from position. Nothing to fix;
recorded here so this is not re-investigated. The *rule* still applies to any future positional
parser in either port.

**Related:** §8-MA97 (art *named* after a control is not necessarily the art *for* it) — that note
warned the name→art mapping lies; this one is the arithmetic underneath it lying too.

## §8-BoB173b — the clipping half of a blit pair, and scaffolds that look like the feature **[ENGINE]**

Two smaller lessons from the same sprint, both of the port's standing "one half of a pair" class.

**1. A panel-aligned blit needs a clip, and on Windows the WINDOW was the clip.** `RBUTTONC.CPP`
draws a control's background by blitting the *parent's* artwork at a negative offset
(`parentrect.left - rect.left`) so one shared image lines up across every control, then relies on
the control's window to clip it to that control's rect. **The source says nothing about clipping,
because Windows did it.** A port with no windows gets the offset right and the clip missing, and
every control paints the whole panel over its neighbours. The host draw path had set the *text*
viewport per control and never the DIB origin/clip — so captions landed correctly and bitmaps did
not, which reads as "the art is wrong" rather than "the art is unclipped".

Adding the clip recovered text on three unrelated front-end screens that had been sitting under
black overpaint for many sprints (`phaseselect`, `entername`, `bobfrag` — tab rows, phase
description, the unit table, the Back/Begin/Fly menu row). None of those had ever been reported as
a clipping problem.

**2. A scaffold that looks like the feature suppresses the report that would have found it.** The
strategic map's clock was a hand-filled rectangle plus a `TextOut` of a string composed with the
*same format the real control uses*. It therefore showed the right date, the right time and the
right accel rate, drawn from the right variables — and it could never grow the four transport
buttons the design puts under it, nor give any control a drawn rect to hit-test. The campaign was
unplayable (the map starts paused by design and waits for a button that could not be clicked) while
every screenshot showed a working clock.

The tell was available the whole time and was a **value, not a picture**: `x0`. Prefer an assertion
on engine state (`MMC.curraccelrate`, `time=`) over "the screen looks right", and **always run the
control arm** — here, the identical recipe with the click removed, which is what turned "the clock
reads x1" into "the clock advanced 220s that it does not advance without the click".

## §8-BoB173c — `pgrep -f` in a wait loop self-matches and waits forever (BoB S173) **[PROCESS]**

Both ports already book *"never `pkill -f <pat>` where `<pat>` matches the test command's own line —
it self-kills the shell"*. Same trap, two more disguises, both hit in one sprint:

1. **A wait loop that never ends.** A chained script began
   `while pgrep -f toggle_test.sh >/dev/null; do sleep 10; done` — wait for the running test to
   release the binary, then rebuild. It waited forever: the script's *own* command line contains the
   string `toggle_test.sh`, so `pgrep -f` matched itself. This fails **silently** — no error, no
   output, just a job that looks like it is still waiting on something legitimate. Worse than the
   self-kill, which at least announces itself.
2. **The self-kill again**, in a command whose only purpose was to clean up after (1):
   `pkill -f toggle_after.sh` from a shell whose command line contained that name. Exit 144.

**Rules.** Match on the *executable*, not the full line: `pgrep -x bob`, `pkill -x bob`. To wait on
a specific job, wait on its **PID** (captured with `$!`) or on the artefact it produces
(`until [ -f out.ppm ]`), never on a name that appears in your own argv. If you must use `-f`, add a
pattern the waiter cannot contain — or just `grep -v $$`.

**And prefer waiting on the artefact anyway.** `until [ -f /tmp/x.ppm ]` cannot self-match, does not
care how the job was launched, and stays correct if the job is restarted by hand.

## §8-BoB173d — ⭐ `GetWindowRect` returning the WHOLE SCREEN made one open dialog eat every click (BoB S173) **[ENGINE]**

The compat had:

```c
void GetWindowRect(LPRECT r) const { if (r) {
    int w=0,h=0; bob_gdi_screen_size(&w,&h); r->left=r->top=0; r->right=w; r->bottom=h; } }
```

Plausible, total, and wrong for every caller that asks *where* a window is rather than *how big the
screen is*. It went unnoticed for many sprints because nothing consequential asked — until S156 added
the rule *"a click landing on an open dialog but hitting no control is swallowed, so the dialog's
background does not select the map behind it"*:

```c
CRect r; d->GetWindowRect(r);
if (cx >= r.left && cx < r.right && cy >= r.top && cy < r.bottom) return 1;   /* swallow */
```

With that stub the test is `cx>=0 && cx<1024 && cy>=0 && cy<768` — **true for every pixel**. From the
moment *any* dialog was open, every click on the strategic map was discarded before reaching the
time controls, either icon row, the event log or unit selection. Measured, not inferred — once the
swallow trace was ungated it printed its own rect: `inside dialog 3/6 (0,0)-(1024,768) -- SWALLOWED`.

**Three lessons, in order of transferability.**

1. **A geometry stub is dangerous in proportion to how reasonable its answer looks.** Returning the
   screen rect is defensible for a full-screen game and produces no error, no warning and no visibly
   wrong pixel. It only shows up as *behaviour*: a UI that goes inert under a condition nobody
   thought to test (here: "with something open"). Grep the compat for functions that answer a
   geometric question with a constant, and ask what would happen if a caller believed them.
2. **Derive hit and swallow regions from the PAINT, never from a window query.** Both ports already book
   *"the click walk must mirror the paint walk"*; this is the same rule for *regions* rather than
   for *controls*. The fix unions the dialog subtree's paint-recorded control rects
   (`bob_ole_drawn_bounds`) over the same nodes the click walk visits, so the swallow region and the
   hit region come from one traversal of one set of rects and cannot drift from what the player sees.
3. **Make the discarding path say so.** The swallow's trace was gated behind `BOB_TRACE_OLE`, which
   is unusable for click questions (per-control-per-frame; it once wrote 70 MB and starved a run past
   its timeout). So the single most consequential thing that dispatch does — *throwing a click away*
   — was the one thing it never reported, and "the click never arrived" was indistinguishable from
   "it arrived and the handler declined". Those are opposite bugs. Per-click traces are cheap:
   print them unconditionally.

**Choose the safe default when the data is missing.** If a dialog drew nothing hit-testable, the fix
does **not** swallow. Letting a click through to the map is recoverable; eating it is not.

**Sister-port check (MA): done, NOT AFFECTED — and MA is the model.** MA's `CWnd::GetWindowRect`
already answers from the control's paint-recorded geometry
(`r->left = m_maX; r->top = m_maY; r->right = m_maX + m_maW; …`). BoB should have copied that when it
copied the surrounding shim.

## §8-BoB173e — ⭐ The D3D→GL viewport origin flip: correct for full-screen, wrong for everything else (BoB S173) **[ENGINE]**

```c
/* wrong */ glViewport(vp->dwX, vp->dwY, vp->dwWidth, vp->dwHeight);
/* right */ glViewport(vp->dwX, targetH - vp->dwY - vp->dwHeight, vp->dwWidth, vp->dwHeight);
```

DirectDraw/Direct3D measure a viewport's Y from the **top** of the render target. OpenGL measures it
from the **bottom**. Passing `dwY` through unchanged is exactly right whenever the viewport covers
the whole target — `H - 0 - H == 0` — and wrong by `H - h` for every smaller one.

**That is what makes it dangerous.** A port spends its first year rendering full-screen, so this
line is correct for everything anyone looks at, and it stays correct until some subsystem renders a
*sub-region* into a target. In BoB that subsystem was the landscape tile compositor: it renders each
terrain tile into a corner of a 256×256 scratch render target with a w×h viewport, then reads the
**top-left** w×h rect back out. GL had put the pixels in the bottom-left, the read-back saw untouched
memory, and the tile was uploaded black. The 256×256 tiles were fine — their viewport covers the
whole target, so both corners are the same region — and *that* asymmetry is the tell: **when a
defect tracks the size of a thing rather than its identity, suspect a coordinate convention.**

**Cost of not spotting it:** the symptom (black patches in terrain, moving with altitude and view
angle, varying between runs) survived nine eliminated mechanisms, a falsified fix, an arithmetic
coincidence that fit perfectly, a confound in the measurement method, an inverted description of the
geometry, and a retraction that had to be reversed. None of that was wasted — each step removed a
real candidate — but the whole chase collapsed the moment the question changed from *what is in the
source* to **where in the source it is**:

```
src corner means (raw565): topLeft=0 bottomLeft=10667   <- names the bug in one line
```

**Generalise it:** when a copy or read-back comes out empty, sample the *other* corner before
theorising about the contents. Origin conventions differ between DirectDraw (top-left), OpenGL
(bottom-left), BMP rows (bottom-up) and this engine's surfaces (top-down), and a port crosses all
four. Three separate places in this compat already flip rows for exactly this reason; the viewport
was the one that did not.

**Sister-port check (MA): SAME CODE, LATENT — action required.** `~/ma/SRC/compat/bob_video.cpp`
`DEV_SetViewport` is byte-identical and unflipped. MA's flight path is the DX5/6 software
rasteriser, so its D3D7 device may never set a sub-viewport and the bug may never fire — but the
line is wrong there too, and the fix is safe by construction (identical output for any full-size
viewport). Apply it with MA's own gate run; do not assume BoB's gate covers it.

## §8-BoB180 — ⭐ Gating a renderer OFF is half a change; something must be gated ON (BoB S180) **[ENGINE]**

**The bug.** The port renders the strategic map from one branch of an idle tick and the front-end /
full-screen pages from the branch below it:

```c
if (g_map_active && onMapPage) { ...paint the map...; return; }
if (!g_activeFullPane) return;                 /* the front-end / full-pane path */
```

`g_map_active` is the port's own flag, set when the map launches. `onMapPage` reads the GAME's page
state. When the game moved to a full-screen page (`LaunchFullPane` → `m_currentpage = 1`) the first
branch stopped matching — and the second was still gated behind it. **Neither painted.** The window
kept its last map frame forever. The clock and every toolbar handler are correctly gated on the same
page state, so they went quiet too, and the result was indistinguishable from a hang. The game was
running normally throughout; the port had simply stopped drawing.

The preceding sprint had added the *suppress* half deliberately, to stop a stale map being drawn
over a page. Its own comment even said *"the flag which LaunchMap sets and NOTHING CLEARS"* — and
the clear was still not written. **Suppressing the wrong renderer and selecting the right one are
two changes; shipping only the first produces a black hole rather than a wrong picture.**

**Rule.** When a mode flag decides *which* subsystem owns the screen, the transition must be a
handoff: the same commit that stops one painting must start the other, and both directions must be
driven by ordinary play. If you can grep your own diff for a "set" with no matching "clear", stop.

## §8-BoB180b — ⭐ A scaffold that compensates for a missing production hook passes BECAUSE of the bug **[PROCESS]**

The seam in §8-BoB180 had a dedicated harness, and it worked for forty sprints. It contained:

```c
g_map_active = 0;                       /* leave map mode -> front-end paint path */
view->LaunchFullPane(&briefing, ...);   /* the production entry */
```

That first line is the hook production code was missing. The harness had **patched around the
defect in order to reach the thing it was testing** — so it exercised the seam successfully while
every real click path through the same seam froze the game. The green test was evidence *for* the
bug's continued existence.

**The tell:** a scaffold that sets engine state *immediately before* calling a public entry point.
The entry point should be setting that state itself; if it doesn't, the scaffold is documenting a
missing line, not preparing a fixture. Delete the compensating line and make the harness fail —
then fix production. Related: §8-BoB173b (scaffolds that look like the feature).

## §8-BoB181 — ⭐ A dialog's art is a SHEET; the window is what clipped it **[ENGINE]**

Rowan dialogs blit a whole background bitmap at an offset and let the window clip it — the player
sees a dialog-sized *window onto a larger shared sheet*. A 272x104-DLU message box draws from a
780x585 image; an OOB dialog draws its own region of a common plate.

A port with no windows draws into one screen-wide DC, so it sets an origin and no clip, and the
whole sheet lands on the screen. Symptoms differ enough to look like unrelated bugs:

- the message box covered most of the screen while its buttons drew correctly centred inside it;
- a campaign dialog on the map painted past the screen edge and over its neighbour;
- controls sharing one panel plate each painted the whole plate over each other (§8-BoB173b).

All three are the same missing clip. **Wherever the port substitutes a screen-wide DC for a
window, it inherits the window's clipping responsibility** — and the clip rect is the template's
own size, which the resource parser already knows (or can, cheaply: the `DIALOG` statement's
`x, y, cx, cy` is usually parsed and thrown away).

**Corollary — a gate pinned to the defect fails when you fix it.** The modal's button coordinates
in the gate suite had been *fitted to the unclipped layout*. That gate could only pass while the
bug survived. Derive test coordinates from the template (control DLU rect → px), and assert the
geometry alongside the behaviour, so a layout change fails loudly instead of as silently-missed
clicks.

## §8-BoB182 — a stub that returns SUCCESS deletes the evidence of its own gap **[ENGINE]**

```c
static inline LONG ChangeDisplaySettings(LPDEVMODE, DWORD) { return 0; /*DISP_CHANGE_SUCCESSFUL*/ }
```

The game's resolution setting was found, validated, applied through this, and discarded. Nothing
anywhere reported a problem, because the stub said it worked. The *enumeration* half had been
implemented a sprint earlier, which made it worse: the search now succeeded, so the failure moved
from "no modes offered" (visible: an empty combo) to "your choice is silently ignored" (invisible).

Generalises §8-BoB158: prefer a stub that returns FAILURE, or one that logs once. **A constant
"success" is the single worst return value for an unimplemented call**, and implementing one half
of an enumerate/apply pair without the other converts a visible gap into a silent one.

## §8-BoB183 — a control that is not in your walk's collection does not exist **[ENGINE]**

The campaign's only exit is a small system box — and it is not a member of the toolbar array every
paint and click walk iterates. So nothing drew it and nothing hit-tested it: there was no X, and no
way out of a campaign short of killing the window. It had been missing for the whole life of the
map screen without ever producing an error.

**Rule.** Enumerate owners from the window/dialog tree, not from the array you happen to have a
loop over. When you do add a member by hand, add it to the paint walk and the click walk *in the
same edit* — this port has now shipped the paint-only half three times (§8-BoB173d).

## §8-BoB185 — ⭐ Deriving font size from the CONTROL BOX truncates the game's own captions **[ENGINE]**

The port sized each control's text from its own box (`height - 4`) rather than from the font the
game selected, on the stated reasoning that the real fonts were "tiny in our enlarged boxes". That
was never measured. Measured, it is backwards for dialogs drawn at native DLU scale: a 230x25-DLU
static asked for a **36px** font where the game had selected **14**, and its sentence rendered at
twice the control's width.

The decisive evidence was not the overflow but a **disappearing ellipsis**: the R* controls'
own Shrink/GetTextExtent logic had been *truncating captions* because the font handed to it was too
big. Fixing the size made truncated captions read in full. **When a widget's own fit/shrink logic
is trimming content, suspect the metric you gave it before suspecting the widget.**

**Do not adopt blindly, though.** The same measurement showed a 48px ART-face font being selected
into a 26px box elsewhere, so "use `m_height`" would have broken screens that work. The safe rule
is **shrink-only** — adopt the game's font when it is smaller than the box-derived value, keep the
old value when larger. It fixes every overflow (text that fits its own control cannot paint over a
neighbour) and declines the one direction the evidence does not support.

**Instrument before you change a global.** One env-gated line printing
`(dialog, control, box-derived, real-font)` turned a risky rewrite into a two-line rule with a
table behind it — and showed which screens would move before any of them did.

## §8-MA104 — ⭐ Two flags for one fact: BoB's freeze, and why MA never had it **[ENGINE]**

§8-BoB180 froze BoB whenever the game moved to a full-screen page: the port's idle tick chose its
renderer from a **port-owned** `g_map_active` flag ANDed with the **game's** page state, and when
those two disagreed neither branch ran.

MA's equivalent dispatch cannot express that bug:

```c
RFullPanelDial* fp = GetFullPanel(view);
if (fp)                              { ...paint the full pane... }
else if (view->m_currentpage == 0)   { ...paint the map... }
```

It branches on **the object that actually exists**, with the map as the fallback. There is no second
variable to fall out of step, so there is no state in which nothing paints. BoB kept a parallel
boolean saying the same thing the game already knew, and the freeze was the two copies diverging.

**Rule.** Derive "which subsystem owns the screen" from the game's own objects/state, never from a
port-side mirror of it. If a mirror already exists, the fix is not only to keep it in sync at every
transition (BoB S180) but to plan its removal — a flag that must be cleared in N places will be
missed in one. Cross-port: MA is the reference design here; BoB should converge on it.

## §8-MA105 — do not cross-port a rendering change you cannot measure on the target **[PROCESS]**

BoB S173v flips the D3D→GL viewport ORIGIN (§8-BoB173e), and MA's `DEV_SetViewport` is the same
function, unflipped. Applying it looks like a one-line cross-port.

It was not applied, on purpose. The flip is **inert for a full-screen viewport** and only matters if
the game sets a sub-viewport — and MA's front end needs real mouse clicks to reach 3D, so the
measurement that would answer "does MA ever set one?" could not be taken in this session. Shipping
the flip blind would have risked a working renderer to fix a defect not shown to exist there.

What was landed instead: `MA_TRACE_VIEWPORT=1`, printing one line per distinct viewport rect and
labelling it `[full-height: flip inert]` or `[SUB-VIEWPORT: flip MATTERS]`. The next session that
reaches 3D on MA answers the question in one run. **A cross-port note is a hypothesis about the
other codebase, not a patch for it** — carry the instrument across first when the check is cheap and
the change is not.

## §8-MA106 — the box-derived font bug is BoB-only; MA's GDI object model already prevents it **[ENGINE]**

§8-BoB185 (text sized from the control's box rather than the selected font, truncating the game's
own captions) does not exist in MA. BoB's `CDC::SelectObject(CFont*)` keeps only a face code and
discards `m_height`, leaving each drawing site to invent a size. MA's passes the font through to a
real GDI object model:

```c
CFont* SelectObject(CFont* f) { if (f) ma_gdi_set_font((void*)m_hDC, (void*)f->m_hObject); return NULL; }
```

so the size the game chose is the size that gets drawn, everywhere, with no per-site heuristic to be
wrong. **When one port has a class of bug the other cannot express, the difference is usually a
missing abstraction rather than a missing fix** — BoB's real remedy is a DC that carries the font,
not a better rule for guessing one.

## §8-MA107 — ⭐ A note that is SYNCED is not a note that is PROCESSED **[PROCESS]**

MA spent Sprint 159 discovering that its campaign dialogs blit their background bitmap at the
bitmap's own size, with no window to clip it, so a 540×602 backdrop landed in a 327×316 dialog and
hung a 286 px skirt over the map. Nine of nine campaign dialogs were affected.

**§8-BoB181 describes that bug exactly, and it was already in MA's copy of this file.**

> *"A port with no windows draws into one screen-wide DC, so it sets an origin and no clip, and the
> whole sheet lands on the screen … wherever the port substitutes a screen-wide DC for a window, it
> inherits the window's clipping responsibility."*

The sync is not the failure — the file was byte-identical in both trees, and the guard proves it
every sprint. The failure is that **syncing was treated as processing.** A note arrives, the file
matches, the checkbox is ticked; nobody asks *"does this one describe something in MY tree?"*
Between S157 and S159, three BoB notes (181, 182, 183) sat in MA's tree unanswered while MA
rediscovered one of them from a play-test defect.

**What it cost:** a sprint. Not wasted — S159's fix is measured and gated, and it found nine
dialogs where the PO reported one — but it is a sprint that a fifteen-minute read would have
started with the answer.

**Fix, and it is structural rather than a resolution to try harder:** the shared doc now carries a
**ledger** (`§8-LEDGER`) with one row per inbound note and an explicit verdict — *applied / N/A with
the reason / open*. A note with no row is unprocessed by definition, and "we synced it" cannot fill
the row in. Both ports keep their own column, because a note is N/A for one and live for the other
far more often than not (§8-MA104, §8-MA106, and §8-MA110 below are all that shape).

**The generalisable half:** *an inbound artefact needs a per-item verdict, not a per-batch one.*
This is the same shape as MA S83's "a search that finds nothing is only as trustworthy as its
pattern" — a batch-level "done" hides every item-level miss inside it.

## §8-MA108 — ⭐ Two constructors, one fix: the sim thread that ran before the world existed **[ENGINE]**

`Inst3d::Inst3d()` and `Inst3d::Inst3d(bool)` sit 100 lines apart in `STUB3D.CPP` and do the same
job for two entry points — a flight, and the **map view** the target dossier's *Photo* button opens
for a 3D recon. Both start the sim thread and only then initialise the members that thread reads:

```c
movethread=AfxBeginThread(moveloop,this,THREAD_PRIORITY_ABOVE_NORMAL,50000,0);
mapview=flag;  …  Master_3d.currinst=this;   // "at this point the thread starts receiving timer messages"
world=new WorldStuff;  viewedwin=NULL;  livelist=NULL;  …  Three_Dee.InitialiseCache();
```

The game's own comment — nine lines *below* the thread start — claims the worker starts there. It
does not; it started already. MA's S69 found this (an AppImage's squashfs slowed the ctor's file I/O
just enough for the worker to win the race that a local filesystem always lost) and moved the
`AfxBeginThread` to the end of the constructor. **It fixed one of the two.** The map-view twin kept
racing for another 90 sprints, until S160 drove the Photo button and the game stopped dead:

```
Thread 11 "wmig" received signal SIGSEGV     #0 Inst3d::moveloop(void*)
Thread 1:  #3 CRectangularCache::CRectangularCache  #5 ThreeDee::InitialiseCache
           #6 Inst3d::Inst3d(bool)  #7 Rtestsh1::Launch3d(bool)
```

The worker had already crashed while the main thread was **still inside the constructor**, building
the landscape cache the worker reads.

**Rule: when a fix is a REORDERING INSIDE A CONSTRUCTOR, find the constructor's twins before closing
it.** Overloads of one class are the highest-risk case in this engine — they are copy-edited from
each other, they are rarely adjacent, and a diff of the file shows the fix applied "in `Inst3d`",
which reads as done. (The dead-code line above both starts is the tell: the original used
`CREATE_SUSPENDED` and had no race at all, so the regression was introduced in both at once.)

**Second half, for whoever meets this in BoB:** BoB's `Inst3d` ctors do not start a move thread —
its generation of the engine starts `drawloop` elsewhere (`STUB3D.CPP:912`) — so **this specific bug
is MA-only**. The *rule* is not.

**Technique worth stealing.** `ptrace_scope=1` refuses `gdb -p` on anything that is not a
descendant, which is most of what a test harness starts. Run the program **under** gdb and let a
timeout interrupt it:

```sh
timeout -s INT 240 gdb -batch -ex "set pagination off" -ex run \
    -ex "thread apply all bt 18" -ex kill --args ./wmig
```

`run` blocks; the SIGINT stops the inferior; `-batch` then executes the next `-ex`. One command,
every thread's stack, no ptrace permissions and no core file. It turned "the game hangs" into the
answer in a single run — after two runs of `ps` had established only that the process was sleeping
rather than spinning, which was true and useless.

## §8-MA109 — measure something the RENDERER CAN PRODUCE **[PROCESS]**

The new gate for the recon view asserted "this frame is a rendered scene, not a flat fill" as
**more than 2000 distinct colours** — and failed a perfectly good frame. MA's software rasterizer is
**8-bit palettised**: it cannot produce more than 256 colours, ever, so the threshold was
unsatisfiable by construction and the gate was testing the renderer's colour depth, not its output.

The test now asks for **≥64 distinct colours with no single colour covering ≥70 %** of the frame.
The recon frame measures 193 / 31.8 %; a black or flat frame is 1–2 colours at ~100 %.

Same family as MA S64's *"never judge SIZE or DENSITY across a gold↔native boundary"* and BoB's
own §8-BoB185 (*a metric handed to a widget decided what the widget did*): **before choosing a
threshold, ask what range the thing under test can occupy.** A threshold outside that range fails
closed and looks like a real defect.

## §8-MA110 — MA's verdicts on BoB notes 182 and 183 **[ENGINE]**

**§8-BoB182 (a stub returning SUCCESS) — N/A in MA, by the opposite asymmetry.** MA has the identical
stub, character for character:

```c
static inline LONG ChangeDisplaySettings(LPDEVMODE, DWORD) { return 0; /*DISP_CHANGE_SUCCESSFUL*/ }
```

BoB was bitten because it had implemented the **enumerate** half and not the **apply** half, so the
search succeeded and the result was silently thrown away. MA has implemented **neither** —
`EnumDisplaySettings` returns `FALSE`, so `Win3d.cpp`'s mode-search loop finds nothing, `if (f)` is
false, and the `CDS_FULLSCREEN` call is never reached. Only the restore call runs, and it is a no-op
by intent.

And here the no-op is **correct rather than missing**: the sole caller switches the *desktop* to
640×480 for a 1999 full-screen game, and this port owns its own window and resolution
(`MA_FORCE_RES`, S122/S127). What the stub owed its reader was to *say* it was declining, which is
all MA changed (`MA_TRACE_STUB=1`). **BoB's rule survives the N/A verdict:** "success" was still the
wrong thing to say silently, even when success is the right outcome.

**§8-BoB183 (a control outside the walk's collection does not exist) — N/A in MA, already closed.**
The same defect existed here and was reported by the PO as *"there is no way out of the campaign
map"* (PO-1, closed S97). MA's paint walk (`ma_map_paint_oob`) and click walk (`ma_map_click_oob`)
enumerate **the same two toolbars** — `m_toolbar2` and `m_toolbar5` — and S106 added the second to
both in one edit, which is BoB's stated rule. The system box is driven by the `port/sysbox_exit.sh`
gate: it opens the confirmation, locates "Yes" from the control's own metrics, clicks it, and
requires 99 % of the map area to change. **BoB's "add it to the paint walk and the click walk in the
same edit" is the right rule and MA can confirm it holds in practice** — the one time this port
shipped the paint-only half (S106's MISSION RESULTS panel), the symptom was identical.

---

## §8-LEDGER — inbound-note verdicts (added MA S161)

**Why this exists.** §8-MA107: three BoB notes sat in MA's byte-identical copy of this file
unprocessed while MA rediscovered one of them (§8-BoB181) from a play-test defect, at the cost of a
sprint. Syncing was being mistaken for processing. **A note with no row here is unprocessed, and
"we synced it" cannot fill the row in.**

**How to use it.** When you send a note, add its row with your own column filled and the sibling's
blank. When you receive one, fill your column with **applied** (and the sprint), **N/A** (and the
reason — one line, checked, not assumed), or **open** (and what is blocking). A verdict of N/A is a
result: §8-MA104, §8-MA106 and §8-MA110 are all "not affected, and here is the structural reason
why", which is the most useful thing either port has sent the other.

**Never fill a row from memory.** MA S83's rule applies to this table too: verify per file, in both
directions — BoB's warning about diverged case-variant twins was true in BoB's tree and false in
MA's, and MA's later S94 correction found the opposite again in a different file.

| Note | Subject | MA verdict | BoB verdict |
|---|---|---|---|
| §8-BoB167 | held file block behind `WM_GETFILE` | applied (S154 era) | origin |
| §8-BoB169 | mip-map by alpha kind | **applied S154** (reverse cross-port) | origin |
| §8-BoB171 | `SetTextAlign` no-op clips the ruler | applied (S135 measured its own) | origin |
| §8-BoB173 | a parser that skips entries must still count them | *not yet assessed* | origin |
| §8-BoB173b | the clipping half of a blit pair | *superseded for MA by §8-BoB181 → S159* | origin |
| §8-BoB173c | `pgrep -f` self-matches in a wait loop | **N/A — already MA's own rule** (memory `pkill -f self-match`; MA gates use `pkill -x`) | origin |
| §8-BoB173d | `GetWindowRect` returning the whole screen | *not yet assessed* | origin |
| §8-BoB173e | D3D→GL viewport origin flip | **open, deliberately** (S157: inert for a full-screen viewport; `MA_TRACE_VIEWPORT` lands the measurement, needs one run that reaches 3D) | origin |
| §8-BoB180 | gating a renderer off is half a change | **N/A S157** (§8-MA104: MA's idle branches on the object that exists) | origin |
| §8-BoB180b | a scaffold that compensates for a missing hook | *not yet assessed* | origin |
| §8-BoB181 | ⭐ a dialog's art is a sheet; the window clipped it | **LIVE — applied S159** (PO-49; nine of nine campaign dialogs). ⚠ Rediscovered independently; see §8-MA107 | origin |
| §8-BoB182 | a stub returning SUCCESS hides its own gap | **N/A S161** (§8-MA110: MA implemented *neither* half, and declining is correct here; stub now says so) | origin |
| §8-BoB183 | a control outside the walk's collection | ⚠ **CORRECTED S164 — partially N/A.** Control level: closed (PO-1/S97). **DIALOG level: LIVE** — the OOB walk enumerates 3 of the 5 dialogs on screen, so a click on the wave folder fires the toolbar button underneath (PO-50). S161's "N/A" was answered at the wrong granularity; see §8-MA112 | origin |
| §8-BoB185 | font sized from the control box | **N/A S157** (§8-MA106: MA's `CDC` passes the real `CFont` through) | origin |
| §8-MA104 | two flags for one fact | origin | **applied S180** — the freeze MA diagnosed; the idle now branches on the object that exists, as MA's does |
| §8-MA105 | do not cross-port what you cannot measure | origin | **adopted (process)** — S192–S197 each measured in this tree before changing it; §8-BoB194 is the clearest case (the fix shipped anyway, but the *claim* was limited to what the count proved) |
| §8-MA106 | box-derived font bug is BoB-only | origin | **applied S185** — shrink-only adoption of the game's own font; MA's diagnosis was correct |
| §8-MA107 | ⭐ synced ≠ processed; this ledger | origin | **adopted** — this column is the adoption. S194/S196/S197 are all ledger-driven, and answering MA111 (which looked N/A) found two live bugs |
| §8-MA108 | ⭐ two constructors, one fix (`Inst3d` race) + gdb under `ptrace_scope=1` | origin (applied S160) | **N/A, measured S198** — `STUB3D.CPP` has exactly **one** `AfxBeginThread` (line 912, `drawloop`) and neither `Inst3d` ctor starts a worker, so the twin-ctor race cannot exist here. The **rule** is adopted: when a fix is a reordering inside a constructor, find its twins. The gdb-under-`ptrace_scope=1` recipe is adopted outright |
| §8-MA109 | measure what the renderer can produce | origin | **adopted (process)** — the campaign gate asserts on emitted log evidence, not on drawn text; two draft assertions that would have failed a working campaign were caught by exactly this rule (S195) |
| §8-MA110 | MA's verdicts on 182 / 183 | origin | n/a (reply) |
| §8-MA111 | ⭐ a control type missing from the click walk (combos, 3rd time) — **and a question** | origin (applied S163) | **answered BoB S196 (§8-BoB196)** — MA's construct cannot exist here (type-agnostic walk), but the QUESTION found `RSPINBUT` drawn-and-inert **and** MA's S166 listbox clamp live on BoB's click path |
| §8-MA112 | ⭐ §8-BoB183 at DIALOG granularity; and "N/A" needs its scope stated | ⚠ origin — **facts corrected by §8-MA113**; conclusion stands | **N/A, measured** — `bob_map_click_oob` enumerates `{TB_REPORT, TB_MISC, TB_MAIN}`, the paint walk's set in reverse (topmost-first), and says so in the code (S187–S188). Same collection, deliberate order |
| §8-MA113 | ⚠ correction to MA112: a summary NUMBER cannot answer a SET question; and "filter, don't cap" (3rd) | origin (PO-50 closed S165) | **adopted, and immediately needed** — S193 read `instance == 0` as "never launched" when it also means "already finished"; the fix was to print the state that distinguishes them. Same shape one level down |
| §8-MA114 | ⭐ `--allow-multiple-definition` + a `__LINE__`-named registrar deleted four eventsink maps — **check your tree, two commands** | origin (fixed S168) | **N/A at runtime, BoB S194** — collision real (9 objects share `BobEvtAuto_0C1Ev`), link flag present, but the registry is **81 classes before and after**, so nothing was lost. Keyed on the class anyway; see §8-BoB194 |
| §8-BoB194 | answering MA114: the before/after count, not the symbol table, settles a "do you have this too?" | **adopted (process), MA S170** — the rule was applied to this sprint's own first result: the spinner "not moving" was read as a broken host until the LIST COUNT (2 entries, index at maximum) showed the control refusing correctly. A delivered click is not a working control. | origin (fixed S194) |
| §8-BoB196 | ⭐ MA's S166 listbox clamp is LIVE in BoB on the real click path; and RSPINBUT drawn-and-inert | **half N/A, half worse — MA S170.** MA's spin type was not drawn-and-inert, it was **never hosted at all**: no CLSID branch, so every `InvokeHelper` was a silent no-op. Hosted in S170. The clamp half stands as fixed in S166. | origin (fixed S196) |
| §8-BoB197 | ⭐ a shared accessor on a per-window object lies to every control (`GetClientRect`) — **and MA hosts no spin type at all, which its own EPIC K step 8 needs** | **applied S170.** Accessor half stays N/A (MA's `CWnd::GetClientRect` already reads the per-window `m_maW/m_maH`, re-checked). Spin half **fixed**: `ma_olespin.cpp` hosts RSpinBut, and EPIC K step 8 now runs end to end (Mission Folder Flights 2→3). BoB's warning that a spinner at its limit refuses correctly is what stopped MA publishing a wrong cause. | origin (fixed S197) |
| §8-MA115 | ⭐ a recipe that addresses a ROW addresses a CELL — and picks the middle column | origin (fixed S170) | **CONFIRMED IN MIRROR IMAGE, BoB S199.** BoB could name a COLUMN and not a ROW: `bob_ole_ctrl_point` always returned `sy + sh/2`, so any recipe naming a multi-row list clicked its middle row. `#ID:COL.ROW` added, resolved through the control's own `rowAtY`; an unmapped row REFUSES rather than falling back to the centre. Measured: `#1000:0` → y=36 (centre), `#1000:0.0` → y=24 (row 0) — different points, so that control has rows and every prior recipe landed on whichever sat at the centre. |
| §8-MA116 | the click-type filter is an allowlist — four silent failures now (S87/S140/S163/S170) | origin (S170) | *awaiting — answered in advance by §8-BoB196 (type-agnostic walk); confirm* |
| §8-MA117 | ⭐ a teardown destroys a SUBTREE; the registry lost exactly one window (two live copies of one dialog) | origin (fixed S171) | **LATENT, DETECTOR ADDED, BoB S199.** BoB's `hosts()` is never erased BY DESIGN (draw/click filter by `parentDlg`), which is safe until the allocator RECYCLES a dead dialog's address — then two hosts match one `(dlg,id)` and `find_wrapper` returns by hash order. `bob_ole_find_wrapper` now counts matches and warns. **No collision observed** across the full suite — but none of those recipes closes and reopens a dialog, so this is *not observed*, not *cannot happen*. |
| §8-MA118 | ⭐ a gate reported PASS on a run that SEGFAULTED — assert on how the run ENDED | origin (fixed S171) | **WORSE HERE, FIXED BoB S199.** `bob_gates.sh` PRINTED `exit=N` for all 14 GATE-1 recipes and **nothing ever read them**; stderr went to `/dev/null` so the crash banner was discarded too. GATES 2/3/4 the same. One `checkrun` helper now checks banner + exit code for every run and the script ends with a `### RUNS:` verdict that sets its exit status. |
| §8-MA119 | the same engine file a year apart: BoB already carries the empty-list fix MA lacks (`RDH 29/10/99`) | origin (fixed S171, host-side) | **N/A for BoB — already fixed in BoB's game source**; the transferable half is *read the other port's copy before theorising* |
| §8-MA120 | ⭐ a workaround's comment records the hazard as it was THAT DAY — re-check before designing around it | origin (S172) | *awaiting — both trees are full of dated avoidance notes* |
| §8-MA121 | ⭐ a trace is code; prefer oracles that can be IMPOSSIBLE, not merely wrong | origin (fixed S172) | **adopted as a rule, BoB S199** — no instance found needing the fix this sprint; recorded so any future trace reading through a "currently selected / last hit" member is written against it. |
| §8-MA122 | ⭐ a dialog can be ambiguous with ITSELF by design (N identical sub-dialogs), not only after a reopen | origin (fixed S173) | *awaiting — count hosts per id on a repeated OOB panel* |
| §8-MA123 | a written walkthrough step does not say which WIDGET the game uses — look it up before writing the criterion | origin (S173) | *awaiting — process note, applies to any gold-video-derived story* |
| §8-MA124 | ⭐ a synthetic DRIVER is code, and it fails in the SHAPE of the bug you are hunting | origin (S174) | *awaiting — BOB_AUTOCLICK / BOB_KEYSEQ / BOB_AUTOFLY are all this kind of driver* |
| §8-MA125 | a drive recipe keyed to a FRAME COUNT breaks the first time the path in front of it grows | origin (fixed S174) | *awaiting — key drives to a STATE (BoB has `InThe3D`, `g_bobActiveFP`)* |

**Rows marked *not yet assessed* are MA's own debt** and are named rather than quietly omitted —
that is the whole point of the table. They are the top of MA's next cross-port slot.

## §8-MA111 — ⭐ A control type missing from the click walk is drawn, inert, and invisible for years — do you have this too? **[ENGINE]**

MA's OOB/toolbar click walk filters by control type:

```c
if (h.type != CT_BUTTON && h.type != CT_TABS && h.type != CT_LISTBOX &&
    h.type != CT_RADIO && h.type != CT_SCROLL) continue;
```

**`CT_COMBO` was not in that list**, so every combo box in every campaign-map dialog was **drawn,
correct, populated — and inert** for the port's whole life. The tell is one trace line:

```
[oobclick] swallowed (1737,77) inside dialog rect
```

i.e. no control wanted the point and the dialog-rect catch-all consumed it. Note how benign that
looks: the dialog is open, the widget is on screen with the right value in it, and clicking does
nothing. It reads as "that combo doesn't do anything in this game", not as a missing route.

**This is now the THIRD control type found the same way in this port** — MA S87 (listbox rows),
S140 (`RScrlBar`), S163 (combos) — and each was found only when a specific feature needed it, years
apart. **The list is a denylist wearing an allowlist's clothes: every type nobody has needed yet is
silently excluded.** Whoever reads this next: go and print the set of hosted control types your
click walk actually accepts, next to the set your DRAW walk paints. The difference is your bug list.
MA's remaining gap after S163 is `CT_EDIT`/`CT_EDTBT` on this path, which is now *named* rather than
waiting to be discovered by a feature.

**A combo is three things, not one click**, and getting two of them right leaves it broken:
1. **click** — open the dropdown (or cycle, for a ≤1-item combo);
2. **draw** — paint the open list **after the whole dialog tree**, not inside the per-dialog pass:
   drawn per dialog it is painted over by the next dialog in the walk;
3. **dismiss** — the open list gets **first refusal** on the next click and consumes it either way.
   Windows does not pass the dismissing click through to what is behind an open combo.

The draw and the dismiss are mirror images; if they disagree, the list is either invisible or
un-closable. Share the row arithmetic with whatever path already had dropdowns working — two
implementations of "which row is under the cursor" drift.

**❓ Question for BoB:** your OCX hosting is organised differently (`bob_eventsink.cpp` rather than a
single control router), so the literal filter may not exist — but the *shape* might: **is there a
control type your dialogs DRAW that your click routing never offers a point to?** MA's answer was
"yes, three times". If the answer here is no, say so and say what makes it structurally impossible —
that answer is worth as much as a fix (cf. §8-MA104, §8-MA106).

## §8-MA112 — ⭐ §8-BoB183 again, with "control" replaced by "dialog" — MA's S161 verdict was too narrow **[ENGINE]**

In S161 MA answered §8-BoB183 (*a control that is not in your walk's collection does not exist*)
with **"N/A, already closed"**, on the strength of the *control* case: MA's paint walk and click walk
enumerate the same two toolbars, and the missing system box was fixed as PO-1.

S164 measured the same screen one level up:

```
[oob] painted 3 open dialog(s)      <- what the OOB walk enumerates, at its peak
five dialogs are actually drawn     <- (811,426) (1575,0) (0,867) (200,24) (724,406)
```

**Five dialogs on screen, three in the collection.** The campaign wave folder is one of the two
outside it, it is painted at (200,24) directly over the main toolbar row, and a click on its wave
list therefore falls through and fires `IDC_OVERVIEW` underneath — *clicking a row of the mission you
are editing opens an unrelated dialog.*

So the honest correction: **BoB's note was live in MA, at the granularity above the one MA checked.**
The verdict "N/A" was not wrong about controls; it was answered at the wrong altitude.

**Two rules out of this, and the second is the one that generalises:**

1. **A paint pass and a click pass must not just share their offsets — they must share their
   COLLECTION.** MA's mirror the offsets faithfully (S82) and still disagree about which dialogs
   exist. The cheapest possible check is a counter on each: *print how many things each walk visited,
   in the same frame.* Three versus five took one line of trace to see and had been true for the
   port's whole life.
2. **When answering an inbound note "N/A", state the granularity you checked at.** §8-LEDGER (S161)
   gives every note a verdict; this one shows a verdict can be confidently wrong by being narrow.
   "N/A for controls; dialogs not checked" would have been the true answer, and it would have found
   this a sprint earlier. **A verdict without its scope is a claim about everything.**

MA's row in the ledger is corrected to *"partially N/A — control-level closed (PO-1/S97),
DIALOG-level LIVE (PO-50, S164)"*.

## §8-MA113 — ⚠ Correction to §8-MA112, and the mistake that produced it **[PROCESS]**

**§8-MA112 is wrong on its facts and right on its conclusion.** It reported that MA's OOB walk
*"paints 3 dialogs while five are drawn"*, and offered that as evidence that a paint pass and a click
pass can disagree about which dialogs exist. The disagreement is real. The number was not: `[oob]
painted N` is a **per-frame counter of top-level children painted in that pass**, and reading it as
"the dialogs on screen" produced a specific, confident, incorrect claim that went into a sprint
record, a status page and this file before anyone checked it.

**What is actually true (MA S165):** the paint walk descends **two levels of logged children** — a
dialog may be logged on another dialog, and MA's campaign wave folder is a child of the Mission
Folder, not of the toolbar. The click walk descended **one**. So those dialogs were painted and no
click could ever be offered to them; on the campaign map they sit over the main toolbar, and clicking
a row of the mission being edited fired the toolbar's Overview button underneath.

**The mistake worth carrying, because it is not about counters:**

> **A summary NUMBER was used to infer a SET DIFFERENCE.**

The question was *"does the click walk see the same dialogs as the paint walk?"* — a set question.
It was answered with two integers that happened to differ, and the difference was read as the answer.
The correct instrument took two lines: print **one line per node each walk visits, deduped by node**,
and diff the two lists. `[oobrender]` versus `[oobvisit]` named the missing dialog immediately, along
with its rect and its depth.

So, for both ports: **when you want to know whether two passes agree about a collection, emit the
collections, not their sizes.** A count tells you *that* they differ; only the sets tell you *which*
and *why*, and a count can differ for reasons that have nothing to do with your question.

**And the reason the sets were not available (third booking):** MA's node trace was budgeted
`if (_r++<40)`, so the entire budget went to the first dialog tree walked and a dialog opened later
never appeared in the log at all. That is **"filter, don't cap"** (§8-MA83, MA S64→S67) for the third
time in this project. It is now dedup-by-node: every distinct node prints exactly once, ever —
bounded by the number of nodes rather than by frames, so a late arrival cannot be starved out.

**Ledger discipline note.** §8-MA112's own lesson was *"when answering N/A, state the granularity you
checked at"*. This correction adds the sibling: **state the instrument.** "Measured" is not a
provenance; *"measured with a per-frame counter"* would have been, and would have invited the
question that took one sprint to ask.

## §8-MA114 — ⭐ `-Wl,--allow-multiple-definition` silently deleted four eventsink maps **[ENGINE]**

MA's compat `BEGIN_EVENTSINK_MAP` built its auto-registrar like this:

```c
#define BEGIN_EVENTSINK_MAP(theClass, baseClass) \
    static struct MaEvtAuto_##__LINE__ { MaEvtAuto_##__LINE__(); } g_maEvtAuto_##__LINE__; \
    MaEvtAuto_##__LINE__::MaEvtAuto_##__LINE__() { theClass::MaRegEvents(); } \
    void theClass::MaRegEvents() {
```

The object is `static`, but **the constructor is defined out of line, so its symbol has EXTERNAL
linkage**. Two translation units whose `BEGIN_EVENTSINK_MAP` happens to sit on the **same line
number** therefore emit the same constructor symbol. Both ports link with
`-Wl,--allow-multiple-definition` — so the linker keeps the first, discards the second, and says
nothing.

The consequence is not subtle and it is completely invisible: **the losing class's entire eventsink
map never registers.** Every button on that dialog draws, highlights, toggles its pressed artwork —
and does nothing. The winning class registers **twice**.

MA measured it before fixing: **68 TUs carry a sink map, and four pairs collide.**

| line | classes | what it cost MA |
|---|---|---|
| 126 | `SQDNLBUT` / `WPBUT` | the waypoint buttons |
| 130 | `LISTBX` / `WAVETABS` | the campaign wave tabs |
| 159 | `MAPFLTRS` / `MISSFLDR` | the Mission Folder's Intelligence / Profile / Delete / **Frag** |
| 162 | `SERVICE` / `SESSION` | |

One macro fault, four dead dialogs, for the port's whole life.

**❓ For BoB — checked from MA's side, and the answer needs your eyes, not mine.** BoB's macro does
not use `__LINE__`; it uses **`__COUNTER__`**:

```c
#define BEGIN_EVENTSINK_MAP(theClass, baseClass) BOB_EVTSINK_IMPL(theClass, __COUNTER__)
#define BOB_EVTSINK_IMPL(theClass, ctr) \
    static struct BobEvtAuto_##ctr { BobEvtAuto_##ctr(); } g_bobEvtAuto_##ctr; \
    BobEvtAuto_##ctr::BobEvtAuto_##ctr() { theClass::MaRegEvents(); } \
    void theClass::MaRegEvents() {
```

**`__COUNTER__` has exactly the same property that broke MA: it is unique within a translation unit
and restarts at zero in the next one.** The constructor is still defined out of line, so its symbol
is still external. On the face of it that is a *worse* key than `__LINE__` — line numbers at least
spread out, whereas every TU's first sink map wants to be `BobEvtAuto_0`.

**BoB plainly works, so something must be preventing the collision** — a different link flag, an
anonymous namespace, a header that advances the counter unevenly, or a build layout where these TUs
never meet. **That "something" is worth knowing deliberately rather than by luck**, because it is the
only thing standing between BoB and four-dialogs-worth of silently dead buttons. MA is not going to
guess at it from outside; the question is: *what makes `BobEvtAuto_0` unique in your link?*

⚠ **A caveat on any listing you generate:** a naive scan of BoB's tree reports ~60 "collisions", and
**most are case-variant twins of one file** (`RDEMPTYP.CPP` / `RDEmptyP.cpp`), which are the same
translation unit and not a collision at all. Resolve twins first (§8-MA94's rule: check per file, in
both directions) or the count is meaningless. The pairs that look like genuinely different classes
include `MSCTLBR`/`TELETYPE`, `LOAD`/`LOCKER`/`MAINTBAR`, `SIDESEL`/`SYSBOX`, `SERVICE`/`SESSION`
and `RAFTASKS`/`SUPPLY`.

**Whatever the answer, the safe key is the same one MA now uses: the class name**, with the ctor
defined in-class. It cannot collide, because a class has exactly one sink map.

**The fix, and why it is the key rather than the collision:** name the registrar after the **class**
— a class has exactly one sink map, so the class name is the correct unique key — and define the
constructor **inside** the struct so it never reaches the external symbol table at all:

```c
#define BEGIN_EVENTSINK_MAP(theClass, baseClass) \
    static struct MaEvtAuto_##theClass { MaEvtAuto_##theClass() { theClass::MaRegEvents(); } } \
        g_maEvtAuto_##theClass; \
    void theClass::MaRegEvents() {
```

**The generalisable rule: `--allow-multiple-definition` converts an ODR violation from a link error
into a silent behavioural bug.** Both ports carry that flag to get past duplicated symbols in a
1999 codebase, and it has been quietly paying for itself in ways nobody was measuring. Any macro
that synthesises a symbol name from `__LINE__`, `__COUNTER__` or anything else that is not unique
*across the program* is a live instance. Audit them; there are not many.

**And the diagnostic route is worth copying, because it is repeatable.** Three steps, no guessing:
1. make an **unmatched dispatch report itself** — a "firing" trace printed *before* the dispatch
   reads exactly like success, which is why this survived so long;
2. add a **filtered** (never capped) registration trace, and run it against a class you know works
   as a control;
3. when the symbol exists but the code never runs, `objdump -d` the TU's `_GLOBAL__sub_I…` and read
   who it actually calls. It named the wrong callee in one line.

## §8-BoB194 — answering §8-MA114: same shape, no bug — and the count that proves it **[ENGINE]**

MA asked whether this port has the eventsink-registrar collision that cost it four dialogs. Measured
here, in this order, before changing anything:

1. **The collision is real and widespread.** `BobEvtAuto_0C1Ev` is defined in **nine** objects
   (`_LW`, `_SA`, `_FULL`, `_TOOL`, `_MFC`, `_RAF`, `BOBFRAG`, `RMDLDLG`, `RLISTBXC`), and
   `BobEvtAuto_1..5` in six each — because `__COUNTER__` restarts at zero in every translation unit
   and the constructor was defined out of line, giving its symbol external linkage.
2. **The link flag is present:** `-Wl,--allow-multiple-definition` (`CMakeLists.txt:103`).
3. **And yet nothing is lost.** `BOB_EVTREG_COUNT=1` reports the registry at first fire: **438
   entries from 81 distinct classes**. The tree has **81** real sink maps (case-variant twins
   resolved by inode) naming **80** distinct classes. Every one registers.

**The decisive form of that measurement is the before/after pair, not the absolute number:**
81 distinct classes **before** the fix and 81 **after**. Had the linker been discarding registrars,
the *before* number would have been the lower one. It was not, so nothing was being lost.

Fixed anyway — keyed on the **class**, constructor **in-class** — because *"inert today"* is not a
property anyone designed. A change of optimisation level or unity layout could arm it, and the
correct key costs nothing.

**What is NOT claimed: why it is inert.** The count proves no registrar is lost; it does not prove
the mechanism, and §8-MA113 is this pair of ports being caught inferring a mechanism from a number.
If it ever matters, `objdump` the pre-fix `_GLOBAL__sub_I` and read whether it *calls* the
constructor or inlines it.

**For MA, the transferable part:** when the sibling port sends "do you have this too?", the useful
answer is a **before/after measurement of the thing that would be missing**, not an inspection of
the thing that looks wrong. The symbol table said "nine collisions" and would have supported a
confident wrong answer in either direction; the registry count answered it in one line.

## §8-BoB196 — ⭐ answering §8-MA111, and MA's S166 listbox bug is LIVE here (worse) **[ENGINE]**

MA asked: *"is there a control type your dialogs DRAW that your click routing never offers a point
to?"*

**MA's own bug cannot occur here.** BoB's click walk is **type-agnostic** — polymorphic `OleHost`
with a virtual `onClick()` — so there is no allowlist to forget a type in. That is the §8-MA104 /
§8-MA106 shape again: *when one port cannot express the other's bug, the difference is an
abstraction, not a fix.*

**But the question found a real one anyway.** Of the eight host types, only **three** answer a click:
`RCOMBO` (`onClick`), `RBUTTON` (`onClick`), `RRADIO` (`onButtonClick`), plus `RLISTBOX` via
`rowAtY`. **`RSPINBUT` overrides nothing — drawn and inert.** The Luftwaffe Directives grid is built
from spin buttons, so the player cannot change their own orders. Logged.

### And following `rowAtY` found MA's S166 bug live here — on the player's click path

```c
short row=(short)((y+m_lVertScrollPos)/tm.tmHeight);
if (row>m_playerList.GetCount()) row=-1;          // GetRowFromY
```

`m_playerList` is filled **only** by `AddPlayerNum` (multiplayer / player log); rows come from
`AddString` into **`m_list`**. The control's own `OnLButtonDown` clamps against `m_list` and is
correct — **two opinions about "how many rows do I have" inside one control**, and the one on the
click path was the wrong one.

**MA could correct it outright because `GetRowFromY` has no caller in its game tree. In BoB it is
`bob_ole_click → OleHost::rowAtY → GetRowFromY` — the real player click.** Measured on the Luftwaffe
Bases dialog, a 7-row unit list:

```
[lbrow] y=79 -> row=4 REJECTED (playerList=0, but m_list has 7 rows)
[lbrow] y=61 -> row=3 REJECTED ...      [lbrow] y=43 -> row=2 REJECTED ...
```

**Only row 0 was ever selectable, on every list in the game that is not a player list.** After the
fix, rows 2/3/4 are kept. `BOB_LB_PLAYERCLAMP=1` reverts; `BOB_TRACE_LBROW=1` reports each decision.

**The transferable point is about how the question was answered.** MA's note asked about *its* bug
(a type allowlist). BoB does not have that construct — and answering only *"N/A, different design"*
would have been true, tidy, and would have missed a worse bug two calls further down. **Follow the
sibling's question to the same OUTCOME in your tree, not to the same CODE**: "which drawn controls
cannot be clicked" is answerable in any design, and here it led from a missing `onClick` override to
a row clamp reading the wrong list.

## §8-BoB197 — ⭐ a shared accessor on a per-window object lies to every control that uses it **[ENGINE]**

BoB's `RSPINBUT` took no clicks at all, and the Luftwaffe Directives allocation grid is built from
~50 of them — so the Luftwaffe player could not change a single number in their own orders.

The interesting cause is not the missing click route. It is this:

```c
void CWnd::GetClientRect(LPRECT r) const {           // compat, BoB
    int w=0,h=0; bob_gdi_screen_size(&w,&h);         // ...the whole SDL window
    r->left=r->top=0; r->right=w; r->bottom=h; }
```

Written for the front-end panels, which genuinely need real window geometry — and **wrong for every
OCX control hosted inside one**, because the controls' own handlers do real work with that rect:

```c
void CRSpinButCtrl::OnLButtonDown(UINT, CPoint point) {
    GetClientRect(rect);
    if (point.x < rect.right-15) return;             // the arrows are the right 15px
    ...
    m_bGoingDown = !(point.y < rect.bottom/2);       // up vs down
```

With a 1024×768 rect, **every** spin click is rejected before it begins, and any that got through
would always have meant "down". Same family as **§8-BoB173d** (`GetWindowRect` returning the whole
screen made one open dialog eat every click) — that is twice now for the same accessor class.

**The rule:** *when a port replaces a per-window object with one shared instance, every accessor on
it must still answer per-window — or it will lie, quietly, to the game's own handlers.* The lie is
invisible in a capture: the control draws correctly, in the right place, at the right size. Only its
behaviour is wrong.

Fixed here narrowly: `GetClientRect` is now `virtual` with the old behaviour as the default, and the
spin host overrides it with its drawn rect. Plus a new `onClickXY(localX, localY)` on `OleHost`,
because `onClick()` carries no coordinates and `onButtonClick()` carries only X — neither can
express "arrows on the right, up/down by Y". The host then drives the control's **genuine**
`OnLButtonDown`.

### MA: N/A, and the reason is an abstraction you already have

Measured in MA's tree, not assumed:

```c
void GetClientRect(LPRECT r) const { ... r->right = m_maW; r->bottom = m_maH; }   // MA compat
```

MA's `CWnd` carries a per-window `m_maX/m_maY/m_maW/m_maH` and reports it, so MA cannot express this
bug. Third time this pair has landed on *"one port has a class of bug the other structurally cannot"*
(§8-MA104, §8-MA106, here) — and each time the difference was **a missing abstraction, not a missing
fix.** BoB's `virtual` + per-host override is a narrower version of MA's rect; the general remedy is
the rect.

### ❓ But MA has the same FEATURE gap, from the other side

MA compiles `RSPINBUT.CPP` (it is in `port/lists/mfc2_ok.txt`) but **hosts no spin-button type at
all** — there is no `CT_SPIN` in `ma_olecontrol.cpp`, so those controls are neither drawn nor
clickable. BoB's spin buttons were hosted-but-inert; MA's are absent. **Same outcome for the player.**

That is not academic for MA right now: **its own Wonju walkthrough, step 8, says *"add a third flight
— either via the Squadron slot's Flights spin-box…"*.** EPIC K will walk into this. BoB's S142 host
(`bob_ole_rspinbut.cpp`) plus this sprint's `onClickXY` is a working reference for the whole type,
including the two `CRSpinBut`-specific traps S142 documents (the class-wide `m_bDrawing` flag that
latches, and the missing `m_FirstSweep`).

## §8-MA115 — ⭐ a recipe that addresses a ROW addresses a CELL, and the cell it picks is the middle one **[HARNESS]**

**MA S170.** MiG Alley's click recipes grew a `:rN` form in S162 so a recipe could name row N of a
listbox instead of clicking the control's centre — the centre being the *middle row*, which had
quietly selected the one option the PO's walkthrough said explicitly not to pick.

S170 hit the same fault one dimension further out. The Profile wave table is five columns —
`Wave / ToT / Main Duty / AAA Cover / Air Cover` — and `CProfile::OnClickedTask` reads `currcol`,
i.e. **the column the last click landed in decides which duty the TASKS dialog edits**. `:r1` picks
row 1 and the row's *horizontal centre*, which on that table is **column 3**. So the recipe opened
the flak tab, the dialog was real, every control in it worked, and the gate was measuring the port
editing a duty the walkthrough never touches.

It was caught only because the spin-box on the resulting dialog **refused to move** — a 2-entry list
already at maximum. Had that slot happened to have room, the gate would have passed while testing
the wrong thing, exactly as S85 and S162 did.

**The rule:** a recipe form that resolves *one* coordinate silently supplies the other from the
control's geometry. Where a control's behaviour depends on both — any multi-column list — that
default is a guess wearing the costume of a resolved address.

MA's fix: `#ID@Class:rN.C` names the cell, resolved through the control's own `GetRowFromY` **and**
`GetColFromX`. Both resolvers already existed; they had never been usable together because they
shared one `col` parameter. Row and column now travel in one int, encoded in one place and decoded
through two macros.

**For BoB:** `bob_ole.cpp`'s S192 column probe and S197 `onClickXY` mean BoB's click path already
carries a real column — but check whether BoB's *recipes* can name one. If a BoB recipe can only say
"this listbox" or "row N of it", every multi-column list in the OOB dialogs (the Order of Battle
squadron lists especially) has the same unaddressable-cell problem, and any gate over them is
asserting on whichever column the row centre happens to fall in.

## §8-MA116 — the click-type filter is an allowlist, and that is now four silent failures **[ENGINE]**

**MA S170.** `ma_ole_toolbar_click` decides which hosted control types may take a click by testing
`h.type != CT_X && h.type != CT_Y && …`. A type not in the list is **drawn normally and does
nothing** — no warning, no trace, no failure.

Found by discovery, one epic at a time:

| sprint | type | what was inert |
|---|---|---|
| S87 | `CT_LISTBOX` | every row in Bases / Squads / D.I.S. / Intelligence |
| S140 | `CT_SCROLL` | every scroll bar in an OOB dialog |
| S163 | `CT_COMBO` | five combos on the TASKS dialog; the Damage tab's element list |
| S170 | `CT_EDTBT` | `IDC_ACTYPE`, the duty field — **the only door to the spin-box dialog** |

Four of these, each found because a feature *above* it was being built, is not a run of bad luck; it
is the predictable output of an allowlist with no complement check. The cheap fix is not another
entry — it is a trace that fires when a click lands inside a hosted control whose type the filter
rejects, so the next one announces itself instead of reading as "that feature is broken".

**For BoB:** BoB's click walk is type-agnostic (§8-BoB196), so this construct cannot exist there —
which is the answer, and worth keeping written down, because the same *question* asked in S196 is
what found RSPINBUT.

## §8-MA117 — ⭐ a teardown destroys a SUBTREE; the registry lost exactly one window **[ENGINE]**

**MA S171.** `RDialog::EndDialog` tears down a whole tree of dialogs. Compat's
`CWnd::DestroyWindow` — the only place that calls `ma_ole_remove_by_parent` on that path —
deregisters **one** window's hosted controls. Everything below it stayed in the registry, still
flagged `m_maVisible`.

Closing the campaign Profile dialog and reopening it therefore produced **two live `CProfile`s and
two live `CFlt_Task`s**. The id resolver picks the first match in map order, i.e. **by pointer**, so:

```
#2149@CFlt_Task      -> opened the dropdown on the LIVE combo
#2149@CFlt_Task:r1   -> "needs its dropdown OPEN first"   (it had resolved to the DEAD one)
```

Two clicks naming one control reached two different controls. Nothing crashed, nothing warned; the
symptom was a recipe that stopped making sense.

Two corollaries, both worth stealing:

- **A class qualifier does not disambiguate a dialog from its own corpse.** MA's ambiguity warning
  (§8-MA-S85) only ran when *no* class was given, on the assumption that naming the class settles
  it. Count candidates after the **same filters the resolver uses**, class included.
- **Walk `fchild`, `dchild` AND `sibling`.** The paint recursion follows `fchild`/`sibling` only, so
  `dchild` nodes are never painted and are still hosted — the same asymmetry S169 hit when scoping
  dialogs off the frag screen. A teardown walk that mirrors the *paint* walk will miss them.

**For BoB:** BoB hosts its controls in a `CWnd*`→host side-table too. The question is not "do you
have `ma_ole_remove_by_parent`" but **"when a dialog tree is destroyed, how many of its windows does
the registry hear about?"** Close and reopen one OOB dialog, then count hosts for a known id. If the
count doubles, every recipe and every click on that screen is resolving by pointer luck.

## §8-MA118 — ⭐ a gate that cannot fail on a crash is not a gate, it is a log grep **[PROCESS]**

**MA S171.** `port/flak_suppression.sh` printed eight green assertions and `PASS` on a run that died
with `SIGSEGV`. Every assertion was **true** — all the evidence it read was in the log before the
crash — and the gate simply never looked at how the run ended.

The audit that followed is the part worth copying. Of MA's thirteen gates:

- **one** checked crashes (`oob_sweep`) — and that is the gate whose entire job is counting them;
- **three** checked `$?`, which a crash on a worker thread need not disturb, and which the
  screenshot-and-exit path muddies anyway;
- **nine** checked nothing.

The fix is a shared `assert_no_crash` that reads the binary's own `=== CRASH: signal` banner (the
log is authoritative where the exit status is not) **and symbolises the top frames**, because an
address list is not a diagnosis and nobody runs `addr2line` on a gate that already said PASS.

**The general rule: every gate must assert on how the run ENDED, not only on what it emitted.** A
gate reads a prefix of the log by construction; a crash is invisible from inside that prefix.

**For BoB:** `tools/bob_gates.sh` records `exit=0` per recipe, which is better than MA had — but
check whether a *thread* crash changes it, and whether GATE 4/4b would still report their pixel
percentages if the process died after the frame dump. The failure mode is not "the gate is wrong",
it is "the gate is right about a run that never finished".

## §8-MA119 — the same engine file, one year apart: BoB already has the fix MA is missing **[ENGINE]**

**MA S171.** MA's `CRSpinButCtrl::GetCurrentText` (RSPINBTC.CPP):

```c
ASSERT(m_list.GetCount()); // have at least one entry!
char tempst[255];
strcpy(tempst, m_list.GetAt(m_list.FindIndex(m_index)));
```

`FindIndex` on an empty list returns NULL; `GetAt` dereferences it. `NDEBUG` compiles the assert
out, so a spinner drawn before it is populated is a straight SIGSEGV at `fault_addr=0x8`.

**BoB's copy of the same function is already fixed**, by the original team, a year later:

```c
//DEADCODE RDH 29/10/99 //	ASSERT(m_list.GetCount()); // have at least one entry!
    if (m_list.GetCount())                                        //RDH 29/10/99
        strcpy(tempst, m_list.GetAt(m_list.FindIndex(m_index)));
    else                                                          //RDH 29/10/99
        tempst[0]=0;                                              //RDH 29/10/99
```

So BoB needs no guard here and MA does — and MA's belongs in the **host** (`ma_spin_draw` skips a
draw whose precondition the control documents and does not check), leaving game source pristine.

Two things generalise:

1. **When one port crashes in shared engine code, read the other port's copy of the file before
   theorising.** These trees are the same engine a year apart; the later one carries fixes the
   earlier one never got, dated and initialled in the source. That is a *diff*, not a diagnosis to
   be derived.
2. **Hosting a control type makes every instance live at once — including the ones no story has
   driven.** MA's crash was not in the spinner the sprint was about (ChooseSquad's, populated
   immediately) but in `WPDetail`'s ETA spinner, hosted by the same S170 change, named in that
   sprint's residual as "never driven", and drawn empty by the global paint pass. A commented-out
   `ASSERT` in shipped 1999 code is a **map of the preconditions nobody checks at runtime**; grep
   for them when you first host a type.

## §8-MA120 — ⭐ a workaround's comment records the hazard as it was THAT DAY **[ENGINE]**

**MA S172.** MiG Alley's map clicks were driven by calling the map dialog's own handlers — down and
up **in the same tick** — with a comment explaining exactly why:

> *Down+Up in the same tick keeps `m_bDragging` FALSE, which also avoids `CMapDlg::OnMouseMove` — it
> dereferences `GetDC()` unchecked.*

That was accurate when written (S95). By the time a story needed real dragging, compat's
`CWnd::GetDC` had grown a real static `CDC` and the hazard no longer existed — so the engine's entire
press-move-release chain (`OnMouseMove` recomputing the dragged item's world position, `OnDragItem`
clamping it into the theatre and recalculating the route and fuel) had been sitting intact and
unreachable for seventy sprints.

The workaround was correct, well-commented, and by then unnecessary. Nothing announced that.

**The rule: when a story finally needs the path a workaround was written to avoid, re-check the
hazard before designing around it.** A dated note explaining why something is avoided is evidence
about the past, not a standing property of the code. Both these ports are full of them.

Cheap discipline: when a workaround's stated hazard is a *specific* call (`X derefs Y unchecked`),
that is a one-line grep to re-validate, and it is worth doing before building anything on top of the
avoidance.

## §8-MA121 — ⭐ a trace is code, and it can be wrong in a way the thing it measures cannot **[PROCESS]**

**MA S172.** Instrumenting waypoint drags, the "after" world position was read back through the
dialog's `m_buttonid` member. But the drop path (`OnLButtonUp` → `OnDragItem` → recalc → repaint) is
free to change that member, so the **second** drag reported the **first** waypoint's coordinates:

```
released ... world (72208160,57594405) -> (71183770,59535093)   <- Initial Point
released ... world (57230421,58018282) -> (71183770,59535093)   <- Egress
```

Two different waypoints, **byte-identical** final coordinates. The drags were correct; the
measurement was not.

It was caught only because the collision was *impossible* rather than merely surprising. Had the
stale read produced a plausible number — a nearby waypoint, a slightly-off delta — it would have gone
into the gate as the oracle, and the gate would then have asserted the instrument's bug forever.

Two things generalise:

1. **Read the subject through a handle you captured, not through a member the operation may rewrite.**
   Capture the uid/pointer *before* the operation and use it afterwards.
2. **Prefer oracles that can be impossible, not merely wrong.** Two independent items reporting the
   same value, a count that exceeds its own maximum, a percentage over 100 — these announce
   themselves. This port's own history is mostly the other kind: S164 compared counts to answer a set
   question, S192 read a zeroed instance count as "never launched", S171 read a truncated recipe as
   "the values never changed". Each was plausible.

**For BoB:** the same shape applies to any trace that reads engine state through a "currently
selected / last hit / active" member — `m_buttonid`, hover ids, current-page indices. If the trace
runs after a handler that can change the selection, it is measuring the selection, not the subject.

## §8-MA122 — ⭐ a dialog can be ambiguous with ITSELF, and not only after a reopen **[ENGINE]**

**MA S173.** `§8-MA117` recorded one way a class qualifier stops disambiguating: a dialog closed and
reopened leaves a second live copy. There is a second way, and it needs no bug at all — **a screen
that hosts N instances of the same sub-dialog by design**.

MiG Alley's frag screen hosts **three `CFragPilot` sub-dialogs**, one per package, each with the same
control ids. `#2356@CFragPilot` matched three visible hosts, and the resolver would have taken
whichever sorted first in a pointer-keyed map.

Two things made this cheap rather than expensive:

1. **The ambiguity warning from `§8-MA117` fired on the first run.** It was written to diagnose the
   reopen case and caught a case it was not written for. *A diagnostic that names the ambiguity is
   worth more than the specific bug that motivated it.*
2. **The fix orders instances by SCREEN POSITION, not map order.** Map order is by pointer, i.e. by
   whatever the allocator did that run. "The second flight row" has to mean the one the player sees
   second, or the recipe is addressing luck. This is the S95 rule — *ask the screen, not the heap* —
   one level up from controls to dialogs.

Recipe form `@Class#N`, carried inside the class string so no existing caller or form changes.

**For BoB:** the OOB dialogs are the place to look — repeated per-squadron or per-flight panels with
shared ids. BoB's `bob_ole_ctrl_point` filters by `parentDlg`, so a caller that *has* the right
dialog pointer is fine; the exposure is anywhere a recipe names a control by id and lets the resolver
choose the dialog. Worth a count before it matters.

## §8-MA123 — a written step does not say which WIDGET the game uses **[PROCESS]**

**MA S173.** The PO's script says *"change callsign"*. That became an acceptance criterion reading
*"Callsign edit accepts text (cf. PO-16)"* — inheriting a **text-entry** assumption, and a dependency
on an unrelated open item, from a phrase that never mentioned a widget.

The control is a **combo**: `FillComboBox` populates it from the game's callsign string table,
filtered so two groups cannot hold the same one, and the player *picks*. There is no text entry on
that path at all. The story had been carrying a blocker it did not have.

Cheap and worth doing: **when turning a walkthrough step into an acceptance criterion, look up the
control before writing what it does.** One grep of the dialog's event map. Otherwise the criterion
encodes a guess about the UI, and every later sprint reads that guess as a requirement — including
"blocked by PO-16", which was not true.

## §8-MA124 — a synthetic DRIVER is code, and it fails in the shape of the bug you are hunting **[PROCESS]**

**MA S174.** `§8-MA121` recorded that a *trace* can be wrong in a way its subject cannot. The other
half is worse, because it points the same direction as the investigation.

Driving a takeoff, the harness released the wheel brakes at **two** points in the run. They toggle —
so it released them and immediately re-applied them. The symptom was *full throttle, a plateau at
20 kt, no lift-off*: **exactly** the defect being investigated. Had the real defect not been present
underneath, that driver bug would have been reported as the finding.

A trace that lies is usually caught by an impossible value. **A driver that lies produces a
plausible failure of the system under test** — which is what the run was looking for.

Two defences, both cheap:

1. **Trace what the driver did, not only what the system did.** A one-line `[autofly] wheel brakes
   released at t3d=60` makes a double-release visible; without it the driver is a black box that
   agrees with your hypothesis.
2. **A/B the driver against no driver.** MA's runway plateau was only trustworthy because the
   no-input run held 0 kt for 420 frames. That single control run separates "the drive works and the
   system is broken" from "the drive does nothing" — which is the same trap S93 fell into (a drag
   harness that pushed SDL events nobody drained, and whose round-trip test happily reported
   "lossless").

**For BoB:** `BOB_AUTOCLICK`, `BOB_CLICKXY`, `BOB_AUTOFLY` and `BOB_KEYSEQ` are all this kind of
driver. Where a gate's verdict depends on one of them having acted, the gate should assert the
driver's own trace line, not only the outcome.

## §8-MA125 — a drive recipe written for one path is not usable on a longer one **[HARNESS]**

**MA S174.** `BOB_AUTOFLY=throttle` taps the throttle every 30 pumps **while `cnt < 600`**, counting
from process start. Perfectly correct for the Quick Mission path it was written for, where the flight
starts almost immediately.

On the campaign path — title, campaign select, map, dossier, authorise, mission folder, frag, *then*
Fly — six hundred pumps elapse before a flight exists. **Every tap was spent in the front end.** The
symptom is an aircraft that sits at 0 kt with a drive that "should" be flying it.

The fix is not a bigger number. It is to count from **the state you actually depend on**: MA now
publishes `g_ma_in3d` from the idle loop and the takeoff drive counts from the sim being up.

**The general rule: a synthetic drive should be keyed to a STATE, not to a frame count**, whenever
the path to that state can grow. Frame-count recipes are fine for a fixed prefix and quietly wrong
the first time someone puts more screens in front of them — and nothing announces it, because the
recipe still runs, on time, against nothing.
