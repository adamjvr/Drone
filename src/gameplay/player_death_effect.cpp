#include <drone/gameplay/player_death_effect.hpp>

#include <drone/gameplay/gameplay_phase.hpp>

namespace drone::gameplay {

void trigger_player_death_effect(
    PlayerDeathEffectState& state,
    const PlayerMotionState& player) noexcept {

    // center_entity_on_entity (0x00402770) centers the 42x38 singleton over the
    // 22x22 player and copies the player's common-entity motion fields.
    state.x = player.x + ((player_sprite_width - canonical_player_death_effect_width) / 2);
    state.y = player.y + ((player_sprite_height - canonical_player_death_effect_height) / 2);
    state.motion_x = player.horizontal_motion;
    // The recovered player-control path never assigns a vertical common-entity
    // motion component; canonical gameplay therefore contributes zero here.
    state.motion_y = 0;
    state.activity = PlayerDeathEffectActivity::PreRoll;
    state.frame = canonical_player_death_effect_initial_frame;
}

PlayerDeathEffectStepResult step_player_death_effect(
    PlayerDeathEffectState& state,
    const std::int32_t gameplay_substep_phase) noexcept {

    PlayerDeathEffectStepResult result{};
    if (!is_win32_phase2(gameplay_substep_phase) ||
        state.activity == PlayerDeathEffectActivity::Inactive) {
        return result;
    }

    result.advanced = true;
    state.x += state.motion_x;
    state.y += state.motion_y;

    // Original 0x0040E20C..0x0040E240 uses literal right/bottom bounds and
    // negated sprite width/height for the left/top tests.
    if (state.x > 319 ||
        state.x < -canonical_player_death_effect_width ||
        state.y > 199 ||
        state.y < -canonical_player_death_effect_height) {
        state.activity = PlayerDeathEffectActivity::Inactive;
        result.cleared_out_of_bounds = true;
    }

    // The code continues after an out-of-bounds clear. If the pre-roll frame
    // increments from -1 to 0 on that same update, activity is written back to
    // visible before the terminal-frame comparison. Preserve that oddity.
    ++state.frame;
    if (state.frame == 0) {
        state.activity = PlayerDeathEffectActivity::Visible;
        result.became_visible = true;
    }

    if (state.frame == canonical_player_death_effect_terminal_frame) {
        state.activity = PlayerDeathEffectActivity::Inactive;
        result.retired_at_terminal_frame = true;
    }

    return result;
}

} // namespace drone::gameplay
