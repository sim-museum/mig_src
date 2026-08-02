# Sprint 66 — "Face it" (autonomous)

**Goal:** the font FACE — the remaining half of cross-cutting deviation #1. Planned and
displaced by the Player Log in S64, S65 and (as S65-3) not started at all. **Scheduled
first this sprint specifically to protect it.**

**Committed (~8 pts):**
| Story | Pts | Definition |
|---|---|---|
| S66-1 Front-end font FACE | 5 | Identify the typeface gold uses, and render with it |
| S66-2 Title bar width + `?`/`✓` | 2 | `UpdateTitle` sizes from `viewsize.right`; the `?`/`✓` buttons |
| S66-3 Cross-port note 23 + close | 1 | Note 23; docs md5-identical; board/burndown/parity/`RUNNING.md`/rollup; gates |

## Results

**Sprint outcome: 6 of 8 pts. CROSS-CUTTING DEVIATION #1 IS SOLVED — both halves.**
**Gate caveat: ASan produced one intermittent failure (details under Gates). Not attributed.** The
colour half landed in S63; the face half lands here.

### S66-1 Front-end font FACE (5 pts) — ✅
**The game ships its own typeface and we were never loading it.**
`drive_c/windows/Fonts/Intel.ttf` — *"Copyright (c) Rowan Software, 1998"* — is the art
face gold renders with. `MIG.CPP`'s font setup asks for it by name (`myfont = "Intel"`,
with Arial / Arial Italic as the Western fallbacks).

Two independent reasons it never arrived:
1. **`ma_gdi_font_create` ignores the requested face entirely** (`(void)face;`) — the port
   loads ONE global TTF and draws everything with it.
2. **That one global load was rejecting `Intel.ttf`.** The loader already tried it *first*,
   yet every run reported `[gdifont] loaded …/DejaVuSerif-Bold.ttf`. Cause:
   **`stbtt_InitFont` only accepts platform-3 cmap encodings 1 (UNICODE_BMP) and 10
   (UNICODE_FULL)**, and Intel.ttf — like many 1990s decorative fonts — ships a
   **(3,0) SYMBOL** cmap. With no usable `index_map`, init fails outright and the port
   silently fell back to a system serif *for all text*.

Fix, two contained parts: let the vendored stb accept `STBTT_MS_EID_SYMBOL` (documented
as a local change), and — since symbol tables address characters at **`0xF000+c`** — route
every glyph lookup through a single `ma_cp()` helper rather than sprinkling the constant.
Now: `[gdifont] loaded …/Intel.ttf (symbol cmap)`.

**Verified against gold, not just "looks better":** the title menu renders
`PREFERENCES / SINGLE PLAYER / MULTI-PLAYER / …` in yellow **small caps** — and the gold
title shot is the identical small-caps yellow face. Preferences likewise: yellow small-caps
tab bar, blue small-caps labels, yellow small-caps values, matching gold's scheme and
typeface.

**Cross-cutting deviation #1 (font/typeface) is closed**: colour in S63, face here. The
named residuals on those rows are now only the **BDG tab** (a resource delta — gold is the
BDG-patched build) and **combo chrome** (cross-cutting #2, black-filled vs translucent).

*Note this also retires the "GDI DejaVu fallback" phrasing that had been repeated across
the parity doc and several cross-port notes since S56. It was accurate — but it described a
symptom (we fell back) as though it were the design, and nobody asked why the fallback was
being taken.*

### S66-2 Title bar width + `?`/`✓` (2 pts) — ⬜ not started
Displaced by S66-1, which was the deliberate priority. Carried whole.

### S66-3 Cross-port note 23 + close (1 pt) — ✅

### Gates
- **2D parity sweep — all 7 references deliberately REBASED, not byte-identical.** The
  typeface changes every screen by design; this is the same kind of deliberate rebase as
  S63's, and it must not be read as a pass. Byte-identical checking resumes in S67.
- ⚠️ **`port/asan_all.sh` — FAILED once, then PASSED. A real, INTERMITTENT finding, and
  the first ASan report since the S15–S43 epic closed.** The first suite run produced
  **2 × `stack-use-after-return`**:
  - `SRC/H/worldinc.h:257` — `ITEM_STATUS::…::T_size::operator ITEM_SIZE()`
  - `SRC/H/worldinc.h:565` — `item::T_shape::operator ShapeNum()`

  Both are `BITFIELD`/`ONLYFIELD` macro-generated proxy accessors on the packed item
  structs — the same MSVC-ism family as S41's `AddChildren` stack-use-after-scope.
  Reproduction attempts: 4 single-mode runs **clean**, then a full second suite run
  **PASS 0 reports**. So roughly **1 occurrence in ~20 runs**.

  **NOT ATTRIBUTED, and deliberately not claimed either way.** S66's diff is font-loading
  only, which makes causation implausible — but "implausible" is not evidence, and I did
  not build the pre-S66 (S65) ASan binary to test it. It is equally consistent with a
  pre-existing latent bug surfaced by changed per-frame timing (glyph rasterisation
  differs with the new face). **S67-1 should attribute it before anything else.**
- `port/stress_launch.sh` — **PASS 20/20**.

### Carry-over to S67
1. ⚠️ **Attribute the intermittent ASan `stack-use-after-return`** (above) — build the S65
   ASan binary and run the suite repeatedly to establish whether S66 introduced it or
   merely perturbed timing. It is rare (~1 in 20 runs), so a single clean run proves
   nothing; this needs several.
2. Title bar **width** (`UpdateTitle` sizes from `viewsize.right`) + the `?`/`✓` buttons.
3. **Per-face font selection** — `ma_gdi_font_create` still ignores the face name, so
   *everything* now draws in Intel.ttf, including text the game asked to be Arial
   (`straightfont`/`curlyfont`). Gold appears to use the art face throughout the front end,
   so this is not visibly wrong today, but it is luck rather than correctness.
4. Cross-cutting **#2 combo chrome** — now the largest remaining visual deviation.
5. Career content table (other half of I4); RScrlBar hosting; `ma_tabs_hit` click routing;
   #12 debrief capture.
