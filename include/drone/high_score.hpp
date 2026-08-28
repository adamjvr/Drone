#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace drone {

inline constexpr std::size_t high_score_entry_count = 10;
inline constexpr std::size_t high_score_name_storage_bytes = 30;
inline constexpr std::size_t high_score_interactive_name_max_chars = 25;

// Both original builds persist ten high-score entries. Win32 stores the values
// in parallel arrays; DOS uses corresponding arrays. All four numeric fields
// are now semantically established. The mothership field remains int16_t here
// (rather than bool) because the legacy file stores it as a decimal integer.
struct HighScoreEntry {
    std::string name;
    std::int16_t drones_disarmed = 0;
    std::int16_t score = 0;
    std::int16_t mothership_destroyed = 0;
    std::int16_t percentage_hit = 0;
};

using HighScoreTable = std::array<HighScoreEntry, high_score_entry_count>;

} // namespace drone
