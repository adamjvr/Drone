#pragma once

#include <drone/gameplay/boss_progression.hpp>
#include <drone/gameplay/scoring.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace drone::gameplay {

// Common activity values used by the recovered Win32 boss roots.
inline constexpr std::uint8_t boss_activity_inactive = 0;
inline constexpr std::uint8_t boss_activity_active = 1;
inline constexpr std::uint8_t boss_activity_destruction = 2;

// Lid/Top initializes the top/root active and the lid in activity 6. Native
// movement, bomb emission and vulnerability transitions are reconstructed in
// lid_top_boss.hpp; these constants remain shared with the lifecycle owner.
inline constexpr std::uint8_t lid_top_initial_lid_activity = 6;
inline constexpr std::uint16_t lid_top_lid_destruction_updates = 25;
inline constexpr std::uint16_t lid_top_top_destruction_phase2_ticks = 30;

// Gemini body destruction counters advance only on gameplay phase 2 and retire
// the corresponding body at exactly 20 ticks.
inline constexpr std::uint16_t gemini_body_destruction_phase2_ticks = 20;
inline constexpr std::int32_t canonical_shareware_boss_score_award = 100;

struct LidTopBossLifecycleState {
    std::uint8_t top_activity = boss_activity_inactive;
    std::uint8_t lid_activity = boss_activity_inactive;
    std::uint16_t lid_destruction_progress = 0;
    std::uint16_t top_destruction_progress = 0;

    // Native Win32 0x00417220 / 0x00416700 runtime state. The original stores
    // integer position at +0/+4, 16.16 position at +8/+0x0C, and motion at
    // +0x10/+0x14 on the 68x56 top/root common entity.
    std::uint8_t lid_frame = 0;
    std::int32_t root_x = 0;
    std::int32_t root_y = -100;
    std::int32_t root_fixed_x = 0;
    std::int32_t root_fixed_y = -100 * 65536;
    std::int32_t root_velocity_x = 0;
    std::int32_t root_velocity_y = 0;
    std::int32_t horizontal_speed_cap = 0;
    bool runtime_initialized = false;
};

struct GeminiBossSideLifecycleState {
    std::uint8_t body_activity = boss_activity_inactive;
    std::uint8_t head_activity = boss_activity_inactive;
    std::uint16_t body_destruction_progress = 0;
};

struct GeminiBossLifecycleState {
    GeminiBossSideLifecycleState side_a{};
    GeminiBossSideLifecycleState side_b{};
};

// Phase 4 owns persistent state for both shareware bosses. Lid/Top geometry,
// movement, bombs and weapon vulnerability are native; Gemini local movement
// and damage production remain separate until their contracts are integrated.
struct BossEncounterState {
    std::optional<BossFamily> family{};
    LidTopBossLifecycleState lid_top{};
    GeminiBossLifecycleState gemini{};
};

// These are not raw collision hits. GameSession now produces Lid/Top combat
// natively; this legacy enum remains useful to the low-level lifecycle tests and
// carries Gemini already-validated transitions until its producer is native.
enum class SharewareBossDestructionTrigger : std::uint8_t {
    LidTopLid,
    GeminiSideA,
    GeminiSideB,
};

struct BossEncounterStepResult {
    std::size_t destruction_transitions = 0;
    std::size_t components_retired = 0;
    std::int32_t score_delta = 0;

    // Legacy lifecycle-only callers still surface the Lid/Top motion-stop side
    // effect as an event; GameSession native Lid/Top owns the actual root motion.
    bool lid_top_motion_stop_requested = false;
};

// Initialize exactly the activity-state subset recovered for the canonical
// shareware boss dispatch slots (0 = Lid/Top, 1 = Gemini). Returns false for a
// duplicate activation or any registered-only/nonexistent dispatch slot.
[[nodiscard]] bool activate_shareware_boss_for_processed_drones(
    BossEncounterState& state,
    std::uint8_t processed_drones) noexcept;

// Advance the lifecycle/score portion of the active shareware boss. The caller
// supplies the already-advanced gameplay substep phase and any exact validated
// destruction transitions produced during this logical state-2 update.
[[nodiscard]] BossEncounterStepResult step_shareware_boss_encounter(
    BossEncounterState& state,
    std::int32_t gameplay_substep_phase,
    std::span<const SharewareBossDestructionTrigger> destruction_triggers,
    ScoreState& score) noexcept;

} // namespace drone::gameplay
