# Sprint 6 — "Make it audible" (M3 audio, increment 1)

**Goal:** replace the silent Miles (AIL) stubs with a real audio backend so the game produces
sound natively. Increment 1 = the **digital-sample path on OpenAL** (SFX / UI / engine / radio
samples). MIDI/XMIDI music + the in-flight engine-sound *triggering* are increment 2.

## M3 increment 1 — DONE (digital backend implemented + verified)

### What shipped
`SRC/compat/ma_openal.cpp` (NEW): the Miles AIL **digital-sample subset** implemented on OpenAL.
MA's engine speaks Miles AIL directly (no DirectSound), and `AILCALL == cdecl` (caller-cleans), so
these are drop-in C-linkage definitions matching the call sites' arg layout. Implemented:
- **Driver init:** `AIL_startup`, `AIL_waveOutOpen` (opens an OpenAL device+context; sets the
  `HDIGDRIVER`), `AIL_last_error`, `AIL_shutdown`. Degrades gracefully to silence if no device
  (dig stays NULL — exactly like the old stubs); `MA_NO_AUDIO=1` forces that path.
- **Sample handles:** `AIL_allocate_sample_handle` (one OpenAL source each), `init`, `release`.
- **Sample data:** `AIL_set_sample_file` (parses a RIFF/WAVE image — fmt+data chunk walk → AL
  buffer; the engine hands it whole WAV images via `getdata(blockptr)`), `AIL_set_sample_address`
  (raw PCM for radio), `AIL_set_sample_type` (mono/stereo + 8/16-bit for the raw path).
- **Playback:** `start`/`stop`(pause)/`resume`/`end`(stop); `set_sample_volume` (0..127→AL_GAIN),
  `set_sample_pan` (0..127, 64=centre → constant-power 2D pan via relative source position),
  `set_sample_playback_rate` (→AL_PITCH vs native), `set_sample_loop_count` (0=infinite→AL_LOOPING),
  `sample_status` (AL state→SMP_PLAYING/STOPPED/DONE), `sample_position` (AL_BYTE_OFFSET),
  `set_digital_master_volume` (→AL listener gain).
The MIDI/XMIDI sequence + DLS stubs stay no-op in `miles_ail_stub.cpp` (increment 2).

### Verified
- **End-to-end self-test** (`MA_AUDIO_SELFTEST=<wav>`): loads a real game sample
  (`SAMPLES/battlelp.wav`, 1ch/11025Hz/8-bit) through the AIL path and confirms OpenAL renders it —
  source state PLAYING throughout, byte offset advancing at ~11025 bytes/s (the correct rate).
  Proves the backend works independent of engine triggers.
- **Wired into the game's real init:** `StartUpMiles → NewDigitalDriver → AIL_waveOutOpen` opens
  OpenAL (`[al] OpenAL ready`), and 4 sample handles allocate into the engine's `soundqueue` pool
  (`allowedsamples = nohandlers/3`). So sound will play wherever the engine calls
  `PlaySample/PlayOnce/OverrideSample`.
- **No regression:** 3D-launch stress 4/4, menu↔flight round-trip clean, boot exit 0.

### Also fixed
- **Window-close / Ctrl+ESC exit hang** (`bob_video.cpp`): with the OpenAL mixer thread + GL context
  live, `SDL_Quit()` could block on teardown. Terminal exits now `ma_save_preferences()` then
  `_exit(0)` directly (skip `SDL_Quit`; the OS reclaims everything).

### Build
`port/rebuild.sh`: `ma_openal` added to the compat TU list; `-lopenal` on the link line.

## Increment / Review (PO standing-accept)
Native audio backend is live: OpenAL opens on boot, the digital driver + sample pool initialize
through the game's own Miles path, and a real game WAV plays through it at the correct rate
(self-test). The engine is no longer hard-silent — any `PlaySample`/`PlayOnce` now produces sound.

## Retro
- **Went well:** the self-test decoupled "is the backend correct" from "does the engine trigger
  sounds" — proved correctness immediately instead of chasing flight-sim state.
- **Learned:** MA is pure Miles AIL (no DirectSound), so BoB's DirectSound→OpenAL shim wasn't
  directly reusable — implementing AIL→OpenAL directly is fewer call-site changes and cleaner.
- **Open (increment 2):** the in-flight engine rumble / SFX don't fire yet — `Miles::ProcessSpot`
  gates on `Manual_Pilot.ControlledAC2` + an engine-sound FSM (Freq/CurrentFile/OverrideSample)
  driven by the flight physics per frame; needs tracing the move-loop sound update. Plus MIDI music
  (the `tune[]`/sequence path) and 3D positional placement (`SetListener`/`ProcessSpot` listener).

## M3 increment 1b — in-flight SFX now FIRING (added same sprint)
Traced why flight was still silent: `Miles::ProcessSpot` (the per-frame engine-sound update, called
from `DoMoveCycle`) ran with all gates passing (`ControlledAC2` set, not paused, `EngineSound.Freq`
live) **except the volumes were 0**. Root cause: every pre-OpenAL run had `dig`==NULL, so
`NewDigitalDriver`'s else zeroed `sfx`/`uisfx` and the MIDI-fail path zeroed `music`, and
`ma_save_preferences` persisted that all-zero triple to `settings.mig` — which then loaded back every
boot. **Fix (`MILES.CPP`, MA_LINUX):** when the OpenAL digital driver opens successfully and the
volume triple is all-zero (the stale-stub-save signature, not a user choice), restore audible
defaults (sfx/uisfx 125, music 64, rchat 125). The user can still adjust/mute live in the Sound
settings screen (the engine reads `Save_Data.vol` per frame).

**Result:** in flight, real game SFX now play through OpenAL — 4 `start_sample` / 3 `sample_file` in
a short throttled Hot Shot: the engine rumble (loop, gain scaling with throttle), one-shots, and
positional sounds (pan varying). 8-bit/11025Hz mono WAVs decode + render correctly. Round-trip + 3D
stress still clean. (`MA_TRACE_AUDIO` traces `ProcessSpot` gates + each sample.) Music (MIDI) stays
silent — that's increment 2.

## ➡ Next
- **M3 increment 2:** MIDI/XMIDI music (the `tune[]`/sequence path → a soft-synth or pre-rendered
  audio) + 3D positional listener placement (`SetListener` from the view point) + radio chatter.
- **M2** 3D fidelity A/B vs Wine (the dominant remaining chunk).
