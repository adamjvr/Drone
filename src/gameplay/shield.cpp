#include <drone/gameplay/shield.hpp>

namespace drone::gameplay {

void reset_player_shield(PlayerShieldState& shield) {
    shield.energy = shield_nominal_max_energy;
    shield.active = false;
}

void regenerate_player_shield(PlayerShieldState& shield) {
    // The original masks the accumulator before comparing against 75<<16.
    // Use an unsigned mask to make the bit operation defined for all int32_t
    // values, then compare the resulting positive high-word quantity.
    const auto high_word_energy = static_cast<std::uint32_t>(shield.energy) & 0xFFFF0000u;
    if (high_word_energy < static_cast<std::uint32_t>(shield_nominal_max_energy)) {
        shield.energy += shield_regen_per_update;
    }
}

PlayerShieldStepResult step_player_shield(
    PlayerShieldState& shield,
    const bool shield_requested,
    const bool player_active,
    const bool animation_tick_is_two) {

    regenerate_player_shield(shield);
    shield.active = false;

    if (shield_requested && player_active) {
        shield.energy -= shield_drain_per_update;
        if (shield.energy < 0) {
            shield.energy = 0;
        }
        if (shield.energy > 0) {
            shield.active = true;
        }
    }

    return PlayerShieldStepResult{
        .active = shield.active,
        .play_sound = shield.active && animation_tick_is_two,
    };
}

std::int32_t displayed_shield_units(const PlayerShieldState& shield) {
    return shield.energy >> 16;
}

} // namespace drone::gameplay
