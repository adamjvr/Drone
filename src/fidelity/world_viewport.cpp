#include <drone/fidelity/world_viewport.hpp>

#include <algorithm>
#include <cstddef>

namespace drone::fidelity {
namespace {

bool copy_cyclic_world_rows(
    std::span<const std::uint8_t> world,
    std::span<std::uint8_t> framebuffer,
    const std::int32_t scroll_row,
    const std::int32_t destination_row,
    const std::int32_t row_count) noexcept {
    if (world.size() < scenery_world_bytes || framebuffer.size() < logical_viewport_bytes) {
        return false;
    }
    if (scroll_row < 0 || scroll_row >= scenery_world_height) return false;
    if (destination_row < 0 || row_count < 0 ||
        destination_row + row_count > logical_viewport_height) {
        return false;
    }

    const auto first_rows = std::min(row_count, scenery_world_height - scroll_row);
    const auto first_bytes = static_cast<std::size_t>(first_rows) * logical_width;
    const auto source_offset = static_cast<std::size_t>(scroll_row) * logical_width;
    const auto destination_offset = static_cast<std::size_t>(destination_row) * logical_width;

    std::copy_n(world.begin() + static_cast<std::ptrdiff_t>(source_offset),
                first_bytes,
                framebuffer.begin() + static_cast<std::ptrdiff_t>(destination_offset));

    const auto total_bytes = static_cast<std::size_t>(row_count) * logical_width;
    const auto remaining_bytes = total_bytes - first_bytes;
    if (remaining_bytes != 0U) {
        std::copy_n(world.begin(),
                    remaining_bytes,
                    framebuffer.begin() + static_cast<std::ptrdiff_t>(
                        destination_offset + first_bytes));
    }
    return true;
}

} // namespace

bool compose_scrolling_world_viewport(
    std::span<const std::uint8_t> world,
    std::span<std::uint8_t> framebuffer,
    const std::int32_t scroll_row) noexcept {
    return copy_cyclic_world_rows(
        world, framebuffer, scroll_row, 0, logical_viewport_height);
}

bool compose_ordering_information_world_background(
    std::span<const std::uint8_t> world,
    std::span<std::uint8_t> framebuffer,
    const std::int32_t scroll_row) noexcept {
    return copy_cyclic_world_rows(
        world,
        framebuffer,
        scroll_row,
        ordering_information_background_top_row,
        ordering_information_background_height);
}

} // namespace drone::fidelity
