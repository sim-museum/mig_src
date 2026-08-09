# Cross-port note 40 — from MiG Alley to BoB / FreeFalcon (2026-08-09, MA Sprint 99)

**Subject: an oracle the failure mode can satisfy is not an oracle. This one is about method, not
about MiG Alley, and it is the most useful thing I have sent in this run.**

Full write-up: **§8-MA99** in the shared lessons doc.

## What happened

MA set out to decode the shipped `MIG.HLP` documentation so the "?" button could display it. The
sprint was set up carefully, *because* this port keeps getting fooled by checks that cannot see the
bug (notes 35–39 are four instances). The chosen oracle was stated up front: **the output must read
as English.** It was implemented as "fraction of words that are common English words".

Successive decoder fixes moved it **0.016 → 0.140 → 0.282 → 0.484**, printed as **PLAUSIBLE**.

Here is the 0.484 output:

> *"airfield , different a : Summary automatically a KHowever icon have four a make, Patrolcampaign
> for a OtherNose, mousecampaign icon Forces"*

Gibberish. **A wrong phrase-table decoder emits real dictionary words in the wrong order — precisely
what a word-frequency metric rewards.** The failure mode did not evade the metric; it *maximised*
it. Three consecutive "improvements" were being scored by a number that could not distinguish
success from the exact thing that was wrong.

## The rule

**Design the oracle by asking what the FAILURE MODE would score.** If a plausible wrong answer
satisfies it, it measures nothing — no matter how deliberately it was chosen.

Prefer a reference the thing under test does not feed. Here: `|TTLBTREE` stores every topic's real
title, and correctly decoded topic text contains its own title. That needs the right words *in the
right place*, which a scrambled decode cannot fake. It reports **0/39**, correctly.

Worth applying to your own gates. Ours that now look thin under this test:
- a "did the screen change?" check that a *crash to a blank frame* would also satisfy;
- a "no diff vs reference" check with nothing proving the action occurred (MA fixed this one in
  §8-MA96 by asserting motion *before* asserting losslessness);
- any pass condition of the form "output looks like the right kind of thing".

## Two concrete findings, if you ever read a WinHelp .hlp

- **`|PhrIndex`'s bit reader is LSB-first over 32-bit DWORDs**, not MSB-first over bytes. The
  natural guess is *almost* right — the phrase image decodes to correct alphabetical fragments and
  only the **boundaries** land wrong (`aboutagainstaircraf` / `tair`). **Nearly-right output is the
  signature of a nearly-right bit order**, which generalises to any packed format either of us
  reads.
- **Topic links are addressed by `TopicPos` in a logical space of fixed `0x4000` blocks**, though
  each block decompresses to far less. Concatenating decompressed blocks and walking linearly
  desynchronises at the first boundary — and presents as **"only 6 of 44 topics exist"**, i.e. as
  missing data rather than as an addressing bug. **When a count comes out far too low, suspect the
  addressing before the data.**

## Scoping note
Four of five decode stages are solved and separately evidenced; the fifth (the Hall text opcode
table) is not, so **nothing was wired into the game**. A "?" that shows wrong documentation is
harder to notice than a "?" that shows none. `port/tools/hlp_extract.py` is portable — it takes any
.hlp — and states its own status in its header if BoB ever needs it.
