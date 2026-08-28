#include <drone/gameplay/trajectory_spawn.hpp>

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

namespace {

using drone::formats::FlyRecord;
using namespace drone::gameplay;

TrajectoryPathCatalogView make_paths(
    std::array<std::vector<FlyRecord>, canonical_trajectory_path_family_count>& storage) {
    for (std::size_t family = 0; family < storage.size(); ++family) {
        auto& path = storage[family];
        path.resize(1000);
        for (std::size_t i = 0; i < path.size(); ++i) {
            path[i].x = static_cast<std::int16_t>((17 * family + i) % 300);
            path[i].y = static_cast<std::int16_t>(20 + ((11 * family + i) % 150));
            path[i].aux = 0;
        }
    }
    TrajectoryPathCatalogView view{};
    for (std::size_t i = 0; i < storage.size(); ++i) view.families[i] = storage[i];
    return view;
}

} // namespace

int main() {
    using namespace drone::gameplay;

    // Exact 0x004181E2 reset formula: 310 - 20*processed - 30*difficulty,
    // with the live interval counter initialized 30 below the threshold.
    {
        TrajectorySpawnSchedulerState scheduler{};
        reset_trajectory_spawn_scheduler(scheduler, DifficultyLevel::Beginner, 0);
        assert(scheduler.interval_threshold == 280);
        assert(scheduler.interval_counter == 250);

        reset_trajectory_spawn_scheduler(scheduler, DifficultyLevel::Intermediate, 1);
        assert(scheduler.interval_threshold == 230);
        assert(scheduler.interval_counter == 200);

        reset_trajectory_spawn_scheduler(scheduler, DifficultyLevel::Advanced, 4);
        assert(scheduler.interval_threshold == 140);
        assert(scheduler.interval_counter == 110);
    }

    std::array<std::vector<FlyRecord>, canonical_trajectory_path_family_count> storage{};
    const auto paths = make_paths(storage);

    // Demo playback bypasses the live producer entirely: no scheduler movement
    // and no CRT draw belongs to this branch.
    {
        TrajectoryEncounterState encounter{};
        reset_trajectory_encounter(encounter, &paths);
        TrajectorySpawnSchedulerState scheduler{280, 250};
        OriginalRandomState random{};
        seed_original_random(random, 11);

        const auto result = step_live_trajectory_spawn(
            scheduler, encounter, paths, random,
            TrajectorySpawnContext{
                .gameplay_phase = 2,
                .demo_playback_mode = true,
            });
        assert(!result.phase2_tick);
        assert(!result.activated);
        assert(scheduler.interval_counter == 250);
        assert(random.draws == 0);
    }

    // A forced interval crossing still consumes the rand()%1200 roll before
    // later gates. Drone Y>=200 suppresses activation after exactly that draw.
    {
        TrajectoryEncounterState encounter{};
        reset_trajectory_encounter(encounter, &paths);
        TrajectorySpawnSchedulerState scheduler{280, 277};
        OriginalRandomState random{};
        seed_original_random(random, 11);

        const auto result = step_live_trajectory_spawn(
            scheduler, encounter, paths, random,
            TrajectorySpawnContext{
                .gameplay_phase = 2,
                .drone_y = 200,
            });
        assert(result.phase2_tick);
        assert(result.spawn_roll_forced);
        assert(result.spawn_roll_passed);
        assert(!result.activated);
        assert(scheduler.interval_counter == 0);
        assert(random.draws == 1);
    }

    // Early shareware progression draws candidates from 0..11. Seed 11 makes
    // the second CRT draw select group 0. If primary group 0 has retired, the
    // original live producer is allowed to reactivate it as transient mode 2.
    {
        TrajectoryEncounterState encounter{};
        reset_trajectory_encounter(encounter, &paths);
        encounter.groups[0].lifecycle.mode = TrajectoryGroupMode::Inactive;
        encounter.groups[0].lifecycle.active_entity_count = 0;
        for (auto& actor : encounter.groups[0].actors) {
            actor.activity = TrajectoryEntityActivity::Inactive;
        }
        encounter.active_group_count = 0;

        TrajectorySpawnSchedulerState scheduler{280, 277};
        OriginalRandomState random{};
        seed_original_random(random, 11);

        const auto result = step_live_trajectory_spawn(
            scheduler, encounter, paths, random,
            TrajectorySpawnContext{.gameplay_phase = 2});
        assert(result.activated);
        assert(result.group_index == 0);
        assert(encounter.groups[0].lifecycle.mode == TrajectoryGroupMode::RetireOnPathWrap);
        assert(encounter.groups[0].actors[0].activity == TrajectoryEntityActivity::FollowingPath);
        assert(encounter.active_group_count == 1);
    }

    // Registered progression counts 2/3 draw 1..16 but explicitly reroll fixed
    // groups 12/13 through the early 0..11 pool. Seed 3 first selects 13 and
    // the reroll selects 6, proving the odd exclusion branch and extra RNG draw.
    {
        TrajectoryEncounterState encounter{};
        reset_trajectory_encounter(encounter, &paths);
        TrajectorySpawnSchedulerState scheduler{
            trajectory_spawn_interval_threshold(DifficultyLevel::Beginner, 2),
            static_cast<std::int16_t>(trajectory_spawn_interval_threshold(
                DifficultyLevel::Beginner, 2) - 3)};
        OriginalRandomState random{};
        seed_original_random(random, 3);

        const auto result = step_live_trajectory_spawn(
            scheduler, encounter, paths, random,
            TrajectorySpawnContext{
                .processed_drone_count = 2,
                .gameplay_phase = 2,
            });
        assert(result.activated);
        assert(result.group_index == 6);
        assert(result.group_index != 12 && result.group_index != 13);
        assert(random.draws >= 3); // roll + rejected candidate + reroll
    }

    // The active-group suppression gate is strict at 16 and occurs after the
    // spawn roll, preventing the producer from exhausting all 17 slots.
    {
        TrajectoryEncounterState encounter{};
        reset_trajectory_encounter(encounter, &paths);
        encounter.active_group_count = 16;
        TrajectorySpawnSchedulerState scheduler{280, 277};
        OriginalRandomState random{};
        seed_original_random(random, 1);

        const auto result = step_live_trajectory_spawn(
            scheduler, encounter, paths, random,
            TrajectorySpawnContext{.gameplay_phase = 2});
        assert(result.spawn_roll_forced && result.spawn_roll_passed);
        assert(!result.activated);
        assert(random.draws == 1);
    }

    std::cout << "trajectory spawn tests passed\n";
    return 0;
}
