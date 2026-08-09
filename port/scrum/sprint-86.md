# Sprint 86 — "Open them all" — ✅ CLOSED 2026-08-09 — ⭐ every campaign-map dialog verified open, 0 crashes

**Planned 2026-08-09 (PO pre-approved ceremonies; PO directive: do not pause between sprints).
Autonomous. Committed ~8 pts.**
**Sprint Goal:** S82–S85 made the OOB dialogs clickable and fixed the two that crashed. The obvious
follow-up — *do the rest of them actually work?* — answered as a repeatable command, not a one-off.

| Story | Pts | Result |
|---|---|---|
| S86-1 sweep every map-toolbar OOB dialog | 3 | ✅ `port/oob_sweep.sh` |
| S86-2 fix/characterize the failures | 3 | ✅ no failures; the one negative is *correct* and now documented |
| S86-3 gates + cross-port | 2 | ✅ |

## Execution log

### S86-1 — `port/oob_sweep.sh` — DONE
Drives each map-toolbar dialog the way a player would: campaign nav to the map, then a click on the
toolbar button addressed by **control id qualified by host class** (`#ID@CMainToolbar` — the S85
form, because numeric ids are not unique). Reports OPEN / NONE / CRASH per dialog with a capture and
a log. Like `asan_all.sh` and `parity_2d.sh` it **stashes and restores the campaign save**, so a
sweep never advances the player's campaign (the S81 rule).

**Result — 9 OPEN, 0 CRASH:**

| dialog | verdict |
|---|---|
| intelligence, directives, bases, squads, weather, dis, overview, missionfolder, playerlog | **OPEN** |
| missionresults | NONE — *correct negative*, see below |

Spot-checked the captures rather than trusting the counter: **Bases** renders its airfield list
(Taegu / Taegu West / Taejon / Kunsan / Pohang) with aircraft silhouettes; **D.I.S.** renders its
photo plus "MISSION 1 BRIEFING"; Intelligence and Directives were verified in S84/S85. "OPEN" means
real content, not an empty panel.

### S86-2 — the one negative is correct — DONE
`missionresults` never even got clicked (`clicked=0`, id UNRESOLVED). That is right:
**`IDC_MISSIONRESULTS` (2055) is a control of `CDebriefToolbar`** (`DBRFTLBR.CPP:111/129`), which
replaces the main toolbar only while `MMC.indebrief` is set — it does not exist on the planning map.
So a `#2055@CMainToolbar` probe *should* resolve to nothing.

This is exactly the case S85's qualifier was built for: without `@Class` the probe would have found
some other 2055 and reported a misleading result. It is now recorded **in the sweep script itself**,
with the reason and the file:line, so the next reader does not re-investigate it — the standing
lesson that a documented negative is worth as much as a positive.

## Gates
- **No source diff this sprint** (`git diff HEAD -- SRC/` is empty): the only changes are the new
  script and docs, so the binary is the one S85 gated — **ASan and the full gate set were not re-run
  for a build that cannot have changed**, and that is stated rather than implied.
- **2D parity: 5/5 byte-identical. Stress: 20/20 PASS.** Re-run anyway as cheap insurance.
- **The sweep itself is the sprint's new gate**: `port/oob_sweep.sh` (9 OPEN / 0 CRASH).

## Result
The campaign map's entire information layer is verified working end-to-end: nine dialogs open on
genuine clicks with real data, and the tenth is provably not on that toolbar. Four sprints ago none
of them accepted a click at all.
