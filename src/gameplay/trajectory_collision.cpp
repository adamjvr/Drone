#include <drone/gameplay/trajectory_collision.hpp>

#include <drone/gameplay/collision.hpp>

#include <algorithm>

namespace drone::gameplay {
namespace {

std::int16_t hitbox_extent(std::int16_t sprite_extent) noexcept {
    if (sprite_extent <= 0) return 0;
    return static_cast<std::int16_t>((static_cast<std::int32_t>(sprite_extent) * 85) / 100);
}

CollisionEntityView actor_view(const TrajectoryActorState& actor) noexcept {
    return CollisionEntityView{
        .x = actor.x,
        .y = actor.y,
        .sprite_width = actor.sprite_width,
        .sprite_height = actor.sprite_height,
        .hitbox_width = hitbox_extent(actor.sprite_width),
        .hitbox_height = hitbox_extent(actor.sprite_height),
    };
}

void accumulate_destroyed(
    TrajectoryWeaponCollisionResult& result,
    const TrajectoryHitResult& hit) noexcept {
    if (!hit.destroyed) return;
    ++result.actors_destroyed;
    if (hit.group_retired) ++result.groups_retired;
    result.destruction_bursts += hit.destruction_burst_count;
    result.score_delta += hit.score_delta;
}

} // namespace

std::span<const std::uint8_t> TrajectorySpriteMaskCatalogView::frame(
    const std::size_t group_index,
    const std::uint8_t frame_index) const noexcept {
    if (group_index >= frames.size() || frame_index >= canonical_trajectory_collision_frame_slots) {
        return {};
    }
    return frames[group_index][frame_index];
}

void activate_stinger_display_at(
    StingerDisplayState& display,
    const std::int32_t special_x,
    const std::int32_t special_y) noexcept {
    display.x = special_x - (canonical_stinger_display_width >> 1);
    display.y = special_y - (canonical_stinger_display_height >> 1);
    display.active = true;
    // The original activation path does not explicitly reset current_frame.
}

void advance_stinger_display(StingerDisplayState& display) noexcept {
    if (!display.active) return;
    ++display.current_frame;
    if (display.current_frame >= canonical_stinger_display_frame_count) {
        display.active = false;
        display.current_frame = 0;
    }
}

TrajectoryWeaponCollisionResult collide_rapid_missiles_with_trajectories(
    TrajectoryEncounterState& encounter,
    RapidMissilePool& missiles,
    const TrajectorySpriteMaskCatalogView& masks,
    ScoreState& score) noexcept {
    TrajectoryWeaponCollisionResult result{};

    for (std::size_t group_index = 0; group_index < encounter.groups.size(); ++group_index) {
        auto& group = encounter.groups[group_index];
        if (group.lifecycle.mode == TrajectoryGroupMode::Inactive) continue;
        const auto count = static_cast<std::size_t>(std::max<std::int8_t>(0, group.lifecycle.entity_count));
        for (std::size_t actor_index = 0; actor_index < count && actor_index < group.actors.size(); ++actor_index) {
            auto& actor = group.actors[actor_index];
            if (actor.activity == TrajectoryEntityActivity::Inactive) continue;

            for (std::size_t missile_index = 0; missile_index < missiles.missiles.size(); ++missile_index) {
                auto& missile = missiles.missiles[missile_index];
                if (!missile.active || actor.activity == TrajectoryEntityActivity::Inactive) continue;
                const auto frame = masks.frame(group_index, actor.current_frame);
                if (frame.empty()) continue;
                if (!point_hits_opaque_pixel(Point{missile.x, missile.y}, actor_view(actor), frame)) continue;

                ++result.collisions;
                if (deactivate_rapid_missile(missiles, missile_index)) ++result.rapid_missiles_consumed;
                const auto hit = apply_trajectory_hit(
                    encounter,
                    TrajectoryHitEvent{
                        static_cast<std::uint8_t>(group_index),
                        static_cast<std::uint8_t>(actor_index),
                        3,
                        TrajectoryHitSource::RapidMissile},
                    score);
                accumulate_destroyed(result, hit);
            }
        }
    }
    return result;
}

TrajectoryWeaponCollisionResult collide_stinger_display_with_trajectories(
    TrajectoryEncounterState& encounter,
    const StingerDisplayState& display,
    ScoreState& score) noexcept {
    TrajectoryWeaponCollisionResult result{};
    if (!display.active || display.current_frame <= 2) return result;

    const CollisionEntityView display_view{
        .x = display.x,
        .y = display.y,
        .sprite_width = canonical_stinger_display_width,
        .sprite_height = canonical_stinger_display_height,
        .hitbox_width = canonical_stinger_display_hitbox_width,
        .hitbox_height = canonical_stinger_display_hitbox_height,
    };

    for (std::size_t group_index = 0; group_index < encounter.groups.size(); ++group_index) {
        auto& group = encounter.groups[group_index];
        if (group.lifecycle.mode == TrajectoryGroupMode::Inactive) continue;
        const auto count = static_cast<std::size_t>(std::max<std::int8_t>(0, group.lifecycle.entity_count));
        for (std::size_t actor_index = 0; actor_index < count && actor_index < group.actors.size(); ++actor_index) {
            auto& actor = group.actors[actor_index];
            if (actor.activity == TrajectoryEntityActivity::Inactive) continue;
            if (!entity_hitbox_overlaps_sprite_rect(display_view, actor_view(actor))) continue;

            ++result.collisions;
            const auto hit = apply_trajectory_hit(
                encounter,
                TrajectoryHitEvent{
                    static_cast<std::uint8_t>(group_index),
                    static_cast<std::uint8_t>(actor_index),
                    15,
                    TrajectoryHitSource::SpecialWeapon},
                score);
            accumulate_destroyed(result, hit);
        }
    }
    return result;
}

TrajectoryWeaponCollisionResult collide_launched_special_with_trajectories(
    TrajectoryEncounterState& encounter,
    SpecialWeaponState& special,
    const bool entered_late_block_as_launched,
    const bool demo_playback_mode,
    StingerDisplayState& stinger_display,
    ScoreState& score) noexcept {
    TrajectoryWeaponCollisionResult result{};
    if (!entered_late_block_as_launched) return result;

    const Point point{special.x, special.y};
    const auto kind = special.kind;
    for (std::size_t group_index = 0; group_index < encounter.groups.size(); ++group_index) {
        auto& group = encounter.groups[group_index];
        if (group.lifecycle.mode == TrajectoryGroupMode::Inactive) continue;
        const auto count = static_cast<std::size_t>(std::max<std::int8_t>(0, group.lifecycle.entity_count));
        for (std::size_t actor_index = 0; actor_index < count && actor_index < group.actors.size(); ++actor_index) {
            auto& actor = group.actors[actor_index];
            if (actor.activity == TrajectoryEntityActivity::Inactive) continue;
            if (!point_in_hitbox(point, actor_view(actor))) continue;

            ++result.collisions;
            result.special_collision = true;
            const auto hit = destroy_trajectory_actor_direct(
                encounter,
                static_cast<std::uint8_t>(group_index),
                static_cast<std::uint8_t>(actor_index),
                score);
            accumulate_destroyed(result, hit);

            if (demo_playback_mode || kind == SpecialWeaponKind::Probe) {
                special.activity = SpecialWeaponActivity::Inactive;
            } else {
                activate_stinger_display_at(stinger_display, special.x, special.y);
                result.stinger_display_activated = true;
            }
            // Deliberately continue scanning even when Probe collision just made
            // the special inactive: the original entered this block from a
            // captured state-3 gate and does not re-test activity per actor.
        }
    }
    return result;
}

} // namespace drone::gameplay
