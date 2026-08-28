#include <drone/gameplay/game_session.hpp>
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
            path[i].x = static_cast<std::int16_t>((19 * family + i) % 300);
            path[i].y = static_cast<std::int16_t>(30 + ((13 * family + i) % 150));
            path[i].aux = 0;
        }
    }
    TrajectoryPathCatalogView view{};
    for (std::size_t i = 0; i < storage.size(); ++i) view.families[i] = storage[i];
    return view;
}

void retire_primary_slot(TrajectoryEncounterState& encounter, const std::size_t index) {
    auto& primary = encounter.groups[0];
    assert(index < static_cast<std::size_t>(primary.lifecycle.entity_count));
    assert(primary.actors[index].activity != TrajectoryEntityActivity::Inactive);
    primary.actors[index].activity = TrajectoryEntityActivity::Inactive;
    --primary.lifecycle.active_entity_count;
}

} // namespace

int main() {
    // The replenisher is completely skipped on shared phase 2, including RNG.
    {
        TrajectoryEncounterState encounter{};
        reset_trajectory_encounter(encounter);
        retire_primary_slot(encounter, 0);
        OriginalRandomState random{};
        seed_original_random(random, 1);
        const auto result = step_primary_trajectory_replenishment(
            encounter, random,
            PrimaryTrajectoryReplenishmentContext{.gameplay_phase = 2});
        assert(!result.eligible_substep);
        assert(!result.activated);
        assert(random.draws == 0);
    }

    // Strict probability gate: Beginner/count0 threshold is 4. Seed 3125 gives
    // low-11-bit roll 3 and therefore passes; seed 1871 gives exactly 4 and
    // therefore fails. A full primary group prevents activation after the pass.
    {
        TrajectoryEncounterState encounter{};
        reset_trajectory_encounter(encounter);
        OriginalRandomState pass_random{};
        seed_original_random(pass_random, 3125);
        const auto pass = step_primary_trajectory_replenishment(
            encounter, pass_random,
            PrimaryTrajectoryReplenishmentContext{.gameplay_phase = 1});
        assert(pass.eligible_substep);
        assert(pass.spawn_roll_passed);
        assert(!pass.activated);
        assert(pass_random.draws == 1);

        OriginalRandomState miss_random{};
        seed_original_random(miss_random, 1871);
        const auto miss = step_primary_trajectory_replenishment(
            encounter, miss_random,
            PrimaryTrajectoryReplenishmentContext{.gameplay_phase = 1});
        assert(!miss.spawn_roll_passed);
        assert(miss_random.draws == 1);
    }

    // Demo playback still consumes the initial random draw but forces its
    // effective value to 1. First inactive slot wins, retained path/frame state
    // survives, and seed1 gives the right-side entry. Primary replenishment
    // consumes exactly the initial gate draw plus the entry-position draw.
    {
        TrajectoryEncounterState encounter{};
        reset_trajectory_encounter(encounter);
        retire_primary_slot(encounter, 2);
        auto& actor = encounter.groups[0].actors[2];
        actor.path_index = 123;
        actor.current_frame = 9;

        OriginalRandomState random{};
        seed_original_random(random, 1);
        const auto result = step_primary_trajectory_replenishment(
            encounter, random,
            PrimaryTrajectoryReplenishmentContext{
                .gameplay_phase = 0,
                .demo_playback_mode = true,
            });
        assert(result.roll_forced_to_one);
        assert(result.spawn_roll_passed);
        assert(result.activated);
        assert(result.actor_index == 2);
        assert(result.entry_x == 350 && result.entry_y == 100);
        assert(actor.activity == TrajectoryEntityActivity::AcquiringPath);
        assert(actor.path_index == 123);
        assert(actor.current_frame == 9);
        assert(encounter.groups[0].lifecycle.active_entity_count == 7);
        assert(random.draws == 2);
    }

    // Empty group0 itself forces the effective roll to one. Seed4 yields the
    // top entry, and the group is restored to persistent mode/count ownership.
    {
        TrajectoryEncounterState encounter{};
        reset_trajectory_encounter(encounter);
        auto& primary = encounter.groups[0];
        for (auto& actor : primary.actors) actor.activity = TrajectoryEntityActivity::Inactive;
        primary.lifecycle.active_entity_count = 0;
        primary.lifecycle.mode = TrajectoryGroupMode::Inactive;
        encounter.active_group_count = 0;

        OriginalRandomState random{};
        seed_original_random(random, 4);
        const auto result = step_primary_trajectory_replenishment(
            encounter, random,
            PrimaryTrajectoryReplenishmentContext{.gameplay_phase = 3});
        assert(result.roll_forced_to_one);
        assert(result.activated);
        assert(result.group_reactivated);
        assert(result.actor_index == 0);
        assert(result.entry_x == 160 && result.entry_y == -30);
        assert(primary.lifecycle.mode == TrajectoryGroupMode::PersistentLoop);
        assert(primary.lifecycle.active_entity_count == 1);
        assert(encounter.active_group_count == 1);
    }

    // Suppression by Drone destruction occurs after the consumed/forced roll
    // but before the entry-position draw.
    {
        TrajectoryEncounterState encounter{};
        reset_trajectory_encounter(encounter);
        retire_primary_slot(encounter, 1);
        OriginalRandomState random{};
        seed_original_random(random, 2);
        const auto result = step_primary_trajectory_replenishment(
            encounter, random,
            PrimaryTrajectoryReplenishmentContext{
                .gameplay_phase = 0,
                .demo_playback_mode = true,
                .drone_activity = canonical_drone_destruction_activity,
            });
        assert(result.spawn_roll_passed);
        assert(!result.activated);
        assert(random.draws == 1);
    }

    std::array<std::vector<FlyRecord>, canonical_trajectory_path_family_count> storage{};
    const auto paths = make_paths(storage);

    // Whole-session ownership: primary replenishment increments the encounter
    // local 0x00466B04 semantic total from the reset value seven to eight.
    {
        GameSession session{};
        session.runtime.demo_playback_mode = true;
        retire_primary_slot(session.encounter.trajectories, 0);
        seed_original_random(session.original_random, 2); // left entry
        const auto tick = step_game_session(
            session, GameplayInputFrame{},
            GameSessionTargetContext{.trajectory_paths = &paths});
        assert(tick.primary_trajectory_replenishment_checked);
        assert(tick.primary_trajectory_actor_replenished);
        assert(tick.primary_trajectory_actor_index == 0);
        assert(tick.primary_trajectory_entry_x == -30 && tick.primary_trajectory_entry_y == 100);
        assert(session.encounter.encounter_alien_ships_total == 8);
        assert(tick.encounter_alien_ships_total == 8);
    }

    // A native transient group's first actor also contributes one to the same
    // encounter-local total. Primary replenishment is phase-2 skipped here.
    {
        GameSession session{};
        session.encounter.gameplay_substep_phase = 1; // next update is phase 2
        session.encounter.trajectory_spawn.interval_counter =
            static_cast<std::int16_t>(session.encounter.trajectory_spawn.interval_threshold - 3);
        seed_original_random(session.original_random, 11);
        const auto tick = step_game_session(
            session, GameplayInputFrame{},
            GameSessionTargetContext{.trajectory_paths = &paths});
        assert(tick.trajectory_group_spawned);
        assert(session.encounter.encounter_alien_ships_total == 8);
        assert(tick.encounter_alien_ships_total == 8);
    }

    // Later stagger activation increments the same total by one more actor.
    {
        GameSession session{};
        const bool activated = activate_transient_trajectory_group(
            session.encounter.trajectories, 1, paths);
        assert(activated);
        session.encounter.encounter_alien_ships_total = 8; // first actor already accounted
        auto& group = session.encounter.trajectories.groups[1];
        group.lifecycle.spawn_delay_counter = static_cast<std::int16_t>(
            group.lifecycle.spawn_delay_interval - 1);
        seed_original_random(session.original_random, 1);
        const auto tick = step_game_session(
            session, GameplayInputFrame{},
            GameSessionTargetContext{.trajectory_paths = &paths});
        assert(tick.trajectory_actors_activated == 1);
        assert(session.encounter.encounter_alien_ships_total == 9);
        assert(tick.encounter_alien_ships_total == 9);
    }

    std::cout << "primary trajectory replenishment tests passed\n";
    return 0;
}
