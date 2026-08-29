#include <drone/gameplay/game_session.hpp>

#include <drone/gameplay/gameplay_phase.hpp>

namespace drone::gameplay {

namespace {

void append_mission_interstitial_audio(
    drone::audio::AudioEventQueue& queue,
    const MissionInterstitialPlan& plan) noexcept {
    const auto cue = plan.sound == MissionInterstitialSound::Deepness
        ? drone::audio::AudioCue::MissionDeepness
        : drone::audio::AudioCue::MissionDetonate;
    (void)queue.push({cue, drone::audio::AudioAction::Play});
}

void append_original_explosion_variants(
    drone::audio::AudioEventQueue& queue,
    drone::audio::OriginalAudioRuntimeState& audio_runtime,
    const std::size_t count) noexcept {
    for (std::size_t i = 0; i < count; ++i) {
        (void)queue.push({
            drone::audio::next_original_explosion_sfx_cue(audio_runtime),
            drone::audio::AudioAction::Play});
    }
}

drone::audio::AudioCue post_game_results_audio_cue(
    const MissionResultsMusic music) noexcept {
    using drone::audio::AudioCue;
    switch (music) {
    case MissionResultsMusic::Choral: return AudioCue::ResultsChoral;
    case MissionResultsMusic::Suspense: return AudioCue::ResultsSuspense;
    case MissionResultsMusic::Moon: return AudioCue::ResultsMoon;
    case MissionResultsMusic::Hiphop: return AudioCue::ResultsHiphop;
    }
    return AudioCue::ResultsSuspense;
}

void append_post_game_phase_start_audio(
    drone::audio::AudioEventQueue& queue,
    const PostGameModalPhase phase,
    const Win32PostGamePlan& plan) noexcept {
    using drone::audio::AudioAction;
    using drone::audio::AudioCue;
    switch (phase) {
    case PostGameModalPhase::ResultsConfirmLock:
        (void)queue.push({post_game_results_audio_cue(plan.outcome_summary.music), AudioAction::Play});
        break;
    case PostGameModalPhase::OrderingInformation:
        (void)queue.push({AudioCue::OrderingInformation, AudioAction::Play});
        break;
    case PostGameModalPhase::CompletionCredits:
        (void)queue.push({AudioCue::CompletionCredits, AudioAction::Play});
        break;
    case PostGameModalPhase::Inactive:
    case PostGameModalPhase::ResultsAwaitConfirmation:
    case PostGameModalPhase::HighScoreTable:
    case PostGameModalPhase::Complete:
        break;
    }
}

void append_post_game_phase_stop_audio(
    drone::audio::AudioEventQueue& queue,
    const PostGameModalPhase phase,
    const Win32PostGamePlan& plan) noexcept {
    using drone::audio::AudioAction;
    using drone::audio::AudioCue;
    switch (phase) {
    case PostGameModalPhase::ResultsAwaitConfirmation:
        (void)queue.push({post_game_results_audio_cue(plan.outcome_summary.music),
                          AudioAction::StopAndRewind});
        break;
    case PostGameModalPhase::OrderingInformation:
        (void)queue.push({AudioCue::OrderingInformation, AudioAction::StopAndRewind});
        break;
    case PostGameModalPhase::CompletionCredits:
        (void)queue.push({AudioCue::CompletionCredits, AudioAction::StopAndRewind});
        break;
    case PostGameModalPhase::Inactive:
    case PostGameModalPhase::ResultsConfirmLock:
    case PostGameModalPhase::HighScoreTable:
    case PostGameModalPhase::Complete:
        break;
    }
}

void synchronize_post_game_raw_state(GameSession& session) noexcept {
    if (!session.post_game.plan) {
        return;
    }

    switch (session.post_game.phase) {
    case PostGameModalPhase::ResultsConfirmLock:
    case PostGameModalPhase::ResultsAwaitConfirmation:
        // Results is inline in the original state-2 tail.
        session.state = GameState::ActiveGameplay;
        break;
    case PostGameModalPhase::OrderingInformation:
        session.state = GameState::OrderingInformation;
        break;
    case PostGameModalPhase::HighScoreTable:
        session.state = GameState::HighScores;
        break;
    case PostGameModalPhase::CompletionCredits:
        // The perfect-completion path is normalized to state 1 by the recovered
        // semantic planner around the synchronous credits call.
        session.state = session.post_game.plan->final_state;
        break;
    case PostGameModalPhase::Complete:
        session.state = session.post_game.plan->final_state;
        break;
    case PostGameModalPhase::Inactive:
        break;
    }
}

} // namespace

GameSession::GameSession() {
    reset_trajectory_encounter(encounter.trajectories);
    reset_trajectory_spawn_scheduler(
        encounter.trajectory_spawn, runtime.difficulty, campaign.mission.processed_count);
}

void reset_game_session(GameSession& session, const GameplaySessionResetScope scope) {
    if (scope == GameplaySessionResetScope::FullCampaign) {
        session.campaign = GameCampaignState{};
        session.post_game = PostGameRuntimeState{};
        session.total_gameplay_updates = 0;
    }

    session.encounter = GameEncounterState{};
    reset_trajectory_encounter(session.encounter.trajectories);
    reset_trajectory_spawn_scheduler(
        session.encounter.trajectory_spawn,
        session.runtime.difficulty,
        session.campaign.mission.processed_count);
    // Encounter rebuilds reactivate the player entity while preserving the
    // campaign life count. The active flag lives in the older combined helper
    // type, so normalize it here at the session ownership boundary.
    session.campaign.player_lifecycle.player_active =
        session.campaign.player_lifecycle.lives > 0;
    session.state = GameState::ActiveGameplay;
}

GameSessionTickResult step_game_session(
    GameSession& session,
    const GameplayInputFrame& input,
    const GameSessionTargetContext& targets) {

    GameSessionTickResult result{};
    if (session.post_game.phase != PostGameModalPhase::Inactive) {
        result.post_game_phase = session.post_game.phase;
        result.post_game_plan = session.post_game.plan;
        return result;
    }
    if (session.state != GameState::ActiveGameplay) {
        return result;
    }

    // Win32 state 2 branches directly into the inline 0x004115BE post-game
    // tail when player_lives <= 0, before any ordinary gameplay subsystem can
    // consume input, advance RNG or mutate encounter state on that dispatch.
    if (win32_enters_post_game_results(session.campaign.player_lifecycle.lives)) {
        const auto plan = win32_post_game_plan(
            Win32PostGameContext{
                .mission = session.campaign.mission,
                .mothership_destroyed = session.campaign.mothership_destroyed,
                .suppress_results_and_ordering =
                    session.campaign.suppress_results_and_ordering,
                .demo_playback_mode = session.runtime.demo_playback_mode,
                .high_score_disqualified = session.campaign.high_score_disqualified,
                .score = session.campaign.score.total,
                .alien_ships_hit = session.campaign.alien_ships_hit,
                .alien_ships_total = session.campaign.alien_ships_total,
            },
            session.high_scores);
        if (!plan) {
            result.post_game_plan_invalid = true;
            return result;
        }

        (void)begin_post_game_runtime(session.post_game, *plan);
        append_post_game_phase_start_audio(
            result.audio_events, session.post_game.phase, *session.post_game.plan);
        synchronize_post_game_raw_state(session);
        result.post_game_started = true;
        result.post_game_phase = session.post_game.phase;
        result.post_game_plan = session.post_game.plan;
        result.advanced = true;
        return result;
    }

    auto& encounter = session.encounter;
    auto& campaign = session.campaign;
    result.encounter_alien_ships_total = encounter.encounter_alien_ships_total;
    result.encounter_alien_ships_hit = encounter.encounter_alien_ships_hit;
    result.mission_alien_ships_total = campaign.alien_ships_total;
    result.mission_alien_ships_hit = campaign.alien_ships_hit;
    bool end_run_transition = false;

    const auto fold_alien_statistics_for_interstitial = [&]() {
        result.encounter_alien_statistics = make_encounter_alien_statistics(
            encounter.encounter_alien_ships_hit,
            encounter.encounter_alien_ships_total);
        fold_encounter_alien_statistics(
            encounter.encounter_alien_ships_hit,
            encounter.encounter_alien_ships_total,
            campaign.alien_ships_hit,
            campaign.alien_ships_total);
        result.encounter_alien_statistics_folded = true;
    };

    // 0x00491CAC is processed before the shared four-phase scheduler. If this
    // update reaches 99, the original immediately restores 100, triggers the
    // logical detonation setup, commits outcome 2 and applies its score penalty.
    const auto destruction_countdown_result = step_drone_destruction_countdown(
        encounter.drone,
        campaign.mission,
        campaign.score);
    result.drone_destruction_countdown_advanced =
        destruction_countdown_result.advanced;
    result.drone_detonation_started = destruction_countdown_result.detonation_started;
    result.drone_detonation_outcome_committed =
        destruction_countdown_result.outcome_committed;
    result.drone_detonation_score_delta = destruction_countdown_result.score_delta;

    // Win32 state 2 advances the shared four-phase scalar only after the
    // pre-detonation countdown. A newly triggered detonation therefore resets
    // its logical tick to zero above and becomes tick 1 later in this same update.
    encounter.gameplay_substep_phase =
        advance_win32_gameplay_substep_phase(encounter.gameplay_substep_phase);
    bool animation_tick = is_win32_phase2(encounter.gameplay_substep_phase);

    advance_drone_detonation_tick(encounter.drone);

    // The shared Drone settlement scalar advances in the early phase-2 block,
    // before the destruction-settlement gate and before trajectory/normal-Drone
    // work later in state 2. A normal Drone reaching Y=230 can reset it below.
    encounter.drone_settlement_tick = advance_drone_settlement_tick(
        encounter.drone_settlement_tick,
        encounter.gameplay_substep_phase);

    // The destruction settlement gate is early in state 2 and observes the
    // phase-0 effect-side counter from prior updates. It consumes one life only
    // after that field exceeds 70. With more than one life, the mission
    // interstitial/encounter transition executes first, exactly as in Win32.
    if (drone_destruction_settlement_ready(encounter.drone)) {
        result.drone_destruction_settled = true;
        const auto processed_count = campaign.mission.processed_count;
        const bool run_interstitial = campaign.player_lifecycle.lives > 1;

        if (run_interstitial) {
            result.mission_interstitial = mission_interstitial_plan(campaign.mission);
            if (result.mission_interstitial.has_value()) {
                append_mission_interstitial_audio(
                    result.audio_events, *result.mission_interstitial);
                result.encounter_transition = win32_post_drone_transition_plan(
                    result.mission_interstitial->processed_count,
                    result.mission_interstitial->detonated_count);
                // Win32 renders the encounter-local summary and then folds the
                // complete local pair into mission-wide Results counters before
                // its encounter-only reinitialization call.
                fold_alien_statistics_for_interstitial();
            }

            if (result.encounter_transition.has_value()) {
                end_run_transition =
                    result.encounter_transition->disposition ==
                    EncounterTransitionDisposition::EndRun;
                if (end_run_transition) {
                    // The canonical shareware count-2 branch clears lives inside
                    // run_mission_outcome_transition before the caller performs
                    // its unconditional destruction-settlement decrement.
                    campaign.player_lifecycle.lives = 0;
                }

                reset_game_session(session, GameplaySessionResetScope::EncounterOnly);
                result.drone_destruction_transition_started = true;
                // Reset may replace the scheduler with phase 0; later helpers in
                // this same logical update must observe the rebuilt encounter.
                animation_tick =
                    is_win32_phase2(encounter.gameplay_substep_phase);
            }
        }

        --campaign.player_lifecycle.lives;
        result.drone_life_lost = true;
        encounter.drone.y = drone_reentry_y_for_processed_count(
            static_cast<std::int32_t>(processed_count));

        if (campaign.player_lifecycle.lives > 0) {
            campaign.player_lifecycle.player_active = true;
            encounter.drone_settlement_tick = canonical_drone_settlement_tick_cap;
            encounter.drone.activity = canonical_drone_active_activity;
        } else {
            campaign.player_lifecycle.player_active = false;
            result.drone_game_over_pending = true;
        }
    }

    // Probe decode progression is an early state-2 subsystem, before normal
    // Drone movement. This is crucial on the completion update: status 0->3 can
    // start phase 2 immediately, and status 3->1 releases the Drone later in the
    // same gameplay update rather than one update late.
    const auto probe_decode_result = step_probe_decode(
        encounter.special_weapon,
        session.original_random,
        campaign.score);
    result.probe_decode_phase1_completed = probe_decode_result.phase1_completed;
    result.probe_decode_completed = probe_decode_result.disarm_completed;
    result.probe_decode_score_delta = probe_decode_result.score_delta;
    result.probe_decode_completion_effect_random =
        probe_decode_result.completion_effect_random;
    if (probe_decode_result.disarm_completed) {
        (void)mark_drone_disarm_completed(encounter.drone);
    }

    // The persistent primary Loop replenisher at 0x0040CEE8 runs before the
    // phase-2 transient formation producer. It owns one actor insertion at a
    // time and increments the encounter-local alien total 0x00466B04.
    if (targets.trajectory_paths != nullptr) {
        const auto primary_replenishment = step_primary_trajectory_replenishment(
            encounter.trajectories,
            session.original_random,
            PrimaryTrajectoryReplenishmentContext{
                .difficulty = session.runtime.difficulty,
                .processed_drone_count = static_cast<std::int32_t>(campaign.mission.processed_count),
                .gameplay_phase = encounter.gameplay_substep_phase,
                .demo_playback_mode = session.runtime.demo_playback_mode,
                .drone_activity = encounter.drone.activity,
            });
        result.primary_trajectory_replenishment_checked =
            primary_replenishment.eligible_substep;
        result.primary_trajectory_roll_forced_to_one =
            primary_replenishment.roll_forced_to_one;
        result.primary_trajectory_spawn_roll_passed =
            primary_replenishment.spawn_roll_passed;
        result.primary_trajectory_actor_replenished = primary_replenishment.activated;
        result.primary_trajectory_group_reactivated = primary_replenishment.group_reactivated;
        result.primary_trajectory_actor_index = primary_replenishment.actor_index;
        result.primary_trajectory_entry_x = primary_replenishment.entry_x;
        result.primary_trajectory_entry_y = primary_replenishment.entry_y;
        if (primary_replenishment.activated) {
            ++encounter.encounter_alien_ships_total;
        }

        // Win32 0x0040D390..0x0040D947 owns live transient formation timing,
        // template selection, path-side randomization and the accompanying CRT
        // draws before normal trajectory advancement.
        const auto spawn_result = step_live_trajectory_spawn(
            encounter.trajectory_spawn,
            encounter.trajectories,
            *targets.trajectory_paths,
            session.original_random,
            TrajectorySpawnContext{
                .difficulty = session.runtime.difficulty,
                .processed_drone_count = static_cast<std::int32_t>(campaign.mission.processed_count),
                .gameplay_phase = encounter.gameplay_substep_phase,
                .demo_playback_mode = session.runtime.demo_playback_mode,
                .demo_recording_mode = session.runtime.demo_recording_mode,
                .drone_y = encounter.drone.y,
                .drone_activity = encounter.drone.activity,
                .mothership_destruction_active = targets.mothership_destruction_active,
            });
        result.trajectory_group_spawned = spawn_result.activated;
        result.trajectory_spawn_forced = spawn_result.spawn_roll_forced;
        result.trajectory_spawn_roll_passed = spawn_result.spawn_roll_passed;
        result.trajectory_spawned_group = spawn_result.group_index;
        result.trajectory_spawn_sound_index = spawn_result.sound_index;
        if (spawn_result.sound_index.has_value()) {
            (void)result.audio_events.push({
                drone::audio::trajectory_flight_cue(*spawn_result.sound_index),
                drone::audio::AudioAction::Play});
        }
        result.trajectory_spawn_runtime_family = spawn_result.runtime_path_family;
        result.trajectory_spawn_x_offset = spawn_result.group_x_offset;
        result.trajectory_spawn_y_offset = spawn_result.group_y_offset;
        result.trajectory_spawn_actor_offsets_randomized =
            spawn_result.actor_offsets_randomized;
        if (spawn_result.activated) {
            ++encounter.encounter_alien_ships_total;
        }
        const auto trajectory_result = advance_trajectory_encounter(
            encounter.trajectories,
            *targets.trajectory_paths,
            encounter.gameplay_substep_phase,
            campaign.score);
        result.trajectory_actors_activated = trajectory_result.actors_activated;
        encounter.encounter_alien_ships_total +=
            static_cast<std::int32_t>(trajectory_result.actors_activated);
        // Each later stagger activation in a transient group increments both
        // encounter-local 0x00466B04 and mission-wide 0x00446078 at
        // 0x0041610E..0x00416121. The later interstitial still folds the full
        // encounter total again; preserve that original double-accounting quirk.
        campaign.alien_ships_total +=
            static_cast<std::int32_t>(trajectory_result.actors_activated);
        result.trajectory_actors_escaped = trajectory_result.actors_escaped;
        result.trajectory_groups_retired += trajectory_result.groups_retired;
        result.trajectory_score_delta += trajectory_result.escape_score_delta;
    }

    // Rapid-missile trajectory collision is part of the trajectory updater in
    // Win32: post-path/current-frame actor state is tested against pre-movement
    // missile coordinates through the current extracted sprite mask.
    if (targets.trajectory_sprite_masks != nullptr) {
        const auto rapid_trajectory = collide_rapid_missiles_with_trajectories(
            encounter.trajectories,
            encounter.rapid_missiles,
            *targets.trajectory_sprite_masks,
            campaign.score);
        result.trajectory_rapid_collisions = rapid_trajectory.collisions;
        result.trajectory_rapid_missiles_consumed = rapid_trajectory.rapid_missiles_consumed;
        result.trajectory_actors_destroyed += rapid_trajectory.actors_destroyed;
        result.trajectory_groups_retired += rapid_trajectory.groups_retired;
        result.trajectory_destruction_bursts += rapid_trajectory.destruction_bursts;
        result.trajectory_score_delta += rapid_trajectory.score_delta;
        encounter.encounter_alien_ships_hit +=
            static_cast<std::int32_t>(rapid_trajectory.actors_destroyed);
        campaign.alien_ships_hit +=
            static_cast<std::int32_t>(rapid_trajectory.actors_destroyed);
        append_original_explosion_variants(
            result.audio_events, session.original_audio,
            rapid_trajectory.explosion_sfx_variant_calls);
    }

    // update_drone_detonation_effect is called after trajectory processing and
    // before the normal Drone/boss region. The portable core now owns the exact
    // update-side 17-draw CRT sequence and eight explosion requests. The separate
    // render-only radial-noise routine remains a fidelity renderer concern.
    const auto detonation_effect_result = step_drone_detonation_effect_logic(
        encounter.drone,
        encounter.gameplay_substep_phase,
        session.original_random);
    result.drone_detonation_effect_tick = detonation_effect_result.logical_effect_tick;
    result.drone_detonation_explosion_spawns_requested =
        detonation_effect_result.explosion_spawns_requested;
    result.drone_detonation_random_draws_consumed =
        detonation_effect_result.random_draws_consumed;
    result.drone_detonation_radial_start_angle =
        detonation_effect_result.radial_start_angle;
    result.drone_detonation_explosions = detonation_effect_result.explosions;
    result.drone_detonation_settlement_reset = detonation_effect_result.settlement_reset;
    result.drone_detonation_settlement_advanced =
        detonation_effect_result.settlement_advanced;

    // Canonical ordering places normal Drone objective motion/settlement after
    // trajectory and detonation-effect updates and before boss dispatch.
    const auto drone_result = step_drone_objective_normal(
        encounter.drone,
        encounter.gameplay_substep_phase,
        campaign.mission,
        encounter.drone_settlement_tick);
    result.drone_moved = drone_result.moved;
    result.drone_disarm_committed = drone_result.disarm_committed;
    result.drone_settlement_tick_reset = drone_result.settlement_tick_reset;
    result.drone_hover_timeout_reached = drone_result.hover_timeout_reached;
    result.drone_destruction_countdown_started =
        drone_result.destruction_countdown_started;
    if (drone_result.disarm_completion_cleared) {
        result.probe_decode_cleared = clear_completed_probe_decode(
            encounter.special_weapon);
    }

    if (drone_result.resolution_transition_ready) {
        result.mission_interstitial = mission_interstitial_plan(campaign.mission);
        if (result.mission_interstitial.has_value()) {
            append_mission_interstitial_audio(
                result.audio_events, *result.mission_interstitial);
            result.encounter_transition = win32_post_drone_transition_plan(
                result.mission_interstitial->processed_count,
                result.mission_interstitial->detonated_count);
            fold_alien_statistics_for_interstitial();
        }

        if (result.encounter_transition.has_value()) {
            const auto processed_count = campaign.mission.processed_count;
            end_run_transition =
                result.encounter_transition->disposition ==
                EncounterTransitionDisposition::EndRun;
            if (end_run_transition) {
                // Canonical shareware objective 2 explicitly zeroes lives before
                // its encounter-only rebuild so the next gameplay dispatch enters
                // the already-recovered post-game/results path.
                campaign.player_lifecycle.lives = 0;
            }

            reset_game_session(session, GameplaySessionResetScope::EncounterOnly);
            encounter.drone.y = drone_reentry_y_for_processed_count(processed_count);
            result.drone_resolution_transition_started = true;
            animation_tick = is_win32_phase2(encounter.gameplay_substep_phase);
        }
    }

    // Stinger target selection occurs before the boss update/dispatch region in
    // the original. Capture the already-existing boss lifecycle here so a boss
    // activated or retired later in this same update cannot become/disappear as
    // a Stinger target one update too early.
    const auto stinger_boss_snapshot = encounter.boss;

    // Boss selection no longer consumes an external boundary event: the owned
    // Drone path emits exact Y == -200 after its phase-2 movement. Registered-
    // only dispatch slots are still rejected by the shareware boss owner.
    if (drone_result.boss_approach_boundary_reached) {
        result.boss_activated = activate_shareware_boss_for_processed_drones(
            encounter.boss,
            campaign.mission.processed_count);
        if (result.boss_activated) {
            result.boss_activated_family = encounter.boss.family;
            if (encounter.boss.family == BossFamily::LidTop) {
                initialize_lid_top_boss_runtime(
                    encounter.boss.lid_top,
                    session.runtime.difficulty,
                    session.runtime.demo_playback_mode);
                // Win32 0x004172EC..0x00417323 starts retro1.wav with
                // DSBPLAY_LOOPING as part of the encounter initializer.
                (void)result.audio_events.push({
                    drone::audio::AudioCue::LidTopBossLoop,
                    drone::audio::AudioAction::Play});
            } else if (encounter.boss.family == BossFamily::Gemini) {
                initialize_gemini_boss_runtime(
                    encounter.boss.gemini,
                    session.runtime.difficulty);
                // Win32 0x00405F92..0x00405FA1 starts gemini.wav looping
                // after the paired Gemini state has been initialized.
                (void)result.audio_events.push({
                    drone::audio::AudioCue::GeminiBossLoop,
                    drone::audio::AudioAction::Play});
            }
        }
    }

    // Both recovered cooldown/gate scalars advance once per active state-2
    // update before their producer paths can consume the ready values. The
    // bomb gate is also the exact post-death quiet-period counter.
    advance_enemy_bomb_spawn_gate(encounter.enemy_bomb_spawn_gate);
    advance_rapid_missile_cooldown(encounter.rapid_missiles);

    // Win32 0x0040E1DA..0x0040E271 advances the singleton player-death
    // explosion only on shared substep phase 2, immediately before the deferred
    // respawn gate observes its activity byte. This actor is now session-owned;
    // only its immutable frame pixels/random debris presentation remain outside.
    const auto player_death_effect_step = step_player_death_effect(
        encounter.player_death_effect, encounter.gameplay_substep_phase);
    result.player_death_effect_advanced = player_death_effect_step.advanced;
    result.player_death_effect_became_visible =
        player_death_effect_step.became_visible;
    result.player_death_effect_cleared_out_of_bounds =
        player_death_effect_step.cleared_out_of_bounds;
    result.player_death_effect_retired =
        player_death_effect_step.retired_at_terminal_frame;
    result.player_death_effect_visible =
        player_death_effect_visible(encounter.player_death_effect);
    result.player_death_effect_frame = encounter.player_death_effect.frame;

    // Player life consumption is deferred from collision time. The original
    // settles only after the bomb gate rises above -356, the native death actor
    // is inactive, the player is inactive, lives remain, and the Drone is not
    // in destruction activity 2.
    const auto player_respawn = settle_player_death(
        campaign.player_lifecycle,
        encounter.player,
        encounter.shield,
        PlayerRespawnGate{
            .bomb_spawn_gate_allows_settlement =
                enemy_bomb_spawn_gate_allows_respawn(encounter.enemy_bomb_spawn_gate),
            .death_effect_inactive =
                player_death_effect_inactive(encounter.player_death_effect),
            .player_inactive = !campaign.player_lifecycle.player_active,
            .drone_allows_respawn =
                encounter.drone.activity != canonical_drone_destruction_activity,
        });
    result.player_life_consumed = player_respawn.consumed_life;
    result.player_respawned = player_respawn.respawned;
    result.player_respawn_shield_reset = player_respawn.shield_reset;
    result.player_game_over_banner_requested = player_respawn.game_over;

    const bool player_active = campaign.player_lifecycle.player_active;

    // Physical input has already converged to the portable semantic frame.
    step_player_directional_motion(encounter.player, input.movement, animation_tick);

    result.rapid_missile_fired = try_fire_rapid_missile(
        encounter.rapid_missiles,
        encounter.player,
        input.rapid_fire,
        player_active);
    if (result.rapid_missile_fired) {
        (void)result.audio_events.push({
            drone::audio::AudioCue::RapidMissileFire,
            drone::audio::AudioAction::Play});
    }

    // The loaded special's recovered switch counter advances independently of
    // Down-key input. A Down action loads when inactive, otherwise attempts the
    // threshold-gated Probe/Stinger cycle.
    advance_special_weapon_switch_progress(encounter.special_weapon);
    if (input.special_load_cycle) {
        if (encounter.special_weapon.activity == SpecialWeaponActivity::Inactive) {
            result.special_loaded = load_special_weapon(
                encounter.special_weapon, encounter.player, true, player_active);
            if (result.special_loaded) {
                (void)result.audio_events.push({
                    drone::audio::AudioCue::SpecialLoadCycle,
                    drone::audio::AudioAction::Play});
                // Win32 0x0040CD83 resets the shared target pointer to the
                // center-screen dummy object on every successful load.
                reset_stinger_target(encounter.stinger_target);
            }
        } else if (encounter.special_weapon.activity == SpecialWeaponActivity::LoadedTracking) {
            result.special_cycled = toggle_loaded_special_weapon(
                encounter.special_weapon, true, player_active);
            if (result.special_cycled) {
                (void)result.audio_events.push({
                    drone::audio::AudioCue::SpecialLoadCycle,
                    drone::audio::AudioAction::Play});
            }
        }
    }

    result.special_launched = launch_special_weapon(
        encounter.special_weapon,
        input.special_launch,
        player_active);
    if (result.special_launched) {
        (void)result.audio_events.push({
            drone::audio::AudioCue::SpecialLaunch,
            drone::audio::AudioAction::Play});
    }

    // Common special movement now owns the original Stinger target-priority
    // chain. Geometry for boss families whose movement is not reconstructed is
    // still supplied as actor facts, but the host no longer preselects a target.
    if (encounter.special_weapon.activity == SpecialWeaponActivity::ProbeAttachedDecoding) {
        (void)pin_attached_probe_to_drone(encounter.special_weapon, encounter.drone.x);
    } else if (encounter.special_weapon.activity == SpecialWeaponActivity::LoadedTracking ||
               encounter.special_weapon.activity == SpecialWeaponActivity::LaunchedHoming) {
        std::int32_t target_x = probe_drone_target_x(encounter.drone.x);
        if (encounter.special_weapon.kind == SpecialWeaponKind::Stinger) {
            auto stinger_context = targets.stinger_targets;

            // For the two shareware boss families already owned by GameSession,
            // activity comes from the pre-boss-update snapshot rather than the
            // host. Geometry/frame facts remain external until movement is native.
            if (stinger_boss_snapshot.family == BossFamily::Gemini) {
                stinger_context.gemini_body_a_active =
                    stinger_boss_snapshot.gemini.side_a.body_activity == boss_activity_active;
                stinger_context.gemini_body_b_active =
                    stinger_boss_snapshot.gemini.side_b.body_activity == boss_activity_active;
                stinger_context.gemini_head_a = StingerTargetGeometry{
                    .x = stinger_boss_snapshot.gemini.side_a.head_x,
                    .width = gemini_head_width,
                };
                stinger_context.gemini_head_b = StingerTargetGeometry{
                    .x = stinger_boss_snapshot.gemini.side_b.head_x,
                    .width = gemini_head_width,
                };
            } else if (stinger_boss_snapshot.family == BossFamily::LidTop) {
                stinger_context.lid_top_top_active =
                    stinger_boss_snapshot.lid_top.top_activity == boss_activity_active;
                stinger_context.lid_current_frame =
                    stinger_boss_snapshot.lid_top.lid_frame;
                stinger_context.lid_top_top = StingerTargetGeometry{
                    .x = stinger_boss_snapshot.lid_top.root_x,
                    .width = lid_top_top_width,
                };
            }

            const auto selection = select_stinger_target(
                encounter.stinger_target,
                stinger_context,
                encounter.player.x);
            target_x = selection.desired_x;
            result.stinger_target_identity = selection.identity;
            result.stinger_target_desired_x = selection.desired_x;
            result.stinger_target_changed = selection.target_changed;
        }
        (void)step_special_weapon_homing(
            encounter.special_weapon, encounter.player, target_x);
    } else if (encounter.special_weapon.activity == SpecialWeaponActivity::ImpactConsumed) {
        (void)settle_special_weapon_terminal_state(encounter.special_weapon);
    }

    const auto shield_result = step_player_shield(
        encounter.shield,
        input.shield,
        player_active,
        animation_tick);
    result.shield_active = shield_result.active;
    result.shield_sound_requested = shield_result.play_sound;
    if (result.shield_sound_requested) {
        (void)result.audio_events.push({
            drone::audio::AudioCue::ShieldPulse,
            drone::audio::AudioAction::Play});
    }

    // The separate six-frame stinger.jba display is processed before the later
    // bomb/special collision block. Frames 3..5 deal +15 trajectory damage; the
    // display then advances and retires after frame 5. Activations caused later
    // in this update therefore cannot damage trajectories until a future tick.
    const auto stinger_display_trajectory = collide_stinger_display_with_trajectories(
        encounter.trajectories, encounter.stinger_display, campaign.score);
    result.trajectory_stinger_display_collisions = stinger_display_trajectory.collisions;
    result.trajectory_actors_destroyed += stinger_display_trajectory.actors_destroyed;
    result.trajectory_groups_retired += stinger_display_trajectory.groups_retired;
    result.trajectory_destruction_bursts += stinger_display_trajectory.destruction_bursts;
    result.trajectory_score_delta += stinger_display_trajectory.score_delta;
    encounter.encounter_alien_ships_hit +=
        static_cast<std::int32_t>(stinger_display_trajectory.actors_destroyed);
    append_original_explosion_variants(
        result.audio_events, session.original_audio,
        stinger_display_trajectory.explosion_sfx_variant_calls);
    advance_stinger_display(encounter.stinger_display);

    step_rapid_missiles(encounter.rapid_missiles, animation_tick);

    step_enemy_bombs(
        encounter.enemy_bombs,
        animation_tick,
        EnemyBombSteeringContext{
            .player_x = encounter.player.x,
            .redirect_to_attached_probe = enemy_bombs_target_attached_probe(
                campaign.mission.processed_count, encounter.special_weapon.activity),
            .attached_probe_x = encounter.special_weapon.x,
        });

    result.rapid_missiles_retired =
        retire_rapid_missiles_above_top(encounter.rapid_missiles);
    result.enemy_bombs_retired =
        retire_enemy_bombs_below_bottom(encounter.enemy_bombs);

    // Shareware boss updates occur in the original after common projectile
    // movement and before the later global bomb/special/Drone collision region.
    // Keeping native Lid/Top here is important: a boss collision that changes a
    // launched special to terminal state 10 must not be settled until the next
    // gameplay update. Newly spawned boss bombs likewise begin moving later.
    if (encounter.boss.family == BossFamily::LidTop) {
        const auto lid_top_result = step_lid_top_boss(
            encounter.boss.lid_top,
            encounter.gameplay_substep_phase,
            encounter.player.x,
            session.runtime.difficulty,
            session.runtime.demo_playback_mode,
            session.original_random,
            encounter.enemy_bombs,
            encounter.enemy_bomb_spawn_gate,
            encounter.rapid_missiles,
            encounter.special_weapon,
            campaign.score,
            targets.lid_top_sprite_mask);
        result.boss_destruction_transitions += lid_top_result.destruction_transitions;
        result.boss_components_retired += lid_top_result.components_retired;
        result.boss_score_delta += lid_top_result.score_delta;
        result.lid_top_motion_stop_requested = lid_top_result.top_motion_stopped;
        result.lid_top_root_moved = lid_top_result.root_moved;
        result.lid_top_vertical_retreat_started =
            lid_top_result.vertical_retreat_started;
        result.lid_top_enemy_bomb_spawned = lid_top_result.enemy_bomb_spawned;
        result.lid_top_enemy_bomb_spawn_index =
            lid_top_result.enemy_bomb_spawn_index;
        if (lid_top_result.enemy_bomb_spawned) {
            (void)result.audio_events.push({
                drone::audio::AudioCue::EnemyBombFire,
                drone::audio::AudioAction::Play});
        }
        result.lid_top_rapid_missiles_consumed =
            lid_top_result.rapid_missiles_consumed;
        result.lid_top_rapid_top_opaque_collisions =
            lid_top_result.rapid_top_opaque_collisions;
        result.lid_top_rapid_open_collisions =
            lid_top_result.rapid_lid_open_collisions;
        result.lid_top_lid_opened = lid_top_result.lid_opened;
        result.lid_top_lid_close_started = lid_top_result.lid_close_started;
        result.lid_top_special_closed_top_impact =
            lid_top_result.special_closed_top_impact;
        result.lid_top_stinger_core_hit = lid_top_result.stinger_core_hit;
        append_original_explosion_variants(
            result.audio_events, session.original_audio,
            lid_top_result.explosion_sfx_variant_calls);
        if (lid_top_result.destruction_transitions != 0) {
            // The exposed-core Stinger collision stops retro1.wav immediately
            // at Win32 0x00416C1E..0x00416C2A, before lid activity becomes 2.
            // Unlike the closed-top impact branches, this exposed-core kill has
            // no 0x00402900 explosion-variant call; the append above is therefore
            // empty for this transition and the loop stop remains the exact event.
            (void)result.audio_events.push({
                drone::audio::AudioCue::LidTopBossLoop,
                drone::audio::AudioAction::StopAndRewind});
        }
    } else if (encounter.boss.family == BossFamily::Gemini) {
        const auto gemini_result = step_gemini_boss(
            encounter.boss.gemini,
            encounter.gameplay_substep_phase,
            encounter.player.x,
            session.runtime.difficulty,
            session.original_random,
            encounter.enemy_bombs,
            encounter.enemy_bomb_spawn_gate,
            encounter.special_weapon,
            encounter.stinger_display,
            campaign.score,
            targets.gemini_sprite_masks);
        result.boss_destruction_transitions += gemini_result.destruction_transitions;
        result.boss_components_retired += gemini_result.components_retired;
        result.boss_score_delta += gemini_result.score_delta;
        result.gemini_root_moved = gemini_result.root_moved;
        result.gemini_vertical_retreat_started =
            gemini_result.vertical_retreat_started;
        result.gemini_enemy_bomb_spawned = gemini_result.enemy_bomb_spawned;
        result.gemini_enemy_bomb_spawn_index = gemini_result.enemy_bomb_spawn_index;
        if (gemini_result.enemy_bomb_spawned) {
            (void)result.audio_events.push({
                drone::audio::AudioCue::EnemyBombFire,
                drone::audio::AudioAction::Play});
        }
        result.gemini_special_hit_side_a = gemini_result.special_hit_side_a;
        result.gemini_special_hit_side_b = gemini_result.special_hit_side_b;
        result.gemini_special_hit_head = gemini_result.special_hit_head;
        result.gemini_special_hit_body = gemini_result.special_hit_body;
        result.gemini_special_damage = gemini_result.special_damage;
        result.gemini_stinger_display_activated =
            gemini_result.stinger_display_activated;
        append_original_explosion_variants(
            result.audio_events, session.original_audio,
            gemini_result.explosion_sfx_variant_calls);
        if (gemini_result.destruction_transitions != 0 &&
            encounter.boss.gemini.side_a.body_activity != boss_activity_active &&
            encounter.boss.gemini.side_b.body_activity != boss_activity_active) {
            // Each Gemini damage branch stops gemini.wav only when the *other*
            // body is no longer activity 1 (0x00405773..0x00405789 and the
            // mirrored 0x00405C4A..0x00405C6B path). Thus the first destroyed
            // side leaves the loop running and the second transition stops it.
            // As above, the impact explosion calls precede this stop.
            (void)result.audio_events.push({
                drone::audio::AudioCue::GeminiBossLoop,
                drone::audio::AudioAction::StopAndRewind});
        }
    }

    // The original late bomb loop is per-slot, not two independent global
    // passes: Probe/Stinger is tested first and the player second for each bomb.
    // It even retains the same bomb coordinates for the player test after a
    // special hit cleared bomb activity, allowing one overlapping bomb to affect
    // both targets before active_count is decremented once.
    const auto bomb_collision = process_enemy_bomb_late_collision_pass(
        encounter.enemy_bombs,
        encounter.special_weapon,
        encounter.player,
        campaign.player_lifecycle,
        encounter.shield.active,
        encounter.enemy_bomb_spawn_gate);
    const auto& bomb_special_hit = bomb_collision.special_impact;
    drone::audio::append_audio_events(result.audio_events, bomb_collision.audio_events);
    result.enemy_bomb_hit_special_weapon = bomb_special_hit.hit;
    result.enemy_bomb_special_hit_index = bomb_special_hit.bomb_index;
    result.enemy_bomb_probe_decode_reset = bomb_special_hit.probe_decode_reset;
    result.enemy_bomb_probe_phase2_interrupt_signal_requested =
        bomb_special_hit.probe_phase2_interrupt_signal_requested;
    result.enemy_bomb_special_launch_sound_stop_requested =
        bomb_special_hit.launch_sound_stop_requested;
    result.enemy_bomb_probe_impact_effect_requested =
        bomb_special_hit.probe_impact_effect_requested;
    result.enemy_bomb_probe_impact_sound_requested =
        bomb_special_hit.probe_impact_sound_requested;
    result.enemy_bomb_stinger_impact_effect_requested =
        bomb_special_hit.stinger_impact_effect_requested;
    result.enemy_bomb_stinger_impact_sound_requested =
        bomb_special_hit.stinger_impact_sound_requested;
    result.enemy_bomb_player_hits = bomb_collision.player_hits;
    result.enemy_bomb_shield_absorptions = bomb_collision.shield_absorptions;
    result.enemy_bomb_first_player_hit_index = bomb_collision.first_player_hit_index;
    result.enemy_bomb_auto_launched_special =
        bomb_collision.loaded_special_auto_launched;
    result.enemy_bomb_auto_launch_sound_requested =
        bomb_collision.auto_launch_sound_requested;
    result.enemy_bomb_player_hit_sfx_requested =
        bomb_collision.player_hit_sfx_requested;
    result.player_destruction_started = bomb_collision.player_destruction_started;
    result.player_death_effect_requested = bomb_collision.player_death_effect_requested;
    if (bomb_collision.player_destruction_started) {
        // trigger_player_destruction_sequence (0x0041CDF0) occurs in this late
        // collision region, after the earlier phase-2 death-effect update. A new
        // actor therefore begins at frame -6 and cannot advance until a future
        // phase-2 substep.
        trigger_player_death_effect(
            encounter.player_death_effect, encounter.player);
        result.player_death_effect_visible =
            player_death_effect_visible(encounter.player_death_effect);
        result.player_death_effect_frame = encounter.player_death_effect.frame;
    }
    result.player_bomb_spawn_suppression_started =
        bomb_collision.bomb_spawn_suppression_started;
    result.special_launched = result.special_launched ||
        bomb_collision.loaded_special_auto_launched;

    if (bomb_special_hit.stinger_impact_effect_requested) {
        activate_stinger_display_at(
            encounter.stinger_display, encounter.special_weapon.x, encounter.special_weapon.y);
        result.trajectory_stinger_display_activated = true;
    }

    // Win32 enters the late special block only when state == 3 *here*, after
    // bomb collisions. A later Drone collision inside that block may change the
    // activity byte, but the subsequent trajectory scan still executes using
    // the retained special coordinates.
    const bool special_entered_late_block_as_launched =
        encounter.special_weapon.activity == SpecialWeaponActivity::LaunchedHoming;

    // The original rapid-missile pool is checked before the common special
    // projectile. Both Drone interactions use 0x00401F60 point-vs-hitbox, not
    // the opaque-pixel primitive. The first destructive hit changes countdown
    // 100->0, naturally suppressing later Drone hits in the same update.
    const auto rapid_drone_hit = collide_rapid_missiles_with_drone(
        encounter.rapid_missiles,
        encounter.drone);
    result.rapid_missile_hit_drone = rapid_drone_hit.hit;
    result.rapid_missile_drone_hit_index = rapid_drone_hit.missile_index;
    result.drone_destruction_countdown_started =
        result.drone_destruction_countdown_started ||
        rapid_drone_hit.destruction_countdown_started;
    result.drone_weapon_hit_explosion_spawns_requested =
        rapid_drone_hit.explosion_spawns_requested;

    const auto special_drone_hit = collide_special_weapon_with_drone(
        encounter.special_weapon,
        encounter.drone,
        session.runtime.difficulty,
        session.runtime.demo_playback_mode,
        session.original_random,
        campaign.score);
    result.special_weapon_hit_drone = special_drone_hit.hit;
    result.probe_attached_to_drone = special_drone_hit.probe_attached;
    result.stinger_hit_drone = special_drone_hit.stinger_hit;
    result.probe_attachment_score_delta = special_drone_hit.score_delta;
    result.drone_destruction_countdown_started =
        result.drone_destruction_countdown_started ||
        special_drone_hit.destruction_countdown_started;
    result.drone_weapon_hit_explosion_spawns_requested = static_cast<std::uint8_t>(
        result.drone_weapon_hit_explosion_spawns_requested +
        special_drone_hit.explosion_spawns_requested);

    // The launched Probe/Stinger trajectory scan is distinct from both rapid
    // sprite-mask hits and the Stinger-display AoE. It uses point-vs-0.85
    // actor hitboxes and directly destroys every overlapping actor without
    // re-testing special activity after the first collision. These direct kills
    // do not touch the encounter/mission hit counters at this site.
    const auto direct_special_trajectory = collide_launched_special_with_trajectories(
        encounter.trajectories,
        encounter.special_weapon,
        special_entered_late_block_as_launched,
        session.runtime.demo_playback_mode,
        encounter.stinger_display,
        campaign.score);
    result.trajectory_direct_special_collisions = direct_special_trajectory.collisions;
    result.trajectory_stinger_display_activated =
        result.trajectory_stinger_display_activated ||
        direct_special_trajectory.stinger_display_activated;
    result.trajectory_actors_destroyed += direct_special_trajectory.actors_destroyed;
    result.trajectory_groups_retired += direct_special_trajectory.groups_retired;
    result.trajectory_destruction_bursts += direct_special_trajectory.destruction_bursts;
    result.trajectory_score_delta += direct_special_trajectory.score_delta;

    encounter.world_scroll_row = advance_gameplay_world_scroll_row(
        encounter.world_scroll_row,
        encounter.gameplay_substep_phase);

    // Original state 2 converts at most one 500-point threshold per update.
    if (!end_run_transition && consume_one_extra_life_threshold(campaign.score)) {
        ++campaign.player_lifecycle.lives;
        result.extra_life_awarded = true;
    }

    result.encounter_alien_ships_total = encounter.encounter_alien_ships_total;
    result.encounter_alien_ships_hit = encounter.encounter_alien_ships_hit;
    result.mission_alien_ships_total = campaign.alien_ships_total;
    result.mission_alien_ships_hit = campaign.alien_ships_hit;

    ++encounter.gameplay_updates;
    ++session.total_gameplay_updates;

    result.advanced = true;
    result.encounter_update = encounter.gameplay_updates;
    result.total_update = session.total_gameplay_updates;
    result.gameplay_substep_phase = encounter.gameplay_substep_phase;
    result.animation_tick = animation_tick;
    return result;
}

PostGameRuntimeStepResult step_game_session_post_game(
    GameSession& session,
    const PostGameModalInput& input) {
    const auto previous_phase = session.post_game.phase;
    auto result = step_post_game_runtime(session.post_game, input);
    if (session.post_game.plan && result.advanced && result.phase != previous_phase) {
        append_post_game_phase_stop_audio(
            result.audio_events, previous_phase, *session.post_game.plan);
        append_post_game_phase_start_audio(
            result.audio_events, result.phase, *session.post_game.plan);
    }
    synchronize_post_game_raw_state(session);
    return result;
}

} // namespace drone::gameplay
