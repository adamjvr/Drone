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
| Lid/Top encounter loop | `retro1.wav` | single buffer | 70 | source/default | native start at boss activation; stop on exposed-core Stinger destruction transition |
| Gemini encounter loop | `gemini.wav` | single buffer | 100 | source/default | native start at boss activation; stop only when the last activity-1 side enters destruction |

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

## Playback flags and long-form ownership

`0x00406730` passes its effective second argument directly to DirectSound `Play` flags after rewinding the selected slot to position zero. The canonical Win32 shareware executable has **13 proven calls whose effective flag is 1**. Eight push literal `1`; five pass a register whose value is established as `1` along the reaching path.

| Play call | asset | slot-index storage | owner/context | proof |
|---|---|---:|---|---|
| `0x00404498` | `bomber1.wav` | `0x00466C88` | Bomber boss | literal 1 |
| `0x0040474D` | `credits.wav` | stack-local | completion credits | literal 1 |
| `0x00405FA1` | `gemini.wav` | `0x00495CE4` | Gemini boss | literal 1 |
| `0x00407AA3` | **unresolved** | `0x0042EFE0` | registered boss slot 2 | literal 1 |
| `0x0040C8F3` | `air.wav` | `0x0043F5F4` | active state-2 air start | register `ESI=1` |
| `0x0040E52F` | `drone.wav` | `0x0047E280` | active state-2 Drone loop | register `EDI=1` |
| `0x00415C72` | `spidey.wav` | `0x00454B00` | Spidey boss | literal 1 |
| `0x00417323` | `retro1.wav` | `0x00438C18` | Lid/Top boss | literal 1 |
| `0x00418B14` | `lowbees.wav` | `0x0053C4E8` | main-menu ambience | literal 1 |
| `0x0041A3EE` | `air.wav` | `0x0043F5F4` | main-menu/new-run air restart path | register `EBP=1` |
| `0x0041B75B` | `thunder2.wav` | stack-local | Ordering Information | literal 1 |
| `0x0041E298` | `thunder2.wav` | `0x0042EFE8` | post-encounter transition | register `EBX=1` |
| `0x0041E395` | `air.wav` | `0x0043F5F4` | post-encounter air restart | register `EBX=1` |

The public `original_directsound_loop_call_sites()` catalog records these call sites without embedding any original audio. The registered slot-2 entry is intentionally asset-empty: the canonical shareware executable references the slot but no canonical loader assignment for `0x0042EFE0` has been found, so Phase 5 does not invent a registered-only filename.

### Results is not a loop

The Results path is a useful counterexample to a generic “music loops” rule. `0x00411726..0x00411759` selects one of `hiphop.wav`, `moon.wav`, `suspense.wav`, or `choral.wav`, loads it into a local slot, then `0x0041176C` calls `Play` with **flags 0**. `0x00411C5D` releases that local slot when Results exits. The clean `ResultsHiphop`, `ResultsMoon`, `ResultsSuspense`, and `ResultsChoral` cues therefore retain `directsound_play_flags == 0`.

### Ordering Information owns a local loop

`run_ordering_information` (`0x0041B730`) loads `thunder2.wav` at `0x0041B73C`, starts it with flags 1 at `0x0041B75B`, and owns both stop/reset (`0x0041C348`) and release (`0x0041C351`) before returning. No initial `SetVolume` is issued between load and play, so the clean cue leaves initial volume as source/default rather than manufacturing a number.

### Completion credits owns a local loop and fade

`run_completion_credits` (`0x00404720`) loads `credits.wav` at `0x00404738` and starts it with flags 1 at `0x0040474D`. Its local fade scalar begins at 100 and is stepped down through the DirectSound volume helper to 0 before the routine stops/resets and releases the slot (`0x00404B1A` / `0x00404B3E`). Phase 5 now owns semantic start/stop at the post-game boundary; the exact per-presentation fade envelope remains a mixer/presentation behavior to implement when the portable backend is introduced.

### Main-menu ambience is independently owned

`run_main_menu` loads `lowbees.wav` when the restart byte requests it, explicitly sets volume 0, starts flags-1 playback at `0x00418B14`, and increments the volume toward 80 in the menu loop (`0x004190F9..0x00419110`). On the established exit states it stop/resets and releases the slot (`0x00419DC6..0x00419DDB`) and arms the restart byte again. This lifecycle is cataloged but is not yet injected into the clean menu host.

## Native shareware boss loop ownership

The two boss families normally reachable in the canonical shareware campaign now own their encounter-loop start/stop events inside `GameSession`, at the same gameplay transitions that already own their mutable combat state.

### Lid/Top — `retro1.wav`

`initialize_lid_top_boss` reads the dedicated `retro1.wav` slot and calls `Play(..., flags=1)` at `0x004172EC..0x00417323`. The resource loader sets that slot to game volume 70. The clean `LidTopBossLoop` cue therefore starts when the Y=-200 processed-count dispatch activates the native Lid/Top runtime.

The active updater does **not** wait for the later 25-count lid retirement or asset release to stop the loop. A valid exposed-core red-Stinger hit calls `directsound_stop_reset(retro1)` at `0x00416C1E..0x00416C2A` and only then writes lid activity 2. That collision is also the already-established exception that does not call the process-global `0x00402900` explosion-SFX selector. The clean event queue therefore emits the loop stop directly on `destruction_transitions != 0`, with no fabricated explosion cue.

### Gemini — `gemini.wav`

`initialize_gemini_boss` starts the dedicated `gemini.wav` slot looping at `0x00405F92..0x00405FA1`; the loader sets game volume 100. Gemini's two destruction branches then preserve an important asymmetry: when one body crosses its damage threshold, the updater checks the *other* body's activity. If the other body is still activity 1, music continues. Only the transition that leaves neither body at activity 1 calls stop/reset on `gemini.wav` (`0x00405773..0x00405789`, mirrored at `0x00405C4A..0x00405C6B`).

Probe/Stinger impact SFX execute earlier in each damage branch than this threshold/loop-stop decision. `GameSession` therefore appends the existing exact explosion-variant calls first and the `GeminiBossLoop` stop afterward when the second surviving side crosses threshold. This preserves both multiplicity and order.

Resource release remains a different lifetime: `release_lid_top_boss_assets` and `release_gemini_boss_assets` eventually release their audio slots, but the semantic encounter loops have already stopped in combat. The portable layer does not conflate release with playback stop.

## Native post-game audio ownership

Phase 5 now connects the already-native `GameSession` post-game modal sequence to the semantic audio queue:

```text
enter Results              -> Play selected Results cue (flags 0)
confirm Results             -> StopAndRewind Results, Play Ordering Information
finish Ordering Information -> StopAndRewind Ordering Information
                              -> Play Completion Credits when perfect completion follows
finish Completion Credits   -> StopAndRewind Completion Credits
```

Suppressed Results/Ordering paths emit neither cue. A high-score modal itself still has no invented sound ownership; when completion credits follow high scores, the credits cue starts exactly at the semantic transition into `CompletionCredits`. This preserves control ownership while leaving UI rendering, the credits fade envelope, and actual sample playback to later platform/mixer work.

## Current clean API

- `include/drone/audio/original_directsound.hpp`
  - exact 20-voice capacity and selector;
  - exact volume conversion;
  - established frequency constants;
  - metadata-only 13-site flags-1 call catalog with literal/register proof classification.
  - native semantic ownership for the shareware Lid/Top and Gemini loop starts/stops.
- `include/drone/audio/audio_event.hpp`
  - semantic cue/action types;
  - metadata-only cue definitions, including the four one-shot Results tracks and looping Ordering/Credits cues;
  - fixed-capacity allocation-free `AudioEventQueue`.
- `GameSessionTickResult::audio_events` and `PostGameRuntimeStepResult::audio_events`
  - exact gameplay event ordering plus native Results/Ordering/Credits ownership transitions.

## Next audio work

1. integrate the already-classified menu/air/Drone loop lifetimes together with the volume-control semantics their clean owners require;
2. recover the repeating `level1.wav` / `level2.wav` boss cadence and finish exact initialization settings for Squad1..14 and still-unidentified pools;
3. implement credits/menu/approach fade envelopes and any pan behavior at the portable mixer boundary;
4. reconstruct DOS HMI behavior and compare it with the Win32 DirectSound ownership model;
5. add the portable mixer/backend only after those remaining original voice-control semantics are explicit.
