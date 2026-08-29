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

## Initial work

- inventory every gameplay/presentation sound event and its source asset;
- recover DirectSound voice-pool allocation/reuse, priority, stop/restart and overlap behavior;
- recover volume/pan/loop semantics and distinguish Windows DirectSound from DOS HMI behavior;
- connect native `GameSession` semantic sound requests to a portable audio event layer;
- preserve deterministic gameplay RNG ownership while documenting any audio/presentation RNG consumers;
- add asset-free event-order regression tests before platform playback backends.

## Non-goals

Exact whole-runtime trace parity remains Phase 6, end-to-end shareware discrepancy closure remains Phase 7, and registered-only content remains Phase 8.
