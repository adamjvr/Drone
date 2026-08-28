#include <drone/gameplay/debris_effects.hpp>

#include <cstdint>

namespace drone::gameplay {
namespace {

std::int16_t increment_word(std::int16_t value) noexcept {
    const auto bits = static_cast<std::uint16_t>(value);
    return static_cast<std::int16_t>(static_cast<std::uint16_t>(bits + 1U));
}

bool outside_particle_screen(std::int32_t x, std::int32_t y) noexcept {
    return x < 0 || x > 319 || y < 0 || y > 199;
}

void advance_primary_visual_code(DebrisParticleState& particle) noexcept {
    switch (particle.visual_code) {
    case 0x34: particle.visual_code = 0x39; break;
    case 0x35: particle.visual_code = 0x34; break;
    case 0x36: particle.visual_code = 0x35; break;
    case 0x37: particle.visual_code = 0x0B; break;
    case 0x38: particle.visual_code = 0x37; break;
    case 0x39: particle.visual_code = 0x38; break;
    default:
        particle.visual_code = static_cast<std::uint8_t>(particle.visual_code - 1U);
        if (particle.visual_code < 3U) particle.active = false;
        break;
    }
}

} // namespace

bool advance_debris_particle(
    DebrisParticleState& particle,
    std::uint8_t gravity_roll_low4,
    std::uint8_t lifetime_roll_low4) noexcept {
    if (!particle.active) return false;

    particle.x += particle.velocity_x;
    particle.y += particle.velocity_y;
    if (outside_particle_screen(particle.x, particle.y)) particle.active = false;

    if ((gravity_roll_low4 & 0x0FU) < 6U) ++particle.velocity_y;

    if ((lifetime_roll_low4 & 0x0FU) < 11U) {
        particle.age = increment_word(particle.age);
        if (particle.age > particle.age_limit) advance_primary_visual_code(particle);
    }

    return !particle.active;
}

bool advance_secondary_debris_particle(
    DebrisParticleState& particle,
    std::uint8_t gravity_roll_mod10) noexcept {
    if (!particle.active) return false;

    particle.x += particle.velocity_x;
    particle.y += particle.velocity_y;
    if (outside_particle_screen(particle.x, particle.y)) particle.active = false;

    if ((gravity_roll_mod10 % 10U) < 3U) ++particle.velocity_y;

    particle.age = increment_word(particle.age);
    if (particle.age > particle.age_limit) {
        particle.visual_code = static_cast<std::uint8_t>(particle.visual_code - 1U);
        if (particle.visual_code < 3U) particle.active = false;
    }

    return !particle.active;
}

bool advance_debris_sprite(
    DebrisSpriteState& sprite,
    std::uint8_t gravity_roll_low7) noexcept {
    if (!sprite.active) return false;

    sprite.x += sprite.velocity_x;
    sprite.y += sprite.velocity_y;

    const auto max_x = 319 - static_cast<std::int32_t>(sprite.sprite_width);
    const auto max_y = 199 - static_cast<std::int32_t>(sprite.sprite_height);
    if (sprite.x < 0 || sprite.x > max_x || sprite.y < 0 || sprite.y > max_y) {
        sprite.active = false;
        return true;
    }

    if ((gravity_roll_low7 & 0x7FU) < 10U) ++sprite.velocity_y;

    const auto frame_bits = static_cast<std::uint8_t>(
        static_cast<std::uint8_t>(sprite.current_frame) +
        static_cast<std::uint8_t>(sprite.frame_step));
    sprite.current_frame = frame_bits;

    if (sprite.current_frame == sprite.frame_count) {
        sprite.current_frame = 0;
    } else if (static_cast<std::int8_t>(sprite.current_frame) < 0) {
        sprite.current_frame = static_cast<std::uint8_t>(sprite.frame_count - 1U);
    }

    return false;
}

} // namespace drone::gameplay
