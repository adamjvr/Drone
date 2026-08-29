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
| Drone approach/decode loop | `drone.wav` | single buffer | 90 at load; runtime-controlled | source/default | native Y=-117 loop start, 0..80 approach control, Probe decode 60/80 changes and exact stop producers |
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
  - semantic cue/action types, including parameterized `SetVolume` in original game-scale units;
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

## Parameterized `drone.wav` ownership and volume control

`drone.wav` demonstrates why long-form playback cannot be represented by Play/Stop alone. The resource setup loads the dedicated slot and applies game volume 90. The active objective path later owns a separate process-global volume scalar at `0x00440278`, and the clean core now exposes its writes as parameterized `SetVolume` events while leaving actual attenuation/mixing to a future backend.

The recovered active sequence is:

```text
Drone reaches Y=-117 on phase 2
  -> volume scalar = 0
  -> Play drone.wav with flags 1
  -> SetVolume(0)
  -> landmark skip leaves Y=-116

subsequent eligible phase-2 updates, while -116 < Y < 45
  -> if scalar < 80: scalar += 1; SetVolume(scalar)
  -> then normal Drone movement executes

Probe decode phase 1 completes
  -> SetVolume(60)
  -> decoder enters phase 2

attached phase-2 Probe is knocked off by enemy bomb
  -> SetVolume(80)
  -> clear decoder state
  -> emit Probe impact sound

Probe decode completes
  -> completion one-shot (asset mapping still unresolved in this slice)
  -> StopAndRewind drone.wav

Y=45 hold reaches exactly 4200 phase-2 ticks
  -> StopAndRewind drone.wav
  -> begin shared destructive countdown

rapid missile or red Stinger hits active Drone
  -> StopAndRewind drone.wav
  -> begin shared destructive countdown
```

The key call sites are `0x0040E529..0x0040E544` for start-at-zero, `0x0040E4C9..0x0040E4E5` for the 0->80 approach ramp, `0x0040CE00..0x0040CE0B` for decode-phase volume 60, `0x0040F3C8..0x0040F3D7` for interruption restore to 80, and `0x0040CEB5`, `0x0040E5B8`, `0x0040F249`, `0x0040F69E` for established stop producers. A blue Probe attachment itself leaves the loop running.

Because `0x00440278` is process-global rather than encounter-owned, `GameSession::original_audio` retains the scalar across campaign/encounter resets just like the established explosion-SFX selector. The Y=-117 start writes zero before the live ramp consumes it, so a new objective does not require a fabricated reset.

This slice deliberately does not name the Y=-40 one-shot or the decode-completion one-shot until their slot-to-asset identities are proven. It also leaves `air.wav` separate: air has multiple start/restart sites plus bidirectional state-dependent fades, so its ownership requires its own control-state integration rather than piggybacking on Drone semantics.

## Native `air.wav` state-2 envelope

The canonical gameplay ambience slot `0x0043F5F4` is not a constant-volume background track. Its process-global game-scale scalar is `0x004729A0`, and the state-2 monolith treats that scalar as a bidirectional envelope tied to Drone settlement state.

The gameplay load/start path at `0x0041F83C..0x0041F84C` establishes `air.wav` at volume 50. The active state-2 start at `0x0040C8E5..0x0040C90E` starts flags-1 playback and writes/applies scalar 50. Once the shared post-encounter tail has restarted the slot at zero, the next active encounter uses this exact update order:

```text
pre-scheduler:
    if drone_settlement_tick >= 60 and air_volume < 50:
        air_volume += 1
        SetVolume(air_volume)

advance four-phase scheduler / phase-2 settlement

post-settlement:
    if gameplay_phase == 2 and drone_settlement_tick < 60 and air_volume > 0:
        air_volume -= 1
        SetVolume(air_volume)
```

The pre-scheduler fade-up is `0x0040BFB5..0x0040BFDC`; the phase-2-only fade-down is `0x0040C0BD..0x0040C162`. This ordering matters: a detonation trigger reaches `trigger_drone_detonation_sequence` first and stops/rewinds the air slot at `0x0041D220..0x0041D22D`, while the normal settlement envelope remains tied to the same scalar already owned by `GameSession`.

The shared post-encounter tail at `0x0041E373..0x0041E3A5` is also explicit rather than an implicit mixer reset:

```text
air_volume = 0
StopAndRewind(air.wav)
Play(air.wav, looping)
SetVolume(air.wav, 0)
```

The clean session now emits that exact semantic sequence after both normal and destructive encounter rebuilds. `OriginalAudioRuntimeState::air_loop_volume_0_to_100` owns `0x004729A0` above campaign/encounter resets.

The gameplay slice still does not own menu/overlay presentation, but those two paths are now reconstructed in a separate clean host runtime described below.

## Main-menu and gameplay-overlay audio host

`run_main_menu` owns `lowbees.wav` independently of the state-2 gameplay session. Bootstrap code arms byte `0x00459F8C = 1`; menu entry consumes that byte only when it actually loads/starts the ambience. The exact startup order at `0x00418ADB..0x00418B1C` is:

```text
if lowbees_restart_armed == 1:
    load lowbees.wav
    local_volume = 0
    SetVolume(lowbees.wav, 0)
    Play(lowbees.wav, looping)
    lowbees_restart_armed = 0
```

At `0x004190F9..0x00419110` the local volume increases by exactly one per main-menu iteration while below 80. Cleanup at `0x00419DA6..0x00419DE3` is selective: raw states `0`, `2`, `7`, `13`, and `-1` stop/reset and release the slot, then re-arm `0x00459F8C = 1`. Instructions (`3`) and High Scores (`8`) do not take that cleanup branch, so the clean owner preserves the ambience across those synchronous modal paths rather than manufacturing a restart.

The tail after `run_main_menu` returns has separate `air.wav` ownership. At `0x0041A3D6..0x0041A41C`, raw state zero skips the restart; every non-zero state executes:

```text
Play(air.wav, looping)
air_volume = 0
SetVolume(air.wav, 0)
SetFrequency(air.wav, 11025)
```

The order matters: frequency is forced only after the loop has been rewound/started and volume zero applied. `AudioAction::SetFrequency` now preserves that parameter without binding the clean core to DirectSound.

Pause (`5`), quit-confirmation (`6`), and the nine-lives notice (`99`) share one overlay path at `0x0040C521..0x0040C665`. Each overlay iteration checks the same process-global `0x004729A0` air scalar: positive values are decremented by one and applied through SetVolume; once the scalar is zero, the path stop/rewinds `air.wav`. When the overlay returns to active state `2`, `0x0040C82C..0x0040C913` takes the active-gameplay restart branch, starts the loop and restores scalar/volume 50.

The clean implementation is intentionally host-side in `include/drone/audio/presentation_audio.hpp` and `src/audio/presentation_audio.cpp`. It shares `OriginalAudioRuntimeState::air_loop_volume_0_to_100` with gameplay but does not inject menu/modal cadence into deterministic `GameSession`.

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
  - established frequency plus `drone.wav`, `air.wav`, and main-menu `lowbees.wav` control constants;
  - metadata-only 13-site flags-1 call catalog with literal/register proof classification;
  - native semantic ownership for the shareware Lid/Top and Gemini loop starts/stops.
- `include/drone/audio/audio_event.hpp`
  - semantic cue/action types, including parameterized `SetVolume` and `SetFrequency`;
  - metadata-only cue definitions, including `drone.wav`, `air.wav`, `lowbees.wav`, the two native boss loops, four one-shot Results tracks and looping Ordering/Credits cues;
  - process-lifetime original-audio control state for the explosion selector, `0x00440278` Drone volume scalar and `0x004729A0` air volume scalar;
  - fixed-capacity allocation-free `AudioEventQueue`.
- `GameSessionTickResult::audio_events` and `PostGameRuntimeStepResult::audio_events`
  - exact gameplay event ordering plus native Drone approach/decode volume control, state-2 air envelope/restart control, boss-loop ownership, and Results/Ordering/Credits ownership transitions.
- `include/drone/audio/presentation_audio.hpp`
  - host-side main-menu `lowbees.wav` restart/fade/cleanup ownership;
  - main-menu tail `air.wav` zero-volume 11025-Hz restart;
  - raw-state 5/6/99 overlay fade-to-stop and state-2 resume restart.

## Next audio work

1. execute the recovered completion-credits fade envelope and finish any remaining long-form volume/pan semantics;
2. map the still-unidentified Y=-40 Drone one-shot and decode-completion one-shot, then recover the repeating `level1.wav` / `level2.wav` boss cadence and remaining Squad/pool initialization settings;
3. reconstruct DOS HMI behavior and compare it with the Win32 DirectSound ownership model;
4. add the portable mixer/backend only after those remaining original voice-control semantics are explicit.
