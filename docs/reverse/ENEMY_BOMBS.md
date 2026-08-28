# Enemy bomb reconstruction

This document records the established Win32 behavior of the projectile pool rooted at `0x004651A0`. The pool is also the concrete consumer of demo channels 10–12.

## Identity and storage

The pool uses the common `0x154`-byte Win32 entity layout (`WIN-STRUCT-002`). Canonical capacity is the initialized 32-bit value at `0x0042B1A4`:

```text
capacity = 10
```

The active-count scalar is `0x00446F6C`.

Initialization at `0x00407D70` constructs ten 1×9 entities. Asset initialization at `0x0040A4B2` loads `bomb.jba`, extracts cells `(0,0)`, `(1,0)`, `(2,0)`, and propagates the three frame pointers through the rest of the pool. The metadata-only frame hashes are in `manifests/recovered_sprite_frames.csv`.

## Live spawn

The main live spawn path is in the state-2 region around `0x0040DA80..0x0040DDA5`.

A spawn is attempted only after several game-state/random/capacity guards. Once an inactive bomb entry has been selected:

1. `+0x142` becomes active state `1`;
2. X/Y are derived from a selected active trajectory-group entity plus fixed offsets;
3. `+0x10` receives `rand() % 3`, giving horizontal steering magnitude `0`, `1`, or `2`;
4. the spawn is rejected if the resulting initial position is outside the path's accepted bounds;
5. a sound is emitted through the same twenty-voice pool rooted at `0x0047EBE8` (cloned from the `missile.wav` source buffer);
6. the active bomb count increments;
7. if demo recording is enabled, channels 10–12 store spawn flag/X/Y.

The public clean helper accepts already-chosen X/Y and steering magnitude. Random selection remains outside the helper until the complete spawn scheduler is reconstructed.


## Shared spawn gate / post-death quiet period

`0x00438C14` is now established as the shared enemy-bomb spawn gate/cooldown counter. State-2 increments it by one while it is below `5`, then saturates at `5`. The live bomb-spawn branches require exactly `5` and reset it to `0` after a successful spawn. Canonical session initialization writes `-450`, creating an initial bomb-free interval.

Player destruction deliberately reuses this same scalar. `0x0041CDF0` computes:

```text
counter = -20 * signed(death_effect_terminal_frame)
```

The canonical terminal frame byte is `27`, so player death writes **`-540`**. State-2 continues incrementing the counter toward 5, while the later respawn settlement gate requires:

```text
counter > -356
```

Starting from `-540`, that first becomes true after **185 state-2 increments**, at `-355`. No wall-clock duration is assigned until simulation cadence is proven.

This is an original subsystem coupling: one scalar both suppresses bomb spawning and contributes to the post-death respawn delay. The clean `EnemyBombSpawnGate` preserves the saturating increment, spawn reset, death suppression, and respawn threshold without coupling those rules to rendering/audio.

## Demo playback quirk

Playback around `0x0040D949..0x0040DA7B` reconstructs a bomb from channels 10–12. It restores X and Y, but then explicitly performs:

```text
bomb.+0x10 = 0
```

Therefore **replayed bombs fall vertically even though live bombs may steer horizontally**. This is original behavior, not a clean-engine normalization. `spawn_replay_enemy_bomb()` preserves it explicitly.

## Active update

The active-pool update is in the state-2 region around `0x0040E36C..0x0040E49B`.

For each entity whose `+0x142 == 1`:

- on the shared animation-tick condition, `+0x140` increments and wraps `3 -> 0`;
- a horizontal target is selected;
- if bomb X is below the target, `+0x10` is added;
- if bomb X is above the target, `+0x10` is subtracted;
- Y increases by exactly `2` every update;
- `+0x143` is recomputed from logical visibility bounds.

Normal target X is:

```text
player.x + 17
```

When an additional still-unnamed gameplay condition is greater than one **and** the Probe/Stinger entity is in attached/decode state `2`, the bomb instead targets:

```text
attached_probe.x + 1
```

The clean API exposes this redirect as an explicit caller-supplied condition. We intentionally do not invent a name for the additional original gate yet.

### Visibility versus lifetime

The update sets `+0x143 = 1` outside:

```text
X: 0..319
Y: 0..190
```

This does **not** immediately deactivate the projectile. The later collision/cleanup pass only clears activity once:

```text
Y > 198
```

That two-stage boundary behavior resembles the separately recovered rapid-missile path, where an edge flag is set before final retirement.

## Collision pass

The later state-2 pass around `0x0040F330..0x0040F5A3` tests bombs against the Probe/Stinger and player. The player branch at `0x0040F4BB..0x0040F589` is now established precisely:

1. player collision is evaluated only while the player entity is active;
2. `0x00402000` performs the recovered Y+9 hitbox test;
3. the bomb is deactivated immediately on collision;
4. the code reads `player_shield_active` (`0x0046198C`);
5. **without shield**, a merely loaded Probe/Stinger (state 1) is first advanced to launched state 3 and its launch sound is started, then the 20-voice `bigexp3.wav` pool at `0x004603A8` is played and `0x0041CDF0` executes player destruction;
6. **with shield**, player destruction is skipped, bomb motion fields `+0x10/+0x14` are zeroed, and `spawn_mini_explosion_sprite` (`0x00401E60`) consumes the bomb as a stationary effect source;
7. if the bomb is now inactive, `0x00446F6C` is decremented.

This proves that `player_shield_active` is gameplay protection, not merely a render flag. It also preserves a small but important original rule: an unshielded lethal bomb hit auto-launches a special weapon that was only loaded/tracking before the player's death sequence begins.

`resolve_enemy_bomb_player_impact()` reconstructs the consequence decision as a platform-neutral event result. Collision detection, DirectSound playback, player-death effects, and mini-explosion rendering remain owned by their respective higher-level systems.

## Clean implementation

Implemented in:

- `include/drone/gameplay/enemy_bomb.hpp`
- `src/gameplay/enemy_bomb.cpp`
- synthetic regression coverage in `tests/test_formats.cpp`

The clean state intentionally contains only fields whose semantics are established:

```text
x, y
horizontal_step
frame
active
out_of_bounds
```

No proprietary pixels or replay records are required by the unit tests.

## Remaining questions

- What is the best semantic name for the additional condition that redirects bombs toward an attached Probe?
- Which original enemy families feed each live bomb-spawn path and what are their exact scheduler probabilities?
- What are all Probe/Stinger state transitions caused by bomb collision?
- Which original enemy families feed every remaining bomb/special collision consequence and how do those outcomes interact with special states 4/10?
