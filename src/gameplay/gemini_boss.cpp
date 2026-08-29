#include <drone/gameplay/gemini_boss.hpp>

#include <drone/gameplay/collision.hpp>
#include <drone/gameplay/gameplay_phase.hpp>

#include <algorithm>
#include <cstdint>

namespace drone::gameplay {
namespace {

constexpr std::int32_t fixed_one = 1 << 16;
constexpr std::int32_t live_horizontal_acceleration = 0x44c;
constexpr std::int32_t traversal_reset_fixed_y = -100 * fixed_one;
constexpr std::uint8_t traversal_sound_period = 8;
constexpr std::int32_t side_b_body_x_offset = 170;
constexpr std::int32_t head_x_offset = 6;
constexpr std::int32_t head_y_offset = 41;
constexpr std::uint8_t probe_damage = 3;
constexpr std::uint8_t stinger_damage = 15;

std::int32_t arithmetic_fixed_to_int(const std::int32_t fixed) noexcept {
    if (fixed >= 0) return fixed >> 16;
    const auto magnitude = -static_cast<std::int64_t>(fixed);
    return -static_cast<std::int32_t>((magnitude + 0xffff) >> 16);
}

std::int32_t live_initial_velocity_x(const DifficultyLevel difficulty) noexcept {
    return 10923 * static_cast<std::int32_t>(difficulty_multiplier(difficulty));
}

std::int32_t live_initial_velocity_y(const DifficultyLevel difficulty) noexcept {
    return 0x4001 + 0x1555 * static_cast<std::int32_t>(difficulty_multiplier(difficulty));
}

std::int32_t live_horizontal_speed_cap(const DifficultyLevel difficulty) noexcept {
    return (static_cast<std::int32_t>(difficulty_multiplier(difficulty)) + 3) * 3 * 0x1000;
}

std::uint8_t side_a_damage_threshold(const DifficultyLevel difficulty) noexcept {
    return difficulty == DifficultyLevel::Beginner ? 20u : 35u;
}

std::uint8_t side_b_damage_threshold(const DifficultyLevel difficulty) noexcept {
    return difficulty == DifficultyLevel::Beginner ? 20u : 30u;
}

void update_derived_geometry(GeminiBossLifecycleState& state) noexcept {
    const auto root_x = arithmetic_fixed_to_int(state.root_fixed_x);
    const auto root_y = arithmetic_fixed_to_int(state.root_fixed_y);

    state.side_a.body_x = root_x;
    state.side_a.body_y = root_y;
    state.side_a.head_x = root_x + head_x_offset;
    state.side_a.head_y = root_y + head_y_offset;

    state.side_b.body_x = root_x + side_b_body_x_offset;
    state.side_b.body_y = root_y;
    state.side_b.head_x = state.side_b.body_x + head_x_offset;
    state.side_b.head_y = root_y + head_y_offset;
}

CollisionEntityView body_view(const GeminiBossSideLifecycleState& side) noexcept {
    return CollisionEntityView{
        .x = side.body_x,
        .y = side.body_y,
        .sprite_width = gemini_body_width,
        .sprite_height = gemini_body_height,
        .hitbox_width = static_cast<std::int16_t>((gemini_body_width * 85) / 100),
        .hitbox_height = static_cast<std::int16_t>((gemini_body_height * 85) / 100),
    };
}

CollisionEntityView head_view(const GeminiBossSideLifecycleState& side) noexcept {
    return CollisionEntityView{
        .x = side.head_x,
        .y = side.head_y,
        .sprite_width = gemini_head_width,
        .sprite_height = gemini_head_height,
        .hitbox_width = static_cast<std::int16_t>((gemini_head_width * 85) / 100),
        .hitbox_height = static_cast<std::int16_t>((gemini_head_height * 85) / 100),
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
    // Gemini's boss branch, like Lid/Top, does not initialize +0x10/+0x140.
    bomb.out_of_bounds = y < 0 || y > 190 || x < 0 || x > 319;
    ++pool.active_count;
}

void advance_root_motion(
    GeminiBossLifecycleState& state,
    const std::int32_t player_x,
    GeminiBossStepResult& result) noexcept {
    state.root_fixed_x += state.root_velocity_x;
    state.root_fixed_y += state.root_velocity_y;
    update_derived_geometry(state);
    result.root_moved = true;

    // Win32 compares the midpoint between the two 43x34 head entities against
    // player left X, then accelerates the shared root by +/-0x44C.
    const auto midpoint_x = (state.side_a.head_x + state.side_b.head_x) / 2;
    if (midpoint_x < player_x) {
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

    if (state.side_a.body_y >= 240) {
        // Win32 0x004050BA..0x004050F9 mirrors Lid/Top: increment the shared
        // eight-pass sound byte, emit level2.wav on the eighth crossing, then
        // reset fixed Y at 0x00467544 to -100<<16 without changing velocity.
        ++state.traversal_sound_counter;
        if (state.traversal_sound_counter == traversal_sound_period) {
            state.traversal_sound_counter = 0;
            result.level2_cadence_sound_requested = true;
        }
        state.root_fixed_y = traversal_reset_fixed_y;
        result.vertical_traversal_wrapped = true;
    }
}

void advance_body_animation(
    GeminiBossLifecycleState& state,
    const std::int32_t phase) noexcept {
    if (!is_win32_phase2(phase)) return;

    auto& a = state.side_a;
    a.body_frame = static_cast<std::uint8_t>(a.body_frame + 1u);
    if (a.body_frame == a.body_frame_count) a.body_frame = 0;

    auto& b = state.side_b;
    if (b.body_frame == 0) {
        b.body_frame = static_cast<std::uint8_t>(b.body_frame_count - 1u);
    } else {
        --b.body_frame;
    }
}

void maybe_spawn_boss_bomb(
    GeminiBossLifecycleState& state,
    const std::int32_t phase,
    const DifficultyLevel difficulty,
    OriginalRandomState& random,
    EnemyBombPool& bombs,
    EnemyBombSpawnGate& gate,
    GeminiBossStepResult& result) noexcept {
    if (!is_win32_phase2(phase)) return;

    // The chance draw happens before activity/gate/capacity rejection.
    const auto chance = original_random_mod(random, 100);
    if (chance >= 2u * difficulty_multiplier(difficulty)) return;
    if (state.side_a.body_activity != boss_activity_active &&
        state.side_b.body_activity != boss_activity_active) {
        return;
    }
    if (!enemy_bomb_spawn_gate_allows_spawn(gate) ||
        bombs.active_count >= static_cast<std::int32_t>(EnemyBombPool::capacity)) {
        return;
    }

    const auto index = first_inactive_bomb(bombs);
    if (!index.has_value()) return;

    const auto side_roll = original_random_mod(random, 10);
    const GeminiBossSideLifecycleState* source = nullptr;
    if (side_roll < 5) {
        source = state.side_a.body_activity == boss_activity_active
            ? &state.side_a : &state.side_b;
    } else {
        source = state.side_b.body_activity == boss_activity_active
            ? &state.side_b : &state.side_a;
    }

    spawn_boss_bomb_preserving_motion(
        bombs, *index, source->head_x + 21, source->head_y + 33);
    reset_enemy_bomb_spawn_gate_after_spawn(gate);
    result.enemy_bomb_spawned = true;
    result.enemy_bomb_spawn_index = index;
}

void advance_destruction_side(
    GeminiBossSideLifecycleState& side,
    const bool phase2,
    GeminiBossStepResult& result) noexcept {
    if (!phase2 || side.body_activity != boss_activity_destruction) return;
    ++side.body_destruction_progress;
    if (side.body_destruction_progress == gemini_body_destruction_phase2_ticks) {
        side.body_activity = boss_activity_inactive;
        ++result.components_retired;
    }
}

bool valid_frame(std::span<const std::uint8_t> frame, const std::size_t expected) noexcept {
    return frame.size() >= expected;
}

bool special_hits_side(
    const GeminiBossSideLifecycleState& side,
    const GeminiBossSpriteMaskView* masks,
    const SpecialWeaponState& special,
    bool& hit_head,
    bool& hit_body) noexcept {
    hit_head = false;
    hit_body = false;
    if (masks == nullptr || side.body_y <= -20) return false;

    const auto point = Point{special.x, special.y};
    if (valid_frame(masks->head_frame,
                    static_cast<std::size_t>(gemini_head_width * gemini_head_height)) &&
        point_hits_opaque_pixel(point, head_view(side), masks->head_frame)) {
        hit_head = true;
        return true;
    }

    const auto frame_index = static_cast<std::size_t>(side.body_frame);
    if (frame_index >= masks->body_frames.size()) return false;
    const auto frame = masks->body_frames[frame_index];
    if (valid_frame(frame, static_cast<std::size_t>(gemini_body_width * gemini_body_height)) &&
        point_hits_opaque_pixel(point, body_view(side), frame)) {
        hit_body = true;
        return true;
    }
    return false;
}

void begin_side_destruction(
    GeminiBossSideLifecycleState& side,
    ScoreState& score,
    GeminiBossStepResult& result) noexcept {
    side.body_activity = boss_activity_destruction;
    side.body_destruction_progress = 0;
    apply_score_delta(score, canonical_shareware_boss_score_award);
    result.score_delta += canonical_shareware_boss_score_award;
    ++result.destruction_transitions;
}

bool process_special_collision_side(
    GeminiBossSideLifecycleState& side,
    const bool is_side_a,
    SpecialWeaponState& special,
    StingerDisplayState& stinger_display,
    ScoreState& score,
    const GeminiBossSpriteMaskView* masks,
    GeminiBossStepResult& result) noexcept {
    if (side.body_activity != boss_activity_active ||
        special.activity != SpecialWeaponActivity::LaunchedHoming) {
        return false;
    }

    bool hit_head = false;
    bool hit_body = false;
    if (!special_hits_side(side, masks, special, hit_head, hit_body)) return false;

    special.activity = SpecialWeaponActivity::ImpactConsumed;
    special.motion_y = 0;
    result.special_hit_head = hit_head;
    result.special_hit_body = hit_body;
    if (is_side_a) result.special_hit_side_a = true;
    else result.special_hit_side_b = true;

    const auto damage = special.kind == SpecialWeaponKind::Probe
        ? probe_damage : stinger_damage;
    result.special_damage = damage;
    // Win32 0x00405637..0x0040570E and the mirrored side-B block route
    // Probe impacts through one 0x00402900 call and Stinger impacts through two.
    result.explosion_sfx_variant_calls +=
        special.kind == SpecialWeaponKind::Probe ? 1u : 2u;
    if (special.kind == SpecialWeaponKind::Stinger) {
        activate_stinger_display_at(stinger_display, special.x, special.y);
        result.stinger_display_activated = true;
    }

    side.head_damage = static_cast<std::uint8_t>(side.head_damage + damage);
    if (side.head_damage > side.head_damage_threshold) {
        begin_side_destruction(side, score, result);
    }
    return true;
}

} // namespace

void initialize_gemini_boss_runtime(
    GeminiBossLifecycleState& state,
    const DifficultyLevel difficulty) noexcept {
    state = GeminiBossLifecycleState{};
    state.side_a.body_activity = boss_activity_active;
    state.side_a.head_activity = boss_activity_active;
    state.side_b.body_activity = boss_activity_active;
    state.side_b.head_activity = boss_activity_active;
    state.side_a.body_frame_count = static_cast<std::uint8_t>(gemini_body_frame_count);
    state.side_b.body_frame_count = static_cast<std::uint8_t>(gemini_body_frame_count);
    state.side_a.head_damage_threshold = side_a_damage_threshold(difficulty);
    state.side_b.head_damage_threshold = side_b_damage_threshold(difficulty);
    state.root_fixed_x = 0;
    state.root_fixed_y = -100 * fixed_one;
    state.root_velocity_x = live_initial_velocity_x(difficulty);
    state.root_velocity_y = live_initial_velocity_y(difficulty);
    state.horizontal_speed_cap = live_horizontal_speed_cap(difficulty);
    state.runtime_initialized = true;
    update_derived_geometry(state);
}

GeminiBossStepResult step_gemini_boss(
    GeminiBossLifecycleState& state,
    const std::int32_t gameplay_substep_phase,
    const std::int32_t player_x,
    const DifficultyLevel difficulty,
    OriginalRandomState& random,
    EnemyBombPool& enemy_bombs,
    EnemyBombSpawnGate& bomb_spawn_gate,
    SpecialWeaponState& special,
    StingerDisplayState& stinger_display,
    ScoreState& score,
    const GeminiBossSpriteMaskView* sprite_masks) noexcept {
    GeminiBossStepResult result{};
    if (!state.runtime_initialized) return result;

    advance_root_motion(state, player_x, result);
    advance_body_animation(state, gameplay_substep_phase);
    maybe_spawn_boss_bomb(
        state, gameplay_substep_phase, difficulty, random,
        enemy_bombs, bomb_spawn_gate, result);

    const bool phase2 = is_win32_phase2(gameplay_substep_phase);

    // Side A's destruction tail precedes its active collision branch.
    advance_destruction_side(state.side_a, phase2, result);
    (void)process_special_collision_side(
        state.side_a, true, special, stinger_display, score, sprite_masks, result);

    // Side B repeats the same ordering after side A. If side A consumed the
    // special, B observes state 10 and cannot receive the same projectile.
    advance_destruction_side(state.side_b, phase2, result);
    (void)process_special_collision_side(
        state.side_b, false, special, stinger_display, score, sprite_masks, result);

    return result;
}

} // namespace drone::gameplay
