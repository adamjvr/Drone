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

// Lid/Top initializes the top/root active and the lid in activity 6. The
// unrecovered activity-6 -> vulnerable/activity-1 movement/attack producer is
// intentionally not guessed here.
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

// Phase 4 owns the persistent lifecycle/score state for the two bosses that are
// reachable by the canonical shareware campaign. Geometry, movement, bombs,
// exact sprite-mask collisions, audio and debris/effect emission remain their
// own producers until those contracts are integrated.
struct BossEncounterState {
    std::optional<BossFamily> family{};
    LidTopBossLifecycleState lid_top{};
    GeminiBossLifecycleState gemini{};
};

// These are not raw collision hits. They are already-validated transitions
// emitted by the exact boss-local collision/damage producer. Keeping that
// distinction explicit prevents this clean owner from inventing hit masks,
// vulnerability gates or Gemini damage-threshold semantics.
enum class SharewareBossDestructionTrigger : std::uint8_t {
    LidTopLid,
    GeminiSideA,
    GeminiSideB,
};

struct BossEncounterStepResult {
    std::size_t destruction_transitions = 0;
    std::size_t components_retired = 0;
    std::int32_t score_delta = 0;

    // Lid/Top's 25-count transition zeros the top/root motion in the original.
    // Movement is still externally owned, so publish that exact side effect as
    // an event instead of introducing guessed boss-motion state here.
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
