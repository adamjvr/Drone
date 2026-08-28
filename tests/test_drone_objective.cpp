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
    // starts on the update that first reaches 45, and exposes the exact 4200
    // phase-2 timeout without starting the not-yet-integrated detonation path.
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
        assert(drone.hover_phase2_ticks == canonical_drone_hover_timeout_phase2_ticks);
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

    std::cout << "Drone objective progression tests passed\n";
    return 0;
}
