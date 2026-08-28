#include <drone/gameplay/scoring.hpp>

namespace drone::gameplay {

void apply_score_delta(ScoreState& state, const std::int32_t delta) {
    state.total += delta;
    state.extra_life_progress += delta;
}

void apply_drone_detonation_penalty(ScoreState& state) {
    if (state.total < drone_detonation_penalty) {
        state.total = 0;
    } else {
        state.total -= drone_detonation_penalty;
    }
    state.extra_life_progress = 0;
}

bool consume_one_extra_life_threshold(ScoreState& state) {
    if (state.extra_life_progress < extra_life_points) {
        return false;
    }
    state.extra_life_progress -= extra_life_points;
    return true;
}

void normalize_score_for_hud(ScoreState& state) {
    if (state.total < 0) {
        state.total = 0;
    }
    if (state.extra_life_progress < 0) {
        state.extra_life_progress = 0;
    }
    if (state.total >= score_display_wrap_value) {
        state.total -= score_display_wrap_value;
    }
}

} // namespace drone::gameplay
