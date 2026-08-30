#include <drone/gameplay/special_weapon.hpp>

namespace drone::gameplay {

void advance_special_weapon_switch_progress(SpecialWeaponState& state) {
    if (state.activity == SpecialWeaponActivity::LoadedTracking &&
        state.switch_progress < state.switch_threshold) {
        ++state.switch_progress;
    }
}

bool load_special_weapon(
    SpecialWeaponState& state,
    const PlayerMotionState& player,
    const bool load_requested,
    const bool player_active) {

    if (!load_requested || !player_active ||
        state.activity != SpecialWeaponActivity::Inactive) {
        return false;
    }

    state.activity = SpecialWeaponActivity::LoadedTracking;
    state.y = player.y + 7;
    state.out_of_bounds = false;
    state.switch_progress = 0;
    return true;
}

bool toggle_loaded_special_weapon(
    SpecialWeaponState& state,
    const bool toggle_requested,
    const bool player_active) {

    if (!toggle_requested || !player_active ||
        state.activity != SpecialWeaponActivity::LoadedTracking ||
        state.switch_progress != state.switch_threshold) {
        return false;
    }

    state.kind = state.kind == SpecialWeaponKind::Probe
        ? SpecialWeaponKind::Stinger
        : SpecialWeaponKind::Probe;
    state.switch_progress = 0;
    return true;
}

bool launch_special_weapon(
    SpecialWeaponState& state,
    const bool launch_requested,
    const bool player_active) {

    if (!launch_requested || !player_active ||
        state.activity != SpecialWeaponActivity::LoadedTracking) {
        return false;
    }
    state.activity = SpecialWeaponActivity::LaunchedHoming;
    return true;
}

bool step_special_weapon_homing(
    SpecialWeaponState& state,
    const PlayerMotionState& player,
    const std::int32_t target_x) {

    if (state.activity != SpecialWeaponActivity::LoadedTracking &&
        state.activity != SpecialWeaponActivity::LaunchedHoming) {
        return false;
    }

    if (state.activity == SpecialWeaponActivity::LoadedTracking) {
        state.x = player.x + 14;
        state.y = player.y + 7;
    }

    state.y -= 2;
    if (state.x < target_x) {
        ++state.x;
    } else if (state.x > target_x) {
        --state.x;
    }

    // The canonical update path first clears activity when X leaves 0..319 and
    // raises +0x143.  The later common visibility block then handles the Y
    // lifecycle independently: Y<0 raises +0x143, and Y<-60 fully retires the
    // special entity back to its canonical staging Y=182 while clearing the
    // edge flag.  This delayed top retirement is what makes missed Probes and
    // Stingers reusable without inventing an ammo counter.
    if (state.x < 0 || state.x > 319) {
        state.activity = SpecialWeaponActivity::Inactive;
        state.out_of_bounds = true;
    }

    if (state.y < 0) {
        state.out_of_bounds = true;
        if (state.y < -60) {
            state.activity = SpecialWeaponActivity::Inactive;
            state.y = 182;
            state.out_of_bounds = false;
        }
    } else if (state.y > 199) {
        // This lower-edge reset is also present in the same Win32 block.  The
        // special projectile normally travels upward, but preserve the proven
        // common-entity boundary behavior for completeness.
        state.activity = SpecialWeaponActivity::Inactive;
        state.y = 182;
        state.out_of_bounds = false;
    }
    return true;
}

std::int32_t stinger_target_x(const std::int32_t target_x, const std::int16_t target_width) {
    return target_x + static_cast<std::int32_t>(target_width) / 2;
}

std::int32_t probe_drone_target_x(const std::int32_t drone_x) {
    return drone_x + 4;
}

bool attach_probe_to_drone(SpecialWeaponState& state) {
    if (state.activity != SpecialWeaponActivity::LaunchedHoming ||
        state.kind != SpecialWeaponKind::Probe) {
        return false;
    }
    state.activity = SpecialWeaponActivity::ProbeAttachedDecoding;
    state.motion_x = 0;
    state.motion_y = 0;
    return true;
}


bool pin_attached_probe_to_drone(SpecialWeaponState& state, const std::int32_t drone_x) {
    if (state.activity != SpecialWeaponActivity::ProbeAttachedDecoding) {
        return false;
    }
    state.x = drone_x + 5;
    return true;
}

bool enter_special_weapon_hole_interaction(SpecialWeaponState& state) {
    if (state.activity != SpecialWeaponActivity::LaunchedHoming) {
        return false;
    }
    state.activity = SpecialWeaponActivity::HoleInteraction;
    return true;
}

bool settle_special_weapon_terminal_state(SpecialWeaponState& state) {
    if (state.activity != SpecialWeaponActivity::ImpactConsumed) {
        return false;
    }
    state.activity = SpecialWeaponActivity::Inactive;
    return true;
}

void initialize_probe_decode_timing(
    ProbeDecodeState& decode,
    const DifficultyLevel difficulty,
    const bool demo_playback_mode,
    OriginalRandomState& random) noexcept {
    decode.status = ProbeDecodeStatus::Phase1Decoding;
    decode.phase1_elapsed = 0;
    decode.phase2_elapsed = 0;

    if (demo_playback_mode) {
        decode.phase1_threshold = 210;
        decode.phase2_threshold = 150;
        return;
    }

    const auto multiplier = difficulty_multiplier(difficulty);
    decode.phase1_threshold = static_cast<std::uint16_t>(
        (original_random_mod(random, 70) + 450u) * multiplier);
    decode.phase2_threshold = static_cast<std::uint16_t>(
        (original_random_mod(random, 70) + 300u) * multiplier);
}

ProbeDecodeTickResult step_probe_decode(
    SpecialWeaponState& state,
    OriginalRandomState& random,
    ScoreState& score) noexcept {
    ProbeDecodeTickResult result{};
    auto& decode = state.probe_decode;

    if (state.activity != SpecialWeaponActivity::ProbeAttachedDecoding ||
        decode.status == ProbeDecodeStatus::Complete) {
        return result;
    }

    if (decode.status == ProbeDecodeStatus::Phase1Decoding) {
        ++decode.phase1_elapsed;
        if (decode.phase1_elapsed == decode.phase1_threshold) {
            decode.status = ProbeDecodeStatus::Phase2Disarming;
            decode.phase2_elapsed = 0;
            result.phase1_completed = true;
        }
    }

    // Deliberately not `else if`: the Win32 block falls through after writing
    // status 3, so phase 2 advances from zero to one on the same update.
    if (decode.status == ProbeDecodeStatus::Phase2Disarming) {
        ++decode.phase2_elapsed;
        if (decode.phase2_elapsed == decode.phase2_threshold) {
            result.completion_effect_random = static_cast<std::uint16_t>(
                original_random_mod(random, 60) + 40u);
            apply_score_delta(score, drone_disarm_score_award);
            decode.status = ProbeDecodeStatus::Complete;
            result.disarm_completed = true;
            result.score_delta = drone_disarm_score_award;
        }
    }

    return result;
}

bool clear_completed_probe_decode(SpecialWeaponState& state) noexcept {
    auto& decode = state.probe_decode;
    if (decode.status != ProbeDecodeStatus::Complete) {
        return false;
    }
    decode.status = ProbeDecodeStatus::Phase1Decoding;
    decode.phase1_elapsed = 0;
    decode.phase2_elapsed = 0;
    return true;
}

} // namespace drone::gameplay
