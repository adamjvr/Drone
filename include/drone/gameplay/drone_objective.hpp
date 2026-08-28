#pragma once

#include <drone/gameplay/mission_progression.hpp>
#include <drone/gameplay/world_scroll.hpp>

#include <cstdint>

namespace drone::gameplay {

// Canonical shareware Drone entity activity values observed at the reset and
// detonation paths. Phase 4 currently owns the normal activity-1 route; the
// destructive activity-2 lifecycle is intentionally left to the next slice.
inline constexpr std::uint8_t canonical_drone_active_activity = 1;
inline constexpr std::int32_t canonical_drone_hover_y = 45;
inline constexpr std::uint16_t canonical_drone_hover_timeout_phase2_ticks = 4200;

struct DroneObjectiveState {
    std::int32_t x = canonical_drone_session_initial_x;
    std::int32_t y = canonical_drone_session_initial_y;
    std::uint8_t activity = canonical_drone_active_activity;

    // Clean semantic counterpart of the completed Probe decode/disarm status.
    // The Probe decoder remains an explicit producer in this milestone.
    bool disarm_completed = false;

    // Exact WORD counter that advances while an unresolved Drone holds at Y=45.
    // Reaching 4200 is exposed as a semantic event; the destructive countdown
    // and detonation sequence are integrated separately rather than guessed.
    std::uint16_t hover_phase2_ticks = 0;
};

struct DroneObjectiveTickResult {
    bool moved = false;
    bool boss_approach_boundary_reached = false;
    bool disarm_committed = false;
    bool settlement_tick_reset = false;
    bool resolution_transition_ready = false;
    bool hover_timeout_reached = false;
};

// Accept a completed Probe decode from the still-external exact decoder. The
// status persists until the Drone has moved beyond Y=230, matching the original
// state-2 path that clears the decode status during settlement.
[[nodiscard]] bool mark_drone_disarm_completed(DroneObjectiveState& state) noexcept;

// Advance the canonical normal Drone objective path for one already-advanced
// gameplay substep. This owns only the shareware-reachable non-destructive
// route: phase-2 approach, Y=45 hold, completed-disarm departure, Y=201 ledger
// commit, Y=230 settlement reset, and the exact Y>230/tick==60 transition gate.
// The 4200-tick hover timeout is reported but its detonation producer is not
// started here yet.
[[nodiscard]] DroneObjectiveTickResult step_drone_objective_normal(
    DroneObjectiveState& state,
    std::int32_t gameplay_substep_phase,
    MissionOutcomeState& mission,
    std::int32_t& settlement_tick) noexcept;

} // namespace drone::gameplay
