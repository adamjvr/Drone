#pragma once

#include <cstdint>

namespace drone::gameplay {

// Canonical DOS fidelity cadence.
//
// The DOS executable explicitly selects BIOS VGA mode 13h and, in ordinary
// sync-enabled gameplay, performs one Tab-gated VGA retrace wait at the tail
// of each logical gameplay update. Standard mode-13h timing uses the classic
// 25.175 MHz VGA pixel clock, 800 clocks per scanline, and 449 total scanlines.
//
// This is the historical DOS fidelity target. It is intentionally NOT used to
// reinterpret the later Win32 port's non-normalized 15,000-count QPC limiter.
inline constexpr std::uint64_t dos_mode13_pixel_clock_hz = 25'175'000;
inline constexpr std::uint32_t dos_mode13_clocks_per_scanline = 800;
inline constexpr std::uint32_t dos_mode13_scanlines_per_frame = 449;
inline constexpr std::uint64_t dos_mode13_clocks_per_frame =
    static_cast<std::uint64_t>(dos_mode13_clocks_per_scanline) *
    dos_mode13_scanlines_per_frame;

inline constexpr double canonical_dos_fidelity_tick_hz =
    static_cast<double>(dos_mode13_pixel_clock_hz) /
    static_cast<double>(dos_mode13_clocks_per_frame);

inline constexpr double canonical_dos_fidelity_tick_seconds =
    static_cast<double>(dos_mode13_clocks_per_frame) /
    static_cast<double>(dos_mode13_pixel_clock_hz);

[[nodiscard]] constexpr double canonical_dos_duration_for_updates(
    std::uint32_t updates) noexcept {
    return static_cast<double>(updates) * canonical_dos_fidelity_tick_seconds;
}

} // namespace drone::gameplay
