#pragma once

#include <drone/audio/audio_event.hpp>
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

// The original post-game region is inline inside raw game state 2.  Results,
// Ordering Information, the high-score table and completion credits are
// synchronous/modal calls rather than independent dispatcher states.  Keep a
// separate semantic phase so the clean GameSession can own that control flow
// without inventing a nonexistent "Results" GameState value.
enum class PostGameModalPhase : std::uint8_t {
    Inactive,
    ResultsConfirmLock,
    ResultsAwaitConfirmation,
    OrderingInformation,
    HighScoreTable,
    CompletionCredits,
    Complete,
};

struct PostGameRuntimeState {
    PostGameModalPhase phase = PostGameModalPhase::Inactive;
    std::optional<Win32PostGamePlan> plan{};
    std::int32_t results_presentations_remaining = 0;
};

// One host/UI iteration of the already-recovered synchronous modal paths.
// `results_presentation_advanced` means one original result-screen
// present/vblank iteration completed. Confirmation is deliberately separate:
// Win32 does not poll 0x004174A0 until the 58 locked presentations are over.
struct PostGameModalInput {
    bool results_presentation_advanced = false;
    bool confirm_pressed = false;
    bool ordering_information_finished = false;
    bool high_score_table_finished = false;
    bool completion_credits_finished = false;
};

struct PostGameRuntimeStepResult {
    // GameSession fills this queue at semantic modal ownership boundaries.
    // The lower-level step_post_game_runtime() remains presentation/audio agnostic.
    drone::audio::AudioEventQueue audio_events{};
    bool advanced = false;
    bool results_presentation_counted = false;
    bool results_lock_expired = false;
    bool results_confirmation_accepted = false;
    bool ordering_information_started = false;
    bool high_score_table_started = false;
    bool completion_credits_started = false;
    bool completed = false;
    std::optional<GameState> final_state{};
    PostGameModalPhase phase = PostGameModalPhase::Inactive;
    std::int32_t results_presentations_remaining = 0;
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

// Begin the modal sequence described by an already-validated plan. Returns
// false only if a post-game sequence is already active.
[[nodiscard]] bool begin_post_game_runtime(
    PostGameRuntimeState& runtime,
    Win32PostGamePlan plan) noexcept;

// Advance one modal/UI iteration. Presentation and persistence are still host
// responsibilities; the ordering, raw-state handoff points and confirmation
// lock are owned here.
[[nodiscard]] PostGameRuntimeStepResult step_post_game_runtime(
    PostGameRuntimeState& runtime,
    const PostGameModalInput& input) noexcept;

} // namespace drone::gameplay
