#include <drone/gameplay/game_session.hpp>
#include <drone/gameplay/lid_top_boss.hpp>

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>

namespace {

using namespace drone::gameplay;

std::uint32_t seed_for_mod_less_than(
    const std::uint16_t modulus,
    const std::uint16_t threshold) {
    for (std::uint32_t seed = 1; seed < 100000; ++seed) {
        OriginalRandomState random{};
        seed_original_random(random, seed);
        if (original_random_mod(random, modulus) < threshold) {
            return seed;
        }
    }
    assert(false && "could not find deterministic RNG seed");
    return 1;
}

void freeze_root(LidTopBossLifecycleState& state, const std::int32_t x, const std::int32_t y) {
    state.root_x = x;
    state.root_y = y;
    state.root_fixed_x = x << 16;
    state.root_fixed_y = y << 16;
    state.root_velocity_x = 0;
    state.root_velocity_y = 0;
    state.horizontal_speed_cap = 0;
}

} // namespace

int main() {
    using namespace drone::gameplay;

    // Exact Win32 0x00417220 initialization constants for live and demo play.
    {
        LidTopBossLifecycleState state{};
        initialize_lid_top_boss_runtime(state, DifficultyLevel::Beginner, false);
        assert(state.runtime_initialized);
        assert(state.top_activity == boss_activity_active);
        assert(state.lid_activity == lid_top_initial_lid_activity);
        assert(state.lid_frame == 0);
        assert(state.root_x == 0 && state.root_y == -100);
        assert(state.root_fixed_x == 0 && state.root_fixed_y == -100 * 65536);
        assert(state.root_velocity_x == 10923);
        assert(state.root_velocity_y == 0x5556);
        assert(state.horizontal_speed_cap == 0xc000);

        initialize_lid_top_boss_runtime(state, DifficultyLevel::Advanced, false);
        assert(state.root_velocity_x == 32769);
        assert(state.root_velocity_y == 0x8000);
        assert(state.horizontal_speed_cap == 0x12000);

        initialize_lid_top_boss_runtime(state, DifficultyLevel::Beginner, true);
        assert(state.root_velocity_x == 0x8000);
        assert(state.root_velocity_y == 0x8000);
        assert(state.horizontal_speed_cap == 0x12000);
    }

    // Root movement uses 16.16 state and compares boss center against the
    // player's left X. Reaching Y >= 240 reverses vertical motion to -100 px.
    {
        LidTopBossLifecycleState state{};
        initialize_lid_top_boss_runtime(state, DifficultyLevel::Beginner, false);
        freeze_root(state, 10, 20);
        state.horizontal_speed_cap = 0x10000;
        state.root_velocity_x = 0x8000;
        state.root_velocity_y = 0x10000;
        EnemyBombPool bombs{};
        EnemyBombSpawnGate gate{};
        RapidMissilePool missiles{};
        SpecialWeaponState special{};
        ScoreState score{};
        OriginalRandomState random{};
        const auto result = step_lid_top_boss(
            state, 1, 300, DifficultyLevel::Beginner, true, random,
            bombs, gate, missiles, special, score);
        assert(result.root_moved);
        assert(state.root_x == 10 && state.root_y == 21);
        assert(state.root_velocity_x == 0x844c);

        freeze_root(state, 5, 240);
        state.horizontal_speed_cap = 0x10000;
        const auto retreat = step_lid_top_boss(
            state, 1, 0, DifficultyLevel::Beginner, true, random,
            bombs, gate, missiles, special, score);
        assert(retreat.vertical_retreat_started);
        assert(state.root_velocity_y == -100 * 65536);
    }

    // Boss bomb chance is drawn before the gate/capacity tests. A successful
    // spawn preserves reused-slot horizontal motion and animation frame.
    {
        LidTopBossLifecycleState state{};
        initialize_lid_top_boss_runtime(state, DifficultyLevel::Advanced, false);
        freeze_root(state, 100, 40);
        EnemyBombPool bombs{};
        bombs.bombs[0].horizontal_step = 7;
        bombs.bombs[0].frame = 2;
        EnemyBombSpawnGate gate{.counter = enemy_bomb_spawn_gate_ready};
        RapidMissilePool missiles{};
        SpecialWeaponState special{};
        ScoreState score{};
        OriginalRandomState random{};
        const auto seed = seed_for_mod_less_than(100, 6);
        seed_original_random(random, seed);
        OriginalRandomState expected = random;
        (void)original_random_mod(expected, 100);
        const auto side = original_random_mod(expected, 10);

        const auto result = step_lid_top_boss(
            state, 2, 0, DifficultyLevel::Advanced, false, random,
            bombs, gate, missiles, special, score);
        assert(result.enemy_bomb_spawned);
        assert(result.enemy_bomb_spawn_index == 0);
        assert(bombs.active_count == 1);
        assert(bombs.bombs[0].active);
        assert(bombs.bombs[0].x == 100 + (side < 5 ? 30 : 41));
        assert(bombs.bombs[0].y == 93);
        assert(bombs.bombs[0].horizontal_step == 7);
        assert(bombs.bombs[0].frame == 2);
        assert(gate.counter == 0);
        assert(random.draws == 2);

        // Gate-not-ready still consumes the first chance draw if chance passes.
        EnemyBombPool blocked_bombs{};
        EnemyBombSpawnGate blocked_gate{.counter = 4};
        seed_original_random(random, seed);
        const auto blocked = step_lid_top_boss(
            state, 2, 0, DifficultyLevel::Advanced, false, random,
            blocked_bombs, blocked_gate, missiles, special, score);
        assert(!blocked.enemy_bomb_spawned);
        assert(random.draws == 1);
    }

    // The rapid-missile loop has two distinct tests. The top opaque-mask test
    // is pre-gated to X < root+39, while the frame-0 opening weakpoint begins at
    // root+53. Each consumes the missile and decrements the pool count once. A
    // weakpoint hit on phase 2 immediately advances the opened lid to frame 1.
    {
        LidTopBossLifecycleState state{};
        initialize_lid_top_boss_runtime(state, DifficultyLevel::Beginner, true);
        freeze_root(state, 50, 30);
        EnemyBombPool bombs{};
        EnemyBombSpawnGate gate{};
        RapidMissilePool missiles{};
        missiles.missiles[0] = {.x = 70, .y = 53, .active = true};
        missiles.active_count = 1;
        SpecialWeaponState special{};
        ScoreState score{};
        OriginalRandomState random{};
        std::array<std::uint8_t, lid_top_top_width * lid_top_top_height> top{};
        top[(53 - 30) * lid_top_top_width + (70 - 50)] = 1;
        const LidTopBossSpriteMaskView mask{.top_frame = top};

        auto result = step_lid_top_boss(
            state, 1, 0, DifficultyLevel::Beginner, true, random,
            bombs, gate, missiles, special, score, &mask);
        assert(result.rapid_top_opaque_collisions == 1);
        assert(result.explosion_sfx_variant_calls == 1);
        assert(result.rapid_lid_open_collisions == 0);
        assert(result.rapid_missiles_consumed == 1);
        assert(!result.lid_opened);
        assert(missiles.active_count == 0);

        missiles.missiles[0] = {.x = 103, .y = 53, .active = true};
        missiles.active_count = 1;
        result = step_lid_top_boss(
            state, 2, 0, DifficultyLevel::Beginner, true, random,
            bombs, gate, missiles, special, score, &mask);
        assert(result.rapid_top_opaque_collisions == 0);
        assert(result.rapid_lid_open_collisions == 1);
        assert(result.explosion_sfx_variant_calls == 1);
        assert(result.rapid_missiles_consumed == 1);
        assert(result.lid_opened);
        assert(missiles.active_count == 0);
        assert(state.lid_activity == boss_activity_active);
        assert(state.lid_frame == 1);
    }

    // At frame 8 the live close chance can transition 1->6, and the separate
    // state-6 animation block immediately decrements frame 8->7.
    {
        LidTopBossLifecycleState state{};
        initialize_lid_top_boss_runtime(state, DifficultyLevel::Advanced, false);
        freeze_root(state, 0, 20);
        state.lid_activity = boss_activity_active;
        state.lid_frame = 8;
        EnemyBombPool bombs{};
        EnemyBombSpawnGate gate{};
        RapidMissilePool missiles{};
        SpecialWeaponState special{};
        ScoreState score{};
        OriginalRandomState random{};
        // First phase-2 draw is the bomb chance; second can be close chance only
        // if bomb chance fails. Search for that exact pair.
        std::uint32_t seed = 1;
        for (; seed < 100000; ++seed) {
            OriginalRandomState probe{};
            seed_original_random(probe, seed);
            const auto bomb_roll = original_random_mod(probe, 100);
            if (bomb_roll < 6) continue;
            if (original_random_mod(probe, 200) < 3) break;
        }
        assert(seed < 100000);
        seed_original_random(random, seed);
        const auto result = step_lid_top_boss(
            state, 2, 0, DifficultyLevel::Advanced, false, random,
            bombs, gate, missiles, special, score);
        assert(result.lid_close_started);
        assert(state.lid_activity == lid_top_initial_lid_activity);
        assert(state.lid_frame == 7);
    }

    // A launched special hitting a closed top is consumed without opening it.
    {
        LidTopBossLifecycleState state{};
        initialize_lid_top_boss_runtime(state, DifficultyLevel::Beginner, true);
        freeze_root(state, 50, 20);
        EnemyBombPool bombs{};
        EnemyBombSpawnGate gate{};
        RapidMissilePool missiles{};
        SpecialWeaponState special{};
        special.activity = SpecialWeaponActivity::LaunchedHoming;
        special.kind = SpecialWeaponKind::Probe;
        special.x = 60;
        special.y = 30;
        special.motion_y = -2;
        ScoreState score{};
        OriginalRandomState random{};
        const auto result = step_lid_top_boss(
            state, 1, 0, DifficultyLevel::Beginner, true, random,
            bombs, gate, missiles, special, score);
        assert(result.special_closed_top_impact);
        assert(result.explosion_sfx_variant_calls == 1);
        assert(special.activity == SpecialWeaponActivity::ImpactConsumed);
        assert(special.motion_y == 0);
        assert(state.lid_activity == lid_top_initial_lid_activity);
    }

    // Only a Stinger can hit the exposed core once frame > 6 and lid Y > 0.
    // The same update enters destruction progress 1; +100 lands at count 25,
    // stops root motion, and the top then needs 30 later phase-2 ticks to retire.
    {
        LidTopBossLifecycleState state{};
        initialize_lid_top_boss_runtime(state, DifficultyLevel::Beginner, true);
        freeze_root(state, 40, 10);
        state.lid_activity = boss_activity_active;
        state.lid_frame = 7;
        EnemyBombPool bombs{};
        EnemyBombSpawnGate gate{};
        RapidMissilePool missiles{};
        SpecialWeaponState special{};
        special.activity = SpecialWeaponActivity::LaunchedHoming;
        special.kind = SpecialWeaponKind::Stinger;
        special.x = 69;
        special.y = 42;
        special.motion_y = -2;
        ScoreState score{};
        OriginalRandomState random{};

        auto result = step_lid_top_boss(
            state, 1, 0, DifficultyLevel::Beginner, true, random,
            bombs, gate, missiles, special, score);
        assert(result.stinger_core_hit);
        assert(result.destruction_transitions == 1);
        assert(state.lid_activity == boss_activity_destruction);
        assert(state.lid_destruction_progress == 1);
        assert(special.activity == SpecialWeaponActivity::ImpactConsumed);

        for (int i = 0; i < 23; ++i) {
            result = step_lid_top_boss(
                state, 1, 0, DifficultyLevel::Beginner, true, random,
                bombs, gate, missiles, special, score);
            assert(result.score_delta == 0);
        }
        assert(state.lid_destruction_progress == 24);
        state.root_velocity_x = 123;
        state.root_velocity_y = 456;
        result = step_lid_top_boss(
            state, 1, 0, DifficultyLevel::Beginner, true, random,
            bombs, gate, missiles, special, score);
        assert(result.score_delta == 100);
        assert(result.top_motion_stopped);
        assert(state.lid_activity == boss_activity_inactive);
        assert(state.top_activity == boss_activity_destruction);
        assert(state.top_destruction_progress == 0);
        assert(state.root_velocity_x == 0 && state.root_velocity_y == 0);
        assert(score.total == 100 && score.extra_life_progress == 100);

        for (int i = 0; i < 29; ++i) {
            result = step_lid_top_boss(
                state, 2, 0, DifficultyLevel::Beginner, true, random,
                bombs, gate, missiles, special, score);
            assert(result.components_retired == 0);
        }
        assert(state.top_destruction_progress == 29);
        result = step_lid_top_boss(
            state, 2, 0, DifficultyLevel::Beginner, true, random,
            bombs, gate, missiles, special, score);
        assert(result.components_retired == 1);
        assert(state.top_activity == boss_activity_inactive);
    }

    // GameSession dispatch initializes the native runtime. Boss collisions occur
    // after common special dispatch, so state 10 survives the collision update
    // and is settled to zero only on the next gameplay update.
    {
        GameSession session{};
        session.encounter.boss.family = BossFamily::LidTop;
        initialize_lid_top_boss_runtime(
            session.encounter.boss.lid_top,
            DifficultyLevel::Beginner,
            true);
        freeze_root(session.encounter.boss.lid_top, 50, 20);
        session.encounter.special_weapon.activity = SpecialWeaponActivity::LaunchedHoming;
        session.encounter.special_weapon.kind = SpecialWeaponKind::Probe;
        session.encounter.special_weapon.x = 60;
        session.encounter.special_weapon.y = 32; // common movement -> y 30, still inside top
        session.campaign.player_lifecycle.player_active = true;

        const auto first = step_game_session(session, {}, {});
        assert(first.lid_top_special_closed_top_impact);
        assert(session.encounter.special_weapon.activity == SpecialWeaponActivity::ImpactConsumed);
        const auto second = step_game_session(session, {}, {});
        assert(second.advanced);
        assert(session.encounter.special_weapon.activity == SpecialWeaponActivity::Inactive);
    }

    std::cout << "Drone native Lid/Top boss combat tests passed\n";
    return 0;
}
