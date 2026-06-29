# Sprint 29 — "Compare notes" cross-port ASan hardening (R-hardening)

_2026-06-29 · Product Owner pre-approved planning + review._

## Sprint Goal
Compare notes with the sister BoB port and adopt every shared-engine memory-safety
fix from BoB's S46→S62 ASan arc that MiG Alley also carries — closing latent
heap/over-read bugs across the shared 1999 Rowan engine.

## Sprint Backlog & outcome

| Item | BoB origin | MA verdict | Result |
|---|---|---|---|
| `MathLib::rnd()` `rndlookup[55]` over-read | S55 | identical (`MATH.CPP:1722/1730`) | ✅ FIXED (`% table-size`) |
| compat `BITSET/BITTEST` dword→byte granular | S59 | identical (`mathasm_linux.h`) | ✅ FIXED (byte-granular) |
| `LBMCPP.H` IFF ByteRun1 unpack reads past file buffer | S47 | `LBMCPP.H` byte-identical, no guard | ✅ FIXED (`LBM_INBOUNDS`/`cend`; +no-op sentinel in the uncalled `UnpackRow`) |
| `CRListBoxCtrl` cell-string `new[]/delete` | S58 | shared, but at `DeleteRow:2145` (MA's `ReplaceString` was already correct) | ✅ FIXED (`delete[]`) + ASan differential-validated |
| `DrawSubShape`/`dodigitdial` shape-opcode `new[]/delete` | S49/S53 | opcodes absent from MA `3DCOM.CPP` | ❎ not shared |
| `g_devTex` UAF / `~View3d` teardown race | S60/S61 | DX7/Lib3D-specific; MA software path differs | ❎ not shared |
| `FindNextBf` `GR_Scram_*[8]` >8 groups | S54 | no `glind`/unbounded loop; bounded `<8` + 8 named refs | ❎ not shared |
| `LaunchScreen resolutions[-1]` | S57 | already guarded in MA (`FULLPANE.CPP:2037`, independent "ASan(MA)" fix) | ✅ pre-existing |

## Definition of Done
- ✅ All fixes compile clean into the unity/MFC/OLE build set; full rebuild + link OK (8.7 MB ELF).
- ✅ Headless boot exercises the startup palette/image path (`InitPalette→FixLbmImageMap`), renders, exit 0.
- ✅ **S58 fix ASan-validated** via a differential test (`MA_ASAN_LISTBOX_SELFTEST=1`): scalar
  `delete` → `alloc-dealloc-mismatch (new[] vs delete) at DeleteRow:2149`; `delete[]` → zero ASan errors.
  The harness is proven sensitive (catches the reverted bug).
- ✅ No regression (title/front-end render unchanged).
- ✅ Shared lessons doc (`port/BOB_PORT_LESSONS.md`) + cross-port reply written and mirrored byte-identical to `~/bob/doc`.

## Demo (Sprint Review)
`MA_ASAN_LISTBOX_SELFTEST=1 ./wmig` under ASan: self-test drives the real `DeleteRow`, zero ASan
errors; reverting the one-line fix reproduces the mismatch on the same harness.

## Commits
`dfa1ca8` rnd/BITSET · `c7e4e36` LBM guard · `6161c61` CRListBoxCtrl · `67b0b92` ASan self-test ·
plus the doc-sync commits. Cross-port reply: `port/CROSS-PORT-FROM-MA-2026-06-29.md`.

## Retro
- **What worked:** verifying each BoB finding against MA's *own* source before adopting — half the arc
  turned out non-shared (different game data / renderer), so blind copying would have been wrong.
- **Carry-forward:** the engine-wide primitives (RNG, bitfield ops) + communal IFF unpack are where the
  shared bugs live; renderer/shape-table bugs usually aren't. Record verdicts both ways so neither port
  re-investigates a dead end.
