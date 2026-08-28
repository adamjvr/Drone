#pragma once

#include <drone/gameplay/game_state.hpp>
#include <drone/gameplay/mission_outcome.hpp>
#include <drone/high_score.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>

namespace drone::gameplay {

// Win32 result presentation at 0x00411AEA holds confirmation input disabled
// for exactly 0x3A present/vblank iterations before 0x004174A0 is consulted.
inline constexpr std::int32_t win32_results_confirm_lock_presentations = 58;
inline constexpr const char* win32_high_score_bottom_slot_placeholder_name =
    "ENTER YOUR NAME";

[[nodiscard]] constexpr bool win32_enters_post_game_results(
    const std::int32_t player_lives) noexcept {
    return player_lives <= 0;
}

struct PostGameStatistics {
    std::int32_t alien_ships_hit = 0;
    std::int32_t alien_ships_missed = 0;
    std::int32_t alien_ships_total = 0;
    std::int32_t percentage_hit = 0;
    std::int32_t score = 0;
    std::size_t drones_disarmed = 0;
};

enum class HighScoreInsertionInteraction : std::uint8_t {
    None,
    // Original Win32 index-0 quirk: the candidate is installed with the
    // literal placeholder "ENTER YOUR NAME", but no editor is entered and
    // save_high_scores_file is not called on that path.
    BottomSlotPlaceholderOnly,
    InteractiveNameAndPersist,
};

struct PostGameHighScorePlan {
    bool eligible = false;
    std::optional<std::size_t> insertion_index;
    HighScoreInsertionInteraction interaction = HighScoreInsertionInteraction::None;
    bool invoke_high_score_table = false;
    bool prompt_for_name = false;
    bool persist_scores_file = false;
    std::uint8_t disarmed_count_argument = 0;
};

struct Win32PostGameContext {
    MissionOutcomeState mission{};
    bool mothership_destroyed = false;
    bool suppress_results_and_ordering = false;
    bool demo_playback_mode = false;
    bool high_score_disqualified = false;
    std::int32_t score = 0;
    std::int32_t alien_ships_hit = 0;
    std::int32_t alien_ships_total = 0;
};

struct Win32PostGamePlan {
    MissionOutcomeSummary outcome_summary{};
    PostGameStatistics statistics{};
    bool show_results_screen = true;
    bool show_ordering_information = true;
    bool show_completion_credits = false;
    PostGameHighScorePlan high_score{};
    GameState final_state = GameState::MainMenuResetEntry;
};

// Models the established Win32 post-game region beginning at 0x004115BE.
// Returns nullopt only for impossible/unsafe clean inputs that the original
// code assumed valid (processed_count > 6, negative hit counts, or a zero /
// inconsistent alien total that would make its integer percentage divide
// invalid). Presentation suppression is intentionally independent of score
// qualification, matching the original quit/abort flag behavior.
[[nodiscard]] std::optional<Win32PostGamePlan> win32_post_game_plan(
    const Win32PostGameContext& context,
    const HighScoreTable& table) noexcept;

} // namespace drone::gameplay
