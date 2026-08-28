#pragma once

#include <cstdint>
#include <span>

namespace drone::gameplay {

struct Point {
    std::int32_t x{};
    std::int32_t y{};
};

// Semantic view of the fields consumed by the recovered Win32 collision
// primitives. This deliberately does not expose the original raw 0x154-byte
// object layout; that layout belongs to the reverse-engineering evidence layer.
struct CollisionEntityView {
    std::int32_t x{};
    std::int32_t y{};
    std::int16_t sprite_width{};
    std::int16_t sprite_height{};
    std::int16_t hitbox_width{};
    std::int16_t hitbox_height{};
};

// Reconstructs Win32 0x00401F60. The original uses inclusive comparisons at
// the right/bottom hitbox boundary.
[[nodiscard]] bool point_in_hitbox(Point point, const CollisionEntityView& entity) noexcept;

// Bounds-safe semantic reconstruction of Win32 0x00401FA0. Palette index zero
// is transparent in the original renderer/collision path. The original binary
// uses inclusive outer comparisons before indexing; this implementation rejects
// coordinates outside the actual frame span rather than reproducing an unsafe
// out-of-bounds read at an ambiguous right/bottom edge.
[[nodiscard]] bool point_hits_opaque_pixel(
    Point point,
    const CollisionEntityView& entity,
    std::span<const std::uint8_t> frame) noexcept;

// Reconstructs the distinct Win32 0x00402000 probe: the point's Y coordinate is
// shifted by +9 before the same inclusive hitbox test. Its gameplay-level role
// remains deliberately unnamed in the reverse-engineering ledger.
[[nodiscard]] bool point_plus_y9_in_hitbox(Point point, const CollisionEntityView& entity) noexcept;

// Reconstructs the asymmetric Win32 0x00402FC0 broad-phase overlap test.
// The first entity contributes a collision rectangle centered inside its
// sprite from the +0x28/+0x2A extents; the second contributes its full sprite
// rectangle. The original comparisons are inclusive.
[[nodiscard]] bool entity_hitbox_overlaps_sprite_rect(
    const CollisionEntityView& hitbox_entity,
    const CollisionEntityView& sprite_entity) noexcept;

} // namespace drone::gameplay
