#pragma once

#include <drone/gameplay/difficulty.hpp>
#include <drone/gameplay/drone_objective.hpp>
#include <drone/gameplay/original_random.hpp>
#include <drone/gameplay/rapid_missile.hpp>
#include <drone/gameplay/scoring.hpp>
#include <drone/gameplay/special_weapon.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>

namespace drone::gameplay {

inline constexpr std::int16_t canonical_drone_collision_width_extent = 12;
inline constexpr std::int16_t canonical_drone_collision_height_extent = 32;
inline constexpr std::uint8_t canonical_drone_weapon_hit_explosion_requests = 8;

struct RapidMissileDroneCollisionResult {
    bool hit = false;
    std::optional<std::size_t> missile_index{};
    bool destruction_countdown_started = false;
    std::uint8_t explosion_spawns_requested = 0;
};

struct SpecialWeaponDroneCollisionResult {
    bool hit = false;
    bool probe_attached = false;
    bool stinger_hit = false;
    bool destruction_countdown_started = false;
    std::uint8_t explosion_spawns_requested = 0;
    std::int32_t score_delta = 0;
};

// Original rapid-missile collision iteration is ascending pool order. The
// missile's X/Y fields are treated as a point and checked by Win32 0x00401F60
// against the active Drone's 85%-derived 12x32 inclusive collision extents.
[[nodiscard]] RapidMissileDroneCollisionResult collide_rapid_missiles_with_drone(
    RapidMissilePool& missiles,
    DroneObjectiveState& drone) noexcept;

// Win32 0x0040F62D performs the same point-vs-Drone hitbox test for a launched
// Probe/Stinger. A Stinger starts the shared destructive countdown; a Probe
// enters attached/decode state, initializes its exact timers and awards +10.
[[nodiscard]] SpecialWeaponDroneCollisionResult collide_special_weapon_with_drone(
    SpecialWeaponState& special,
    DroneObjectiveState& drone,
    DifficultyLevel difficulty,
    bool demo_playback_mode,
    OriginalRandomState& random,
    ScoreState& score) noexcept;

} // namespace drone::gameplay
