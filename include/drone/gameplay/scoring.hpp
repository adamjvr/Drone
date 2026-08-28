#pragma once

#include <cstdint>

namespace drone::gameplay {

inline constexpr std::int32_t extra_life_points = 500;
inline constexpr std::int32_t probe_attachment_score_award = 10;
inline constexpr std::int32_t drone_disarm_score_award = 500;
inline constexpr std::int32_t drone_detonation_penalty = 1000;
inline constexpr std::int32_t score_display_wrap_value = 9999;

// The Win32 build keeps total score and a separate rolling extra-life progress
// accumulator. Normal score awards/escape penalties mutate both; special Drone
// detonation handling instead resets extra-life progress to zero.
struct ScoreState {
    std::int32_t total = 0;
    std::int32_t extra_life_progress = 0;
};

// Normal event delta used by recovered enemy/probe paths. Positive awards and
// ordinary escape penalties apply the same signed delta to both fields.
void apply_score_delta(ScoreState& state, std::int32_t delta);

// Dedicated Drone-detonation consequence recovered from Win32 0x0041D220:
// score loses 1000 with a floor at zero and extra-life progress is cleared.
void apply_drone_detonation_penalty(ScoreState& state);

// State 2 performs at most one 500-point conversion per update. Returning true
// means callers should increment the life count once. Remainder is preserved.
bool consume_one_extra_life_threshold(ScoreState& state);

// The HUD path normalizes negative score/progress to zero. It also performs the
// original one-step score wrap when total >= 9999. This is intentionally not a
// conventional clamp/modulo helper: one call mirrors one original HUD pass.
void normalize_score_for_hud(ScoreState& state);

} // namespace drone::gameplay
