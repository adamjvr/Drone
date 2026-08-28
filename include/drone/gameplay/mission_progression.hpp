#pragma once

#include <drone/gameplay/mission_outcome.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>

namespace drone::gameplay {

// Exact Win32 state-2 landmarks for the non-destructive Drone resolution path.
inline constexpr std::int32_t canonical_drone_disarm_commit_y = 201;
inline constexpr std::int32_t canonical_drone_post_disarm_y = 202;
inline constexpr std::int32_t canonical_drone_settlement_timer_reset_y = 230;
inline constexpr std::int32_t canonical_drone_transition_min_y = 230;
inline constexpr std::int32_t canonical_drone_transition_settlement_tick = 60;
inline constexpr std::int32_t canonical_drone_settlement_tick_cap = 61;
inline constexpr std::uint8_t canonical_drone_destruction_activity = 2;

// 0x00417F50 is called with 1 for a new campaign/session and with 0 by the
// post-objective transition routine. The zero form deliberately preserves
// mission-wide score/lives/outcomes/processed-count while rebuilding the
// per-encounter gameplay state.
enum class GameplaySessionResetScope : std::uint8_t {
    EncounterOnly,
    FullCampaign,
};

[[nodiscard]] constexpr GameplaySessionResetScope gameplay_session_reset_scope(
    const bool full_campaign_argument) noexcept {
    return full_campaign_argument ? GameplaySessionResetScope::FullCampaign
                                  : GameplaySessionResetScope::EncounterOnly;
}

// Exact normal-resolution commit at Drone Y == 201. Destruction activity 2 is
// excluded. In the original the current slot is written to Disarmed only when
// still zero, but processed_count advances once the boundary is accepted.
[[nodiscard]] bool commit_disarmed_drone_at_boundary(
    MissionOutcomeState& state,
    std::int32_t drone_y,
    std::uint8_t drone_activity) noexcept;

// Exact destructive-outcome bookkeeping performed by the detonation path.
[[nodiscard]] bool commit_detonated_drone(MissionOutcomeState& state) noexcept;

// The shared settlement scalar advances only on gameplay phase 2 and saturates
// at 61. The mission transition tests for exactly 60 after Drone Y has passed
// 230, so callers should preserve the exact phase ordering.
[[nodiscard]] std::int32_t advance_drone_settlement_tick(
    std::int32_t current_tick,
    std::int32_t gameplay_phase) noexcept;

[[nodiscard]] constexpr bool drone_resolution_transition_ready(
    const std::int32_t drone_y,
    const std::int32_t settlement_tick) noexcept {
    return drone_y > canonical_drone_transition_min_y &&
           settlement_tick == canonical_drone_transition_settlement_tick;
}

enum class MissionInterstitialTone : std::uint8_t {
    Good,
    Bad,
};

enum class MissionInterstitialSound : std::uint8_t {
    Deepness,
    Detonate,
};

enum class MissionBriefingCard : std::uint8_t {
    Mission1,
    Mission2,
    Mission3,
    Mission4,
    Mission5,
    Mission6Yes,
    Mission6No,
};

struct MissionInterstitialPlan {
    MissionInterstitialTone tone = MissionInterstitialTone::Good;
    MissionInterstitialSound sound = MissionInterstitialSound::Deepness;
    MissionBriefingCard briefing = MissionBriefingCard::Mission1;
    std::uint8_t result_ordinal = 0; // good1..6 or bad1..6 in original assets
    std::uint8_t processed_count = 0;
    std::uint8_t disarmed_count = 0;
    std::uint8_t detonated_count = 0;
};

// Reconstructs the two-card interstitial at 0x0041D690 without depending on
// proprietary art/audio: first goodN/badN plus deepness/detonate sound, then
// mission1..5 or miss6yes/miss6no.
[[nodiscard]] std::optional<MissionInterstitialPlan>
mission_interstitial_plan(const MissionOutcomeState& state) noexcept;

enum class SceneryTransitionPlan : std::uint8_t {
    DesertStack,
    SharewareTerminationDesertBottomOnly,
    IsleStack,
    HouseStack,
    NightStack,
    RiverStack,
    RegisteredTerminationNightBottomOnly,
};

enum class EncounterTransitionTarget : std::uint8_t {
    Gemini,
    Results,
    Spidey,
    LidTop,
    Bomber,
    Mothership,
};

enum class EncounterTransitionDisposition : std::uint8_t {
    ContinueCampaign,
    EndRun,
    EnterMothershipEndgame,
};

struct EncounterTransitionPlan {
    SceneryTransitionPlan scenery = SceneryTransitionPlan::DesertStack;
    EncounterTransitionTarget target = EncounterTransitionTarget::Results;
    EncounterTransitionDisposition disposition = EncounterTransitionDisposition::EndRun;
    bool use_encounter_only_reset = true;
    bool latent_registered_branch = false;
};

// Exact six-way dispatch compiled into the canonical shareware Win32 PE. Count
// 2 is the normal shareware termination gate, making branches 3..6 dormant in
// ordinary shareware play. They are retained here as executable evidence, not
// asserted as byte-for-byte retail behavior. Branch 6 proves the intended
// all-six-disarmed -> Mothership endgame relation.
[[nodiscard]] std::optional<EncounterTransitionPlan>
win32_post_drone_transition_plan(
    std::uint8_t processed_count,
    std::uint8_t detonated_count) noexcept;

} // namespace drone::gameplay
