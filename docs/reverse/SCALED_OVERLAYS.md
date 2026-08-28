# Win32 Scaled Overlay Presentation

Phase 3 resolves the state-2 scaled-overlay block at `0x00410C15..0x00410E9D`.
It contains two pool-routing passes followed by a dedicated three-sprite
objective-destruction effect. All of this occurs after the ordinary world/effect
subpasses and before the initial gameplay palette fade and HUD.

## Explosion-pool scaled routing

Two common-entity pools are visited in order:

| order | pool root | capacity | asset family | render gate |
|---:|---:|---:|---|---|
| 1 | `0x00480318` | 110 | `miniexp1.jba` | `activity_state == 1 && +0x14E == 1` |
| 2 | `0x00446FC8` | 165 | `explode1.jba` | `activity_state == 1 && +0x14E == 1` |

The pool records are the same common Win32 `0x154` entities used by the
unscaled explosion passes. The contextual byte at `+0x14E` is therefore an
established **scaled-render routing flag for these effect families**. It remains
family-contextual globally because the Probe/Stinger family reuses that byte for
a different special-weapon purpose.

Matching active records call `blit_entity_scaled_transparent` (`0x00403460`),
which derives the destination rectangle from `x/y` and `render_width/
render_height` and forwards to the clipped 16.16 scaled blitter.

## Shared objective-destruction scaled debris

A single byte at `0x00441A6A` gates three dedicated scaled sprites:

| order | entity | asset | source size | frames | initial velocity |
|---:|---:|---|---:|---:|---:|
| 1 | `0x00441928` | `debris1.jba` | 25x18 | 8 | `(-3,+4)` |
| 2 | `0x004417D0` | `debris2a.jba` | 27x17 | 16 | `(-5,-1)` |
| 3 | `0x00441AC8` | `debris3.jba` | 26x20 | 16 | `(+3,+1)` |

The activation flag is **not Mothership-only**:

- the Mothership destruction path around `0x00412E66..0x00412F30` enables it,
  anchors all three sprites at the Mothership core position, resets render
  dimensions from source dimensions, and installs the velocity triplet above;
- `trigger_drone_detonation_sequence` around `0x0041D558..0x0041D63D` also
  enables the same flag, anchors all three sprites at the Drone target center,
  resets dimensions, and installs the same velocities.

The clean semantic name is therefore **objective-destruction scaled debris**.

### Update and growth

The ordinary state-2 update tail advances each sprite position by its dedicated
velocity and increments/wraps its animation frame. During rendering, only
**gameplay phase 2** performs the symmetric growth step:

```text
render_width  += 2
render_height += 2
x -= 1
y -= 1
```

This grows the destination rectangle by one pixel on every side once per
four-phase cycle. The source sprite and frame selection do not change because
of the growth operation.

### Original visibility prefilter

Before calling the clipped scaled blitter, each debris sprite is skipped when:

```text
x <= -render_width
y <= -render_height
x >= 319
y >= 199
```

Otherwise the destination rectangle is:

```text
left   = x
top    = y
right  = x + render_width
bottom = y + render_height
```

The strict inequalities are preserved in the clean helper because they are
observable at exact edge coordinates.

## Clean implementation

The presentation-only contracts live in:

- `include/drone/fidelity/scaled_overlay_presentation.hpp`
- `src/fidelity/scaled_overlay_presentation.cpp`
- `manifests/scaled_overlay_presentation.csv`
- `tests/test_fidelity.cpp`

No original sprite pixels are checked into the repository. The descriptors keep
asset names, roots, capacities, geometry, and evidence spans only.
