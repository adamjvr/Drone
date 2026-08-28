#pragma once

#include <cstdint>

namespace drone::gameplay {

inline constexpr std::int32_t canonical_world_scroll_row_count = 600;
inline constexpr std::int32_t canonical_world_scroll_initial_row = 599;
inline constexpr std::int32_t canonical_drone_boss_approach_y = -200;
inline constexpr std::int32_t canonical_drone_session_initial_x = 155;
inline constexpr std::int32_t canonical_drone_session_initial_y = -850;

// Exact canonical row step shared by the active-gameplay and ordering-info
// scrolling backgrounds. The original decrements a signed row and replaces
// exactly -1 with 599. Callers own cadence/gating.
[[nodiscard]] std::int32_t advance_cyclic_world_scroll_row(
    std::int32_t current_row) noexcept;

// Win32 state 2 advances the scenery row only when the shared four-phase
// gameplay substep has just reached phase 2.
[[nodiscard]] std::int32_t advance_gameplay_world_scroll_row(
    std::int32_t current_row,
    std::int32_t gameplay_substep_phase) noexcept;

// The ordering-information modal uses its own three-step local counter and
// applies the same row decrement on local phase 2. This counter is unrelated
// to the four-phase state-2 gameplay scheduler.
[[nodiscard]] std::int32_t advance_ordering_information_scroll_phase(
    std::int32_t current_phase) noexcept;

[[nodiscard]] std::int32_t advance_ordering_information_world_scroll_row(
    std::int32_t current_row,
    std::int32_t ordering_scroll_phase) noexcept;

// 0x00446080 is the canonical drone.jba entity root. +0x04 therefore names
// the Drone target entity's Y coordinate, not a second world-scroll scalar.
[[nodiscard]] constexpr bool drone_is_at_boss_approach_boundary(
    std::int32_t drone_position_y) noexcept {
    return drone_position_y == canonical_drone_boss_approach_y;
}

// State-2 settlement code rebuilds an off-screen Drone approach Y from the
// number of already processed Drone objectives as (-7-count)*150.
[[nodiscard]] constexpr std::int32_t drone_reentry_y_for_processed_count(
    std::int32_t processed_count) noexcept {
    return (-7 - processed_count) * 150;
}

} // namespace drone::gameplay
