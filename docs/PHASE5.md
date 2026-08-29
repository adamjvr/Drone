# Phase 5 — Audio Reconstruction

**Status:** in progress.

Phase 5 begins from the Phase-4-complete portable simulation. Its job is to reconstruct the original audio behavior behind a portable event/mixer boundary without changing established gameplay semantics.

## First integrated foundation

The first Phase-5 slice establishes the portable event/runtime boundary without bundling audio:

- exact Win32 DirectSound slot/play/stop/volume/frequency/status primitives are documented;
- the exact 20-voice first-free/all-busy-slot-0 policy is clean-tested;
- a fixed-capacity semantic `AudioEventQueue` is emitted from proven `GameSession` call sites;
- metadata-only DOS/Windows audio crosswalk generation is checked in;
- original assets remain private evidence.

See [`reverse/AUDIO_SYSTEM.md`](reverse/AUDIO_SYSTEM.md).

## Ordered impact/audio integration

The second Phase-5 slice removes the first lossy presentation bridges and preserves original call order/multiplicity:

- late enemy-bomb collisions now emit `probe3` stop/restart plus Probe/Stinger/player-hit events directly inside the per-slot collision loop;
- `0x00402900` is represented as a process-global explosion-SFX cycle (`explode2`, `explode2`, `explode3`, `explode4`) backed by established byte `0x0042EFD8`;
- rapid-missile and Stinger-display trajectory impacts expose their exact 1-or-3 variant-call counts;
- Gemini Probe/Stinger impacts expose exact 1/2 variant-call counts;
- Lid/Top closed-top and rapid-missile impact lanes expose their exact variant calls while the exposed-core Stinger kill correctly emits none through `0x00402900`;
- successful Gemini/Lid-Top enemy-bomb spawns emit the original `missile.wav` 20-voice pool cue at source/default frequency;
- the session audio runtime owns the explosion selector above campaign/encounter resets, matching its process-global Win32 lifetime.

The per-update queue is fixed at 256 entries: still allocation-free, but deliberately large enough to preserve same-tick multi-actor Stinger/trajectory sound fanout rather than collapsing valid calls.

## Long-form playback ownership

The third Phase-5 slice closes the first long-form/loop ownership gap:

- all 13 currently proven Win32 `Play(..., flags=1)` call sites are cataloged, including five register-propagated flag values that are easy to miss in a literal-only scan;
- the registered boss slot-2 loop remains explicitly unresolved because the canonical shareware binary does not establish an asset load for its slot;
- Results playback is proven **one-shot** (`flags=0`) across `choral.wav`, `suspense.wav`, `moon.wav`, and `hiphop.wav`;
- Ordering Information owns a stack-local looping `thunder2.wav` buffer and stop/release lifetime;
- completion credits own a stack-local looping `credits.wav` buffer, including the recovered 100->0 fade-before-stop behavior;
- main-menu `lowbees.wav` ownership is documented as volume-0 loop start, fade-in toward 80, and stop/release on the established exit states;
- `GameSession` now emits semantic Results -> Ordering -> Credits start/stop events at the already-native post-game modal boundaries, without embedding samples or moving UI/persistence duties into gameplay.

The portable backend still deliberately waits: this slice specifies ownership and flags first, so a future mixer does not bake in the false assumption that every music-like asset loops.

## Initial work

- inventory every gameplay/presentation sound event and its source asset;
- recover DirectSound voice-pool allocation/reuse, priority, stop/restart and overlap behavior;
- recover volume/pan/loop semantics and distinguish Windows DirectSound from DOS HMI behavior;
- connect native `GameSession` semantic sound requests to a portable audio event layer;
- preserve deterministic gameplay RNG ownership while documenting any audio/presentation RNG consumers;
- add asset-free event-order regression tests before platform playback backends.

## Non-goals

Exact whole-runtime trace parity remains Phase 6, end-to-end shareware discrepancy closure remains Phase 7, and registered-only content remains Phase 8.
