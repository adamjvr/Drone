#include <drone/gameplay/game_session.hpp>

#include <drone/gameplay/gameplay_phase.hpp>

namespace drone::gameplay {

GameSession::GameSession() {
    reset_trajectory_encounter(encounter.trajectories);
}

void reset_game_session(GameSession& session, const GameplaySessionResetScope scope) {
    if (scope == GameplaySessionResetScope::FullCampaign) {
        session.campaign = GameCampaignState{};
        session.total_gameplay_updates = 0;
    }

    session.encounter = GameEncounterState{};
    reset_trajectory_encounter(session.encounter.trajectories);
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
    if (session.state != GameState::ActiveGameplay) {
        return result;
    }

    auto& encounter = session.encounter;
    auto& campaign = session.campaign;
    bool end_run_transition = false;

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
                result.encounter_transition = win32_post_drone_transition_plan(
                    result.mission_interstitial->processed_count,
                    result.mission_interstitial->detonated_count);
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

    // The recovered state-2 formation producer runs before trajectory updates.
    // This milestone keeps random/template selection external but owns the
    // actual mode-2 activation and all subsequent group/actor lifecycle here.
    if (targets.trajectory_paths != nullptr && targets.trajectory_spawn_group.has_value()) {
        result.trajectory_group_spawned = activate_transient_trajectory_group(
            encounter.trajectories,
            *targets.trajectory_spawn_group,
            *targets.trajectory_paths,
            targets.trajectory_spawn_x_offset,
            targets.trajectory_spawn_y_offset);
    }

    if (targets.trajectory_paths != nullptr) {
        const auto trajectory_result = advance_trajectory_encounter(
            encounter.trajectories,
            *targets.trajectory_paths,
            encounter.gameplay_substep_phase,
            campaign.score);
        result.trajectory_actors_activated = trajectory_result.actors_activated;
        result.trajectory_actors_escaped = trajectory_result.actors_escaped;
        result.trajectory_groups_retired += trajectory_result.groups_retired;
        result.trajectory_score_delta += trajectory_result.escape_score_delta;
    }

    // update_drone_detonation_effect is called after trajectory processing and
    // before the normal Drone/boss region. The portable core publishes only its
    // proven logical events; random explosion placement and framebuffer
    // distortion remain fidelity/presentation responsibilities.
    const auto detonation_effect_result = step_drone_detonation_effect_logic(
        encounter.drone,
        encounter.gameplay_substep_phase);
    result.drone_detonation_effect_tick = detonation_effect_result.logical_effect_tick;
    result.drone_detonation_explosion_spawns_requested =
        detonation_effect_result.explosion_spawns_requested;
    result.drone_detonation_settlement_reset = detonation_effect_result.settlement_reset;
    result.drone_detonation_settlement_advanced =
        detonation_effect_result.settlement_advanced;

    // Probe decode timing is still owned by the special-weapon reconstruction,
    // but once completion is proven the Drone objective itself is session state.
    if (targets.drone_disarm_completed) {
        result.drone_disarm_completion_accepted =
            mark_drone_disarm_completed(encounter.drone);
    }

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

    if (drone_result.resolution_transition_ready) {
        result.mission_interstitial = mission_interstitial_plan(campaign.mission);
        if (result.mission_interstitial.has_value()) {
            result.encounter_transition = win32_post_drone_transition_plan(
                result.mission_interstitial->processed_count,
                result.mission_interstitial->detonated_count);
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

    // Boss selection no longer consumes an external boundary event: the owned
    // Drone path emits exact Y == -200 after its phase-2 movement. Registered-
    // only dispatch slots are still rejected by the shareware boss owner.
    if (drone_result.boss_approach_boundary_reached) {
        result.boss_activated = activate_shareware_boss_for_processed_drones(
            encounter.boss,
            campaign.mission.processed_count);
        if (result.boss_activated) {
            result.boss_activated_family = encounter.boss.family;
        }
    }

    const auto boss_result = step_shareware_boss_encounter(
        encounter.boss,
        encounter.gameplay_substep_phase,
        targets.boss_destruction_triggers,
        campaign.score);
    result.boss_destruction_transitions = boss_result.destruction_transitions;
    result.boss_components_retired = boss_result.components_retired;
    result.boss_score_delta = boss_result.score_delta;
    result.lid_top_motion_stop_requested = boss_result.lid_top_motion_stop_requested;

    // Both recovered cooldown/gate scalars advance once per active state-2
    // update before their producer paths can consume the ready values.
    advance_enemy_bomb_spawn_gate(encounter.enemy_bomb_spawn_gate);
    advance_rapid_missile_cooldown(encounter.rapid_missiles);

    const bool player_active = campaign.player_lifecycle.player_active;

    // Physical input has already converged to the portable semantic frame.
    step_player_directional_motion(encounter.player, input.movement, animation_tick);

    result.rapid_missile_fired = try_fire_rapid_missile(
        encounter.rapid_missiles,
        encounter.player,
        input.rapid_fire,
        player_active);

    // The loaded special's recovered switch counter advances independently of
    // Down-key input. A Down action loads when inactive, otherwise attempts the
    // threshold-gated Probe/Stinger cycle.
    advance_special_weapon_switch_progress(encounter.special_weapon);
    if (input.special_load_cycle) {
        if (encounter.special_weapon.activity == SpecialWeaponActivity::Inactive) {
            result.special_loaded = load_special_weapon(
                encounter.special_weapon, encounter.player, true, player_active);
        } else if (encounter.special_weapon.activity == SpecialWeaponActivity::LoadedTracking) {
            result.special_cycled = toggle_loaded_special_weapon(
                encounter.special_weapon, true, player_active);
        }
    }

    result.special_launched = launch_special_weapon(
        encounter.special_weapon,
        input.special_launch,
        player_active);

    // Common special movement consumes already-selected target geometry. The
    // target-selection/encounter producer remains outside this first session
    // milestone rather than inventing enemy-selection semantics.
    if (encounter.special_weapon.activity == SpecialWeaponActivity::ProbeAttachedDecoding) {
        (void)pin_attached_probe_to_drone(encounter.special_weapon, encounter.drone.x);
    } else if (encounter.special_weapon.activity == SpecialWeaponActivity::LoadedTracking ||
               encounter.special_weapon.activity == SpecialWeaponActivity::LaunchedHoming) {
        std::int32_t target_x = probe_drone_target_x(encounter.drone.x);
        if (encounter.special_weapon.kind == SpecialWeaponKind::Stinger &&
            targets.stinger_target.has_value()) {
            target_x = stinger_target_x(
                targets.stinger_target->x,
                targets.stinger_target->width);
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

    step_rapid_missiles(encounter.rapid_missiles, animation_tick);

    const bool attached_probe =
        encounter.special_weapon.activity == SpecialWeaponActivity::ProbeAttachedDecoding;
    step_enemy_bombs(
        encounter.enemy_bombs,
        animation_tick,
        EnemyBombSteeringContext{
            .player_x = encounter.player.x,
            .redirect_to_attached_probe =
                attached_probe && targets.redirect_bombs_to_attached_probe,
            .attached_probe_x = encounter.special_weapon.x,
        });

    result.rapid_missiles_retired =
        retire_rapid_missiles_above_top(encounter.rapid_missiles);
    result.enemy_bombs_retired =
        retire_enemy_bombs_below_bottom(encounter.enemy_bombs);

    // Collision detection itself still owns the original extracted-frame mask.
    // Once a hit is proven, destruction/score/group teardown belongs to the
    // continuously owned trajectory encounter and is dispatched here. Exact
    // rapid/special collision producers that can start the Drone destruction
    // countdown remain a later collision milestone; timeout now owns the same
    // internal countdown transition without approximating those masks.
    for (const auto& hit : targets.trajectory_hits) {
        const auto hit_result = apply_trajectory_hit(encounter.trajectories, hit, campaign.score);
        if (!hit_result.destroyed) continue;
        ++result.trajectory_actors_destroyed;
        result.trajectory_destruction_bursts += hit_result.destruction_burst_count;
        result.trajectory_score_delta += hit_result.score_delta;
        if (hit_result.group_retired) ++result.trajectory_groups_retired;
    }

    encounter.world_scroll_row = advance_gameplay_world_scroll_row(
        encounter.world_scroll_row,
        encounter.gameplay_substep_phase);

    // Original state 2 converts at most one 500-point threshold per update.
    if (!end_run_transition && consume_one_extra_life_threshold(campaign.score)) {
        ++campaign.player_lifecycle.lives;
        result.extra_life_awarded = true;
    }

    ++encounter.gameplay_updates;
    ++session.total_gameplay_updates;

    result.advanced = true;
    result.encounter_update = encounter.gameplay_updates;
    result.total_update = session.total_gameplay_updates;
    result.gameplay_substep_phase = encounter.gameplay_substep_phase;
    result.animation_tick = animation_tick;
    return result;
}

} // namespace drone::gameplay
