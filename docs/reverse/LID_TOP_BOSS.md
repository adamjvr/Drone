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
- root X = 0 and root Y = -100;
- 16.16 fixed-point X/Y mirror that initial location;
- live initial horizontal velocity is `10923 * difficulty`;
- live initial vertical velocity is `0x4001 + 0x1555 * difficulty`;
- live horizontal speed cap is `(difficulty + 3) * 3 * 0x1000`;
- demo playback instead uses X/Y velocity `0x8000` and horizontal cap `0x12000`;
- the lid frame resets to 0;
- `lid.activity = 6` at encounter start;
- several companion-object positions are derived from the root;
- dedicated boss audio is started/reset.

The clean Phase-4 owner now preserves these exact gameplay fields while leaving audio-only companion state outside the portable simulation.

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

### Native movement and bomb emission

While `top.activity == 1`, the updater advances the root's 16.16 position and derives integer X/Y with arithmetic right-shift semantics. Horizontal tracking compares `root.x + 34` (the 68-pixel top sprite's center) against the **player's left X**, not the player center:

```text
if boss_center_x < player.x: velocity_x += 0x44C
else:                        velocity_x -= 0x44C
```

The result is clamped to the initializer's horizontal speed cap. When integer root Y reaches 240, `0x00416885..0x004168C5` does **not** reverse vertical velocity. It increments shared byte `0x00454B04`, optionally plays `level1.wav` when that byte reaches 8, then writes `0xFF9C0000` to fixed-position field `0x00446E0C`. That field is the 16.16 root Y position, so the boss wraps to Y=-100 while retaining its positive descent velocity and begins another downward traversal. The initializer clears the shared cadence byte at `0x00417304`. `level1.wav` is therefore a one-shot (`Play` flags 0) every eighth completed traversal, not an eight-update timer or an upward-retreat sound.

Boss bomb emission is live-only and phase-2-only. The updater consumes `rand()%100` **before** the shared bomb-gate/capacity checks and succeeds when the result is below `2*difficulty`. It then requires the shared gate to equal 5 and an available slot, chooses the first inactive bomb, consumes `rand()%10`, and spawns at either `(root.x+30, root.y+53)` or `(root.x+41, root.y+53)`. The reused bomb's horizontal step and animation frame are intentionally not reset. The shared gate is reset to zero after a successful spawn.

### Rapid-missile lid opening

The rapid-missile loop has two distinct collision tests in slot order:

1. if `missile.x < root.x + 39`, `0x00401FA0` tests the missile against the single current `top.jba` frame's opaque pixels; an opaque hit consumes the missile;
2. if `lid.activity == 6` and `lid.frame == 0`, `0x00401F60` tests the retained missile point against an 11×6 helper at `(root.x+53, root.y+23)` with inclusive 9×5 common hitbox extents; a hit consumes the missile and changes the lid to activity 1.

Because the top-mask pre-gate ends at `root.x+38` and the weakpoint begins at `root.x+53`, these are separate geometric lanes even though the original loop does not re-read missile activity between them. Active missile count is decremented only once at the end of a consumed slot's iteration.

On phase 2, an activity-1 lid advances its frame toward 8. Once the increment would reach frame 9, it clamps back to 8; in live play only, `rand()%200 < difficulty` begins closing by switching to activity 6. The closing branch is a second `if`, so that same update immediately decrements frame 8 to 7. Activity 6 continues decrementing toward frame 0, clamping byte underflow back to zero. A missile that opens the lid on phase 2 therefore advances from frame 0 to frame 1 in that same boss update.

### Special-weapon vulnerability

There are two different launched-special paths. With `top.activity == 1` and a closed lid (`lid.activity == 6`), `0x00401F60` tests the special point against the 68×56 top/root common hitbox. A hit consumes the special into state 10 and zeros its Y motion, but does not open the lid.

The destructive core is narrower. It requires:

- `lid.activity == 1`;
- the special kind/frame to be **Stinger** (`1`);
- `lid.y > 0`;
- `lid.frame > 6`;
- a point hit on the 13×5 helper at `(root.x+29, root.y+32)`, whose common 0.85 extents are 11×4.

A valid hit changes the lid to activity 2, resets its destruction counter, consumes the Stinger into state 10 and then immediately enters the activity-2 progression block, so destruction progress finishes that same update at **1**, not zero. Common special dispatch occurs earlier in state 2 than the boss call, so state 10 persists until the next gameplay update before being settled to inactive.

This is direct evidence that the lid entity is not merely graphical decoration: its opening animation and exposed Stinger-only core are the boss's actual vulnerability protocol.

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

### Top/root destruction retirement

The same updater contains the subsequent top/root activity-2 tail at `0x004169E6..0x00416A12`. Unlike the lid's 25-count progression, this counter advances only when the shared gameplay substep argument is phase 2. The word at `top + 0x32` (`0x00446E32`) increments to exactly **30**, then `top.activity` is cleared to 0. Because this block occurs earlier in the function than the lid's 25-count transition, a top destruction state created by the lid milestone does not consume its first 30-count tick until a later gameplay update.

The Phase-4 clean boss owner preserves that ordering and now owns root geometry/motion, bomb emission, lid opening/closing, and boss-local weapon collision. Randomized explosion/debris rendering and audio remain presentation-side events; immutable `top.jba` frame pixels remain asset input only for the exact rapid-missile opaque-mask test.

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
- root 16.16 movement, player-X tracking and horizontal acceleration/cap are exact; Y>=240 resets fixed Y to -100 without changing vertical velocity, producing repeated downward traversals;
- shared byte `0x00454B04` advances once per completed traversal, resets at 8, and requests one-shot `level1.wav` (volume 90) on every eighth wrap;
- live phase-2 boss bomb chance/gate/slot/position behavior is exact;
- rapid missiles open a frame-0 closed lid through the recovered weakpoint and `top.jba` opaque pixels shield the separate top lane;
- activity 1 opens toward frame 8 and activity 6 closes toward frame 0 with the recovered difficulty-scaled close chance;
- only a launched Stinger can enter the exposed frame>6 core and begin destruction;
- lid state 2 advances a 25-count destruction progression;
- reaching 25 awards +100 and transitions the top/root into activity state 2;
- progress-index dispatch selects this family for processed Drone counts 0 and 4.

### Open

- original source-level names for the provisional lid states beyond their established gameplay roles;
- identity and state of every audio/debris-only companion subobject positioned around the root;
- the five other boss-initializer table entries as original asset families/names;
- exact campaign/demo reachability for each boss in the shareware build;
- DOS correspondence for this specific boss family.

See `Q-BOSS-001`, [`WORLD_SCENERY.md`](WORLD_SCENERY.md), and [`../ORIGINAL_BEHAVIOR.md`](../ORIGINAL_BEHAVIOR.md).
