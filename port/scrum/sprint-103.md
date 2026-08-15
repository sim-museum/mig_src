# Sprint 103 — "The startup step nobody ran" (PO-8) — ✅ CLOSED 2026-08-15 (goal MET) — ⭐ preferences have never once been loaded

**Planned 2026-08-14 (PO pre-approved ceremonies; continuous-sprint directive). Autonomous. ~8 pts.**
**Sprint Goal:** the in-flight **info line** reads out aircraft state (PO-8), and PO-6 (map window
text) is driven and characterised now that the glyph path works.

| Story | Pts | Result |
|---|---|---|
| S103-1 root-cause `infoLineCount==0` | 3 | ✅ `SaveData::InitPreferences` was never called by the port |
| S103-2 make the info line draw, prove it | 3 | ✅ "Speed: 438Kts Mach: 0.73 Alt.: 16724ft Hdg: 279 Thrust: 0" |
| S103-3 PO-6: drive the map window, characterise | 2 | ✅ **M opens it; every text element is missing** — PO-6 localised |

## ⭐ The finding is bigger than the info line: preferences were never loaded, ever

`SaveData::InitPreferences` has exactly two callers: `MAINFRM.CPP` inside `#ifdef MIG_DEMO_VER`, and
the top of `RFullPanelDial::IntroSmackInit` — the intro-Smacker route. The port launches the title
front-end directly (Smacker is stubbed, E1), so **neither ever ran**. That function is not only the
game's default-setting code, it is also **the only reader of `settings.mig` in the entire tree**.

So for the port's whole life: `ma_save_preferences` faithfully WROTE the player's settings on every
exit, and nothing ever read them back. The game ran on a never-initialised `Save_Data`.
(The A2 backlog claim "reloaded on next boot" was never true; what S2 verified was an *in-process*
round trip — change a combo, switch tab, come back.)

**Three separate local patches had each treated one symptom of this one missing call**, none of them
asking who was supposed to set the value:
- `STUB3D.CPP`: `if (!Save_Data.alt.mediummm) Save_Data.SetUnits();` — the zeroed unit factors
  (a SIGFPE when the info panel was toggled on).
- `STUB3D.CPP`: `Save_Data.gamedifficulty |= GD_HUDINSTACTIVE` forced per flight — the HUD
  instruments "the port's loaded default left off".
- `MILES.CPP`: restore sound volumes when all three are zero — with a comment blaming a "stale
  all-zero settings.mig being loaded back", **which could not have been happening**, because
  nothing loaded settings.mig at all.

All three are retired this sprint. The last two were also actively wrong once preferences load: they
override a real player choice (mute the game, turn the HUD instruments off).

## Fix, in four parts

1. `MIG.CPP`: run `Save_Data.InitPreferences()` + `_Replay.TruncateTempFiles()` on the deferred-title
   path, mirroring `IntroSmackInit`'s order. `MA_NO_INITPREFS=1` reverts.
2. `SAVEGAME.CPP`: the settings stream's build-date guard (`date2 = "Rowan Savegame: " __DATE__`)
   threw the file away wholesale on any build compiled on a later day. The **campaign** stream had
   already been given exactly this treatment in an earlier sprint ("the save FORMAT is
   packing-stable across rebuilds, so this build-date guard is pure friction"); the settings stream
   never was, because nobody had noticed it was unreachable. Same decision, same override
   (`MA_ENFORCE_SAVE_DATE=1`).
3. `SAVEGAME.CPP`: a one-time migration for installs that ran the port before this sprint. Their
   `settings.mig` was written from a never-defaulted `Save_Data`, and now that the load works those
   zeros would become the player's preferences. Signature — **measured from this install's own
   pre-S103 file, not guessed**: `infoLine=0 keysens=0 targetsize=0 autopilot(0,0)`. The volumes are
   deliberately excluded: they are 125/125/64 there, because MILES.CPP's workaround patched them
   before the save. A guess of "all zero" would have missed the real file.
4. `port/parity_2d.sh`: pin `settings.mig` around every capture (new fixture
   `port/ref/save/settings_pristine.mig`), the same reasoning as the pinned campaign save. Until
   this sprint the Preferences screens were *accidentally* state-independent; now they render live
   player state, and an oracle that renders mutable state is not an oracle (S80).

## Evidence

| arm | `[prefs]` | `[infobar]` | frame |
|---|---|---|---|
| fixed, fresh install | `InitPreferences() ... infoLineCount=1` | `-> drawing` | **"select your own target! / Speed: 438Kts Mach: 0.73 Alt.: 16724ft Hdg: 279 Thrust: 0"** |
| `MA_NO_INITPREFS=1` | (not run) | `infoLineCount=0 -> EARLY RETURN` | no info line |
| pre-S103 `settings.mig` | migration fires, defaults rebuilt | `-> drawing` | info line present |
| second run after the first | `loaded settings.mig: ok=1 infoLine=1 keysens=2 vol(125,125,64) targetsize=1 autopilot(2,2)` | `-> drawing` | preferences **survive a restart** — a first for this port |

**A prediction that was wrong, and worth recording.** The sprint opened predicting that calling
`InitPreferences` would set `infoLineCount=1`. It ran, and the trace still said 0 — because the
function *ends* by loading `settings.mig` over its own defaults, and this install's file said 0.
The trace that printed the loaded values (`ok=`, and each field) is what turned a confusing result
into the real finding. *Print what was loaded, not just that you loaded.*

## PO-6 characterised (next sprint's work)

`M` (GOTOMAPKEY, DIK 0x32 — confirmed against the game's own binding dump) **opens the in-flight map
window**: map, route line, kneeboard panel, cockpit art all render. **Every text element is
missing** vs gold (`full` @ ~90 s): no "5:24 Koesan" clock/waypoint line, no waypoint table
(Rendezvous / Ingress / Initial Point with alt/time/heading/range), no right-hand command list
("1.NextWP=HighlightedWP", "2.AccelToNextWP", "0.Exit"). Note `DrawInfoBar` deliberately returns
early when `pCurScr==&mapViewScr`, so the map screen's text comes from its own drawing code, not
the info bar.

**PO-7 gets a sharper statement too:** the R tap IS delivered (`[keyseq] tap dik=0x13`) and R IS
bound to RADIOCOMMS in the game's own table, and nothing opens — while the same injection path
opens the map with M. So this is not key delivery: it is `KeyPress3d(RADIOCOMMS)` or
`SetToRadioScreen` itself. That is a much smaller haystack than "typing R yields nothing".

## Gates

parity **5/5 after rebaseline** (see below) · sweep 9 OPEN/0 CRASH · map click · map drag ·
sysbox exit · help click · stress 20/20 · ASan 0.

**Rebaselined `prefs_3d` and `prefs_others`** — and the reason is the fix, checked against gold, not
convenience: the only differing pixels are combo VALUES (`Gamma Correction` Minimum→**Medium**,
`Music Volume` Maximum→Medium, one more Off→Low). Gold shot #2 shows **"Gamma Correction: Medium"**,
so the new state is *closer* to gold, not further. The other three screens stayed byte-identical.

## Result

PO-8 **CLOSED**, and the port gained working preference persistence on the way — a defect nobody had
filed because saving worked and only the loading half was missing. The tell was three separate
local patches for three symptoms of one cause: **when the same subsystem needs a third workaround,
stop patching symptoms and find who was supposed to initialise it.**
