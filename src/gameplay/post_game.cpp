#include <drone/gameplay/post_game.hpp>

#include <drone/gameplay/high_scores.hpp>

#include <limits>

namespace drone::gameplay {

std::optional<Win32PostGamePlan> win32_post_game_plan(
    const Win32PostGameContext& context,
    const HighScoreTable& table) noexcept {
    if (context.mission.processed_count > canonical_drone_objective_count ||
        context.alien_ships_total <= 0 ||
        context.alien_ships_hit < 0 ||
        context.alien_ships_hit > context.alien_ships_total) {
        return std::nullopt;
    }

    Win32PostGamePlan plan{};
    plan.outcome_summary = summarize_mission_outcomes(
        context.mission, context.mothership_destroyed);

    plan.statistics.alien_ships_hit = context.alien_ships_hit;
    plan.statistics.alien_ships_missed =
        context.alien_ships_total - context.alien_ships_hit;
    plan.statistics.alien_ships_total = context.alien_ships_total;
    plan.statistics.percentage_hit =
        static_cast<std::int32_t>(
            (static_cast<std::int64_t>(context.alien_ships_hit) * 100) /
            context.alien_ships_total);
    plan.statistics.score = context.score;
    plan.statistics.drones_disarmed = plan.outcome_summary.disarmed;

    plan.show_results_screen = !context.suppress_results_and_ordering;
    plan.show_ordering_information = !context.suppress_results_and_ordering;
    plan.show_completion_credits = plan.outcome_summary.use_mothership_result_art;

    plan.high_score.eligible = high_score_session_eligible(
        context.demo_playback_mode, context.high_score_disqualified);
    plan.high_score.disarmed_count_argument = static_cast<std::uint8_t>(
        plan.outcome_summary.disarmed);

    plan.high_score.insertion_index = find_high_score_insertion_index(
        table,
        context.score,
        context.demo_playback_mode,
        context.high_score_disqualified);

    if (plan.high_score.insertion_index) {
        plan.high_score.invoke_high_score_table = true;
        if (*plan.high_score.insertion_index == 0) {
            plan.high_score.interaction =
                HighScoreInsertionInteraction::BottomSlotPlaceholderOnly;
        } else {
            plan.high_score.interaction =
                HighScoreInsertionInteraction::InteractiveNameAndPersist;
            plan.high_score.prompt_for_name = true;
            plan.high_score.persist_scores_file = true;
        }
    }

    // Ordinary post-results insertion returns through state 4. No qualifying
    // score returns through state 1. Perfect six-disarm + Mothership completion
    // always proceeds through credits and is normalized to state 1 by the
    // outer results path, regardless of high-score qualification.
    if (plan.show_completion_credits) {
        plan.final_state = GameState::MainMenuResetEntry;
    } else if (plan.high_score.invoke_high_score_table) {
        plan.final_state = GameState::MainMenuReentry;
    } else {
        plan.final_state = GameState::MainMenuResetEntry;
    }

    return plan;
}

} // namespace drone::gameplay
