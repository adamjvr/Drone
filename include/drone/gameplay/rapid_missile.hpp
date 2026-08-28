#pragma once

#include <drone/gameplay/player.hpp>

#include <array>
#include <cstddef>
#include <cstdint>

namespace drone::gameplay {

// Reconstructed Win32 rapid-fire missile pool rooted at 0x0042F200.
// Only fields whose behavior is established in the original state-2 paths are
// represented here; the original entries are full 0x154-byte common entities.
struct RapidMissileState {
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::uint8_t frame = 0;             // original entity +0x140, frames 0..2
    bool active = false;                // semantic view of +0x142 == 1
    bool passed_top_edge = false;       // semantic view of +0x143 after y < 0
};

struct RapidMissilePool {
    static constexpr std::size_t capacity = 8;
    static constexpr std::int32_t cooldown_ready = 8;

    std::array<RapidMissileState, capacity> missiles{};
    std::int32_t active_count = 0;       // Win32 0x00440274
    std::int32_t fire_cooldown = cooldown_ready; // Win32 0x004406F4
};

// Reconstructs the state-2 cooldown increment. It saturates at eight.
void advance_rapid_missile_cooldown(RapidMissilePool& pool);

// Reconstructs the Ctrl-fire allocation/spawn path. The original caller also
// gates this on the player entity being in active state 1; callers expose that
// explicitly instead of folding an unknown original entity state into PlayerMotionState.
// Returns true only when a missile is actually allocated.
bool try_fire_rapid_missile(
    RapidMissilePool& pool,
    const PlayerMotionState& player,
    bool fire_requested,
    bool player_active);

// Reconstructs the per-update missile movement/animation path. Every active
// missile moves upward by three pixels. On the original animation-tick
// condition its three-frame animation advances and wraps 0..2.
void step_rapid_missiles(RapidMissilePool& pool, bool animation_tick);

// Reconstructs the top-bound cleanup in the later collision pass. The update
// path marks +0x143 as soon as y < 0, but the missile is not deactivated for
// leaving the screen until y < -7. Returns the number retired.
std::size_t retire_rapid_missiles_above_top(RapidMissilePool& pool);

// Collision paths in the original deactivate a missile first and decrement the
// pool's active counter once at the end of that missile's collision iteration.
// This helper provides the corresponding clean operation for reconstructed
// collision handlers.
bool deactivate_rapid_missile(RapidMissilePool& pool, std::size_t index);

} // namespace drone::gameplay
