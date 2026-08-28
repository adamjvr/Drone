# Gameplay Effects Reconstruction

Explosion/debris logic is one of the first clusters split out of the Win32 gameplay orchestrator. Names below describe evidenced behavior and are not original source identifiers.

## Entity velocity fields

Multiple independent effect routines establish that common entity fields `+0x10` and `+0x14` are velocity X and velocity Y:

- `0x00402430` assigns randomized values directly to `+0x10/+0x14`;
- `0x00401E60` copies those fields from a source entity into a newly activated mini-explosion sprite;
- `0x00402770` centers one sprite on another and copies the same two fields.

The structure ledger therefore promotes these fields from unknown to established velocity semantics.

## Mini-explosion pool — `0x00401E60`

The routine scans an inline `0x154`-stride pool beginning at `0x00480318`, selects an inactive object, marks it active, chooses a frame from a caller byte, centers it within the source object's dimensions, and inherits source velocity.

Initialization associates this family with `miniexp1.jba` / `miniexp2.jba`, making a mini-explosion effect interpretation high confidence.

## Composite explosion — `0x00402050`

This routine computes the source entity center and emits a randomized mixture of explosion sprites, mini-explosions and debris/particle objects. A 16-bit cadence counter at `0x0042EFD4` increments each call. At value `0x13` (19), it resets and delegates to the larger effect at `0x004024B0`.

The same cluster uses the 20-voice sound-pool helper with the pool initialized from `expscale.wav`.

## Large explosion — `0x004024B0`

The larger effect performs a broader randomized emission around the source center, including normal explosion sprites, one or more mini-explosion sprites, and debris/particles. It also triggers `expscale.wav` through the recovered reusable voice-pool mechanism.

## Explosion sound variant — `0x00402900`

A byte at `0x0042EFD8` cycles through cases 1..4. The cases select three 20-voice DirectSound pools:

| cases | voice-pool base | initialization asset |
|---|---:|---|
| 1, 2 | `0x004603F8` | `explode2.wav` |
| 3 | `0x00446238` | `explode3.wav` |
| 4 | `0x0042EFF0` | `explode4.wav` |

This is now named `play_explosion_sfx_variant` with high confidence.

## Debris/particle groups — `0x004027B0`, `0x00402B40`, `0x00402CA0`

`0x004027B0` is the producer for a pool rooted at `0x00472B00`. Groups use stride `0x61C`; each active group has a byte active flag, a 16-bit active-record count, and `0x18`-byte particle records. The updater `0x00402B40` establishes the record semantics used by the clean model:

```text
+0x00 int32 x
+0x04 int32 y
+0x08 int32 velocity_x
+0x0C int32 velocity_y
+0x10 int16 age
+0x12 int16 age_limit
+0x14 uint8 visual_code
+0x15 uint8 active
```

For every active record it adds velocity to position, retires outside X `0..319` / Y `0..199`, applies downward acceleration when `(rand() & 0x0f) < 6`, and advances age when an independent `(rand() & 0x0f) < 11`. Once signed 16-bit age exceeds the threshold, visual codes `0x34..0x39` use the original small transition table; other codes decrement and retire below 3. A retiring record decrements the group active count; a zero group count clears the group active flag and decrements the global active-group count.

`0x00402CA0` is the presentation consumer. It walks these records plus the secondary bank below and writes active particle visual codes directly into the indexed software framebuffer through the established `y*320` row-offset table.

The clean implementation passes the two random rolls into `advance_debris_particle()` explicitly. This preserves exact update semantics without making `drone_core` depend on the original Microsoft CRT PRNG.

## Secondary debris bank — `0x00402EF0`

A second bank is guarded by active flag `0x00440FF8` and active count `0x00440FFA`. It uses the same `0x18` record shape but a simpler update:

- velocity motion and the same 320×200 retirement bounds;
- downward acceleration when `rand()%10 < 3`;
- age increments every update;
- after `age > age_limit`, `visual_code--`;
- the record retires when the decremented code is below 3;
- zero active count clears the bank flag.

This is mirrored by `advance_secondary_debris_particle()`.

## Sprite debris — `junk1`, `junk2`, `wheel`

The remaining small composite-explosion debris is not the `0x18` record family. It consists of three parallel **15-entry** common-entity pools:

| pool | asset | geometry |
|---:|---|---:|
| `0x0042FCA0` | `junk1.jba` | 7×7 |
| `0x00431090` | `junk2.jba` | 12×9 |
| `0x0045A9B8` | `wheel.jba` | 5×5 |

`0x004032C0` walks all three pools and dispatches active entities to `0x00403330`. That shared updater adds common-entity velocity `+0x10/+0x14`, retires sprites that no longer fit fully inside the 320×200 logical screen, applies rare downward acceleration when `(rand() & 0x7f) < 10`, and advances the current frame by the **signed byte at `+0x32`** with the recovered wrap behavior. This is another concrete demonstration that `+0x32` is object-family scratch: it is a trajectory index for trajectory entities but a frame step for these debris entities.

The clean `DebrisSpriteState` / `advance_debris_sprite()` contract reproduces this update deterministically.

## Status

`Q-FX-001` is resolved: the small debris visual identities and their lifetime/update consumers are established. Remaining effect work is presentation fidelity (exact palette appearance, layering, and special late effects), not basic pool lifecycle semantics.
