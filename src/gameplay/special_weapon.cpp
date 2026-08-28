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

    // The canonical update path clears activity when X leaves 0..319 and sets
    // +0x143. Y termination is handled by later special-weapon logic and is
    // deliberately not invented here.
    if (state.x < 0 || state.x > 319) {
        state.activity = SpecialWeaponActivity::Inactive;
        state.out_of_bounds = true;
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

} // namespace drone::gameplay
