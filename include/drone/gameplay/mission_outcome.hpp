#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace drone::gameplay {

inline constexpr std::size_t canonical_drone_objective_count = 6;

enum class DroneOutcome : std::uint8_t {
    Unresolved = 0,
    Disarmed = 1,
    Detonated = 2,
};

enum class MissionResultsMusic : std::uint8_t {
    Choral,
    Suspense,
    Moon,
    Hiphop,
};

struct MissionOutcomeState {
    std::array<DroneOutcome, canonical_drone_objective_count> outcomes{};
    std::size_t processed_count = 0;
};

struct MissionOutcomeSummary {
    std::size_t disarmed = 0;
    std::size_t detonated = 0;
    MissionResultsMusic music = MissionResultsMusic::Suspense;
    std::uint8_t disarm_art_index = 0; // disarm0.jba .. disarm6.jba
    bool use_mothership_result_art = false;
};

// Reconstructs the established post-game result-selection logic beginning near
// Win32 0x004116CF. The original condition is Mothership core-target state == 2;
// by the time results are reached this is the successful Mothership-destruction
// outcome, also persisted as the high-score mothership_destroyed statistic.
MissionOutcomeSummary summarize_mission_outcomes(
    const MissionOutcomeState& state,
    bool mothership_destroyed);

} // namespace drone::gameplay
