#pragma once

#include <cstdint>

namespace drone::gameplay {

// Player shield accumulator recovered from Win32 global 0x0047FCB0.
// The HUD consumes the high 16 bits as the displayed shield-unit count. The
// low 16 bits are a recharge/drain accumulator; the original does not clamp
// them away at the nominal maximum.
struct PlayerShieldState {
    std::int32_t energy = 75 << 16;
    bool active = false; // semantic view of Win32 0x0046198C
};

inline constexpr std::int32_t shield_nominal_units = 75;
inline constexpr std::int32_t shield_nominal_max_energy = shield_nominal_units << 16;
inline constexpr std::int32_t shield_regen_per_update = 0x514; // 1300
inline constexpr std::int32_t shield_drain_per_update = 0xBB80; // 48000

struct PlayerShieldStepResult {
    bool active = false;
    bool play_sound = false;
};

// Respawn/initialization writes exactly 75<<16 and clears the active flag.
void reset_player_shield(PlayerShieldState& shield);

// Mirrors the original high-word-only recharge guard. This deliberately does
// NOT clamp to shield_nominal_max_energy; if the displayed level is 74 while
// the hidden low word is near 0xffff, adding 0x514 can overshoot 75<<16.
void regenerate_player_shield(PlayerShieldState& shield);

// Reconstruct the per-gameplay-update ordering:
//   1. recharge;
//   2. clear active;
//   3. when requested and player active, subtract 0xBB80;
//   4. clamp negative energy to zero;
//   5. active iff energy remains positive;
//   6. request shields.wav only when active and animation_tick_is_two.
PlayerShieldStepResult step_player_shield(
    PlayerShieldState& shield,
    bool shield_requested,
    bool player_active,
    bool animation_tick_is_two);

// Exact HUD interpretation recovered from 0x0041EB70.
std::int32_t displayed_shield_units(const PlayerShieldState& shield);

} // namespace drone::gameplay
