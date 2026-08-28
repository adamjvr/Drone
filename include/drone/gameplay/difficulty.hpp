#pragma once

#include <cstdint>

namespace drone::gameplay {

// Exact user-facing values stored by Win32 global 0x0042B1A8.
enum class DifficultyLevel : std::uint8_t {
    Beginner = 1,
    Intermediate = 2,
    Advanced = 3,
};

[[nodiscard]] constexpr std::uint16_t difficulty_multiplier(
    const DifficultyLevel difficulty) noexcept {
    return static_cast<std::uint16_t>(difficulty);
}

} // namespace drone::gameplay
