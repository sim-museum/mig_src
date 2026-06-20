# B2 — 3D fidelity A/B harness

Compare the native software rasterizer against the **Wine pixel-oracle** screen by screen
(the discipline that cracked the BoB cockpit — see `port/BOB_PORT_LESSONS.md` §7).

## Reference frames — `ref/wine/`

Captured from `mig.exe` under Wine, **software rendering, 640×480** (2026-06-19). The
window grabs are 1189×1076 and carry a **known horizontal-offset glitch** in the
mig/Wine setup (windscreen pushed right) — so pixel-exact match is *not* the goal;
these are a **structural** oracle (sky band / horizon / terrain mass / cockpit framing /
gauges).

| File | View | Switch key |
|------|------|-----------|
| `01_cockpit_fwd_gunsight.png` | cockpit forward, gunsight lit | default (no key) |
| `02_cockpit_fwd_hud.png`      | cockpit forward, HUD reticle | default |
| `03_cockpit_panel_down.png`   | cockpit panel, look-down     | (look-down pan) |
| `04_ext_chase_high.png`       | external chase, high         | F6 OUTSIDETOG (DIK 0x40) |
| `05_ext_chase_low_airfield.png` | external chase, low / airfield | F9 CHASETOG (0x43) |
| `06_ext_flyby_terrain.png`    | satellite / fly-by, terrain  | F10 SATELLITOG (0x44) |

View→key map from `SRC/H/KEYMAPS.H`: F6 outside, F7 inside, F9 chase, F10 satellite,
F1/F2/F3 enemy/friend/ground-target, ESC reset.

## Run

    port/ab.sh                       # all views
    port/ab.sh cockpit               # one view
    AB_CROP=0.04,0.02,0.04,0.10 port/ab.sh cockpit   # trim Wine window chrome (L,T,R,B; frac or px)

Outputs in `port/out/ab/<view>_{native,wine,diff,sidebyside}.png` plus printed stats
(RMSE / mean|diff| / %changed / per-channel). Open `*_sidebyside.png` = **native | wine | diff**.

Env: `BOB_DRIVE_C` (Wine drive_c), `WMIG` (binary, default `/tmp/wmig`),
`DUMP_FRAME` (back-Blt index, default 220), `KEY_AT` (pump count to tap the view key, default 80).

## How it works

- Native side: `MA_DUMP_BACK=N` writes the N-th back-surface Blt (16-bit 565) as a P6 PPM
  to `/tmp/maback.ppm` (`SRC/compat/ddraw_legacy.h`). `ab.sh` copies it per view.
- View switch: `BOB_KEYSEQ="pump,dik;…"` taps a DIK through the DI-keyboard path
  (`SRC/compat/bob_video.cpp` `pump_events`) before the dump.
- Compare: `ab_compare.py` (Python/PIL/numpy) normalizes both to 640×480, builds the
  side-by-side + abs-diff heatmap, prints stats.

## Status / known limitations (B2 work, not harness bugs)

- **View switching WORKS** (fixed 2026-06-19): F6/F9/F10 taps switch the rendered view
  and the **external/chase/satellite views render natively** (F-86 model + terrain +
  horizon). Root cause was *not* acquisition — `BOB_KEYSEQ` parsed the DIK with `%d`, so
  `0x40` read as decimal 0 (`kb_push(0)`, a no-op); switched to `%i`. Verified via
  `MA_TRACE_KEY`: `[key] DOWN scancode=0x40 -> action index=160` (OUTSIDETOG) and the
  captured frame switching from cockpit to external (external RMSE 127→72).
- **Timing coupling:** `KEY_AT` counts only pumps where the DI keyboard is *acquired*
  (acquisition is a few seconds into flight). The view tap must land before the dump, so
  the defaults are `KEY_AT=10` (tap soon after acquisition) + `DUMP_FRAME=300` (let the
  new view settle). `MA_TRACE_KEY` prints `[di] keyboard ACQUIRED` and `[keyseq] tap …`.
- **Cockpit-forward** (the #1 ROADMAP target) captures and compares cleanly.
- **Flight-state / camera-distance mismatch:** native captures soon after launch; Wine
  refs are in cruise, and the native satellite (F10) camera sits closer than Wine's
  top-down. Matching state/zoom is B2 fidelity refinement.
- **Crop calibration:** Wine grabs include window chrome + the offset glitch; tune
  `AB_CROP` per the captures for a fairer structural diff.
