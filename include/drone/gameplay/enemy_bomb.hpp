#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace drone::gameplay {

// Reconstructed Win32 enemy-bomb pool rooted at 0x004651A0.
// The original entries are 0x154-byte common entities initialized as 1x9
// sprites with three shared frames extracted from bomb.jba.
struct EnemyBombState {
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int32_t horizontal_step = 0; // original common-entity +0x10
    std::uint8_t frame = 0;           // +0x140, frames 0..2
    bool active = false;              // semantic view of +0x142 == 1
    bool out_of_bounds = false;       // semantic view of +0x143
};

struct EnemyBombPool {
    static constexpr std::size_t capacity = 10; // Win32 0x0042B1A4

    std::array<EnemyBombState, capacity> bombs{};
    std::int32_t active_count = 0; // Win32 0x00446F6C
};


// Shared bomb-spawn gate/cooldown at Win32 0x00438C14. State-2 increments
// the scalar toward 5; live bomb-spawn paths require exactly 5 and reset it
// to zero. Player destruction deliberately drives the same counter negative,
// reusing bomb suppression as part of the respawn quiet-period gate.
struct EnemyBombSpawnGate {
    std::int32_t counter = -450; // canonical session initialization
};

inline constexpr std::int32_t enemy_bomb_spawn_gate_ready = 5;
inline constexpr std::int32_t player_respawn_bomb_gate_threshold = -356;
inline constexpr std::int32_t canonical_death_effect_terminal_frame = 27;

void advance_enemy_bomb_spawn_gate(EnemyBombSpawnGate& gate);
bool enemy_bomb_spawn_gate_allows_spawn(const EnemyBombSpawnGate& gate);
void reset_enemy_bomb_spawn_gate_after_spawn(EnemyBombSpawnGate& gate);
void suppress_enemy_bomb_spawns_for_player_destruction(
    EnemyBombSpawnGate& gate,
    std::int32_t death_effect_terminal_frame = canonical_death_effect_terminal_frame);
bool enemy_bomb_spawn_gate_allows_respawn(const EnemyBombSpawnGate& gate);

struct EnemyBombSteeringContext {
    std::int32_t player_x = 0;

    // The original update redirects bombs toward the attached Probe only when
    // an additional gameplay condition is true and special state == 2. The
    // still-unnamed condition is deliberately exposed to callers rather than
    // guessed here.
    bool redirect_to_attached_probe = false;
    std::int32_t attached_probe_x = 0;
};

// Allocate the first inactive entry. This mirrors the live path after the
// caller has already chosen the spawn location and rand()%3 steering value.
// The original spawn path does not reset the current animation frame.
bool spawn_live_enemy_bomb(
    EnemyBombPool& pool,
    std::int32_t x,
    std::int32_t y,
    std::int32_t horizontal_step);

// Demo playback restores only recorded X/Y and explicitly writes +0x10 = 0.
// Keeping this separate prevents an apparently harmless live-spawn default
// from destroying an original replay quirk.
bool spawn_replay_enemy_bomb(
    EnemyBombPool& pool,
    std::int32_t x,
    std::int32_t y);

// Reconstructs the state-2 active-bomb update. On the shared animation tick
// the three frames advance and wrap. X steers by horizontal_step toward either
// player.x+17 or (when caller enables the recovered redirect condition)
// attached_probe.x+1. Y always advances by two pixels.
void step_enemy_bombs(
    EnemyBombPool& pool,
    bool animation_tick,
    const EnemyBombSteeringContext& context);

// The original update sets +0x143 from logical bounds X=0..319, Y=0..190.
// A later collision/cleanup pass separately retires bombs only after y > 198.
std::size_t retire_enemy_bombs_below_bottom(EnemyBombPool& pool);

bool deactivate_enemy_bomb(EnemyBombPool& pool, std::size_t index);


struct EnemyBombPlayerImpactResult {
    bool bomb_deactivated = false;
    bool shield_absorbed = false;
    bool destroy_player = false;
    bool launch_loaded_special = false;
    bool play_player_hit_sfx = false;
    bool spawn_absorption_effect = false;
};

// Semantic reconstruction of the established player-hit branch at
// Win32 0x0040F4BB..0x0040F589. Collision detection itself remains a caller
// responsibility. The original always deactivates the bomb first. With an
// active shield the player survives and the bomb becomes a stationary mini
// explosion source; without shield the player is destroyed, bigexp3.wav is
// requested, and a merely loaded Probe/Stinger is auto-launched first.
//
// special_weapon_loaded is intentionally a semantic input here rather than a
// dependency on SpecialWeaponState, keeping the projectile module decoupled.
EnemyBombPlayerImpactResult resolve_enemy_bomb_player_impact(
    EnemyBombPool& pool,
    std::size_t index,
    bool player_shield_active,
    bool special_weapon_loaded);

} // namespace drone::gameplay
