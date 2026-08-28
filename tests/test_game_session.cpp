#include <drone/gameplay/game_session.hpp>

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>


namespace {

drone::gameplay::TrajectoryPathCatalogView make_session_trajectory_paths(
    std::array<std::vector<drone::formats::FlyRecord>, 10>& storage) {
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

    // Default session is an active, fresh shareware-style gameplay run with
    // campaign and encounter ownership separated cleanly.
    {
        GameSession session{};
        assert(session.state == GameState::ActiveGameplay);
        assert(session.campaign.player_lifecycle.lives == canonical_starting_lives);
        assert(session.campaign.player_lifecycle.player_active);
        assert(session.encounter.player.x == canonical_respawn_x);
        assert(session.encounter.player.y == canonical_respawn_y);
        assert(session.encounter.world_scroll_row == canonical_world_scroll_initial_row);
        assert(session.encounter.drone.x == canonical_drone_session_initial_x);
        assert(session.encounter.drone.y == canonical_drone_session_initial_y);
        assert(session.encounter.drone_settlement_tick == canonical_drone_settlement_tick_cap);
        assert(session.encounter.enemy_bomb_spawn_gate.counter == -450);
        assert(session.encounter.rapid_missiles.fire_cooldown == RapidMissilePool::cooldown_ready);
        assert(!session.encounter.boss.family.has_value());
    }

    // Encounter-only reset preserves campaign progress while rebuilding all
    // per-encounter state. Full-campaign reset discards both.
    {
        GameSession session{};
        session.campaign.player_lifecycle.lives = 2;
        session.campaign.player_lifecycle.player_active = false;
        session.campaign.score = {1234, 321};
        session.campaign.mission.processed_count = 1;
        session.campaign.mission.outcomes[0] = DroneOutcome::Disarmed;
        session.campaign.high_score_disqualified = true;
        session.campaign.alien_ships_hit = 12;
        session.encounter.player.x = 42;
        session.encounter.world_scroll_row = 17;
        session.encounter.gameplay_updates = 99;
        assert(activate_shareware_boss_for_processed_drones(session.encounter.boss, 1));
        session.total_gameplay_updates = 400;

        reset_game_session(session, GameplaySessionResetScope::EncounterOnly);
        assert(session.state == GameState::ActiveGameplay);
        assert(session.campaign.player_lifecycle.lives == 2);
        assert(session.campaign.player_lifecycle.player_active);
        assert(session.campaign.score.total == 1234);
        assert(session.campaign.score.extra_life_progress == 321);
        assert(session.campaign.mission.processed_count == 1);
        assert(session.campaign.mission.outcomes[0] == DroneOutcome::Disarmed);
        assert(session.campaign.high_score_disqualified);
        assert(session.campaign.alien_ships_hit == 12);
        assert(session.encounter.player.x == canonical_respawn_x);
        assert(session.encounter.world_scroll_row == canonical_world_scroll_initial_row);
        assert(session.encounter.gameplay_updates == 0);
        assert(!session.encounter.boss.family.has_value());
        assert(session.total_gameplay_updates == 400);

        reset_game_session(session, GameplaySessionResetScope::FullCampaign);
        assert(session.campaign.player_lifecycle.lives == canonical_starting_lives);
        assert(session.campaign.score.total == 0);
        assert(session.campaign.mission.processed_count == 0);
        assert(!session.campaign.high_score_disqualified);
        assert(session.campaign.alien_ships_hit == 0);
        assert(session.total_gameplay_updates == 0);
    }

    // One continuous session tick composes the already-proven atomic helpers.
    // Initial phase 0 advances to phase 1, so slow animation/scroll is not yet
    // active on the first update.
    {
        GameSession session{};
        session.campaign.score = {600, 500};

        GameplayInputFrame input{};
        input.movement.left = true;
        input.movement.up = true;
        input.rapid_fire = true;
        input.shield = true;
        input.special_load_cycle = true;

        const auto first = step_game_session(session, input);
        assert(first.advanced);
        assert(first.encounter_update == 1 && first.total_update == 1);
        assert(first.gameplay_substep_phase == 1 && !first.animation_tick);
        assert(first.rapid_missile_fired);
        assert(first.special_loaded && !first.special_cycled && !first.special_launched);
        assert(first.shield_active && !first.shield_sound_requested);
        assert(first.extra_life_awarded);

        assert(session.encounter.player.x == 145);
        assert(session.encounter.player.y == 174);
        assert(session.encounter.rapid_missiles.active_count == 1);
        assert(session.encounter.rapid_missiles.missiles[0].x == 156);
        assert(session.encounter.rapid_missiles.missiles[0].y == 168);
        assert(session.encounter.special_weapon.activity == SpecialWeaponActivity::LoadedTracking);
        assert(session.encounter.special_weapon.x == 159);
        assert(session.encounter.special_weapon.y == 179);
        assert(session.encounter.shield.active);
        assert(session.encounter.world_scroll_row == 599);
        assert(session.encounter.enemy_bomb_spawn_gate.counter == -449);
        assert(session.campaign.player_lifecycle.lives == 4);
        assert(session.campaign.score.extra_life_progress == 0);

        // Phase 2 is the recovered slow-cadence tick: missile animation,
        // shield sound request and world scrolling all engage together.
        GameplayInputFrame second_input{};
        second_input.shield = true;
        const auto second = step_game_session(session, second_input);
        assert(second.advanced && second.animation_tick);
        assert(second.gameplay_substep_phase == 2);
        assert(!second.rapid_missile_fired);
        assert(second.shield_sound_requested);
        assert(session.encounter.world_scroll_row == 598);
        assert(session.encounter.rapid_missiles.missiles[0].frame == 1);
        assert(session.encounter.rapid_missiles.missiles[0].y == 165);
        assert(session.encounter.special_weapon.switch_progress == 1);
        assert(session.encounter.enemy_bomb_spawn_gate.counter == -448);
    }

    // Existing bomb motion can now run continuously under session ownership.
    {
        GameSession session{};
        assert(spawn_live_enemy_bomb(session.encounter.enemy_bombs, 100, 50, 2));

        GameplayInputFrame input{};
        const auto result = step_game_session(session, input);
        assert(result.advanced);
        // Player starts at x=147, so bomb target is 164 and +2 steering applies.
        assert(session.encounter.enemy_bombs.bombs[0].x == 102);
        assert(session.encounter.enemy_bombs.bombs[0].y == 52);
    }

    // Session tick is strictly an active-gameplay boundary. Modal state does
    // not accidentally advance timers, cooldowns or entity positions.
    {
        GameSession session{};
        session.state = GameState::PauseOverlay;
        const auto before_phase = session.encounter.gameplay_substep_phase;
        const auto before_counter = session.encounter.enemy_bomb_spawn_gate.counter;
        const auto result = step_game_session(session, GameplayInputFrame{});
        assert(!result.advanced);
        assert(session.encounter.gameplay_substep_phase == before_phase);
        assert(session.encounter.enemy_bomb_spawn_gate.counter == before_counter);
        assert(session.total_gameplay_updates == 0);
    }

    // Stinger target geometry and attached-Probe bomb redirection remain
    // explicit encounter inputs until actor collections are integrated.
    {
        GameSession session{};
        session.encounter.special_weapon.activity = SpecialWeaponActivity::LoadedTracking;
        session.encounter.special_weapon.kind = SpecialWeaponKind::Stinger;
        session.encounter.special_weapon.switch_progress = 12;

        GameSessionTargetContext targets{};
        targets.stinger_target = SpecialTargetGeometry{.x = 200, .width = 20};
        const auto result = step_game_session(session, GameplayInputFrame{}, targets);
        assert(result.advanced);
        // Loaded state anchors at player.x+14=161 then steps toward target 210.
        assert(session.encounter.special_weapon.x == 162);

        session.encounter.special_weapon.kind = SpecialWeaponKind::Probe;
        session.encounter.special_weapon.activity = SpecialWeaponActivity::ProbeAttachedDecoding;
        session.encounter.special_weapon.x = 0;
        assert(spawn_live_enemy_bomb(session.encounter.enemy_bombs, 100, 50, 2));
        session.encounter.drone.x = 80;
        targets.redirect_bombs_to_attached_probe = true;
        (void)step_game_session(session, GameplayInputFrame{}, targets);
        assert(session.encounter.special_weapon.x == 85); // Drone.x + 5
        assert(session.encounter.enemy_bombs.bombs[0].x == 98); // target = probe.x+1=86
    }


    // Phase-4 whole-session integration owns the trajectory collection and can
    // execute the established formation -> path update -> proven-hit ->
    // destruction/score path in one logical tick. Group 1 is the canonical
    // weak LeftDive template (threshold 1, burst 1, score 1).
    {
        std::array<std::vector<drone::formats::FlyRecord>, 10> storage{};
        const auto paths = make_session_trajectory_paths(storage);

        GameSession session{};
        const TrajectoryHitEvent hit{1, 0, 3};
        const std::array<TrajectoryHitEvent, 1> hits{hit};

        GameSessionTargetContext targets{};
        targets.trajectory_paths = &paths;
        targets.trajectory_spawn_group = 1;
        targets.trajectory_spawn_x_offset = 7;
        targets.trajectory_spawn_y_offset = -3;
        targets.trajectory_hits = hits;

        const auto result = step_game_session(session, GameplayInputFrame{}, targets);
        assert(result.advanced);
        assert(result.trajectory_group_spawned);
        assert(result.trajectory_actors_destroyed == 1);
        assert(result.trajectory_groups_retired == 1);
        assert(result.trajectory_destruction_bursts == 1);
        assert(result.trajectory_score_delta == 1);
        assert(session.campaign.score.total == 1);
        assert(session.campaign.score.extra_life_progress == 1);
        assert(session.encounter.trajectories.groups[1].lifecycle.mode ==
               TrajectoryGroupMode::Inactive);
        assert(session.encounter.trajectories.groups[1].actors[0].activity ==
               TrajectoryEntityActivity::Inactive);
        assert(session.encounter.trajectories.active_group_count == 1);
    }

    // Drone travel is now continuously owned. Reaching -200 on the recovered
    // phase-2 cadence directly dispatches the shareware boss selected by the
    // number of already processed Drone outcomes.
    {
        GameSession session{};
        session.encounter.drone.y = -201;
        session.encounter.gameplay_substep_phase = 1;
        GameSessionTargetContext targets{};

        auto result = step_game_session(session, GameplayInputFrame{}, targets);
        assert(result.advanced && result.drone_moved);
        assert(session.encounter.drone.y == canonical_drone_boss_approach_y);
        assert(result.boss_activated);
        assert(result.boss_activated_family == BossFamily::LidTop);
        assert(session.encounter.boss.family == BossFamily::LidTop);
        assert(session.encounter.boss.lid_top.top_activity == boss_activity_active);
        assert(session.encounter.boss.lid_top.lid_activity == lid_top_initial_lid_activity);

        // Remaining at/after the boundary cannot reinitialize an owned boss.
        result = step_game_session(session, GameplayInputFrame{}, targets);
        assert(!result.boss_activated);

        const std::array lid_hit{SharewareBossDestructionTrigger::LidTopLid};
        targets.boss_destruction_triggers = lid_hit;
        result = step_game_session(session, GameplayInputFrame{}, targets);
        assert(result.boss_destruction_transitions == 1);
        assert(session.encounter.boss.lid_top.lid_destruction_progress == 1);

        targets.boss_destruction_triggers = {};
        for (int i = 0; i < 24; ++i) {
            result = step_game_session(session, GameplayInputFrame{}, targets);
        }
        assert(result.boss_score_delta == 100);
        assert(result.lid_top_motion_stop_requested);
        assert(session.campaign.score.total == 100);
        assert(session.campaign.score.extra_life_progress == 100);
        assert(session.encounter.boss.lid_top.lid_activity == boss_activity_inactive);
        assert(session.encounter.boss.lid_top.top_activity == boss_activity_destruction);
    }

    // Processed Drone count 1 selects Gemini. Its halves remain independent at
    // the session boundary and each exact destruction transition contributes
    // the original +100 award.
    {
        GameSession session{};
        session.campaign.mission.processed_count = 1;
        session.campaign.mission.outcomes[0] = DroneOutcome::Disarmed;
        session.encounter.drone.y = -201;
        session.encounter.gameplay_substep_phase = 1;

        GameSessionTargetContext targets{};
        auto result = step_game_session(session, GameplayInputFrame{}, targets);
        assert(result.boss_activated);
        assert(result.boss_activated_family == BossFamily::Gemini);

        const std::array both{
            SharewareBossDestructionTrigger::GeminiSideA,
            SharewareBossDestructionTrigger::GeminiSideB,
        };
        targets.boss_destruction_triggers = both;
        result = step_game_session(session, GameplayInputFrame{}, targets);
        assert(result.boss_destruction_transitions == 2);
        assert(result.boss_score_delta == 200);
        assert(session.campaign.score.total == 200);
        assert(session.campaign.score.extra_life_progress == 200);
        assert(session.encounter.boss.gemini.side_a.body_activity == boss_activity_destruction);
        assert(session.encounter.boss.gemini.side_b.body_activity == boss_activity_destruction);

        // The canonical shareware stop never initializes dispatch slot 2.
        reset_game_session(session, GameplaySessionResetScope::EncounterOnly);
        session.campaign.mission.processed_count = 2;
        session.encounter.drone.y = -201;
        session.encounter.gameplay_substep_phase = 1;
        targets.boss_destruction_triggers = {};
        result = step_game_session(session, GameplayInputFrame{}, targets);
        assert(!result.boss_activated);
        assert(!session.encounter.boss.family.has_value());
    }

    // The normal disarm route is now a continuous GameSession transition: a
    // completed Probe release commits at Y=201, resets settlement at Y=230,
    // waits at Y=231 for tick 60, then performs the encounter-only transition.
    {
        GameSession session{};
        GameSessionTargetContext targets{};
        targets.drone_disarm_completed = true;
        session.encounter.drone.y = 200;
        session.encounter.gameplay_substep_phase = 1;

        auto result = step_game_session(session, GameplayInputFrame{}, targets);
        assert(result.drone_disarm_completion_accepted);
        assert(result.drone_disarm_committed);
        assert(session.campaign.mission.processed_count == 1);
        assert(session.campaign.mission.outcomes[0] == DroneOutcome::Disarmed);
        assert(session.encounter.drone.y == canonical_drone_post_disarm_y);

        targets.drone_disarm_completed = false;
        session.encounter.drone.y = 229;
        session.encounter.drone.disarm_completed = true;
        session.encounter.gameplay_substep_phase = 1;
        result = step_game_session(session, GameplayInputFrame{}, targets);
        assert(result.drone_settlement_tick_reset);
        assert(session.encounter.drone.y == 230);
        assert(session.encounter.drone_settlement_tick == 0);

        // Reconstruct the exact settled state just before the final phase-2
        // increment: Y is frozen at 231 after decode status is cleared.
        session.encounter.drone.y = 231;
        session.encounter.drone.disarm_completed = false;
        session.encounter.drone_settlement_tick = 59;
        session.encounter.gameplay_substep_phase = 1;
        result = step_game_session(session, GameplayInputFrame{}, targets);
        assert(result.drone_resolution_transition_started);
        assert(result.mission_interstitial);
        assert(result.mission_interstitial->briefing == MissionBriefingCard::Mission1);
        assert(result.encounter_transition);
        assert(result.encounter_transition->target == EncounterTransitionTarget::Gemini);
        assert(result.encounter_transition->disposition ==
               EncounterTransitionDisposition::ContinueCampaign);
        assert(session.campaign.player_lifecycle.lives == canonical_starting_lives);
        assert(session.campaign.player_lifecycle.player_active);
        assert(session.encounter.drone.y == drone_reentry_y_for_processed_count(1));
        assert(session.encounter.drone.y == -1200);
        assert(session.encounter.drone_settlement_tick == canonical_drone_settlement_tick_cap);
        assert(!session.encounter.boss.family.has_value());
    }

    // Objective 2 is the compiled shareware termination branch. It zeroes lives
    // before rebuilding the encounter and exposes Results/EndRun to the host.
    {
        GameSession session{};
        session.campaign.mission.processed_count = 2;
        session.campaign.mission.outcomes[0] = DroneOutcome::Disarmed;
        session.campaign.mission.outcomes[1] = DroneOutcome::Disarmed;
        session.encounter.drone.y = 231;
        session.encounter.drone_settlement_tick = 59;
        session.encounter.gameplay_substep_phase = 1;

        const auto result = step_game_session(session, GameplayInputFrame{});
        assert(result.drone_resolution_transition_started);
        assert(result.encounter_transition);
        assert(result.encounter_transition->target == EncounterTransitionTarget::Results);
        assert(result.encounter_transition->disposition ==
               EncounterTransitionDisposition::EndRun);
        assert(session.campaign.player_lifecycle.lives == 0);
        assert(!session.campaign.player_lifecycle.player_active);
        assert(session.encounter.drone.y == drone_reentry_y_for_processed_count(2));
        assert(session.encounter.drone.y == -1350);
    }

    std::cout << "Drone continuous game-session tests passed\n";
    return 0;
}
