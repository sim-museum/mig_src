# Sprint 16 — ASan flight-path grind: eliminate the high-frequency heap corruptors

**Goal:** with the oracle on the 3D flight path (Sprint 15), fix the remaining **high-frequency**
heap corruptors it surfaced.
**Outcome: DONE.** The three remaining high-frequency bugs are fixed + verified gone. Across
S15+S16 the **five** per-frame corruptors (~31.7k invalid heap ops per flight) are eliminated;
what's left under ASan is only low-frequency singletons (1–3× per flight). No production regression.

## Fixed + verified (each count → 0)
- **`3dcom.cpp:10386` `shape::dodigitdial`** (7510×/flight): `digits = new UByte[nodigits]` freed
  with scalar `delete` → `delete[]`.
- **`worldinc.h:708` `mobileitem::operator delete`** (7510×/flight): the custom
  `operator delete(void* obj) {::delete(MovingItemPtr) obj;}` used a *delete-expression* to free,
  which re-ran `~MovingItem` on a base sub-object that `delete ip` (`Viewsel.cpp:8161`) had **already
  destructed** — a double-destruction. Changed to `{::operator delete(obj);}` (free the raw memory
  only; the dtor chain already ran). Exact **BoB R1.3d/e** pattern.
- **`Landscap.cpp:730` `LandScape::ManageHighLandTextures`** (139×/flight): `droppedTextures = new
  Dropped` is a **scalar** allocation but was freed with `delete[]` → plain `delete`. (The inverse of
  the usual array/scalar slip; same landscape-tile site family as BoB R3.9.)

## Method / notes
- All three are root-cause `new`/`delete` form mismatches (BoB's most common heap-bug class on this
  engine). Each fix is the minimal correct form, applied **byte-safely with `sed`** to preserve the
  files' original encodings (`3DCOM.CPP`/`WORLDINC.H` are ISO-8859 with high-byte license banners;
  the Edit tool re-encodes them to UTF-8 — a lesson from S15, where `3DCOM.CPP` got mass-re-encoded
  and had to be restored). `worldinc.h`→`WORLDINC.H` symlink; edit the real target.
- `WORLDINC.H` is a core header (included broadly) → full rebuild; compiled clean, no fallout from
  the `operator delete` change (signature unchanged).

## Validation
- ASan flight: `dodigitdial` 7510→**0**, `mobileitem::operator delete` 7510→**0**,
  `ManageHighLandTextures` 139→**0**; flight still reaches the rasterizer (`Blt #30` dumped).
- **Production stress 4/4** reached & sustained 100 3D frames — no regression.

## Remaining (Sprint 17 backlog — all low-frequency, `port/scrum/asan-findings.md`)
`LauncherToWorld` (heap-overflow ×3), `DoCloudLayer` (stack-overflow ×7) + `PerspectivePoly`,
`mobileitem … nationality` use-after-free (`worldinc.h:715`), `Reg3dConv` (**BoB R1.3b**, proven
fix), `Rchatter::InitROL`, `FixLbmImageMap` (×3). These fire once per flight, not per-frame — lower
severity. Alternative pivot: the operational-map colour fidelity (gold reference now in-repo).
