# Sprint 160 — "Photo" (EPIC K, K1 + K2) — ✅ CLOSED 2026-08-21 (goal MET, 8/8) — ⭐ the 3D recon of the Wonju Supply Dump renders natively

**Planned 2026-08-21** (PO ceremonies pre-approved), continuing the Wonju walkthrough in script order.
**Sprint Goal:** steps 4 and 5 of the PO's script — find the Wonju Supply Dump on the map, open its
dossier, and get the 3D recon from the Photo button.

| Story | Pts | Result |
|---|---|---|
| S160-1 address a map item by the name the game shows for it | 3 | ✅ `MA_MAP_CLICK_NAME=Wonju` — the dump is `id=9801 (0x2649)`, `AmberSupply` |
| S160-2 make Photo work | 3 | ⭐ the sim thread was started ~40 lines before the landscape cache it reads |
| S160-3 a gate that keeps it working | 2 | ✅ `port/recon_photo.sh`, with its negative control checked |

## K1 — finding the target

S158 could ask the map for a *class* of item. K1's acceptance criterion names one:
*"look north of it for the Wonju Supply Dump icon"* — and this save carries **twenty** `AmberSupply`
items, so a band cannot express it. The scan now reports each item's name through **the game's own
`GetTargName`** (the call `DossierButtons::OnClickedPhoto` uses for the photo caption), and
`MA_MAP_CLICK_NAME=<substring>` selects by it:

```
[mapitem] (618,0)   id=9793(0x2641) band=0x2500 AmberSupply  "Sakchu Supply + Comms Center"
[mapitem] (756,300) id=9798(0x2646) band=0x2500 AmberSupply  "Sukchon Warehouses"
[mapitem] (738,204) id=10206(0x27de) band=0x2700 AmberBridge "Taeryong Road Bridge"
…
[mapitem] name match "Wonju Supply Dump" -> id=9801(0x2649) band=0x2500 AmberSupply at (1320,948)
```

The dossier it opens matches the PO's script *on content, not just layout*. The script predicts
**"no MiGs expected, but a large AAA presence"**; the port's dossier reads **Threat AAA: High,
MiG 15: Low**, MSR **Central** (the script: *"north of the … Central Front Line icon"*), Activity
High, Repairs Operational. That is the first time this port has been checked against the gold's
*claims about the game world* rather than its pixels.

A name that matches nothing **clicks nothing and says so** — it does not fall back to a band or to
"the first item". A recipe that silently addresses something else is S85's failure mode.

## K2 — the finding

Driving Photo with 3D enabled left the game with no further idle output. `MA_DISABLE_3D=1` — which
every headless gate we own sets — showed the photo dialog with its loader art and no 3D at all, so
nothing we had could see this.

`ptrace_scope=1` blocks attaching to a non-descendant, so the process was run **under** gdb
(`timeout -s INT 240 gdb -batch -ex run -ex "thread apply all bt"`). That gave it away at once:

```
Thread 11 "wmig" received signal SIGSEGV
Thread 11:  #0 Inst3d::moveloop(void*)
Thread 1:   #3 CRectangularCache::CRectangularCache(unsigned short)
            #4 CMigLand::CMigLand()
            #5 ThreeDee::InitialiseCache()
            #6 Inst3d::Inst3d(bool)
            #7 Rtestsh1::Launch3d(bool)
```

**The worker had already crashed while the main thread was still in the constructor**, building the
landscape cache the worker reads.

`Inst3d::Inst3d(bool)` — the map-view ctor, the one the dossier's Photo takes — starts
`AfxBeginThread(moveloop, this, …)` and only *afterwards* sets `mapview`, `world`, `viewedwin`,
`livelist`, `Master_3d.currinst` and calls `Three_Dee.InitialiseCache()`. The game's own comment sits
nine lines below the thread start and says *"at this point the thread starts receiving timer
messages"* — it does not; it started already.

**This bug was found and fixed a year of sprints ago — in the other constructor.** The no-argument
`Inst3d::Inst3d()` twin, 100 lines below, carries an `#ifndef MA_LINUX` guard and a paragraph of
S69 commentary describing exactly this race (found when an AppImage's squashfs slowed the ctor's
I/O enough for the worker to win). The fix never crossed the gap between two near-identical
functions. S160 applies the same deferral: the thread now starts at the end of the ctor.

> **The lesson, and it is a repeat:** when a fix is a *reordering inside a constructor*, look for the
> constructor's twins before closing it. The dead-code line above both starts says the original used
> `CREATE_SUSPENDED` and had no race at all — whoever switched it to start-immediately introduced it
> in both, and S69 only closed one.

After the fix the same drive shows no SIGSEGV; the main thread sits in `bob_msg_wait` (the normal
message loop) and the sim thread paces in `ma_sim_pace`. `MA_DUMP_BACK` then yields the recon frame:
warehouse rows, vehicles, roads, terrain, the recon toolbar, and the caption **"Wonju Supply Dump"**
— the scene the gold shows at t≈65–72.

## The gate, and the measure that was wrong first

`port/recon_photo.sh` asserts four things, because any one alone passes for the wrong reason: the
target is found **by name** (not "the first item", which on this save is a bridge); the Photo button
takes the click; a 3D frame is produced at all; and the frame is a **rendered scene**.

The first version of that last test asked for **>2000 distinct colours** and failed the good frame.
The software rasterizer is **8-bit palettised** — it cannot produce more than 256, ever. The test now
asks for ≥64 distinct colours with no single colour over 70 % of the frame (the recon frame measures
**193 colours / 31.8 %**; a black or flat frame is 1–2 colours at ~100 %). Same family as S64's rule:
**measure something the renderer can actually produce.**

Negative control checked: with `MA_DISABLE_3D=1` the gate reports *"no 3D frame was produced — FAIL"*.
A gate that has never been seen to fail is not evidence.

## Gates

| Gate | Result |
|---|---|
| `port/recon_photo.sh` (new) | **PASS** — 193 colours / 31.8 % top; FAILs under `MA_DISABLE_3D=1` as designed |
| `port/stress_launch.sh` | **20/20** reached and sustained 100 3D frames |
| `port/parity_2d.sh` | 5/5 byte-identical |
| `port/map_icon_click.sh` | PASS |

`stress_launch` is the one that matters here: this sprint moved a thread start inside a 3D
constructor, and that gate is the one Phase 5.1 built for exactly that class of change.
