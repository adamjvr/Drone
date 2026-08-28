#include <drone/gameplay/alien_accounting.hpp>
#include <drone/gameplay/game_session.hpp>

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

namespace {

drone::gameplay::TrajectoryPathCatalogView make_paths(
    std::array<std::vector<drone::formats::FlyRecord>, drone::gameplay::canonical_trajectory_path_family_count>& storage) {
    for (auto& path : storage) {
        path.resize(1000);
        for (std::size_t i = 0; i < path.size(); ++i) {
            path[i].x = static_cast<std::int16_t>(i % 300);
            path[i].y = static_cast<std::int16_t>(20 + (i % 150));
            path[i].aux = 0;
        }
    }
    drone::gameplay::TrajectoryPathCatalogView view{};
    for (std::size_t i = 0; i < storage.size(); ++i) view.families[i] = storage[i];
    return view;
}

void arm_single_trajectory_actor(drone::gameplay::GameSession& session) {
    using namespace drone::gameplay;
    auto& group = session.encounter.trajectories.groups[1];
    group.lifecycle.mode = TrajectoryGroupMode::RetireOnPathWrap;
    group.lifecycle.entity_count = 1;
    group.lifecycle.active_entity_count = 1;
    group.lifecycle.activated_entity_count = 1;
    group.actors[0].activity = TrajectoryEntityActivity::FollowingPath;
    group.actors[0].damage_accumulator = 0;
    group.actors[0].destruction_threshold = 3;
    group.actors[0].score_value = 1;
}

} // namespace

int main() {
    using namespace drone::gameplay;

    {
        const auto stats = make_encounter_alien_statistics(3, 8);
        assert(stats.has_value());
        assert(stats->hit == 3);
        assert(stats->missed == 5);
        assert(stats->total == 8);
        assert(stats->percentage_hit == 37);
    }

    assert(!make_encounter_alien_statistics(-1, 8).has_value());
    assert(!make_encounter_alien_statistics(9, 8).has_value());
    assert(!make_encounter_alien_statistics(0, 0).has_value());

    {
        std::int32_t mission_hit = 4;
        std::int32_t mission_total = 11;
        fold_encounter_alien_statistics(3, 8, mission_hit, mission_total);
        assert(mission_hit == 7);
        assert(mission_total == 19);
    }

    // Full-session initialization seeds mission total to seven while every
    // encounter independently starts at local 7/0.
    {
        GameSession session{};
        assert(session.campaign.alien_ships_total == 7);
        assert(session.campaign.alien_ships_hit == 0);
        assert(session.encounter.encounter_alien_ships_total == 7);
        assert(session.encounter.encounter_alien_ships_hit == 0);
    }

    // Win32 0x0041610E increments both local and mission totals for a later
    // stagger activation in a transient trajectory group.
    {
        GameSession session{};
        std::array<std::vector<drone::formats::FlyRecord>, canonical_trajectory_path_family_count> storage{};
        const auto paths = make_paths(storage);
        auto& group = session.encounter.trajectories.groups[1];
        group.lifecycle.mode = TrajectoryGroupMode::RetireOnPathWrap;
        group.lifecycle.entity_count = 2;
        group.lifecycle.active_entity_count = 1;
        group.lifecycle.activated_entity_count = 1;
        group.lifecycle.spawn_delay_interval = 5;
        group.lifecycle.spawn_delay_counter = 4;
        group.actors[0].activity = TrajectoryEntityActivity::FollowingPath;
        group.actors[1].activity = TrajectoryEntityActivity::Inactive;

        GameSessionTargetContext targets{};
        targets.trajectory_paths = &paths;
        const auto tick = step_game_session(session, GameplayInputFrame{}, targets);
        assert(tick.trajectory_actors_activated == 1);
        assert(session.encounter.encounter_alien_ships_total == 8);
        assert(session.campaign.alien_ships_total == 8);
    }

    // Both weapon kill paths increment local hits, but only rapid missiles also
    // increment the mission-wide hit counter immediately.
    {
        GameSession rapid{};
        arm_single_trajectory_actor(rapid);
        const std::array rapid_hits{TrajectoryHitEvent{1, 0, 3, TrajectoryHitSource::RapidMissile}};
        GameSessionTargetContext targets{};
        targets.trajectory_hits = rapid_hits;
        const auto tick = step_game_session(rapid, GameplayInputFrame{}, targets);
        assert(tick.trajectory_actors_destroyed == 1);
        assert(rapid.encounter.encounter_alien_ships_hit == 1);
        assert(rapid.campaign.alien_ships_hit == 1);

        GameSession special{};
        arm_single_trajectory_actor(special);
        const std::array special_hits{TrajectoryHitEvent{1, 0, 3, TrajectoryHitSource::SpecialWeapon}};
        targets.trajectory_hits = special_hits;
        const auto special_tick = step_game_session(special, GameplayInputFrame{}, targets);
        assert(special_tick.trajectory_actors_destroyed == 1);
        assert(special.encounter.encounter_alien_ships_hit == 1);
        assert(special.campaign.alien_ships_hit == 0);
    }

    // The mission interstitial snapshots local statistics before adding the
    // entire encounter pair into already-live mission counters, then encounter
    // reset restores local 7/0. This intentionally preserves the original's
    // mixed double-counting behavior.
    {
        GameSession session{};
        session.campaign.alien_ships_total = 9;
        session.campaign.alien_ships_hit = 2;
        session.encounter.encounter_alien_ships_total = 10;
        session.encounter.encounter_alien_ships_hit = 4;
        session.campaign.mission.processed_count = 1;
        session.campaign.mission.outcomes[0] = DroneOutcome::Disarmed;
        session.encounter.drone.y = 231;
        session.encounter.drone_settlement_tick = 59;
        session.encounter.gameplay_substep_phase = 1;

        const auto tick = step_game_session(session, GameplayInputFrame{});
        assert(tick.drone_resolution_transition_started);
        assert(tick.encounter_alien_statistics_folded);
        assert(tick.encounter_alien_statistics.has_value());
        assert(tick.encounter_alien_statistics->hit == 4);
        assert(tick.encounter_alien_statistics->missed == 6);
        assert(tick.encounter_alien_statistics->total == 10);
        assert(tick.encounter_alien_statistics->percentage_hit == 40);
        assert(session.campaign.alien_ships_total == 19);
        assert(session.campaign.alien_ships_hit == 6);
        assert(session.encounter.encounter_alien_ships_total == 7);
        assert(session.encounter.encounter_alien_ships_hit == 0);
    }

    std::cout << "alien accounting tests passed\n";
    return 0;
}
