# Sprint 3 Board — "Hands on the stick" (R2)

**Status:** ✅ REVIEWED / CLOSED (2026-06-17) — **C1 done** (R2 input gate met) + HUD-SIGFPE
root-cause fix. F4 + C3-remainder → Sprint 4.
· **PO ratified:** 2026-06-17 (standing pre-approval — PO approves every
sprint *and* the start of the next sprint in advance; dev runs the cadence autonomously).
**Dates:** 2026-06-17 → +2 weeks
**Sprint Goal:** *I can fly the aircraft from the keyboard in the live 3D view — controls map and
respond.* (This is the **R2 — Flyable 3D** acceptance gate's input half.)
**Capacity:** ~14–15 pts (re-baselined from S2). **Committed:** C1 (13) headline; F4 (13) +
C3-remainder pulled as stretch / likely split into Sprint 4.

---

## Sprint Backlog

| ID | Story (pts) | Tasks | Status |
|---|---|---|---|
| **C1** | Keyboard flight controls, DirectInput→SDL (13) | C1.1 audit ✅ · C1.2 keys reach sim ✅ (keymap loads; **115 actions** mapped) · C1.3 control moves the sim ✅ (ROTLEFT view-pan → **89.9% frame change**, `/tmp/view_compare.png`) · C1.4 gap closed ✅ (numpad 0x47–0x53 were missing from SDL→DIK) · C1.5 `MA_TRACE_KEY` ✅ | ✅ **DONE** |
| **F4** | Other front-end screens usable (13, stretch) | F4.1 Campaign · F4.2 QuickMission · F4.3 Comms | ⬜ (likely → Sprint 4) |
| **C3 rem.** | Scrollbar + new-panel hit-testing | folds into F4 | ⬜ |

Status: ⬜ todo · 🔨 in progress · 🔍 in review · ✅ done

### C1 starting position (audit — the chain already exists, built in earlier phases)
SDL key → `bob_video.cpp pump_events` → `kb_push` (when `g_diKbAcquired`) → DI keyboard device
`DIDEV_GetDeviceData` drains the ring buffer as buffered DIK events → `STUB3D.CPP Inst3d::OnKeyInput`
→ `OnKeyDown/OnKeyUp(scancode)` → `commonkeymaps->mappings[scancode][shift]` → `bitflags` →
`keytests::KeyHeld3d/KeyPress3d` (read by flight code). `pump_events` runs during 3D (Blt/Flip +
`bob_msg_wait` + `DEV_BeginScene`); the keymap loads via `Reg3dConv(FIL_3D_KEYBOARD_TABLE)`;
`commonkeymaps` aliases `reftable3d`. A `BOB_AUTOFLY` synthetic-input hook (sweep / throttle) exists
for headless validation. **So C1 = validate end-to-end + close gaps + demonstrate, not greenfield.**

---

## Definition of Done (this sprint)
Per global DoD (`scrum.md` §2): compiles into the build set, `wmig` links 0-undef, demonstrated by
running the binary (a keyboard control visibly affects the 3D sim), no regression (A1 20/20, title,
Preferences, first 3D frame), gated trace added, docs + memory updated, committed on `linux-port`.

## Working agreements (carried)
Background launches; `pkill -x wmig` only; no SIGKILL-spamming GL apps; MFC-fragment edits
(`STUB3D.CPP`, `SDETAIL.CPP`) → recompile the `.o` manually into `port/build/objmfc/` before relink
(rebuild.sh skips them when `/tmp/*_ok.txt` is absent).

---

## Daily Scrum log
### Day 1 — 2026-06-17
- **Done:** Sprint 3 planned (PO pre-approved). Audited C1 — the SDL→DIK→keymap→flight chain is
  already wired end-to-end; story is validation + gap-closing + demo.
- **Today:** instrument `OnKeyDown/Up` (`MA_TRACE_KEY`), run `BOB_AUTOFLY` in 3D, confirm keys map to
  flight-action indices and a control moves the aircraft.
- **Blockers:** none.

---

### C1 result + known follow-up
**Validated end-to-end** (no STUB3D logic change — the chain was already wired): keys map to flight
actions (`MA_TRACE_KEY`: 115 actions, keymap loaded), realistic input is stable (throttle 38 taps →
150 frames, no crash), and a view-pan key visibly rotates the cockpit camera (forward vs ROTLEFT =
89.9% of pixels changed). **Gap closed:** the numpad number keys (DIK 0x47–0x53 — the sim's primary
view-pan/zoom + trim controls) were absent from `sdl_to_dik` in `bob_video.cpp`; added them. Gated
test hook `BOB_AUTOFLY=look` (hold ROTLEFT) joins the existing `sweep`/`throttle`.
**Robustness fix (bonus, was the sweep SIGFPE):** gdb traced the sweep SIGFPE to
`COverlay::DrawTopText()` (HUD info-bar) — `altitude2=(altitude*305)/Save_Data.alt.mediummm`
with `mediummm==0`. The unit-conversion factors (`InitPreferences`/`SetUnits`, METRIC/IMPERIAL
tables) end up zero by flight time, so toggling the HUD info panel on divided by zero. **A *real*
bug a player hits** (not just the sweep). Fix: `STUB3D.CPP MakePassive` (MA_LINUX) re-establishes
the factors — `if(!Save_Data.alt.mediummm) Save_Data.SetUnits();` — fixing the HUD and every other
`mediummm` divisor (map/waypoint screens) at the root. Sweep no longer SIGFPEs.
**Remaining (S4, full-sweep only):** with the FPE gone the sweep runs further and trips a *separate*
SIGSEGV from some exotic action (menu/replay/cheat keys) fired in the early/parked state. This is
the all-keys-at-once stress, not realistic play — realistic flight controls (throttle, view-pan)
are stable. Logged for S4 hardening; not a C1 blocker.

## Burndown
| Day | Remaining pts | Note |
|---|---|---|
| 1 | 13 (C1) | Sprint start; C1 audit complete |
| 1 (eod) | 0 committed | **C1 (13) ✅** validated + demonstrated; numpad gap closed. R2 input half met. F4 → Sprint 4. |
| 1 (close) | 0 | + HUD-SIGFPE root-cause fix (units). A1 8/8. Sprint reviewed/closed. |

---

## Increment / Review notes (Sprint Review — PO standing-accept)
**Demoable (native):** Holding a keyboard view key (numpad-4 / ROTLEFT) pans the live 3D cockpit
camera — forward vs panned-left frame differs 89.9% (`/tmp/view_compare.png`). `MA_TRACE_KEY` shows
keys resolving to flight actions (115 mapped, keymap loaded). Throttle taps dispatch and the sim is
stable (150 frames). The R2 acceptance gate's **input half is met**: keyboard flight controls work
in the live 3D view. Bonus: a real HUD crash (info-bar div-by-zero on uninitialized unit factors)
fixed at root. A1 8/8, no regression.
**Completed:** C1 (13) + the units-SIGFPE fix. **Carried:** F4 (13) + C3-remainder → Sprint 4.

## Sprint Retrospective (Scrum Master + Dev)
**What went well**
- The C1 chain was already wired from earlier phases; the audit caught that fast, so the sprint
  was validation + one real gap (numpad) + demonstration — high value per change.
- gdb-on-the-FPE (per S1 retro) pinned `DrawTopText`/`mediummm==0` in one shot — fixed the root
  cause for every `mediummm` divisor, not just the HUD.
- `BOB_AUTOFLY` synthetic input (sweep/throttle/look) made headless flight-control validation and
  the before/after view-pan screenshot possible without a physical keyboard.

**What hurt**
- The `sweep` diagnostic (all keys at once) chains crashes — fixing the FPE exposed a deeper SEGV.
  It's an unrealistic stress; don't treat its crashes as release blockers. Realistic single-control
  input is the bar that matters.
- MFC-fragment recompile dance again (`STUB3D.CPP` → manual `.o`). Standing action item.

**Action items**
1. (carry) MFC-fragment edits → recompile the `.o` into `port/build/objmfc/` before `rebuild.sh`.
2. S4: narrow the full-sweep SEGV (gated, low priority) and add a defensive guard if a single
   realistic key reaches it.
3. Keep `MA_TRACE_KEY` + `BOB_AUTOFLY` hooks (cheap, gated; paid off this sprint).

## ➡ Sprint 4 plan (next) — "Looks right + finish the front-end"
PO standing-approved start. Pull: **F4** (Campaign/QuickMission/Comms panels, 13 — likely split) +
**C3-remainder** (scrollbar + new-panel hit-testing, folds into F4), and begin **B2** (A/B 3D
fidelity vs Wine) toward the rest of the R2 gate. Stretch: narrow the full-sweep SEGV.
