#include <drone/gameplay/game_session.hpp>

#include <drone/gameplay/gameplay_phase.hpp>

namespace drone::gameplay {

GameSession::GameSession() {
    reset_trajectory_encounter(encounter.trajectories);
    reset_trajectory_spawn_scheduler(
        encounter.trajectory_spawn, runtime.difficulty, campaign.mission.processed_count);
}

void reset_game_session(GameSession& session, const GameplaySessionResetScope scope) {
    if (scope == GameplaySessionResetScope::FullCampaign) {
        session.campaign = GameCampaignState{};
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
    if (session.state != GameState::ActiveGameplay) {
        return result;
    }

    auto& encounter = session.encounter;
    auto& campaign = session.campaign;
    result.encounter_alien_ships_total = encounter.encounter_alien_ships_total;
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
    // update before their producer paths can consume the ready values. The
    // bomb gate is also the exact post-death quiet-period counter.
    advance_enemy_bomb_spawn_gate(encounter.enemy_bomb_spawn_gate);
    advance_rapid_missile_cooldown(encounter.rapid_missiles);

    // Player life consumption is deferred from collision time. The original
    // settles only after the bomb gate rises above -356, the death presentation
    // is inactive, the player is inactive, lives remain, and the Drone is not
    // in destruction activity 2. Presentation inactivity remains an explicit
    // fidelity-host fact until that effect pool is reconstructed.
    const auto player_respawn = settle_player_death(
        campaign.player_lifecycle,
        encounter.player,
        encounter.shield,
        PlayerRespawnGate{
            .bomb_spawn_gate_allows_settlement =
                enemy_bomb_spawn_gate_allows_respawn(encounter.enemy_bomb_spawn_gate),
            .death_effect_inactive = targets.player_death_effect_inactive,
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

    // The loaded special's recovered switch counter advances independently of
    // Down-key input. A Down action loads when inactive, otherwise attempts the
    // threshold-gated Probe/Stinger cycle.
    advance_special_weapon_switch_progress(encounter.special_weapon);
    if (input.special_load_cycle) {
        if (encounter.special_weapon.activity == SpecialWeaponActivity::Inactive) {
            result.special_loaded = load_special_weapon(
                encounter.special_weapon, encounter.player, true, player_active);
            if (result.special_loaded) {
                // Win32 0x0040CD83 resets the shared target pointer to the
                // center-screen dummy object on every successful load.
                reset_stinger_target(encounter.stinger_target);
            }
        } else if (encounter.special_weapon.activity == SpecialWeaponActivity::LoadedTracking) {
            result.special_cycled = toggle_loaded_special_weapon(
                encounter.special_weapon, true, player_active);
        }
    }

    result.special_launched = launch_special_weapon(
        encounter.special_weapon,
        input.special_launch,
        player_active);

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
            } else if (stinger_boss_snapshot.family == BossFamily::LidTop) {
                stinger_context.lid_top_top_active =
                    stinger_boss_snapshot.lid_top.top_activity == boss_activity_active;
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
    result.player_bomb_spawn_suppression_started =
        bomb_collision.bomb_spawn_suppression_started;
    result.special_launched = result.special_launched ||
        bomb_collision.loaded_special_auto_launched;

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

    // Collision detection for trajectory actors still owns their extracted-frame
    // masks. Drone weapon entry producers above are now internal because their
    // canonical paths use the already-recovered point/hitbox primitive. Once a
    // trajectory hit is proven, destruction/score/group teardown belongs to the
    // continuously owned trajectory encounter and is dispatched here.
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

    result.encounter_alien_ships_total = encounter.encounter_alien_ships_total;

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
