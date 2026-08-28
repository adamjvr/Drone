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
| 8 | initial gameplay palette fade-in | `0x00410E9D..0x00410F34` | working palette |
| 9 | score/lives text | `0x00410FCE..0x00411012` | indexed framebuffer |
| 10 | six-Drone outcome strip | `0x00411080` | indexed framebuffer |
| 11 | current Drone-outcome cursor (`square.jba`) | `0x004110D7` | indexed framebuffer |
| 12 | special-target overlay | `0x004111A4` | indexed framebuffer |
| 13 | shield meter | `0x004111AC` | indexed framebuffer |
| 14 | active player-shield overlay | `0x004111DE` | indexed framebuffer |
| 15 | Probe/Stinger status text | `0x00411351..0x004113AE` | indexed framebuffer |
| 16 | dynamic palette animation | `0x00411402..0x00411448` | working palette |
| 17 | optional Win32 QPC pacing | `0x00411463..0x00411494` | host |
| 18 | phase-sliced or full palette upload | `0x004114F6..0x00411556` | host palette |
| 19 | software-framebuffer present | `0x004115A5` | host surface |

## Detailed world/effect ownership

The original three coarse sprite-span labels remain in the stable 19-pass outer
contract, but Phase 3 now has a finer evidence-backed decomposition of the
world/effect work they contain. In particular, the first span mixes ordinary
transparent sprites with a 650-record fixed-point point-particle bank and the
Gemini procedural surface effect, so treating it as one homogeneous sprite
batch would be inaccurate.

The detailed order is recorded in
[`WORLD_PRESENTATION_SUBPASSES.md`](WORLD_PRESENTATION_SUBPASSES.md) and in the
portable `canonical_win32_world_presentation_subpasses()` catalog. The recovered
ordering includes fixed boss composites, Drone/flare/chute/special-projectile
sprites, Gemini procedural work, unscaled explosion pools, direct particle
pixels, the interleaved junk1/junk2/wheel loop, Drone detonation noise,
retro/trajectory/projectile/player layers, and the late 15-slot impact pool.

This refinement does **not** replace the outer 19-pass contract. It nests inside
the existing framebuffer stages so callers can reason at either level without
copying original globals into the clean renderer.

## Scaled overlays and startup fade

The scaled block is now separately decomposed in
[`SCALED_OVERLAYS.md`](SCALED_OVERLAYS.md). It contains scaled members of the
`miniexp1` and `explode1` pools followed by the shared three-sprite objective-
destruction debris effect (`debris1.jba`, `debris2a.jba`, `debris3.jba`).

Immediately afterward, `0x00410E9D..0x00410F34` performs the initial gameplay
palette fade from black while the settlement counter is 0..60. This corrects
the earlier 18-pass catalog, which skipped the fade and jumped directly from
scaled overlays to HUD. The corrected portable catalog contains 19 passes.

## Late HUD ordering

The recovered tail also establishes useful HUD precedence. Exact geometry, text/timer semantics, reticle clamping, and shield-meter colors are reconstructed in [`HUD_PRESENTATION.md`](HUD_PRESENTATION.md):

```text
score/lives text
  -> Drone outcome strip
  -> current Drone-outcome cursor (square.jba)
  -> special-target overlay
  -> shield meter
  -> active shield overlay
  -> Probe/Stinger status text
```

The player-shield effect is therefore an overlay **after** the shield meter,
not part of the world-sprite batch.

## Palette and host separation

Palette ownership has two stages: the startup fade occurs after world/scaled drawing but before HUD, while the later dynamic palette animation occurs after all framebuffer drawing is complete.
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

## Framebuffer comparison boundary

The presentation contract makes the local fidelity oracle explicit: after all indexed-framebuffer drawing is complete, the clean project can snapshot the 64,000 index bytes plus the current working palette before host-surface behavior is allowed to obscure the result. Phase 3 encodes that oracle as the local-only `DRONEFB1` format and compares it with `drone_framecheck`; see [`../FRAMEBUFFER_VALIDATION.md`](../FRAMEBUFFER_VALIDATION.md).
