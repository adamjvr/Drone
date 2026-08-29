#pragma once

#include <drone/gameplay/player.hpp>

#include <cstdint>

namespace drone::gameplay {

// Singleton player-destruction explosion entity rooted at Win32 0x00491CE0.
// It reuses explode1-derived frame pixels, but its lifecycle is independent of
// the normal explosion pool. Only semantic state lives here; immutable sprite
// pixels remain an asset/presentation input.
enum class PlayerDeathEffectActivity : std::uint8_t {
    Inactive = 0,
    Visible = 1,
    PreRoll = 3,
};

inline constexpr std::int32_t canonical_player_death_effect_width = 42;
inline constexpr std::int32_t canonical_player_death_effect_height = 38;
inline constexpr std::int32_t canonical_player_death_effect_terminal_frame = 27;
inline constexpr std::int32_t canonical_player_death_effect_initial_frame = -6;

struct PlayerDeathEffectState {
    std::int32_t x = 30;
    std::int32_t y = 31;
    std::int32_t motion_x = 0;
    std::int32_t motion_y = 0;
    std::int32_t frame = 0;
    PlayerDeathEffectActivity activity = PlayerDeathEffectActivity::Inactive;
};

struct PlayerDeathEffectStepResult {
    bool advanced = false;
    bool cleared_out_of_bounds = false;
    bool became_visible = false;
    bool retired_at_terminal_frame = false;
};

[[nodiscard]] constexpr bool player_death_effect_inactive(
    const PlayerDeathEffectState& state) noexcept {
    return state.activity == PlayerDeathEffectActivity::Inactive;
}

[[nodiscard]] constexpr bool player_death_effect_visible(
    const PlayerDeathEffectState& state) noexcept {
    return state.activity == PlayerDeathEffectActivity::Visible;
}

// Reconstructs the state-bearing portion of trigger_player_destruction_sequence
// (Win32 0x0041CDF0). The original also emits randomized explosion/debris/audio
// presentation; those draws/events remain separate until their presentation
// owner is integrated.
void trigger_player_death_effect(
    PlayerDeathEffectState& state,
    const PlayerMotionState& player) noexcept;

// Reconstructs Win32 0x0040E1DA..0x0040E271. The singleton advances only on
// gameplay substep phase 2. Bounds clearing deliberately occurs before frame
// increment/zero activation, preserving the original same-update ordering.
[[nodiscard]] PlayerDeathEffectStepResult step_player_death_effect(
    PlayerDeathEffectState& state,
    std::int32_t gameplay_substep_phase) noexcept;

} // namespace drone::gameplay
