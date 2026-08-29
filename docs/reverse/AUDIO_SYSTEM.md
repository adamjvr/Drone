# Audio System Reconstruction

Phase 5 reconstructs the original audio behavior behind an asset-free portable event/mixer boundary. This document records the currently established Win32 DirectSound runtime, the DOS/Windows audio corpus relationship, and the first clean semantic cue layer.

## Evidence boundary

Original sound bytes are private evidence and are not stored in this repository. Checked-in manifests contain only path, size, extension and SHA-256 metadata. `scripts/analyze_audio_assets.py` derives `manifests/audio_asset_crosswalk.csv` exclusively from those metadata manifests.

The canonical shareware manifests currently contain:

- **61 Windows WAV files**;
- **59 DOS audio files**: 56 CLV, 2 WAV and 1 HMI;
- **58 matched stems**, four Windows-only stems (`doorclos`, `dooropen`, `firebal2`, `pulse`) and one DOS-only stem (`top1`);
- 63 crosswalk rows because DOS carries both `TEST.HMI` and `TEST.WAV` under stem `test`.

Metadata hashes prove two useful aliases without distributing audio: Windows `Bomb.wav` and `Missile.wav` are byte-identical, while DOS `TOP1.CLV` and `BOSS.CLV` are byte-identical.

The CLV waveform relationship remains documented separately in [`../formats/CLV.md`](../formats/CLV.md): canonical CLV is headerless 22,050 Hz unsigned 8-bit stereo PCM and corresponding Win32 WAVs are generally mono conversions.

## Win32 DirectSound primitive layer

| address | clean name | established behavior |
|---|---|---|
| `0x00406200` | `load_wav` | scans the 20-byte sound-slot table for a free entry, parses RIFF/WAVE `fmt ` + `data`, requires PCM, creates a DirectSound buffer, returns slot index or -1 |
| `0x004065B0` | `duplicate_directsound_buffer_slot` | finds a free 20-byte slot, copies source metadata, calls `DuplicateSoundBuffer`, returns new slot or -1 |
| `0x00406730` | `directsound_play_slot` | `SetCurrentPosition(0)` then `Play(0,0,flags)`; second effective argument is the DirectSound play flags |
| `0x00406780` | `directsound_set_volume` | converts game volume `v` to attenuation `30 * (v - 100)` then calls `SetVolume` |
| `0x004067B0` | `directsound_set_frequency` | passes caller frequency directly to `SetFrequency` |
| `0x004067D0` | `directsound_stop_reset` | `Stop()` then `SetCurrentPosition(0)` |
| `0x00406800` | `directsound_release_slot` | stop/reset, release the DirectSound buffer, clear the slot pointer |
| `0x00406860` | `directsound_get_status` | returns raw `GetStatus` flags, or -1 for a null slot |
| `0x00420020` | `play_sfx_from_20_voice_pool` | scans exactly 20 handles from index 0, chooses first raw status != 1, otherwise steals/restarts index 0 |
| `0x00420070` | `release_20_voice_pool` | releases exactly 20 handles through `0x00406800` |

### Exact 20-voice selection policy

`0x00420020` has no round-robin cursor. Every call starts at voice 0:

```text
for i = 0..19:
    status = GetStatus(pool[i])
    if status != 1:
        play pool[i]
        return pool[i]

play pool[0]   # all twenty status values were exactly 1
return pool[0]
```

The comparison is to raw status **exactly equal to 1** (`DSBSTATUS_PLAYING`), not a generic bit test. The clean `select_original_sfx_voice()` deliberately preserves that behavior.

### Exact game-volume conversion

`0x00406780` implements:

```text
DirectSound attenuation = 30 * (game_volume - 100)
```

Examples:

| game volume | DirectSound attenuation |
|---:|---:|
| 100 | 0 |
| 90 | -300 |
| 70 | -900 |
| 50 | -1500 |

## Proven cue mappings in the first portable layer

The clean core uses semantic `AudioCue` values and metadata-only `AudioCueDefinition` records. No original sample bytes are compiled into the engine.

| semantic cue | original asset | voice policy | volume | frequency | current GameSession emission |
|---|---|---|---:|---:|---|
| Rapid missile fire | `missile.wav` | 20-voice pool | 50 | 22050 | native, exact fire site |
| Shield pulse | `shields.wav` | 20-voice pool | 50 | 11025 | native, recovered phase-2 cadence |
| Special load/cycle | `ignite2.wav` | single buffer | 90 | source/default | native load/cycle sites |
| Special launch | `probe3.wav` | single buffer | 70 | source/default | native ordinary launch site |
| Probe bomb impact | `explode4.wav` | 20-voice pool | 50 | source/default | native ordered late-bomb event |
| Stinger impact | `stinger1.wav` | 20-voice pool | 100 | source/default | native ordered late-bomb event |
| Player lethal hit | `bigexp3.wav` | 20-voice pool | 90 | 15000 | native ordered late-bomb event |
| Enemy boss bomb fire | `missile.wav` | 20-voice pool | 50 | source/default | native Gemini/Lid-Top successful spawn site |
| Explosion variant 1/2 | `explode2.wav` | 20-voice pool | 60 | source/default | native process-global variant cycle |
| Explosion variant 3 | `explode3.wav` | 20-voice pool | 50 | source/default | native process-global variant cycle |
| Explosion variant 4 | `explode4.wav` | 20-voice pool | 50 | source/default | native process-global variant cycle |
| Trajectory flight 1..14 | `squad1.wav` .. `squad14.wav` | 20-voice pools | not yet cataloged | source/default | native from exact `rand()%14` result |
| Mission disarm interstitial | `deepness.wav` | single buffer | 90 | source/default | native transition site |
| Mission detonation interstitial | `detonate.wav` | single buffer | 90 | source/default | native transition site |

The late enemy-bomb collision subsystem still returns historical compatibility booleans for existing callers, but audio order no longer depends on them. The collision pass itself now carries an authoritative event queue, preserving slot order and same-slot fallthrough: special stop/impact precedes the same bomb's later player-hit sound, and an earlier slot may auto-launch `probe3.wav` before a later slot stops it. Shield absorption has no DirectSound call in that recovered late-bomb branch.

## Process-global explosion-SFX cycle — `0x00402900` / `0x0042EFD8`

The already-established helper `play_explosion_sfx_variant` increments byte `explosion_sfx_variant_cycle`, wraps value 5 back to 1, and selects:

```text
selector 1 -> explode2.wav
selector 2 -> explode2.wav
selector 3 -> explode3.wav
selector 4 -> explode4.wav
```

The selector has process lifetime, not encounter/campaign lifetime. `GameSession::original_audio` therefore survives gameplay resets. Exact recovered call multiplicity now reaches the portable event queue:

- rapid-missile trajectory collision: one call on every opaque hit, plus two more when the hit destroys an actor whose `destruction_burst_count != 1`;
- Stinger-display trajectory overlap: same one-plus-optional-two rule;
- Gemini Probe hit: one call; Gemini Stinger hit: two calls;
- Lid/Top closed-top Probe/Stinger impact: one/two calls; rapid top-mask and lid-open weakpoint hits: one call each; exposed-core Stinger destruction does not call `0x00402900`.

This preserves both sound multiplicity and the global `explode2, explode2, explode3, explode4` phase seen by later impacts.

## Current clean API

- `include/drone/audio/original_directsound.hpp`
  - exact 20-voice capacity and selector;
  - exact volume conversion;
  - established frequency constants.
- `include/drone/audio/audio_event.hpp`
  - semantic cue/action types;
  - metadata-only cue definitions;
  - fixed-capacity allocation-free `AudioEventQueue` for one session update (256 entries to retain worst-case multi-impact fanout).
- `GameSessionTickResult::audio_events`
  - emitted at proven gameplay call sites for rapid fire, shield pulse, special load/cycle, ordinary special launch, ordered bomb impacts/player hits, boss bomb fire, explosion variants, transient trajectory flight sounds and mission interstitial sounds.

A fixed-capacity queue is used so the deterministic gameplay core does not depend on allocator behavior or a platform audio API.

## Playback flags and looping

`0x00406730` passes its effective second argument directly to DirectSound `Play` flags. Gameplay one-shot paths reconstructed above use flags 0. Some callers elsewhere pass 1; Phase 5 will classify those call sites before assigning loop behavior to semantic cues. No unproven loop flag is encoded in the first cue table.

## Next audio work

1. finish exact initialization settings for Squad1..14, remaining boss/presentation cues and any still-unidentified pools;
2. classify every `Play(... flags=1)` caller and stop/restart interaction;
3. reconstruct Results/credits/long-form playback and DOS HMI differences;
4. replace remaining compatibility-only sound booleans where exact per-call producers are recovered;
5. add a portable mixer/backend interface only after original voice semantics are fully specified.
