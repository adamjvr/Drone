# Common Sprite/Entity Layout Correspondence

## Status

`Q-ENTITY-002` is **resolved for Phase 2 simulation architecture**.

The canonical Windows shareware build uses a common **0x154-byte** sprite/entity family, and the DOS build uses a corresponding **0x14F-byte** family. Producer/consumer tracing across initialization, transparent rendering, frame ownership, collision, destruction and scoring is now strong enough to establish a field-level cross-build correspondence while keeping family-specific scratch fields contextual.

The clean engine deliberately does **not** reproduce either packed binary structure. It keeps semantic state in subsystem-specific C++ types and uses this document as a fidelity map back to the originals.

## Lifecycle anchors

| operation | DOS | Windows |
|---|---:|---:|
| initialize common entity | `0x00068220` | `0x00401780` |
| release frame allocations | `0x000682D0` | `0x00401820` |
| clipped transparent blit | `0x00068300` | `0x00401660` |
| common collision helper | `0x000684D0` | collision family including `0x00401F60`, `0x00402000`, `0x00402FC0` |
| representative damage/destruction path | `0x0007F800` | state-2/trajectory destruction paths around `0x0040EE36` and `0x004164A8` |

The init/free/blit trio independently proves that these are not merely similarly sized records: they own the same position/dimension/frame lifecycle with a systematic layout shift.

## Established semantic correspondence

| semantic field | DOS `0x14F` | Win32 `0x154` | status |
|---|---:|---:|---|
| position X | `+0x00` | `+0x00` | established |
| position Y | `+0x04` | `+0x04` | established |
| velocity X | `+0x10` | `+0x10` | established |
| velocity Y | `+0x14` | `+0x14` | established |
| trajectory X offset / family overlay | `+0x18` | `+0x18` | contextual |
| trajectory Y offset / family overlay | `+0x1A` | `+0x1A` | contextual |
| sprite width | `+0x20` | `+0x20` | established |
| sprite height | `+0x22` | `+0x22` | established |
| collision width extent | `+0x28` | `+0x28` | established |
| collision height extent | `+0x2A` | `+0x2A` | established |
| damage accumulator | `+0x30` | `+0x30` | established |
| destruction threshold | `+0x31` | `+0x31` | established |
| trajectory index / family parameter | `+0x32` | `+0x32` | contextual |
| trajectory end / family parameter | `+0x38` | `+0x38` | contextual |
| 32 frame-pixel pointers | `+0x3E..+0xBD` | `+0x40..+0xBF` | established |
| preserved unreferenced block | `+0xBE..+0x13D` | `+0xC0..+0x13F` | established as unreferenced |
| current frame | `+0x13E` | `+0x140` | established |
| loaded frame count | `+0x13F` | `+0x141` | established |
| activity/state | `+0x140` | `+0x142` | established/contextual enum |
| off-screen/out-of-bounds flag | `+0x141` | `+0x143` | established |
| legacy initializer word = 1 | `+0x14A` | `+0x14C` | physical meaning established; gameplay semantic absent |
| family-contextual flag | `+0x14C` | `+0x14E` | contextual |
| destruction burst count | `+0x14D` | `+0x14F` | established for common destructibles |
| score-value byte (Win32 consumers sign-extend) | `+0x14E` | `+0x150` | established for common destructibles; canonical trajectory values are 1/25 |

### Why the strides differ

The observed field map accounts for the **five-byte** difference without pretending to know the historical compiler/source reason:

- before the frame table, Win32 has two additional bytes (`+0x3E..+0x3F`) relative to DOS;
- after that insertion, corresponding frame/status/tail fields stay two bytes later in Win32;
- Win32 then has three additional unreferenced tail bytes at `+0x151..+0x153`.

That is sufficient to explain `0x14F + 2 + 3 = 0x154` structurally. It does **not** prove why the original source/platform ABI changed.

## Damage and destruction metadata

The earlier low-confidence interpretation of `+0x30` as trajectory state is superseded.

Both builds implement the same protocol:

```text
entity.damage_accumulator += incoming_damage
if entity.damage_accumulator >= entity.destruction_threshold:
    entity.activity = inactive
    entity.damage_accumulator = 0
    emit entity.destruction_burst_count destruction effects
    apply entity.score_value to score/progress according to destruction/retirement path
```

Representative DOS evidence is especially compact at `0x0007F82B..0x0007F98B`:

- `+0x30` receives `+3` damage;
- `+0x31` is the comparison threshold;
- `+0x140` activity is cleared on destruction;
- `+0x14D` controls one-vs-many destruction effects;
- `+0x14E` is zero-extended and added to both score and extra-life progress in this DOS path; Win32 common-trajectory consumers sign-extend their corresponding `+0x150` byte. Canonical fixed trajectory values are only 1 or 25, so the observed gameplay semantics coincide for this family.

The Win32 state-2 and trajectory destruction paths perform the same operations at `+0x30`, `+0x31`, `+0x14F`, and `+0x150`.

For the 17 fixed Win32 trajectory templates, startup writes now recover exact combat profiles in `manifests/trajectory_group_templates.csv` and `TrajectoryGroupTemplate::combat`.

## Family-specific overlays

A common physical record does **not** imply one universal semantic C++ object. Several offsets are intentionally reused by different families.

### Trajectory breakaway mode

In trajectory group mode 10:

- `+0x08` / `+0x0C` are 16.16 X/Y accumulators;
- `+0x10` / `+0x14` become 16.16 breakaway velocity components;
- `+0x1C` / `+0x1E` hold off-screen exit targets;
- `+0x32`, `+0x36`, and `+0x38` retain trajectory-family control semantics.

Those meanings must not be projected onto players, missiles, bosses, or effect sprites.

### `+0x34`

Boss roots use the word at `+0x34` as destruction/progression counters. Other effect-family routines access state at the same physical location with different widths/semantics. The clean engine therefore keeps it as owning-subsystem state rather than a universal entity member.

### Win32 `+0x14E` / DOS `+0x14C`

This byte is another proven contextual overlay:

- explosion/effect entities use it as a growth/expansion flag affecting render dimensions/position;
- the Probe/Stinger special entity uses it as target-lock/attachment state.

The same contextual reuse exists at the corresponding DOS offset.

## The preserved 128-byte middle block

The 32 frame pointers end at Win32 `+0xBF` / DOS `+0xBD`. In both builds there is then an exact **128-byte gap** before the current-frame byte:

```text
Windows: +0xC0 .. +0x13F
DOS:     +0xBE .. +0x13D
```

A complete direct-offset scan of the canonical shareware disassemblies found no consumer of either region. The common initializers do not establish semantic contents for this block.

Therefore the fidelity rule is deliberately conservative:

> Preserve the fact that the historical packed record contains the block, but do not invent a name, second frame bank, animation cache, or clean-engine member until a real producer/consumer appears.

## DOS width/height code-generation note

The DOS blitter sometimes loads overlapping DWORDs such as `entity+0x1E` or `entity+0x20` and arithmetic-shifts by 16 to obtain the signed words at `+0x20` / `+0x22`. This is compiler/code-generation behavior, not evidence for different dimension offsets. The semantic width/height words align with Win32.

## Clean-engine policy

The portable architecture should consume semantic contracts such as:

- `PlayerState`;
- projectile state;
- trajectory state and `TrajectoryCombatProfile`;
- boss-family state;
- effect/debris state;
- rendering sprite/frame references.

It should **not** expose a public packed `SpriteEntity154` or `SpriteEntity14F` structure. Reproducing the raw padding, pointer layout and contextual unions would couple Linux/macOS/iPadOS/Windows code to historical implementation accidents without improving behavioral fidelity.

`CORR-SPRITE-001` is therefore an evidence correspondence, not a portable ABI.
