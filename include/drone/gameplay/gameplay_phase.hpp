#pragma once

#include <cstdint>

namespace drone::gameplay {

// Shared Win32 state-2 substep phase at original global 0x0053C4D8. The
// gameplay orchestrator advances it once near the start of every state-2
// update. On its canonical domain it cycles 0 -> 1 -> 2 -> 3 -> 0.
[[nodiscard]] std::int32_t advance_win32_gameplay_substep_phase(
    std::int32_t current_phase) noexcept;

[[nodiscard]] constexpr bool is_win32_phase2(std::int32_t phase) noexcept {
    return phase == 2;
}

} // namespace drone::gameplay
