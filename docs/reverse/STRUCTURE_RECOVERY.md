# Structure Recovery

## Policy

The project records original memory layouts as evidence but does not require the clean engine to copy them. Original structures are reconstructed from access patterns and only receive semantic field names when consumers establish them.

Machine-readable records:

- `reverse/structures/structure_ledger.csv`
- `reverse/structures/fields.csv`

## Win32 `0x154` sprite/entity object

**Status:** established common/core layout with explicit family-contextual overlays. See [`ENTITY_LAYOUT.md`](ENTITY_LAYOUT.md) for the complete DOS/Win32 comparison.

A common Win32 object is initialized by `0x00401780`, consumed by transparent blitter `0x00401660`, has frame allocations released by `0x00401820`, and is iterated throughout initialization/gameplay with a stride of `0x154` bytes.

Established or strong fields:

| offset | width | provisional meaning | confidence | evidence |
|---:|---:|---|---|---|
| `0x00` | 4 | `position_x` | high | blitter and trajectory consumer |
| `0x04` | 4 | `position_y` | high | blitter and trajectory consumer |
| `0x10` | 4 | velocity X | high | randomized by effect spawners; inherited/copied by effect helpers |
| `0x14` | 4 | velocity Y | high | randomized by effect spawners; inherited/copied by effect helpers |
| `0x18` | 2 | trajectory X offset in path-owned objects | medium | group spawn code |
| `0x1A` | 2 | trajectory Y offset in path-owned objects | medium | group spawn code |
| `0x20` | 2 | sprite width | high | clipped blitter |
| `0x22` | 2 | sprite height | high | clipped blitter |
| `0x24` | 2 | scaled/render destination width | high | `0x00403460` wrapper into scaled blitter |
| `0x26` | 2 | scaled/render destination height | high | `0x00403460` wrapper into scaled blitter |
| `0x28` | 2 | collision-width extent | high | `int(width * 0.85)` in init; consumed by hitbox tests |
| `0x2A` | 2 | collision-height extent | high | `int(height * 0.85)` in init; consumed by hitbox tests |
| `0x30` | 1 | damage accumulator | high | collision/destruction paths; exact DOS correspondence |
| `0x31` | 1 | destruction threshold | high | compared against accumulated `+0x30`; exact DOS correspondence |
| `0x32` | 2 | trajectory index in path-owned objects | high contextual | `0x00415FA0` increments then indexes X/Y/AUX arrays |
| `0x36` | 2 | trajectory step/increment in path-owned objects | high contextual | `0x00415FA0` adds it to `+0x32` every trajectory update |
| `0x38` | 2 | trajectory end/wrap index in path-owned objects | high contextual | `0x00415FA0` signed-compares advanced `+0x32` and wraps to zero when greater |
| `0x40..0xBF` | 32 × 4 | frame pixel pointers | high | blitter + release loop |
| `0x140` | 1 | current frame index | high | indexes frame pointer table in blitter |
| `0x141` | 1 | loaded frame count | high | frame-release iteration bound |
| `0x142` | 1 | activity/state byte | high contextual / partial global | trajectory family establishes 0 inactive, 1 following path, 3 acquiring path; other object families reuse field |
| `0x143` | 1 | out-of-bounds/offscreen flag | high | explosion/effect spawn helpers set from screen-bound checks |
| `0x14C` | 2 | legacy initializer word = 1; no canonical live consumer | high physical | initializer plus complete direct-use corpus |
| `0x14E` | 1 | family-contextual flag | high contextual | effect growth flag; Probe/Stinger attachment state |
| `0x14F` | 1 | destruction burst count | high | common destruction effect loop; DOS `+0x14D` |
| `0x150` | 1 signed | score value | high | score/extra-life progress add/subtract; DOS `+0x14E` |

The debris-sprite updater additionally proves that `+0x32` must remain family-contextual: `junk1`/`junk2`/`wheel` entities consume its low byte as a signed animation-frame step, while trajectory-owned entities consume the word as a path index.

The `0x142` field must **not** be represented as `bool`: the original writes at least `0`, `1`, and `3`.

The frame pointer table is followed by a preserved **128-byte reserved/unreferenced block** (`+0xC0..+0x13F` in Win32, `+0xBE..+0x13D` in DOS). No canonical direct consumer was found; clean code deliberately assigns it no semantics.

The floating constant used to derive collision extents at `0x0042A000` is exactly `0.85` as an IEEE-754 double. Effect routines `0x00402430`, `0x00401E60`, and `0x00402770` independently establish `+0x10/+0x14` as velocity X/Y. Collision routines `0x00401F60` and `0x00402000` independently establish `+0x28/+0x2A` as collision extents. See [`COLLISION.md`](COLLISION.md).

### Transparent sprite contract

`0x00401660`:

- reads X/Y and width/height from the object;
- indexes `frame_pixels[current_frame]`;
- clips X against logical `0..319` and Y against the current framebuffer-height global;
- treats source pixel value `0` as transparent;
- writes non-zero palette indices directly into the 8-bit framebuffer.

This gives the clean fidelity renderer a concrete original contract rather than an inferred sprite API.

## Win32 trajectory-group descriptor

**Status:** core lifecycle established / template-specific layout partial.

The canonical pool begins at `0x00495CF0`, contains 17 records, and advances by a fixed `0x2148`-byte stride. Each record begins with a recovered header followed by inline `0x154` objects:

| offset | width | established meaning | confidence |
|---:|---:|---|---|
| `0x00` | 1 | group mode: 0 inactive, 1 persistent loop, 2 retire-on-wrap, 10 breakaway | high |
| `0x01` | 1 signed | entity count | high |
| `0x02` | 1 | active entity count | high |
| `0x04` | 4 | pointer to X/Y/AUX pointer slots | high |
| `0x08` | 2 | X group offset | high |
| `0x0A` | 2 | Y group offset | high |
| `0x0C` | 2 | stagger/spawn-delay counter | high |
| `0x0E` | 2 | stagger/spawn-delay interval | high |
| `0x10` | 2 | activated fixed-slot count | high |
| `0x14` | — | first inline `0x154` sprite/entity | high |

The descriptor's `+0x04` pointer addresses three adjacent pointer slots for X, Y, and AUX arrays. All three are consumed by the normal trajectory update. For a path-owned entity:

```text
entity.x = x[path_index] + entity.x_offset + group.x_offset
entity.y = y[path_index] + entity.y_offset + group.y_offset
entity.frame = apply_aux(entity.frame, aux[path_index])
```

`+0x02` is incremented when a staggered member activates and decremented when an active member retires. Reaching zero sets the group to mode 0 and decrements the global active-group count. `+0x10` is different: it counts how many fixed slots have been activated at least once. See [`TRAJECTORY_GROUPS.md`](TRAJECTORY_GROUPS.md) for mode-2 retirement and mode-10 breakaway behavior.

## DOS `0x14F` object family correspondence

**Status: established field-level family correspondence.** DOS `0x00068220` initializes the `0x14F` record, `0x000682D0` releases its frame allocations, and `0x00068300` performs the clipped zero-transparent sprite blit. Later collision/destruction code independently establishes activity, damage, destruction-burst and score metadata.

The frame table begins at DOS `+0x3E` versus Win32 `+0x40`; from there the current-frame/count/activity and tail fields are consistently two bytes earlier in DOS. Both layouts preserve an unreferenced 128-byte block immediately after the frame table, while Win32 adds three final unreferenced bytes at `+0x151..+0x153`. Keep the packed layouts separate and use only semantic correspondence in clean code. See [`ENTITY_LAYOUT.md`](ENTITY_LAYOUT.md).

## Resolved `0x14` record — FONT2 glyph descriptor/cache

The Phase 1 `0x14`-byte cleanup-stride question is now resolved and was **not** another gameplay object family. Win32 `0x00401470` lazily initializes 64 records at `0x00466C90`; DOS `0x000809B0` independently initializes the same layout at data offset `0x6F80`. The exact descriptor is:

| offset | width | meaning |
|---:|---:|---|
| `0x00` | 4 | mutable X |
| `0x04` | 4 | mutable Y |
| `0x08` | 4 | width = 7 |
| `0x0C` | 4 | height = 5 |
| `0x10` | 4 | glyph-mask pixel pointer |

The 64 records map ASCII `0x20..0x5F` to a 16×4 grid in `font2.jba`; crop origins are `x=1+8*column`, `y=1+6*row`. This is presentation/UI cache state, not simulation architecture. See [`BITMAP_FONT.md`](BITMAP_FONT.md).

## Arrays and pools

For every pool, record:

- base global/address;
- element stride;
- maximum/active count;
- initialization pattern;
- lifetime;
- free/in-use state encoding;
- update iteration order.

Iteration order can affect collision, spawn, and projectile behavior, so the clean engine must not reorder original pools casually before parity is established.

## Win32 JBA sheet container

A separate asset container has a decoded-pixel pointer at `+0x4494`. `0x00401900` allocates `0xFA01` bytes into this field, `0x004012B0` fills the first 64,000 bytes with the decoded 320×200 JBA image, `0x00401860` reads it for sprite-frame extraction, and `0x00401450` frees it. The earlier container fields remain unknown and are not copied into the clean engine. See [`SPRITE_SHEETS.md`](SPRITE_SHEETS.md).
