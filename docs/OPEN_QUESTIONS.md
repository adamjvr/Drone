# Open Questions

This is the human-readable research queue. Machine-readable tracking lives in `reverse/ledger/open_questions.csv`. Questions should be closed only when the evidence and resulting documentation/implementation are linked.

## Critical — blocks simulation architecture

### Resolved — Q-TIME-001 canonical DOS fidelity cadence

Cross-build placement now establishes the ordinary DOS gameplay boundary: replay/update advances once and the sync-enabled gameplay tail performs one VGA retrace wait while running BIOS mode 13h. The clean DOS fidelity scheduler therefore uses the standard mode-13h cadence of approximately **70.0863 Hz**. The Win32 15,000-QPC-count wall-clock behavior remains separately tracked as `Q-TIME-003` because the executable does not query QPC frequency.

### Resolved — Q-STATE-001 Win32 user-facing state protocol

The tracked values are now mapped: `0` exit transition, `1` menu-reset entry, `2` active gameplay, `3` instructions, `4` menu re-entry, `5` pause, `6` quit confirmation, `7` ordering information, `8` high scores, `13` demo-launch sentinel, and `99` the nine-lives/disqualification notice. The direct dispatcher still handles only `0..5`; larger values are consumed by subordinate menu/modal/gameplay paths. See [`../reverse/windows/state_machine.md`](../reverse/windows/state_machine.md).

### Resolved — Q-ENTITY-001 FONT2 0x14-byte glyph descriptor/cache

The repeated `0x14` stride is not a gameplay entity family. Win32 `0x00401470` builds 64 descriptors at `0x00466C90`, indexed by `character - 0x20`; DOS `0x000809B0` independently builds the same 64×`0x14` FONT2 cache at data offset `0x6F80`. Both establish mutable X/Y, 7×5 dimensions, a glyph-mask pointer at `+0x10`, identical 16×4 gutter geometry, and the same mask renderer. The clean engine models the semantic font layout without reproducing the pointer-bearing original record. See [`reverse/BITMAP_FONT.md`](reverse/BITMAP_FONT.md).


### Resolved — Q-ENTITY-002 common Win32/DOS sprite-entity family

Producer/consumer tracing now establishes the Win32 `0x154` and DOS `0x14F` records as a field-level common sprite/entity correspondence rather than a loose candidate. The core position/velocity/dimension/collision/frame/activity fields align; DOS places the frame table and later metadata two bytes earlier; both preserve an unreferenced 128-byte middle block; and matched destruction paths establish `+0x30/+0x31` damage/threshold plus the shifted destruction-burst and score-value tail fields. Family overlays remain owned by their subsystems instead of being forced into one universal clean struct. See [`reverse/ENTITY_LAYOUT.md`](reverse/ENTITY_LAYOUT.md).

## High — active cross-phase research

### Resolved — Q-INPUT-001 canonical gameplay-action aggregation

Neither original build owns one packed gameplay-action bitfield. Win32 combines direct key tests with normalized DirectInput joystick action bytes; DOS combines keyboard/game-port paths; both then replace exactly the same six controls with replay channels 1–6 during demo playback. Live vertical movement remains outside replay substitution. The clean engine therefore uses independent semantic booleans in `GameplayInputFrame`, with physical-device normalization kept in platform hosts. See [`reverse/INPUT.md`](reverse/INPUT.md).

### Resolved — Q-FLY-001 normal FLY AUX/path semantics

Win32 `0x00415FA0` establishes trajectory index/step/wrap and normal AUX animation: AUX `<=1` is a signed relative frame delta and AUX `>1` selects absolute frame `aux-2`. The broader group lifecycle is also recovered; remaining work is limited to special-family substitutions/producers and full template cataloging. See [`formats/FLY.md`](formats/FLY.md) and [`reverse/TRAJECTORY_GROUPS.md`](reverse/TRAJECTORY_GROUPS.md).

### Resolved — Q-RENDER-001 late dynamic-palette ownership and scheduling

The formerly unclassified late helpers are palette presentation code, not gameplay/HUD state. `0x00403490` is the generic dynamic-palette animation kernel; `0x0041EFE0` initializes four purpose-built gameplay palette bands; `0x0041EE90` advances those bands; and `0x004011E0` is the inclusive DirectDraw palette-range upload wrapper. State 2 phase-slices settled uploads across exact palette ranges and falls back to `0..255` during transitions. The clean fidelity layer now models these algorithms semantically without reproducing the original packed `0x44` records. See [`reverse/PALETTE_EFFECTS.md`](reverse/PALETTE_EFFECTS.md).

### Q-LEVEL-001 — What registered/endgame world-transition semantics remain beyond the recovered shareware scroll?

The three-screen scenery buffer, exact cyclic gameplay scroll cadence, Ordering Information's modal reuse, Drone Y=-200 boss-approach gate, river/desert/isle/house/night transition table, and dedicated second-outcome shareware termination gate are now mapped. The branch-6 condition is now established as six clean disarms -> river/Mothership and any detonation -> results. Remaining work is exact original level/mission naming and retail-only reachability once lawful full-game evidence is available. See [`reverse/WORLD_SCENERY.md`](reverse/WORLD_SCENERY.md).


### Resolved — Q-RESULT-001 Win32 post-game/results control flow

The inline state-2 tail beginning at `0x004115BE` is now partitioned: lives<=0 entry, the independent results/Ordering-Information suppression flag, six numeric result statistics, 58-presentation confirmation lock, shareware Ordering Information handoff, exact high-score call mode and slot-zero no-save quirk, state1/state4 returns, and the six-disarm Mothership completion credits path are established and clean-tested. See [`reverse/POST_GAME_FLOW.md`](reverse/POST_GAME_FLOW.md) and [`reverse/HIGH_SCORES.md`](reverse/HIGH_SCORES.md).

## Medium — format/presentation completeness

### Resolved — Q-BOSS-001 pre-Drone boss-family dispatch

The six-entry dispatch is now family-classified as Lid/Top → Gemini → registered-slot-2 unknown → Spidey → Lid/Top reused → Bomber. Gemini, Spidey and Bomber lifecycle ownership is established; only the proper asset/name identity and missing loader for registered slot 2 remains open as `Q-BOSS-002`. See [`reverse/BOSS_PROGRESSION.md`](reverse/BOSS_PROGRESSION.md).


### Resolved — high-score statistics and DOS score semantics

The two former unknown Win32 arrays are now established as **Mothership destroyed** (`0x00440428`) and **Percentage hit** (`0x00446DE8`). DOS producer arithmetic and score-file ordering corroborate the same four logical numeric fields: Drones disarmed, score, Mothership destroyed, Percentage hit. See [`reverse/HIGH_SCORES.md`](reverse/HIGH_SCORES.md), [`reverse/MOTHERSHIP.md`](reverse/MOTHERSHIP.md), and [`formats/SCORES.md`](formats/SCORES.md).

### Q-MSHIP-001 — What are the complete Mothership encounter state semantics?

The load/core-hit/destruction milestones are established and `lid.jba` ownership is now resolved as a separate boss family. The complete hull/panel/hub/motor/core state machines, attack/movement behavior, late destruction thresholds, and live DOS destruction producer remain open. DOS `0x00085D10` is presentation-only, not the live +500 producer.

### Q-JBA-002 — What is the complete small-JBA/embedded-PCX container format?

At least several Windows-only small `.JBA` files contain embedded 128×128 8-bit PCX data. Determine surrounding header/metadata fields and all variants.

### Q-AUDIO-002 — Are all CLV→WAV pairs converted by the same stereo-to-mono rule?

The relationship is exact over compared common sample regions for nearly all matching assets examined in Phase 0/1. Build a corpus report that records length/tail exceptions explicitly.

## Phase 5 — DOS audio runtime

### Resolved — Q-AUDIO-003 canonical DOS digital voice count

The canonical DOS executable does not merely inherit the S.O.S. SDK default. `sosDIGIInitDriver` at `0x0008D4AF` allocates `0x1E00` bytes for voice records and explicitly writes `0x20` to driver `+0x14` at `0x0008D78A`. With the recovered `0xF0` voice-record size, this is exactly **32 digital voices**.

### Resolved — Q-AUDIO-004 allocation / priority / saturation policy

`sosDIGIStartSample` at `0x0008AC82` scans voice indices from zero upward and selects the first record without `_SACTIVE`. The caller's full `0xF0` descriptor is copied into that voice and the returned handle is the voice index. If all configured voices are active, start returns `0xFFFFFFFF`; there is **no priority comparison and no voice-steal fallback** in this allocation path. This is intentionally different from Win32 Drone's 20-buffer first-free/else-slot-0 pool selector.

### Resolved — Q-AUDIO-005 native HMI volume representation

Drone DOS writes HMI-native packed channel values directly. `sosDIGISetSampleVolume` at `0x0008AFC1` stores the caller's packed value into active voice `wVolume` without a universal conversion stage. Canonical producers use values such as `0x41004100`, `0x32003200`, `0x70007000`, `0x7FFF7FFF`, and low-word mono levels such as `0x3000`. The main-menu `lowbees.clv` fade advances a native level by `0x7D` per iteration and packs the equal-channel value before the control call. Therefore the Win32 `0..100 -> DirectSound attenuation` helper is not a cross-platform game-volume law.

### Resolved — Q-AUDIO-006 sustained-loop encoding

Matched sustained DOS descriptors write `wLoopCount = 0xFFFFFFFF` before `sosDIGIStartSample`. This is independently present for Gemini, `air.clv`, `retro1.clv`, `lowbees.clv`, `drone.clv`, and a local `thunder2.clv` presentation descriptor. Ordinary one-shots retain zero/default loop count. The clean backend contract therefore records zero as one-shot and `UINT32_MAX` as indefinite repetition rather than translating DirectSound Play flags.

### Q-AUDIO-007 — What exact stop/pause/restart lifecycle does Drone DOS use across all states?

The low-level ownership contract is now established: controlled samples retain the voice index returned by `sosDIGIStartSample`, and later use `sosDIGISampleDone`, `sosDIGIStopSample`, `sosDIGISetSampleVolume`, or `sosDIGISetSampleRate` with that handle. Air, Drone and Lowbees all have proven retained handles, and global teardown performs SampleDone-before-Stop on tracked voices. Remaining work is narrower: finish classifying every menu/modal raw-state branch and exact stop/restart ordering before declaring complete DOS presentation lifecycle parity.

## Evidence acquisition

### Q-FULL-001 — Which exact full/registered release should become the canonical retail reference?

When a lawful full release is obtained, hash and identify it before analysis. Do not mix retail addresses/content into shareware ledgers without a distinct evidence ID.

## Phase 2 closure

Phase 2 is complete. Its architecture questions have narrowed or resolved as follows:

- **Timing:** canonical DOS fidelity cadence is solved at ~70.0863 Hz from mode 13h plus one ordinary sync-tail retrace wait per logical update. Win32 QPC wall-clock behavior remains a separate nonblocking historical-port question.
- **State:** raw values `0..8`, `13`, and `99` are semantically mapped and represented by a narrow clean protocol module.
- **FLY/trajectory:** X/Y/AUX, path index/step/wrap, short-file reachability, group modes/counts/staggering, breakaway lifecycle, and all 17 static templates are solved. Dynamic special-family substitutions remain later integration/detail work.
- **Rendering/entity:** `Q-ENTITY-002` is resolved: Win32 `0x154` ↔ DOS `0x14F` core layout/correspondence, combat tail, contextual overlays and reserved block are mapped. Only subsystem-specific behavior and the separate exact collision-edge quirk remain open.
- **Determinism:** Win32 `srand`/`rand` are anchored; seed ownership and call-order parity still need recovery before PRNG can become a trace oracle.
