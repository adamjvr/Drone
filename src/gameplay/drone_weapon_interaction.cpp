#include <drone/gameplay/drone_weapon_interaction.hpp>

#include <drone/gameplay/collision.hpp>

namespace drone::gameplay {
namespace {

[[nodiscard]] CollisionEntityView drone_collision_view(
    const DroneObjectiveState& drone) noexcept {
    return CollisionEntityView{
        .x = drone.x,
        .y = drone.y,
        .sprite_width = static_cast<std::int16_t>(canonical_drone_sprite_width),
        .sprite_height = static_cast<std::int16_t>(canonical_drone_sprite_height),
        .hitbox_width = canonical_drone_collision_width_extent,
        .hitbox_height = canonical_drone_collision_height_extent,
    };
}

[[nodiscard]] bool drone_can_receive_weapon_hit(const DroneObjectiveState& drone) noexcept {
    return drone.activity == canonical_drone_active_activity &&
           drone.destruction_countdown > canonical_drone_destruction_countdown_trigger;
}

} // namespace

RapidMissileDroneCollisionResult collide_rapid_missiles_with_drone(
    RapidMissilePool& missiles,
    DroneObjectiveState& drone) noexcept {
    RapidMissileDroneCollisionResult result{};
    if (!drone_can_receive_weapon_hit(drone)) {
        return result;
    }

    const auto drone_view = drone_collision_view(drone);
    for (std::size_t i = 0; i < missiles.missiles.size(); ++i) {
        const auto& missile = missiles.missiles[i];
        if (!missile.active) {
            continue;
        }
        if (!point_in_hitbox(Point{missile.x, missile.y}, drone_view)) {
            continue;
        }

        if (!deactivate_rapid_missile(missiles, i)) {
            return result;
        }
        result.hit = true;
        result.missile_index = i;
        result.destruction_countdown_started = start_drone_destruction_countdown(drone);
        result.explosion_spawns_requested = canonical_drone_weapon_hit_explosion_requests;
        return result;
    }
    return result;
}

SpecialWeaponDroneCollisionResult collide_special_weapon_with_drone(
    SpecialWeaponState& special,
    DroneObjectiveState& drone,
    const DifficultyLevel difficulty,
    const bool demo_playback_mode,
    OriginalRandomState& random,
    ScoreState& score) noexcept {
    SpecialWeaponDroneCollisionResult result{};
    if (special.activity != SpecialWeaponActivity::LaunchedHoming ||
        !drone_can_receive_weapon_hit(drone)) {
        return result;
    }

    const auto drone_view = drone_collision_view(drone);
    if (!point_in_hitbox(Point{special.x, special.y}, drone_view)) {
        return result;
    }
    result.hit = true;

    if (special.kind == SpecialWeaponKind::Stinger) {
        // The Drone-special case writes activity directly to zero rather than
        // using the generic state-10 terminal path.
        special.activity = SpecialWeaponActivity::Inactive;
        result.stinger_hit = true;
        result.destruction_countdown_started = start_drone_destruction_countdown(drone);
        result.explosion_spawns_requested = canonical_drone_weapon_hit_explosion_requests;
        return result;
    }

    if (!attach_probe_to_drone(special)) {
        return result;
    }
    initialize_probe_decode_timing(
        special.probe_decode,
        difficulty,
        demo_playback_mode,
        random);
    apply_score_delta(score, probe_attachment_score_award);
    result.probe_attached = true;
    result.score_delta = probe_attachment_score_award;
    return result;
}

} // namespace drone::gameplay
