#pragma once

#include <drone/high_score.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>

namespace drone::gameplay {

// The post-results qualifier is bypassed for demos and for sessions marked by
// the nine-lives cheat. This mirrors the two explicit Win32 gates.
bool high_score_session_eligible(bool demo_playback_mode,
                                 bool high_score_disqualified);

// The original table is ordered lowest score at index 0 to highest at index 9.
// Qualification uses a strict 'new_score > existing_score' comparison. The
// final matching index is the insertion point, so equal scores remain ahead of
// a new tied score.
std::optional<std::size_t> find_high_score_insertion_index(
    const HighScoreTable& table,
    std::int32_t new_score,
    bool demo_playback_mode,
    bool high_score_disqualified);

// Insertion discards the lowest entry, shifts entries [1..index] one slot
// toward zero, then installs the new entry at index. Entries above index are
// unchanged. Returns false only for an out-of-range index.
bool insert_high_score(HighScoreTable& table,
                       std::size_t index,
                       HighScoreEntry entry);

} // namespace drone::gameplay
