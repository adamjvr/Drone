#include <drone/gameplay/enemy_bomb.hpp>

#include <cassert>

using namespace drone::gameplay;

int main() {
    // Player ship.jba is 22x22; common initialization yields 18x18 collision
    // extents. 0x00402000 adds nine to bomb Y and keeps inclusive edges.
    {
        EnemyBombPool bombs{};
        SpecialWeaponState special{};
        PlayerMotionState player{};
        PlayerLifecycleState lifecycle{};
        EnemyBombSpawnGate gate{};
        assert(spawn_live_enemy_bomb(bombs, player.x + 18, player.y + 9, 0));
        const auto r = process_enemy_bomb_late_collision_pass(
            bombs, special, player, lifecycle, true, gate);
        assert(r.player_hits == 1 && r.shield_absorptions == 1);
        assert(r.first_player_hit_index == 0);
        assert(lifecycle.player_active);
        assert(bombs.active_count == 0);
    }

    // One pixel beyond the inclusive right edge does not collide.
    {
        EnemyBombPool bombs{};
        SpecialWeaponState special{};
        PlayerMotionState player{};
        PlayerLifecycleState lifecycle{};
        EnemyBombSpawnGate gate{};
        assert(spawn_live_enemy_bomb(bombs, player.x + 19, player.y - 9, 0));
        const auto r = process_enemy_bomb_late_collision_pass(
            bombs, special, player, lifecycle, false, gate);
        assert(r.player_hits == 0);
        assert(lifecycle.player_active);
        assert(bombs.active_count == 1);
    }

    // Unshielded impact launches a merely loaded special before destroying the
    // player and suppresses bomb spawning through the shared -540 gate.
    {
        EnemyBombPool bombs{};
        SpecialWeaponState special{};
        special.activity = SpecialWeaponActivity::LoadedTracking;
        special.kind = SpecialWeaponKind::Probe;
        PlayerMotionState player{};
        PlayerLifecycleState lifecycle{};
        EnemyBombSpawnGate gate{};
        assert(spawn_live_enemy_bomb(bombs, player.x, player.y - 9, 2));
        const auto r = process_enemy_bomb_late_collision_pass(
            bombs, special, player, lifecycle, false, gate);
        assert(r.player_hits == 1);
        assert(r.loaded_special_auto_launched);
        assert(r.auto_launch_sound_requested);
        assert(r.player_hit_sfx_requested);
        assert(r.player_destruction_started && r.player_death_effect_requested);
        assert(r.bomb_spawn_suppression_started);
        assert(!lifecycle.player_active);
        assert(special.activity == SpecialWeaponActivity::LaunchedHoming);
        assert(gate.counter == -540);
        assert(bombs.active_count == 0);
    }

    // Shielded impacts may absorb multiple colliding bombs because the player
    // remains active. Each bomb is consumed exactly once.
    {
        EnemyBombPool bombs{};
        SpecialWeaponState special{};
        PlayerMotionState player{};
        PlayerLifecycleState lifecycle{};
        EnemyBombSpawnGate gate{};
        assert(spawn_live_enemy_bomb(bombs, player.x, player.y - 9, 2));
        assert(spawn_live_enemy_bomb(bombs, player.x + 1, player.y - 9, -2));
        const auto r = process_enemy_bomb_late_collision_pass(
            bombs, special, player, lifecycle, true, gate);
        assert(r.player_hits == 2 && r.shield_absorptions == 2);
        assert(lifecycle.player_active);
        assert(bombs.active_count == 0);
        assert(bombs.bombs[0].horizontal_step == 0);
        assert(bombs.bombs[1].horizontal_step == 0);
    }

    // Critical fall-through quirk: the same bomb may first destroy a special
    // entity and then hit the player in the same slot iteration. active_count
    // still drops only once.
    {
        EnemyBombPool bombs{};
        SpecialWeaponState special{};
        PlayerMotionState player{};
        PlayerLifecycleState lifecycle{};
        EnemyBombSpawnGate gate{};
        special.activity = SpecialWeaponActivity::ProbeAttachedDecoding;
        special.kind = SpecialWeaponKind::Probe;
        special.x = player.x;
        special.y = player.y;
        assert(spawn_live_enemy_bomb(bombs, player.x, player.y - 9, 0));
        const auto r = process_enemy_bomb_late_collision_pass(
            bombs, special, player, lifecycle, false, gate);
        assert(r.special_impact.hit && r.special_impact.bomb_index == 0);
        assert(r.player_hits == 1 && r.player_destruction_started);
        assert(special.activity == SpecialWeaponActivity::Inactive);
        assert(!lifecycle.player_active);
        assert(bombs.active_count == 0);
    }

    // After an unshielded lethal hit, later bombs no longer test the player,
    // but the per-slot special-first behavior remains available if a special is
    // still active.
    {
        EnemyBombPool bombs{};
        SpecialWeaponState special{};
        PlayerMotionState player{};
        PlayerLifecycleState lifecycle{};
        EnemyBombSpawnGate gate{};
        assert(spawn_live_enemy_bomb(bombs, player.x, player.y - 9, 0));
        assert(spawn_live_enemy_bomb(bombs, player.x, player.y - 9, 0));
        const auto r = process_enemy_bomb_late_collision_pass(
            bombs, special, player, lifecycle, false, gate);
        assert(r.player_hits == 1);
        assert(!lifecycle.player_active);
        assert(!bombs.bombs[0].active);
        assert(bombs.bombs[1].active);
        assert(bombs.active_count == 1);
    }

    return 0;
}
