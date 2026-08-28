#pragma once

#include <drone/gameplay/rapid_missile.hpp>
#include <drone/gameplay/special_weapon.hpp>
#include <drone/gameplay/trajectory_encounter.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace drone::gameplay {

inline constexpr std::size_t canonical_trajectory_collision_frame_slots = 32;

// Immutable extracted current-frame sprite masks. The Win32 rapid-missile
// trajectory path samples palette index zero as transparent; mutable trajectory
// lifecycle stays inside GameSession while callers provide only asset pixels.
struct TrajectorySpriteMaskCatalogView {
    std::array<std::array<std::span<const std::uint8_t>, canonical_trajectory_collision_frame_slots>,
               canonical_trajectory_group_count> frames{};

    [[nodiscard]] std::span<const std::uint8_t> frame(
        std::size_t group_index,
        std::uint8_t frame_index) const noexcept;
};

// Win32 0x00434C10: separate six-frame stinger.jba display/effect entity.
inline constexpr std::int16_t canonical_stinger_display_width = 61;
inline constexpr std::int16_t canonical_stinger_display_height = 53;
inline constexpr std::int16_t canonical_stinger_display_hitbox_width = 51;
inline constexpr std::int16_t canonical_stinger_display_hitbox_height = 45;
inline constexpr std::uint8_t canonical_stinger_display_frame_count = 6;

struct StingerDisplayState {
    std::int32_t x = 146;
    std::int32_t y = 182;
    std::uint8_t current_frame = 0;
    bool active = false;
};

void activate_stinger_display_at(
    StingerDisplayState& display,
    std::int32_t special_x,
    std::int32_t special_y) noexcept;

void advance_stinger_display(StingerDisplayState& display) noexcept;

struct TrajectoryWeaponCollisionResult {
    std::size_t collisions = 0;
    std::size_t actors_destroyed = 0;
    std::size_t groups_retired = 0;
    std::uint32_t destruction_bursts = 0;
    std::int32_t score_delta = 0;
    std::size_t rapid_missiles_consumed = 0;
    bool special_collision = false;
    bool stinger_display_activated = false;
};

// Win32 update_trajectory_groups rapid-fire path: actors are outer-looped,
// missiles are checked in ascending pool order against the actor's current
// extracted frame with 0x00401FA0. A hit consumes the missile and adds 3 damage.
[[nodiscard]] TrajectoryWeaponCollisionResult collide_rapid_missiles_with_trajectories(
    TrajectoryEncounterState& encounter,
    RapidMissilePool& missiles,
    const TrajectorySpriteMaskCatalogView& masks,
    ScoreState& score) noexcept;

// Win32 0x0040ED85 Stinger display path. Display frames 3..5 use the display's
// centered 51x45 hitbox against each actor's full sprite rectangle and add 15
// damage. The display is not consumed and can hit multiple actors per update.
[[nodiscard]] TrajectoryWeaponCollisionResult collide_stinger_display_with_trajectories(
    TrajectoryEncounterState& encounter,
    const StingerDisplayState& display,
    ScoreState& score) noexcept;

// Win32 late launched Probe/Stinger path. `entered_late_block_as_launched`
// captures the original state-3 gate before the Drone collision; the actor scan
// still runs later in that update even if the Drone collision changed activity.
// Collision is point-vs-actor 0.85 hitbox and destroys actors directly. A Probe
// can therefore destroy multiple overlapping actors despite becoming inactive
// after the first hit. A live Stinger remains launched and activates/repositions
// the separate Stinger display on every direct hit.
[[nodiscard]] TrajectoryWeaponCollisionResult collide_launched_special_with_trajectories(
    TrajectoryEncounterState& encounter,
    SpecialWeaponState& special,
    bool entered_late_block_as_launched,
    bool demo_playback_mode,
    StingerDisplayState& stinger_display,
    ScoreState& score) noexcept;

} // namespace drone::gameplay
