# Sprint 161 — "Synced is not processed" (cross-port debt) — ✅ CLOSED 2026-08-21 (goal MET, 8/8) — ⭐ S159 rediscovered a bug that was already written down in our own tree

**Planned 2026-08-21** (PO ceremonies pre-approved). Taken ahead of the next EPIC K step on the
standing rule *"ship the finding that helps someone else before the one that only helps your
burndown"* — and then it turned out the debt had already cost a sprint.

| Story | Pts | Result |
|---|---|---|
| S161-1 answer the unprocessed inbound BoB notes | 3 | ⭐ **BoB181 described S159's PO-49 exactly, and had been sitting in MA's tree** |
| S161-2 stop it happening again | 3 | ✅ `§8-LEDGER` — a per-note verdict table, with MA's own unassessed rows named |
| S161-3 say what `port/ref/native/` actually is | 2 | ✅ 5 oracles, 50 undated snapshots, several showing bugs we have since fixed |

## The finding

S159 spent a sprint discovering that MA's campaign dialogs blit their background bitmap at the
**bitmap's** size with no window to clip it — a 540×602 backdrop in a 327×316 dialog, on nine of nine
campaign dialogs.

**§8-BoB181 says exactly that, and it was already in `port/BOB_PORT_LESSONS.md`:**

> *"A port with no windows draws into one screen-wide DC, so it sets an origin and no clip, and the
> whole sheet lands on the screen … wherever the port substitutes a screen-wide DC for a window, it
> inherits the window's clipping responsibility."*

The sync was never the problem: the file is byte-identical in both trees and a guard proves it every
sprint. **Syncing was being mistaken for processing.** A note arrives, the file matches, the box is
ticked — and nobody asks *"does this one describe something in MY tree?"* Between S157 and S159,
three BoB notes (181, 182, 183) sat here unanswered while MA rediscovered one of them from a
play-test defect.

Cost: a sprint. Not a wasted one — S159's fix is measured, gated, and found nine dialogs where the PO
reported one — but a sprint that a fifteen-minute read would have *started* with the answer.

## The structural fix

`§8-LEDGER` in the shared doc: one row per note, one verdict per port — **applied** (with the
sprint), **N/A** (with the reason, checked rather than assumed), or **open** (with the blocker). A
note with no row is unprocessed by definition, and "we synced it" cannot fill the row in.

It is populated honestly, which means **MA's own gaps are now visible in it**: `§8-BoB173`,
`§8-BoB173d` and `§8-BoB180b` are marked *not yet assessed*. Naming them is the point of the table;
they are the top of the next cross-port slot.

*The generalisable half:* an inbound artefact needs a **per-item** verdict, not a per-batch one — the
same shape as S83's "a search that finds nothing is only as trustworthy as its pattern". A
batch-level "done" hides every item-level miss inside it.

## The two verdicts delivered (§8-MA110)

**§8-BoB182 (a stub returning SUCCESS hides its own gap) — N/A, by the opposite asymmetry.** MA has
the identical `ChangeDisplaySettings` stub, character for character. BoB was bitten because it had
implemented the *enumerate* half and not the *apply* half. MA has implemented **neither** —
`EnumDisplaySettings` returns `FALSE`, so `Win3d.cpp`'s mode-search finds nothing and the
`CDS_FULLSCREEN` call is never reached. And here declining is **correct**: the sole caller switches
the *desktop* to 640×480 for a 1999 full-screen game, and this port owns its own window and
resolution. What the stub owed its reader was to *say* so — added under `MA_TRACE_STUB=1`. BoB's
rule survives the N/A: "success" was still the wrong thing to say silently.

**§8-BoB183 (a control outside the walk's collection) — N/A, already closed.** Same defect, reported
by the PO as *"no way out of the campaign map"* (PO-1, S97). MA's paint walk and click walk enumerate
the same two toolbars and S106 added the second to both in one edit — which is BoB's stated rule,
confirmed in practice: the one time this port shipped the paint-only half, the symptom was identical.

## Notes sent (MA 107–110)

- **§8-MA107** ⭐ a note that is synced is not a note that is processed — and the ledger.
- **§8-MA108** ⭐ **two constructors, one fix**: S69 fixed the `Inst3d` sim-thread race in one ctor and
  its map-view twin raced on for 90 sprints (S160). *When a fix is a reordering inside a constructor,
  find the constructor's twins before closing it.* Includes the technique: `ptrace_scope=1` refuses
  `gdb -p` on non-descendants, so run the program **under** gdb and let a timeout interrupt it —
  `timeout -s INT 240 gdb -batch -ex run -ex "thread apply all bt" --args ./wmig`. BoB is told plainly
  that its own `Inst3d` ctors start no move thread, so the bug is MA-only and only the rule travels.
- **§8-MA109** measure something the renderer can produce — the ">2000 distinct colours" test that
  failed a good frame from an 8-bit palettised rasterizer.
- **§8-MA110** the two verdicts above.

Both copies re-synced byte-identical (`port/BOB_PORT_LESSONS.md` == `bob/doc/ROWAN_ENGINE_LINUX_PORT_NOTES.md`).

## `port/ref/native/` — 5 oracles and 50 pictures

BoB181's corollary is *"a gate pinned to the defect fails when you fix it"*. MA's version turned out
to be quieter: **nothing here is pinned to anything.** `parity_2d.sh` is the only consumer of this
directory and it uses **five** of the 55 files. The other fifty are snapshots taken to illustrate one
sprint's result, with no gate, no recorded recipe and no expiry — and several are no longer true:

- the `1021×644` / `1021×777` captures predate the canvas-overhang fix, and **that size is itself the
  bug** (an overhanging blit used to grow the canvas instead of being clipped);
- every campaign-dialog snapshot from before today shows the **unclipped** backdrop art S159 removed.

They are **not** refreshed wholesale, deliberately: their recipes were never recorded, so a "refresh"
would be a new capture under a guessed recipe wearing an old file's name — worse than a dated
snapshot. Instead `port/ref/native/README.md` now states, per file, which five are oracles and what
the rest are. **If you need a reference to be true, put it behind a gate; an oracle that nothing runs
is a picture.**

## Gates

`parity_2d.sh` 5/5 byte-identical, `map_icon_click.sh` PASS, ninja clean (the compat header change
rebuilt 190 TUs).
