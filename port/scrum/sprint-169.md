# Sprint 169 — "The dialogs belonged to someone else's screen" (PO-51) — ✅ CLOSED 2026-08-22 (goal MET, 8/8)

**Planned 2026-08-22** (PO ceremonies pre-approved). The PO: *"fix PO-51 so the frag screen isn't
covered."*

| Story | Pts | Result |
|---|---|---|
| S169-1 why do map dialogs draw over a full-screen panel? | 3 | the **global control pass**, not the map branch |
| S169-2 fix it with the mechanism that already exists | 3 | ✅ `ma_ole_set_parent_scoped`, the S97 route |
| S169-3 get the frag screen clean | 2 | ✅ `Viper` / `Rattler`, pilot roster, `Map Fly Preferences` |

## The finding

The idle's map and panel branches are a proper `if (_fp) … else if (currentpage == 0) …`, so when the
frag pane launched **the map itself was correctly gone**. What remained were the campaign dialogs'
still-hosted **controls** — `ma_ole_draw_all`, the global front-end pass, composites every visible
hosted control it knows about, and nothing had told it those belong to another screen.

S97 built the mechanism for exactly this when the map *chrome* was drawing on the title screen:
`ma_ole_set_parent_scoped(dialog)` marks a dialog as composited by the parent-scoped path only. The
toolbars were registered; the **dialogs never were**.

**Scoping only the painted nodes left a second layer of residue** — Route's `S. Wonju / Position /
Altitude ft / ETA` columns were still drawn across the frag screen. Those nodes hang off **`dchild`**,
and the paint recursion follows `fchild`/`sibling` only, so they are never painted by the OOB walk
*and* are still hosted. The scoper therefore walks **`fchild`, `dchild` and `sibling`** — deliberately
wider than the paint walk: **a node the OOB walk does not paint has no business being drawn by the
front-end pass either.**

## Result

The frag screen renders its own content and nothing else: `F80 1 (05:11) Reconn` with callsign
**Viper**, the `I.D. / Wave / Role` headers, two `F80 1 (08:11) Bomb` rows with callsign **Rattler**,
the full pilot roster (E. B. Best, John Fox, Arnold Eagleston, John C. Marsh, Stanton G. Preston,
Robert V. Kratt, Allen McElroy, Harry T. Raebel, Sam P. Colman, Charles A. Mitchell, James D. Brown),
`Return to Player`, and `Map Fly Preferences` on the bottom bar. **No campaign-map residue at all.**

Still open on that screen and unchanged by this sprint: **PO-37**, the panel occupies an ~800×600
corner of a 1920×1080 canvas instead of filling it.

## Gates

`parity_2d` 5/5 byte-identical · `oob_sweep` OPEN=9 NONE=0 CRASH=0 · `authorize_mission` PASS ·
`damage_elements` PASS · `dialog_scroll` PASS · `map_filter` PASS · `help_click` PASS ·
`sysbox_exit` PASS (99.1 %) · `map_icon_click` PASS.

The whole set matters here: this changes which pass draws a dialog, and every one of those gates
asserts on a dialog being drawn.
