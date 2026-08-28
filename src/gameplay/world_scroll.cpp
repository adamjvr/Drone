#include <drone/gameplay/world_scroll.hpp>

namespace drone::gameplay {

std::int32_t advance_cyclic_world_scroll_row(const std::int32_t current_row) noexcept {
    const auto next = current_row - 1;
    return next == -1 ? canonical_world_scroll_initial_row : next;
}

std::int32_t advance_gameplay_world_scroll_row(
    const std::int32_t current_row,
    const std::int32_t gameplay_substep_phase) noexcept {
    return gameplay_substep_phase == 2
        ? advance_cyclic_world_scroll_row(current_row)
        : current_row;
}

std::int32_t advance_ordering_information_scroll_phase(
    const std::int32_t current_phase) noexcept {
    const auto next = current_phase + 1;
    return next == 3 ? 0 : next;
}

std::int32_t advance_ordering_information_world_scroll_row(
    const std::int32_t current_row,
    const std::int32_t ordering_scroll_phase) noexcept {
    return ordering_scroll_phase == 2
        ? advance_cyclic_world_scroll_row(current_row)
        : current_row;
}

} // namespace drone::gameplay
