# Lid/Top Boss Family Reconstruction

## Scope and naming policy

The canonical Win32 shareware executable contains a complete boss subsystem built around the assets `lid.jba` and `top.jba`, with `retro1.wav` and `level1.wav` as its dedicated audio pair. The supplied README independently states that boss ships appear immediately before Drones and that destroying a boss awards **100 points**.

The executable proves that this resource/object family is one of those boss ships: its active update path accepts player rapid-missile and special-weapon collisions, spawns enemy bombs, runs a multi-stage destruction sequence, and awards exactly +100 to both score and extra-life progress.

The original source-level class/name is unknown. This project therefore uses **Lid/Top boss family** as a descriptive provisional name derived only from the two primary asset filenames. It must not be converted into a fictional in-universe boss name unless original evidence supplies one.

## Common-entity roots

The subsystem is built from the same `0x154`-byte common entity family already recovered elsewhere.

| address | field relationship | role |
|---|---|---|
| `0x004406F8` | entity root | 36×40 `lid.jba` entity |
| `0x00440838` | lid `+0x140` | current frame |
| `0x00440839` | lid `+0x141` | frame count = 9 |
| `0x0044083A` | lid `+0x142` | activity/encounter substate |
| `0x0044072C` | lid `+0x34` | 16-bit destruction progression counter |
| `0x00446E00` | entity root | 68×56 `top.jba` / composite-root entity |
| `0x00446F42` | top `+0x142` | top/root activity state |

This resolves another temporary reverse-engineering ambiguity: `0x0044083A` and `0x0044072C` are not independent singleton globals in the conceptual model. They are fields *inside the `lid.jba` common entity*. They remain listed in the global ledger because absolute-address tracking is useful during decompilation, but clean code should model them as entity/encounter fields.

The common-entity constructor calls around `0x00407FF0` independently establish the sprite dimensions:

```text
lid entity:  x=1, y=1, width=36, height=40
top entity:  x=1, y=1, width=68, height=56
```

## Asset lifecycle

### Loader — `0x00417350`

`0x00417350` is established as `load_lid_top_boss_assets`.

It is guarded by byte `0x004D95B8`, then performs the following coherent load:

1. decode `lid.jba` through the canonical JBA loader;
2. extract **nine 36×40 frames** into entity `0x004406F8`:
   - frame 0..4 from cells `(0..4, 0)`;
   - frame 5..8 from cells `(0..3, 1)`;
3. decode `top.jba`;
4. extract one **68×56** frame into entity `0x00446E00`;
5. load `retro1.wav` and store its DirectSound handle at `0x00438C18` with volume 70;
6. load `level1.wav` and store its handle at `0x0042EFEC` with volume 90;
7. set the loaded guard to 1.

The metadata-only sprite manifest now includes the nine lid frames and the top frame. No original pixels enter the repository.

### Cleanup — `0x00417450`

`0x00417450` is the paired `release_lid_top_boss_assets` routine. When the loaded guard is set it releases frame allocations for the two entities and releases the two DirectSound buffers, then clears `0x004D95B8`.

This paired lifecycle is the strongest evidence that `lid/top/retro1/level1` form one subsystem rather than unrelated resources.

## Encounter initialization — `0x00417220`

`0x00417220` initializes the boss encounter.

Established behavior includes:

- `top_entity.activity = 1`;
- root X = 0;
- root Y = -100;
- fixed-point position fields mirror that initial location;
- motion parameters differ between live and demo playback paths and depend on difficulty in live play;
- the lid frame is reset;
- `lid.activity = 6` at encounter start;
- several companion-object positions are derived from the root;
- dedicated boss audio is started/reset.

The exact meanings of every lid state (`0`, `1`, `2`, `6` are observed) are not yet all named. We preserve the raw state values wherever semantics are incomplete.

## Active update — `0x00416700`

State-2 gameplay calls `0x00416700` whenever the top/root activity byte at `0x00446F42` is nonzero.

The update path establishes that this is a gameplay boss rather than decorative scenery. It:

- advances fixed-point root position and derives integer X/Y;
- positions lid and multiple companion pieces relative to that root;
- interacts with the `retro1.wav` and `level1.wav` sounds;
- spawns enemy bombs using coordinates derived from the boss;
- tests player rapid missiles against boss pieces;
- tests the Probe/Stinger special projectile;
- runs boss-state transitions;
- emits explosion/debris effects;
- eventually transitions the top/root entity into a destruction state.

### Special-weapon transition

Around `0x00416BE4..0x00416C42`, a special-weapon collision can advance lid activity from state 1 to state 2. The special projectile is then moved into the already established state-10 impact-consumed terminal state, and the lid's `+0x34` progression counter is reset.

This is direct evidence that the lid entity is not merely graphical decoration: its state is part of the boss's damage/destruction protocol.

## +100 destruction milestone

When:

```text
lid.activity == 2
```

`0x00416700` increments the 16-bit counter at `lid + 0x34` (`0x0044072C`). At exactly **25** increments, the original performs:

```text
total_score          += 100
extra_life_progress  += 100
lid.activity           = 0
top.activity           = 2
top.motion_x           = 0
top.motion_y           = 0
```

It then plays destruction SFX including the established `bigexp3.wav` voice pool and emits a large randomized debris/explosion sequence around the composite object.

This +100 value exactly matches the original README's documented **Destroy Boss — 100** rule, providing an independent behavioral cross-check.

The 25 count is a progression/update threshold, not a claim of “25 hits.”

## Where this boss appears

The state-2 gameplay orchestrator checks a Drone objective Y coordinate at `0x00446084` for `-200`. At that boundary it indexes a six-entry boss-initializer dispatch table using `drone_outcome_processed_count` (`0x00433B54`).

Canonical table:

| processed Drone outcomes | initializer |
|---:|---|
| 0 | `0x00417220` — Lid/Top boss |
| 1 | `0x00405EF0` — boss family unresolved |
| 2 | `0x00407980` — boss family unresolved |
| 3 | `0x00415AC0` — boss family unresolved |
| 4 | `0x00417220` — Lid/Top boss again |
| 5 | `0x00404350` — boss family unresolved |

This proves two important facts without requiring invented level names:

1. the game chooses a boss family as a function of progress through the six Drone objectives;
2. the Lid/Top family is reused for processed-outcome counts 0 and 4.

The README's statement that boss ships appear immediately before Drones is consistent with this control-flow placement, but the exact human-facing ordinal/level names of each initializer remain under reconstruction.

## Shareware/demo relationship

The Lid/Top asset loader is used outside the normal boss selection path as well. Attract/demo setup loads the desert scenery family and calls `load_lid_top_boss_assets`, which is consistent with recorded demo sequences exercising this boss family.

This means resource availability alone must not be used to infer normal campaign reachability. Demo/attract presentation and shareware campaign gating are distinct paths.

## Established vs open

### Established

- `lid.jba` and `top.jba` belong to one boss resource family;
- lid is a 36×40 nine-frame common entity;
- top/root is a 68×56 one-frame common entity;
- the family owns `retro1.wav` and `level1.wav`;
- load/release pair is `0x00417350` / `0x00417450`;
- encounter initializer is `0x00417220`;
- active update is `0x00416700`;
- the update handles bombs, player weapons, special-weapon interaction and destruction effects;
- lid state 2 advances a 25-count destruction progression;
- reaching 25 awards +100 and transitions the top/root into activity state 2;
- progress-index dispatch selects this family for processed Drone counts 0 and 4.

### Open

- exact semantic names of lid states 1/2/6 beyond their established transition roles;
- complete movement/attack phase meanings;
- identity and state of every companion subobject positioned around the root;
- the five other boss-initializer table entries as original asset families/names;
- exact campaign/demo reachability for each boss in the shareware build;
- DOS correspondence for this specific boss family.

See `Q-BOSS-001`, [`WORLD_SCENERY.md`](WORLD_SCENERY.md), and [`../ORIGINAL_BEHAVIOR.md`](../ORIGINAL_BEHAVIOR.md).
