#include <drone/gameplay/trajectory_spawn.hpp>

#include <algorithm>
#include <cstddef>

namespace drone::gameplay {
namespace {

constexpr std::int32_t kSpawnRollModulus = 1200;
constexpr std::int32_t kRecordingSpawnChance = 28;
constexpr std::int32_t kMaximumActiveGroupGate = 16; // group_count - 1

std::int32_t select_live_candidate_index(
    OriginalRandomState& random,
    const std::int32_t processed_drone_count) noexcept {
    if (processed_drone_count < 2) {
        return static_cast<std::int32_t>(original_random_mod(random, 12));
    }

    if (processed_drone_count < 4) {
        auto candidate = static_cast<std::int32_t>(original_random_mod(random, 16)) + 1;
        // Registered progression 2/3 explicitly excludes fixed groups 12/13
        // by re-rolling through the early 0..11 pool.
        if (candidate > 11 && candidate < 14) {
            candidate = static_cast<std::int32_t>(original_random_mod(random, 12));
        }
        return candidate;
    }

    return static_cast<std::int32_t>(original_random_mod(random, 16)) + 1;
}

std::int32_t select_inactive_live_group(
    const TrajectoryEncounterState& encounter,
    std::int32_t candidate) noexcept {
    if (candidate < 0 || candidate >= static_cast<std::int32_t>(canonical_trajectory_group_count)) {
        candidate = 1;
    }

    // The original can begin at group 0, but after reaching the end of the pool
    // it wraps to group 1 rather than revisiting group 0.
    for (std::size_t attempts = 0; attempts < canonical_trajectory_group_count; ++attempts) {
        if (encounter.groups[static_cast<std::size_t>(candidate)].lifecycle.mode ==
            TrajectoryGroupMode::Inactive) {
            return candidate;
        }
        ++candidate;
        if (candidate >= static_cast<std::int32_t>(canonical_trajectory_group_count)) {
            candidate = 1;
        }
    }
    return -1;
}

void choose_runtime_family_and_group_offsets(
    TrajectoryGroupState& group,
    OriginalRandomState& random) noexcept {
    switch (group.path_family) {
    case TrajectoryPathFamily::Swarm:
        group.group_x_offset = static_cast<std::int16_t>(
            static_cast<std::int32_t>(original_random_mod(random, 200)) - 50);
        break;

    case TrajectoryPathFamily::Swoop:
    case TrajectoryPathFamily::NewCurly:
        group.group_x_offset = static_cast<std::int16_t>(
            static_cast<std::int32_t>(original_random_mod(random, 250)) - 50);
        break;

    case TrajectoryPathFamily::Frisbee1:
    case TrajectoryPathFamily::Generated402:
    case TrajectoryPathFamily::Generated422:
        group.group_y_offset = static_cast<std::int16_t>(
            static_cast<std::int32_t>(original_random_mod(random, 100)) - 50);
        group.group_x_offset = 0;
        break;

    case TrajectoryPathFamily::Frisbee2:
        group.group_x_offset = 0;
        break;

    case TrajectoryPathFamily::LeftDrop:
    case TrajectoryPathFamily::RightDrop:
        group.group_x_offset = 0;
        group.path_family = original_random_mod(random, 10) < 5
            ? TrajectoryPathFamily::LeftDrop
            : TrajectoryPathFamily::RightDrop;
        break;

    default:
        // Loop and either dive direction are converted to one of the two dive
        // descriptors for a live transient activation.
        group.path_family = original_random_mod(random, 10) < 5
            ? TrajectoryPathFamily::LeftDive
            : TrajectoryPathFamily::RightDive;
        group.group_x_offset = static_cast<std::int16_t>(
            static_cast<std::int32_t>(original_random_mod(random, 200)) - 100);
        break;
    }
}

bool update_actor_formation_offsets(
    TrajectoryGroupState& group,
    OriginalRandomState& random,
    const std::int32_t processed_drone_count,
    const bool recording_mode) noexcept {
    if (group.path_family == TrajectoryPathFamily::Swarm || recording_mode) {
        return false;
    }

    const auto count = static_cast<std::size_t>(std::max<std::int8_t>(0, group.lifecycle.entity_count));
    const bool randomize = static_cast<std::int32_t>(original_random_mod(random, 22)) <
        processed_drone_count + 2;

    for (std::size_t i = 0; i < count && i < group.actors.size(); ++i) {
        auto& actor = group.actors[i];
        if (randomize) {
            actor.formation_x_offset = static_cast<std::int16_t>(
                static_cast<std::int32_t>(original_random_mod(random, 60)) - 30);
            actor.formation_y_offset = static_cast<std::int16_t>(
                static_cast<std::int32_t>(original_random_mod(random, 60)) - 30);
        } else {
            actor.formation_x_offset = 0;
            actor.formation_y_offset = 0;
        }
    }
    return randomize;
}

} // namespace

void reset_trajectory_spawn_scheduler(
    TrajectorySpawnSchedulerState& state,
    const DifficultyLevel difficulty,
    const std::int32_t processed_drone_count) noexcept {
    state.interval_threshold = trajectory_spawn_interval_threshold(difficulty, processed_drone_count);
    state.interval_counter = static_cast<std::int16_t>(state.interval_threshold - 30);
}

TrajectorySpawnResult step_live_trajectory_spawn(
    TrajectorySpawnSchedulerState& scheduler,
    TrajectoryEncounterState& encounter,
    const TrajectoryPathCatalogView& paths,
    OriginalRandomState& random,
    const TrajectorySpawnContext& context) noexcept {
    TrajectorySpawnResult result{};
    if (context.demo_playback_mode || context.gameplay_phase != 2) {
        return result;
    }
    result.phase2_tick = true;

    auto spawn_chance = context.demo_recording_mode
        ? kRecordingSpawnChance
        : 3 * context.processed_drone_count +
            4 * static_cast<std::int32_t>(difficulty_multiplier(context.difficulty));

    scheduler.interval_counter = static_cast<std::int16_t>(scheduler.interval_counter + 3);
    if (scheduler.interval_counter >= scheduler.interval_threshold) {
        scheduler.interval_counter = 0;
        spawn_chance = kSpawnRollModulus;
        result.spawn_roll_forced = true;
    }

    // The random roll occurs before the remaining activity/position gates, so
    // preserve the draw even when a spawn is subsequently suppressed.
    const auto roll = static_cast<std::int32_t>(original_random_mod(random, kSpawnRollModulus));
    if (roll >= spawn_chance) {
        return result;
    }
    result.spawn_roll_passed = true;

    if (context.drone_y >= 200 ||
        encounter.active_group_count >= kMaximumActiveGroupGate ||
        context.drone_activity == canonical_drone_destruction_activity ||
        context.mothership_destruction_active) {
        return result;
    }

    const auto candidate = select_live_candidate_index(random, context.processed_drone_count);
    const auto selected = select_inactive_live_group(encounter, candidate);
    if (selected < 0) {
        return result;
    }

    auto& group = encounter.groups[static_cast<std::size_t>(selected)];

    // Activation is followed by the optional one-of-fourteen flight SFX choice.
    if ((next_original_random(random) & 0x7fu) < 0x50u) {
        result.sound_index = static_cast<std::uint8_t>(original_random_mod(random, 14));
    }

    choose_runtime_family_and_group_offsets(group, random);
    result.actor_offsets_randomized = update_actor_formation_offsets(
        group, random, context.processed_drone_count, context.demo_recording_mode);

    result.group_index = static_cast<std::uint8_t>(selected);
    result.runtime_path_family = group.path_family;
    result.group_x_offset = group.group_x_offset;
    result.group_y_offset = group.group_y_offset;
    result.activated = activate_transient_trajectory_group(
        encounter,
        static_cast<std::size_t>(selected),
        paths,
        group.group_x_offset,
        group.group_y_offset);
    return result;
}

} // namespace drone::gameplay
