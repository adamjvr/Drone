#include <drone/gameplay/rapid_missile.hpp>

#include <algorithm>

namespace drone::gameplay {

void advance_rapid_missile_cooldown(RapidMissilePool& pool) {
    if (pool.fire_cooldown < RapidMissilePool::cooldown_ready) {
        ++pool.fire_cooldown;
    }
}

bool try_fire_rapid_missile(
    RapidMissilePool& pool,
    const PlayerMotionState& player,
    const bool fire_requested,
    const bool player_active) {

    if (!fire_requested || !player_active) {
        return false;
    }
    if (pool.active_count >= static_cast<std::int32_t>(RapidMissilePool::capacity)) {
        return false;
    }
    if (pool.fire_cooldown != RapidMissilePool::cooldown_ready) {
        return false;
    }

    const auto free_it = std::find_if(
        pool.missiles.begin(), pool.missiles.end(),
        [](const RapidMissileState& missile) { return !missile.active; });
    if (free_it == pool.missiles.end()) {
        // This should agree with active_count in a consistent state, but the
        // original searches the entity flags rather than trusting the counter.
        return false;
    }

    free_it->active = true;
    free_it->x = player.x + 11;
    free_it->y = player.y - 3;
    free_it->passed_top_edge = false;
    // The original pool is initialized to frame zero and a reused missile
    // leaves its wrapped 0..2 frame untouched at spawn. Do not forcibly reset
    // frame here; this preserves that observed pool-reuse behavior.

    ++pool.active_count;
    pool.fire_cooldown = 0;
    return true;
}

void step_rapid_missiles(RapidMissilePool& pool, const bool animation_tick) {
    for (auto& missile : pool.missiles) {
        if (!missile.active) {
            continue;
        }

        if (animation_tick) {
            ++missile.frame;
            if (missile.frame == 3) {
                missile.frame = 0;
            }
        }

        missile.y -= 3;
        if (missile.y < 0) {
            missile.passed_top_edge = true;
        }
    }
}

bool deactivate_rapid_missile(RapidMissilePool& pool, const std::size_t index) {
    if (index >= pool.missiles.size() || !pool.missiles[index].active) {
        return false;
    }
    pool.missiles[index].active = false;
    if (pool.active_count > 0) {
        --pool.active_count;
    }
    return true;
}

std::size_t retire_rapid_missiles_above_top(RapidMissilePool& pool) {
    std::size_t retired = 0;
    for (std::size_t i = 0; i < pool.missiles.size(); ++i) {
        if (pool.missiles[i].active && pool.missiles[i].y < -7) {
            if (deactivate_rapid_missile(pool, i)) {
                ++retired;
            }
        }
    }
    return retired;
}

} // namespace drone::gameplay
