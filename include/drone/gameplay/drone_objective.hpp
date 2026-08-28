#pragma once

#include <drone/gameplay/mission_progression.hpp>
#include <drone/gameplay/scoring.hpp>
#include <drone/gameplay/world_scroll.hpp>

#include <cstdint>

namespace drone::gameplay {

// Canonical shareware Drone entity/timing values established from the Win32
// state-2 objective path and the 0x0041D220/0x0041E4D0 detonation pair.
inline constexpr std::uint8_t canonical_drone_active_activity = 1;
inline constexpr std::int32_t canonical_drone_hover_y = 45;
inline constexpr std::uint16_t canonical_drone_hover_timeout_phase2_ticks = 4200;
inline constexpr std::uint16_t canonical_drone_destruction_countdown_idle = 100;
inline constexpr std::uint16_t canonical_drone_destruction_countdown_trigger = 99;
inline constexpr std::int32_t canonical_drone_detonation_tick_initial = 61;
inline constexpr std::int32_t canonical_drone_detonation_tick_effect_start = 25;
inline constexpr std::int32_t canonical_drone_detonation_tick_settlement_reset = 329;
inline constexpr std::int32_t canonical_drone_detonation_tick_cap = 330;
inline constexpr std::uint16_t canonical_drone_destruction_settlement_gate = 70;
inline constexpr std::int32_t canonical_drone_sprite_width = 15;
inline constexpr std::int32_t canonical_drone_sprite_height = 38;
inline constexpr std::uint8_t canonical_drone_detonation_explosions_per_effect_tick = 4;

struct DroneObjectiveState {
    std::int32_t x = canonical_drone_session_initial_x;
    std::int32_t y = canonical_drone_session_initial_y;
    std::uint8_t activity = canonical_drone_active_activity;

    // Clean semantic counterpart of the completed Probe decode/disarm status.
    // The exact Probe decoder remains an explicit producer in this milestone.
    bool disarm_completed = false;

    // Exact WORD counter that advances while an unresolved Drone holds at Y=45.
    std::uint16_t hover_phase2_ticks = 0;

    // 0x00491CAC is idle at 100. Timeout/collision producers set it to zero;
    // the pre-phase state-2 block advances it and detonates when it reaches 99,
    // immediately restoring 100 before entering the destruction sequence.
    std::uint16_t destruction_countdown = canonical_drone_destruction_countdown_idle;

    // Logical destruction timing is separate from the four-phase presentation
    // cadence. The original initializes this to 61 outside active destruction,
    // resets it to zero at detonation, then increments every state-2 update up
    // to 330.
    std::int32_t detonation_tick = canonical_drone_detonation_tick_initial;

    // Contextual Drone +0x32 field. The phase-0 detonation updater resets it at
    // tick 329, increments it after 329, and the earlier state-2 gate settles
    // destruction only when it is greater than 70.
    std::uint16_t destruction_settlement_phase0_ticks = 0;

    // Captured by trigger_drone_detonation_sequence as the sprite center. The
    // clean core retains logical coordinates only; randomized/direct-framebuffer
    // presentation remains in the fidelity layer.
    std::int32_t detonation_center_x = 0;
    std::int32_t detonation_center_y = 0;
};

struct DroneObjectiveTickResult {
    bool moved = false;
    bool boss_approach_boundary_reached = false;
    bool disarm_committed = false;
    bool settlement_tick_reset = false;
    bool disarm_completion_cleared = false;
    bool resolution_transition_ready = false;
    bool hover_timeout_reached = false;
    bool destruction_countdown_started = false;
};

struct DroneDestructionCountdownTickResult {
    bool advanced = false;
    bool detonation_started = false;
    bool outcome_committed = false;
    std::int32_t score_delta = 0;
};

struct DroneDetonationEffectTickResult {
    bool logical_effect_tick = false;
    std::uint8_t explosion_spawns_requested = 0;
    bool settlement_reset = false;
    bool settlement_advanced = false;
};

// Accept a completed Probe decode from the still-external exact decoder. The
// status persists until the Drone has moved beyond Y=230, matching the original
// state-2 path that clears the decode status during settlement.
[[nodiscard]] bool mark_drone_disarm_completed(DroneObjectiveState& state) noexcept;

// Start the shared pre-detonation countdown only while the Drone is active and
// the scalar is in its canonical idle state (>99). Timeout owns one producer;
// exact sprite-mask weapon collisions can call this same semantic transition in
// a later Phase-4 collision milestone.
[[nodiscard]] bool start_drone_destruction_countdown(DroneObjectiveState& state) noexcept;

// Runs before the shared four-phase scheduler, exactly like the recovered
// state-2 preamble. Reaching 99 starts the logical 0x0041D220 destruction
// sequence, commits outcome 2 and applies the dedicated -1000 score consequence.
[[nodiscard]] DroneDestructionCountdownTickResult step_drone_destruction_countdown(
    DroneObjectiveState& state,
    MissionOutcomeState& mission,
    ScoreState& score) noexcept;

// The original increments this scalar on every active state-2 update regardless
// of Drone activity, saturating at 330. A detonation triggered earlier in the
// same update is reset to zero and therefore becomes tick 1 here.
void advance_drone_detonation_tick(DroneObjectiveState& state) noexcept;

// Portable logical counterpart of update_drone_detonation_effect. It preserves
// the exact phase/tick gates, center drift, four-spawn request and destruction
// settlement timing, while intentionally omitting randomized framebuffer work.
[[nodiscard]] DroneDetonationEffectTickResult step_drone_detonation_effect_logic(
    DroneObjectiveState& state,
    std::int32_t gameplay_substep_phase) noexcept;

[[nodiscard]] constexpr bool drone_destruction_settlement_ready(
    const DroneObjectiveState& state) noexcept {
    return state.activity == canonical_drone_destruction_activity &&
           state.destruction_settlement_phase0_ticks >
               canonical_drone_destruction_settlement_gate;
}

// Advance the canonical normal Drone objective path for one already-advanced
// gameplay substep. This owns the shareware-reachable phase-2 approach, Y=45
// hold, exact 4200-tick timeout->countdown handoff, completed-disarm departure,
// Y=201 ledger commit, Y=230 settlement reset, and Y>230/tick==60 transition.
[[nodiscard]] DroneObjectiveTickResult step_drone_objective_normal(
    DroneObjectiveState& state,
    std::int32_t gameplay_substep_phase,
    MissionOutcomeState& mission,
    std::int32_t& settlement_tick) noexcept;

} // namespace drone::gameplay
