# Player Lives, Death Settlement, Respawn, and Game Over — Win32 Shareware

This document records the recovered ownership of player lives and the transition from player destruction to respawn or game over in the canonical Win32 shareware executable. The key fidelity result is that **collision/destruction does not consume a life immediately**. Life consumption is deferred until a later state-2 settlement gate.

## Lives global

`0x0042B1AC` is the canonical player-life counter.

Independent evidence:

- normal gameplay/session initialization at `0x00417F50` writes `3`;
- the hidden `L`-key path writes `9` immediately before entering the message state that displays `<L> KEY GRANTED NINE LIVES!`;
- the HUD path at `0x00410FD3` reads the value and clamps values above 9 to 9 for display;
- state-2 entry at `0x0040BAB9` tests it and branches to the post-game/results flow when it is non-positive;
- the respawn settlement path at `0x0040E2A3` decrements it.

The clean compatibility default is therefore three lives. The cheat behavior is archaeological evidence and is not required to be exposed by a modern host UI.

## Destruction is not life consumption

The dedicated player destruction/effects routine around `0x0041CDF0` disables the player and emits the explosion/debris sequence, but it does **not** decrement `0x0042B1AC`.

The life decrement occurs later at `0x0040E272..0x0040E2DB`, after several independent conditions allow the death presentation to settle.

This distinction matters for deterministic reconstruction. A clean engine that decrements at collision time moves the life transition earlier than the original and can change HUD, replay, effect, and game-over timing.

## Settlement gate

The original branch requires all of the following before consuming a life. The first condition is an intentionally reused enemy-bomb cooldown rather than a dedicated respawn timer:

1. enemy-bomb spawn gate/cooldown `0x00438C14 > -356` (`0xFFFFFE9C`); this same scalar increments toward 5, gates live bomb spawning at 5, and is driven to -540 by player destruction;
2. death-effect activity byte `0x00491E22 == 0`;
3. `player_lives > 0`;
4. player activity byte `0x00466C5A == 0`;
5. Drone activity/state byte `0x004461C2 != 2`.

Only then does the original consume a life. From the canonical death write of -540, the bomb gate reaches the first respawn-eligible value (-355) after 185 state-2 increments. This is recorded as update-count behavior only; no seconds/FPS conversion is made yet.

## Exact settlement order

At `0x0040E2A3` the original performs the following order:

```text
player_lives--
shield_energy = 75 << 16
player.frame = 0
player.x = 147
player.y = 175

if player_lives > 0:
    player.active = 1
else:
    animate_game_over_banner_slide()
```

The reset occurs even when the decrement reaches zero. In particular, the original resets shield energy and respawn position/frame **before** deciding to run the game-over banner.

The independently written reconstruction lives in:

- `include/drone/gameplay/player_lifecycle.hpp`
- `src/gameplay/player_lifecycle.cpp`

Regression coverage proves both the 3→2 respawn case and the 1→0 game-over case, including the original reset ordering. `GameSession` now owns this settlement continuously: it advances the shared enemy-bomb gate and combines its `> -356` result with the native player-death explosion activity, player/Drone state, and remaining lives. No presentation-side death-effect boolean is required.

## Native player-death explosion lifecycle

The singleton common entity at `0x00491CE0` is initialized as a **42×38** sprite entity and later receives **27** `explode1`-derived frame pointers. The loader writes terminal frame byte `0x00491E21 = 27`. It is separate from the ordinary pooled explosion entities.

`trigger_player_destruction_sequence` (`0x0041CDF0`) performs the state-bearing setup after lethal collision:

```text
player.active = 0
bomb_spawn_gate = -20 * terminal_frame   // -540 canonically
death_effect centered over 22x22 player
death_effect.motion = player common-entity motion
death_effect.activity = 3
death_effect.frame = -6
```

The centering helper `0x00402770` gives the canonical player position `(147,175)` an explosion origin `(137,167)`. The recovered player-control path writes only horizontal common-entity motion, so canonical vertical inherited motion is zero. The destruction routine also emits randomized debris/explosion/audio work; that presentation-side RNG/effect stream remains a separate fidelity task and is not required for the singleton lifecycle or respawn gate.

State-2 code `0x0040E1DA..0x0040E271` advances the singleton **only when the shared gameplay substep equals 2**:

1. if activity is nonzero, add inherited X/Y motion;
2. clear activity when `x > 319`, `x < -42`, `y > 199`, or `y < -38`;
3. increment the signed frame byte;
4. when the increment reaches `0`, set activity to `1` (visible);
5. when frame reaches terminal value `27`, set activity to `0`.

The ordering is exact and slightly odd: the bounds clear happens before frame increment, so an out-of-bounds pre-roll frame `-1` can be cleared and then written back to visible when it increments to `0` in the same update. Rendering at `0x00410BA8` draws the singleton only while activity is exactly `1`. Canonically, frame `-6` therefore spends six phase-2 ticks in activity `3`, frame `0` becomes visible on the sixth tick, frames `0..26` are drawable, and frame `27` retires the actor. The later respawn gate reads the resulting activity byte in the same update.

The independent clean reconstruction lives in:

- `include/drone/gameplay/player_death_effect.hpp`
- `src/gameplay/player_death_effect.cpp`
- `tests/test_player_death_effect.cpp`

## `gameover.jba` entity

The startup asset path initializes the common entity rooted at `0x004671E8` as:

- width: **117**;
- height: **20**;
- one frame from `Sights/Gameover.jba`, cell `(0,0)`;
- runtime start X: `325`;
- runtime Y: `90`.

The dimensions come directly from the argument list to common entity initializer `0x00401780`; the frame is populated by `0x00401860`.

The metadata-only frame hash is recorded in `manifests/recovered_sprite_frames.csv`. Original pixels remain outside Git.

## Game-over slide routine

`0x0041E420` is established as `animate_game_over_banner_slide`.

It first allocates a 64,000-byte framebuffer snapshot and copies the current display surface into it. It then loops while the fixed-point horizontal velocity is positive:

```text
fixed_x -= velocity
velocity -= 2500
x = fixed_x >> 16
restore 320x200 framebuffer snapshot
blit gameover.jba entity
wait for vertical blank
present framebuffer
```

Initialization at `0x00417F50` supplies:

```text
x             = 325
y             = 90
fixed_x       = 325 << 16
velocity      = 270000
velocity_step = 2500
```

This produces exactly **108 presented iterations**, ending at:

```text
x = 100
velocity = 0
```

A 117-pixel-wide banner at X=100 occupies 100..216, placing it approximately centrally in the original 320-pixel logical width.

The gameplay core models only the fixed-point state transition in `gameplay/game_over.*`; framebuffer restore, sprite blit, vertical-blank wait, and platform presentation remain in the fidelity/presentation layer.

## Post-game flow boundary

After the banner, the next state-2 dispatch sees `player_lives <= 0` and enters the large region beginning at `0x004115BE`. That region is **not** being named `game_over` wholesale: it also contains post-game/disarm/results presentation, soundtrack selection, and later transitions. Known assets referenced there include `disarm0.jba` through `disarm6.jba`/`disarm6m.jba`, plus `choral.wav`, `suspense.wav`, `moon.wav`, and `hiphop.wav`.

Until that region is decomposed further, the documentation calls it the **post-game/results flow** rather than assigning narrower semantics to code that clearly does more than the game-over banner.

## Open questions

- Full randomized debris/audio/pixel presentation emitted by `0x0041CDF0` beyond the now-native singleton explosion lifecycle.
- Full semantics of the post-game/results region at `0x004115BE`.
- Whether the DOS release uses an equivalent deferred decrement and banner motion algorithm; this remains a cross-build target.
