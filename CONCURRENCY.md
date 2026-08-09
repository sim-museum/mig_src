# Parallel-session rules — ma

Other Claude sessions may be working on the sibling projects at the same time. This box has
**one display and 4 cores**; see `~/CONCURRENCY.md` for the full picture.

## The rule that matters

Wrap anything that renders, opens a window, or captures a screenshot:

```bash
export PATH="$HOME/bin:$PATH"
gl-lock <your command>
gl-lock --status          # who has the display right now
```

`gl-lock` also refuses to start when the desktop is locked, which otherwise looks exactly like
the port hanging at the title screen.

### What the lock is actually for (corrected 2026-08-02)

An earlier version of this note claimed that two sims rendering at once corrupts screenshots.
**That was wrong.** Every capture path here — Julia's `JM_SHOTS` and the MA/BoB parity dumps —
uses `glReadPixels` against its OWN GL framebuffer (Julia's window is even created with
`GLFW.VISIBLE, false`). Two processes drawing at once each read their own buffer; neither can
see the other's pixels. Pixel content is safe.

The real reason to serialise is **contention for one GTX 1660 SUPER and 4 cores**. That
matters because results here can be frame-rate dependent — MiG Alley's stress gate scores a
run `HANG` when it misses its frame target, so a second sim hammering the GPU can manufacture
a failure that looks like a port defect. Julia captures also slow down under load.

So: still wrap GL runs in `gl-lock`, but if you see a contention alert, the question to ask is
"were any timing-sensitive results taken in that window?" — not "must I discard my captures?".

#### If your launch prompt says otherwise, the prompt is stale (added 22:15, 2026-08-02)

All three sessions running right now were started with this line in their prompt: *"A capture
taken while another sim owns the screen gives plausible but WRONG pixels and will poison your
parity verdicts."* That is the debunked claim — the prompts were written before the correction
above. **Do not re-take or discard any capture on concurrency grounds.** If you have already
recorded a parity residual as "possibly contended pixels", that reason is void; re-judge it on
its own merits.

**Practical consequence:** the trigger for taking `gl-lock` is *load*, not *pixels*. Your stress
gate is the thing most at risk on this box — it scores `HANG` on a missed frame target, so a
neighbour's CPU-heavy work can fake a port defect in *your* results. That cuts both ways: BoB
wrapping its `SDL_VIDEODRIVER=dummy` front-end captures in `gl-lock` is deliberate and correct,
even though they never reach the screen. Note `~/bin/gl-lock`'s own header comment still states
the old pixel rationale; `~/CONCURRENCY.md` and this file are the authority.


## This repo's slot

`ma` runs concurrently with the other two flight sims — separate repos, no shared files.
The **Julia Racer** session works through its 5 tracks sequentially and takes the display in
~3-minute blocks; expect to wait occasionally at gate time.

Keep build and capture output in your own scratch directory. `~/gold standard/` is the
shared parity oracle and is read-only for port work.
