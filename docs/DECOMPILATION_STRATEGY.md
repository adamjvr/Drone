# Decompilation Strategy

## Objective

Recover original behavior with enough evidence that the clean implementation can be tested for parity, while avoiding a codebase that merely mirrors decompiler output or original platform APIs.

## Why the Win32 build is primary

The 1999 binary is a compact PE32/i386 image with ordinary Win32/DirectX imports. Calls into the OS and Microsoft runtime provide strong analysis anchors. It is therefore the fastest route to high-level game logic and platform/game boundaries.

## Why the DOS build remains an equal witness

The DOS build is the earlier Watcom/DOS4GW implementation. Shared algorithms can be compared across independently compiled binaries. A behavior visible in both builds is less likely to be a compiler/decompiler artifact.

This dual-build method has already strengthened:

- JBA decoding;
- common asset namespace/load relationships;
- FLY physical layout;
- separation of game behavior from platform-specific audio/video code.

## Analysis layers

### 1. Binary/platform shell

Identify and fence off:

- startup/shutdown;
- memory/runtime functions;
- DirectX/HMI/VGA/keyboard/joystick plumbing;
- file I/O;
- timing sources.

These paths define host contracts but generally do not belong in `drone_core`.

### 2. Data contracts

Recover loaders/writers and structure layouts before interpreting high-level behavior. Correct data types make later decompilation dramatically more reliable.

### 3. Frame/update boundaries

Recover state dispatch, timing, input aggregation, render-buffer acquisition, and any update/render separation. Do not design the clean scheduler until this contract is established.

### 4. Observable systems

Menus, HUD, player controls, scoring, sound events, and framebuffer blitters give externally testable anchors.

### 5. Dynamic gameplay systems

Recover entity allocation/pools, update ordering, collision, projectiles, target/probe/stinger behavior, enemies, Drone behavior, bosses, and explosions.

### 6. Level/demo systems

Recover scrolling/encounter sequencing, FLY consumers, level transitions, demo playback/recording, and deterministic trace opportunities.

## Function recovery order

1. startup/shutdown/platform glue;
2. file/audio/image loaders;
3. main dispatcher and timing;
4. input and menu paths;
5. framebuffer/blitters/HUD;
6. entity allocation/update/collision;
7. player/projectile/probe/stinger;
8. enemies, FLY consumers, Drone/boss logic;
9. level sequencer/scrolling;
10. demos and full trace equivalence.

## Giant-function policy

The original compiler may have large functions that represent several logical systems. We do not need to reproduce those boundaries. First identify behavioral regions and data dependencies; then express them as maintainable clean subsystems while preserving update order and observable semantics.

## Promotion criteria

A routine/field should move from hypothesis to stable semantic name only when supported by callers/callees, data flow, constants/assets, cross-build correspondence, runtime evidence, or exact output behavior. See `docs/RE_HANDBOOK.md` for the complete finding lifecycle.

## Desired output of decompilation

The final artifact is **not** a giant body of decompiler-generated C. It is:

- a well-documented map of the original binaries;
- independent specifications of data and behavior;
- clean portable source;
- deterministic validation proving the clean behavior matches the reference where intended.
