#pragma once

#include <cstdint>

namespace drone::gameplay {

// Clean representation of the player-motion state recovered from the Win32
// gameplay loop. This is intentionally narrower than the original 0x154-byte
// entity: only fields whose player-control semantics are established live here.
struct PlayerMotionState {
    std::int32_t x = 147;
    std::int32_t y = 175;
    std::int32_t horizontal_motion = 0; // original entity +0x10: -1, 0, +1 in player input path
    std::int32_t frame = 0;             // original entity +0x140, valid ship frames 0..14
};

struct PlayerDirectionalInput {
    bool left = false;
    bool right = false;
    bool up = false;
    bool down = false;
};

inline constexpr std::int32_t player_min_x = 2;
inline constexpr std::int32_t player_max_x = 297;
inline constexpr std::int32_t player_min_y = 120;
inline constexpr std::int32_t player_max_y = 175;
inline constexpr std::int32_t player_ship_frame_count = 15;

// Reconstructs the directional-control portion of Win32 state-2 gameplay.
//
// The original processes left then right, followed by up then down. Therefore
// simultaneous opposing directions cancel position deltas, while the later
// horizontal direction leaves horizontal_motion at +1. Ship bank animation is
// advanced only on the original code's separate animation-tick condition;
// callers supply that condition explicitly rather than assigning semantics to
// the still-unnamed original timing/mode global.
void step_player_directional_motion(
    PlayerMotionState& state,
    const PlayerDirectionalInput& input,
    bool animation_tick);

} // namespace drone::gameplay
