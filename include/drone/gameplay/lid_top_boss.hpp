#pragma once

#include <drone/gameplay/boss_encounter.hpp>
#include <drone/gameplay/difficulty.hpp>
#include <drone/gameplay/enemy_bomb.hpp>
#include <drone/gameplay/original_random.hpp>
#include <drone/gameplay/rapid_missile.hpp>
#include <drone/gameplay/scoring.hpp>
#include <drone/gameplay/special_weapon.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace drone::gameplay {

inline constexpr std::int16_t lid_top_top_width = 68;
inline constexpr std::int16_t lid_top_top_height = 56;
inline constexpr std::int16_t lid_top_top_hitbox_width = 57;
inline constexpr std::int16_t lid_top_top_hitbox_height = 47;
inline constexpr std::int16_t lid_top_missile_weakpoint_width = 11;
inline constexpr std::int16_t lid_top_missile_weakpoint_height = 6;
inline constexpr std::int16_t lid_top_missile_weakpoint_hitbox_width = 9;
inline constexpr std::int16_t lid_top_missile_weakpoint_hitbox_height = 5;
inline constexpr std::int16_t lid_top_stinger_core_width = 13;
inline constexpr std::int16_t lid_top_stinger_core_height = 5;
inline constexpr std::int16_t lid_top_stinger_core_hitbox_width = 11;
inline constexpr std::int16_t lid_top_stinger_core_hitbox_height = 4;

struct LidTopBossSpriteMaskView {
    // top.jba has one recovered 68x56 frame. Palette index zero is transparent.
    std::span<const std::uint8_t> top_frame{};
};

struct LidTopBossStepResult {
    bool root_moved = false;
    bool vertical_retreat_started = false;
    bool enemy_bomb_spawned = false;
    std::optional<std::size_t> enemy_bomb_spawn_index{};
    std::size_t rapid_missiles_consumed = 0;
    std::size_t rapid_top_opaque_collisions = 0;
    std::size_t rapid_lid_open_collisions = 0;
    bool lid_opened = false;
    bool lid_close_started = false;
    bool special_closed_top_impact = false;
    bool stinger_core_hit = false;
    std::size_t destruction_transitions = 0;
    std::size_t components_retired = 0;
    std::int32_t score_delta = 0;
    bool top_motion_stopped = false;
};

// Exact initializer subset from Win32 0x00417220. This owns the root fixed-point
// position/motion and the lid animation state. Audio-only companion state is
// deliberately not represented.
void initialize_lid_top_boss_runtime(
    LidTopBossLifecycleState& state,
    DifficultyLevel difficulty,
    bool demo_playback_mode) noexcept;

// Reconstruct the shareware-reachable gameplay portion of Win32 0x00416700:
// root movement/tracking, boss bomb spawn, launched-special collision gates,
// 25-update lid destruction, rapid-missile top/weak-point ordering, and phase-2
// lid opening/closing animation. Randomized debris/audio remain presentation
// events. `sprite_mask` is immutable original-asset data used only by the
// top.jba opaque-pixel rapid-missile shield test.
[[nodiscard]] LidTopBossStepResult step_lid_top_boss(
    LidTopBossLifecycleState& state,
    std::int32_t gameplay_substep_phase,
    std::int32_t player_x,
    DifficultyLevel difficulty,
    bool demo_playback_mode,
    OriginalRandomState& random,
    EnemyBombPool& enemy_bombs,
    EnemyBombSpawnGate& bomb_spawn_gate,
    RapidMissilePool& rapid_missiles,
    SpecialWeaponState& special,
    ScoreState& score,
    const LidTopBossSpriteMaskView* sprite_mask = nullptr) noexcept;

} // namespace drone::gameplay
