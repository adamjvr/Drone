#include <drone/gameplay/player.hpp>

#include <algorithm>

namespace drone::gameplay {
namespace {

void wrap_ship_frame(PlayerMotionState& state) {
    if (state.frame > player_ship_frame_count - 1) {
        state.frame = 0;
    }
    if (state.frame < 0) {
        state.frame = player_ship_frame_count - 1;
    }
}

void recenter_ship_bank(PlayerMotionState& state) {
    // Original Win32 code treats frame zero as neutral. Frames 1..8 walk down
    // toward zero; frames 9..14 walk upward, with 14 -> 15 -> wrap to zero.
    if (state.frame > 0 && state.frame <= 8) {
        --state.frame;
    } else if (state.frame > 8 && state.frame <= 14) {
        ++state.frame;
    }
}

} // namespace

void step_player_directional_motion(
    PlayerMotionState& state,
    const PlayerDirectionalInput& input,
    bool animation_tick) {

    state.horizontal_motion = 0;

    if (input.left) {
        if (animation_tick) {
            ++state.frame;
        }
        state.x -= 2;
        state.horizontal_motion = -1;
    }

    if (input.right) {
        if (animation_tick) {
            --state.frame;
        }
        state.x += 2;
        state.horizontal_motion = 1;
    }

    if (input.up) {
        --state.y;
    }
    if (input.down) {
        ++state.y;
    }

    if (animation_tick && !input.left && !input.right) {
        recenter_ship_bank(state);
    }

    wrap_ship_frame(state);

    state.x = std::clamp(state.x, player_min_x, player_max_x);
    state.y = std::clamp(state.y, player_min_y, player_max_y);
}

} // namespace drone::gameplay
