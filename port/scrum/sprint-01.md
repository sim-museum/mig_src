# Sprint 1 Board — "Dependable launch"  ✅ CLOSED / ACCEPTED (PO, 2026-06-17)

**PO acceptance:** A1 (20/20) and A4 ACCEPTED as done. A2 accepted code-complete; its live
round-trip re-demo carries to Sprint 2 (gated on display recovery).

**Dates:** 2026-06-17 → +2 weeks
**Sprint Goal:** *The game launches to 3D flight reliably and remembers my settings.*
**Capacity:** ~20 pts · **Committed:** 21 pts · **PO ratified:** 2026-06-17
**Release context:** First PO acceptance gate is **R2 — Flyable 3D**. Sprint 1 is the reliability
foundation that gate depends on.

---

## Sprint Backlog

| ID | Story (pts) | Tasks | Status |
|---|---|---|---|
| **A1** | Harden intermittent 3D launch (13) | A1.1 diagnose ✅ · A1.2 barrier ✅ · A1.3 guard ✅ · A1.4 20× stress ✅ (**20/20 OK**) | ✅ **DONE** |
| **A2** | Persist `Save_Data` to disk (5) | A2.1 locate ✅ · A2.2 serialize-on-exit ✅ · A2.3 reload-on-boot ✅ (already worked) · A2.4 round-trip ⚠️ live demo blocked by env | 🔍 code-complete, demo blocked |
| **A4** | Stress-launch harness (3) | A4.1 `port/stress_launch.sh` ✅ (poll-for-dump + signal classification) | ✅ **DONE** |
| **+** | Build hardening (bonus) | `rebuild.sh` bare-name resolution for `mfc2_ok.txt` → from-scratch rebuild now links (was relying on stale objects) | ✅ done |

Status: ⬜ todo · 🔨 in progress · 🔍 in review · ✅ done

### A1 — root cause (the real bug, deeper than "add a guard")
`View3d` ctor (`STUB3D.CPP:730`) published `this` into `inst->viewedwin` (under the mutex) but
initialised `drawing`/`View_Point`/`Whole_Screen`/`mode` *after* releasing the mutex. `drawing`
is not in the ctor init list → it held garbage between publish and init. The sim thread's
`DoMoveCycle` guard `if (!view->Drawing()) continue;` therefore tested garbage; if it ≠ `D_NO`
the guard passed and dereferenced the (also garbage) `View_Point` → intermittent wild-pointer
crash. **Fix:** initialise those fields *before* publishing the view (MA_LINUX block before the
mutex). Validated **20/20** consecutive 3D launches, no SIGSEGV/SIGFPE.

### A2 — what's done / what's blocked
`SaveData::SavePreferences()` (write) and the boot load (`SAVEGAME.CPP:1591`, already working)
both pre-exist. The only user-exit that called save was the in-game Exit menu (`ConfirmExit`);
the SDL window-close / Ctrl+ESC paths `_exit()`'d without saving. Added `ma_save_preferences()`
(FULLPANE.CPP) and wired it into both SDL shutdown paths + the `BOB_EXIT_AFTER_DUMP` clean-exit
hook (bob_video.cpp). **Code complete + builds + links.** The live round-trip demo is blocked by
an environment wedge (below), not by the code.

### ⚠️ Environment note (not a code defect)
Aggressive SIGKILL stress-testing of GL apps wedged the session's SDL window-mapping path:
`SDL_CreateWindow` blocks forever on the X11 `MapNotify` (gdb backtrace: `ensure_window` →
SDL2 → `XIfEvent` → `poll(-1)`). Confirmed environmental: `glxgears` (direct GLX) renders at
58 fps, plain X windows map, but SDL's window+GL-context path hangs (XWayland MapNotify infinite;
native Wayland fails EGL "driver (null)"). Process/window cleanup, IME-disable, and driver-pin
did not clear it. **Likely needs a display-session restart (PO/user call) or self-heal over time.**
The A1 20/20 validation ran *before* the wedge; the binary itself is sound (gdb proves the hang is
in SDL infra, not game code).

---

## Definition of Done (this sprint)
Per global DoD in `scrum.md` §2: compiles into the build set, `wmig` links 0-undef, demonstrated
by running the binary (not just compiled), no regression to title/Preferences/first-3D-frame,
gated trace added, `CLAUDE.md` + memory updated, committed on `linux-port`.

---

## Daily Scrum log
*(async; 3 questions — yesterday / today / blockers)*

### Day 1 — 2026-06-17
- **Done:** Sprint planned & PO-ratified; board created. **A1**: mapped the startup threading,
  root-caused the publish-before-init race in the `View3d` ctor, fixed it, **stress-validated
  20/20**. **A4**: built `port/stress_launch.sh` (poll-for-dump + signal classification), proven.
  **A2**: located the load/save machinery, wired `SavePreferences()` into the SDL shutdown paths.
  **Bonus**: fixed a `rebuild.sh` from-scratch link failure (bare `mfc2_ok.txt` names).
- **Blockers:** A2 live round-trip + a fresh full re-validation are blocked by a session-level SDL
  window-map wedge induced by SIGKILL stress-testing (gdb-confirmed environmental, not code).
- **Next:** Sprint Review with PO; recommend display-session recovery to re-demo A2 end-to-end.

---

## Burndown
| Day | Remaining pts | Note |
|---|---|---|
| 1 | 21 | Sprint start |
| 1 (eod) | ~3 | A1 (13) ✅, A4 (3) ✅ done & validated; A2 (5) code-complete, live demo blocked |

---

## Increment / Review notes (for Sprint Review with PO)
- **Demoable now:** A1 fix + `port/stress_launch.sh` → **20/20** 3D launches reached & sustained
  the cockpit view, zero crashes (run earlier this session, before the display wedge).
- **Code-complete, demo pending display recovery:** A2 preference persistence on window-close.
- **Carry-over to Sprint 2:** A2.4 live round-trip re-demo once the GL/SDL display path recovers
  (or on a fresh session). 3 pts of A2 effectively done; ~2 pts of verification carried.
- **Process note:** switch stress runs to background tasks (tool-timeout-safe); never `pkill -f`
  a pattern that matches the test harness's own command line (it self-kills the shell).

---

## Sprint Retrospective (Scrum Master + Dev; no PO required)

**What went well**
- A1 paid off beyond the story: instead of bolting on another guard, we found and fixed the
  actual publish-before-init data race. Validated empirically (20/20), not just by inspection.
- gdb backtrace cut through hours of wrong theories (IPC, error-log modal, build regression) to
  the real blocker (SDL X11 MapNotify wait) in one shot — reach for it sooner next time.
- A4 harness + the rebuild.sh fix are durable infrastructure beyond this sprint.

**What hurt**
- Self-inflicted display wedge from `SIGKILL`-spamming GL apps blocked live validation. Cost real
  time and blocked A2.4.
- `pkill -f wmig` matched the test command's own shell → silently self-killed the shell and ate
  output; chased phantom "regressions" as a result.
- Long stress runs exceeded the 120s tool timeout when run inline → lost output.

**Action items (carry into Sprint 2 working agreements)**
1. Run app launches/stress as **background tasks**, never inline > ~90s.
2. Kill only by exact name: `pkill -x wmig` (never `pkill -f` a pattern matching our own command).
3. Stress-test in **bounded batches** (≤5) with pauses; don't SIGKILL GL apps in tight loops —
   it wedges the compositor.
4. Reach for **gdb on the hung pid** early when a launch hangs, before theorizing.
5. After any from-scratch `rm -rf port/build`, confirm the link TU count (~266) — guards the
   ok-list/path regression class.

---

## ▶ POST-RESTART RESUME (display wedge clears on reboot)

The SDL window-map wedge was session-level; a **reboot clears it**. After restart, from the
data dir, re-validate the carried A2.4 demo and confirm A1 still holds:

```bash
# 1. Rebuild if needed (binary is /tmp/wmig, lost on reboot):
bash port/rebuild.sh            # expect: 266 TUs, "OK -> /tmp/wmig"

# 2. Re-confirm A1 (should be 20/20):
bash port/stress_launch.sh 20 100 25             # run as a background task; ≤5 per batch if flaky

# 3. A2.4 round-trip (the carried demo): clean-exit save + reload, in 3D mode
cd /home/m/sgl/TUE/MigAlley/WP/drive_c/rowan/mig
SET=SaveGame/settings.mig; touch -d 2020-01-01 "$SET"; before=$(stat -c %Y "$SET")
timeout -k3 -sKILL 45 env BOB_RUN_INIT=1 MA_ENABLE_3D=1 MA_TRACE_SAVE=1 BOB_CLICKSEQ="50,588,232" \
  BOB_DUMP_FRAME=120 BOB_EXIT_AFTER_DUMP=1 BOB_DRIVE_C=/home/m/sgl/TUE/MigAlley/WP/drive_c /tmp/wmig
[ "$(stat -c %Y "$SET")" -gt "$before" ] && echo "A2 PASS: settings.mig rewritten on clean exit"
```
Working agreements (from Retro): background tasks for launches; `pkill -x wmig` only (never `-f`);
don't SIGKILL GL apps in tight loops. Then close A2 → Sprint 2 Planning (PO touchpoint).
