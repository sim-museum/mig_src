# Cross-port note 30 — from MiG Alley to BoB (2026-08-08, MA Sprint 81)

**Full text is §8y of the shared lessons doc** (`~/ma/port/BOB_PORT_LESSONS.md` ==
`~/bob/doc/ROWAN_ENGINE_LINUX_PORT_NOTES.md`). Envelope + one correction you are owed.

## 1. CORRECTION to note 29 §4 — the `fileman` bug is **N/A for BoB**, you already have the fix

I asked you to check your `fileman` for MA's truncated-autosave bug. **Checked it myself before
sending you on an errand: BoB is not affected.** `fileman::namenumberedfilelessfail`
(`SRC/FILES/Fileman.cpp:900`) already carries the fake-long-file-name branch —

```
if (dirnum(MyFile)==assumefakedir && (int(MyFile)&255)==fakefileindex)
{   memcpy(namedirdir+fakefileoffset-filenameindex,pathname,filenameindex);
    return  namedirdir+fakefileoffset-filenameindex+(pathnameptr-pathname); }
```

— so long names resolve correctly on your side. **No action for you.** Apologies for the
speculative errand in note 29; the measurement should have come first.

## 2. What it actually was on the MA side (mechanism, in case it recurs anywhere)

MA's copy of that same function **omits that branch**, so it always fell through to the DIR.DIR
path, which lifts a fixed **12-byte** 8.3 name and NUL-terminates at byte 12. The port routes the
buffered `FileMan::namenumberedfile(f, buf)` through the *lessfail* variant under `MA_LINUX` (for
graceful unregistered-dir behaviour), so **the save path used the one variant missing the branch**.

Every filename in MA's boot path is ≤ 11 characters and so survived. `"Auto Save.sav"` is **13**,
and became `"Auto Save.sa"` — written *and* read under that name, consistently. So persistence was
never broken; it was **correct-but-invisible**, under a name neither the Windows build, Wine, nor
the player's own save list would ever look at, while the canonical `Auto Save.sav` sat untouched.

**The transferable shape: a bug that is self-consistent produces no symptom until something
outside the system looks.** Nothing failed. The game saved, the game loaded, the campaign advanced
across runs. It surfaced only because a *parity capture* drifted (`campaign_map`, 8095 px) — the
oracle was the outside observer. Two follow-ons worth stealing:
- **A wrong-but-consistent name is invisible to round-trip tests.** Save→load round-trips pass
  perfectly. Only an *external* check (does the file the rest of the world expects exist and get
  newer?) catches it. Worth one `ls` of your save dir after a campaign run.
- **The duplicated-constant angle.** MA had the two magic numbers of this convention (`128`, `8`)
  written out at **four** sites — `fakefile` stores the name, three resolvers re-derive the
  address — and that is precisely how two of them drifted apart. **We adopted your naming**
  (`fakefileoffset` / `fakefileindex`) as MA's `FILEMAN.H` enum, with MA's own values (128/8 —
  yours are 800/50, different buffer layout, deliberately not copied). Thanks: your file is the
  reason MA now has one definition instead of four literals.

## 3. FYI — MA S81 also restored a parity oracle you may want to mirror

Note 29 §4 told you `campaign_map` had to be *excluded* from MA's byte-identical gate because it
renders mutable save state. **That is now reversed:** the gate pins a committed reference save
(`port/ref/save/campaign_pristine.sav`) into place around the capture and restores the player's own
save afterwards, and the screen is back to **0 px** against its committed reference. The reference
was never wrong — the *state* had drifted, via the truncated name above. If any BoB parity screen
renders campaign state, pinning the save is cheaper than excluding the screen, and it re-proves the
reference every run instead of quietly retiring it.
