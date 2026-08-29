#include <drone/gameplay/post_game.hpp>

#include <drone/gameplay/high_scores.hpp>

#include <limits>
#include <utility>

namespace drone::gameplay {

namespace {

PostGameModalPhase phase_after_results(const Win32PostGamePlan& plan) noexcept {
    if (plan.show_ordering_information) {
        return PostGameModalPhase::OrderingInformation;
    }
    if (plan.high_score.invoke_high_score_table) {
        return PostGameModalPhase::HighScoreTable;
    }
    if (plan.show_completion_credits) {
        return PostGameModalPhase::CompletionCredits;
    }
    return PostGameModalPhase::Complete;
}

PostGameModalPhase phase_after_ordering(const Win32PostGamePlan& plan) noexcept {
    if (plan.high_score.invoke_high_score_table) {
        return PostGameModalPhase::HighScoreTable;
    }
    if (plan.show_completion_credits) {
        return PostGameModalPhase::CompletionCredits;
    }
    return PostGameModalPhase::Complete;
}

PostGameModalPhase phase_after_high_scores(const Win32PostGamePlan& plan) noexcept {
    return plan.show_completion_credits
        ? PostGameModalPhase::CompletionCredits
        : PostGameModalPhase::Complete;
}

void mark_phase_start(
    PostGameRuntimeStepResult& result,
    const PostGameModalPhase phase) noexcept {
    result.ordering_information_started =
        phase == PostGameModalPhase::OrderingInformation;
    result.high_score_table_started =
        phase == PostGameModalPhase::HighScoreTable;
    result.completion_credits_started =
        phase == PostGameModalPhase::CompletionCredits;
}

} // namespace

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

bool begin_post_game_runtime(
    PostGameRuntimeState& runtime,
    Win32PostGamePlan plan) noexcept {
    if (runtime.phase != PostGameModalPhase::Inactive) {
        return false;
    }

    runtime.plan = std::move(plan);
    runtime.results_presentations_remaining = 0;
    if (runtime.plan->show_results_screen) {
        runtime.phase = PostGameModalPhase::ResultsConfirmLock;
        runtime.results_presentations_remaining =
            win32_results_confirm_lock_presentations;
    } else {
        runtime.phase = phase_after_results(*runtime.plan);
    }
    return true;
}

PostGameRuntimeStepResult step_post_game_runtime(
    PostGameRuntimeState& runtime,
    const PostGameModalInput& input) noexcept {
    PostGameRuntimeStepResult result{};
    result.phase = runtime.phase;
    result.results_presentations_remaining = runtime.results_presentations_remaining;

    if (!runtime.plan || runtime.phase == PostGameModalPhase::Inactive) {
        return result;
    }

    const auto transition_to = [&](const PostGameModalPhase next) {
        runtime.phase = next;
        result.advanced = true;
        mark_phase_start(result, next);
    };

    switch (runtime.phase) {
    case PostGameModalPhase::ResultsConfirmLock:
        // 0x00411AF4 tests the positive counter before calling 0x004174A0.
        // Thus confirmation is not polled during these 58 presentation loops.
        if (input.results_presentation_advanced &&
            runtime.results_presentations_remaining > 0) {
            --runtime.results_presentations_remaining;
            result.advanced = true;
            result.results_presentation_counted = true;
            if (runtime.results_presentations_remaining == 0) {
                runtime.phase = PostGameModalPhase::ResultsAwaitConfirmation;
                result.results_lock_expired = true;
            }
        }
        break;

    case PostGameModalPhase::ResultsAwaitConfirmation:
        if (input.confirm_pressed) {
            result.results_confirmation_accepted = true;
            transition_to(phase_after_results(*runtime.plan));
        }
        break;

    case PostGameModalPhase::OrderingInformation:
        if (input.ordering_information_finished) {
            transition_to(phase_after_ordering(*runtime.plan));
        }
        break;

    case PostGameModalPhase::HighScoreTable:
        if (input.high_score_table_finished) {
            transition_to(phase_after_high_scores(*runtime.plan));
        }
        break;

    case PostGameModalPhase::CompletionCredits:
        if (input.completion_credits_finished) {
            transition_to(PostGameModalPhase::Complete);
        }
        break;

    case PostGameModalPhase::Complete:
        result.completed = true;
        result.final_state = runtime.plan->final_state;
        break;

    case PostGameModalPhase::Inactive:
        break;
    }

    if (runtime.phase == PostGameModalPhase::Complete) {
        result.completed = true;
        result.final_state = runtime.plan->final_state;
    }
    result.phase = runtime.phase;
    result.results_presentations_remaining = runtime.results_presentations_remaining;
    return result;
}

} // namespace drone::gameplay
