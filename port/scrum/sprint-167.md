# Sprint 167 — "Logged children nest; stop hardcoding how deep" (K5/K8) — ✅ CLOSED 2026-08-22 (goal MET, 8/8) — ⭐ the TASKS dialog AND the ROUTE dialog were being built and never painted

**Planned 2026-08-22** (PO ceremonies pre-approved). The PO: *"keep going on PO-50 so I can run the
mission."* PO-50 itself closed in S165; this sprint takes the next link in the same chain.
**Sprint Goal:** the dialogs the mission builder opens actually appear.

| Story | Pts | Result |
|---|---|---|
| S167-1 why does `Task` open nothing? | 3 | ⭐ a **third** level of logged children that neither walk descends |
| S167-2 stop hardcoding the depth | 3 | ✅ both walks recurse, as mirror images |
| S167-3 see what that reveals | 2 | ✅ **TASKS** and **ROUTE** — two of the PO's script steps |

## The finding

S166 left `Task` firing and no dialog appearing, with two candidates. Reading the code settled it
without a run: `CProfile::MakeTaskTabs` does `LogChild(0, MakeTopDialog(… new CTaskTabs))` — it logs
the TASKS dialog **on itself**. And `CProfile` is already a logged child of the Mission Folder, which
is a logged child of the toolbar. That is **three levels**, and both walks handled exactly two:

| level | added by | for |
|---|---|---|
| toolbar → dialog | S106 | the MISSION RESULTS panel |
| dialog → dialog | S142 | the D.I.S. briefing, *"created on every open and never painted"* |
| dialog → dialog → dialog | — | **the TASKS and ROUTE dialogs** |

Each level was added the hard way, years apart, by whichever feature needed it. **A `ggslot` loop
would have worked and would have left the fourth level for the next feature to discover.** Both
walks now recurse over logged children to any depth, written as mirror images of each other —
`ma_oob_paint_logged_rec` and `ma_oob_click_logged_rec`, same recursion, same dedup, children first
on the click side because children are painted last and therefore sit on top (S82).

`seen` does double duty: it keeps a dialog reachable through several slots from being painted twice
(S123 — repeated painting darkens a translucent panel until one dialog reads as several) and it
stops a cycle in the logged-child graph from recursing forever, with a depth cap as a second belt.

## What appeared

Two dialogs that the port has been **building on every open and never showing**:

- **TASKS** at depth 2 — tabs `Bomb / AAA Cover / Air Cover`, a `Squadron` combo, and `Flight 1…4`
  rows each with a stores combo and a `Target` combo. That is the gold's t≈155 frame, and it is
  where the PO's script steps 8–12 happen.
- **ROUTE** at depth 2 — `Main Route / Target Zone` tabs, `Summary / Detail`, and the mission's real
  waypoint list: `1. Take Off Taegu Airfield · 2. Rendezvous N.E. Kumi · 3. Ingress Chomchon ·
  4. Initial Point S. Wonju · 5. Regroup W. Wonju · 6. Egress S.W. Inchon · 7. Disperse W. Kimchon ·
  8. Landing Taegu Airfield`. That is script step 13, and the Initial Point the PO drags is in it.

Neither needed a fix of its own. Both were correct all along and unreachable.

## Gates

`parity_2d` 5/5 byte-identical · `oob_sweep` OPEN=9 NONE=0 CRASH=0 · `authorize_mission` PASS ·
`damage_elements` PASS · `dialog_scroll` PASS · `map_filter` PASS · `help_click` PASS ·
`sysbox_exit` PASS (99.1 %) · `map_icon_click` PASS.

`oob_sweep` is the one that matters: this change makes the paint walk visit strictly more dialogs,
and the fastest way to break the campaign map would be to paint one of them twice or forever.

## Next: FRAG

`Frag` on the Mission Folder fires and `CMainToolbar::OnClickedFrag2` runs, but no frag panel
appears. Its two branches — *"not flyable"* → a message box, and *"flyable"* →
`LaunchFullPane(&RFullPanelDial::singlefrag)` — **look identical from outside in a headless capture:
the map simply stays on screen.** `MA_TRACE_FRAG=1` now prints which, once. Instrumented rather than
inferred, because inferring from absent pixels is precisely the mistake S164 made.
