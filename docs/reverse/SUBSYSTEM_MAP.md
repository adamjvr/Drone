# Evolving Engine Subsystem Map

This map separates what is known about the original executables from the desired clean architecture. Names ending in `candidate` are research aids, not settled historical names.

## Win32 outer loop — high confidence

```text
0x00404E30  winmain-like
   |
   +--> 0x00404B60  DirectX/platform init
   |       +--> DirectDraw / 320x200x8 / primary surface / palette
   |       +--> 0x004068E0 DirectInput creation
   |       +--> 0x00406650 DirectSound init
   |
   +--> message pump
   |
   +--> lock/acquire surface
   |       +--> update framebuffer pointer/pitch globals
   |
   +--> 0x0040BA50 game_dispatch_update
   |
   +--> unlock/present
   |
   +--> shutdown paths
           +--> 0x00404D60 DirectDraw shutdown
           +--> 0x0040B380 game shutdown
```

## Game-state layer — partial

`0x0040BA50` begins with a six-entry direct dispatch on `game_state_raw` (`0x0042B188`). State 2 enters the large active gameplay path. Values outside 0..5 are also written by other routines, so the variable is currently modeled as a protocol value rather than a closed enum.

See `reverse/windows/state_machine.md`.

## Timing — partial

The active gameplay path contains the QPC limiter documented in `docs/TIMING.md`. Intended simulation Hz and update/render coupling remain unresolved.

## Input — partial

- DirectInput creation: `0x004068E0`.
- Candidate poll routine: `0x00406AC0`.
- Canonical action/bit representation: open.
- DOS keyboard/joystick correspondence: open.

## Rendering — partial/open

Confirmed host-facing facts:

- logical display mode is 320×200×8 indexed;
- JBA full-screen assets decode to the same logical buffer shape;
- `0x004D9584` is a medium-confidence current surface-pixel pointer candidate;
- `0x004DA780` is a medium-confidence surface-pitch global candidate.

Still to recover:

- sprite storage/frame metadata;
- blitter families;
- clipping/transparency semantics;
- HUD composition;
- scrolling background/scenery composition;
- coordinate conventions.

## Audio — partial

- DOS uses HMI-era audio paths and CLV sample data.
- Windows uses RIFF/WAV plus DirectSound.
- Win WAV loader: `0x00406200`.
- DirectSound init: `0x00406650`.

Gameplay-level sound event semantics and channel/priority behavior remain to be mapped.

## Entities/gameplay — open/partial

Research has observed a 0x14-byte stride in a cleanup context, but its exact structure role is not yet established. Player, enemies, projectile, probe, stinger, Drone, explosions, collisions, target selection, and boss logic remain Phase 2+ recovery areas.

## Levels/scripts — open/partial

FLY physical records are known, but field semantics and ownership are not. Executable strings establish a broader level/mission asset namespace than the shareware corpus supplies. Scrolling/encounter sequencing is not yet reconstructed.

## Demo system — open/partial

Demo DAT record width is known (14 signed integers), but playback/recording functions and field semantics remain open. This subsystem is a high-value future deterministic validation source.
