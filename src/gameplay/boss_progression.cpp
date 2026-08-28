#include "drone/gameplay/boss_progression.hpp"

#include <array>

namespace drone::gameplay {
namespace {
constexpr std::array<BossFamily, 6> kBossDispatch = {
    BossFamily::LidTop,
    BossFamily::Gemini,
    BossFamily::RegisteredSlot2Unknown,
    BossFamily::Spidey,
    BossFamily::LidTop,
    BossFamily::Bomber,
};
} // namespace

std::optional<BossFamily>
boss_family_for_processed_drones(const std::uint8_t processed_drones) noexcept {
    if (processed_drones >= kBossDispatch.size()) {
        return std::nullopt;
    }
    return kBossDispatch[processed_drones];
}

bool shareware_campaign_reaches_boss_slot(const std::uint8_t processed_drones) noexcept {
    return processed_drones < 2;
}

} // namespace drone::gameplay
