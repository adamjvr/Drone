#include <drone/gameplay/drone_objective.hpp>

#include <cassert>
#include <iostream>

int main() {
    using namespace drone::gameplay;

    // Exact Win32 0x0041E4D0 update-side RNG sequence for seed 1, tick 26.
    // The function consumes eight center-scatter draws, one rand()%90 angle,
    // and eight radial-ring jitter draws: 17 total.
    {
        DroneObjectiveState drone{};
        drone.activity = canonical_drone_destruction_activity;
        drone.detonation_tick = 26;
        drone.detonation_center_x = 107;
        drone.detonation_center_y = 64;
        OriginalRandomState random{};

        const auto effect = step_drone_detonation_effect_logic(drone, 0, random);
        assert(effect.logical_effect_tick);
        assert(effect.explosion_spawns_requested == 8);
        assert(effect.random_draws_consumed == 17);
        assert(random.draws == 17);
        assert(random.state == 0x0bb31550u);
        assert(drone.detonation_center_y == 65);

        const auto& c0 = effect.explosions[0];
        const auto& c1 = effect.explosions[1];
        const auto& c2 = effect.explosions[2];
        const auto& c3 = effect.explosions[3];
        assert(c0.kind == DroneDetonationExplosionKind::CenterScatter);
        assert(c0.x == 126 && c0.y == 90);
        assert(c1.x == 105 && c1.y == 121);
        assert(c2.x == 70 && c2.y == 17);
        assert(c3.x == 81 && c3.y == 79);

        assert(effect.radial_start_angle == 52);
        for (std::size_t i = 4; i < 8; ++i) {
            assert(effect.explosions[i].kind == DroneDetonationExplosionKind::RadialRing);
            assert(effect.explosions[i].center_x == 107);
            assert(effect.explosions[i].center_y == 65);
            assert(effect.explosions[i].radius == 18);
        }
        assert(effect.explosions[4].angle_degrees == 52);
        assert(effect.explosions[4].jitter_x == 16 && effect.explosions[4].jitter_y == 9);
        assert(effect.explosions[5].angle_degrees == 142);
        assert(effect.explosions[5].jitter_x == 17 && effect.explosions[5].jitter_y == 17);
        assert(effect.explosions[6].angle_degrees == 232);
        assert(effect.explosions[6].jitter_x == 27 && effect.explosions[6].jitter_y == 9);
        assert(effect.explosions[7].angle_degrees == 322);
        assert(effect.explosions[7].jitter_x == 11 && effect.explosions[7].jitter_y == 19);
    }

    // Ineligible phase/tick/activity paths consume no RNG at all.
    {
        OriginalRandomState random{};
        DroneObjectiveState drone{};
        drone.activity = canonical_drone_destruction_activity;
        drone.detonation_tick = 25;
        auto effect = step_drone_detonation_effect_logic(drone, 0, random);
        assert(!effect.logical_effect_tick && random.draws == 0);

        drone.detonation_tick = 26;
        effect = step_drone_detonation_effect_logic(drone, 1, random);
        assert(!effect.logical_effect_tick && random.draws == 0);

        drone.activity = canonical_drone_active_activity;
        effect = step_drone_detonation_effect_logic(drone, 0, random);
        assert(!effect.logical_effect_tick && random.draws == 0);
    }

    // Settlement writes occur after the fixed 17-draw explosion sequence.
    {
        OriginalRandomState random{};
        DroneObjectiveState drone{};
        drone.activity = canonical_drone_destruction_activity;
        drone.detonation_tick = canonical_drone_detonation_tick_settlement_reset;
        drone.destruction_settlement_phase0_ticks = 12;
        auto effect = step_drone_detonation_effect_logic(drone, 0, random);
        assert(effect.settlement_reset && !effect.settlement_advanced);
        assert(drone.destruction_settlement_phase0_ticks == 0);
        assert(random.draws == 17);

        drone.detonation_tick = canonical_drone_detonation_tick_cap;
        effect = step_drone_detonation_effect_logic(drone, 0, random);
        assert(effect.settlement_advanced);
        assert(drone.destruction_settlement_phase0_ticks == 1);
        assert(random.draws == 34);
    }

    std::cout << "Drone detonation update-side RNG/presentation tests passed\n";
    return 0;
}
