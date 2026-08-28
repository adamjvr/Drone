#pragma once

#include <cstdint>
#include <optional>

namespace drone::gameplay {

// The encounter interstitial at Win32 0x0041D91A..0x0041D9DB renders this
// exact four-value summary from encounter-local counters 0x0047EC3C / 0x00466B04.
struct EncounterAlienStatistics {
    std::int32_t hit = 0;
    std::int32_t missed = 0;
    std::int32_t total = 0;
    std::int32_t percentage_hit = 0;
};

[[nodiscard]] std::optional<EncounterAlienStatistics> make_encounter_alien_statistics(
    std::int32_t hit,
    std::int32_t total) noexcept;

// Win32 0x0041E237..0x0041E25D adds the entire encounter-local pair into the
// mission-wide Results pair immediately before encounter-only reinitialization.
void fold_encounter_alien_statistics(
    std::int32_t encounter_hit,
    std::int32_t encounter_total,
    std::int32_t& mission_hit,
    std::int32_t& mission_total) noexcept;

} // namespace drone::gameplay
