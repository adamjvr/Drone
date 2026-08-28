#include <drone/gameplay/game_over.hpp>

namespace drone::gameplay {

bool step_game_over_banner(GameOverBannerState& state) {
    if (state.velocity_fixed <= 0) {
        return false;
    }

    state.fixed_x -= state.velocity_fixed;
    state.velocity_fixed -= game_over_banner_deceleration_fixed;
    state.x = state.fixed_x >> 16;
    return true;
}

} // namespace drone::gameplay
