#include <drone/gameplay/drone_objective.hpp>

#include <cassert>
#include <iostream>

int main() {
    using namespace drone::gameplay;

    {
        DroneObjectiveState drone{};
        assert(drone.x == canonical_drone_session_initial_x);
        assert(drone.y == canonical_drone_session_initial_y);
        assert(drone.activity == canonical_drone_active_activity);
        assert(!drone.disarm_completed);
        assert(drone.hover_phase2_ticks == 0);
        assert(drone.destruction_countdown == canonical_drone_destruction_countdown_idle);
        assert(drone.detonation_tick == canonical_drone_detonation_tick_initial);
        assert(drone.destruction_settlement_phase0_ticks == 0);
    }

    // Movement is owned only on the recovered phase-2 cadence. The two audio
    // landmarks are transient: -117 becomes -116 and -40 becomes -39 in the
    // same unresolved approach update.
    {
        MissionOutcomeState mission{};
        std::int32_t settlement = canonical_drone_settlement_tick_cap;
        DroneObjectiveState drone{};
        drone.y = -118;

        auto result = step_drone_objective_normal(drone, 1, mission, settlement);
        assert(!result.moved && drone.y == -118);

        result = step_drone_objective_normal(drone, 2, mission, settlement);
        assert(result.moved && drone.y == -116);
        assert(!result.boss_approach_boundary_reached);

        drone.y = -41;
        result = step_drone_objective_normal(drone, 2, mission, settlement);
        assert(result.moved && drone.y == -39);

        drone.y = -201;
        result = step_drone_objective_normal(drone, 2, mission, settlement);
        assert(result.moved && drone.y == canonical_drone_boss_approach_y);
        assert(result.boss_approach_boundary_reached);
    }

    // The unresolved Drone stops at Y=45. The hold counter is reset at Y=44,
    // starts on the update that first reaches 45, and at exactly 4200 phase-2
    // ticks starts the same pre-detonation countdown used by destructive hits.
    {
        MissionOutcomeState mission{};
        std::int32_t settlement = canonical_drone_settlement_tick_cap;
        DroneObjectiveState drone{};
        drone.y = 43;
        drone.hover_phase2_ticks = 99;

        auto result = step_drone_objective_normal(drone, 2, mission, settlement);
        assert(result.moved && drone.y == 44);
        assert(drone.hover_phase2_ticks == 0);

        result = step_drone_objective_normal(drone, 2, mission, settlement);
        assert(result.moved && drone.y == canonical_drone_hover_y);
        assert(drone.hover_phase2_ticks == 1);

        result = step_drone_objective_normal(drone, 2, mission, settlement);
        assert(!result.moved && drone.y == canonical_drone_hover_y);
        assert(drone.hover_phase2_ticks == 2);

        drone.hover_phase2_ticks = canonical_drone_hover_timeout_phase2_ticks - 1;
        result = step_drone_objective_normal(drone, 2, mission, settlement);
        assert(result.hover_timeout_reached);
        assert(result.destruction_countdown_started);
        assert(drone.hover_phase2_ticks == canonical_drone_hover_timeout_phase2_ticks);
        assert(drone.destruction_countdown == 0);

        // An already-active countdown is not restarted by the hover counter.
        drone.hover_phase2_ticks = canonical_drone_hover_timeout_phase2_ticks - 1;
        drone.destruction_countdown = 40;
        result = step_drone_objective_normal(drone, 2, mission, settlement);
        assert(!result.hover_timeout_reached);
        assert(!result.destruction_countdown_started);
        assert(drone.destruction_countdown == 40);
    }

    // The countdown is an every-state2 pre-phase scalar. It detonates when an
    // increment reaches 99, restores the idle value 100 before the trigger,
    // captures the 15x38 Drone center, commits outcome 2, and applies the exact
    // score/progress consequence.
    {
        MissionOutcomeState mission{};
        ScoreState score{.total = 1750, .extra_life_progress = 620};
        DroneObjectiveState drone{};
        drone.x = 100;
        drone.y = 45;
        assert(start_drone_destruction_countdown(drone));
        assert(!start_drone_destruction_countdown(drone));
        assert(drone.destruction_countdown == 0);

        auto countdown = step_drone_destruction_countdown(drone, mission, score);
        assert(countdown.advanced && !countdown.detonation_started);
        assert(drone.destruction_countdown == 1);

        drone.destruction_countdown = canonical_drone_destruction_countdown_trigger - 1;
        countdown = step_drone_destruction_countdown(drone, mission, score);
        assert(countdown.advanced && countdown.detonation_started);
        assert(countdown.outcome_committed);
        assert(countdown.score_delta == -1000);
        assert(drone.destruction_countdown == canonical_drone_destruction_countdown_idle);
        assert(drone.activity == canonical_drone_destruction_activity);
        assert(drone.detonation_tick == 0);
        assert(drone.detonation_center_x == 107);
        assert(drone.detonation_center_y == 64);
        assert(score.total == 750);
        assert(score.extra_life_progress == 0);
        assert(mission.processed_count == 1);
        assert(mission.outcomes[0] == DroneOutcome::Detonated);
    }

    // The logical detonation tick advances every state-2 update and caps at
    // 330. The post-trajectory effect logic is phase-0-only, starts above tick
    // 25, moves the captured center Y, requests four explosion emissions,
    // resets settlement at 329, then increments settlement after 329.
    {
        DroneObjectiveState drone{};
        drone.activity = canonical_drone_destruction_activity;
        drone.detonation_center_y = 100;
        drone.detonation_tick = 25;

        OriginalRandomState random{};
        auto effect = step_drone_detonation_effect_logic(drone, 0, random);
        assert(!effect.logical_effect_tick);

        drone.detonation_tick = 26;
        effect = step_drone_detonation_effect_logic(drone, 1, random);
        assert(!effect.logical_effect_tick);
        effect = step_drone_detonation_effect_logic(drone, 0, random);
        assert(effect.logical_effect_tick);
        assert(effect.explosion_spawns_requested == 8);
        assert(effect.random_draws_consumed == 17);
        assert(random.draws == 17);
        assert(drone.detonation_center_y == 101);

        drone.detonation_tick = canonical_drone_detonation_tick_settlement_reset;
        drone.destruction_settlement_phase0_ticks = 17;
        effect = step_drone_detonation_effect_logic(drone, 0, random);
        assert(effect.settlement_reset);
        assert(!effect.settlement_advanced);
        assert(drone.destruction_settlement_phase0_ticks == 0);

        drone.detonation_tick = canonical_drone_detonation_tick_cap;
        effect = step_drone_detonation_effect_logic(drone, 0, random);
        assert(effect.settlement_advanced);
        assert(drone.destruction_settlement_phase0_ticks == 1);

        drone.detonation_tick = canonical_drone_detonation_tick_cap - 1;
        advance_drone_detonation_tick(drone);
        assert(drone.detonation_tick == canonical_drone_detonation_tick_cap);
        advance_drone_detonation_tick(drone);
        assert(drone.detonation_tick == canonical_drone_detonation_tick_cap);

        drone.destruction_settlement_phase0_ticks = canonical_drone_destruction_settlement_gate;
        assert(!drone_destruction_settlement_ready(drone));
        ++drone.destruction_settlement_phase0_ticks;
        assert(drone_destruction_settlement_ready(drone));
    }

    // Completed Probe decode releases the Y=45 hold. Y=201 commits exactly one
    // disarmed objective and is replaced by Y=202 in the same update.
    {
        MissionOutcomeState mission{};
        std::int32_t settlement = canonical_drone_settlement_tick_cap;
        DroneObjectiveState drone{};
        drone.y = canonical_drone_hover_y;
        drone.hover_phase2_ticks = 12;

        assert(mark_drone_disarm_completed(drone));
        assert(!mark_drone_disarm_completed(drone));
        auto result = step_drone_objective_normal(drone, 2, mission, settlement);
        assert(result.moved && drone.y == 46);
        assert(drone.hover_phase2_ticks == 13);

        drone.y = 200;
        result = step_drone_objective_normal(drone, 2, mission, settlement);
        assert(result.disarm_committed);
        assert(drone.y == canonical_drone_post_disarm_y);
        assert(mission.processed_count == 1);
        assert(mission.outcomes[0] == DroneOutcome::Disarmed);
    }

    // At Y=230 the early phase-2 settlement increment is overwritten with 0.
    // The next completed-disarm movement reaches 231 and clears decode status;
    // Y then stays fixed while the shared scalar approaches exactly 60.
    {
        MissionOutcomeState mission{};
        mission.processed_count = 1;
        mission.outcomes[0] = DroneOutcome::Disarmed;
        std::int32_t settlement = canonical_drone_settlement_tick_cap;
        DroneObjectiveState drone{};
        drone.y = 229;
        drone.disarm_completed = true;

        auto result = step_drone_objective_normal(drone, 2, mission, settlement);
        assert(result.settlement_tick_reset);
        assert(drone.y == canonical_drone_settlement_timer_reset_y);
        assert(settlement == 0);
        assert(drone.disarm_completed);

        settlement = 1; // caller's next early phase-2 increment
        result = step_drone_objective_normal(drone, 2, mission, settlement);
        assert(drone.y == 231);
        assert(!drone.disarm_completed);
        assert(!result.resolution_transition_ready);

        settlement = canonical_drone_transition_settlement_tick;
        result = step_drone_objective_normal(drone, 2, mission, settlement);
        assert(!result.moved && drone.y == 231);
        assert(result.resolution_transition_ready);
    }

    std::cout << "Drone objective progression/detonation tests passed\n";
    return 0;
}
