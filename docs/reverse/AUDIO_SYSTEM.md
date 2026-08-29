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
| Trajectory flight 1..14 | `squad1.wav` .. `squad14.wav` | 20-voice pools | 80 | source/default | native from exact `rand()%14` result |
| Mission disarm interstitial | `deepness.wav` | single buffer | 90 | source/default | native transition site |
| Mission detonation interstitial | `detonate.wav` | single buffer | 90 | source/default | native transition site |
| Drone approach/decode loop | `drone.wav` | single buffer | 90 at load; runtime-controlled | source/default | native Y=-117 loop start, 0..80 approach control, Probe decode 60/80 changes and exact stop producers |
| Lid/Top encounter loop | `retro1.wav` | single buffer | 70 | source/default | native start at boss activation; stop on exposed-core Stinger destruction transition |
| Gemini encounter loop | `gemini.wav` | single buffer | 100 | source/default | native start at boss activation; stop only when the last activity-1 side enters destruction |

### Trajectory Squad static pool initialization

The canonical static audio loader `0x0041F4F0` initializes all fourteen transient trajectory sound families identically: load the named WAV, apply game volume **80**, then duplicate the base buffer nineteen times. Each resulting storage range therefore contains exactly twenty DWORD DirectSound slot handles and is consumed by `0x00420020`. No Squad initialization path calls `0x004067B0`, so frequency remains the WAV/source default.

| asset | `load_wav` call | filename literal | 20-slot pool storage |
|---|---:|---:|---:|
| `squad1.wav` | `0x0041FBE6` | `0x0042BEF4` | `0x0042F1A8..0x0042F1F7` |
| `squad2.wav` | `0x0041FC34` | `0x0042BEE8` | `0x00440F58..0x00440FA7` |
| `squad3.wav` | `0x0041FC81` | `0x0042BEDC` | `0x0045A2A8..0x0045A2F7` |
| `squad4.wav` | `0x0041FCCF` | `0x0042BED0` | `0x00467390..0x004673DF` |
| `squad5.wav` | `0x0041FD1D` | `0x0042BEC4` | `0x004339B0..0x004339FF` |
| `squad6.wav` | `0x0041FD6A` | `0x0042BEB8` | `0x004461E0..0x0044622F` |
| `squad7.wav` | `0x0041FDB8` | `0x0042BEAC` | `0x0047FE48..0x0047FE97` |
| `squad8.wav` | `0x0041FE06` | `0x0042BEA0` | `0x00438A70..0x00438ABF` |
| `squad9.wav` | `0x0041FE53` | `0x0042BE94` | `0x00446290..0x004462DF` |
| `squad10.wav` | `0x0041FEA1` | `0x0042BE88` | `0x00440FA8..0x00440FF7` |
| `squad11.wav` | `0x0041FEEF` | `0x0042BE7C` | `0x00432488..0x004324D7` |
| `squad12.wav` | `0x0041FF3C` | `0x0042BE70` | `0x004677F0..0x0046783F` |
| `squad13.wav` | `0x0041FF8A` | `0x0042BE64` | `0x004629F8..0x00462A47` |
| `squad14.wav` | `0x0041FFD8` | `0x0042BE58` | `0x00441770..0x004417BF` |

The same pool bases appear in the exact `rand()%14` playback jump tables at `0x0040D294..0x0040D388` and `0x0040D596..0x0040D5F6`, and the global cleanup path releases all fourteen pools through `0x00420070` at `0x0040B932..0x0040B9E5`. The clean `original_trajectory_pool_initializations()` catalog makes those storage and loader facts regression-testable without original audio bytes.

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

## DOS HMI S.O.S. middleware contract

The canonical DOS executable is already classified as an HMI S.O.S.-based audio build, with `0x0007ECB4` tracked as the DOS-side HMI sample-loader candidate. The current Phase-5 evidence set does **not** contain a fresh executable-level trace of Drone's HMI start/stop callers, so this slice deliberately separates two evidence levels:

1. **HMI middleware capability**, established from surviving public S.O.S. 4.x SDK headers;
2. **Drone DOS policy**, which remains open until the canonical DOS call sites can be re-read.

The public HMI headers establish the following library-level contract:

| HMI S.O.S. capability | established middleware behavior | Drone-specific conclusion |
|---|---|---|
| mixer/voice ceiling | `_SOS_MIXER_CHANNELS = 32`, `_SOS_MAX_VOICES = 32` | **do not infer Drone configured 32** |
| sample start | `sosDIGIStartSample(driver, PSOSSAMPLE)` starts a caller-provided `_SOS_SAMPLE` descriptor | descriptor-backed API, not a required preduplicated pool |
| sample state | descriptor flags include active/processed/done/loop state | exact Drone polling policy still open |
| volume | descriptor `wVolume`; `sosDIGISetSampleVolume` / getter exported | exact Drone game-volume conversion still open |
| looping | descriptor `wLoopCount` plus loop state/callback support | exact Drone sustained-loop encoding still open |
| rate | descriptor `wRate`; `sosDIGISetSampleRate` / getter exported | exact Drone frequency overrides still need DOS call-site proof |
| pan | descriptor `wPanPosition`; setter/getter exported | no Drone panning claim yet |
| priority | descriptor `wPriority` | exact Drone priority/arbitration policy still open |
| lifecycle | explicit `StartSample`, `StopSample`, `StopAllSamples`, `SampleDone` plus completion callbacks | exact menu/game/modal lifecycle still open |

The clean metadata representation lives in `include/drone/audio/original_hmi.hpp`. It records the middleware facts above but intentionally has **no field asserting Drone's DOS configured voice count or steal policy**. The 32 values are capability/default constants from HMI, not reconstructed game configuration.

This creates an important cross-build distinction. Win32 Drone itself constructs selected high-overlap sounds as 20 duplicated DirectSound buffers and owns an exact first-status-not-1 / otherwise-slot-0 reuse policy. HMI S.O.S. instead accepts sample descriptors at its public start boundary; a DirectSound-style twenty-copy pool is not required by the middleware API. Drone DOS may still maintain its own game-level slot table, priority scheme or reuse rule, but that must be recovered from the executable rather than normalized from either backend.

Public API provenance used only as middleware documentation:

- `https://github.com/Wohlstand/SOSPLAY/blob/master/sos/include/sos.h`
- `https://github.com/Wohlstand/SOSPLAY/blob/master/sos/include/sosfnct.h`

No HMI source/header payload is copied into this repository.

## Canonical DOS HMI runtime voice semantics

The canonical DOS executable now closes the central HMI-runtime questions that the middleware-only foundation deliberately left open.

`0x0008D4AF` is the linked `sosDIGIInitDriver`. Its setup allocates `0x1E00` bytes for voice records and explicitly writes `0x20` to driver `+0x14` at `0x0008D78A`. Each voice record is `0xF0` bytes, matching the recovered/public `_SOS_SAMPLE` layout, so Drone DOS configures exactly **32 digital voices**.

`0x0008AC82` is `sosDIGIStartSample`. Its allocator is straightforward and materially different from Win32 Drone's reusable DirectSound pools:

```text
for voice = 0 .. configured_voice_count-1:
    if (voice.flags & _SACTIVE) == 0:
        copy caller's 0xF0 sample descriptor into voice
        voice.hSample = voice
        mark voice active
        link voice into active list
        return voice
return -1
```

There is no priority comparison and no saturation steal. The clean DOS contract therefore records **first inactive voice wins / saturated start fails**, while the Win32 contract retains its historically separate 20-buffer first-free / otherwise-slot-0 policy.

The exact linked control routines are also identified:

| DOS VA | HMI routine | recovered behavior |
|---|---|---|
| `0x0008AC82` | `sosDIGIStartSample` | first inactive voice; copy descriptor; return voice index; `-1` on saturation |
| `0x0008AE02` | `sosDIGIStopSample` | mark selected active voice processed/stopping |
| `0x0008AE74` | `sosDIGIStopAllSamples` | stop/mark all configured voices |
| `0x0008AFC1` | `sosDIGISetSampleVolume` | direct write of caller packed volume to active voice `+0x2C` |
| `0x0008B2A7` | `sosDIGISetSampleRate` | runtime rate write through retained voice handle |
| `0x0008B549` | `sosDIGISampleDone` | returns zero while selected voice remains active, one otherwise |
| `0x0008D4AF` | `sosDIGIInitDriver` | canonical driver setup; 32-voice configuration |

### Native DOS volume contract

HMI volume is not a normalized `0..100` scalar at this boundary. Drone writes native HMI packed 16-bit channel levels. `sosDIGISetSampleVolume` accepts the packed value directly; there is no equivalent of the Win32 DirectSound `30 * (value - 100)` conversion in the HMI control routine.

Examples from the canonical executable include `drone.clv` descriptor volume `0x41004100`, `retro1.clv` `0x52005200`, gameplay `air.clv` start volume `0x00003000`, and menu `lowbees.clv` starting at zero. The Lowbees fade increments its native level by `0x7D` per menu iteration toward `0x7000`, packs equal left/right channels, and calls `sosDIGISetSampleVolume` with the retained handle.

### Native DOS loop contract

Sustained sounds explicitly write `_SOS_SAMPLE.wLoopCount = 0xFFFFFFFF` before `sosDIGIStartSample`; ordinary one-shots leave the loop count at zero/default. Proven sustained descriptors include Gemini, Air, Retro1, Lowbees, Drone and local Thunder2 presentation audio. This is the executable-level DOS loop mechanism represented by the clean contract; `_SASR_LOOP` capability alone is not substituted for game evidence.

### Retained-handle lifecycle boundary

Controlled sounds save the voice index returned by StartSample and use it for later status/stop/volume/rate operations. Proven examples include gameplay Air (`StartSample 0x00077737`, handle global `0x004CCC8`), Drone (`0x0007A60D`, handle `0x004CB90`) and main-menu Lowbees (`0x00081709`, handle `0x004CBF0`). The broad cleanup region at `0x0007DE88..0x0007E165` performs SampleDone checks before stopping active tracked handles.

This is enough to close configured voices, arbitration, volume representation and loop encoding. `Q-AUDIO-007` deliberately remains open only for the **complete state-by-state menu/modal lifecycle catalog**; the portable mixer must not wait on already-established low-level voice semantics, but exact DOS presentation parity still requires that final classification.

## Current clean API

- `include/drone/audio/original_directsound.hpp`
  - exact 20-voice capacity and selector;
  - exact volume conversion;
  - established frequency constants;
  - exact 14-family Squad static-loader/pool initialization catalog.
- `include/drone/audio/original_hmi.hpp`
  - middleware-level HMI S.O.S. descriptor/control capability metadata;
  - 32-channel/voice library ceiling recorded explicitly as capability rather than Drone configuration;
  - explicit separation from Win32 Drone's game-owned 20-buffer pool policy.
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

`run_completion_credits` (`0x00404720`) loads `credits.wav` at `0x00404738` and starts it with flags 1 at `0x0040474D`. Its local fade scalar begins at 100 and is stepped down through the DirectSound volume helper to 0 before the routine stops/resets and releases the slot (`0x00404B1A` / `0x00404B3E`). Phase 5 now owns semantic start/stop at the post-game boundary and the presentation host executes the exact fade cadence: after the visual scroll terminates, the local scalar decrements before every SetVolume call, producing 99..0 across exactly 100 presentation iterations, then the modal completion boundary stops/rewinds the slot before release.

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
  -> Play parachut.wav (flags 0, volume 60 dedicated slot)
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

The two once-unidentified Drone one-shots are now proven from the static loader family. Slot `0x004D8510` is `hintdron.wav` at volume 80 and is played only by the transient Y=-40 branch; slot `0x0042F1F8` is `parachut.wav` at volume 60 and the decode-completion path plays it once immediately before stopping `drone.wav`. The parachute slot is reused by other presentation/input paths, so the semantic cue remains asset-centered rather than pretending decode owns the resource globally. `air.wav` remains a separate control owner because its start/restart and bidirectional fades are modeled independently.

## Shareware boss traversal one-shot cadence

The normally reachable Lid/Top and Gemini encounters each pair their continuous boss loop with a second **one-shot traversal cue**. These are not timers detached from movement: both are driven by the same shared byte `0x00454B04`, incremented exactly when the boss root reaches the bottom boundary and its fixed Y position is wrapped back to -100. Each boss initializer clears the byte, so the sequential encounters reuse rather than accumulate one cadence state.

Lid/Top (`0x00416885..0x004168C5`):

```text
if integer_root_y >= 240:
    0x00454B04 += 1
    if 0x00454B04 == 8:
        Play(level1.wav handle 0x0042EFEC, flags=0)
        0x00454B04 = 0
    fixed_root_y 0x00446E0C = -100 << 16
    # vertical velocity is preserved
```

`0x00417220` clears the byte at `0x00417304`; `0x00417350` loads `level1.wav` into `0x0042EFEC` and applies game volume 90.

Gemini (`0x004050BA..0x004050F9`) is structurally parallel but independently proven:

```text
if integer_body_a_y >= 240:
    0x00454B04 += 1
    if 0x00454B04 == 8:
        Play(level2.wav handle 0x00466B0C, flags=0)
        0x00454B04 = 0
    fixed_body_a_y 0x00467544 = -100 << 16
    # vertical velocity 0x0046754C is preserved
```

`0x00405EF0` clears the byte at `0x00405F87`; `0x00405FB0` loads `level2.wav` into `0x00466B0C` and applies volume 100. The write targets prove an important gameplay correction: the original resets **fixed Y position**, not vertical velocity. The clean bosses therefore perform repeated downward traversals, and `GameSession` emits `LidTopLevel1Cadence` / `GeminiLevel2Cadence` only on every eighth wrap. Both cues use DirectSound Play flags 0.

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

Suppressed Results/Ordering paths emit neither cue. A high-score modal itself still has no invented sound ownership; when completion credits follow high scores, the credits cue starts exactly at the semantic transition into `CompletionCredits`. This preserves control ownership while leaving UI rendering and actual sample playback to later platform/mixer work; the credits fade envelope itself is now recovered and host-owned.

## Current clean API

- `include/drone/audio/original_directsound.hpp`
  - exact 20-voice capacity and selector;
  - exact volume conversion;
  - established frequency plus `drone.wav`, `air.wav`, main-menu `lowbees.wav`, and eight-traversal shareware-boss cadence constants;
  - metadata-only 13-site flags-1 call catalog with literal/register proof classification;
  - native semantic ownership for the shareware Lid/Top and Gemini loop starts/stops.
- `include/drone/audio/audio_event.hpp`
  - semantic cue/action types, including parameterized `SetVolume` and `SetFrequency`;
  - metadata-only cue definitions, including `drone.wav`, `hintdron.wav`, `parachut.wav`, `air.wav`, `lowbees.wav`, the two native boss loops, one-shot `level1.wav`/`level2.wav` traversal cues, four one-shot Results tracks and looping Ordering/Credits cues;
  - process-lifetime original-audio control state for the explosion selector, `0x00440278` Drone volume scalar and `0x004729A0` air volume scalar;
  - fixed-capacity allocation-free `AudioEventQueue`.
- `GameSessionTickResult::audio_events` and `PostGameRuntimeStepResult::audio_events`
  - exact gameplay event ordering plus native Drone approach/decode volume control and one-shots (`hintdron.wav` at Y=-40, `parachut.wav` before decode stop), state-2 air envelope/restart control, boss-loop and every-eighth-traversal one-shot ownership, and Results/Ordering/Credits ownership transitions.
- `include/drone/audio/presentation_audio.hpp`
  - host-side main-menu `lowbees.wav` restart/fade/cleanup ownership;
  - main-menu tail `air.wav` zero-volume 11025-Hz restart;
  - raw-state 5/6/99 overlay fade-to-stop and state-2 resume restart.

## Next audio work

1. finish the remaining state-by-state DOS menu/modal lifecycle classification tracked by `Q-AUDIO-007`, using the now-identified Start/Stop/Done/Volume/Rate primitives and retained handles;
2. capture the resulting Windows-vs-DOS backend differences as executable regression fixtures, especially saturation behavior (Win32 steal-slot-0 versus DOS return-failure) and native volume/loop control;
3. finalize the portable mixer/backend around shared semantic `AudioEvent`s while keeping historically distinct voice arbitration, volume representation and loop encoding behind backend adapters;
4. leave later exact audio-trace parity and device/host latency validation to deterministic/fidelity phases rather than collapsing those concerns into the event contract.
