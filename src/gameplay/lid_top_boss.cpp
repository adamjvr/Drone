#include <drone/gameplay/lid_top_boss.hpp>

#include <drone/gameplay/collision.hpp>
#include <drone/gameplay/gameplay_phase.hpp>

#include <algorithm>
#include <cstdint>

namespace drone::gameplay {
namespace {

constexpr std::int32_t fixed_one = 1 << 16;
constexpr std::int32_t live_horizontal_acceleration = 0x44c;
constexpr std::int32_t demo_initial_velocity = 0x8000;
constexpr std::int32_t demo_horizontal_speed_cap = 0x12000;
constexpr std::int32_t traversal_reset_fixed_y = -100 * fixed_one;
constexpr std::uint8_t traversal_sound_period = 8;
constexpr std::uint8_t lid_frame_count = 9;

std::int32_t arithmetic_fixed_to_int(const std::int32_t fixed) noexcept {
    if (fixed >= 0) return fixed >> 16;
    const auto magnitude = -static_cast<std::int64_t>(fixed);
    return -static_cast<std::int32_t>((magnitude + 0xffff) >> 16);
}

std::int32_t live_initial_velocity_x(const DifficultyLevel difficulty) noexcept {
    // Win32 0x0041726C..0x00417281 reduces algebraically to 10923*difficulty.
    return 10923 * static_cast<std::int32_t>(difficulty_multiplier(difficulty));
}

std::int32_t live_initial_velocity_y(const DifficultyLevel difficulty) noexcept {
    return 0x4001 + 0x1555 * static_cast<std::int32_t>(difficulty_multiplier(difficulty));
}

std::int32_t live_horizontal_speed_cap(const DifficultyLevel difficulty) noexcept {
    return (static_cast<std::int32_t>(difficulty_multiplier(difficulty)) + 3) * 3 * 0x1000;
}

CollisionEntityView top_view(const LidTopBossLifecycleState& state) noexcept {
    return CollisionEntityView{
        .x = state.root_x,
        .y = state.root_y,
        .sprite_width = lid_top_top_width,
        .sprite_height = lid_top_top_height,
        .hitbox_width = lid_top_top_hitbox_width,
        .hitbox_height = lid_top_top_hitbox_height,
    };
}

CollisionEntityView missile_weakpoint_view(const LidTopBossLifecycleState& state) noexcept {
    return CollisionEntityView{
        .x = state.root_x + 53,
        .y = state.root_y + 23,
        .sprite_width = lid_top_missile_weakpoint_width,
        .sprite_height = lid_top_missile_weakpoint_height,
        .hitbox_width = lid_top_missile_weakpoint_hitbox_width,
        .hitbox_height = lid_top_missile_weakpoint_hitbox_height,
    };
}

CollisionEntityView stinger_core_view(const LidTopBossLifecycleState& state) noexcept {
    return CollisionEntityView{
        .x = state.root_x + 29,
        .y = state.root_y + 32,
        .sprite_width = lid_top_stinger_core_width,
        .sprite_height = lid_top_stinger_core_height,
        .hitbox_width = lid_top_stinger_core_hitbox_width,
        .hitbox_height = lid_top_stinger_core_hitbox_height,
    };
}

std::optional<std::size_t> first_inactive_bomb(const EnemyBombPool& pool) noexcept {
    for (std::size_t i = 0; i < pool.bombs.size(); ++i) {
        if (!pool.bombs[i].active) return i;
    }
    return std::nullopt;
}

void spawn_boss_bomb_preserving_motion(
    EnemyBombPool& pool,
    const std::size_t index,
    const std::int32_t x,
    const std::int32_t y) noexcept {
    auto& bomb = pool.bombs[index];
    bomb.active = true;
    bomb.x = x;
    bomb.y = y;
    // Win32 boss branch deliberately leaves +0x10 and +0x140 untouched.
    bomb.out_of_bounds = y < 0 || y > 190 || x < 0 || x > 319;
    ++pool.active_count;
}

void advance_root_motion(
    LidTopBossLifecycleState& state,
    const std::int32_t player_x,
    LidTopBossStepResult& result) noexcept {
    state.root_fixed_x += state.root_velocity_x;
    state.root_fixed_y += state.root_velocity_y;
    state.root_x = arithmetic_fixed_to_int(state.root_fixed_x);
    state.root_y = arithmetic_fixed_to_int(state.root_fixed_y);
    result.root_moved = true;

    // Win32 compares top.x + width/2 to the player's *left* X coordinate.
    const auto boss_center_x = state.root_x + lid_top_top_width / 2;
    if (boss_center_x < player_x) {
        state.root_velocity_x += live_horizontal_acceleration;
        if (state.root_velocity_x > state.horizontal_speed_cap) {
            state.root_velocity_x = state.horizontal_speed_cap;
        }
    } else {
        state.root_velocity_x -= live_horizontal_acceleration;
        if (state.root_velocity_x < -state.horizontal_speed_cap) {
            state.root_velocity_x = -state.horizontal_speed_cap;
        }
    }

    if (state.root_y >= 240) {
        // Win32 0x00416885..0x004168C5 increments shared byte 0x00454B04,
        // plays level1.wav when it reaches eight, then resets the *fixed Y
        // position* at 0x00446E0C to -100<<16. It does not reverse velocity.
        ++state.traversal_sound_counter;
        if (state.traversal_sound_counter == traversal_sound_period) {
            state.traversal_sound_counter = 0;
            result.level1_cadence_sound_requested = true;
        }
        state.root_fixed_y = traversal_reset_fixed_y;
        result.vertical_traversal_wrapped = true;
    }
}

void maybe_spawn_boss_bomb(
    LidTopBossLifecycleState& state,
    const std::int32_t phase,
    const DifficultyLevel difficulty,
    const bool demo_playback_mode,
    OriginalRandomState& random,
    EnemyBombPool& bombs,
    EnemyBombSpawnGate& gate,
    LidTopBossStepResult& result) noexcept {
    if (demo_playback_mode || !is_win32_phase2(phase)) return;

    // The chance draw is consumed before the spawn-gate/capacity checks.
    const auto chance = original_random_mod(random, 100);
    if (chance >= 2u * difficulty_multiplier(difficulty)) return;
    if (!enemy_bomb_spawn_gate_allows_spawn(gate) ||
        bombs.active_count >= static_cast<std::int32_t>(EnemyBombPool::capacity)) {
        return;
    }

    const auto index = first_inactive_bomb(bombs);
    if (!index.has_value()) return;

    const auto side_roll = original_random_mod(random, 10);
    const auto x = state.root_x + (side_roll < 5 ? 30 : 41);
    const auto y = state.root_y + 53;
    spawn_boss_bomb_preserving_motion(bombs, *index, x, y);
    reset_enemy_bomb_spawn_gate_after_spawn(gate);
    result.enemy_bomb_spawned = true;
    result.enemy_bomb_spawn_index = index;
}

void process_special_collisions(
    LidTopBossLifecycleState& state,
    SpecialWeaponState& special,
    LidTopBossStepResult& result) noexcept {
    if (state.top_activity != boss_activity_active ||
        special.activity != SpecialWeaponActivity::LaunchedHoming) {
        return;
    }

    if (state.lid_activity == lid_top_initial_lid_activity &&
        point_in_hitbox(Point{special.x, special.y}, top_view(state))) {
        // Closed-lid/top collision consumes the launched special but does not
        // alter the lid state.
        special.activity = SpecialWeaponActivity::ImpactConsumed;
        special.motion_y = 0;
        result.special_closed_top_impact = true;
        // Win32 0x00416B11..0x00416BC6: Probe uses one 0x00402900
        // explosion call; Stinger uses two.
        result.explosion_sfx_variant_calls +=
            special.kind == SpecialWeaponKind::Probe ? 1u : 2u;
    }

    // The original does not re-read special activity here. This second branch
    // is gated by lid activity 1, so it cannot follow the state-6 collision in
    // the same call, but retaining the explicit ordering prevents a future
    // cleanup from changing the semantics.
    if (state.lid_activity == boss_activity_active &&
        special.kind == SpecialWeaponKind::Stinger &&
        state.root_y + 5 > 0 &&
        state.lid_frame > 6 &&
        point_in_hitbox(Point{special.x, special.y}, stinger_core_view(state))) {
        state.lid_activity = boss_activity_destruction;
        state.lid_destruction_progress = 0;
        special.activity = SpecialWeaponActivity::ImpactConsumed;
        special.motion_y = 0;
        result.stinger_core_hit = true;
        ++result.destruction_transitions;
    }
}

void advance_lid_destruction(
    LidTopBossLifecycleState& state,
    ScoreState& score,
    LidTopBossStepResult& result) noexcept {
    if (state.lid_activity != boss_activity_destruction) return;

    ++state.lid_destruction_progress;
    if (state.lid_destruction_progress != lid_top_lid_destruction_updates) return;

    apply_score_delta(score, canonical_shareware_boss_score_award);
    result.score_delta += canonical_shareware_boss_score_award;
    state.lid_activity = boss_activity_inactive;
    state.top_activity = boss_activity_destruction;
    state.top_destruction_progress = 0;
    state.root_velocity_x = 0;
    state.root_velocity_y = 0;
    result.top_motion_stopped = true;
    ++result.components_retired;
}

void process_rapid_missiles(
    LidTopBossLifecycleState& state,
    RapidMissilePool& pool,
    const LidTopBossSpriteMaskView* sprite_mask,
    LidTopBossStepResult& result) noexcept {
    const auto weakpoint = missile_weakpoint_view(state);
    const auto top = top_view(state);
    const bool have_top_mask = sprite_mask != nullptr &&
        sprite_mask->top_frame.size() >=
            static_cast<std::size_t>(lid_top_top_width * lid_top_top_height);

    for (auto& missile : pool.missiles) {
        if (!missile.active) continue;
        const bool active_at_entry = true;

        // Win32 pre-checks only missile.x < top.x + top.width/2 + 5 before
        // calling the opaque-pixel primitive. A top hit clears activity, but
        // the later lid weak-point test still runs on the retained coordinates.
        if (have_top_mask &&
            missile.x < state.root_x + lid_top_top_width / 2 + 5 &&
            point_hits_opaque_pixel(Point{missile.x, missile.y}, top, sprite_mask->top_frame)) {
            missile.active = false;
            ++result.rapid_top_opaque_collisions;
            ++result.explosion_sfx_variant_calls;
        }

        if (state.lid_activity == lid_top_initial_lid_activity &&
            state.lid_frame == 0 &&
            point_in_hitbox(Point{missile.x, missile.y}, weakpoint)) {
            missile.active = false;
            state.lid_activity = boss_activity_active;
            result.lid_opened = true;
            ++result.rapid_lid_open_collisions;
            ++result.explosion_sfx_variant_calls;
        }

        if (active_at_entry && !missile.active) {
            if (pool.active_count > 0) --pool.active_count;
            ++result.rapid_missiles_consumed;
        }
    }
}

void advance_lid_animation(
    LidTopBossLifecycleState& state,
    const std::int32_t phase,
    const DifficultyLevel difficulty,
    const bool demo_playback_mode,
    OriginalRandomState& random,
    LidTopBossStepResult& result) noexcept {
    if (!is_win32_phase2(phase)) return;

    if (state.lid_activity == boss_activity_active) {
        ++state.lid_frame;
        if (state.lid_frame == lid_frame_count) {
            state.lid_frame = 8;
            if (!demo_playback_mode &&
                original_random_mod(random, 200) < difficulty_multiplier(difficulty)) {
                state.lid_activity = lid_top_initial_lid_activity;
                result.lid_close_started = true;
            }
        }
    }

    // Deliberately a second `if`, not `else if`: a close transition above falls
    // through in the original and decrements frame 8 -> 7 on the same update.
    if (state.lid_activity == lid_top_initial_lid_activity) {
        const auto next = static_cast<std::uint8_t>(state.lid_frame - 1u);
        state.lid_frame = next == 0xffu ? 0u : next;
    }
}

} // namespace

void initialize_lid_top_boss_runtime(
    LidTopBossLifecycleState& state,
    const DifficultyLevel difficulty,
    const bool demo_playback_mode) noexcept {
    state.top_activity = boss_activity_active;
    state.lid_activity = lid_top_initial_lid_activity;
    state.lid_destruction_progress = 0;
    state.top_destruction_progress = 0;
    state.lid_frame = 0;
    state.root_x = 0;
    state.root_y = -100;
    state.root_fixed_x = 0;
    state.root_fixed_y = -100 * fixed_one;
    state.root_velocity_x = demo_playback_mode
        ? demo_initial_velocity
        : live_initial_velocity_x(difficulty);
    state.root_velocity_y = demo_playback_mode
        ? demo_initial_velocity
        : live_initial_velocity_y(difficulty);
    state.horizontal_speed_cap = demo_playback_mode
        ? demo_horizontal_speed_cap
        : live_horizontal_speed_cap(difficulty);
    state.runtime_initialized = true;
}

LidTopBossStepResult step_lid_top_boss(
    LidTopBossLifecycleState& state,
    const std::int32_t gameplay_substep_phase,
    const std::int32_t player_x,
    const DifficultyLevel difficulty,
    const bool demo_playback_mode,
    OriginalRandomState& random,
    EnemyBombPool& enemy_bombs,
    EnemyBombSpawnGate& bomb_spawn_gate,
    RapidMissilePool& rapid_missiles,
    SpecialWeaponState& special,
    ScoreState& score,
    const LidTopBossSpriteMaskView* sprite_mask) noexcept {
    LidTopBossStepResult result{};
    if (!state.runtime_initialized || state.top_activity == boss_activity_inactive) {
        return result;
    }

    if (state.top_activity == boss_activity_active) {
        advance_root_motion(state, player_x, result);
        maybe_spawn_boss_bomb(
            state, gameplay_substep_phase, difficulty, demo_playback_mode,
            random, enemy_bombs, bomb_spawn_gate, result);
    }

    // The top/root destruction retirement block occurs before the special/lid
    // collision block. If it reaches 30 here the remainder of this already-
    // entered update still executes, matching the original function body.
    if (is_win32_phase2(gameplay_substep_phase) &&
        state.top_activity == boss_activity_destruction) {
        ++state.top_destruction_progress;
        if (state.top_destruction_progress == lid_top_top_destruction_phase2_ticks) {
            state.top_activity = boss_activity_inactive;
            ++result.components_retired;
        }
    }

    process_special_collisions(state, special, result);
    advance_lid_destruction(state, score, result);
    process_rapid_missiles(state, rapid_missiles, sprite_mask, result);
    advance_lid_animation(
        state, gameplay_substep_phase, difficulty, demo_playback_mode, random, result);

    return result;
}

} // namespace drone::gameplay
