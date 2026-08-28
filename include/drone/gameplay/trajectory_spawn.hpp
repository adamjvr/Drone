#pragma once

#include <drone/gameplay/difficulty.hpp>
#include <drone/gameplay/drone_objective.hpp>
#include <drone/gameplay/original_random.hpp>
#include <drone/gameplay/trajectory_encounter.hpp>

#include <cstdint>
#include <optional>

namespace drone::gameplay {

// Win32 0x00466B04 resets to the seven persistent primary actors at each
// encounter rebuild. This is distinct from the mission-wide Results total.
inline constexpr std::int32_t canonical_initial_encounter_alien_ships_total = 7;

struct PrimaryTrajectoryReplenishmentContext {
    DifficultyLevel difficulty = DifficultyLevel::Beginner;
    std::int32_t processed_drone_count = 0;
    std::int32_t gameplay_phase = 0;
    bool demo_playback_mode = false;
    std::uint8_t drone_activity = canonical_drone_active_activity;
};

struct PrimaryTrajectoryReplenishmentResult {
    bool eligible_substep = false;
    bool roll_forced_to_one = false;
    bool spawn_roll_passed = false;
    bool activated = false;
    bool group_reactivated = false;
    std::optional<std::uint8_t> actor_index{};
    std::int32_t entry_x = 0;
    std::int32_t entry_y = 0;
};

// Reconstruct Win32 0x0040CEE8..0x0040D070.
// This is the persistent primary (group 0) replenisher, distinct from the
// phase-2 transient formation producer below. It preserves retained actor path
// index/frame state and re-enters an inactive actor through activity 3.
[[nodiscard]] PrimaryTrajectoryReplenishmentResult step_primary_trajectory_replenishment(
    TrajectoryEncounterState& encounter,
    OriginalRandomState& random,
    const PrimaryTrajectoryReplenishmentContext& context) noexcept;

// Win32 globals 0x00464B38 / 0x0045BEFC. The threshold is rebuilt at each
// encounter reset from processed-Drone count and difficulty; the counter then
// advances only on shared gameplay phase 2.
struct TrajectorySpawnSchedulerState {
    std::int16_t interval_threshold = 280;
    std::int16_t interval_counter = 250;
};

struct TrajectorySpawnContext {
    DifficultyLevel difficulty = DifficultyLevel::Beginner;
    std::int32_t processed_drone_count = 0;
    std::int32_t gameplay_phase = 0;
    bool demo_playback_mode = false;
    bool demo_recording_mode = false;
    std::int32_t drone_y = canonical_drone_session_initial_y;
    std::uint8_t drone_activity = canonical_drone_active_activity;

    // Registered-only Mothership core state 2 suppresses new formations in the
    // original. The shareware path never reaches it, so geometry/state ownership
    // remains an explicit producer until the Mothership encounter is integrated.
    bool mothership_destruction_active = false;
};

struct TrajectorySpawnResult {
    bool phase2_tick = false;
    bool spawn_roll_forced = false;
    bool spawn_roll_passed = false;
    bool activated = false;
    std::optional<std::uint8_t> group_index{};
    TrajectoryPathFamily runtime_path_family = TrajectoryPathFamily::Loop;
    std::int16_t group_x_offset = 0;
    std::int16_t group_y_offset = 0;
    bool actor_offsets_randomized = false;
    std::optional<std::uint8_t> sound_index{};
};

[[nodiscard]] constexpr std::int16_t trajectory_spawn_interval_threshold(
    const DifficultyLevel difficulty,
    const std::int32_t processed_drone_count) noexcept {
    return static_cast<std::int16_t>(
        310 - 20 * processed_drone_count -
        30 * static_cast<std::int32_t>(difficulty_multiplier(difficulty)));
}

void reset_trajectory_spawn_scheduler(
    TrajectorySpawnSchedulerState& state,
    DifficultyLevel difficulty,
    std::int32_t processed_drone_count) noexcept;

// Reconstruct the normal-live transient formation producer in Win32
// 0x0040D390..0x0040D947. Demo playback uses replay channels instead and is
// intentionally not synthesized here.
[[nodiscard]] TrajectorySpawnResult step_live_trajectory_spawn(
    TrajectorySpawnSchedulerState& scheduler,
    TrajectoryEncounterState& encounter,
    const TrajectoryPathCatalogView& paths,
    OriginalRandomState& random,
    const TrajectorySpawnContext& context) noexcept;

} // namespace drone::gameplay
