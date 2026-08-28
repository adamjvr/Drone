#include <drone/gameplay/enemy_bomb.hpp>

#include <cassert>

using namespace drone::gameplay;

int main() {
    // 0x00402000 is inclusive and adds nine to the bomb Y before testing the
    // special entity's 2x6 collision extents.
    {
        EnemyBombPool bombs{};
        SpecialWeaponState special{};
        special.activity = SpecialWeaponActivity::LaunchedHoming;
        special.kind = SpecialWeaponKind::Probe;
        special.x = 100;
        special.y = 50;

        assert(spawn_live_enemy_bomb(bombs, 102, 47, 0)); // y+9 = 56 bottom edge
        const auto hit = collide_enemy_bombs_with_special_weapon(bombs, special);
        assert(hit.hit);
        assert(hit.bomb_index == 0);
        assert(hit.previous_activity == SpecialWeaponActivity::LaunchedHoming);
        assert(hit.launch_sound_stop_requested);
        assert(hit.probe_impact_effect_requested);
        assert(hit.probe_impact_sound_requested);
        assert(!hit.probe_decode_reset);
        assert(!bombs.bombs[0].active);
        assert(bombs.active_count == 0);
        assert(special.activity == SpecialWeaponActivity::Inactive);
        assert(special.motion_y == 1);
    }

    // Phase-2 attached-Probe knockoff resets status and elapsed counters.
    {
        EnemyBombPool bombs{};
        SpecialWeaponState special{};
        special.activity = SpecialWeaponActivity::ProbeAttachedDecoding;
        special.kind = SpecialWeaponKind::Probe;
        special.x = 120;
        special.y = 70;
        special.probe_decode.status = ProbeDecodeStatus::Phase2Disarming;
        special.probe_decode.phase1_elapsed = 500;
        special.probe_decode.phase2_elapsed = 17;
        special.probe_decode.phase1_threshold = 501;
        special.probe_decode.phase2_threshold = 350;

        assert(spawn_live_enemy_bomb(bombs, 120, 61, 0)); // y+9 = 70 top edge
        const auto hit = collide_enemy_bombs_with_special_weapon(bombs, special);
        assert(hit.hit);
        assert(hit.probe_decode_reset);
        assert(hit.probe_phase2_interrupt_signal_requested);
        assert(special.probe_decode.status == ProbeDecodeStatus::Phase1Decoding);
        assert(special.probe_decode.phase1_elapsed == 0);
        assert(special.probe_decode.phase2_elapsed == 0);
        assert(special.probe_decode.phase1_threshold == 501);
        assert(special.probe_decode.phase2_threshold == 350);
        assert(special.activity == SpecialWeaponActivity::Inactive);
    }

    // The original asymmetry matters: a bomb can knock off a phase-1 Probe,
    // but phase1-only elapsed state is not explicitly cleared by this branch.
    {
        EnemyBombPool bombs{};
        SpecialWeaponState special{};
        special.activity = SpecialWeaponActivity::ProbeAttachedDecoding;
        special.kind = SpecialWeaponKind::Probe;
        special.x = 140;
        special.y = 80;
        special.probe_decode.status = ProbeDecodeStatus::Phase1Decoding;
        special.probe_decode.phase1_elapsed = 77;
        special.probe_decode.phase2_elapsed = 0;

        assert(spawn_live_enemy_bomb(bombs, 141, 71, 0));
        const auto hit = collide_enemy_bombs_with_special_weapon(bombs, special);
        assert(hit.hit);
        assert(!hit.probe_decode_reset);
        assert(special.probe_decode.status == ProbeDecodeStatus::Phase1Decoding);
        assert(special.probe_decode.phase1_elapsed == 77);
        assert(special.probe_decode.phase2_elapsed == 0);
        assert(special.activity == SpecialWeaponActivity::Inactive);
    }

    // Completed status is never reset by the bomb branch, even though the
    // special entity itself is consumed.
    {
        EnemyBombPool bombs{};
        SpecialWeaponState special{};
        special.activity = SpecialWeaponActivity::ProbeAttachedDecoding;
        special.kind = SpecialWeaponKind::Probe;
        special.x = 150;
        special.y = 90;
        special.probe_decode.status = ProbeDecodeStatus::Complete;
        special.probe_decode.phase1_elapsed = 123;
        special.probe_decode.phase2_elapsed = 456;

        assert(spawn_live_enemy_bomb(bombs, 150, 81, 0));
        const auto hit = collide_enemy_bombs_with_special_weapon(bombs, special);
        assert(hit.hit);
        assert(!hit.probe_decode_reset);
        assert(special.probe_decode.status == ProbeDecodeStatus::Complete);
        assert(special.probe_decode.phase1_elapsed == 123);
        assert(special.probe_decode.phase2_elapsed == 456);
    }

    // Stinger impacts zero both motion fields and request the established
    // Stinger-specific effect/audio branch.
    {
        EnemyBombPool bombs{};
        SpecialWeaponState special{};
        special.activity = SpecialWeaponActivity::LaunchedHoming;
        special.kind = SpecialWeaponKind::Stinger;
        special.x = 200;
        special.y = 100;
        special.motion_x = 5;
        special.motion_y = -2;

        assert(spawn_live_enemy_bomb(bombs, 200, 91, 0));
        const auto hit = collide_enemy_bombs_with_special_weapon(bombs, special);
        assert(hit.hit);
        assert(hit.stinger_impact_effect_requested);
        assert(hit.stinger_impact_sound_requested);
        assert(!hit.probe_impact_effect_requested);
        assert(special.motion_x == 0);
        assert(special.motion_y == 0);
    }

    // Ascending slot order is preserved; later bombs cannot hit after the
    // special has already been consumed by the first colliding slot.
    {
        EnemyBombPool bombs{};
        SpecialWeaponState special{};
        special.activity = SpecialWeaponActivity::LaunchedHoming;
        special.x = 220;
        special.y = 110;
        assert(spawn_live_enemy_bomb(bombs, 220, 101, 0));
        assert(spawn_live_enemy_bomb(bombs, 220, 101, 0));

        const auto hit = collide_enemy_bombs_with_special_weapon(bombs, special);
        assert(hit.hit && hit.bomb_index == 0);
        assert(!bombs.bombs[0].active);
        assert(bombs.bombs[1].active);
        assert(bombs.active_count == 1);
    }

    return 0;
}
