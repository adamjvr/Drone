# Target Architecture

The clean implementation is intentionally independent of DOS4GW, Win32, DirectX, and any future UI/game framework.

```text
                     +------------------------------+
original evidence -> | reverse/ + specifications    |
                     +---------------+--------------+
                                     |
                                     v
+--------------------------------------------------------------------+
| drone_core                                                          |
| deterministic state | entities | collision | weapons | levels       |
| asset contracts      | indexed framebuffer | audio event semantics  |
+----------------------------+--------------------+-------------------+
                             |                    |
                 +-----------+------+     +-------+----------------+
                 | host/platform   |     | tooling / validation    |
                 | video/input     |     | trace/compare/extract   |
                 | audio/fs/life   |     | deterministic fixtures  |
                 +-----------------+     +--------------------------+
```

## Architectural goals

1. **One simulation core on every target.** Linux, macOS, iPadOS, and Windows must not fork gameplay behavior.
2. **Evidence-driven timing.** Simulation cadence is reconstructed before it is encoded in the scheduler.
3. **Deterministic compatibility path.** Host refresh rate, audio buffering, touch UI, and high-resolution presentation do not alter simulation results.
4. **Original-data adapters are optional.** The core consumes logical assets/data; original proprietary files are decoded by adapters/tools outside the rules engine.
5. **Remaster features wrap rather than replace fidelity behavior.** See `COMPATIBILITY_REMASTER_POLICY.md`.

## Fidelity rendering contract

Preserve a logical:

```text
320 × 200 indexed framebuffer
+ 256-entry palette
```

as the reference output path. This mirrors the original display/data model and creates a powerful deterministic comparison surface.

A high-resolution renderer can later render from semantic scene state or replacement assets, but fidelity rendering remains available for regression and original-style presentation.

## Simulation

The eventual simulation layer should own only behavior that can affect game state:

- canonical inputs;
- state transitions;
- player/entities/projectiles;
- collisions;
- score/lives/shield;
- Drone/probe/stinger logic;
- level/script progression;
- deterministic RNG if recovered;
- gameplay sound events.

Original memory layout does not dictate clean class/structure design, but original update order and numerical behavior may be compatibility requirements.

The recovered input boundary now makes `canonical inputs` concrete: `drone::gameplay::GameplayInputFrame` contains independent semantic booleans. Platform hosts OR their physical sources before handing the frame to the simulation; original demo playback replaces only the six recorded controls while leaving unrecorded vertical/meta input live. See [`reverse/INPUT.md`](reverse/INPUT.md).

## Host layer

The host owns:

- app/window lifecycle;
- physical input polling and action mapping;
- display presentation/scaling;
- audio device/output buffering;
- monotonic time source and scheduler integration;
- filesystem/config locations;
- OS lifecycle events.

Phase 2 now includes a dependency-light native fidelity host rather than committing the project to a cross-platform framework. The current host backends are X11 on Linux, Win32/GDI on Windows, and Cocoa/CoreGraphics on macOS. They all present the same `drone::fidelity::IndexedFramebuffer` produced by `drone_core`.

This native shell is deliberately small and replaceable. It proves the host boundary while keeping `drone_core` independent of X11, Win32, Cocoa, SDL, JUCE, or any other presentation framework. iPadOS remains a later host implementation using the same core/framebuffer contracts.

## Tooling/validation layer

Analysis tools should remain first-class targets. Long-term tools include:

- original package bootstrap/inventory;
- asset decoders/converters;
- runtime trace capture/normalization;
- framebuffer/palette diffing;
- audio event/sample comparison;
- corpus validators;
- demo-driven regression.

## Data ownership

Original assets remain local under `.reference/`. Checked-in manifests carry filenames/sizes/hashes, allowing exact corpus identification without distributing payload bytes.

## Threading

Do not introduce multithreaded simulation until original ordering is fully understood. Rendering/audio presentation may eventually use host threads, but compatibility state updates should begin single-threaded and deterministic.
