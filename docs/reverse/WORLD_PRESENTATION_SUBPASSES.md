# World Presentation Subpasses

Phase 3 originally preserved the Win32 state-2 world renderer as three coarse
transparent-sprite spans separated by particle/effect passes. Further recovery
shows that the first span is **not** a homogeneous sprite batch: it contains
boss composites, a fixed-point one-pixel particle bank, a Gemini procedural
surface effect, ordinary sprite pools, and two explosion families with explicit
unscaled/scaled routing.

This document refines that evidence without replacing the stable 18-pass outer
presentation contract in [`PRESENTATION_ORDER.md`](PRESENTATION_ORDER.md).
The clean engine records semantic subpasses; it does not reproduce the original
global-memory layout or infer a scene graph that the executable did not have.

## Canonical pre-scaled world/effect order

After the 320x600 world viewport is composed, the ordinary Win32 state-2 path
paints the following subpasses in order before the scaled-overlay stage:

| order | subpass | primitive | evidence | principal root / pool |
|---:|---|---|---|---:|
| 1 | Mothership composite | conditional sprite composite | `0x004100DD..0x00410230` | core owner `0x00472598` |
| 2 | Stinger entity | transparent sprite | `0x00410233..0x0041024C` | `0x00434C10` |
| 3 | Lid/Top composite | conditional sprite composite | `0x0041024F..0x0041027D` | `0x00446E00` |
| 4 | registered boss slot 2 | conditional sprite composite | `0x00410280..0x004102DC` | `0x00441618` |
| 5 | Spidey composite | conditional sprite composite | `0x004102DF..0x0041033B` | owner `0x004402D0` |
| 6 | Bomber composite | conditional sprite composite | `0x0041033E..0x004103B6` | owner `0x00464BE8` |
| 7 | paired fixed-actor pool | transparent sprite pool | `0x004103B9..0x0041043C` | `0x0047E288` / `0x0043F5F8` |
| 8 | fixed-point point-particle bank | direct one-pixel writes | `0x0041043E..0x0041049B` | `0x00434D80` |
| 9 | Drone | transparent sprite | `0x0041049D..0x004104B7` | `0x00446080` |
| 10 | flare | transparent sprite | `0x004104B7..0x004104D4` | `0x00440E00` |
| 11 | chute | transparent sprite | `0x004104D7..0x004104F0` | `0x0045BDA8` |
| 12 | Probe/Stinger projectile | transparent sprite | `0x004104F3..0x00410511` | `0x0045A148` |
| 13 | Gemini procedural beam/surface effect | procedural framebuffer + host-surface effect | `0x00410514..0x004107AE` | Gemini pair |
| 14 | Gemini body/head A | sprite composite | `0x004107AE..0x004107DB` | `0x00467538` / `0x00464D40` |
| 15 | Gemini body/head B | sprite composite | `0x004107DE..0x0041080C` | `0x00467690` / `0x00464E98` |
| 16 | `miniexp1` pool, unscaled members | transparent sprite pool | `0x0041080F..0x00410864` | `0x00480318` |
| 17 | `explode1` pool, unscaled members | transparent sprite pool | `0x00410866..0x004108BB` | `0x00446FC8` |
| 18 | debris particle pixels | direct pixel effect | `0x004108BD` | particle groups |
| 19 | sprite-debris triplet loop | transparent sprites | `0x004108C2..0x00410974` | `junk1` / `junk2` / `wheel` |
| 20 | Drone detonation radial noise | direct pixel effect | `0x0041097A..0x004109CE` | procedural |
| 21 | `retro.jba` sprite A | transparent sprite | `0x004109D1..0x00410A11` | `0x004673E0` |
| 22 | `retro.jba` sprite B | transparent sprite | `0x00410A14..0x00410A53` | `0x00438C80` |
| 23 | active trajectory-group entities | trajectory sprite batch | `0x00410A56..0x00410AEB` | `0x00495CF0` |
| 24 | rapid missiles | transparent sprite pool | `0x00410AF1..0x00410B3C` | `0x0042F200` |
| 25 | enemy bombs | transparent sprite pool | `0x00410B3E..0x00410B89` | `0x004651A0` |
| 26 | player | transparent sprite | `0x00410B8B..0x00410BA5` | `0x00466B18` |
| 27 | player-destruction `explode1` sprite | transparent sprite | `0x00410BA8..0x00410BC1` | `0x00491CE0` |
| 28 | secondary 15-slot impact/effect pool | transparent sprite pool | `0x00410BC4..0x00410C0B` | `0x004605A0` |

The corresponding portable catalog is implemented by
`canonical_win32_world_presentation_subpasses()` in
`include/drone/fidelity/world_presentation_subpasses.hpp`.

## Point-particle bank

The render loop at `0x0041043E` establishes a distinct 650-record bank at
`0x00434D80`. Session setup writes capacity `650` to `0x0042B1A0`, clears the
`+0x15` active byte in all `0x18`-byte records, and clears active count
`0x004D95F0`.

The record prefix is compatible with the established particle layout:

```text
+0x00  fixed-point X
+0x04  fixed-point Y
+0x08  velocity/step X
+0x0C  velocity/step Y
+0x14  palette/visual code
+0x15  active byte
```

Many unrelated encounter/effect producers allocate free records. The renderer
shifts X/Y by 16 and writes `visual_code` as one palette-index byte directly to
the software framebuffer. Because producer ownership is broad, the clean name
is deliberately `point_particle_bank`, not a guessed effect name.

## Sprite debris is interleaved

The post-particle sprite-debris path is one 15-index loop. For each slot `i` it
conditionally draws, in exact order:

```text
junk1[i]
  -> junk2[i]
  -> wheel[i]
```

It is therefore incorrect to model this as “render the whole junk1 pool, then
the whole junk2 pool, then the whole wheel pool.” The ordering can matter when
active sprites overlap.

## Gemini procedural effect

When both Gemini halves are active, `0x00410514..0x004107AE` computes the
horizontal interval between the two head entities and paints a randomized band
into the indexed framebuffer. If the active player lies inside the interval,
the path performs 55 outer iterations of six randomized direct writes to the
locked DirectDraw surface, invokes `0x0041EB70`, blits the player to the locked
surface, copies the software framebuffer to the temporary JBA sheet buffer,
and calls the presentation helper at `0x004018F0`.

This is a special procedural Gemini inter-head effect, not an ordinary sprite
batch. The project uses the descriptive clean name `GeminiProceduralBeam`
without claiming an original developer term such as “laser” or “electricity.”

## Explosion family routing

Two established sprite families are painted immediately before debris
particles when common-entity family flag `+0x14E == 0`:

- `0x00480318` — `miniexp1.jba` family;
- `0x00446FC8` — `explode1.jba` family.

Members with `+0x14E == 1` are omitted here and routed to the later scaled
transparent-overlay pass through `0x00403460`. This is an explicit original
unscaled/scaled presentation split, not a modern renderer policy.

`0x00491CE0` is a separate singleton used by the player-destruction sequence.
Its frame-pointer table is copied from the `explode1` source entity, so it is
named `player_destruction_explosion_entity` rather than treated as a new asset
family.

The 15-entry pool at `0x004605A0` is also separate. It has a recovered update
and render lifecycle and many boss/effect producers, but exact frame-source
ownership remains unproven. It stays `secondary_impact_sprite_pool` rather
than being mislabeled as `miniexp`.

## Evidence boundaries

The paired six-slot family rooted at `0x0047E288` / `0x0043F5F8` remains
semantically unnamed. Its update path is established and its paired rendering
order is exact, but asset/name evidence is not strong enough to invent an
identity.

Likewise, registered boss slot 2 remains a known composite with unknown proper
retail identity under `Q-BOSS-002`. Renderer recovery does not close that
content-evidence question.
