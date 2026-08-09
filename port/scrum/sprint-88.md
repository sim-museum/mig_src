# Sprint 88 — "Bind it yourself" — ✅ CLOSED 2026-08-09 — ⭐ H2: key bindings are user-editable

**Planned 2026-08-09 (PO pre-approved ceremonies; continuous-sprint directive). Autonomous. ~8 pts.**
**Sprint Goal:** turn from the campaign-UI arc (S82–S87) to the ship backlog and deliver **H2 —
"As a user, I can rebind controls, so the game fits my setup"** — plus the H3 docs it needs.

| Story | Pts | Result |
|---|---|---|
| S88-1 scope the binding path | 3 | ✅ table-driven, with a checksum trap |
| S88-2 deliver the H2 increment | 3 | ✅ dump → edit → applied, verified |
| S88-3 gates + H3 docs | 2 | ✅ |

## Execution log

### S88-1 — how bindings actually work — DONE
Key handling is **table-driven**: `keytests::Reg3dConv(FIL_3D_KEYBOARD_TABLE)` loads a
`KeyMapping[]` out of game file 0x7501 and fills `KeyMap3d::mappings[scancode][shiftstate]` with an
action value; `STUB3D::OnKeyInput` looks the pressed scancode up there. So a rebind is one array
store — the difficulty is naming, not mechanism.

**Two things the scoping found that shaped the design:**
- **`Reg3dConv` checksums the table it loads** and calls
  `EmitSysErr("Key table has changed between loads???")` when two loads disagree. So overrides must
  **not** modify the loaded table; they are applied *after* the game's own load, straight into the
  live `mappings` array, where the checksum never sees them.
- **The actions already have names.** `KEYMAPS.H` carries a 177-entry `KeyName(index, NAME)`
  X-macro list (`ELEVATOR_BACK`, `ROTLEFT`, `RESETVIEW`, …) defining each `KeyVal3D` as `index*2` —
  exactly the value stored in `mappings`. Extracted to `SRC/compat/ma_keyactions.inc` with a
  regenerator (`port/gen_keyactions.py`) so the config file speaks the game's own vocabulary
  instead of magic numbers.

### S88-2 — the increment — DONE
`SRC/compat/ma_keybind.cpp` + a call after each `Reg3dConv` site:
- **`MA_DUMP_BINDINGS=1`** writes every live binding as `ACTION = 0xSC[, shift]` — **615 bindings**.
- The same file is read at startup and applied — **576 named bindings** (the remainder are actions
  with no `KeyName` entry, emitted as comments and honestly labelled).
- `MA_CONTROLS=<path>` relocates the file; `MA_TRACE_KEY=1` logs each applied binding.

**Verified end to end:** dumped, edited `RESETVIEW` from `0x01` to `0x0F`, re-ran →
`[keybind] RESETVIEW -> scancode 0x0F shift 0 (action 130)`.

**A bug in the first cut, worth recording because it defeats the whole feature.** The dump wrote the
shift state as a *following comment* (`AROTDNLEFT = 0x02` / `#  ^ shiftstate 2`). Reloading that
file would bind every shifted action at **shift 0** and clobber whatever else lived there — the dump
was not round-trippable, i.e. feeding back the file the tool itself produced would have silently
corrupted the user's controls. Shift is now on the line. **A dump that cannot be fed back is not a
bindings file**, and the only reason this surfaced is that the round-trip was actually run rather
than assumed.

**Default behaviour is unchanged:** with no `controls.cfg` present (the install has none) the game
uses its own table exactly as before — which is what keeps the parity/stress/ASan gates honest.

### S88-3 — H3 docs — DONE
`RUNNING.md` gains a "Rebinding keys" section: the three-step recipe, the file format, the env vars,
and *why* overrides are applied after the load rather than to the table.

## Gates — all under `gl-lock`
- **2D parity 5/5 byte-identical. OOB sweep 9 OPEN / 0 CRASH. Stress 20/20 PASS.**
- **ASan: FAILED FIRST, then passed.** The new TU was added to `CMakeLists.txt` but not to
  `port/rebuild.sh`, which is what the ASan build uses — so `/tmp/wmig-asan` failed to link with
  `undefined reference to ma_keybind_apply` while the primary Ninja build was green. Exactly the
  divergence a second builder exists to catch, and a reminder that **this tree has two build
  systems and a new file must be added to both**. Fixed in `rebuild.sh`; **re-run: 0 reports, all 4 paths 2/2.**

## Result
H2 is delivered in its useful form: a player can see exactly what is bound, in the game's own action
names, edit a line, and have it take effect. Remaining H2 polish (a bindings UI on the Controls tab)
is a separate, larger story.
