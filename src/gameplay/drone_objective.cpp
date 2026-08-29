#include <drone/gameplay/drone_objective.hpp>

#include <drone/gameplay/gameplay_phase.hpp>

#include <limits>

namespace drone::gameplay {
namespace {

// Two approach coordinates are immediately replaced by their following value
// after the original audio trigger. They are therefore not persistent state at
// the end of a normal unresolved phase-2 update.
enum class DroneApproachLandmark : std::uint8_t {
    None,
    LoopStart,
    HintOneShot,
};

DroneApproachLandmark apply_approach_landmark_skip(DroneObjectiveState& state) noexcept {
    if (state.y == -117) {
        state.y = -116;
        return DroneApproachLandmark::LoopStart;
    }
    if (state.y == -40) {
        state.y = -39;
        return DroneApproachLandmark::HintOneShot;
    }
    return DroneApproachLandmark::None;
}

} // namespace

bool mark_drone_disarm_completed(DroneObjectiveState& state) noexcept {
    if (state.activity == canonical_drone_destruction_activity || state.disarm_completed) {
        return false;
    }
    state.disarm_completed = true;
    return true;
}

bool start_drone_destruction_countdown(DroneObjectiveState& state) noexcept {
    if (state.activity != canonical_drone_active_activity ||
        state.destruction_countdown <= canonical_drone_destruction_countdown_trigger) {
        return false;
    }
    state.destruction_countdown = 0;
    return true;
}

DroneDestructionCountdownTickResult step_drone_destruction_countdown(
    DroneObjectiveState& state,
    MissionOutcomeState& mission,
    ScoreState& score) noexcept {
    DroneDestructionCountdownTickResult result{};

    if (state.destruction_countdown >= canonical_drone_destruction_countdown_idle) {
        return result;
    }

    ++state.destruction_countdown;
    result.advanced = true;
    if (state.destruction_countdown != canonical_drone_destruction_countdown_trigger) {
        return result;
    }

    // The original increments 99 to 100 immediately before 0x0041D220.
    state.destruction_countdown = canonical_drone_destruction_countdown_idle;
    state.activity = canonical_drone_destruction_activity;
    state.disarm_completed = false;
    state.detonation_tick = 0;
    state.destruction_settlement_phase0_ticks = 0;
    state.detonation_center_x = state.x + (canonical_drone_sprite_width >> 1);
    state.detonation_center_y = state.y + (canonical_drone_sprite_height >> 1);

    const auto previous_score = score.total;
    apply_drone_detonation_penalty(score);
    result.score_delta = score.total - previous_score;
    result.outcome_committed = commit_detonated_drone(mission);
    result.detonation_started = true;
    return result;
}

void advance_drone_detonation_tick(DroneObjectiveState& state) noexcept {
    if (state.detonation_tick < canonical_drone_detonation_tick_cap) {
        ++state.detonation_tick;
    }
}

DroneDetonationEffectTickResult step_drone_detonation_effect_logic(
    DroneObjectiveState& state,
    const std::int32_t gameplay_substep_phase,
    OriginalRandomState& random) noexcept {
    DroneDetonationEffectTickResult result{};

    if (gameplay_substep_phase != 0 ||
        state.detonation_tick <= canonical_drone_detonation_tick_effect_start ||
        state.activity != canonical_drone_destruction_activity) {
        return result;
    }

    result.logical_effect_tick = true;
    ++state.detonation_center_y;

    std::size_t request_index = 0;
    for (std::uint8_t i = 0;
         i < canonical_drone_detonation_center_explosions_per_effect_tick;
         ++i) {
        const auto random_x = next_original_random(random);
        const auto random_y = next_original_random(random);
        auto& request = result.explosions[request_index++];
        request.kind = DroneDetonationExplosionKind::CenterScatter;
        request.x = state.detonation_center_x - static_cast<std::int32_t>(random_x & 0x7fu) + 60;
        request.y = state.detonation_center_y - static_cast<std::int32_t>(random_y & 0x7fu) + 60;
        request.center_x = state.detonation_center_x;
        request.center_y = state.detonation_center_y;
    }

    result.radial_start_angle = original_random_mod(random, 90);
    const std::int32_t radius = (state.detonation_tick >> 1) + 5;
    for (std::uint8_t i = 0;
         i < canonical_drone_detonation_ring_explosions_per_effect_tick;
         ++i) {
        auto& request = result.explosions[request_index++];
        request.kind = DroneDetonationExplosionKind::RadialRing;
        request.center_x = state.detonation_center_x;
        request.center_y = state.detonation_center_y;
        request.angle_degrees = static_cast<std::uint16_t>(
            result.radial_start_angle + static_cast<std::uint16_t>(i) * 90u);
        request.radius = radius;
        request.jitter_x = static_cast<std::uint8_t>(next_original_random(random) & 0x1fu);
        request.jitter_y = static_cast<std::uint8_t>(next_original_random(random) & 0x1fu);
    }

    result.explosion_spawns_requested = static_cast<std::uint8_t>(request_index);
    result.random_draws_consumed = canonical_drone_detonation_random_draws_per_effect_tick;

    if (state.detonation_tick == canonical_drone_detonation_tick_settlement_reset) {
        state.destruction_settlement_phase0_ticks = 0;
        result.settlement_reset = true;
    }

    if (state.detonation_tick > canonical_drone_detonation_tick_settlement_reset) {
        ++state.destruction_settlement_phase0_ticks;
        result.settlement_advanced = true;
    }

    return result;
}

DroneObjectiveTickResult step_drone_objective_normal(
    DroneObjectiveState& state,
    const std::int32_t gameplay_substep_phase,
    MissionOutcomeState& mission,
    std::int32_t& settlement_tick) noexcept {
    DroneObjectiveTickResult result{};

    if (!is_win32_phase2(gameplay_substep_phase) ||
        state.activity == canonical_drone_destruction_activity) {
        return result;
    }

    // Before Probe completion the Drone advances only while above the Y=45
    // holding point. The original handles the -117/-40 sound landmarks before
    // the later completed-disarm movement path, hence the skips live here.
    if (!state.disarm_completed && state.y < canonical_drone_hover_y) {
        ++state.y;
        result.moved = true;
        const auto landmark = apply_approach_landmark_skip(state);
        result.approach_loop_start_landmark_reached =
            landmark == DroneApproachLandmark::LoopStart;
        result.approach_hint_landmark_reached =
            landmark == DroneApproachLandmark::HintOneShot;
    }

    // These comparisons happen after the unresolved approach movement but
    // before the completed-disarm +1 movement in the canonical routine.
    if (state.y == canonical_drone_hover_y - 1) {
        state.hover_phase2_ticks = 0;
    } else if (state.y == canonical_drone_hover_y) {
        if (state.hover_phase2_ticks < std::numeric_limits<std::uint16_t>::max()) {
            ++state.hover_phase2_ticks;
        }
        if (state.destruction_countdown > canonical_drone_destruction_countdown_trigger &&
            state.hover_phase2_ticks == canonical_drone_hover_timeout_phase2_ticks) {
            result.hover_timeout_reached = true;
            result.destruction_countdown_started =
                start_drone_destruction_countdown(state);
        }
    }

    // A completed Probe decode bypasses the unresolved approach increment and
    // receives its single +1 here instead. In ordinary play completion occurs
    // at the Y=45 hold, after which this carries the Drone through settlement.
    if (state.disarm_completed) {
        ++state.y;
        result.moved = true;
    }

    result.boss_approach_boundary_reached =
        drone_is_at_boss_approach_boundary(state.y);

    if (commit_disarmed_drone_at_boundary(mission, state.y, state.activity)) {
        result.disarm_committed = true;
        state.y = canonical_drone_post_disarm_y;
        result.moved = true;
    }

    if (state.y == canonical_drone_settlement_timer_reset_y) {
        settlement_tick = 0;
        result.settlement_tick_reset = true;
    }

    // The canonical code clears the completed-decode status once Y has moved
    // beyond 230. This intentionally freezes Y at 231 while the shared
    // settlement scalar continues toward its exact 60-tick transition gate.
    if (state.y > canonical_drone_transition_min_y) {
        result.disarm_completion_cleared = state.disarm_completed;
        state.disarm_completed = false;
    }

    result.resolution_transition_ready =
        drone_resolution_transition_ready(state.y, settlement_tick);
    return result;
}

} // namespace drone::gameplay
