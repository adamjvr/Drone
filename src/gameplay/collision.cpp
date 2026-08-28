#include <drone/gameplay/collision.hpp>

#include <cstddef>
#include <limits>

namespace drone::gameplay {
namespace {

bool in_inclusive_extent(
    std::int32_t value,
    std::int32_t origin,
    std::int16_t extent) noexcept {
    const auto end = static_cast<std::int64_t>(origin) + static_cast<std::int64_t>(extent);
    const auto v = static_cast<std::int64_t>(value);
    return v >= origin && v <= end;
}

} // namespace

bool point_in_hitbox(Point point, const CollisionEntityView& entity) noexcept {
    return in_inclusive_extent(point.x, entity.x, entity.hitbox_width) &&
           in_inclusive_extent(point.y, entity.y, entity.hitbox_height);
}

bool point_hits_opaque_pixel(
    Point point,
    const CollisionEntityView& entity,
    std::span<const std::uint8_t> frame) noexcept {
    if (entity.sprite_width <= 0 || entity.sprite_height <= 0) return false;
    if (point.x < entity.x || point.y < entity.y) return false;

    const auto relative_x = static_cast<std::int64_t>(point.x) - entity.x;
    const auto relative_y = static_cast<std::int64_t>(point.y) - entity.y;
    const auto width = static_cast<std::int64_t>(entity.sprite_width);
    const auto height = static_cast<std::int64_t>(entity.sprite_height);

    // This is intentionally stricter than the original's inclusive pre-check.
    // See the public header and docs/reverse/COLLISION.md for the binary detail.
    if (relative_x < 0 || relative_y < 0 || relative_x >= width || relative_y >= height) return false;

    const auto index64 = relative_y * width + relative_x;
    if (index64 < 0 || static_cast<std::uint64_t>(index64) >= frame.size()) return false;
    return frame[static_cast<std::size_t>(index64)] != 0;
}

bool point_plus_y9_in_hitbox(Point point, const CollisionEntityView& entity) noexcept {
    const auto shifted_y = static_cast<std::int64_t>(point.y) + 9;
    if (shifted_y > std::numeric_limits<std::int32_t>::max() ||
        shifted_y < std::numeric_limits<std::int32_t>::min()) {
        return false;
    }
    point.y = static_cast<std::int32_t>(shifted_y);
    return point_in_hitbox(point, entity);
}

bool entity_hitbox_overlaps_sprite_rect(
    const CollisionEntityView& hitbox_entity,
    const CollisionEntityView& sprite_entity) noexcept {
    const auto centered_x = static_cast<std::int64_t>(hitbox_entity.x) +
        ((static_cast<std::int64_t>(hitbox_entity.sprite_width) -
          hitbox_entity.hitbox_width) >> 1);
    const auto centered_y = static_cast<std::int64_t>(hitbox_entity.y) +
        ((static_cast<std::int64_t>(hitbox_entity.sprite_height) -
          hitbox_entity.hitbox_height) >> 1);
    const auto hitbox_right = centered_x + hitbox_entity.hitbox_width;
    const auto hitbox_bottom = centered_y + hitbox_entity.hitbox_height;

    const auto sprite_left = static_cast<std::int64_t>(sprite_entity.x);
    const auto sprite_top = static_cast<std::int64_t>(sprite_entity.y);
    const auto sprite_right = sprite_left + sprite_entity.sprite_width;
    const auto sprite_bottom = sprite_top + sprite_entity.sprite_height;

    return centered_x <= sprite_right && hitbox_right >= sprite_left &&
           centered_y <= sprite_bottom && hitbox_bottom >= sprite_top;
}

} // namespace drone::gameplay
