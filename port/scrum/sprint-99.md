# Sprint 99 — "Getting the words out" (PO-4 cont.) — ⚠️ CLOSED PARTIAL 2026-08-09 — 4 of 5 decode stages solved; ⭐ the oracle lied and said 0.484 PLAUSIBLE about gibberish

**Planned 2026-08-09 (PO pre-approved ceremonies; continuous-sprint directive). Autonomous. ~8 pts.**
**Sprint Goal:** finish PO-4 by extracting the shipped documentation from `English/TEXT/MIG.HLP`
and showing it, now that S98 routes the "?" click all the way to `WinHelp`.

| Story | Pts | Result |
|---|---|---|
| S99-1 decode the .hlp topic text | 6 | ◐ **4 of 5 stages solved and verified; 1 unsolved** |
| S99-2 in-game viewer | 2 | ⬜ **not started** — correctly blocked on S99-1 |

## Result up front
**PO-4 is still open, and the "?" still shows nothing.** Four of the five decode stages are solved
and independently verified; the fifth is not, and nothing was wired into the game. The tool ships
with its own status written into its header and a `--verify` that says *WRONG* today.

## What is solved — each with its own evidence, not an impression
| Stage | Evidence it is right |
|---|---|
| container / internal-file B+ tree | 11 files, names exactly as the format specifies |
| LZ77 | `\|PhrImage` decompresses to a clean alphabetical word list |
| `\|PhrIndex` bit reader | **732 phrases with exact boundaries**: `, . : a about against aircraft airfield airfields allocate…` |
| `\|TOPIC` link chain | 43 topic headers found; `\|TTLBTREE` lists 44 |

Two of those were real finds:

- **The bit reader is LSB-first over 32-bit DWORDs**, not MSB-first over bytes. The natural guess
  produces a table that is *almost* right — the phrase image decodes into correct alphabetical
  fragments and only the **boundaries** land wrong (`aboutagainstaircraf` / `tair`). Nearly-right
  output is the signature of a nearly-right bit order.
- **Topic links are addressed by `TopicPos` in a logical space of fixed 0x4000 blocks**, even
  though each block decompresses to ~5 KB. Concatenating the decompressed blocks and walking
  linearly desynchronises at the first block boundary — which presented as **"only 6 of 44 topics
  exist"**, i.e. as missing data rather than as an addressing bug.

## ⭐ The lesson: an oracle the failure mode can satisfy is not an oracle
The tool was written with a deliberate oracle — "the output must read as English" — implemented as
the fraction of common English words. Tuning the decoder took it from 0.016 → 0.140 → 0.282 →
**0.484**, printed as **PLAUSIBLE**. The text at 0.484:

> *"airfield , different a : Summary automatically a KHowever icon have four a make, Patrolcampaign
> for a OtherNose, mousecampaign icon Forces"*

It is gibberish. **A wrong phrase decoder emits real dictionary words in the wrong order — which is
precisely what a word-frequency metric rewards.** The metric did not merely fail to detect the
failure; the failure mode *maximised* it. I was three "improvements" deep into a number that could
not have distinguished success from what I had.

Replaced with an oracle the failure mode cannot satisfy: **`|TTLBTREE` holds every topic's real
title, and correctly decoded topic text contains its own title.** That requires the right words in
the right place, and it reports **0/39** today — correctly.

*This is the fifth time this port has been fooled by a check that could not see the bug
(§8-MA83, S64→S65, §8-MA93, §8-MA96, and now here) — and the first where the check was one I had
designed as the safeguard.*

## What is not solved, precisely
The **Hall opcode table** for the text stream. The current guess (1-byte `c&1` → phrase `c>>1` plus
a space; 2-byte `c&2` → phrase `(c>>2)+(next<<6)`; `c&4` → spaces; `c&8` → literal run) is wrong.
The known-plaintext attack via the topic header did not settle it either: a `TOPICHEADER`'s `data2`
is structured (`09 21 07 e4 2b 00 00 00 10 00 00 00 0c` — the tail is two int32s), not the title in
phrase form, so it is not the crib it looked like.

It is kept behind **`--hall-guess`**, documented as unsolved, so the next attempt has something
concrete to disprove rather than starting from nothing.

## Gates
**No game code changed this sprint** — the only additions are `port/tools/hlp_extract.py` and its
documentation. The gate results committed at S98 therefore stand unchanged (parity 5/5, sweep
9 OPEN/0 CRASH, map click, map drag, sysbox exit, help click, stress 20/20, ASan 0).

## Result
A sprint that did not deliver its feature and is worth more than one that did. The decoder is four
fifths built with each stage independently evidenced, the remaining unknown is named to the byte,
and the sprint caught a safeguard of my own design agreeing enthusiastically with nonsense. **Design
the oracle by asking what the failure mode would score** — if the wrong answer can satisfy it, it
measures nothing.
