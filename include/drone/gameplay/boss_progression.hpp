#pragma once

#include <cstdint>
#include <optional>

namespace drone::gameplay {

enum class BossFamily : std::uint8_t {
    LidTop,
    Gemini,
    RegisteredSlot2Unknown,
    Spidey,
    Bomber,
};

// The original Win32 state-2 boss dispatch is indexed by the number of Drone
// outcomes already processed. Valid indices are 0..5.
[[nodiscard]] std::optional<BossFamily>
boss_family_for_processed_drones(std::uint8_t processed_drones) noexcept;

// In the canonical shareware campaign, the explicit two-level stop occurs
// after the second Drone outcome. Therefore only dispatch slots 0 and 1 are
// normally reachable through the shareware campaign flow.
[[nodiscard]] bool
shareware_campaign_reaches_boss_slot(std::uint8_t processed_drones) noexcept;

} // namespace drone::gameplay
