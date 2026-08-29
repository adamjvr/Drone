#include <drone/gameplay/game_session.hpp>

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>


namespace {

bool has_audio_event(
    const drone::audio::AudioEventQueue& queue,
    const drone::audio::AudioCue cue,
    const drone::audio::AudioAction action) {
    for (const auto event : queue.view()) {
        if (event == drone::audio::AudioEvent{cue, action}) return true;
    }
    return false;
}

bool has_audio_control_event(
    const drone::audio::AudioEventQueue& queue,
    const drone::audio::AudioCue cue,
    const drone::audio::AudioAction action,
    const std::int32_t value) {
    for (const auto event : queue.view()) {
        if (event == drone::audio::AudioEvent{cue, action, value}) return true;
    }
    return false;
}

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
        assert(session.campaign.alien_ships_total == canonical_initial_encounter_alien_ships_total);
        assert(session.campaign.alien_ships_hit == 0);
        assert(session.encounter.encounter_alien_ships_total == canonical_initial_encounter_alien_ships_total);
        assert(session.encounter.encounter_alien_ships_hit == 0);
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
        assert(session.campaign.alien_ships_total == canonical_initial_encounter_alien_ships_total);
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
        assert(first.audio_events.size == 2);
        assert(first.audio_events.view()[0].cue == drone::audio::AudioCue::RapidMissileFire);
        assert(first.audio_events.view()[1].cue == drone::audio::AudioCue::SpecialLoadCycle);
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
        assert(second.audio_events.size == 1);
        assert(second.audio_events.view()[0].cue == drone::audio::AudioCue::ShieldPulse);
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

        // Processed counts 0/1 keep the ordinary player.x+17 steering target.
        session.campaign.mission.processed_count = 1;
        (void)step_game_session(session, GameplayInputFrame{}, targets);
        assert(session.encounter.special_weapon.x == 85); // Drone.x + 5
        assert(session.encounter.enemy_bombs.bombs[0].x == 102);

        // Win32 0x0040E3D9 redirects only after more than one Drone outcome
        // has been processed while the Probe is attached. No host flag exists.
        session.campaign.mission.processed_count = 2;
        (void)step_game_session(session, GameplayInputFrame{}, targets);
        assert(session.encounter.special_weapon.x == 85); // pinned to Drone.x + 5
        assert(session.encounter.enemy_bombs.bombs[0].x == 100); // target = probe.x+1=86
    }


    // Shareware Gemini activity and head geometry are both read from the
    // pre-boss-update session snapshot. The native initializer places head A
    // at x=6 and head B at x=176; player-left x=147 therefore selects B and
    // targets its exact center x=197.
    {
        GameSession session{};
        session.encounter.boss.family = BossFamily::Gemini;
        initialize_gemini_boss_runtime(
            session.encounter.boss.gemini, DifficultyLevel::Beginner);
        session.encounter.special_weapon.kind = SpecialWeaponKind::Stinger;
        session.encounter.special_weapon.activity = SpecialWeaponActivity::LoadedTracking;

        const auto result = step_game_session(session, GameplayInputFrame{});
        assert(result.stinger_target_identity == StingerTargetIdentity::GeminiHeadB);
        assert(result.stinger_target_desired_x == 197);
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

    // Phase-4 whole-session integration owns live transient formation timing,
    // template selection, and native weapon collision production. Force the
    // next phase-2 interval crossing and prove a native spawn; then destroy its
    // first actor through the launched Probe point-hitbox path on a later update.
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
        auto& actor = session.encounter.trajectories.groups[group_index].actors[0];
        actor.x = 100;
        actor.y = 100;
        session.encounter.special_weapon.kind = SpecialWeaponKind::Probe;
        session.encounter.special_weapon.activity = SpecialWeaponActivity::LaunchedHoming;
        session.encounter.drone.x = actor.x - 4;
        session.encounter.special_weapon.x = actor.x;
        session.encounter.special_weapon.y = actor.y + 2;
        // Freeze the spawned actor for this focused collision update; the
        // direct special producer itself no longer depends on path samples.
        targets.trajectory_paths = nullptr;

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
        assert(has_audio_event(
            result.audio_events,
            drone::audio::AudioCue::LidTopBossLoop,
            drone::audio::AudioAction::Play));

        // Remaining at/after the boundary cannot reinitialize an owned boss.
        result = step_game_session(session, GameplayInputFrame{}, targets);
        assert(!result.boss_activated);

        // Lid/Top destruction is no longer a semantic host trigger. Expose the
        // native Stinger core and let the boss-local collision producer enter
        // the same-update count-1 destruction tail.
        auto& lid_top = session.encounter.boss.lid_top;
        lid_top.root_x = 40;
        lid_top.root_y = 10;
        lid_top.root_fixed_x = 40 << 16;
        lid_top.root_fixed_y = 10 << 16;
        lid_top.root_velocity_x = 0;
        lid_top.root_velocity_y = 0;
        lid_top.horizontal_speed_cap = 0;
        lid_top.lid_activity = boss_activity_active;
        lid_top.lid_frame = 7;
        session.encounter.special_weapon.activity = SpecialWeaponActivity::LaunchedHoming;
        session.encounter.special_weapon.kind = SpecialWeaponKind::Stinger;
        session.encounter.special_weapon.x = 69;
        session.encounter.special_weapon.y = 44; // common homing moves to 42
        result = step_game_session(session, GameplayInputFrame{}, targets);
        assert(result.boss_destruction_transitions == 1);
        assert(result.lid_top_stinger_core_hit);
        assert(session.encounter.boss.lid_top.lid_destruction_progress == 1);
        assert(result.audio_events.size >= 1);
        assert((result.audio_events.view()[result.audio_events.size - 1] ==
            drone::audio::AudioEvent{
                drone::audio::AudioCue::LidTopBossLoop,
                drone::audio::AudioAction::StopAndRewind}));

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

    // Processed Drone count 1 selects Gemini. Its movement/geometry and local
    // special-weapon damage are native; no semantic destruction triggers cross
    // the GameSession boundary anymore.
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
        assert(session.encounter.boss.gemini.runtime_initialized);
        assert(has_audio_event(
            result.audio_events,
            drone::audio::AudioCue::GeminiBossLoop,
            drone::audio::AudioAction::Play));

        auto& gemini = session.encounter.boss.gemini;
        gemini.root_fixed_x = 0;
        gemini.root_fixed_y = 20 << 16;
        gemini.root_velocity_x = 0;
        gemini.root_velocity_y = 0;
        gemini.horizontal_speed_cap = 0;
        gemini.side_a.body_x = 0;
        gemini.side_a.body_y = 20;
        gemini.side_a.head_x = 6;
        gemini.side_a.head_y = 61;
        gemini.side_b.body_x = 170;
        gemini.side_b.body_y = 20;
        gemini.side_b.head_x = 176;
        gemini.side_b.head_y = 61;
        gemini.side_a.head_damage = 20;
        gemini.side_b.head_damage = 20;

        std::array<std::uint8_t, gemini_head_width * gemini_head_height> head{};
        // Common homing moves Y upward by two before the boss collision.
        head[8 * gemini_head_width + 21] = 1;
        GeminiBossSpriteMaskView masks{};
        masks.head_frame = head;
        targets.gemini_sprite_masks = &masks;

        session.encounter.player.x = 0; // nearest Gemini head is A
        session.encounter.special_weapon.activity = SpecialWeaponActivity::LaunchedHoming;
        session.encounter.special_weapon.kind = SpecialWeaponKind::Stinger;
        session.encounter.special_weapon.x = gemini.side_a.head_x + 21;
        session.encounter.special_weapon.y = gemini.side_a.head_y + 10;
        result = step_game_session(session, GameplayInputFrame{}, targets);
        assert(result.gemini_special_hit_side_a);
        assert(result.gemini_stinger_display_activated);
        assert(result.boss_destruction_transitions == 1);
        assert(result.boss_score_delta == 100);
        assert(gemini.side_a.body_activity == boss_activity_destruction);
        assert(!has_audio_event(
            result.audio_events,
            drone::audio::AudioCue::GeminiBossLoop,
            drone::audio::AudioAction::StopAndRewind));

        session.encounter.player.x = 250; // only active Gemini side B qualifies
        session.encounter.special_weapon.activity = SpecialWeaponActivity::LaunchedHoming;
        session.encounter.special_weapon.kind = SpecialWeaponKind::Stinger;
        session.encounter.special_weapon.x = gemini.side_b.head_x + 21;
        session.encounter.special_weapon.y = gemini.side_b.head_y + 10;
        result = step_game_session(session, GameplayInputFrame{}, targets);
        assert(result.gemini_special_hit_side_b);
        assert(result.boss_destruction_transitions == 1);
        assert(result.boss_score_delta == 100);
        assert(session.campaign.score.total == 200);
        assert(session.campaign.score.extra_life_progress == 200);
        assert(gemini.side_b.body_activity == boss_activity_destruction);
        assert(result.audio_events.size >= 3);
        assert((result.audio_events.view()[result.audio_events.size - 1] ==
            drone::audio::AudioEvent{
                drone::audio::AudioCue::GeminiBossLoop,
                drone::audio::AudioAction::StopAndRewind}));

        // The canonical shareware stop never initializes dispatch slot 2.
        reset_game_session(session, GameplaySessionResetScope::EncounterOnly);
        session.campaign.mission.processed_count = 2;
        session.encounter.drone.y = -201;
        session.encounter.gameplay_substep_phase = 1;
        targets.gemini_sprite_masks = nullptr;
        result = step_game_session(session, GameplayInputFrame{}, targets);
        assert(!result.boss_activated);
        assert(!session.encounter.boss.family.has_value());
    }

    // drone.wav is a parameterized loop, not a generic music cue. The exact
    // Y=-117 landmark starts it at volume zero; later phase-2 approach updates
    // raise the persistent scalar by one until the canonical cap of 80.
    {
        GameSession session{};
        session.encounter.drone.y = -118;
        session.encounter.gameplay_substep_phase = 1;

        auto result = step_game_session(session, GameplayInputFrame{});
        assert(session.encounter.drone.y == -116);
        assert(session.original_audio.drone_loop_volume_0_to_100 == 0);
        assert(has_audio_event(
            result.audio_events,
            drone::audio::AudioCue::DroneApproachLoop,
            drone::audio::AudioAction::Play));
        assert(has_audio_control_event(
            result.audio_events,
            drone::audio::AudioCue::DroneApproachLoop,
            drone::audio::AudioAction::SetVolume,
            0));

        // A phase-2 update begins at Y=-115, so the pre-movement control path
        // raises the scalar 0->1 before normal movement advances Y to -114.
        session.encounter.drone.y = -115;
        session.encounter.gameplay_substep_phase = 1;
        result = step_game_session(session, GameplayInputFrame{});
        assert(session.encounter.drone.y == -114);
        assert(session.original_audio.drone_loop_volume_0_to_100 == 1);
        assert(has_audio_control_event(
            result.audio_events,
            drone::audio::AudioCue::DroneApproachLoop,
            drone::audio::AudioAction::SetVolume,
            1));

        // The original scalar saturates at 80.
        session.original_audio.drone_loop_volume_0_to_100 = 79;
        session.encounter.drone.y = -20;
        session.encounter.gameplay_substep_phase = 1;
        result = step_game_session(session, GameplayInputFrame{});
        assert(session.original_audio.drone_loop_volume_0_to_100 == 80);
        assert(has_audio_control_event(
            result.audio_events,
            drone::audio::AudioCue::DroneApproachLoop,
            drone::audio::AudioAction::SetVolume,
            80));

        session.encounter.gameplay_substep_phase = 1;
        result = step_game_session(session, GameplayInputFrame{});
        assert(session.original_audio.drone_loop_volume_0_to_100 == 80);
        assert(!has_audio_control_event(
            result.audio_events,
            drone::audio::AudioCue::DroneApproachLoop,
            drone::audio::AudioAction::SetVolume,
            81));
    }

    // Destructive Drone transitions stop the live approach loop at their exact
    // producers. Timeout, rapid missile and Stinger all share StopAndRewind.
    {
        GameSession session{};
        session.encounter.drone.y = canonical_drone_hover_y;
        session.encounter.drone.hover_phase2_ticks =
            canonical_drone_hover_timeout_phase2_ticks - 1;
        session.encounter.gameplay_substep_phase = 1;
        auto result = step_game_session(session, GameplayInputFrame{});
        assert(result.drone_destruction_countdown_started);
        assert(has_audio_event(
            result.audio_events,
            drone::audio::AudioCue::DroneApproachLoop,
            drone::audio::AudioAction::StopAndRewind));

        GameSession rapid{};
        rapid.encounter.drone.x = 100;
        rapid.encounter.drone.y = 45;
        rapid.encounter.gameplay_substep_phase = 0;
        rapid.encounter.rapid_missiles.missiles[0].active = true;
        rapid.encounter.rapid_missiles.missiles[0].x = 100;
        rapid.encounter.rapid_missiles.missiles[0].y = 48;
        rapid.encounter.rapid_missiles.active_count = 1;
        result = step_game_session(rapid, GameplayInputFrame{});
        assert(result.rapid_missile_hit_drone);
        assert(has_audio_event(
            result.audio_events,
            drone::audio::AudioCue::DroneApproachLoop,
            drone::audio::AudioAction::StopAndRewind));

        GameSession stinger{};
        stinger.encounter.drone.x = 100;
        stinger.encounter.drone.y = 45;
        stinger.encounter.gameplay_substep_phase = 0;
        stinger.encounter.special_weapon.kind = SpecialWeaponKind::Stinger;
        stinger.encounter.special_weapon.activity = SpecialWeaponActivity::LaunchedHoming;
        stinger.encounter.special_weapon.x = 100;
        stinger.encounter.special_weapon.y = 47;
        result = step_game_session(stinger, GameplayInputFrame{});
        assert(result.stinger_hit_drone);
        assert(has_audio_event(
            result.audio_events,
            drone::audio::AudioCue::DroneApproachLoop,
            drone::audio::AudioAction::StopAndRewind));
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
        assert(session.original_audio.drone_loop_volume_0_to_100 == 60);
        assert(has_audio_control_event(
            result.audio_events,
            drone::audio::AudioCue::DroneApproachLoop,
            drone::audio::AudioAction::SetVolume,
            60));

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
        assert(has_audio_event(
            result.audio_events,
            drone::audio::AudioCue::DroneApproachLoop,
            drone::audio::AudioAction::StopAndRewind));
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

        const auto random_draws_before = session.original_random.draws;
        auto result = step_game_session(session, GameplayInputFrame{});
        assert(result.drone_detonation_effect_tick);
        assert(result.drone_detonation_explosion_spawns_requested ==
               canonical_drone_detonation_explosions_per_effect_tick);
        assert(result.drone_detonation_random_draws_consumed ==
               canonical_drone_detonation_random_draws_per_effect_tick);
        assert(session.original_random.draws ==
               random_draws_before + canonical_drone_detonation_random_draws_per_effect_tick);
        assert(result.drone_detonation_settlement_reset);
        assert(!result.drone_detonation_settlement_advanced);
        assert(session.encounter.drone.detonation_tick == 329);
        assert(session.encounter.drone.destruction_settlement_phase0_ticks == 0);
        assert(session.encounter.drone.detonation_center_y == 81);

        session.encounter.gameplay_substep_phase = 3;
        const auto second_random_draws_before = session.original_random.draws;
        result = step_game_session(session, GameplayInputFrame{});
        assert(result.drone_detonation_effect_tick);
        assert(result.drone_detonation_explosion_spawns_requested ==
               canonical_drone_detonation_explosions_per_effect_tick);
        assert(result.drone_detonation_random_draws_consumed ==
               canonical_drone_detonation_random_draws_per_effect_tick);
        assert(session.original_random.draws ==
               second_random_draws_before + canonical_drone_detonation_random_draws_per_effect_tick);
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
        assert(result.audio_events.size == 2);
        assert((result.audio_events.view()[0] == drone::audio::AudioEvent{
            drone::audio::AudioCue::DroneApproachLoop,
            drone::audio::AudioAction::SetVolume,
            80}));
        assert((result.audio_events.view()[1] == drone::audio::AudioEvent{
            drone::audio::AudioCue::ProbeImpact,
            drone::audio::AudioAction::Play}));
        assert(session.original_audio.drone_loop_volume_0_to_100 == 80);
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
        for (const auto event : result.audio_events.view()) {
            assert(event.cue != drone::audio::AudioCue::ProbeImpact);
            assert(event.cue != drone::audio::AudioCue::StingerImpact);
            assert(event.cue != drone::audio::AudioCue::PlayerHitExplosion);
        }
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
        assert(result.audio_events.size == 2);
        assert((result.audio_events.view()[0] == drone::audio::AudioEvent{
            drone::audio::AudioCue::SpecialLaunch, drone::audio::AudioAction::Play}));
        assert((result.audio_events.view()[1] == drone::audio::AudioEvent{
            drone::audio::AudioCue::PlayerHitExplosion, drone::audio::AudioAction::Play}));
        assert(result.player_destruction_started);
        assert(result.player_death_effect_requested);
        assert(!result.player_death_effect_visible);
        assert(result.player_death_effect_frame == canonical_player_death_effect_initial_frame);
        assert(session.encounter.player_death_effect.activity ==
               PlayerDeathEffectActivity::PreRoll);
        assert(session.encounter.player_death_effect.x == session.encounter.player.x - 10);
        assert(session.encounter.player_death_effect.y == session.encounter.player.y - 8);
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

        const auto result = step_game_session(
            session, GameplayInputFrame{});
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

    // An active native death explosion blocks settlement even when the bomb
    // gate becomes ready. If phase-2 advancement retires frame 26 -> 27 in the
    // same update, the later settlement gate observes the now-inactive actor
    // and may consume the life immediately.
    {
        GameSession session{};
        session.campaign.player_lifecycle.lives = 3;
        session.campaign.player_lifecycle.player_active = false;
        session.encounter.enemy_bomb_spawn_gate.counter = -356;
        session.encounter.gameplay_substep_phase = 0; // advances to phase 1
        session.encounter.player_death_effect.activity =
            PlayerDeathEffectActivity::Visible;
        session.encounter.player_death_effect.frame = 10;

        auto result = step_game_session(session, GameplayInputFrame{});
        assert(!result.player_death_effect_advanced);
        assert(!result.player_life_consumed);
        assert(session.encounter.enemy_bomb_spawn_gate.counter == -355);

        session.encounter.gameplay_substep_phase = 1; // advances to phase 2
        session.encounter.player_death_effect.frame = 26;
        result = step_game_session(session, GameplayInputFrame{});
        assert(result.player_death_effect_advanced);
        assert(result.player_death_effect_retired);
        assert(result.player_life_consumed);
        assert(result.player_respawned);
        assert(session.campaign.player_lifecycle.lives == 2);
    }

    // The last life follows the same reset ordering but remains inactive and
    // requests the already-recovered game-over banner path.
    {
        GameSession session{};
        session.campaign.player_lifecycle.lives = 1;
        session.campaign.player_lifecycle.player_active = false;
        session.encounter.enemy_bomb_spawn_gate.counter = -356;
        session.encounter.shield.energy = 1;

        const auto result = step_game_session(
            session, GameplayInputFrame{});
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

        const auto result = step_game_session(
            session, GameplayInputFrame{});
        assert(!result.player_life_consumed);
        assert(!result.player_respawned);
        assert(session.campaign.player_lifecycle.lives == 2);
        assert(!session.campaign.player_lifecycle.player_active);
    }

    std::cout << "Drone continuous game-session tests passed\n";
    return 0;
}
