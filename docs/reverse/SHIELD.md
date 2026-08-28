# Player Shield Reconstruction — Win32 Shareware

This document records the evidence-backed Space-key shield behavior recovered during Phase 2. The clean implementation is intentionally narrow: it reconstructs the energy/protection state machine and sound cadence, while the complete original pixel effect remains a rendering-research item.

## Inputs and replay channel

The live Win32 state-2 input path tests `VK_SPACE (0x20)`. Demo playback substitutes replay channel 5 for the same logical action. This matches the supplied README: **Space activates the shield**, while Ctrl is the normal rapid-fire missile.

The shield sound pool rooted at `0x004336A8` is independently initialized from `shields.wav`, so input, audio, documentation, and gameplay state all converge on the same interpretation.

## Energy accumulator

Global **`0x0047FCB0`** is the player shield-energy accumulator.

The HUD routine at `0x0041EB70` reads it and uses:

```text
displayed_units = shield_energy >> 16
```

so the high 16 bits are the visible integer shield level and the low 16 bits act as an internal fractional accumulator.

Canonical initialization and player-respawn paths write:

```text
75 << 16 = 0x004B0000
```

Therefore the nominal full shield is **75 displayed units**.

## Per-update regeneration

Before shield-input consumption, state 2 performs:

```cpp
if ((shield_energy & 0xFFFF0000) < (75 << 16))
    shield_energy += 0x514; // 1300
```

This comparison is deliberately unusual: it examines only the high word and does **not** clamp the complete 32-bit accumulator to exactly `75<<16` afterward.

Consequently a value such as `74.996...` units can receive one final `+1300` increment and land slightly above the exact nominal fixed-point maximum while still displaying 75. The clean implementation preserves this behavior rather than replacing it with a conventional saturated recharge.

## Activation and drain

Global byte **`0x0046198C`** is the per-update shield-active/protection flag.

The recovered ordering is:

```cpp
// recharge first
if ((energy & 0xFFFF0000) < (75 << 16))
    energy += 0x514;

shield_active = false;

if (shield_requested && player_active) {
    energy -= 0xBB80; // 48000

    if (energy < 0)
        energy = 0;

    if (energy > 0)
        shield_active = true;
}
```

Important fidelity consequences:

- recharge still occurs on an update where Space is held;
- the drain is **48,000 accumulator units per gameplay update**;
- a negative post-drain result is clamped to exactly zero;
- protection is active only if energy remains greater than zero after the drain;
- if the player entity is inactive, Space does not drain or activate the shield, although the ordinary recharge step still occurs.

No wall-clock duration is assigned to these constants yet because the canonical simulation cadence remains under reconstruction.

## Sound cadence

When the shield remains active, the original calls the `shields.wav` 20-voice pool only when the shared animation/tick global `0x0053C4D8` equals `2`.

The clean gameplay API therefore accepts that already-derived cadence condition as a Boolean; it does not assign an unsupported Hertz value to the underlying tick.

## HUD meter

Function **`0x0041EB70`** is established as the shield-meter renderer because it:

1. loads `0x0047FCB0`;
2. shifts right 16;
3. iterates once per resulting shield unit;
4. changes the palette/index used for the graphical meter around thresholds near **10** and **25**.

This confirms both the 75-unit scale and the high-word interpretation independently of the input path.

Exact geometry/palette semantics of every meter element can be reconstructed later without changing the gameplay accumulator model.

## Visible shield effect

When `0x0046198C` is active, the player render path calls **`0x0041E6D0`** with player-centered geometry.

The first recovered portion directly modifies pixels in the software framebuffer inside the player sprite region, adding a small random palette-index perturbation to existing pixels; later code consumes additional shield-effect tables.

The function is therefore named provisionally but strongly as `render_player_shield_effect`. Its role is established; the complete pixel algorithm remains **partial** and is not yet reimplemented in `drone_core`.

## Collision ownership

Player-impact paths consult the same `0x0046198C` protection flag. At least the enemy-bomb collision cluster distinguishes normal player destruction from a shield-protected effect path.

Exact damage/effect consequences for every enemy family remain subsystem work, but the ownership of the protection flag is established.

## Clean implementation

Evidence-backed state logic lives in:

- `include/drone/gameplay/shield.hpp`
- `src/gameplay/shield.cpp`
- synthetic regression coverage in `tests/test_formats.cpp`

The tests specifically preserve:

- exact reset value `75<<16`;
- high-word-only recharge guard;
- fractional overshoot at the nominal maximum;
- recharge-before-drain ordering;
- `0xBB80` drain;
- zero clamp;
- player-active gate;
- sound cadence gate;
- HUD high-word conversion.

## Remaining shield questions

The following remain open without blocking the clean energy model:

- complete pixel-for-pixel reconstruction of `0x0041E6D0`;
- exact graphical meaning of the HUD palette thresholds;
- all enemy-specific collision consequences while protected;
- wall-clock recharge/drain rates, pending recovery of the true simulation cadence.

## Bomb collision consequence

The bomb/player branch at `0x0040F4BB..0x0040F589` independently proves the protection semantics of `player_shield_active`. A colliding bomb is consumed in both cases. With shield inactive, `bigexp3.wav` is requested and the player destruction routine runs; with shield active, bomb motion is zeroed and the bomb is converted through `spawn_mini_explosion_sprite` without destroying the player. This decision and collision producer are now owned by the `GameSession` late bomb pass; rendering/audio remain presentation events. See [ENEMY_BOMBS.md](ENEMY_BOMBS.md).


## Presentation cross-reference

The exact late-gameplay HUD geometry and presentation-only status/meter behavior are documented in [`HUD_PRESENTATION.md`](HUD_PRESENTATION.md).
