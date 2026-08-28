#include <drone/fidelity/hud_presentation.hpp>

#include <algorithm>

namespace drone::fidelity {

HudTextPlacement score_text_placement(const std::int32_t displayed_score) noexcept {
    std::int32_t x = 309;
    if (displayed_score > 999) {
        x = 285;
    } else if (displayed_score > 99) {
        x = 293;
    } else if (displayed_score > 9) {
        x = 301;
    }
    return {x, 190, 28};
}

HudTextPlacement lives_text_placement() noexcept {
    return {309, 180, 28};
}

std::array<DroneOutcomeMarker, drone_outcome_marker_count>
plan_drone_outcome_markers(
    const std::array<std::uint8_t, drone_outcome_marker_count>& raw_outcomes) noexcept {
    std::array<DroneOutcomeMarker, drone_outcome_marker_count> result{};
    std::int32_t y = 160;
    for (std::size_t i = 0; i < result.size(); ++i, y -= 19) {
        result[i].x = 3;
        result[i].y = y;
        const auto raw = raw_outcomes[i];
        if (raw >= 1 && raw <= 3) {
            result[i].visible = true;
            result[i].frame_index = static_cast<std::uint8_t>(raw - 1);
        }
    }
    return result;
}

std::int32_t drone_outcome_cursor_target_y(
    const std::uint8_t processed_outcomes) noexcept {
    const auto bounded = std::min<std::uint8_t>(processed_outcomes, drone_outcome_marker_count);
    return drone_outcome_cursor_initial_y -
           static_cast<std::int32_t>(bounded) * drone_outcome_cursor_spacing;
}

DroneOutcomeCursorPlan plan_drone_outcome_cursor(
    const std::uint8_t processed_outcomes,
    const std::int32_t current_y) noexcept {
    DroneOutcomeCursorPlan plan{};
    plan.visible = processed_outcomes < drone_outcome_marker_count;
    plan.y = current_y;
    plan.target_y = drone_outcome_cursor_target_y(processed_outcomes);
    return plan;
}

void advance_drone_outcome_cursor_y(
    std::int32_t& current_y,
    const std::int32_t target_y,
    const std::uint8_t gameplay_phase) noexcept {
    if (gameplay_phase == 2 && current_y > target_y) {
        --current_y;
    }
}

SpecialWeaponHudPlan plan_special_weapon_status(
    const SpecialWeaponHudInputs& inputs,
    SpecialWeaponHudTimers timers) noexcept {
    SpecialWeaponHudPlan result{};
    result.next_timers = timers;
    result.placement = {5, 190, 28};

    switch (inputs.activity_state) {
    case 0:
        if (timers.miss_hold < 110) {
            ++result.next_timers.miss_hold;
            result.status = SpecialWeaponHudStatus::Miss;
            result.visible = true;
            result.text = "MISS";
        }
        break;
    case 1:
        result.status = SpecialWeaponHudStatus::Ready;
        result.visible = true;
        result.text = "READY";
        break;
    case 2:
        if (inputs.decode_phase1_elapsed > 1 &&
            inputs.decode_phase1_elapsed < inputs.decode_phase1_threshold) {
            result.status = SpecialWeaponHudStatus::Decoding;
            result.visible = true;
            result.text = "DECODING";
            break;
        }
        if (inputs.decode_phase2_elapsed > 1 &&
            inputs.decode_phase2_elapsed < inputs.decode_phase2_threshold) {
            result.next_timers.disarmed_hold = 1;
            result.status = SpecialWeaponHudStatus::Disarming;
            result.visible = true;
            result.text = "DISARMING";
            break;
        }
        if (timers.disarmed_hold < 200) {
            ++result.next_timers.disarmed_hold;
            result.status = SpecialWeaponHudStatus::Disarmed;
            result.visible = true;
            result.placement.palette_index = 57;
            result.text = "DISARMED!";
        }
        break;
    case 3:
        result.status = SpecialWeaponHudStatus::Seeking;
        result.visible = true;
        result.text = "SEEKING";
        break;
    default:
        break;
    }
    return result;
}

ReticlePlacement special_target_reticle_placement(const ReticleTargetRect& target) noexcept {
    std::int32_t x = target.x + static_cast<std::int32_t>(target.width) / 2 - 8;
    std::int32_t y = target.y + static_cast<std::int32_t>(target.height) / 2 - 8;

    if (x < 0) x = 0;
    if (x > 302) x = 301; // Original compares against 319-17, then writes 301.
    if (y < 0) y = 0;
    if (y > 186) y = 185; // Original compares against 199-13, then writes 185.
    return {x, y};
}

std::array<ShieldMeterRow, shield_meter_max_rows>
plan_shield_meter_rows(const std::int32_t displayed_energy_units) noexcept {
    std::array<ShieldMeterRow, shield_meter_max_rows> rows{};
    const auto count = std::clamp(displayed_energy_units, 0, shield_meter_max_rows);
    for (std::int32_t i = 0; i < count; ++i) {
        auto color = static_cast<std::uint8_t>(27);
        if (i > 10) color = 57;
        if (i > 25) color = 28;
        rows[static_cast<std::size_t>(i)] = {
            shield_meter_x,
            shield_meter_bottom_y - i,
            shield_meter_width,
            color,
        };
    }
    return rows;
}

} // namespace drone::fidelity
