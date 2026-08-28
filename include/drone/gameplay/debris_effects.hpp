#pragma once

#include <cstdint>

namespace drone::gameplay {

// Semantic reconstruction of the 0x18-byte particle records updated by
// Win32 0x00402B40 and 0x00402EF0. The original keeps group headers outside
// each record; this clean state intentionally models only one particle.
struct DebrisParticleState {
    std::int32_t x{};
    std::int32_t y{};
    std::int32_t velocity_x{};
    std::int32_t velocity_y{};
    std::int16_t age{};
    std::int16_t age_limit{};
    std::uint8_t visual_code{};
    bool active{};
};

// One primary 0x00472B00 particle update. The two random inputs are the low
// four bits of the original independent rand() calls. Passing rolls explicitly
// keeps the clean simulation deterministic without coupling it to a CRT RNG.
// Returns true iff this update retires a particle that was active on entry.
[[nodiscard]] bool advance_debris_particle(
    DebrisParticleState& particle,
    std::uint8_t gravity_roll_low4,
    std::uint8_t lifetime_roll_low4) noexcept;

// The secondary particle bank rooted at 0x00440FF8 uses the same record shape
// but a simpler lifetime rule: gravity is rand()%10 < 3, age increments every
// update, and the visual code decrements after the age threshold.
[[nodiscard]] bool advance_secondary_debris_particle(
    DebrisParticleState& particle,
    std::uint8_t gravity_roll_mod10) noexcept;

// The three 15-entry sprite-debris banks (junk1/junk2/wheel) are normal 0x154
// entities updated by 0x00403330. +0x32 is a signed byte frame step in this
// object family rather than a trajectory index.
struct DebrisSpriteState {
    std::int32_t x{};
    std::int32_t y{};
    std::int32_t velocity_x{};
    std::int32_t velocity_y{};
    std::int16_t sprite_width{};
    std::int16_t sprite_height{};
    std::uint8_t current_frame{};
    std::uint8_t frame_count{};
    std::int8_t frame_step{};
    bool active{};
};

inline constexpr std::uint8_t canonical_debris_sprite_pool_size = 15;

// gravity_roll_low7 is the original rand() & 0x7f result. Values below ten
// increase vertical velocity by one. Returns true iff the sprite retires by
// crossing the original fully-on-screen bounds.
[[nodiscard]] bool advance_debris_sprite(
    DebrisSpriteState& sprite,
    std::uint8_t gravity_roll_low7) noexcept;

} // namespace drone::gameplay
