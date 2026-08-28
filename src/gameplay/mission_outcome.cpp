#include <drone/gameplay/mission_outcome.hpp>

#include <algorithm>

namespace drone::gameplay {

MissionOutcomeSummary summarize_mission_outcomes(
    const MissionOutcomeState& state,
    const bool mothership_destroyed) {

    MissionOutcomeSummary result{};
    const std::size_t count = std::min(state.processed_count, state.outcomes.size());
    for (std::size_t i = 0; i < count; ++i) {
        if (state.outcomes[i] == DroneOutcome::Disarmed) {
            ++result.disarmed;
        } else if (state.outcomes[i] == DroneOutcome::Detonated) {
            ++result.detonated;
        }
    }

    result.disarm_art_index = static_cast<std::uint8_t>(
        std::min(result.disarmed, canonical_drone_objective_count));
    result.use_mothership_result_art =
        result.disarmed == canonical_drone_objective_count && mothership_destroyed;

    // Exact branch order from 0x00411718..0x00411759.
    if (result.disarmed == canonical_drone_objective_count && mothership_destroyed) {
        result.music = MissionResultsMusic::Hiphop;
    } else if (result.detonated > 4) {
        result.music = MissionResultsMusic::Moon;
    } else if (result.disarmed < 4) {
        result.music = MissionResultsMusic::Suspense;
    } else {
        result.music = MissionResultsMusic::Choral;
    }

    return result;
}

} // namespace drone::gameplay
