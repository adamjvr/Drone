#pragma once

#include <cstdint>
#include <span>

namespace drone::fidelity {

inline constexpr std::int32_t logical_width = 320;
inline constexpr std::int32_t logical_viewport_height = 200;
inline constexpr std::int32_t scenery_world_height = 600;
inline constexpr std::size_t logical_viewport_bytes =
    static_cast<std::size_t>(logical_width) * logical_viewport_height;
inline constexpr std::size_t scenery_world_bytes =
    static_cast<std::size_t>(logical_width) * scenery_world_height;

inline constexpr std::int32_t ordering_information_background_top_row = 35;
inline constexpr std::int32_t ordering_information_background_height = 145;
inline constexpr std::size_t ordering_information_background_bytes =
    static_cast<std::size_t>(logical_width) * ordering_information_background_height;

// Bounds-safe reconstruction of Win32 0x004033D0. The canonical world is
// three stacked 320x200 indexed screens. scroll_row is in [0,599]; rows
// 0..400 copy contiguously, while 401..599 split at row 600 and wrap to row 0.
// Returns false for an invalid canonical row or undersized buffers.
[[nodiscard]] bool compose_scrolling_world_viewport(
    std::span<const std::uint8_t> world,
    std::span<std::uint8_t> framebuffer,
    std::int32_t scroll_row) noexcept;

// Reconstruction of Win32 0x00403560, used only by the ordering-information
// modal. It copies the same cyclic 320x600 world into framebuffer rows 35..179
// (145 rows), preserving the modal's top/bottom UI bands.
[[nodiscard]] bool compose_ordering_information_world_background(
    std::span<const std::uint8_t> world,
    std::span<std::uint8_t> framebuffer,
    std::int32_t scroll_row) noexcept;

} // namespace drone::fidelity
