# Bomber Boss

The Bomber family is selected by boss dispatch slot 5 in the registered progression. Its identity is established by a dedicated `bombmid`/`bombleft`/`bombrite`/`bombtop` resource loader, Bomber audio literals, composite entity update, and exact +100 destruction milestone.

## Functions

| address | research name |
|---:|---|
| `0x00403650` | `update_bomber_boss` |
| `0x00404350` | `initialize_bomber_boss` |
| `0x004044B0` | `load_bomber_boss_assets` |
| `0x00404690` | `release_bomber_boss_assets` |

Resource load guard: `0x004D95C8`.

## Resource namespace

The loader references:

- `bombmid.jba`;
- `bombleft.jba`;
- `bombrite.jba`;
- `bombtop.jba`;
- `bomber1.wav`;
- `squish1.wav`;
- `absorb.wav`;
- `level3.wav`.

These names survive in the canonical shareware executable, but the registered boss files themselves are absent from the supplied shareware installation. Their absence is recorded explicitly in `manifests/boss_resource_roles.csv`.

## Composite object

The central common-entity root is `0x00464BE8`; its standard `+0x142` activity byte is `0x00464D2A`. The state-2 destruction/progression counter is stored at `0x00464C1C` (`root + 0x34`). Surrounding component entities include roots at `0x00433A00`, `0x00480198`, `0x00466998`, and `0x00438DD8`.

The update performs composite positioning, bomb/projectile interactions, rapid-missile/special-weapon collision, audio and effects.

## Destruction milestone

A successful special interaction advances the central entity into state 2 and resets the progression counter. While in state 2 the counter increments. At exactly 60 updates, `0x00403650`:

- adds 100 to total score;
- adds 100 to extra-life progress;
- clears the surrounding component activity states;
- zeros central motion/progression fields;
- launches the composite explosion/debris path.

The value 60 is a progression/update count, not a hit count. Wall-clock duration remains dependent on the still-open simulation-cadence reconstruction.
