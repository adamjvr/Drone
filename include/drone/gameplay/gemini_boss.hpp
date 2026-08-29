#pragma once

#include <drone/gameplay/boss_encounter.hpp>
#include <drone/gameplay/difficulty.hpp>
#include <drone/gameplay/enemy_bomb.hpp>
#include <drone/gameplay/original_random.hpp>
#include <drone/gameplay/scoring.hpp>
#include <drone/gameplay/special_weapon.hpp>
#include <drone/gameplay/trajectory_collision.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace drone::gameplay {

inline constexpr std::int16_t gemini_body_width = 56;
inline constexpr std::int16_t gemini_body_height = 41;
inline constexpr std::int16_t gemini_head_width = 43;
inline constexpr std::int16_t gemini_head_height = 34;
inline constexpr std::size_t gemini_body_frame_count = 30;

struct GeminiBossSpriteMaskView {
    std::array<std::span<const std::uint8_t>, gemini_body_frame_count> body_frames{};
    std::span<const std::uint8_t> head_frame{};
};

struct GeminiBossStepResult {
    bool root_moved = false;
    bool vertical_retreat_started = false;
    bool enemy_bomb_spawned = false;
    std::optional<std::size_t> enemy_bomb_spawn_index{};
    bool special_hit_side_a = false;
    bool special_hit_side_b = false;
    bool special_hit_head = false;
    bool special_hit_body = false;
    std::uint8_t special_damage = 0;
    bool stinger_display_activated = false;
    std::size_t destruction_transitions = 0;
    std::size_t components_retired = 0;
    std::int32_t score_delta = 0;
};

// Exact gameplay-relevant initializer subset from Win32 0x00405EF0 plus the
// already-established 30-frame body/head asset metadata and difficulty-specific
// head-damage thresholds initialized during session setup.
void initialize_gemini_boss_runtime(
    GeminiBossLifecycleState& state,
    DifficultyLevel difficulty) noexcept;

// Native shareware Gemini gameplay from Win32 0x00405000: shared-root motion,
// opposing body animation, phase-2 bomb emission, independent 20-tick body
// destruction tails, and launched Probe/Stinger opaque-pixel damage against the
// head-first/body-second sprite pair. Procedural point-particle/debris rendering
// remains presentation-side; immutable body/head frame pixels are the only
// collision asset input.
[[nodiscard]] GeminiBossStepResult step_gemini_boss(
    GeminiBossLifecycleState& state,
    std::int32_t gameplay_substep_phase,
    std::int32_t player_x,
    DifficultyLevel difficulty,
    OriginalRandomState& random,
    EnemyBombPool& enemy_bombs,
    EnemyBombSpawnGate& bomb_spawn_gate,
    SpecialWeaponState& special,
    StingerDisplayState& stinger_display,
    ScoreState& score,
    const GeminiBossSpriteMaskView* sprite_masks = nullptr) noexcept;

} // namespace drone::gameplay
