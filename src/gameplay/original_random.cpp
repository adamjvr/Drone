#include <drone/gameplay/original_random.hpp>

namespace drone::gameplay {

void seed_original_random(OriginalRandomState& random, const std::uint32_t seed) noexcept {
    random.state = seed;
    random.draws = 0;
}

std::uint16_t next_original_random(OriginalRandomState& random) noexcept {
    // 0x00421ED0 reduces algebraically to the classic MSVC CRT LCG.
    random.state = random.state * 214013u + 2531011u;
    ++random.draws;
    return static_cast<std::uint16_t>((random.state >> 16u) & 0x7fffu);
}

std::uint16_t original_random_mod(
    OriginalRandomState& random,
    const std::uint16_t modulus) noexcept {
    if (modulus == 0) {
        return 0;
    }
    return static_cast<std::uint16_t>(next_original_random(random) % modulus);
}

} // namespace drone::gameplay
