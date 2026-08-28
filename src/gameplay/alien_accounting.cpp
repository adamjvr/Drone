#include <drone/gameplay/alien_accounting.hpp>

#include <cstdint>

namespace drone::gameplay {

std::optional<EncounterAlienStatistics> make_encounter_alien_statistics(
    const std::int32_t hit,
    const std::int32_t total) noexcept {
    if (total <= 0 || hit < 0 || hit > total) {
        return std::nullopt;
    }

    EncounterAlienStatistics result{};
    result.hit = hit;
    result.missed = total - hit;
    result.total = total;
    result.percentage_hit = static_cast<std::int32_t>(
        (static_cast<std::int64_t>(hit) * 100) / total);
    return result;
}

void fold_encounter_alien_statistics(
    const std::int32_t encounter_hit,
    const std::int32_t encounter_total,
    std::int32_t& mission_hit,
    std::int32_t& mission_total) noexcept {
    mission_total += encounter_total;
    mission_hit += encounter_hit;
}

} // namespace drone::gameplay
