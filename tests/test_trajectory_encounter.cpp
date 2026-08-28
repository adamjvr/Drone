#include <drone/gameplay/trajectory_encounter.hpp>

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

namespace {

drone::gameplay::TrajectoryPathCatalogView make_paths(
    std::array<std::vector<drone::formats::FlyRecord>, drone::gameplay::canonical_trajectory_path_family_count>& storage) {
    for (std::size_t family = 0; family < storage.size(); ++family) {
        auto& path = storage[family];
        path.resize(1000);
        for (std::size_t i = 0; i < path.size(); ++i) {
            path[i].x = static_cast<std::int16_t>((i + family * 3) % 300);
            path[i].y = static_cast<std::int16_t>(20 + ((i + family * 5) % 150));
            path[i].aux = (i % 4 == 0) ? 1 : 0;
        }
    }

    drone::gameplay::TrajectoryPathCatalogView view{};
    for (std::size_t i = 0; i < storage.size(); ++i) view.families[i] = storage[i];
    return view;
}

} // namespace

int main() {
    using namespace drone::gameplay;

    std::array<std::vector<drone::formats::FlyRecord>, drone::gameplay::canonical_trajectory_path_family_count> storage{};
    const auto paths = make_paths(storage);

    // Startup reconstruction owns all 17 fixed records. The primary Loop group
    // is live with seven following actors; non-primary templates remain idle.
    {
        TrajectoryEncounterState encounter{};
        reset_trajectory_encounter(encounter, &paths);
        assert(encounter.active_group_count == 1);
        assert(encounter.groups[0].lifecycle.mode == TrajectoryGroupMode::PersistentLoop);
        assert(encounter.groups[0].lifecycle.active_entity_count == 7);
        assert(encounter.groups[0].lifecycle.activated_entity_count == 7);
        assert(encounter.groups[0].actors[0].activity == TrajectoryEntityActivity::FollowingPath);
        assert(encounter.groups[1].lifecycle.mode == TrajectoryGroupMode::Inactive);
        assert(encounter.groups[1].actors[0].activity == TrajectoryEntityActivity::Inactive);
        assert(encounter.groups[0].actors[1].path_index == 53);
        assert(encounter.groups[0].actors[1].x == storage[0][53].x);
    }

    // Busy slots are skipped cyclically and group zero is never selected as a
    // transient-wave destination.
    {
        TrajectoryEncounterState encounter{};
        reset_trajectory_encounter(encounter, &paths);
        encounter.groups[1].lifecycle.mode = TrajectoryGroupMode::RetireOnPathWrap;
        encounter.groups[2].lifecycle.mode = TrajectoryGroupMode::RetireOnPathWrap;
        assert(select_inactive_trajectory_group(encounter, 1) == 3);
        assert(select_inactive_trajectory_group(encounter, 16) == 16);
    }

    // Live activation uses the recovered mode-2 contract and stagger branch.
    {
        TrajectoryEncounterState encounter{};
        reset_trajectory_encounter(encounter, &paths);
        assert(activate_transient_trajectory_group(encounter, 1, paths, 7, -3));
        auto& group = encounter.groups[1];
        assert(group.lifecycle.mode == TrajectoryGroupMode::RetireOnPathWrap);
        assert(group.lifecycle.active_entity_count == 1);
        assert(group.lifecycle.activated_entity_count == 1);
        assert(encounter.active_group_count == 2);
        assert(group.actors[0].activity == TrajectoryEntityActivity::FollowingPath);
        assert(group.actors[0].path_step == 1);
        assert(group.actors[0].x == storage[1][0].x + 7);
        assert(group.actors[0].y == storage[1][0].y - 3);

        ScoreState score{};
        for (int i = 0; i < 17; ++i) {
            const auto result = advance_trajectory_encounter(encounter, paths, 1, score);
            assert(result.actors_activated == 0);
        }
        const auto eighteenth = advance_trajectory_encounter(encounter, paths, 1, score);
        assert(eighteenth.actors_activated == 1);
        assert(group.lifecycle.active_entity_count == 2);
        assert(group.lifecycle.activated_entity_count == 2);
        assert(group.actors[1].activity == TrajectoryEntityActivity::FollowingPath);
    }

    // Mode-2 completion is an escape: actor retires, its score value is
    // subtracted from total/progress, and the final active actor tears down the
    // whole group.
    {
        TrajectoryEncounterState encounter{};
        reset_trajectory_encounter(encounter, &paths);
        assert(activate_transient_trajectory_group(encounter, 4, paths));
        auto& actor = encounter.groups[4].actors[0];
        actor.path_end_index = 1;
        ScoreState score{100, 100};
        (void)advance_trajectory_encounter(encounter, paths, 1, score); // index 1
        const auto escaped = advance_trajectory_encounter(encounter, paths, 1, score); // wrap
        assert(escaped.actors_escaped == 1);
        assert(escaped.groups_retired == 1);
        assert(escaped.escape_score_delta == -25);
        assert(score.total == 75 && score.extra_life_progress == 75);
        assert(encounter.groups[4].lifecycle.mode == TrajectoryGroupMode::Inactive);
        assert(encounter.active_group_count == 1); // primary group remains live
    }

    // Collision dispatch uses the recovered byte damage threshold and combat
    // metadata. Group 4 takes nine +3 hits to cross threshold 25, then awards
    // 25 points and hands five destruction bursts to the effect producer.
    {
        TrajectoryEncounterState encounter{};
        reset_trajectory_encounter(encounter, &paths);
        assert(activate_transient_trajectory_group(encounter, 4, paths));
        ScoreState score{};
        for (int i = 0; i < 8; ++i) {
            const auto hit = apply_trajectory_hit(encounter, {4, 0, 3}, score);
            assert(hit.accepted && !hit.destroyed);
        }
        const auto final_hit = apply_trajectory_hit(encounter, {4, 0, 3}, score);
        assert(final_hit.destroyed);
        assert(final_hit.group_retired);
        assert(final_hit.destruction_burst_count == 5);
        assert(final_hit.score_delta == 25);
        assert(score.total == 25 && score.extra_life_progress == 25);
        assert(encounter.active_group_count == 1);
    }

    std::cout << "Drone trajectory encounter integration tests passed\n";
    return 0;
}
