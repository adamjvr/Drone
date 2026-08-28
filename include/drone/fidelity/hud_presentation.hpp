#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace drone::fidelity {

struct HudTextPlacement {
    std::int32_t x{};
    std::int32_t y{};
    std::uint8_t palette_index{};
};

[[nodiscard]] HudTextPlacement score_text_placement(std::int32_t displayed_score) noexcept;
[[nodiscard]] HudTextPlacement lives_text_placement() noexcept;

struct DroneOutcomeMarker {
    bool visible{};
    std::uint8_t frame_index{};
    std::int32_t x{};
    std::int32_t y{};
};

inline constexpr std::size_t drone_outcome_marker_count = 6;

// The original renderer accepts raw outcome values 1..3 for frames 0..2.
// Canonical shareware gameplay currently establishes only 0,1,2; raw value 3
// is retained here because the renderer explicitly supports the third loaded
// mini-probe frame.
[[nodiscard]] std::array<DroneOutcomeMarker, drone_outcome_marker_count>
plan_drone_outcome_markers(const std::array<std::uint8_t, drone_outcome_marker_count>& raw_outcomes) noexcept;

// square.jba is a 13x18 green outline used as the current six-Drone outcome
// slot cursor. It starts at (2,159), one pixel above/left of the first mini-
// probe marker, and its destination moves upward by the same 19-pixel spacing
// whenever an outcome is committed. Current Y chases target Y by one pixel on
// gameplay phase 2 only. After all six outcomes are processed, the renderer
// disables the cursor.
inline constexpr std::int32_t drone_outcome_cursor_x = 2;
inline constexpr std::int32_t drone_outcome_cursor_initial_y = 159;
inline constexpr std::int32_t drone_outcome_cursor_spacing = 19;
inline constexpr std::int16_t drone_outcome_cursor_width = 13;
inline constexpr std::int16_t drone_outcome_cursor_height = 18;

struct DroneOutcomeCursorPlan {
    bool visible{};
    std::int32_t x{drone_outcome_cursor_x};
    std::int32_t y{drone_outcome_cursor_initial_y};
    std::int32_t target_y{drone_outcome_cursor_initial_y};
};

[[nodiscard]] std::int32_t drone_outcome_cursor_target_y(
    std::uint8_t processed_outcomes) noexcept;

[[nodiscard]] DroneOutcomeCursorPlan plan_drone_outcome_cursor(
    std::uint8_t processed_outcomes,
    std::int32_t current_y) noexcept;

void advance_drone_outcome_cursor_y(
    std::int32_t& current_y,
    std::int32_t target_y,
    std::uint8_t gameplay_phase) noexcept;

enum class SpecialWeaponHudStatus : std::uint8_t {
    Hidden,
    Miss,
    Ready,
    Seeking,
    Decoding,
    Disarming,
    Disarmed,
};

struct SpecialWeaponHudTimers {
    std::int16_t miss_hold{};
    std::int16_t disarmed_hold{};
};

struct SpecialWeaponHudInputs {
    std::uint8_t activity_state{};
    std::int16_t decode_phase1_elapsed{};
    std::int16_t decode_phase1_threshold{};
    std::int16_t decode_phase2_elapsed{};
    std::int16_t decode_phase2_threshold{};
};

struct SpecialWeaponHudPlan {
    SpecialWeaponHudStatus status{SpecialWeaponHudStatus::Hidden};
    SpecialWeaponHudTimers next_timers{};
    bool visible{};
    HudTextPlacement placement{};
    std::string_view text{};
};

[[nodiscard]] SpecialWeaponHudPlan plan_special_weapon_status(
    const SpecialWeaponHudInputs& inputs,
    SpecialWeaponHudTimers timers) noexcept;

struct ReticleTargetRect {
    std::int32_t x{};
    std::int32_t y{};
    std::int16_t width{};
    std::int16_t height{};
};

struct ReticlePlacement {
    std::int32_t x{};
    std::int32_t y{};
    bool operator==(const ReticlePlacement&) const = default;
};

// Original target.jba reticle is 17x13. The Win32 renderer centers an implied
// 16x16 reticle footprint using -8 and then clamps one pixel inside the
// nominal right/bottom thresholds: x<=301 and y<=185.
[[nodiscard]] ReticlePlacement special_target_reticle_placement(const ReticleTargetRect& target) noexcept;

struct ShieldMeterRow {
    std::int32_t x{};
    std::int32_t y{};
    std::int32_t width{};
    std::uint8_t palette_index{};
};

inline constexpr std::int32_t shield_meter_x = 313;
inline constexpr std::int32_t shield_meter_bottom_y = 138;
inline constexpr std::int32_t shield_meter_width = 4;
inline constexpr std::int32_t shield_meter_max_rows = 75;

[[nodiscard]] std::array<ShieldMeterRow, shield_meter_max_rows>
plan_shield_meter_rows(std::int32_t displayed_energy_units) noexcept;

} // namespace drone::fidelity
