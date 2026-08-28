#pragma once

#include <cstdint>

namespace drone::gameplay {

// Exact state transition used by Win32 0x00421ED0 (the statically linked MSVC
// rand implementation). The original seeds this process-global state once at
// startup; gameplay/session resets do not reseed it.
struct OriginalRandomState {
    std::uint32_t state = 1;
    std::uint64_t draws = 0; // clean diagnostic, not an original global
};

void seed_original_random(OriginalRandomState& random, std::uint32_t seed) noexcept;

[[nodiscard]] std::uint16_t next_original_random(OriginalRandomState& random) noexcept;

[[nodiscard]] std::uint16_t original_random_mod(
    OriginalRandomState& random,
    std::uint16_t modulus) noexcept;

} // namespace drone::gameplay
