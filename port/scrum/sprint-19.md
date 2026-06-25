# Sprint 19 — M2 sky-colour fidelity: redirect the fix (the filler is NOT the bug)

**Goal:** land the S8/S9 sky-fidelity fix (flight sky renders dark ~[53,51,31] vs Wine's bright
~[227,232,235]).

**PO:** standing pre-approval. Run autonomously.

## Outcome: investigation sprint — **the prior fix plan was based on a wrong assumption; redirected.**

S8/S9 concluded the fix was to **add a true-colour gouraud span filler** to `ma_xasm.nasm`, on the
belief that `XASM_GouraudHoriLine*` is "intensity-only — reads just `vertex_intensity`, ignoring
`specular`/`specFlip`." **That belief is wrong** — read the actual NASM:

```
XASM_GouraudHoriLine1  (ma_xasm.nasm:313)
    mov eax, [esi + vertex_intensity]      ; reads 4 BYTES from the intensity offset =
                                           ;   intensity|specular|specFlip = R|G|B packed
    ...
    shr edx, 16                            ; >>16 picks the 3rd byte (specFlip / B channel)
    cmp edx,0x0F / clamp to 15
    mov ax, word [HorizonFadeData + 2*edx] ; <-- COLOUR from HorizonFadeData (the SKY table)
```

So the gouraud filler **already** (a) covers all three RGB bytes the sky stores, and (b) indexes
**`HorizonFadeData`** — the correct light-blue sky LUT (S9 verified `HorizonFadeData[0..15]` = ~(152,180,216)).
With the sky vertices (`InfiniteStrip` LANDSCAP.CPP:4903-4905 store R=142,G=166,B=200 in
intensity/specular/specFlip), this filler would resolve to `HorizonFadeData[~15]` = **blue**, not dark.

**Therefore the bug is NOT the filler — it is the sky strip's fill-type ROUTING.** The strip renders
dark because it is dispatched to a *different* span filler (a plain/image filler), never reaching
`XASM_GouraudHoriLine`. Building a new NASM filler (the S8/S9 plan) would have been wasted effort.

## Where it goes next (precise, pinned)
- `InfiniteStrip` only *generates* the strip vertices into `SHAPE.newco` (LANDSCAP.CPP:4822). The
  rasterization + fill-type selection happens in the pipeline that consumes `SHAPE.newco` (the
  `RenderLandscape` strip path; cf. the `SHAPE.newco`→`Land_Obj`/`Get3DArea(SPECIAL_TILE_OBJECT)`
  triangle building at LANDSCAP.CPP:1245+, the same shape as `PerspectivePoly`).
- **Next step:** trace the sky-strip poly through that pipeline to the *actual* span filler it hits
  (dispatch tables `ma_xasm.nasm:39-52`, index 1 = gouraud), find why its fill/shade-type index is not
  1 (gouraud), and **fix the routing** so the strip uses `XASM_GouraudHoriLine` (HorizonFadeData) —
  leaving the terrain's (correct) intensity-gouraud path untouched.

## Why no code landed
The fix is now correctly *targeted* but lives in the **software rasterizer's polygon fill-type
dispatch** — the highest-regression-risk area of the port (a wrong change silently corrupts the
*correct* terrain shading). It warrants a focused session with per-change A/B against the Wine oracle,
not a rushed edit. **No regression risk taken; the redirection (filler is correct → fix the routing)
is the bankable result and saves the next session from building an unneeded NASM filler.**

## Validation done
- Read the actual `XASM_GouraudHoriLine1` NASM (HorizonFadeData confirmed as its colour source).
- Cross-checked `InfiniteStrip` vertex packing (R/G/B in intensity/specular/specFlip).
- No build/behaviour change this sprint → no regression.
