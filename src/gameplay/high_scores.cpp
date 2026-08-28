#include <drone/gameplay/high_scores.hpp>

#include <utility>

namespace drone::gameplay {

bool high_score_session_eligible(const bool demo_playback_mode,
                                 const bool high_score_disqualified) {
    return !demo_playback_mode && !high_score_disqualified;
}

std::optional<std::size_t> find_high_score_insertion_index(
    const HighScoreTable& table,
    const std::int32_t new_score,
    const bool demo_playback_mode,
    const bool high_score_disqualified) {
    if (!high_score_session_eligible(demo_playback_mode, high_score_disqualified)) {
        return std::nullopt;
    }

    std::optional<std::size_t> insertion;
    for (std::size_t i = 0; i < table.size(); ++i) {
        if (new_score > static_cast<std::int32_t>(table[i].score)) {
            insertion = i;
        }
    }
    return insertion;
}

bool insert_high_score(HighScoreTable& table,
                       const std::size_t index,
                       HighScoreEntry entry) {
    if (index >= table.size()) {
        return false;
    }

    for (std::size_t i = 0; i < index; ++i) {
        table[i] = std::move(table[i + 1]);
    }
    table[index] = std::move(entry);
    return true;
}

} // namespace drone::gameplay
