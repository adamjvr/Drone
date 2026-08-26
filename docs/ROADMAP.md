# Drone Roadmap

The roadmap is compatibility-first: recover behavior, prove it, then layer remaster features on a stable core. Every milestone updates code **and** durable research documentation.

## M0 — Evidence intake and format reconnaissance — DONE

### Exit criteria met

- canonical DOS and Windows shareware packages hashed;
- DOS Watcom/DOS4GW and native Win32 executable identities established;
- known Windows Wise installer payload safely reconstructed without execution;
- initial core proprietary formats structurally identified;
- evidence kept outside Git.

## M1 — Executable reconstruction + native asset core — DONE

### Delivered

- clean C++20 implementations/tests for confirmed JBA/CLV/FLY/demo structures;
- native `drone_inspect` tool;
- Win32 startup/render/message loop map;
- top-level state dispatcher map;
- Win32 QPC limiter behavior mapped mechanically;
- initial DOS/Windows function anchors;
- Ghidra label script;
- comprehensive provenance/research methodology/ledger documentation.

### Exit criteria met

- repo builds/tests without original data;
- canonical local evidence set reconstructs reproducibly;
- original JBA/CLV/FLY/demo data can be inspected by our code;
- major Phase 1 findings have durable evidence records.

## M2 — Timing, subsystem recovery, and fidelity host — NEXT

### Objectives

- recover DOS timer source and establish intended simulation cadence/update-render relationship;
- classify state protocol and split active gameplay region into behavioral subsystems;
- map input aggregation/canonical actions;
- recover framebuffer/blitters/HUD enough to define sprite/coordinate contracts;
- begin entity pool/structure field recovery;
- recover FLY/demo consumer semantics as evidence permits;
- add first interactive modern host;
- present original 320×200 indexed framebuffer through the new host;
- output original reference audio through a modern host path;
- introduce deterministic fixed-tick harness only after cadence is proven.

### Exit criteria

- modern host opens and runs a deterministic core loop on desktop targets;
- fidelity framebuffer/palette can be presented live from project code;
- canonical input abstraction exists;
- simulation cadence is documented with evidence or explicitly remains isolated behind a temporary research setting;
- at least one gameplay subsystem has clean state/trace validation against reference behavior.

## M3 — Menus/HUD/player reconstruction

### Objectives

- title/menu/options/high-score flows;
- player movement and rapid fire;
- shield, lives, score/extra-life behavior;
- target selection and special-weapon controls;
- HUD composition;
- trace/framebuffer regression points.

### Exit criteria

A user can navigate the shareware front-end and control the player in a reconstructed gameplay scene with documented parity for implemented rules.

## M4 — Entity/enemy/combat simulation

### Objectives

- entity pools and update order;
- collisions/projectile rules;
- stinger/probe homing and attachment/disarm behavior;
- enemy trajectories and FLY semantics;
- explosions;
- Drone and shareware boss behavior.

### Exit criteria

Core shareware combat interactions run entirely in `drone_core` and are covered by deterministic behavioral tests/traces.

## M5 — Level 1/2 behavioral parity

### Objectives

- scrolling scenery and encounter sequencing;
- mission transitions;
- sound/event timing;
- demo system as deterministic regression fixture;
- complete available shareware playthrough.

### Exit criteria

Levels available in the canonical shareware evidence set can be completed under the reconstructed engine with documented known discrepancies approaching zero.

## M6 — Full-game reconstruction

**Dependency:** a lawfully obtained complete registered/retail evidence set must be identified and hashed as a separate canonical build.

### Objectives

- map levels 3–7 and missing mission/boss/Drone/mothership behavior;
- extend correspondence/asset manifests without checking proprietary payload into Git;
- achieve full-game fidelity parity.

## M7 — Remaster layer

### Objectives

- high-resolution renderer/art pipeline;
- modern controller and touch UX;
- accessibility/display/audio options;
- save/settings modernization;
- optional replacement asset packs;
- fidelity mode remains available and validated.

## M8 — Platform hardening and release

### Objectives

- Linux/macOS/iPadOS/Windows validation;
- CI and local reference parity suite;
- packaging/signing/notarization as applicable;
- crash/lifecycle/controller/audio hardening;
- source license and asset-distribution rights resolved;
- public developer/user documentation and release engineering.
