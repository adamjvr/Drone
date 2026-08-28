#include <drone/gameplay/drone_weapon_interaction.hpp>

#include <cassert>

using namespace drone::gameplay;

int main() {
    // 0x00421ED0 is the exact classic MSVC CRT LCG used by the executable.
    {
        OriginalRandomState random{};
        seed_original_random(random, 1);
        assert(next_original_random(random) == 41);
        assert(next_original_random(random) == 18467);
        assert(next_original_random(random) == 6334);
        assert(random.draws == 3);
    }

    // A blue Probe uses the point-vs-Drone hitbox producer, attaches, awards
    // +10, and consumes exactly two live-mode RNG draws for decode thresholds.
    {
        DroneObjectiveState drone{};
        drone.x = 100;
        drone.y = 50;

        SpecialWeaponState special{};
        special.kind = SpecialWeaponKind::Probe;
        special.activity = SpecialWeaponActivity::LaunchedHoming;
        special.x = 112; // inclusive recovered hitbox right edge
        special.y = 82;  // inclusive recovered hitbox bottom edge

        OriginalRandomState random{};
        seed_original_random(random, 1);
        ScoreState score{};
        const auto hit = collide_special_weapon_with_drone(
            special,
            drone,
            DifficultyLevel::Beginner,
            false,
            random,
            score);
        assert(hit.hit && hit.probe_attached && !hit.stinger_hit);
        assert(hit.score_delta == probe_attachment_score_award);
        assert(score.total == 10 && score.extra_life_progress == 10);
        assert(special.activity == SpecialWeaponActivity::ProbeAttachedDecoding);
        assert(special.probe_decode.status == ProbeDecodeStatus::Phase1Decoding);
        assert(special.probe_decode.phase1_threshold == 491); // (41%70+450)*1
        assert(special.probe_decode.phase2_threshold == 357); // (18467%70+300)*1
        assert(random.draws == 2);

        special.probe_decode.phase1_elapsed = 490;
        auto decode = step_probe_decode(special, random, score);
        assert(decode.phase1_completed);
        assert(!decode.disarm_completed);
        assert(special.probe_decode.status == ProbeDecodeStatus::Phase2Disarming);
        // Win32 falls through into phase 2 in the same update.
        assert(special.probe_decode.phase2_elapsed == 1);
        assert(random.draws == 2);

        special.probe_decode.phase2_elapsed = 356;
        decode = step_probe_decode(special, random, score);
        assert(!decode.phase1_completed && decode.disarm_completed);
        assert(decode.score_delta == drone_disarm_score_award);
        assert(decode.completion_effect_random == 74); // 6334%60 + 40
        assert(special.probe_decode.status == ProbeDecodeStatus::Complete);
        assert(score.total == 510 && score.extra_life_progress == 510);
        assert(random.draws == 3);

        const auto old_phase1_threshold = special.probe_decode.phase1_threshold;
        const auto old_phase2_threshold = special.probe_decode.phase2_threshold;
        assert(clear_completed_probe_decode(special));
        assert(special.probe_decode.status == ProbeDecodeStatus::Phase1Decoding);
        assert(special.probe_decode.phase1_elapsed == 0);
        assert(special.probe_decode.phase2_elapsed == 0);
        assert(special.probe_decode.phase1_threshold == old_phase1_threshold);
        assert(special.probe_decode.phase2_threshold == old_phase2_threshold);
    }

    // Demo playback substitutes fixed deterministic thresholds and therefore
    // consumes no attachment RNG draws.
    {
        ProbeDecodeState decode{};
        OriginalRandomState random{};
        seed_original_random(random, 0x12345678u);
        initialize_probe_decode_timing(
            decode,
            DifficultyLevel::Advanced,
            true,
            random);
        assert(decode.phase1_threshold == 210);
        assert(decode.phase2_threshold == 150);
        assert(random.draws == 0);
    }

    // One pixel beyond the inclusive 12x32 Drone hitbox does not attach.
    {
        DroneObjectiveState drone{};
        drone.x = 100;
        drone.y = 50;
        SpecialWeaponState special{};
        special.kind = SpecialWeaponKind::Probe;
        special.activity = SpecialWeaponActivity::LaunchedHoming;
        special.x = 113;
        special.y = 82;
        OriginalRandomState random{};
        ScoreState score{};
        const auto hit = collide_special_weapon_with_drone(
            special,
            drone,
            DifficultyLevel::Beginner,
            false,
            random,
            score);
        assert(!hit.hit);
        assert(special.activity == SpecialWeaponActivity::LaunchedHoming);
        assert(score.total == 0);
        assert(random.draws == 0);
    }

    // A red Stinger hitting the Drone is consumed directly to state 0 and
    // starts the same pre-detonation countdown used by the timeout path.
    {
        DroneObjectiveState drone{};
        drone.x = 100;
        drone.y = 50;
        SpecialWeaponState special{};
        special.kind = SpecialWeaponKind::Stinger;
        special.activity = SpecialWeaponActivity::LaunchedHoming;
        special.x = 105;
        special.y = 60;
        OriginalRandomState random{};
        ScoreState score{};
        const auto hit = collide_special_weapon_with_drone(
            special,
            drone,
            DifficultyLevel::Intermediate,
            false,
            random,
            score);
        assert(hit.hit && hit.stinger_hit && !hit.probe_attached);
        assert(hit.destruction_countdown_started);
        assert(hit.explosion_spawns_requested == 8);
        assert(special.activity == SpecialWeaponActivity::Inactive);
        assert(drone.destruction_countdown == 0);
        assert(score.total == 0 && random.draws == 0);
    }

    // Rapid missiles are tested in ascending pool order. The first colliding
    // active slot is retired and starts the same shared countdown.
    {
        DroneObjectiveState drone{};
        drone.x = 100;
        drone.y = 50;
        RapidMissilePool missiles{};
        missiles.missiles[1].active = true;
        missiles.missiles[1].x = 99; // outside
        missiles.missiles[1].y = 60;
        missiles.missiles[3].active = true;
        missiles.missiles[3].x = 102;
        missiles.missiles[3].y = 70;
        missiles.active_count = 2;

        const auto hit = collide_rapid_missiles_with_drone(missiles, drone);
        assert(hit.hit && hit.missile_index == 3);
        assert(hit.destruction_countdown_started);
        assert(hit.explosion_spawns_requested == 8);
        assert(missiles.missiles[1].active);
        assert(!missiles.missiles[3].active);
        assert(missiles.active_count == 1);
        assert(drone.destruction_countdown == 0);

        // Once the countdown is active, later Drone-hit producers are gated.
        const auto second = collide_rapid_missiles_with_drone(missiles, drone);
        assert(!second.hit);
    }

    return 0;
}
