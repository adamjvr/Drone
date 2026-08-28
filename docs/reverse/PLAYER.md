# Player Reconstruction — Win32 Shareware

This document records player behavior established directly from the canonical Win32 shareware executable. It intentionally distinguishes executable behavior from statements in the supplied README.

## Entity identity

The player uses the common `0x154`-byte sprite/entity family rooted at **`0x00466B18`**.

Confirmed fields at that root include:

| address | entity offset | meaning |
|---|---:|---|
| `0x00466B18` | `+0x00` | X position |
| `0x00466B1C` | `+0x04` | Y position |
| `0x00466B28` | `+0x10` | horizontal-motion component used by input/effects |
| `0x00466C58` | `+0x140` | current ship frame |
| `0x00466C5A` | `+0x142` | activity/state byte; normal player-active value is `1` |

The object is initialized as a **22×22** sprite. The initial constructor path supplies `(146,175)`; the recovered respawn path later sets `(147,175)` and frame zero. That one-pixel discrepancy is preserved as evidence rather than normalized.

## `ship.jba` frame bank

`ship.jba` is decoded with the common JBA loader and the 22×22 ship frames are extracted with the one-pixel-gutter grid routine `0x00401860`.

The Win32 executable loads **15 frames**:

- frames 0–3: grid row 0;
- frames 4–7: row 1;
- frames 8–11: row 2;
- frames 12–14: row 3.

Frame hashes derived by the clean decoder/extractor are recorded in `manifests/recovered_sprite_frames.csv`; the repository stores hashes and coordinates only, not original pixel payloads.

## Directional input

Physical keyboard/joystick/replay aggregation is documented separately in [`INPUT.md`](INPUT.md); this section describes the resulting player-motion behavior.

The executable's keyboard path includes:

| input | executable behavior |
|---|---|
| Left Arrow | `x -= 2`; horizontal-motion field becomes `-1` |
| Right Arrow | `x += 2`; horizontal-motion field becomes `+1` |
| `A` | `y -= 1` |
| `Z` | `y += 1` |

This is a concrete discrepancy with the supplied README, which describes player movement as left/right only. The project treats the executable as authoritative for compatibility behavior while preserving the documentation discrepancy for historical accuracy.

The normal gameplay clamps are exactly:

```text
X = 2 .. 297
Y = 120 .. 175
```

Opposing inputs are processed sequentially rather than collapsed into a canonical axis before movement. Therefore Left+Right cancels the X/frame changes but leaves the later Right branch's `+1` value in the horizontal-motion field. The clean reconstruction preserves this order.

## Banking animation

The 15 ship frames form a banking ring around neutral frame zero.

On the separate animation-tick condition:

```text
left  -> frame + 1
right -> frame - 1
```

with wrap over `0..14`.

When neither horizontal direction is active, the same tick recenters the bank:

```text
frames 1..8  -> decrement toward 0
frames 9..14 -> increment; 15 wraps to 0
```

The timing condition that causes an animation tick is kept as an explicit caller input in `drone_core` until the full game-loop cadence is established.

## Respawn and destruction

A recovered respawn path writes:

```text
x = 147
y = 175
frame = 0
```

and restores player activity when lives remain.

The dedicated destruction sequence around `0x0041CDF0` disables the player, emits the recovered composite explosion/effect sequence, inherits player position/motion into effects, and spawns randomized debris. Lives/game-over ownership is now separated from that effect routine: life consumption is deferred to the later state-2 settlement gate, documented in `PLAYER_LIFECYCLE.md`.

## Clean implementation

The independently written reconstruction lives in:

- `include/drone/gameplay/player.hpp`
- `src/gameplay/player.cpp`
- synthetic regression cases in `tests/test_formats.cpp`

Only established motion/frame semantics are represented; this is deliberately narrower than reproducing the entire original `0x154` object layout prematurely.


## Lives and game over

The player starts a normal session with three lives. Player destruction does not decrement the counter immediately; state-2 later consumes one only after the death-effect/player/Drone settlement gate permits it. The original then resets shield energy, frame and `(147,175)` position before either reactivating the player or running the `gameover.jba` banner slide. See `PLAYER_LIFECYCLE.md` for exact evidence and clean-code ownership.
