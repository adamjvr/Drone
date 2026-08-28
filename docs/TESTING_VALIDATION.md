# Testing and Behavioral Validation Strategy

## Goal

A decompilation/remaster can compile and still be wrong. Drone therefore uses progressively stronger validation layers, from format-level unit tests to deterministic game-state and framebuffer comparisons against the original executables.

## Validation classes

### V0 — Build/static sanity

Required on every supported development host:

- CMake configure succeeds;
- production sources build with warnings enabled;
- test binary builds;
- repository contains no required dependency on `.reference/` for ordinary compilation.

### V1 — Synthetic unit tests

Tests use project-created fixtures only. Current examples:

- encode a known 320×200 pixel pattern into recovered JBA lane order, decode it, and require exact pixel equality;
- verify RGB6 palette expansion behavior;
- decode a synthetic CLV stream and verify exact frame count/downmix bytes;
- parse both counted `CURRENT.FLY` data and raw trajectory triples, including negative values and canonical short-read quirks;
- parse multiple 14-field demo records.

Synthetic tests are publishable and must remain the baseline regression suite.

### V2 — Local reference corpus checks

Optional tests/tools operate on `.reference/` after package hash verification. They may check:

- exact expected executable hashes;
- inventory counts;
- known file sizes;
- JBA dimensions/decoded output hashes;
- CLV↔WAV sample relationships;
- FLY physical counts, recovered loader counts, ranges, and DAT record counts/ranges.

These tests must fail clearly when the evidence binary is not the expected hash rather than silently comparing a different release.

### V3 — Cross-build algorithm correspondence

Where DOS and Windows independently implement the same behavior, compare the recovered contracts. Examples already established:

- JBA palette and 10-lane pixel decoding;
- FLY storage layout family;
- common asset namespaces;
- DOS CLV / Windows WAV conversion relationship for compared assets.

Cross-build agreement substantially raises confidence because compiler/platform-specific decompiler artifacts are less likely to match accidentally.

### V4 — Runtime trace comparison

Planned Phase 2+ infrastructure should record deterministic checkpoints from reference execution and the clean engine. Candidate trace fields:

- simulation tick;
- raw/normalized input mask;
- top-level game state;
- player position/state;
- entity counts and selected entity fields;
- score/lives/shield state;
- Drone/probe/stinger state;
- level/script cursor;
- palette generation/change counter;
- audio event IDs.

Trace formats should be simple, versioned, and diffable. Never make pointer values or host timestamps part of the canonical clean trace.

### V5 — Framebuffer comparison

The fidelity renderer preserves a logical 320×200 indexed image. This gives us a strong output oracle.

At chosen deterministic ticks:

1. capture the 64,000 index bytes and active palette;
2. wrap them in the clean `DRONEFB1` local snapshot format;
3. compare indexed pixels, resolved RGB, palette entries, and minimal mismatch bounds with `drone_framecheck`;
4. record SHA-256-only metadata with `scripts/framebuffer_fixture.py`;
5. if an intentional platform/presentation difference prevents full equality, compare documented regions rather than weakening the test globally.

Actual original-runtime `.drfb` files remain local evidence and are ignored by Git. See [`FRAMEBUFFER_VALIDATION.md`](FRAMEBUFFER_VALIDATION.md). The remaster renderer is validated separately; it must not become the only way to inspect fidelity behavior.

### V6 — Audio/event comparison

Separate **gameplay audio events** from **host audio rendering**.

Compatibility tests should first compare which sound is triggered and when. Where original sample data is user-supplied, decoded sample buffers can additionally be compared. Host mixer resampling, device buffering, and output latency must not be mistaken for simulation differences.

### V7 — Demo-driven regression

The original demo files are now established as 2,101-frame hybrid deterministic replays. Channels 1–6 replay selected controls, 7–9 inject trajectory-group script data, 10–12 recreate enemy-bomb spawn/X/Y checkpoints, and 13–14 force Drone X/Y. Four canonical DOS demos are byte-for-byte identical to their Windows copies, so they are unusually strong cross-build behavioral fixtures.

The clean parser preserves raw 32-bit textual values while the semantic decoder reproduces the original loader's low-byte/low-word narrowing. Replay validation should be layered: first assert channel interpretation and cross-build identity, then consume the proven checkpoints in reconstructed gameplay slices, and finally compare whole-frame state/framebuffer traces as more of the simulation becomes complete.

A critical original quirk is itself a test vector: live bomb spawn assigns `rand()%3` horizontal steering, whereas demo playback restores bomb X/Y and explicitly forces steering to zero. Tests must preserve such replay-specific substitutions rather than demanding that replay initialization look identical to live initialization.

## Parity gates

A subsystem should not be marked reconstructed solely because it looks correct. Suggested gate:

1. binary path documented;
2. inputs/outputs specified;
3. clean code implemented;
4. synthetic edge tests added;
5. at least one reference comparison performed when possible;
6. discrepancies documented rather than normalized away.

## Timing validation

Timing is a first-order behavior. Before fixing a simulation rate:

- use the recovered DOS vertical-retrace primitive at `0x0006940C` to identify which gameplay call sites are frame gates and which are presentation-only waits;
- understand whether update and render cadence are coupled;
- characterize the Win32 QPC threshold on representative hardware/runtime traces rather than assuming a counter frequency;
- identify any accumulator/frame-skip or limiter-disable behavior, including the documented Tab toggle;
- verify movement/projectile rates over multiple seconds, not one frame.

The presence of a VGA vertical-retrace wait is confirmed; the final simulation Hz/FPS is intentionally still unresolved.

## Numerical fidelity

Preserve integer widths, truncation, signedness, overflow-sensitive behavior, and fixed-point operations until tests show they can be safely abstracted. Modern floating point should not replace original integer math merely for convenience in the fidelity core.

## CI direction

When Phase 2 host dependencies are introduced, CI should have two levels:

- **public CI:** build + synthetic tests, no original game data;
- **local/private reference suite:** same build plus user-supplied evidence comparisons.

The public repository must remain testable without copyrighted original payloads.


## Durable phase-exit gates

The normal CTest suite includes repository-level architecture gates:

- `scripts/check_phase2_exit.py` — preserves the completed gameplay-architecture baseline even after later phases advance;
- `scripts/check_phase3_exit.py` — requires the resolved Phase-3 renderer blockers, corrected 19-pass presentation contract, scaled/HUD/framebuffer/host artifacts, and roadmap advancement to Phase 4.

These gates intentionally do not demand later exact trace parity, retail-only content, or production platform hardening. Those requirements belong to their later roadmap phases.

## Phase-4 whole-session oracle

`drone_game_session` tests reset/ownership and continuous tick semantics directly. `drone_game_session_probe` runs an asset-free 120-update script and compares the complete semantic-state summary exactly. This is the first regression gate above isolated subsystem tests; it does not replace later Phase-6 original-runtime trace parity.
