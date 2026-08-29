#include <drone/gameplay/game_session.hpp>

#include <cassert>
#include <cstdint>
#include <iostream>

using namespace drone::gameplay;

namespace {

drone::HighScoreTable ordered_score_table() {
    drone::HighScoreTable table{};
    for (std::size_t i = 0; i < table.size(); ++i) {
        table[i].score = static_cast<std::int16_t>(i * 100);
    }
    return table;
}

GameSession make_results_session() {
    GameSession session{};
    session.campaign.player_lifecycle.lives = 0;
    session.campaign.player_lifecycle.player_active = false;
    session.campaign.mission.processed_count = 2;
    session.campaign.mission.outcomes[0] = DroneOutcome::Disarmed;
    session.campaign.mission.outcomes[1] = DroneOutcome::Detonated;
    session.campaign.alien_ships_hit = 3;
    session.campaign.alien_ships_total = 10;
    session.campaign.score.total = 0;
    session.high_scores = ordered_score_table();
    return session;
}

} // namespace

int main() {
    // Lives <= 0 enters the inline post-game tail before any ordinary gameplay
    // subsystem advances. Results remains under raw state 2 and begins with the
    // exact 58-presentation confirmation lock.
    {
        auto session = make_results_session();
        const auto random_before = session.original_random.state;
        const auto encounter_updates_before = session.encounter.gameplay_updates;
        const auto total_updates_before = session.total_gameplay_updates;

        const auto enter = step_game_session(session, GameplayInputFrame{});
        assert(enter.advanced);
        assert(enter.post_game_started);
        assert(!enter.post_game_plan_invalid);
        assert(enter.post_game_phase == PostGameModalPhase::ResultsConfirmLock);
        assert(enter.post_game_plan);
        assert(enter.post_game_plan->statistics.alien_ships_hit == 3);
        assert(enter.post_game_plan->statistics.alien_ships_missed == 7);
        assert(enter.post_game_plan->statistics.percentage_hit == 30);
        assert(session.state == GameState::ActiveGameplay);
        assert(session.post_game.results_presentations_remaining == 58);
        assert(session.original_random.state == random_before);
        assert(session.encounter.gameplay_updates == encounter_updates_before);
        assert(session.total_gameplay_updates == total_updates_before);
    }

    // Confirmation is not polled during any of the 58 locked presentations.
    // Even a held confirm on the 58th presentation is ignored; the first
    // acceptance opportunity is the following modal iteration.
    {
        auto session = make_results_session();
        (void)step_game_session(session, GameplayInputFrame{});

        for (std::int32_t i = 0; i < 57; ++i) {
            const auto step = step_game_session_post_game(
                session,
                PostGameModalInput{
                    .results_presentation_advanced = true,
                    .confirm_pressed = true,
                });
            assert(step.results_presentation_counted);
            assert(!step.results_confirmation_accepted);
            assert(step.phase == PostGameModalPhase::ResultsConfirmLock);
            assert(step.results_presentations_remaining == 57 - i);
            assert(session.state == GameState::ActiveGameplay);
        }

        const auto last_locked = step_game_session_post_game(
            session,
            PostGameModalInput{
                .results_presentation_advanced = true,
                .confirm_pressed = true,
            });
        assert(last_locked.results_presentation_counted);
        assert(last_locked.results_lock_expired);
        assert(!last_locked.results_confirmation_accepted);
        assert(last_locked.phase == PostGameModalPhase::ResultsAwaitConfirmation);
        assert(last_locked.results_presentations_remaining == 0);

        const auto confirmed = step_game_session_post_game(
            session, PostGameModalInput{.confirm_pressed = true});
        assert(confirmed.results_confirmation_accepted);
        assert(confirmed.ordering_information_started);
        assert(confirmed.phase == PostGameModalPhase::OrderingInformation);
        assert(session.state == GameState::OrderingInformation);

        const auto ordering_done = step_game_session_post_game(
            session, PostGameModalInput{.ordering_information_finished = true});
        assert(ordering_done.completed);
        assert(ordering_done.final_state == GameState::MainMenuResetEntry);
        assert(ordering_done.phase == PostGameModalPhase::Complete);
        assert(session.state == GameState::MainMenuResetEntry);
    }

    // Presentation suppression skips Results and Ordering Information but does
    // not skip high-score qualification. A score of 50 qualifies at slot zero,
    // so the raw state handoff is directly state 8 and the planner preserves the
    // original no-editor/no-persist placeholder interaction.
    {
        auto session = make_results_session();
        session.campaign.suppress_results_and_ordering = true;
        session.campaign.score.total = 50;

        const auto enter = step_game_session(session, GameplayInputFrame{});
        assert(enter.post_game_started && enter.post_game_plan);
        assert(enter.post_game_phase == PostGameModalPhase::HighScoreTable);
        assert(session.state == GameState::HighScores);
        assert(enter.post_game_plan->high_score.insertion_index == 0);
        assert(enter.post_game_plan->high_score.interaction ==
               HighScoreInsertionInteraction::BottomSlotPlaceholderOnly);
        assert(!enter.post_game_plan->high_score.persist_scores_file);

        const auto done = step_game_session_post_game(
            session, PostGameModalInput{.high_score_table_finished = true});
        assert(done.completed);
        assert(done.final_state == GameState::MainMenuReentry);
        assert(session.state == GameState::MainMenuReentry);
    }

    // Suppressed, ineligible sessions have no synchronous modal work at all and
    // can normalize to state 1 on the same post-game entry dispatch.
    {
        auto session = make_results_session();
        session.campaign.suppress_results_and_ordering = true;
        session.runtime.demo_playback_mode = true;
        session.campaign.score.total = 900;

        const auto enter = step_game_session(session, GameplayInputFrame{});
        assert(enter.post_game_started && enter.post_game_plan);
        assert(enter.post_game_phase == PostGameModalPhase::Complete);
        assert(session.state == GameState::MainMenuResetEntry);
        assert(!enter.post_game_plan->high_score.invoke_high_score_table);
    }

    // Perfect six-disarm + Mothership completion runs credits after the normal
    // Results/Ordering sequence and then lands in state 1.
    {
        GameSession session{};
        session.campaign.player_lifecycle.lives = 0;
        session.campaign.player_lifecycle.player_active = false;
        session.campaign.mission.processed_count = canonical_drone_objective_count;
        session.campaign.mission.outcomes.fill(DroneOutcome::Disarmed);
        session.campaign.mothership_destroyed = true;
        session.campaign.alien_ships_hit = 40;
        session.campaign.alien_ships_total = 40;
        session.campaign.score.total = 0;
        session.high_scores = ordered_score_table();

        const auto enter = step_game_session(session, GameplayInputFrame{});
        assert(enter.post_game_plan && enter.post_game_plan->show_completion_credits);

        for (std::int32_t i = 0; i < win32_results_confirm_lock_presentations; ++i) {
            (void)step_game_session_post_game(
                session, PostGameModalInput{.results_presentation_advanced = true});
        }
        (void)step_game_session_post_game(
            session, PostGameModalInput{.confirm_pressed = true});
        const auto ordering_done = step_game_session_post_game(
            session, PostGameModalInput{.ordering_information_finished = true});
        assert(ordering_done.completion_credits_started);
        assert(ordering_done.phase == PostGameModalPhase::CompletionCredits);
        assert(session.state == GameState::MainMenuResetEntry);

        const auto credits_done = step_game_session_post_game(
            session, PostGameModalInput{.completion_credits_finished = true});
        assert(credits_done.completed);
        assert(credits_done.final_state == GameState::MainMenuResetEntry);
        assert(session.state == GameState::MainMenuResetEntry);
    }

    // Persisted high scores survive a full gameplay-campaign reset, while the
    // inline post-game modal state does not.
    {
        auto session = make_results_session();
        session.high_scores[9].name = "KEEP ME";
        (void)step_game_session(session, GameplayInputFrame{});
        assert(session.post_game.phase != PostGameModalPhase::Inactive);

        reset_game_session(session, GameplaySessionResetScope::FullCampaign);
        assert(session.high_scores[9].name == "KEEP ME");
        assert(session.post_game.phase == PostGameModalPhase::Inactive);
        assert(session.state == GameState::ActiveGameplay);
        assert(session.campaign.player_lifecycle.lives == 3);
    }

    std::cout << "Drone GameSession post-game continuity tests passed\n";
    return 0;
}
