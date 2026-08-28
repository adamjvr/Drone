#include <drone/gameplay/game_session.hpp>

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>


namespace {

drone::gameplay::TrajectoryPathCatalogView make_session_trajectory_paths(
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
        assert(session.encounter.trajectory_spawn.interval_threshold == 280);
        assert(session.encounter.trajectory_spawn.interval_counter == 250);
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
        assert(session.encounter.trajectory_spawn.interval_threshold == 260);
        assert(session.encounter.trajectory_spawn.interval_counter == 230);
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

    // Stinger target *selection* is now session-owned. Actor geometry remains
    // an encounter fact until boss movement is integrated; Bomber is selected
    // here without the host supplying a preselected target.
    {
        GameSession session{};
        session.encounter.special_weapon.activity = SpecialWeaponActivity::LoadedTracking;
        session.encounter.special_weapon.kind = SpecialWeaponKind::Stinger;
        session.encounter.special_weapon.switch_progress = 12;

        GameSessionTargetContext targets{};
        targets.stinger_targets.bomber_active = true;
        targets.stinger_targets.bomber = {.x = 200, .width = 20};
        const auto result = step_game_session(session, GameplayInputFrame{}, targets);
        assert(result.advanced);
        assert(result.stinger_target_identity == StingerTargetIdentity::Bomber);
        assert(result.stinger_target_desired_x == 210);
        assert(result.stinger_target_changed);
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


    // Shareware Gemini activity is read from the pre-boss-update session state;
    // only head geometry is supplied. Both sides active use nearest-head X and
    // exact ties select B.
    {
        GameSession session{};
        session.encounter.boss.family = BossFamily::Gemini;
        session.encounter.boss.gemini.side_a.body_activity = boss_activity_active;
        session.encounter.boss.gemini.side_b.body_activity = boss_activity_active;
        session.encounter.special_weapon.kind = SpecialWeaponKind::Stinger;
        session.encounter.special_weapon.activity = SpecialWeaponActivity::LoadedTracking;

        GameSessionTargetContext targets{};
        targets.stinger_targets.gemini_head_a = {.x = 100, .width = 43};
        targets.stinger_targets.gemini_head_b = {.x = 180, .width = 43};

        const auto result = step_game_session(session, GameplayInputFrame{}, targets);
        assert(result.stinger_target_identity == StingerTargetIdentity::GeminiHeadB);
        assert(result.stinger_target_desired_x == 201);
        assert(session.encounter.special_weapon.x == 162);
    }

    // Successful special loading resets the shared Stinger target to the
    // original x=160 dummy before same-update target selection. With no hostile
    // candidate, the loaded Stinger therefore steps from player.x+14 to 160.
    {
        GameSession session{};
        session.encounter.special_weapon.kind = SpecialWeaponKind::Stinger;
        session.encounter.stinger_target.identity = StingerTargetIdentity::Bomber;
        session.encounter.stinger_target.geometry = {.x = 250, .width = 40};
        GameplayInputFrame input{};
        input.special_load_cycle = true;
        const auto result = step_game_session(session, input);
        assert(result.special_loaded);
        assert(result.stinger_target_identity == StingerTargetIdentity::DummyCenter);
        assert(result.stinger_target_desired_x == 160);
        assert(session.encounter.special_weapon.x == 160);
    }

    // Phase-4 whole-session integration now owns live transient formation
    // timing/template selection as well as the mutable trajectory collection.
    // Force the next phase-2 interval crossing and prove a native spawn; then
    // feed the still-external sprite-mask hit producer on a later update. The
    // separate original encounter/campaign alien-accounting scalars remain a
    // later ownership boundary and are not aliased here.
    {
        std::array<std::vector<drone::formats::FlyRecord>, drone::gameplay::canonical_trajectory_path_family_count> storage{};
        const auto paths = make_session_trajectory_paths(storage);

        GameSession session{};
        seed_original_random(session.original_random, 11);
        session.encounter.gameplay_substep_phase = 1;
        session.encounter.trajectory_spawn.interval_counter =
            static_cast<std::int16_t>(session.encounter.trajectory_spawn.interval_threshold - 3);

        GameSessionTargetContext targets{};
        targets.trajectory_paths = &paths;

        const auto spawned = step_game_session(session, GameplayInputFrame{}, targets);
        assert(spawned.advanced);
        assert(spawned.trajectory_group_spawned);
        assert(spawned.trajectory_spawn_forced);
        assert(spawned.trajectory_spawned_group.has_value());

        const auto group_index = *spawned.trajectory_spawned_group;
        const TrajectoryHitEvent hit{group_index, 0, 255};
        const std::array<TrajectoryHitEvent, 1> hits{hit};
        targets.trajectory_hits = hits;

        const auto destroyed = step_game_session(session, GameplayInputFrame{}, targets);
        assert(destroyed.trajectory_actors_destroyed == 1);
        assert(destroyed.trajectory_destruction_bursts > 0);
        assert(destroyed.trajectory_score_delta > 0);
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
        // This block begins from an already completed decoder so it continues
        // to isolate the downstream Y=201/230/231 settlement path. Probe
        // attachment/decode itself is covered by the integration block below.
        session.encounter.drone.disarm_completed = true;
        session.encounter.special_weapon.activity =
            SpecialWeaponActivity::ProbeAttachedDecoding;
        session.encounter.special_weapon.probe_decode.status = ProbeDecodeStatus::Complete;
        session.encounter.drone.y = 200;
        session.encounter.gameplay_substep_phase = 1;

        auto result = step_game_session(session, GameplayInputFrame{}, targets);
        assert(result.drone_disarm_committed);
        assert(session.campaign.mission.processed_count == 1);
        assert(session.campaign.mission.outcomes[0] == DroneOutcome::Disarmed);
        assert(session.encounter.drone.y == canonical_drone_post_disarm_y);

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

    // Probe attachment, decode and release are now continuous session behavior.
    // Demo mode makes the two attachment thresholds deterministic (210/150);
    // the completion-only random draw remains live because Win32 consumes it
    // for an effect parameter even during replay.
    {
        GameSession session{};
        session.runtime.demo_playback_mode = true;
        session.encounter.drone.x = 100;
        session.encounter.drone.y = 45;
        session.encounter.gameplay_substep_phase = 0;
        session.encounter.special_weapon.kind = SpecialWeaponKind::Probe;
        session.encounter.special_weapon.activity = SpecialWeaponActivity::LaunchedHoming;
        session.encounter.special_weapon.x = 104;
        // Common homing runs before collision and moves Y upward by two.
        session.encounter.special_weapon.y = 47;

        auto result = step_game_session(session, GameplayInputFrame{});
        assert(result.special_weapon_hit_drone);
        assert(result.probe_attached_to_drone);
        assert(result.probe_attachment_score_delta == probe_attachment_score_award);
        assert(session.campaign.score.total == 10);
        assert(session.campaign.score.extra_life_progress == 10);
        assert(session.encounter.special_weapon.activity ==
               SpecialWeaponActivity::ProbeAttachedDecoding);
        assert(session.encounter.special_weapon.probe_decode.phase1_threshold == 210);
        assert(session.encounter.special_weapon.probe_decode.phase2_threshold == 150);
        assert(session.original_random.draws == 0);

        session.encounter.special_weapon.probe_decode.phase1_elapsed = 209;
        result = step_game_session(session, GameplayInputFrame{});
        assert(result.probe_decode_phase1_completed);
        assert(!result.probe_decode_completed);
        assert(session.encounter.special_weapon.probe_decode.status ==
               ProbeDecodeStatus::Phase2Disarming);
        assert(session.encounter.special_weapon.probe_decode.phase2_elapsed == 1);

        session.encounter.special_weapon.probe_decode.phase2_elapsed = 149;
        session.encounter.gameplay_substep_phase = 1;
        result = step_game_session(session, GameplayInputFrame{});
        assert(result.probe_decode_completed);
        assert(result.probe_decode_score_delta == drone_disarm_score_award);
        assert(result.probe_decode_completion_effect_random != 0);
        assert(session.encounter.special_weapon.probe_decode.status ==
               ProbeDecodeStatus::Complete);
        // Decode completes before normal Drone phase-2 movement, so Y=45 is
        // released to 46 on this same update.
        assert(session.encounter.drone.disarm_completed);
        assert(session.encounter.drone.y == 46);
        assert(session.campaign.score.total == 510);
        // +500 crosses exactly one extra-life threshold later in this update.
        assert(result.extra_life_awarded);
        assert(session.campaign.player_lifecycle.lives == canonical_starting_lives + 1);
        assert(session.campaign.score.extra_life_progress == 10);
        assert(session.original_random.draws == 1);
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

    // The unresolved Y=45 timeout now enters the internally owned destructive
    // countdown. Because that countdown lives before the four-phase scheduler,
    // a late timeout starts it at zero and the first increment occurs only on
    // the following logical GameSession update.
    {
        GameSession session{};
        session.encounter.drone.y = canonical_drone_hover_y;
        session.encounter.drone.hover_phase2_ticks =
            canonical_drone_hover_timeout_phase2_ticks - 1;
        session.encounter.gameplay_substep_phase = 1;

        auto result = step_game_session(session, GameplayInputFrame{});
        assert(result.drone_hover_timeout_reached);
        assert(result.drone_destruction_countdown_started);
        assert(!result.drone_detonation_started);
        assert(session.encounter.drone.destruction_countdown == 0);
        assert(session.campaign.mission.processed_count == 0);

        session.campaign.score = {1750, 620};
        session.encounter.drone.destruction_countdown =
            canonical_drone_destruction_countdown_trigger - 1;
        result = step_game_session(session, GameplayInputFrame{});
        assert(result.drone_destruction_countdown_advanced);
        assert(result.drone_detonation_started);
        assert(result.drone_detonation_outcome_committed);
        assert(result.drone_detonation_score_delta == -1000);
        assert(session.encounter.drone.activity == canonical_drone_destruction_activity);
        // 0x0041D220 resets this to zero before the scheduler; 0x0040C05A
        // advances it to one later in the same state-2 update.
        assert(session.encounter.drone.detonation_tick == 1);
        assert(session.campaign.score.total == 750);
        assert(session.campaign.score.extra_life_progress == 0);
        assert(session.campaign.mission.processed_count == 1);
        assert(session.campaign.mission.outcomes[0] == DroneOutcome::Detonated);
    }

    // The post-trajectory detonation updater owns only logical timing/events.
    // Phase 0 at tick 329 resets the destruction-settlement field; phase 0 at
    // capped tick 330 begins incrementing it.
    {
        GameSession session{};
        session.encounter.drone.activity = canonical_drone_destruction_activity;
        session.encounter.drone.detonation_tick =
            canonical_drone_detonation_tick_settlement_reset - 1;
        session.encounter.drone.detonation_center_y = 80;
        session.encounter.drone.destruction_settlement_phase0_ticks = 17;
        session.encounter.gameplay_substep_phase = 3;

        auto result = step_game_session(session, GameplayInputFrame{});
        assert(result.drone_detonation_effect_tick);
        assert(result.drone_detonation_explosion_spawns_requested == 4);
        assert(result.drone_detonation_settlement_reset);
        assert(!result.drone_detonation_settlement_advanced);
        assert(session.encounter.drone.detonation_tick == 329);
        assert(session.encounter.drone.destruction_settlement_phase0_ticks == 0);
        assert(session.encounter.drone.detonation_center_y == 81);

        session.encounter.gameplay_substep_phase = 3;
        result = step_game_session(session, GameplayInputFrame{});
        assert(result.drone_detonation_effect_tick);
        assert(!result.drone_detonation_settlement_reset);
        assert(result.drone_detonation_settlement_advanced);
        assert(session.encounter.drone.detonation_tick == 330);
        assert(session.encounter.drone.destruction_settlement_phase0_ticks == 1);
    }

    // Once the phase-0 destruction-settlement field is >70, the first Drone
    // failure consumes one life. With more than one life the canonical mission
    // interstitial and encounter-only reset run first, then the caller decrements
    // lives and positions the next Drone from processed_count.
    {
        GameSession session{};
        session.campaign.player_lifecycle.lives = 3;
        session.campaign.mission.processed_count = 1;
        session.campaign.mission.outcomes[0] = DroneOutcome::Detonated;
        session.encounter.drone.activity = canonical_drone_destruction_activity;
        session.encounter.drone.destruction_settlement_phase0_ticks = 71;

        const auto result = step_game_session(session, GameplayInputFrame{});
        assert(result.drone_destruction_settled);
        assert(result.drone_life_lost);
        assert(!result.drone_game_over_pending);
        assert(result.drone_destruction_transition_started);
        assert(result.mission_interstitial);
        assert(result.mission_interstitial->tone == MissionInterstitialTone::Bad);
        assert(result.mission_interstitial->sound == MissionInterstitialSound::Detonate);
        assert(result.encounter_transition);
        assert(result.encounter_transition->target == EncounterTransitionTarget::Gemini);
        assert(session.campaign.player_lifecycle.lives == 2);
        assert(session.campaign.player_lifecycle.player_active);
        assert(session.campaign.mission.processed_count == 1);
        assert(session.campaign.mission.outcomes[0] == DroneOutcome::Detonated);
        assert(session.encounter.drone.y == drone_reentry_y_for_processed_count(1));
        assert(session.encounter.drone.y == -1200);
        assert(session.encounter.drone.activity == canonical_drone_active_activity);
        assert(session.encounter.drone_settlement_tick == canonical_drone_settlement_tick_cap);
    }

    // A last-life Drone destruction does not run the mission interstitial. The
    // settlement caller still decrements the life and positions the reentry Y,
    // but leaves the destruction actor inactive-for-play so the next dispatch
    // can enter the already-recovered post-game path.
    {
        GameSession session{};
        session.campaign.player_lifecycle.lives = 1;
        session.campaign.mission.processed_count = 1;
        session.campaign.mission.outcomes[0] = DroneOutcome::Detonated;
        session.encounter.drone.activity = canonical_drone_destruction_activity;
        session.encounter.drone.destruction_settlement_phase0_ticks = 71;

        const auto result = step_game_session(session, GameplayInputFrame{});
        assert(result.drone_destruction_settled);
        assert(result.drone_life_lost);
        assert(result.drone_game_over_pending);
        assert(!result.drone_destruction_transition_started);
        assert(!result.mission_interstitial.has_value());
        assert(session.campaign.player_lifecycle.lives == 0);
        assert(!session.campaign.player_lifecycle.player_active);
        assert(session.encounter.drone.y == drone_reentry_y_for_processed_count(1));
        assert(session.encounter.drone.activity == canonical_drone_destruction_activity);
    }

    // The shareware count-2 mission branch zeroes lives inside the interstitial
    // transition, after which the destruction-settlement caller still performs
    // its unconditional decrement. Preserve that signed -1 quirk exactly.
    {
        GameSession session{};
        session.campaign.player_lifecycle.lives = 2;
        session.campaign.mission.processed_count = 2;
        session.campaign.mission.outcomes[0] = DroneOutcome::Disarmed;
        session.campaign.mission.outcomes[1] = DroneOutcome::Detonated;
        session.encounter.drone.activity = canonical_drone_destruction_activity;
        session.encounter.drone.destruction_settlement_phase0_ticks = 71;

        const auto result = step_game_session(session, GameplayInputFrame{});
        assert(result.drone_destruction_settled);
        assert(result.drone_life_lost);
        assert(result.drone_game_over_pending);
        assert(result.drone_destruction_transition_started);
        assert(result.encounter_transition);
        assert(result.encounter_transition->target == EncounterTransitionTarget::Results);
        assert(result.encounter_transition->disposition ==
               EncounterTransitionDisposition::EndRun);
        assert(session.campaign.player_lifecycle.lives == -1);
        assert(!session.campaign.player_lifecycle.player_active);
        assert(session.encounter.drone.y == drone_reentry_y_for_processed_count(2));
        assert(session.encounter.drone.y == -1350);
    }


    // Enemy-bomb collision with an attached phase-2 Probe is now owned by the
    // continuous session. The bomb moves first, then the late collision pass
    // consumes both objects and applies the exact decoder interruption reset.
    {
        GameSession session{};
        session.encounter.special_weapon.activity =
            SpecialWeaponActivity::ProbeAttachedDecoding;
        session.encounter.special_weapon.kind = SpecialWeaponKind::Probe;
        session.encounter.special_weapon.x = session.encounter.drone.x + 5;
        session.encounter.special_weapon.y = 100;
        session.encounter.special_weapon.probe_decode.status =
            ProbeDecodeStatus::Phase2Disarming;
        session.encounter.special_weapon.probe_decode.phase1_elapsed = 480;
        session.encounter.special_weapon.probe_decode.phase1_threshold = 500;
        session.encounter.special_weapon.probe_decode.phase2_elapsed = 10;
        session.encounter.special_weapon.probe_decode.phase2_threshold = 350;

        // step_enemy_bombs adds two to Y first: 89 -> 91, then 0x00402000
        // tests y+9 == 100 against the attached Probe's inclusive hitbox.
        assert(spawn_live_enemy_bomb(
            session.encounter.enemy_bombs,
            session.encounter.drone.x + 5,
            89,
            0));

        const auto result = step_game_session(session, GameplayInputFrame{});
        assert(result.enemy_bomb_hit_special_weapon);
        assert(result.enemy_bomb_special_hit_index == 0);
        assert(result.enemy_bomb_probe_decode_reset);
        assert(result.enemy_bomb_probe_phase2_interrupt_signal_requested);
        assert(result.enemy_bomb_probe_impact_effect_requested);
        assert(result.enemy_bomb_probe_impact_sound_requested);
        assert(!result.enemy_bomb_stinger_impact_effect_requested);
        assert(session.encounter.enemy_bombs.active_count == 0);
        assert(!session.encounter.enemy_bombs.bombs[0].active);
        assert(session.encounter.special_weapon.activity ==
               SpecialWeaponActivity::Inactive);
        assert(session.encounter.special_weapon.probe_decode.status ==
               ProbeDecodeStatus::Phase1Decoding);
        assert(session.encounter.special_weapon.probe_decode.phase1_elapsed == 0);
        assert(session.encounter.special_weapon.probe_decode.phase2_elapsed == 0);
    }


    // Shielded bomb/player collision is owned by the continuous session. Bomb Y
    // advances by two before the late pass, so 164 -> 166 and the collision
    // helper tests y+9 == the canonical player Y 175.
    {
        GameSession session{};
        assert(spawn_live_enemy_bomb(
            session.encounter.enemy_bombs,
            session.encounter.player.x,
            164,
            0));

        GameplayInputFrame input{};
        input.shield = true;
        const auto result = step_game_session(session, input);
        assert(result.enemy_bomb_player_hits == 1);
        assert(result.enemy_bomb_shield_absorptions == 1);
        assert(result.enemy_bomb_first_player_hit_index == 0);
        assert(!result.player_destruction_started);
        assert(session.campaign.player_lifecycle.player_active);
        assert(session.campaign.player_lifecycle.lives == 3);
        assert(session.encounter.enemy_bombs.active_count == 0);
    }

    // An unshielded player hit auto-launches a merely loaded special before
    // entering player destruction and drives the shared bomb spawn/respawn gate
    // to the canonical -540 value in the same update.
    {
        GameSession session{};
        session.encounter.special_weapon.activity =
            SpecialWeaponActivity::LoadedTracking;
        session.encounter.special_weapon.kind = SpecialWeaponKind::Probe;
        assert(spawn_live_enemy_bomb(
            session.encounter.enemy_bombs,
            session.encounter.player.x,
            164,
            0));

        const auto result = step_game_session(session, GameplayInputFrame{});
        assert(result.enemy_bomb_player_hits == 1);
        assert(result.enemy_bomb_auto_launched_special);
        assert(result.enemy_bomb_auto_launch_sound_requested);
        assert(result.enemy_bomb_player_hit_sfx_requested);
        assert(result.player_destruction_started);
        assert(result.player_death_effect_requested);
        assert(result.player_bomb_spawn_suppression_started);
        assert(result.special_launched);
        assert(!session.campaign.player_lifecycle.player_active);
        assert(session.campaign.player_lifecycle.lives == 3);
        assert(session.encounter.special_weapon.activity ==
               SpecialWeaponActivity::LaunchedHoming);
        assert(session.encounter.enemy_bomb_spawn_gate.counter == -540);
    }

    // Life consumption is deferred until the shared bomb gate has advanced
    // above -356 and the fidelity host reports the player death effect inactive.
    // The life is consumed before the positive-life test, then shield/frame/
    // position are reset and the surviving player becomes active again.
    {
        GameSession session{};
        session.campaign.player_lifecycle.lives = 3;
        session.campaign.player_lifecycle.player_active = false;
        session.encounter.player.x = 12;
        session.encounter.player.y = 34;
        session.encounter.player.frame = 7;
        session.encounter.shield.energy = 1234;
        session.encounter.enemy_bomb_spawn_gate.counter = -356;

        GameSessionTargetContext targets{};
        targets.player_death_effect_inactive = true;
        const auto result = step_game_session(
            session, GameplayInputFrame{}, targets);
        assert(result.player_life_consumed);
        assert(result.player_respawned);
        assert(result.player_respawn_shield_reset);
        assert(!result.player_game_over_banner_requested);
        assert(session.campaign.player_lifecycle.lives == 2);
        assert(session.campaign.player_lifecycle.player_active);
        assert(session.encounter.player.frame == 0);
        assert(session.encounter.player.x == canonical_respawn_x);
        assert(session.encounter.player.y == canonical_respawn_y);
        assert(session.encounter.shield.energy == shield_nominal_max_energy);
        assert(session.encounter.enemy_bomb_spawn_gate.counter == -355);
    }

    // The last life follows the same reset ordering but remains inactive and
    // requests the already-recovered game-over banner path.
    {
        GameSession session{};
        session.campaign.player_lifecycle.lives = 1;
        session.campaign.player_lifecycle.player_active = false;
        session.encounter.enemy_bomb_spawn_gate.counter = -356;
        session.encounter.shield.energy = 1;

        GameSessionTargetContext targets{};
        targets.player_death_effect_inactive = true;
        const auto result = step_game_session(
            session, GameplayInputFrame{}, targets);
        assert(result.player_life_consumed);
        assert(!result.player_respawned);
        assert(result.player_respawn_shield_reset);
        assert(result.player_game_over_banner_requested);
        assert(session.campaign.player_lifecycle.lives == 0);
        assert(!session.campaign.player_lifecycle.player_active);
        assert(session.encounter.player.x == canonical_respawn_x);
        assert(session.encounter.player.y == canonical_respawn_y);
        assert(session.encounter.shield.energy == shield_nominal_max_energy);
    }

    // Drone destruction activity 2 blocks player-life settlement even when the
    // other recovered gates are ready.
    {
        GameSession session{};
        session.campaign.player_lifecycle.lives = 2;
        session.campaign.player_lifecycle.player_active = false;
        session.encounter.enemy_bomb_spawn_gate.counter = -356;
        session.encounter.drone.activity = canonical_drone_destruction_activity;

        GameSessionTargetContext targets{};
        targets.player_death_effect_inactive = true;
        const auto result = step_game_session(
            session, GameplayInputFrame{}, targets);
        assert(!result.player_life_consumed);
        assert(!result.player_respawned);
        assert(session.campaign.player_lifecycle.lives == 2);
        assert(!session.campaign.player_lifecycle.player_active);
    }

    std::cout << "Drone continuous game-session tests passed\n";
    return 0;
}
