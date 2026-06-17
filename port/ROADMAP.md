# MiG Alley — native Linux port: roadmap to "fully functional on Ubuntu"

Status as of 2026-06-16. See `CLAUDE.md` (Phase status) and the `migalley-port-state` /
`bob-port-notes-crossref` memory notes for the detailed per-feature history. This file scopes
**what remains** and the order to do it in.

## Where it stands

| Layer | State |
|-------|-------|
| Compile + link (Phase 1–2) | ✅ 15 game unities + MFC + 4 OCX projects → 32-bit i386 ELF, 0 undefined symbols |
| Boot + SDL2 window + main loop (Phase 3) | ✅ runs, presents frames, `BOB_DRIVE_C` resolves game data |
| 2D front-end **infrastructure** (Phase 4) | ✅ GDI canvas, RLE8 BMP decode, **stb_truetype text**, OCX hosting (RListBox/RStatic/RButton/RCombo), RT_DIALOG + RT_DLGINIT parsing, mouse input, RTTI eventsink, panel/control lifecycle |
| **Settings/Preferences** front-end | ✅ end-to-end: labels + combo values + tab nav + click-to-cycle + write-back to `Save_Data` |
| 3D flight, audio, video, campaign flow, MP | ❌ remaining (below) |

The UI *framework* is largely complete. **"Fully functional" is gated almost entirely on the 3D
flight engine (Area B).**

---

## Remaining work by area

### A. Complete the 2D front-end flow — *medium; infrastructure exists, screen-by-screen grind*
54 `FullScreen` definitions exist (`SRC/MFC/FULLPANE.CPP`); only ~6 settings screens are validated.
Not yet exercised: `singleplayer`, `campaignselect`/`campstart`/`campover`/`endcamp`,
`quickmission`(+`flight`/`debrief`), `readyroom*` (×12, the MP lobby), `paintshop`/`variants`
(loadout/skins), `loadgame`/`replayload`/`replaysave`, `credits`, `visitorsbook`, `mapspecials`,
`radio`, `commsfrag`/`commsquick`.
- **More OCX control types**: we host 4 of ~13. Still needed: RTabs, RTickBox, RRadio, REdit,
  RSpinButton, RTree, RScrlBar, RAnibut, RTitle. (Same pattern as the 4 done: real `CR*Ctrl::OnDraw`
  over the canvas + dispid dispatch + `olexxx` build mode.)
- **Icon/bitmap blit subsystem** for widget art (dropdown arrows, button face bitmaps, list icons).
- **DPI/font-scale pass** for resolution-correct layout.

### B. 3D flight engine — *very large; the dominant remaining chunk (~55% of the work left)*
MiG Alley renders 3D via a **software rasterizer** (`SRC/GRAPHICS/{GRAFPRIM,POLYGON,POLYJIM}` + x86
`.ASM` span primitives) into a palettized 8/16-bit framebuffer, plus an optional **D3D5/6
execute-buffer** HW path (`SRC/HARDWARE/{WIN3D,HARDWIN}`). This is a *different renderer* from BoB's
D3D7 — BoB's D3D7→GL/FBO work does NOT transfer; only the *class* of bugs does.
- **Done so far:** `ma_xasm.nasm` ports the **palette** primitives (Get/SetPaletteEntry,
  SelectPalette, GetPaletteTable); `matrasm.nasm` (math). The polygon fillers
  (GRAFPRIM/POLYGON/POLYJIM) compile (in the GRAPHICS unity).
- **Stubbed / not wired (port_link_stubs.cpp):** the 6 span/pixel + fade-table primitives —
  `XASM_SetColour`, `XASM_SetPixelWidth`, `XASM_HoriLineAddr`, `XASM_GetTransparency`,
  `XASM_GetLandFadeTable`, `XASM_GetHorizonFadeTable`. **The world stays black until these write
  pixels.** Other GRAPHICS ASM (`GRAFJIM`, `GRPASM25`, `GRAFPASM`, `LSTRASM`) not yet ported.
- **Two routes:** (a) port the software-rasterizer ASM faithfully to NASM/C (MA's native path —
  preferred), or (b) wire the D3D5/6 execute-buffer to OpenGL. Lean (a) for fidelity; the 2D
  present bridge already shows the 8-bit framebuffer.
- **Sub-work after first pixels:** terrain/landscape tiles, 3D aircraft models, cockpit instruments
  + HUD/gunsight, external/padlock/fly-by views, effects (tracers, smoke, explosions, flak).
- Itself a multi-phase project. See "Area B first-step plan" below.

### C. Input completion — *small-medium*
Mouse ✅. Need **keyboard** (DirectInput→SDL DIK scancodes — *liftable from `~/bob/SRC/compat`,
done there*) and **joystick/throttle** (SDL_Joystick→DirectInput, required for flight).
`SRC/INPUT/KEYTASM.ASM` (keyboard scan table) to port.

### D. Audio — *medium; independent, can run in parallel*
51 Miles `AIL_*` stubs (`miles_ail_stub.cpp`) + DirectSound, all silent. Map to **OpenAL**:
music (`MUSIC/`), SFX/samples (`SAMPLES/`), engine + comms. BoB has a partial OpenAL path to mine.

### E. Smacker video — *small / skippable*
Intro (`introsmack`) + cutscenes stubbed (`CloseSmack`/`OpenSmack`/`DoSmack`). Either `libsmacker`
decode → texture, or a graceful skip. Not on the critical path.

### F. Campaign / mission logic + flow — *medium; code compiles, flow unwired*
Game logic **compiles** (Phase 1). The **flow** must be driven in the engine's natural init order
(campaign → packages → squadrons → persons → nodes → battlefields → mission gen → briefing → fly →
debrief) — do NOT cold-start subsystems (BoB's #1 lesson). Plus **save/load**: `Save_Data`→disk,
campaign saves, and **registry** persistence (settings currently in-memory only). Depends on A + B.

### G. Multiplayer / DirectPlay — *large; optional for single-player "functional"*
The 12 `readyroom*` host/guest lobby screens; DPlay→sockets. Defer unless MP is in scope.

### H. Misc polish
Registry→disk, CD-check stubs, palette/colour fidelity A/B vs Wine, timers, save-game format.

---

## Critical path & sizing

```
Input (C) ──┐
            ├─→ 3D rasterizer (B, LARGE) ─→ flight renders
Audio (D) ──┘                                   │
Front-end screens + controls (A) ───────────────┼─→ Campaign flow (F) ─→ playable single-player
                                                 │
Video (E), Multiplayer (G) ── later / optional ─┘
```

Rough split of remaining effort: **3D flight (B) ~55%**, front-end flow + controls (A) ~20%,
campaign flow + save (F) ~10%, audio (D) ~8%, input (C) ~4%, video/MP/polish ~3%.

**Next milestone recommendation:** Area C (input) + the Area B first-step (a first 3D frame),
since that unblocks the entire flight half. Audio (D) can proceed in parallel by a separate effort.

---

## Area B — first-step plan ("a first 3D frame")

Goal: get the software rasterizer to write *any* correct 3D pixels (horizon + terrain + one model),
proving the pixel path end-to-end before the long fidelity grind.

1. **Wire the 6 stubbed span/pixel primitives.** Port `XASM_SetColour`, `XASM_SetPixelWidth`,
   `XASM_HoriLineAddr` (horizontal-span base address into the fb), `XASM_GetTransparency`,
   `XASM_GetLandFadeTable`, `XASM_GetHorizonFadeTable` from their original `.ASM` to NASM (extend
   `ma_xasm.nasm`) or to C. These are what `POLYGON`/`POLYJIM`/`GRAFPRIM` call to fill spans into
   the palettized framebuffer. (`port_xasm.py` exists to assist x87/x86→intrinsics.)
2. **Confirm the fb target.** The fillers write into the active DirectDraw surface's bits; ensure
   that surface is the one `ma_ddraw_present` (8-bit-indexed path) shows, and that
   `SetPalette`→`ma_ddraw_setpalette` feeds the live 256×3 palette (HARDWIN.CPP:541).
3. **Reach the flight-view render entry.** Find the 3D scene draw call (the flight view's paint /
   `Display3D`-style entry — not yet located; grep the MODEL/3D modules + the `quickmissionflight`
   FullScreen InitProc) and ensure the idle loop drives it, like the 2D `OnTimer`/`OnPaint` path.
4. **First target:** the QuickMission flight screen rendering a horizon + landscape tile + one
   aircraft model. Validate against Wine (frame dump → A/B, non-black coverage), the same discipline
   used for the 2D front-end.
5. **Then iterate:** terrain detail tiles → full aircraft models → cockpit/HUD → effects.

Gotchas to expect (from BoB's catalogue, as *bug classes* — BoB's renderer code does NOT apply):
COM refcount UAF on DirectDraw surfaces (give real `int ref`); `-fpack-struct` ABI boundary around
libc/std types; palette/fade-table correctness; masked-texture transparency.
