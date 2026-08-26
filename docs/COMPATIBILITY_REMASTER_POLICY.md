# Compatibility and Remaster Policy

## Principle

Drone is being reconstructed once and presented in two layers:

- **fidelity/compatibility behavior** — the best-supported reconstruction of original game rules and original logical rendering/audio events;
- **remaster presentation/features** — optional modern improvements built around that stable behavioral core.

The remaster must not become an excuse to stop learning what the original did.

## Fidelity core owns

- simulation tick/update order;
- player/enemy/projectile rules;
- collision behavior;
- scoring/lives/shield/special-weapon rules;
- level sequencing;
- deterministic RNG behavior if/when recovered;
- original asset logical IDs and palette semantics;
- original 320×200 indexed framebuffer path;
- gameplay audio event timing;
- demo behavior;
- compatibility quirks that materially affect gameplay.

## Host/remaster layer owns

- OS window lifecycle;
- display scaling and presentation;
- controller/touch mapping into canonical game actions;
- output audio device and buffering;
- filesystem locations;
- platform settings UI;
- high-resolution artwork/rendering;
- interpolation and visual filters that do not feed back into simulation;
- accessibility/display options;
- modern save/config UX.

## Rule changes

If a modern option intentionally changes game rules—e.g. revised collision, aim assistance, extra checkpoints, altered difficulty—it must be explicit and separable from fidelity mode. A rule change must never quietly replace the reconstructed original behavior.

## Logical rendering reference

The compatibility renderer should be able to produce:

```text
320 × 200 palette indices + 256-entry palette
```

regardless of the physical output resolution. The host may then scale with integer/aspect-correct presentation or feed semantic data to a high-resolution renderer.

This reference path is a testing instrument as much as a visual mode.

## Asset replacement

Original asset decoding and replacement/remaster assets should converge on logical asset interfaces. The simulation should not depend on whether a sprite came from an original JBA, a converted cache, or a legally distributable remaster pack.

## Input normalization

Modern keyboard, gamepad, and touch inputs map to canonical actions. Input-device polling quirks from DirectInput or DOS hardware should not leak into core game logic unless a specific original behavior depends on them.

## Saves and settings

Modern persistence formats may differ from original storage as long as they preserve the required game state. Original high-score/config file import can be implemented as adapters rather than forcing the modern engine to keep unsafe or platform-specific storage layouts.
