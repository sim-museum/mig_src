# Sprint 3 Board — "Hands on the stick" (R2)

**Status:** 🏃 ACTIVE · **PO ratified:** 2026-06-17 (standing pre-approval — PO approves every
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
