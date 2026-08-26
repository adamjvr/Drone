# Open Questions

This is the human-readable research queue. Machine-readable tracking lives in `reverse/ledger/open_questions.csv`. Questions should be closed only when the evidence and resulting documentation/implementation are linked.

## Critical — blocks simulation architecture

### Q-TIME-001 — What is the intended simulation cadence?

Win32 mechanically waits for a low-32-bit `QueryPerformanceCounter` delta of at least 15,000 when the limiter is enabled, but the executable does not query counter frequency. Recover the DOS timer path and compare runtime behavior before asserting an Hz/FPS value.

### Q-STATE-001 — What do all values of `game_state_raw` mean?

The direct dispatcher handles 0..5, while 6, 7, 8, 13, and 99 are written elsewhere. Determine which are persistent states, transition requests, return/status codes, or sentinels.

### Q-ENTITY-001 — What structure uses the observed 0x14-byte cleanup stride?

Collect allocation, initialization, update, and destruction sites. Do not call it an enemy/entity structure until consumers establish the role.

## High — blocks gameplay reconstruction

### Q-INPUT-001 — What is the canonical input bit/action representation?

Map DirectInput polling and DOS keyboard/joystick behavior to player movement, rapid fire, shield, probe/stinger selection/launch, pause, and escape.

### Q-FLY-001 — What do the three FLY record fields represent?

Storage is confirmed as `int16,int16,int8`. Locate consumers and correlate records with runtime object motion/script progression.

### Q-DEMO-001 — What are the 14 demo fields?

Locate playback/recording routines and correlate per-record changes. Determine whether records represent input, state snapshots, timing, RNG, or a mixture.

### Q-RENDER-001 — Which routines implement core software blitting/HUD composition?

Name framebuffer source/destination pointers, pitches, transparent/color-key semantics, clipping, and object coordinate conventions.

### Q-LEVEL-001 — How are scrolling scenery, encounter sequencing, and level completion represented?

Map level initialization, scenery assets, script/trajectory selection, counters, and transition conditions.

## Medium — format/presentation completeness

### Q-JBA-002 — What is the complete small-JBA/embedded-PCX container format?

At least several Windows-only small `.JBA` files contain embedded 128×128 8-bit PCX data. Determine surrounding header/metadata fields and all variants.

### Q-AUDIO-002 — Are all CLV→WAV pairs converted by the same stereo-to-mono rule?

The relationship is exact over compared common sample regions for nearly all matching assets examined in Phase 0/1. Build a corpus report that records length/tail exceptions explicitly.

## Evidence acquisition

### Q-FULL-001 — Which exact full/registered release should become the canonical retail reference?

When a lawful full release is obtained, hash and identify it before analysis. Do not mix retail addresses/content into shareware ledgers without a distinct evidence ID.
