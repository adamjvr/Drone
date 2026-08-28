#include <drone/gameplay/enemy_bomb.hpp>

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
