#include <drone/gameplay/gameplay_phase.hpp>

#include <cstdint>

namespace drone::gameplay {

std::int32_t advance_win32_gameplay_substep_phase(std::int32_t current_phase) noexcept {
    // Win32 0x0040C001..0x0040C01D keeps the increment when the old signed
    // value is < 3 and otherwise replaces the just-incremented value with 0.
    // The valid runtime cycle is therefore four phases, not three.
    const auto next_bits = static_cast<std::uint32_t>(current_phase) + 1U;
    const auto incremented = static_cast<std::int32_t>(next_bits);
    return current_phase < 3 ? incremented : 0;
}

} // namespace drone::gameplay
