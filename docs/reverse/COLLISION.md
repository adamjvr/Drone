# Collision Reconstruction

This page records the first gameplay subsystem promoted from static Win32 evidence into clean `drone_core` code. The clean interface is semantic and does **not** copy the original `0x154` memory layout.

## Established primitives

### `0x00401F60` — point versus entity hitbox

The function consumes a point-like object (`+0` X, `+4` Y) and a common `0x154` entity. It returns true only when both coordinates lie inside the entity's collision extents:

```text
entity.x <= point.x <= entity.x + entity.collision_width_extent
entity.y <= point.y <= entity.y + entity.collision_height_extent
```

The right and bottom comparisons are inclusive in the original machine code.

`0x00401780` initializes those collision extents by multiplying the sprite width and height by the floating constant at `0x0042A000`, which is exactly IEEE-754 double `0.85`, and converting the results to integers through the Microsoft runtime helper at `0x00421E90`.

This establishes:

- entity `+0x28`: collision-width extent;
- entity `+0x2A`: collision-height extent.

### `0x00401FA0` — opaque sprite-pixel collision

This routine first places the point relative to entity X/Y and sprite width/height. It then selects:

```text
frame_pixels[current_frame]
```

and tests the corresponding 8-bit source pixel. A value of `0` is non-colliding/transparent; any non-zero palette index collides. This independently matches the transparent blitter's index-zero rule.

The original routine uses inclusive right/bottom comparisons before calculating the frame offset. That can make an exact edge coordinate address beyond the nominal `width * height` pixel payload. The clean implementation deliberately does **not** reproduce an unsafe out-of-bounds read: it requires the sampled coordinate to fall within the supplied frame span. Exact reachability/intent of the original edge case remains `Q-COLL-001`.

### `0x00402000` — Y+9 hitbox probe

This is mechanically the same inclusive hitbox test as `0x00401F60`, except it adds exactly nine pixels to the point's Y coordinate before testing. Its mechanism is established, but the gameplay-level meaning of the vertical offset is not yet named.

### `0x00402FC0` — centered hitbox versus full sprite rectangle

This later state-2 broad-phase helper is asymmetric. For the first entity it centers the established collision extents inside the sprite:

```text
left = x + ((sprite_width  - collision_width_extent)  >> 1)
top  = y + ((sprite_height - collision_height_extent) >> 1)
right  = left + collision_width_extent
bottom = top  + collision_height_extent
```

It then tests that rectangle against the **full sprite rectangle** of the second entity (`x/y/+0x20/+0x22`). All four separating-axis comparisons are inclusive. Multiple calls in the late collision/destruction block use this before more specific consequences/effects.

The clean name is `entity_hitbox_overlaps_sprite_rect()`, making the asymmetry explicit rather than pretending this is a generic symmetric AABB helper.

## Drone weapon callsites

Re-reading the late state-2 collision region resolves an important producer distinction. Both normal rapid missiles (`0x0040F206..`) and a launched Probe/Stinger (`0x0040F62D..`) test the active Drone through **`0x00401F60`**, so these paths do **not** require a Drone opaque-pixel mask. Drone root `0x00446080` is a 15×38 entity; the standard `0.85` collision initialization yields extents 12×32. The inclusive contract is therefore:

```text
drone.x <= projectile.x <= drone.x + 12
drone.y <= projectile.y <= drone.y + 32
```

Both destructive paths additionally require Drone activity 1 and destruction countdown `0x00491CAC > 99`. A rapid missile or red Stinger starts the shared countdown at zero. A blue Probe instead enters attached/decode state 2, awards +10 and initializes the exact two-stage decoder. This corrects the earlier project assumption that the weapon-to-Drone producer belonged to the opaque-pixel workstream; **trajectory actor hits still do require their extracted-frame sprite masks.**

## Clean implementation

`include/drone/gameplay/collision.hpp` and `src/gameplay/collision.cpp` provide:

- `point_in_hitbox`;
- `point_hits_opaque_pixel`;
- `point_plus_y9_in_hitbox`;
- `entity_hitbox_overlaps_sprite_rect`.

Synthetic tests verify inclusive hitbox boundaries, palette-index-zero transparency, positive opaque pixels, and the Y+9 probe. No original game data is required.

## Why this matters

Collision is the first Phase 2 subsystem where the path is now:

```text
binary instructions
    -> field semantics
    -> documented original contract
    -> clean framework-independent code
    -> regression tests
```

That workflow is the standard for player/projectile/enemy reconstruction going forward.
## Enemy-bomb to special-weapon use of `0x00402000`

The late state-2 bomb loop at `0x0040F35F..0x0040F4B8` independently confirms that `0x00402000` is not player-specific: it tests bomb `(x,y+9)` against the shared 3x8 Probe/Stinger common entity. The special entity's 0.85-derived collision extents are therefore 2x6, with the routine's established inclusive right/bottom comparisons. This producer is now integrated in `GameSession` and drives the attached-Probe decoder interruption path documented in `SPECIAL_WEAPONS.md`.
