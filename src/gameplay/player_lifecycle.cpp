#include <drone/gameplay/player_lifecycle.hpp>

namespace drone::gameplay {

PlayerRespawnResolution settle_player_death(
    PlayerLifecycleState& lifecycle,
    PlayerMotionState& player,
    PlayerShieldState& shield,
    const PlayerRespawnGate& gate) {

    PlayerRespawnResolution result{};
    const bool can_settle =
        gate.bomb_spawn_gate_allows_settlement &&
        gate.death_effect_inactive &&
        gate.player_inactive &&
        gate.drone_allows_respawn &&
        lifecycle.lives > 0;

    if (!can_settle) {
        return result;
    }

    --lifecycle.lives;
    result.consumed_life = true;

    // Original order matters: shield/frame/position are reset before the code
    // tests whether the decremented life count is still positive.
    shield.energy = shield_nominal_max_energy;
    result.shield_reset = true;
    player.frame = 0;
    player.x = canonical_respawn_x;
    player.y = canonical_respawn_y;

    if (lifecycle.lives > 0) {
        lifecycle.player_active = true;
        result.respawned = true;
    } else {
        lifecycle.player_active = false;
        result.game_over = true;
    }

    return result;
}

} // namespace drone::gameplay
