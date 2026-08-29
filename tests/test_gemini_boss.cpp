#include <drone/gameplay/gemini_boss.hpp>

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>

namespace {
using namespace drone::gameplay;

std::uint32_t seed_for_chance_and_side(
    const std::uint16_t chance_threshold,
    const bool want_left) {
    for (std::uint32_t seed = 1; seed < 100000; ++seed) {
        OriginalRandomState random{};
        seed_original_random(random, seed);
        if (original_random_mod(random, 100) >= chance_threshold) continue;
        const auto side = original_random_mod(random, 10);
        if ((side < 5) == want_left) return seed;
    }
    assert(false);
    return 1;
}

void freeze_root(GeminiBossLifecycleState& state, const int x, const int y) {
    state.root_fixed_x = x << 16;
    state.root_fixed_y = y << 16;
    state.root_velocity_x = 0;
    state.root_velocity_y = 0;
    state.horizontal_speed_cap = 0;
    state.side_a.body_x = x;
    state.side_a.body_y = y;
    state.side_a.head_x = x + 6;
    state.side_a.head_y = y + 41;
    state.side_b.body_x = x + 170;
    state.side_b.body_y = y;
    state.side_b.head_x = x + 176;
    state.side_b.head_y = y + 41;
}
}

int main() {
    using namespace drone::gameplay;

    // Exact initializer: paired activity, shared motion root, 170-pixel body
    // separation, 6/41 head offsets, and asymmetric higher-difficulty damage thresholds.
    {
        GeminiBossLifecycleState state{};
        initialize_gemini_boss_runtime(state, DifficultyLevel::Beginner);
        assert(state.runtime_initialized);
        assert(state.side_a.body_activity == boss_activity_active);
        assert(state.side_b.body_activity == boss_activity_active);
        assert(state.side_a.head_activity == boss_activity_active);
        assert(state.side_b.head_activity == boss_activity_active);
        assert(state.side_a.body_x == 0 && state.side_b.body_x == 170);
        assert(state.side_a.head_x == 6 && state.side_b.head_x == 176);
        assert(state.side_a.head_y == -59 && state.side_b.head_y == -59);
        assert(state.root_velocity_x == 10923);
        assert(state.root_velocity_y == 0x5556);
        assert(state.horizontal_speed_cap == 0xc000);
        assert(state.side_a.head_damage_threshold == 20);
        assert(state.side_b.head_damage_threshold == 20);

        initialize_gemini_boss_runtime(state, DifficultyLevel::Advanced);
        assert(state.root_velocity_x == 32769);
        assert(state.root_velocity_y == 0x8000);
        assert(state.horizontal_speed_cap == 0x12000);
        assert(state.side_a.head_damage_threshold == 35);
        assert(state.side_b.head_damage_threshold == 30);
    }

    // Shared-root movement tracks the midpoint of the two heads against player
    // left X. Phase 2 animates A forward and B backward (0 wraps to 29).
    {
        GeminiBossLifecycleState state{};
        initialize_gemini_boss_runtime(state, DifficultyLevel::Beginner);
        freeze_root(state, 10, 20);
        state.horizontal_speed_cap = 0x10000;
        state.root_velocity_x = 0x8000;
        state.root_velocity_y = 0x10000;
        EnemyBombPool bombs{};
        EnemyBombSpawnGate gate{};
        SpecialWeaponState special{};
        StingerDisplayState display{};
        ScoreState score{};
        OriginalRandomState random{};

        auto result = step_gemini_boss(
            state, 1, 300, DifficultyLevel::Beginner, random,
            bombs, gate, special, display, score);
        assert(result.root_moved);
        assert(state.side_a.body_x == 10 && state.side_a.body_y == 21);
        assert(state.side_b.body_x == 180 && state.side_b.body_y == 21);
        assert(state.root_velocity_x == 0x844c);

        freeze_root(state, 5, 240);
        state.horizontal_speed_cap = 0x10000;
        result = step_gemini_boss(
            state, 2, 0, DifficultyLevel::Beginner, random,
            bombs, gate, special, display, score);
        assert(result.vertical_retreat_started);
        assert(state.root_velocity_y == -100 * 65536);
        assert(state.side_a.body_frame == 1);
        assert(state.side_b.body_frame == 29);
    }

    // Phase-2 bomb spawn consumes chance before gates, prefers A/B from rand%10,
    // falls back to the surviving side, and preserves reused bomb motion/frame.
    {
        GeminiBossLifecycleState state{};
        initialize_gemini_boss_runtime(state, DifficultyLevel::Advanced);
        freeze_root(state, 40, 20);
        EnemyBombPool bombs{};
        bombs.bombs[0].horizontal_step = 7;
        bombs.bombs[0].frame = 2;
        EnemyBombSpawnGate gate{.counter = enemy_bomb_spawn_gate_ready};
        SpecialWeaponState special{};
        StingerDisplayState display{};
        ScoreState score{};
        OriginalRandomState random{};
        const auto seed = seed_for_chance_and_side(6, true);
        seed_original_random(random, seed);

        auto result = step_gemini_boss(
            state, 2, 0, DifficultyLevel::Advanced, random,
            bombs, gate, special, display, score);
        assert(result.enemy_bomb_spawned);
        assert(result.enemy_bomb_spawn_index == 0);
        assert(bombs.bombs[0].x == state.side_a.head_x + 21);
        assert(bombs.bombs[0].y == state.side_a.head_y + 33);
        assert(bombs.bombs[0].horizontal_step == 7);
        assert(bombs.bombs[0].frame == 2);
        assert(gate.counter == 0);
        assert(random.draws == 2);

        // The chance draw is consumed before the later shared-gate rejection.
        GeminiBossLifecycleState blocked_state{};
        initialize_gemini_boss_runtime(blocked_state, DifficultyLevel::Advanced);
        freeze_root(blocked_state, 40, 20);
        EnemyBombPool blocked_bombs{};
        EnemyBombSpawnGate blocked_gate{.counter = enemy_bomb_spawn_gate_ready - 1};
        seed_original_random(random, seed);
        result = step_gemini_boss(
            blocked_state, 2, 0, DifficultyLevel::Advanced, random,
            blocked_bombs, blocked_gate, special, display, score);
        assert(!result.enemy_bomb_spawned);
        assert(random.draws == 1);

        EnemyBombPool fallback_bombs{};
        EnemyBombSpawnGate fallback_gate{.counter = enemy_bomb_spawn_gate_ready};
        state.side_a.body_activity = boss_activity_destruction;
        seed_original_random(random, seed);
        result = step_gemini_boss(
            state, 2, 0, DifficultyLevel::Advanced, random,
            fallback_bombs, fallback_gate, special, display, score);
        assert(result.enemy_bomb_spawned);
        assert(fallback_bombs.bombs[0].x == state.side_b.head_x + 21);
    }

    // Opaque collision checks head first, then current body frame. Probe damage
    // is +3 and a transparent head pixel can fall through to the body mask.
    {
        GeminiBossLifecycleState state{};
        initialize_gemini_boss_runtime(state, DifficultyLevel::Beginner);
        freeze_root(state, 20, 20);
        EnemyBombPool bombs{};
        EnemyBombSpawnGate gate{};
        StingerDisplayState display{};
        ScoreState score{};
        OriginalRandomState random{};
        SpecialWeaponState special{};
        special.activity = SpecialWeaponActivity::LaunchedHoming;
        special.kind = SpecialWeaponKind::Probe;
        special.x = state.side_a.body_x + 10;
        special.y = state.side_a.body_y + 10;
        special.motion_y = -2;

        std::array<std::uint8_t, gemini_body_width * gemini_body_height> body{};
        std::array<std::uint8_t, gemini_head_width * gemini_head_height> head{};
        body[10 * gemini_body_width + 10] = 1;
        GeminiBossSpriteMaskView masks{};
        masks.body_frames[0] = body;
        masks.head_frame = head;

        const auto result = step_gemini_boss(
            state, 1, 0, DifficultyLevel::Beginner, random,
            bombs, gate, special, display, score, &masks);
        assert(result.special_hit_side_a);
        assert(!result.special_hit_head);
        assert(result.special_hit_body);
        assert(result.special_damage == 3);
        assert(result.explosion_sfx_variant_calls == 1);
        assert(state.side_a.head_damage == 3);
        assert(special.activity == SpecialWeaponActivity::ImpactConsumed);
        assert(special.motion_y == 0);
    }

    // Side B's Intermediate/Advanced threshold is 30 and the comparison is
    // strict: 30 survives, the next Stinger pushes to 45 and awards +100.
    {
        GeminiBossLifecycleState state{};
        initialize_gemini_boss_runtime(state, DifficultyLevel::Intermediate);
        freeze_root(state, 0, 20);
        state.side_b.head_damage = 15;
        EnemyBombPool bombs{};
        EnemyBombSpawnGate gate{};
        StingerDisplayState display{};
        ScoreState score{};
        OriginalRandomState random{};
        std::array<std::uint8_t, gemini_head_width * gemini_head_height> head{};
        head[10 * gemini_head_width + 10] = 1;
        GeminiBossSpriteMaskView masks{};
        masks.head_frame = head;

        SpecialWeaponState special{};
        special.activity = SpecialWeaponActivity::LaunchedHoming;
        special.kind = SpecialWeaponKind::Stinger;
        special.x = state.side_b.head_x + 10;
        special.y = state.side_b.head_y + 10;
        auto result = step_gemini_boss(
            state, 1, 0, DifficultyLevel::Intermediate, random,
            bombs, gate, special, display, score, &masks);
        assert(result.special_hit_side_b);
        assert(result.special_hit_head);
        assert(result.stinger_display_activated);
        assert(result.explosion_sfx_variant_calls == 2);
        assert(display.active);
        assert(state.side_b.head_damage == 30);
        assert(state.side_b.body_activity == boss_activity_active);
        assert(result.score_delta == 0);

        special.activity = SpecialWeaponActivity::LaunchedHoming;
        special.x = state.side_b.head_x + 10;
        special.y = state.side_b.head_y + 10;
        result = step_gemini_boss(
            state, 1, 0, DifficultyLevel::Intermediate, random,
            bombs, gate, special, display, score, &masks);
        assert(result.explosion_sfx_variant_calls == 2);
        assert(state.side_b.head_damage == 45);
        assert(state.side_b.body_activity == boss_activity_destruction);
        assert(state.side_b.body_destruction_progress == 0);
        assert(result.destruction_transitions == 1);
        assert(result.score_delta == 100);
        assert(score.total == 100 && score.extra_life_progress == 100);

        for (int i = 0; i < 19; ++i) {
            result = step_gemini_boss(
                state, 2, 0, DifficultyLevel::Intermediate, random,
                bombs, gate, special, display, score, &masks);
            assert(state.side_b.body_activity == boss_activity_destruction);
        }
        result = step_gemini_boss(
            state, 2, 0, DifficultyLevel::Intermediate, random,
            bombs, gate, special, display, score, &masks);
        assert(state.side_b.body_activity == boss_activity_inactive);
        assert(result.components_retired >= 1);
        assert(state.side_b.head_activity == boss_activity_active);
    }

    std::cout << "gemini boss tests passed\n";
    return 0;
}
