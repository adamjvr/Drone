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
- completion credits own a stack-local looping `credits.wav` buffer; the host runtime now executes the recovered post-scroll fade as exactly 100 explicit SetVolume calls (99 down through 0) before the existing modal boundary stops/rewinds the slot;
- main-menu `lowbees.wav` ownership is documented as volume-0 loop start, fade-in toward 80, and stop/release on the established exit states;
- `GameSession` now emits semantic Results -> Ordering -> Credits start/stop events at the already-native post-game modal boundaries, without embedding samples or moving UI/persistence duties into gameplay.

The portable backend still deliberately waits: this slice specifies ownership and flags first, so a future mixer does not bake in the false assumption that every music-like asset loops.

## Native shareware boss loop ownership

The fourth Phase-5 slice moves the two shareware-reachable boss loops from catalog-only evidence into the native session boundary:

- Lid/Top activation at the already-native Y=-200 dispatch starts `retro1.wav` with volume 70 and DirectSound loop flag 1, matching `0x004172EC..0x00417323`;
- the exposed-core Stinger transition stops/rewinds `retro1.wav` immediately at `0x00416C1E..0x00416C2A`, before lid activity is written to destruction state 2; this exact hit still emits no `0x00402900` explosion-variant SFX;
- Gemini activation starts `gemini.wav` at volume 100 with loop flag 1, matching `0x00405F92..0x00405FA1`;
- destroying the first Gemini side leaves that loop running; only the threshold transition of the last activity-1 body stops/rewinds it, matching the paired `0x00405773..0x00405789` / `0x00405C4A..0x00405C6B` branches;
- boss impact explosion variants retain their original earlier call order, so the final Gemini loop stop follows the second side's Probe/Stinger impact SFX rather than preceding it.

At the fourth slice boundary, registered-only boss loops remained metadata-only and menu/air/Drone loops still needed a richer mixer-control event than simple Play/Stop; the next slice closes the Drone portion of that gap.

## Parameterized Drone loop control

The fifth Phase-5 slice adds that richer control boundary and fully integrates the shareware `drone.wav` approach/decode lifetime that is already owned by native `GameSession` state:

- `AudioEvent` now carries a parameter value and supports `SetVolume` without introducing a platform mixer or sample data; values remain in the original 0..100 game scale and are translated to DirectSound attenuation only by the backend contract;
- the dedicated `drone.wav` slot is loaded at game volume 90, but the live approach start at Y=-117 explicitly starts looping playback and immediately applies volume 0 (`0x0040E529..0x0040E544`);
- eligible phase-2 approach updates with `-116 < Y < 45` increment process-global volume scalar `0x00440278` by exactly one until 80, before normal Drone movement (`0x0040E4C9..0x0040E4E5`);
- Probe decode phase 1 -> phase 2 forces volume 60 before status changes (`0x0040CE00..0x0040CE0B`), while an enemy-bomb interruption of an attached phase-2 Probe restores volume 80 before the impact SFX (`0x0040F3C8..0x0040F3D7`);
- successful phase-2 decode completion stops/rewinds `drone.wav` at `0x0040CEB5..0x0040CEBE`; at this slice boundary the preceding one-shot was still unmapped, and is resolved by the later Drone one-shot slice below;
- the exact Y=45/4200-tick timeout, rapid-missile Drone hit, and Stinger Drone hit each stop/rewind the loop at their proven producer sites; ordinary Probe attachment does not stop it.

`0x00440278` is kept in `OriginalAudioRuntimeState` above encounter/campaign rebuilds, matching its process-global lifetime. Tests cover start/ramp/cap, both decode volume changes, interruption-before-impact ordering, completion/timeout stops, and both destructive weapon stops. At this slice boundary menu/credits fades and the second Y=-40 Drone one-shot remained separate evidence-backed work; those later slices now resolve them without retroactively guessing their identities here.

## Native `air.wav` ambience envelope

The sixth Phase-5 slice integrates the gameplay-owned portion of `air.wav` without folding menu/overlay behavior into `GameSession`:

- the canonical gameplay load/start path keeps `air.wav` looping at game volume 50, with the live scalar stored process-globally at `0x004729A0`;
- before the four-phase scheduler, `drone_settlement_tick >= 60` increments that scalar by exactly one per logical state-2 update until the volume-50 cap, preserving the original pre-scheduler ordering;
- after phase-2 settlement advancement, `drone_settlement_tick < 60` decrements the scalar by exactly one on phase-2 updates until zero; non-phase-2 updates do not perform this fade-down;
- entering the Drone detonation sequence stops/rewinds `air.wav` immediately, before later gameplay work in that update;
- the shared post-encounter transition performs the exact `StopAndRewind -> Play(loop) -> SetVolume(0)` sequence and stores scalar zero, so the next encounter naturally fades back toward 50;
- the separate main-menu/new-run path that restarts `air.wav` at zero and then forces 11025 Hz, plus the raw-state 5/6/99 overlay fade/stop path, remain host/menu ownership and are intentionally not injected into the clean gameplay session yet.

`0x004729A0` now lives beside the other process-global reconstructed audio controls in `OriginalAudioRuntimeState`. Regression coverage locks the +1/cap, phase-2 -1 floor, detonation stop, post-encounter restart ordering and reset lifetime.

## Menu/overlay audio host ownership

This Phase-5 slice moves the non-`GameSession` ambience controls into a clean presentation/audio-host boundary:

- `lowbees.wav` is now a semantic looping cue with the exact main-menu lifecycle: restart-armed entry applies volume 0 before Play, the menu loop raises volume by one per iteration to 80, and raw states `0`, `2`, `7`, `13`, and `-1` stop ownership and re-arm the original restart byte;
- Instructions (`3`) and High Scores (`8`) preserve the owned menu ambience rather than forcing a restart;
- the main-menu return tail now has an exact asset-free control sequence for every non-zero raw state: `Play(air.wav)` -> `SetVolume(0)` -> `SetFrequency(11025)`;
- pause, quit-confirmation, and nine-lives overlays share the original `air.wav` -1-per-overlay-iteration fade to zero, followed by stop/rewind on subsequent zero-volume iterations;
- resuming active state 2 restarts the air loop and restores the canonical gameplay scalar/volume 50;
- `SetFrequency` joins `SetVolume` as a parameterized semantic audio action, still without introducing a platform mixer or original sample bytes.

These controls live in `audio/presentation_audio` rather than `GameSession`, preserving the original ownership split between deterministic gameplay and synchronous menu/overlay presentation.

## Shareware boss traversal audio cadence

This Phase-5 slice closes the repeating `level1.wav` / `level2.wav` ownership and corrects the movement interpretation that makes those sounds repeat:

- Lid/Top `0x00416885..0x004168C5` and Gemini `0x004050BA..0x004050F9` each increment shared byte `0x00454B04` only when the active root reaches integer Y>=240; each encounter initializer resets the byte (`0x00417304` / `0x00405F87`);
- count 8 performs a flags-0 one-shot and resets the byte: Lid/Top uses `level1.wav` handle `0x0042EFEC` at volume 90, Gemini uses `level2.wav` handle `0x00466B0C` at volume 100;
- after the cadence branch, the executable writes `-100<<16` to the **fixed Y position** (`0x00446E0C` / `0x00467544`), not the vertical-velocity field. Positive descent velocity is preserved, so each boss repeatedly traverses downward and the cue fires once per eight completed passes;
- the clean boss lifecycle now owns the exact traversal counter/wrap and `GameSession` emits `LidTopLevel1Cadence` / `GeminiLevel2Cadence` in the original bottom-crossing order before later boss-bomb attack work.

This supersedes the earlier Phase-4 documentation shorthand that described Y>=240 as an upward retreat.

## Resolved Drone approach/decode one-shots

This Phase-5 slice closes the final two shareware Drone one-shot identity gaps from the static Win32 loader and exact state-2 call sites:

- loader `0x0041F73B..0x0041F75A` assigns slot `0x004D8510` from literal `hintdron.wav` and applies game volume 80. The only playback reference is the transient Y=-40 approach branch at `0x0040E55C..0x0040E56C`, which calls Play with flags 0 and immediately advances Y to -39;
- loader `0x0041F810..0x0041F82F` assigns slot `0x0042F1F8` from literal `parachut.wav` and applies game volume 60. The successful Probe-decode branch at `0x0040CEA9..0x0040CEBE` plays that buffer once and then stop/rewinds `drone.wav` before normal Drone movement releases the Y=45 hold;
- `parachut.wav` is intentionally represented by an asset-centered `ParachuteOneShot` cue rather than a decode-specific resource name because the same dedicated slot is reused by separate presentation/input paths at `0x00419660`, `0x004196F9`, `0x0041A8B6`, and `0x0041A8FC`;
- the clean Drone tick now exposes Y=-117 loop-start and Y=-40 hint landmarks separately. `GameSession` emits the hint one-shot at the latter and preserves decode completion order as `ParachuteOneShot -> StopAndRewind(drone.wav)`.

The Windows/DOS asset inventories independently corroborate both stems (`Sounds/Hintdron.wav` <-> `HINTDRON.CLV`, `Sounds/Parachut.wav` <-> `PARACHUT.CLV`). No original media bytes are required by the semantic runtime.

## Static Squad trajectory pool initialization

This Phase-5 slice closes the last unknown initialization value in the native semantic cue table:

- `squad1.wav` through `squad14.wav` are each loaded by the canonical static audio loader at `0x0041FBE6..0x0041FFD8`, assigned game volume **80**, and never receive a frequency override during initialization;
- every family consists of one loaded base buffer plus nineteen `DuplicateSoundBuffer` results, yielding the exact 20-voice pool consumed by `0x00420020`;
- the fourteen pool storage ranges are now represented in `original_trajectory_pool_initializations()` and tested for exact 20-DWORD extent, asset order, volume 80, and source/default frequency;
- the two trajectory playback switch tables (`0x0040D294..0x0040D388`, `0x0040D596..0x0040D5F6`) preserve the existing `rand()%14` semantic mapping, while global cleanup releases the same pool bases at `0x0040B932..0x0040B9E5`.

With this change, all currently native shareware semantic cues that are statically volume-initialized have explicit initialization metadata. Stack-local Results/Ordering/Credits entries remain source/default where the original issues no initial `SetVolume`; that is evidence, not an unresolved static-loader gap. The next Phase-5 work is Windows-vs-DOS HMI behavior and the portable mixer/backend contract.

## Initial work

- inventory every gameplay/presentation sound event and its source asset;
- recover DirectSound voice-pool allocation/reuse, priority, stop/restart and overlap behavior;
- recover volume/pan/loop semantics and distinguish Windows DirectSound from DOS HMI behavior;
- connect native `GameSession` semantic sound requests to a portable audio event layer;
- preserve deterministic gameplay RNG ownership while documenting any audio/presentation RNG consumers;
- add asset-free event-order regression tests before platform playback backends.

## Non-goals

Exact whole-runtime trace parity remains Phase 6, end-to-end shareware discrepancy closure remains Phase 7, and registered-only content remains Phase 8.
