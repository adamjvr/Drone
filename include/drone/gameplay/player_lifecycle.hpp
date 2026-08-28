#pragma once

#include <cstdint>

#include <drone/gameplay/player.hpp>
#include <drone/gameplay/shield.hpp>

namespace drone::gameplay {

inline constexpr std::int32_t canonical_starting_lives = 3;
inline constexpr std::int32_t canonical_respawn_x = 147;
inline constexpr std::int32_t canonical_respawn_y = 175;

// The original consumes a life only after the player-death presentation has
// settled far enough to permit respawn. These booleans expose only the proven
// gate semantics; the clean API keeps the gate decision separate from the bomb subsystem while preserving original ownership in documentation.
struct PlayerRespawnGate {
    bool bomb_spawn_gate_allows_settlement = false;
    bool death_effect_inactive = false;
    bool player_inactive = false;
    bool drone_allows_respawn = false;
};

struct PlayerLifecycleState {
    std::int32_t lives = canonical_starting_lives;
    bool player_active = true;
};

struct PlayerRespawnResolution {
    bool consumed_life = false;
    bool respawned = false;
    bool game_over = false;
    bool shield_reset = false;
};

// Reconstructs the Win32 state-2 death-settlement branch at
// 0x0040E272..0x0040E2DB. Collision/destruction does not decrement lives;
// callers invoke this only while processing the later settlement gate.
PlayerRespawnResolution settle_player_death(
    PlayerLifecycleState& lifecycle,
    PlayerMotionState& player,
    PlayerShieldState& shield,
    const PlayerRespawnGate& gate);

} // namespace drone::gameplay
