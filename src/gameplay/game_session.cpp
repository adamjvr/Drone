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

    // Win32 state 2 advances the shared four-phase scalar near the beginning
    // of each gameplay update. Existing helpers consume phase 2 as their proven
    // slower animation/scroll cadence.
    encounter.gameplay_substep_phase =
        advance_win32_gameplay_substep_phase(encounter.gameplay_substep_phase);
    const bool animation_tick = is_win32_phase2(encounter.gameplay_substep_phase);

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
        (void)pin_attached_probe_to_drone(encounter.special_weapon, targets.drone_x);
    } else if (encounter.special_weapon.activity == SpecialWeaponActivity::LoadedTracking ||
               encounter.special_weapon.activity == SpecialWeaponActivity::LaunchedHoming) {
        std::int32_t target_x = probe_drone_target_x(targets.drone_x);
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
    if (consume_one_extra_life_threshold(campaign.score)) {
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
