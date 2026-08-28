#pragma once

#include <cstdint>

namespace drone::gameplay {

inline constexpr std::int32_t game_over_banner_start_x = 325;
inline constexpr std::int32_t game_over_banner_y = 90;
inline constexpr std::int32_t game_over_banner_width = 117;
inline constexpr std::int32_t game_over_banner_height = 20;
inline constexpr std::int32_t game_over_banner_initial_velocity_fixed = 270000;
inline constexpr std::int32_t game_over_banner_deceleration_fixed = 2500;

// Pure state corresponding to the fixed-point horizontal slide performed by
// Win32 0x0041E420. Presentation (restore framebuffer, blit, vblank, present)
// remains outside the gameplay core.
struct GameOverBannerState {
    std::int32_t x = game_over_banner_start_x;
    std::int32_t y = game_over_banner_y;
    std::int32_t fixed_x = game_over_banner_start_x << 16;
    std::int32_t velocity_fixed = game_over_banner_initial_velocity_fixed;
};

// Advances one original banner-presentation iteration. Returns false when the
// initial velocity is already non-positive and no frame should be presented.
bool step_game_over_banner(GameOverBannerState& state);

} // namespace drone::gameplay
