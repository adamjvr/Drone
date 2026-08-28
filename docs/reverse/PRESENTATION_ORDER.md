# Win32 Gameplay Presentation Order

Phase 3 refines the Phase-2 simulation/presentation boundary into an explicit
ordinary state-2 draw/palette/present sequence. This document records **relative
presentation order**. It does not claim that every conditional actor in a
sprite batch is emitted on every frame.

The sharp simulation-to-presentation boundary remains the call at `0x004100D8`
to `compose_scrolling_world_viewport` (`0x004033D0`). From that point to the
final `0x004115A5` framebuffer present, the canonical Win32 path is:

| order | presentation pass | evidence span | target |
|---:|---|---|---|
| 1 | compose 320x600 world viewport | `0x004100D8` | indexed framebuffer |
| 2 | ordinary transparent sprite batch A | `0x004100F1..0x0041089F` | indexed framebuffer |
| 3 | debris-particle pixels | `0x004108BD` | indexed framebuffer |
| 4 | ordinary transparent sprite batch B | `0x004108F2..0x0041095E` | indexed framebuffer |
| 5 | Drone detonation radial noise | `0x004109AA..0x004109C9` | indexed framebuffer |
| 6 | ordinary transparent sprite batch C | `0x00410A0C..0x00410BF3` | indexed framebuffer |
| 7 | scaled transparent overlays | `0x00410C3E..0x00410E95` | indexed framebuffer |
| 8 | score/lives text | `0x00410FCE..0x00411012` | indexed framebuffer |
| 9 | six-Drone outcome strip | `0x00411080` | indexed framebuffer |
| 10 | auxiliary HUD sprite | `0x004110D7` | indexed framebuffer |
| 11 | special-target overlay | `0x004111A4` | indexed framebuffer |
| 12 | shield meter | `0x004111AC` | indexed framebuffer |
| 13 | active player-shield overlay | `0x004111DE` | indexed framebuffer |
| 14 | Probe/Stinger status text | `0x00411351..0x004113AE` | indexed framebuffer |
| 15 | dynamic palette animation | `0x00411402..0x00411448` | working palette |
| 16 | optional Win32 QPC pacing | `0x00411463..0x00411494` | host |
| 17 | phase-sliced or full palette upload | `0x004114F6..0x00411556` | host palette |
| 18 | software-framebuffer present | `0x004115A5` | host surface |

## Why the sprite batches remain batches

The state-2 renderer contains dozens of conditional `0x00401660` calls for
trajectory members, bosses, the player, Drone, projectiles and effect sprites.
Many individual roots are already named, but Phase 3 does not need to invent a
single universal "scene graph" before every conditional root has complete
ownership semantics. The important compositor fact is already exact: debris
particles and Drone detonation noise are inserted **between** specific ordinary
sprite spans, scaled overlays come after those spans, and HUD/shield content is
painted after world actors/effects.

This ordering matters because all of these paths mutate the same indexed
320x200 framebuffer and palette index zero is transparent for the sprite
blitters. Reordering them changes visible occlusion even when simulation state
is identical.

## Late HUD ordering

The recovered tail also establishes useful HUD precedence:

```text
score/lives text
  -> Drone outcome strip
  -> auxiliary HUD sprite
  -> special-target overlay
  -> shield meter
  -> active shield overlay
  -> Probe/Stinger status text
```

The player-shield effect is therefore an overlay **after** the shield meter,
not part of the world-sprite batch.

## Palette and host separation

Dynamic palette mutation happens only after framebuffer drawing is complete.
The optional QPC limiter then runs before the DirectDraw palette ranges are
uploaded, and the software framebuffer is copied to the locked surface last.
The clean engine therefore keeps three domains separate:

1. indexed framebuffer composition;
2. working-palette mutation;
3. host pacing/palette upload/present.

The portable contract lives in:

- `include/drone/fidelity/presentation_order.hpp`
- `src/fidelity/presentation_order.cpp`

It intentionally stores original instruction-address spans as evidence anchors,
not as runtime dependencies.
