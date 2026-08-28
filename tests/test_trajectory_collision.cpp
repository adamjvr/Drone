#include <drone/gameplay/game_session.hpp>
#include <drone/gameplay/trajectory_collision.hpp>

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>

namespace {

using namespace drone::gameplay;

void arm_group(
    TrajectoryEncounterState& encounter,
    std::size_t group_index,
    std::size_t actor_count,
    std::int32_t x,
    std::int32_t y,
    std::uint8_t threshold = 3) {
    auto& group = encounter.groups[group_index];
    group.lifecycle.mode = TrajectoryGroupMode::RetireOnPathWrap;
    group.lifecycle.entity_count = static_cast<std::int8_t>(actor_count);
    group.lifecycle.active_entity_count = static_cast<std::int8_t>(actor_count);
    group.lifecycle.activated_entity_count = static_cast<std::int8_t>(actor_count);
    for (std::size_t i = 0; i < actor_count; ++i) {
        auto& actor = group.actors[i];
        actor.x = x;
        actor.y = y;
        actor.sprite_width = 10;
        actor.sprite_height = 10;
        actor.current_frame = 0;
        actor.frame_count = 1;
        actor.activity = TrajectoryEntityActivity::FollowingPath;
        actor.damage_accumulator = 0;
        actor.destruction_threshold = threshold;
        actor.destruction_burst_count = 2;
        actor.score_value = 4;
    }
    encounter.active_group_count = 1;
}

} // namespace

int main() {
    using namespace drone::gameplay;

    // Rapid missiles use the actor's current opaque sprite frame. Transparent
    // pixels miss; a nonzero palette index consumes the missile and adds +3.
    {
        TrajectoryEncounterState encounter{};
        arm_group(encounter, 1, 1, 10, 20, 3);
        RapidMissilePool missiles{};
        missiles.missiles[0] = {.x = 12, .y = 23, .active = true};
        missiles.active_count = 1;
        std::array<std::uint8_t, 100> frame{};
        TrajectorySpriteMaskCatalogView masks{};
        masks.frames[1][0] = frame;
        ScoreState score{};

        auto result = collide_rapid_missiles_with_trajectories(encounter, missiles, masks, score);
        assert(result.collisions == 0);
        assert(missiles.missiles[0].active);

        frame[3 * 10 + 2] = 7;
        masks.frames[1][0] = frame;
        result = collide_rapid_missiles_with_trajectories(encounter, missiles, masks, score);
        assert(result.collisions == 1);
        assert(result.rapid_missiles_consumed == 1);
        assert(result.actors_destroyed == 1);
        assert(result.groups_retired == 1);
        assert(result.destruction_bursts == 2);
        assert(result.score_delta == 4);
        assert(!missiles.missiles[0].active);
    }

    // Actor-major ordering means one missile cannot hit a later overlapping
    // actor after the first actor consumed it.
    {
        TrajectoryEncounterState encounter{};
        arm_group(encounter, 1, 2, 30, 40, 3);
        RapidMissilePool missiles{};
        missiles.missiles[0] = {.x = 31, .y = 41, .active = true};
        missiles.active_count = 1;
        std::array<std::uint8_t, 100> frame{};
        frame[11] = 1;
        TrajectorySpriteMaskCatalogView masks{};
        masks.frames[1][0] = frame;
        ScoreState score{};
        const auto result = collide_rapid_missiles_with_trajectories(encounter, missiles, masks, score);
        assert(result.collisions == 1);
        assert(result.actors_destroyed == 1);
        assert(encounter.groups[1].actors[1].activity != TrajectoryEntityActivity::Inactive);
    }

    // Stinger display frames 0..2 are harmless; frames 3..5 use its centered
    // 51x45 hitbox against the actor's full sprite rectangle and add +15.
    {
        TrajectoryEncounterState encounter{};
        arm_group(encounter, 1, 1, 100, 100, 15);
        StingerDisplayState display{};
        display.x = 70;
        display.y = 74;
        display.active = true;
        display.current_frame = 2;
        ScoreState score{};
        auto result = collide_stinger_display_with_trajectories(encounter, display, score);
        assert(result.collisions == 0);
        display.current_frame = 3;
        result = collide_stinger_display_with_trajectories(encounter, display, score);
        assert(result.collisions == 1);
        assert(result.actors_destroyed == 1);

        display.current_frame = 5;
        advance_stinger_display(display);
        assert(!display.active);
        assert(display.current_frame == 0);
    }

    // Direct Probe collision destroys every overlapping actor using the point
    // hitbox even though the first hit changes the Probe to inactive. Direct
    // destruction intentionally preserves the actor's retained damage byte.
    {
        TrajectoryEncounterState encounter{};
        arm_group(encounter, 1, 2, 50, 60, 99);
        encounter.groups[1].actors[0].damage_accumulator = 7;
        encounter.groups[1].actors[1].damage_accumulator = 9;
        SpecialWeaponState special{};
        special.x = 52;
        special.y = 62;
        special.kind = SpecialWeaponKind::Probe;
        special.activity = SpecialWeaponActivity::LaunchedHoming;
        StingerDisplayState display{};
        ScoreState score{};
        const auto result = collide_launched_special_with_trajectories(
            encounter, special, true, false, display, score);
        assert(result.collisions == 2);
        assert(result.actors_destroyed == 2);
        assert(special.activity == SpecialWeaponActivity::Inactive);
        assert(encounter.groups[1].actors[0].damage_accumulator == 7);
        assert(encounter.groups[1].actors[1].damage_accumulator == 9);
    }

    // Live Stinger direct collision keeps the projectile launched and activates
    // the separate display centered on the projectile without resetting frame.
    {
        TrajectoryEncounterState encounter{};
        arm_group(encounter, 1, 1, 20, 30, 99);
        SpecialWeaponState special{};
        special.x = 22;
        special.y = 32;
        special.kind = SpecialWeaponKind::Stinger;
        special.activity = SpecialWeaponActivity::LaunchedHoming;
        StingerDisplayState display{};
        display.current_frame = 4;
        ScoreState score{};
        const auto result = collide_launched_special_with_trajectories(
            encounter, special, true, false, display, score);
        assert(result.stinger_display_activated);
        assert(special.activity == SpecialWeaponActivity::LaunchedHoming);
        assert(display.active);
        assert(display.current_frame == 4);
        assert(display.x == special.x - 30);
        assert(display.y == special.y - 26);
    }

    // Session-level rapid collision happens before missile movement and owns the
    // original local+mission hit increments on a destruction.
    {
        GameSession session{};
        arm_group(session.encounter.trajectories, 1, 1, 80, 90, 3);
        auto& missile = session.encounter.rapid_missiles.missiles[0];
        missile = {.x = 82, .y = 92, .active = true};
        session.encounter.rapid_missiles.active_count = 1;
        std::array<std::uint8_t, 100> frame{};
        frame[22] = 1;
        TrajectorySpriteMaskCatalogView masks{};
        masks.frames[1][0] = frame;
        GameSessionTargetContext targets{};
        targets.trajectory_sprite_masks = &masks;
        const auto tick = step_game_session(session, GameplayInputFrame{}, targets);
        assert(tick.trajectory_rapid_collisions == 1);
        assert(tick.trajectory_actors_destroyed == 1);
        assert(session.encounter.encounter_alien_ships_hit == 1);
        assert(session.campaign.alien_ships_hit == 1);
        assert(!missile.active);
    }

    // Capture the late-block state-3 gate before Drone collision: attaching a
    // Probe to the Drone does not cancel the trajectory scan later that update.
    {
        GameSession session{};
        arm_group(session.encounter.trajectories, 1, 1, 150, 45, 99);
        session.encounter.drone.x = 150;
        session.encounter.drone.y = 45;
        session.encounter.special_weapon.x = 152;
        session.encounter.special_weapon.y = 47;
        session.encounter.special_weapon.kind = SpecialWeaponKind::Probe;
        session.encounter.special_weapon.activity = SpecialWeaponActivity::LaunchedHoming;
        const auto tick = step_game_session(session, GameplayInputFrame{});
        assert(tick.special_weapon_hit_drone);
        assert(tick.probe_attached_to_drone);
        assert(tick.trajectory_direct_special_collisions == 1);
        assert(tick.trajectory_actors_destroyed == 1);
        // The direct trajectory collision later in the same captured block puts
        // the Probe inactive after the Drone attach transition.
        assert(session.encounter.special_weapon.activity == SpecialWeaponActivity::Inactive);
        assert(session.encounter.encounter_alien_ships_hit == 0);
        assert(session.campaign.alien_ships_hit == 0);
    }

    std::cout << "trajectory collision tests passed\n";
    return 0;
}
