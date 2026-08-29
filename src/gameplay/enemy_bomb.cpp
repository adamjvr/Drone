#include <drone/gameplay/enemy_bomb.hpp>

#include <drone/gameplay/collision.hpp>

#include <algorithm>

namespace drone::gameplay {
namespace {

EnemyBombState* first_inactive(EnemyBombPool& pool) {
    const auto it = std::find_if(
        pool.bombs.begin(), pool.bombs.end(),
        [](const EnemyBombState& bomb) { return !bomb.active; });
    return it == pool.bombs.end() ? nullptr : &*it;
}

void mark_spawned(
    EnemyBombPool& pool,
    EnemyBombState& bomb,
    const std::int32_t x,
    const std::int32_t y,
    const std::int32_t horizontal_step) {

    bomb.active = true;
    bomb.x = x;
    bomb.y = y;
    bomb.horizontal_step = horizontal_step;
    bomb.out_of_bounds = (y < 0 || y > 190 || x < 0 || x > 319);
    // Current frame is intentionally preserved. The original pool reuses the
    // entity without resetting +0x140 on each spawn.
    ++pool.active_count;
}

EnemyBombSpecialImpactResult apply_special_impact_without_count_change(
    EnemyBombState& bomb,
    const std::size_t index,
    SpecialWeaponState& special) noexcept {

    EnemyBombSpecialImpactResult result{};
    result.hit = true;
    result.bomb_index = index;
    result.previous_activity = special.activity;
    result.kind = special.kind;
    result.launch_sound_stop_requested =
        special.activity == SpecialWeaponActivity::LaunchedHoming;

    bomb.active = false;

    auto& decode = special.probe_decode;
    if (decode.status != ProbeDecodeStatus::Complete &&
        decode.phase2_elapsed > 0 &&
        special.activity == SpecialWeaponActivity::ProbeAttachedDecoding) {
        decode.status = ProbeDecodeStatus::Phase1Decoding;
        decode.phase1_elapsed = 0;
        decode.phase2_elapsed = 0;
        result.probe_decode_reset = true;
        result.probe_phase2_interrupt_signal_requested = true;
    }

    special.activity = SpecialWeaponActivity::Inactive;
    special.motion_y = 1;

    if (special.kind == SpecialWeaponKind::Stinger) {
        special.motion_x = 0;
        special.motion_y = 0;
        result.stinger_impact_effect_requested = true;
        result.stinger_impact_sound_requested = true;
    } else {
        result.probe_impact_effect_requested = true;
        result.probe_impact_sound_requested = true;
    }
    return result;
}

} // namespace


void advance_enemy_bomb_spawn_gate(EnemyBombSpawnGate& gate) {
    if (gate.counter < enemy_bomb_spawn_gate_ready) {
        ++gate.counter;
    }
}

bool enemy_bomb_spawn_gate_allows_spawn(const EnemyBombSpawnGate& gate) {
    return gate.counter == enemy_bomb_spawn_gate_ready;
}

void reset_enemy_bomb_spawn_gate_after_spawn(EnemyBombSpawnGate& gate) {
    gate.counter = 0;
}

void suppress_enemy_bomb_spawns_for_player_destruction(
    EnemyBombSpawnGate& gate,
    const std::int32_t death_effect_terminal_frame) {
    // 0x0041CDF0 computes -20 * signed(0x00491E21). Canonical +0x...21
    // is 27, producing -540. The saturated state-2 increment then supplies
    // both a bomb-suppression quiet period and the later respawn gate.
    gate.counter = -20 * death_effect_terminal_frame;
}

bool enemy_bomb_spawn_gate_allows_respawn(const EnemyBombSpawnGate& gate) {
    return gate.counter > player_respawn_bomb_gate_threshold;
}

bool spawn_live_enemy_bomb(
    EnemyBombPool& pool,
    const std::int32_t x,
    const std::int32_t y,
    const std::int32_t horizontal_step) {

    if (pool.active_count >= static_cast<std::int32_t>(EnemyBombPool::capacity)) {
        return false;
    }
    auto* bomb = first_inactive(pool);
    if (bomb == nullptr) {
        return false;
    }
    mark_spawned(pool, *bomb, x, y, horizontal_step);
    return true;
}

bool spawn_replay_enemy_bomb(
    EnemyBombPool& pool,
    const std::int32_t x,
    const std::int32_t y) {

    if (pool.active_count >= static_cast<std::int32_t>(EnemyBombPool::capacity)) {
        return false;
    }
    auto* bomb = first_inactive(pool);
    if (bomb == nullptr) {
        return false;
    }
    // Win32 playback path 0x0040D9FF explicitly writes zero to +0x10.
    mark_spawned(pool, *bomb, x, y, 0);
    return true;
}

void step_enemy_bombs(
    EnemyBombPool& pool,
    const bool animation_tick,
    const EnemyBombSteeringContext& context) {

    const std::int32_t target_x = context.redirect_to_attached_probe
        ? context.attached_probe_x + 1
        : context.player_x + 17;

    for (auto& bomb : pool.bombs) {
        if (!bomb.active) {
            continue;
        }

        if (animation_tick) {
            ++bomb.frame;
            if (bomb.frame == 3) {
                bomb.frame = 0;
            }
        }

        if (bomb.x < target_x) {
            bomb.x += bomb.horizontal_step;
        }
        if (bomb.x > target_x) {
            bomb.x -= bomb.horizontal_step;
        }

        bomb.y += 2;
        bomb.out_of_bounds =
            (bomb.y < 0 || bomb.y > 190 || bomb.x < 0 || bomb.x > 319);
    }
}

bool deactivate_enemy_bomb(EnemyBombPool& pool, const std::size_t index) {
    if (index >= pool.bombs.size() || !pool.bombs[index].active) {
        return false;
    }
    pool.bombs[index].active = false;
    if (pool.active_count > 0) {
        --pool.active_count;
    }
    return true;
}


EnemyBombSpecialImpactResult collide_enemy_bombs_with_special_weapon(
    EnemyBombPool& pool,
    SpecialWeaponState& special) noexcept {

    EnemyBombSpecialImpactResult result{};
    if (special.activity == SpecialWeaponActivity::Inactive) {
        return result;
    }

    const CollisionEntityView special_hitbox{
        .x = special.x,
        .y = special.y,
        .sprite_width = 3,
        .sprite_height = 8,
        .hitbox_width = canonical_special_weapon_collision_width_extent,
        .hitbox_height = canonical_special_weapon_collision_height_extent,
    };

    for (std::size_t index = 0; index < pool.bombs.size(); ++index) {
        auto& bomb = pool.bombs[index];
        if (!bomb.active) continue;

        if (!point_plus_y9_in_hitbox(Point{bomb.x, bomb.y}, special_hitbox)) {
            continue;
        }

        result = apply_special_impact_without_count_change(bomb, index, special);
        if (pool.active_count > 0) --pool.active_count;
        return result;
    }

    return result;
}

EnemyBombPlayerImpactResult resolve_enemy_bomb_player_impact(
    EnemyBombPool& pool,
    const std::size_t index,
    const bool player_shield_active,
    const bool special_weapon_loaded) {

    EnemyBombPlayerImpactResult result;
    if (index >= pool.bombs.size() || !pool.bombs[index].active) {
        return result;
    }

    // Original 0x0040F4E4 clears +0x142 before branching on shield state.
    result.bomb_deactivated = deactivate_enemy_bomb(pool, index);

    if (player_shield_active) {
        // The original zeros +0x10/+0x14 before passing the now-inactive bomb
        // to spawn_mini_explosion_sprite(). EnemyBombState exposes only the
        // established horizontal component; the returned event tells the host
        // to use zero inherited motion for the effect rather than inventing a
        // permanent bomb-field semantic for +0x14.
        pool.bombs[index].horizontal_step = 0;
        result.shield_absorbed = true;
        result.spawn_absorption_effect = true;
        return result;
    }

    result.destroy_player = true;
    result.play_player_hit_sfx = true; // 20-voice bigexp3.wav pool @ 0x4603A8
    result.launch_loaded_special = special_weapon_loaded;
    return result;
}


EnemyBombLateCollisionPassResult process_enemy_bomb_late_collision_pass(
    EnemyBombPool& pool,
    SpecialWeaponState& special,
    const PlayerMotionState& player,
    PlayerLifecycleState& lifecycle,
    const bool player_shield_active,
    EnemyBombSpawnGate& spawn_gate) noexcept {

    EnemyBombLateCollisionPassResult result{};
    const CollisionEntityView player_hitbox{
        .x = player.x,
        .y = player.y,
        .sprite_width = player_sprite_width,
        .sprite_height = player_sprite_height,
        .hitbox_width = player_collision_width_extent,
        .hitbox_height = player_collision_height_extent,
    };

    for (std::size_t index = 0; index < pool.bombs.size(); ++index) {
        auto& bomb = pool.bombs[index];
        if (!bomb.active) continue;

        // The original remembers that this slot entered the late collision loop
        // active and decrements active_count only once after both target tests.
        const bool active_at_loop_entry = true;

        if (special.activity != SpecialWeaponActivity::Inactive) {
            const CollisionEntityView special_hitbox{
                .x = special.x,
                .y = special.y,
                .sprite_width = 3,
                .sprite_height = 8,
                .hitbox_width = canonical_special_weapon_collision_width_extent,
                .hitbox_height = canonical_special_weapon_collision_height_extent,
            };
            if (point_plus_y9_in_hitbox(Point{bomb.x, bomb.y}, special_hitbox)) {
                const auto impact = apply_special_impact_without_count_change(
                    bomb, index, special);
                if (!result.special_impact.hit) result.special_impact = impact;

                // Win32 0x0040F388..0x0040F4B8 performs these audio calls
                // inside the same bomb slot before falling through to the
                // player test. State 3 first stops/rewinds probe3.wav, then the
                // impact cue is selected by Probe/Stinger kind.
                if (impact.launch_sound_stop_requested) {
                    (void)result.audio_events.push({
                        drone::audio::AudioCue::SpecialLaunch,
                        drone::audio::AudioAction::StopAndRewind});
                }
                if (impact.probe_phase2_interrupt_signal_requested) {
                    // Win32 0x0040F3C8 restores drone.wav to volume 80 before
                    // clearing decoder status/counters and before Probe impact SFX.
                    (void)result.audio_events.push({
                        drone::audio::AudioCue::DroneApproachLoop,
                        drone::audio::AudioAction::SetVolume,
                        drone::audio::original_drone_loop_interrupted_decode_volume});
                }
                (void)result.audio_events.push({
                    impact.kind == SpecialWeaponKind::Stinger
                        ? drone::audio::AudioCue::StingerImpact
                        : drone::audio::AudioCue::ProbeImpact,
                    drone::audio::AudioAction::Play});
            }
        }

        // Fidelity quirk: do not re-check bomb.activity here. Win32 falls
        // through from the special-hit branch and tests the same coordinates
        // against the active player even though +0x142 has already been cleared.
        if (lifecycle.player_active &&
            point_plus_y9_in_hitbox(Point{bomb.x, bomb.y}, player_hitbox)) {
            ++result.player_hits;
            if (!result.first_player_hit_index.has_value()) {
                result.first_player_hit_index = index;
            }
            bomb.active = false;

            if (player_shield_active) {
                bomb.horizontal_step = 0;
                ++result.shield_absorptions;
            } else {
                if (special.activity == SpecialWeaponActivity::LoadedTracking) {
                    special.activity = SpecialWeaponActivity::LaunchedHoming;
                    result.loaded_special_auto_launched = true;
                    result.auto_launch_sound_requested = true;
                    (void)result.audio_events.push({
                        drone::audio::AudioCue::SpecialLaunch,
                        drone::audio::AudioAction::Play});
                }
                result.player_hit_sfx_requested = true;
                (void)result.audio_events.push({
                    drone::audio::AudioCue::PlayerHitExplosion,
                    drone::audio::AudioAction::Play});
                result.player_destruction_started = true;
                result.player_death_effect_requested = true;
                lifecycle.player_active = false;
                suppress_enemy_bomb_spawns_for_player_destruction(spawn_gate);
                result.bomb_spawn_suppression_started = true;
            }
        }

        if (active_at_loop_entry && !bomb.active && pool.active_count > 0) {
            --pool.active_count;
        }
    }

    return result;
}

std::size_t retire_enemy_bombs_below_bottom(EnemyBombPool& pool) {
    std::size_t retired = 0;
    for (std::size_t i = 0; i < pool.bombs.size(); ++i) {
        if (pool.bombs[i].active && pool.bombs[i].y > 198) {
            if (deactivate_enemy_bomb(pool, i)) {
                ++retired;
            }
        }
    }
    return retired;
}

} // namespace drone::gameplay
