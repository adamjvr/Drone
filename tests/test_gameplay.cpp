#include <drone/gameplay/boss_progression.hpp>
#include <drone/gameplay/enemy_bomb.hpp>
#include <drone/gameplay/demo_replay.hpp>
#include <drone/gameplay/input.hpp>
#include <drone/gameplay/debris_effects.hpp>
#include <drone/gameplay/game_over.hpp>
#include <drone/gameplay/game_state.hpp>
#include <drone/gameplay/gameplay_phase.hpp>
#include <drone/gameplay/gameplay_update_order.hpp>
#include <drone/gameplay/world_scroll.hpp>
#include <drone/gameplay/high_scores.hpp>
#include <drone/gameplay/mission_outcome.hpp>
#include <drone/gameplay/mission_progression.hpp>
#include <drone/gameplay/post_game.hpp>
#include <drone/gameplay/player_lifecycle.hpp>
#include <drone/gameplay/scoring.hpp>
#include <drone/gameplay/timing.hpp>
#include <drone/gameplay/trajectory.hpp>
#include <drone/gameplay/trajectory_templates.hpp>
#include <drone/formats/scores.hpp>
#include <drone/fidelity/world_viewport.hpp>

#include <cassert>
#include <cstddef>
#include <string>
#include <vector>

int main() {
    using namespace drone::gameplay;

    // Deferred life settlement: collision itself does not spend a life.
    {
        PlayerLifecycleState lifecycle{};
        PlayerMotionState player{};
        PlayerShieldState shield{};
        shield.energy = 123;
        PlayerRespawnGate blocked{};
        const auto no_change = settle_player_death(lifecycle, player, shield, blocked);
        assert(!no_change.consumed_life && lifecycle.lives == 3);

        const PlayerRespawnGate ready{true, true, true, true};
        const auto respawn = settle_player_death(lifecycle, player, shield, ready);
        assert(respawn.consumed_life && respawn.respawned && !respawn.game_over);
        assert(lifecycle.lives == 2 && lifecycle.player_active);
        assert(player.x == 147 && player.y == 175 && player.frame == 0);
        assert(shield.energy == shield_nominal_max_energy);

        lifecycle.lives = 1;
        lifecycle.player_active = false;
        const auto game_over = settle_player_death(lifecycle, player, shield, ready);
        assert(game_over.consumed_life && game_over.game_over && !game_over.respawned);
        assert(lifecycle.lives == 0 && !lifecycle.player_active);
    }

    // Game-over banner: canonical fixed-point slide presents 108 iterations
    // and lands at X=100 with velocity exactly zero.
    {
        GameOverBannerState banner{};
        std::size_t presentations = 0;
        while (step_game_over_banner(banner)) {
            ++presentations;
        }
        assert(presentations == 108);
        assert(banner.x == 100);
        assert(banner.velocity_fixed == 0);
    }

    // Six-Drone result selector and exact music branch order.
    {
        MissionOutcomeState mission{};
        mission.processed_count = 6;
        mission.outcomes = {
            DroneOutcome::Disarmed, DroneOutcome::Disarmed,
            DroneOutcome::Disarmed, DroneOutcome::Disarmed,
            DroneOutcome::Detonated, DroneOutcome::Detonated};
        auto summary = summarize_mission_outcomes(mission, false);
        assert(summary.disarmed == 4 && summary.detonated == 2);
        assert(summary.disarm_art_index == 4);
        assert(summary.music == MissionResultsMusic::Choral);

        mission.outcomes.fill(DroneOutcome::Detonated);
        summary = summarize_mission_outcomes(mission, false);
        assert(summary.music == MissionResultsMusic::Moon);

        mission.outcomes.fill(DroneOutcome::Disarmed);
        summary = summarize_mission_outcomes(mission, false);
        assert(summary.music == MissionResultsMusic::Choral);
        assert(summary.disarm_art_index == 6 && !summary.use_mothership_result_art);
        summary = summarize_mission_outcomes(mission, true);
        assert(summary.music == MissionResultsMusic::Hiphop);
        assert(summary.use_mothership_result_art);
    }


    // Drone objective commit, delayed settlement, mission interstitial and
    // compiled Win32 encounter-transition contracts.
    {
        MissionOutcomeState mission{};
        assert(!commit_disarmed_drone_at_boundary(mission, 200, 0));
        assert(!commit_disarmed_drone_at_boundary(
            mission, canonical_drone_disarm_commit_y, canonical_drone_destruction_activity));
        assert(commit_disarmed_drone_at_boundary(
            mission, canonical_drone_disarm_commit_y, 1));
        assert(mission.processed_count == 1);
        assert(mission.outcomes[0] == DroneOutcome::Disarmed);
        assert(canonical_drone_post_disarm_y == 202);

        auto interstitial = mission_interstitial_plan(mission);
        assert(interstitial);
        assert(interstitial->tone == MissionInterstitialTone::Good);
        assert(interstitial->sound == MissionInterstitialSound::Deepness);
        assert(interstitial->result_ordinal == 1);
        assert(interstitial->briefing == MissionBriefingCard::Mission1);

        assert(commit_detonated_drone(mission));
        interstitial = mission_interstitial_plan(mission);
        assert(interstitial);
        assert(interstitial->tone == MissionInterstitialTone::Bad);
        assert(interstitial->sound == MissionInterstitialSound::Detonate);
        assert(interstitial->result_ordinal == 1);
        assert(interstitial->briefing == MissionBriefingCard::Mission2);

        assert(advance_drone_settlement_tick(0, 0) == 0);
        assert(advance_drone_settlement_tick(0, 2) == 1);
        assert(advance_drone_settlement_tick(60, 2) == 61);
        assert(advance_drone_settlement_tick(61, 2) == 61);
        assert(!drone_resolution_transition_ready(230, 60));
        assert(drone_resolution_transition_ready(231, 60));
        assert(!drone_resolution_transition_ready(231, 61));

        assert(gameplay_session_reset_scope(false) == GameplaySessionResetScope::EncounterOnly);
        assert(gameplay_session_reset_scope(true) == GameplaySessionResetScope::FullCampaign);

        auto transition = win32_post_drone_transition_plan(1, 0);
        assert(transition);
        assert(transition->scenery == SceneryTransitionPlan::DesertStack);
        assert(transition->target == EncounterTransitionTarget::Gemini);
        assert(transition->disposition == EncounterTransitionDisposition::ContinueCampaign);
        assert(!transition->latent_registered_branch);

        transition = win32_post_drone_transition_plan(2, 0);
        assert(transition);
        assert(transition->target == EncounterTransitionTarget::Results);
        assert(transition->disposition == EncounterTransitionDisposition::EndRun);
        assert(transition->scenery == SceneryTransitionPlan::SharewareTerminationDesertBottomOnly);

        transition = win32_post_drone_transition_plan(3, 0);
        assert(transition);
        assert(transition->scenery == SceneryTransitionPlan::IsleStack);
        assert(transition->target == EncounterTransitionTarget::Spidey);
        assert(transition->latent_registered_branch);

        transition = win32_post_drone_transition_plan(4, 0);
        assert(transition);
        assert(transition->scenery == SceneryTransitionPlan::HouseStack);
        assert(transition->target == EncounterTransitionTarget::LidTop);

        transition = win32_post_drone_transition_plan(5, 0);
        assert(transition);
        assert(transition->scenery == SceneryTransitionPlan::NightStack);
        assert(transition->target == EncounterTransitionTarget::Bomber);

        transition = win32_post_drone_transition_plan(6, 0);
        assert(transition);
        assert(transition->target == EncounterTransitionTarget::Mothership);
        assert(transition->scenery == SceneryTransitionPlan::RiverStack);
        assert(transition->disposition == EncounterTransitionDisposition::EnterMothershipEndgame);
        assert(transition->latent_registered_branch);

        transition = win32_post_drone_transition_plan(6, 1);
        assert(transition);
        assert(transition->target == EncounterTransitionTarget::Results);
        assert(transition->scenery == SceneryTransitionPlan::RegisteredTerminationNightBottomOnly);
        assert(transition->disposition == EncounterTransitionDisposition::EndRun);

        MissionOutcomeState perfect{};
        perfect.processed_count = 6;
        perfect.outcomes.fill(DroneOutcome::Disarmed);
        interstitial = mission_interstitial_plan(perfect);
        assert(interstitial);
        assert(interstitial->briefing == MissionBriefingCard::Mission6Yes);
        assert(interstitial->result_ordinal == 6);

        perfect.outcomes[5] = DroneOutcome::Detonated;
        interstitial = mission_interstitial_plan(perfect);
        assert(interstitial);
        assert(interstitial->briefing == MissionBriefingCard::Mission6No);
        assert(interstitial->tone == MissionInterstitialTone::Bad);
        assert(interstitial->result_ordinal == 1);

        assert(!win32_post_drone_transition_plan(0, 0));
        assert(!win32_post_drone_transition_plan(7, 0));

        MissionOutcomeState invalid{};
        invalid.processed_count = 7;
        assert(!mission_interstitial_plan(invalid));
    }

    // Scoring: normal signed deltas affect both total and rolling life progress.
    {
        ScoreState score{};
        apply_score_delta(score, 100);
        apply_score_delta(score, 25);
        assert(score.total == 125 && score.extra_life_progress == 125);

        apply_score_delta(score, -25);
        assert(score.total == 100 && score.extra_life_progress == 100);

        apply_score_delta(score, 405);
        assert(score.total == 505 && score.extra_life_progress == 505);
        assert(consume_one_extra_life_threshold(score));
        assert(score.total == 505 && score.extra_life_progress == 5);
        assert(!consume_one_extra_life_threshold(score));
    }

    // The original converts only one threshold per state-2 update, preserving
    // enough remainder to grant another life on a later update.
    {
        ScoreState score{1200, 1000};
        assert(consume_one_extra_life_threshold(score));
        assert(score.extra_life_progress == 500);
        assert(consume_one_extra_life_threshold(score));
        assert(score.extra_life_progress == 0);
    }

    // Drone detonation has dedicated penalty semantics and clears progress.
    {
        ScoreState score{1200, 499};
        apply_drone_detonation_penalty(score);
        assert(score.total == 200 && score.extra_life_progress == 0);
        score = {999, 400};
        apply_drone_detonation_penalty(score);
        assert(score.total == 0 && score.extra_life_progress == 0);
    }

    // HUD normalization is intentionally quirky: negatives floor at zero and
    // score >=9999 loses exactly one 9999 block per HUD pass.
    {
        ScoreState score{-1, -25};
        normalize_score_for_hud(score);
        assert(score.total == 0 && score.extra_life_progress == 0);

        score = {10000, 42};
        normalize_score_for_hud(score);
        assert(score.total == 1 && score.extra_life_progress == 42);

        score = {19998, 0};
        normalize_score_for_hud(score);
        assert(score.total == 9999);
        normalize_score_for_hud(score);
        assert(score.total == 0);
    }

    // Bomb spawn gate is also the post-death quiet-period gate.
    {
        EnemyBombSpawnGate gate{};
        assert(gate.counter == -450);
        for (int i = 0; i < 455; ++i) advance_enemy_bomb_spawn_gate(gate);
        assert(gate.counter == enemy_bomb_spawn_gate_ready);
        assert(enemy_bomb_spawn_gate_allows_spawn(gate));
        reset_enemy_bomb_spawn_gate_after_spawn(gate);
        assert(gate.counter == 0);
        suppress_enemy_bomb_spawns_for_player_destruction(gate);
        assert(gate.counter == -540);
        for (int i = 0; i < 185; ++i) advance_enemy_bomb_spawn_gate(gate);
        assert(gate.counter == -355);
        assert(enemy_bomb_spawn_gate_allows_respawn(gate));
    }


    // High-score qualification mirrors the original lowest-to-highest table,
    // strict comparison, demo exclusion, and nine-lives disqualification.
    {
        drone::HighScoreTable table{};
        for (std::size_t i = 0; i < table.size(); ++i) {
            table[i].name = "P" + std::to_string(i);
            table[i].score = static_cast<std::int16_t>(i * 100);
        }

        assert(!high_score_session_eligible(true, false));
        assert(!high_score_session_eligible(false, true));
        assert(high_score_session_eligible(false, false));
        assert(!find_high_score_insertion_index(table, 9999, true, false));
        assert(!find_high_score_insertion_index(table, 9999, false, true));
        assert(!find_high_score_insertion_index(table, 0, false, false));
        assert(find_high_score_insertion_index(table, 50, false, false).value() == 0);
        // A tie with 100 remains behind the existing 100-point entry.
        assert(find_high_score_insertion_index(table, 100, false, false).value() == 0);
        assert(find_high_score_insertion_index(table, 950, false, false).value() == 9);

        drone::HighScoreEntry newcomer{};
        newcomer.name = "ACE";
        newcomer.score = 350;
        newcomer.drones_disarmed = 4;
        assert(insert_high_score(table, 3, newcomer));
        assert(table[0].name == "P1");
        assert(table[1].name == "P2");
        assert(table[2].name == "P3");
        assert(table[3].name == "ACE" && table[3].score == 350);
        assert(table[4].name == "P4");
        assert(!insert_high_score(table, table.size(), newcomer));
    }


    // Post-game/results orchestration: direct lives gate, exact six result
    // statistics, presentation suppression, high-score handoff quirks, and the
    // completed Mothership -> credits/state-1 path.
    {
        assert(!win32_enters_post_game_results(1));
        assert(win32_enters_post_game_results(0));
        assert(win32_enters_post_game_results(-1));
        static_assert(win32_results_confirm_lock_presentations == 58);
        assert(std::string(win32_high_score_bottom_slot_placeholder_name) ==
               "ENTER YOUR NAME");

        drone::HighScoreTable table{};
        for (std::size_t i = 0; i < table.size(); ++i) {
            table[i].score = static_cast<std::int16_t>(i * 100);
        }

        Win32PostGameContext context{};
        context.mission.processed_count = 2;
        context.mission.outcomes[0] = DroneOutcome::Disarmed;
        context.mission.outcomes[1] = DroneOutcome::Detonated;
        context.alien_ships_hit = 30;
        context.alien_ships_total = 40;
        context.score = 0;

        auto plan = win32_post_game_plan(context, table);
        assert(plan);
        assert(plan->statistics.alien_ships_hit == 30);
        assert(plan->statistics.alien_ships_missed == 10);
        assert(plan->statistics.alien_ships_total == 40);
        assert(plan->statistics.percentage_hit == 75);
        assert(plan->statistics.score == 0);
        assert(plan->statistics.drones_disarmed == 1);
        assert(plan->show_results_screen && plan->show_ordering_information);
        assert(!plan->high_score.invoke_high_score_table);
        assert(plan->final_state == GameState::MainMenuResetEntry);

        // The quit/abort presentation-suppression flag does not participate in
        // the later high-score eligibility gates.
        context.suppress_results_and_ordering = true;
        context.score = 50;
        plan = win32_post_game_plan(context, table);
        assert(plan);
        assert(!plan->show_results_screen && !plan->show_ordering_information);
        assert(plan->high_score.eligible);
        assert(plan->high_score.invoke_high_score_table);
        assert(plan->high_score.insertion_index == 0);
        assert(plan->high_score.interaction ==
               HighScoreInsertionInteraction::BottomSlotPlaceholderOnly);
        assert(!plan->high_score.prompt_for_name);
        assert(!plan->high_score.persist_scores_file);
        assert(plan->final_state == GameState::MainMenuReentry);

        // Any qualifying slot above zero enters the actual name editor and
        // persists the table after the edit.
        context.suppress_results_and_ordering = false;
        context.score = 250;
        plan = win32_post_game_plan(context, table);
        assert(plan && plan->high_score.insertion_index == 2);
        assert(plan->high_score.interaction ==
               HighScoreInsertionInteraction::InteractiveNameAndPersist);
        assert(plan->high_score.prompt_for_name);
        assert(plan->high_score.persist_scores_file);
        assert(plan->final_state == GameState::MainMenuReentry);

        context.demo_playback_mode = true;
        plan = win32_post_game_plan(context, table);
        assert(plan && !plan->high_score.invoke_high_score_table);
        assert(plan->final_state == GameState::MainMenuResetEntry);
        context.demo_playback_mode = false;
        context.high_score_disqualified = true;
        plan = win32_post_game_plan(context, table);
        assert(plan && !plan->high_score.invoke_high_score_table);
        context.high_score_disqualified = false;

        Win32PostGameContext perfect{};
        perfect.mission.processed_count = canonical_drone_objective_count;
        perfect.mission.outcomes.fill(DroneOutcome::Disarmed);
        perfect.mothership_destroyed = true;
        perfect.score = 950;
        perfect.alien_ships_hit = 40;
        perfect.alien_ships_total = 40;
        plan = win32_post_game_plan(perfect, table);
        assert(plan);
        assert(plan->outcome_summary.use_mothership_result_art);
        assert(plan->outcome_summary.music == MissionResultsMusic::Hiphop);
        assert(plan->show_completion_credits);
        assert(plan->high_score.invoke_high_score_table);
        assert(plan->high_score.disarmed_count_argument == 6);
        assert(plan->final_state == GameState::MainMenuResetEntry);

        Win32PostGameContext invalid{};
        invalid.alien_ships_total = 0;
        assert(!win32_post_game_plan(invalid, table));
        invalid.alien_ships_total = 1;
        invalid.alien_ships_hit = 2;
        assert(!win32_post_game_plan(invalid, table));
        invalid.alien_ships_hit = 0;
        invalid.mission.processed_count = canonical_drone_objective_count + 1;
        assert(!win32_post_game_plan(invalid, table));
    }

    // The legacy Win32 `scores` file is an intentionally padded/obfuscated
    // stream. Our compatible encoder uses deterministic filler; decoding must
    // recover the exact logical ten-entry table regardless of filler bytes.
    {
        drone::HighScoreTable table{};
        for (std::size_t i = 0; i < table.size(); ++i) {
            table[i].name = (i == 9) ? "TOP FLYER 9" : "P" + std::to_string(i);
            table[i].drones_disarmed = static_cast<std::int16_t>(i % 7);
            table[i].score = static_cast<std::int16_t>(i * 777);
            table[i].mothership_destroyed = static_cast<std::int16_t>(i == 9 ? 1 : 0);
            table[i].percentage_hit = static_cast<std::int16_t>(i * 10);
        }

        const auto encoded = drone::formats::encode_legacy_scores(table, 0x12345678u);
        assert(encoded.size() > 10000);
        const auto decoded = drone::formats::decode_legacy_scores(encoded);
        for (std::size_t i = 0; i < table.size(); ++i) {
            assert(decoded[i].name == table[i].name);
            assert(decoded[i].drones_disarmed == table[i].drones_disarmed);
            assert(decoded[i].score == table[i].score);
            assert(decoded[i].mothership_destroyed == table[i].mothership_destroyed);
            assert(decoded[i].percentage_hit == table[i].percentage_hit);
        }
    }

    // The original physical input paths converge on independent semantic
    // actions. Live keyboard/controller sources OR together, while replay
    // playback replaces exactly the six recorded controls and leaves vertical
    // movement plus meta controls live.
    {
        GameplayInputFrame keyboard{};
        keyboard.movement.left = true;
        keyboard.movement.up = true;
        keyboard.rapid_fire = true;
        keyboard.pause = true;

        GameplayInputFrame controller{};
        controller.movement.right = true;
        controller.movement.down = true;
        controller.shield = true;
        controller.special_launch = true;
        controller.quit = true;

        const auto live = merge_live_gameplay_input(keyboard, controller);
        assert(live.movement.left && live.movement.right);
        assert(live.movement.up && live.movement.down);
        assert(live.rapid_fire && live.shield && live.special_launch);
        assert(live.pause && live.quit);

        DemoGameplayFrame demo{};
        demo.horizontal_input.left = false;
        demo.horizontal_input.right = true;
        demo.launch_special = false;
        demo.load_cycle_special = true;
        demo.shield = false;
        demo.rapid_missile = false;

        const auto playback = apply_demo_playback_input(live, demo);
        assert(!playback.movement.left && playback.movement.right);
        assert(playback.special_load_cycle);
        assert(!playback.special_launch && !playback.shield && !playback.rapid_fire);

        // Not present in replay channels: these stay live, including the
        // historically surprising vertical overlay during demo playback.
        assert(playback.movement.up && playback.movement.down);
        assert(playback.pause && playback.quit);
    }

    // The DOS and Win32 executables share the same replay clock semantics:
    // reset at zero, pre-increment once per active gameplay update, and end
    // at index 0x82F (2095). The counter alone does not imply Hz; the
    // separate mode-13h/retrace evidence below establishes the DOS cadence.
    {
        DemoReplayTimeline timeline{};
        assert(timeline.index == 0);
        assert(!timeline.terminal());
        assert(timeline.advance_gameplay_update() == 1);
        assert(timeline.record_index() == 1);
        while (timeline.index < original_demo_terminal_index - 1) {
            (void)timeline.advance_gameplay_update();
        }
        assert(!timeline.terminal());
        assert(timeline.advance_gameplay_update() == original_demo_terminal_index);
        assert(timeline.terminal());
        timeline.reset();
        assert(timeline.index == 0 && !timeline.terminal());
    }

    // Canonical DOS fidelity scheduling is now evidence-backed: mode 13h
    // display timing plus one ordinary sync-tail retrace wait per logical
    // gameplay update. This does not reinterpret the Win32 QPC limiter.
    {
        constexpr double hz = drone::gameplay::canonical_dos_fidelity_tick_hz;
        constexpr double period = drone::gameplay::canonical_dos_fidelity_tick_seconds;
        static_assert(hz > 70.08 && hz < 70.09);
        static_assert(period > 0.01426 && period < 0.01428);
        const double replay_seconds = drone::gameplay::canonical_dos_duration_for_updates(
            static_cast<std::uint32_t>(original_demo_terminal_index));
        assert(replay_seconds > 29.88 && replay_seconds < 29.90);
    }

    // Trajectory-owned entities advance +0x32 by +0x36 using a 16-bit add,
    // then reset to zero when the signed result exceeds +0x38.
    {
        assert(advance_trajectory_index(0, 1, 3) == 1);
        assert(advance_trajectory_index(3, 1, 3) == 0);
        assert(advance_trajectory_index(10, -1, 20) == 9);
        assert(advance_trajectory_index(32767, 1, 32767) == -32768);
    }

    // FLY AUX is recovered sprite-frame control. Relative -1/0/+1 values
    // decrement/hold/increment with wrap; values >1 select AUX-2 directly.
    {
        assert(apply_fly_aux_frame(3, 8, 0) == 3);
        assert(apply_fly_aux_frame(3, 8, 1) == 4);
        assert(apply_fly_aux_frame(0, 8, -1) == 7);
        assert(apply_fly_aux_frame(7, 8, 1) == 0);
        assert(apply_fly_aux_frame(5, 32, 2) == 0);
        assert(apply_fly_aux_frame(5, 32, 17) == 15);
        assert(apply_fly_aux_frame(5, 32, 33) == 31);
        assert(apply_fly_aux_frame(5, 16, 33) == 0);
        assert(apply_fly_aux_frame(5, 0, 1) == 0);
    }

    // Trajectory-group header lifecycle is now recovered from 0x00415FA0.
    // Non-primary groups stagger fixed-slot activation; the primary group does
    // not use this counter path.
    {
        TrajectoryGroupLifecycle group{
            TrajectoryGroupMode::RetireOnPathWrap, 4, 1, 0, 3, 1};
        auto activation = advance_trajectory_group_stagger(group, false);
        assert(!activation.activated && group.spawn_delay_counter == 1);
        activation = advance_trajectory_group_stagger(group, false);
        assert(!activation.activated && group.spawn_delay_counter == 2);
        activation = advance_trajectory_group_stagger(group, false);
        assert(activation.activated && activation.entity_index == 1);
        assert(group.spawn_delay_counter == 0);
        assert(group.activated_entity_count == 2 && group.active_entity_count == 2);

        // Once every fixed slot has activated, equality does not reset the
        // counter: the original simply continues incrementing it.
        group.activated_entity_count = 4;
        group.active_entity_count = 4;
        group.spawn_delay_counter = 2;
        activation = advance_trajectory_group_stagger(group, false);
        assert(!activation.activated && group.spawn_delay_counter == 3);
        activation = advance_trajectory_group_stagger(group, false);
        assert(!activation.activated && group.spawn_delay_counter == 4);

        const auto before_primary = group.spawn_delay_counter;
        activation = advance_trajectory_group_stagger(group, true);
        assert(!activation.activated && group.spawn_delay_counter == before_primary);
    }

    // Mode 2 removes path-following entities on wrap, while an entity still
    // acquiring the trajectory survives. Group teardown happens exactly when
    // byte +0x02 reaches zero.
    {
        assert(trajectory_wrap_retires_entity(
            TrajectoryGroupMode::RetireOnPathWrap,
            TrajectoryEntityActivity::FollowingPath));
        assert(!trajectory_wrap_retires_entity(
            TrajectoryGroupMode::PersistentLoop,
            TrajectoryEntityActivity::FollowingPath));
        assert(!trajectory_wrap_retires_entity(
            TrajectoryGroupMode::RetireOnPathWrap,
            TrajectoryEntityActivity::AcquiringPath));

        TrajectoryGroupLifecycle group{
            TrajectoryGroupMode::RetireOnPathWrap, 3, 2, 0, 0, 3};
        assert(!retire_trajectory_group_entity(group));
        assert(group.active_entity_count == 1 &&
               group.mode == TrajectoryGroupMode::RetireOnPathWrap);
        assert(retire_trajectory_group_entity(group));
        assert(group.active_entity_count == 0 &&
               group.mode == TrajectoryGroupMode::Inactive);
    }

    // Random breakaway mode is a live-play-only transition after every slot in
    // a non-primary active formation has been activated.
    {
        const TrajectoryGroupLifecycle group{
            TrajectoryGroupMode::RetireOnPathWrap, 6, 6, 0, 11, 6};
        assert(trajectory_group_can_enter_breakaway(group, false, false, false, 2, 1, 2));
        assert(!trajectory_group_can_enter_breakaway(group, true, false, false, 2, 1, 2));
        assert(!trajectory_group_can_enter_breakaway(group, false, true, false, 2, 1, 2));
        assert(!trajectory_group_can_enter_breakaway(group, false, false, true, 2, 1, 2));
        assert(!trajectory_group_can_enter_breakaway(group, false, false, false, 1, 1, 2));
        assert(!trajectory_group_can_enter_breakaway(group, false, false, false, 2, 2, 2));

        auto partial = group;
        partial.activated_entity_count = 5;
        assert(!trajectory_group_can_enter_breakaway(partial, false, false, false, 2, 0, 2));
        auto already_breaking = group;
        already_breaking.mode = TrajectoryGroupMode::BreakawayFlyOff;
        assert(!trajectory_group_can_enter_breakaway(
            already_breaking, false, false, false, 2, 0, 2));
    }

    // Breakaway fly-off uses 16.16 coordinates, starts at speed 0x8000,
    // accelerates by 700/update to 0x28000, and retires beyond the recovered
    // [-59..320] x [-59..200] limits. Horizontal animation runs opposite the
    // travel direction on animation phase updates.
    {
        auto right = make_trajectory_breakaway_axis(100, 321);
        assert(right.fixed_position == 100 * 65536 && right.speed == 0x8000);
        const auto first_x = advance_trajectory_breakaway_axis(right);
        assert(first_x == 100);
        assert(right.speed == 0x8000 + 700);
        for (int i = 0; i < 1000; ++i) {
            (void)advance_trajectory_breakaway_axis(right);
        }
        assert(right.speed == 0x28000);

        auto left = make_trajectory_breakaway_axis(0, -60);
        const auto first_left = advance_trajectory_breakaway_axis(left);
        assert(first_left == -1); // arithmetic SAR-16 behavior

        assert(!trajectory_breakaway_is_offscreen(-59, 200));
        assert(!trajectory_breakaway_is_offscreen(320, -59));
        assert(trajectory_breakaway_is_offscreen(-60, 0));
        assert(trajectory_breakaway_is_offscreen(321, 0));
        assert(trajectory_breakaway_is_offscreen(0, 201));
        assert(trajectory_breakaway_is_offscreen(0, -60));

        assert(advance_trajectory_breakaway_frame(0, 8, 10, 321) == 7);
        assert(advance_trajectory_breakaway_frame(7, 8, 10, 321) == 6);
        assert(advance_trajectory_breakaway_frame(7, 8, 10, -60) == 0);
        assert(advance_trajectory_breakaway_frame(2, 8, 10, 10) == 2);
        assert(advance_trajectory_breakaway_frame(2, 0, 10, 321) == 0);
    }

    // The fixed Win32 trajectory pool is fully cataloged. Group 0 is the
    // seven-member Loop formation with 53-sample phase spacing; group 3 is
    // the four-member Swarm diamond. Group 15 intentionally preserves the
    // recovered setup quirk where initial inactive coordinates are sampled
    // from Generated402 even though runtime trajectory ownership is Generated422.
    {
        const auto& groups = canonical_trajectory_group_templates();
        assert(groups.size() == canonical_trajectory_group_count);
        for (std::size_t i = 0; i < groups.size(); ++i) {
            assert(groups[i].group_index == i);
            assert(groups[i].entity_count >= 1 && groups[i].entity_count <= 9);
            assert(groups[i].path_end_index >= 0);
            assert(groups[i].sprite_width > 0 && groups[i].sprite_height > 0);
            assert(groups[i].frame_count > 0);
        }

        const auto& primary = groups[0];
        assert(primary.path_family == TrajectoryPathFamily::Loop);
        assert(primary.initial_mode == TrajectoryGroupMode::PersistentLoop);
        assert(primary.initial_active_entity_count == 7);
        assert(primary.initial_activity == TrajectoryEntityActivity::FollowingPath);
        assert(primary.has_explicit_initial_path_step && primary.explicit_initial_path_step == 1);
        assert(primary.path_end_index == 375);
        for (std::size_t i = 0; i < 7; ++i) {
            assert(primary.slots[i].initial_path_index == static_cast<std::int16_t>(53 * i));
        }

        const auto& swarm = groups[3];
        assert(swarm.path_family == TrajectoryPathFamily::Swarm);
        assert(swarm.entity_count == 4 && swarm.stagger_interval == 1);
        assert(swarm.path_end_index == 945);
        assert(swarm.slots[0].x_offset == 0 && swarm.slots[0].y_offset == -25);
        assert(swarm.slots[1].x_offset == 25 && swarm.slots[1].y_offset == 0);
        assert(swarm.slots[2].x_offset == 0 && swarm.slots[2].y_offset == 25);
        assert(swarm.slots[3].x_offset == -25 && swarm.slots[3].y_offset == 0);

        assert(groups[14].path_family == TrajectoryPathFamily::Generated402);
        assert(groups[14].path_end_index == 402);
        assert(groups[15].path_family == TrajectoryPathFamily::Generated422);
        assert(groups[15].initial_sample_family == TrajectoryPathFamily::Generated402);
        assert(groups[15].path_end_index == 422);
        assert(groups[16].path_family == TrajectoryPathFamily::LeftDive);
        assert(groups[16].sprite_width == 23 && groups[16].sprite_height == 23);

        assert(canonical_trajectory_group_template(16) == &groups[16]);
        assert(canonical_trajectory_group_template(17) == nullptr);
    }

    // Win32 state 2 owns a four-phase shared substep counter. The increment
    // occurs near the start of the gameplay orchestrator, and phase 2 is the
    // established gate used by trajectory AUX animation and several other
    // slower gameplay effects.
    {
        assert(advance_win32_gameplay_substep_phase(0) == 1);
        assert(advance_win32_gameplay_substep_phase(1) == 2);
        assert(advance_win32_gameplay_substep_phase(2) == 3);
        assert(advance_win32_gameplay_substep_phase(3) == 0);
        assert(advance_win32_gameplay_substep_phase(4) == 0);
        assert(advance_win32_gameplay_substep_phase(-1) == 0);
        assert(is_win32_phase2(2));
        assert(!is_win32_phase2(1));
        assert(!is_win32_phase2(3));
    }

    // game_state_raw is now recovered as a protocol rather than an opaque
    // dispatcher integer. Main-menu selection 4 is special: joystick setup
    // executes synchronously and does not assign a replacement game state.
    {
        assert(game_state_for_main_menu_selection(MainMenuSelection::StartGame) ==
               GameState::ActiveGameplay);
        assert(game_state_for_main_menu_selection(MainMenuSelection::Instructions) ==
               GameState::Instructions);
        assert(game_state_for_main_menu_selection(MainMenuSelection::OrderingInformation) ==
               GameState::OrderingInformation);
        assert(game_state_for_main_menu_selection(MainMenuSelection::HighScores) ==
               GameState::HighScores);
        assert(!game_state_for_main_menu_selection(MainMenuSelection::ConfigureJoystick));
        assert(game_state_for_main_menu_selection(MainMenuSelection::PlayDemo) ==
               GameState::DemoLaunch);
        assert(game_state_for_main_menu_selection(MainMenuSelection::ExitDrone) ==
               GameState::ExitTransition);

        assert(game_state_has_direct_dispatch_case(GameState::ExitTransition));
        assert(game_state_has_direct_dispatch_case(GameState::PauseOverlay));
        assert(!game_state_has_direct_dispatch_case(GameState::QuitConfirmation));
        assert(!game_state_has_direct_dispatch_case(GameState::DemoLaunch));
        assert(!game_state_has_direct_dispatch_case(GameState::NineLivesNotice));

        assert(game_state_is_main_menu_entry(GameState::MainMenuResetEntry));
        assert(game_state_is_main_menu_entry(GameState::MainMenuReentry));
        assert(game_state_is_menu_modal(GameState::Instructions));
        assert(game_state_is_menu_modal(GameState::OrderingInformation));
        assert(game_state_is_menu_modal(GameState::HighScores));
        assert(game_state_is_gameplay_overlay(GameState::PauseOverlay));
        assert(game_state_is_gameplay_overlay(GameState::QuitConfirmation));
        assert(game_state_is_gameplay_overlay(GameState::NineLivesNotice));
        assert(resume_gameplay_overlay(GameState::PauseOverlay) == GameState::ActiveGameplay);
        assert(resume_gameplay_overlay(GameState::QuitConfirmation) == GameState::ActiveGameplay);
        assert(resume_gameplay_overlay(GameState::NineLivesNotice) == GameState::ActiveGameplay);
        assert(resume_gameplay_overlay(GameState::Instructions) == GameState::Instructions);

        const auto demo = consume_demo_launch(GameState::DemoLaunch);
        assert(demo && demo->next_state == GameState::ActiveGameplay);
        assert(demo->demo_playback && demo->demo_frame_index == 0);
        assert(!consume_demo_launch(GameState::ActiveGameplay));
    }

    // The primary 0x00472B00 debris records have deterministic motion once
    // their two independent CRT-random rolls are supplied. Off-screen
    // retirement does not skip the gravity/lifetime work later in the original
    // record update.
    {
        DebrisParticleState particle{10, 20, 2, -1, 0, 0, 5, true};
        assert(!advance_debris_particle(particle, 0, 0));
        assert(particle.x == 12 && particle.y == 19);
        assert(particle.velocity_y == 0);
        assert(particle.age == 1 && particle.visual_code == 4 && particle.active);

        assert(!advance_debris_particle(particle, 15, 0));
        assert(particle.visual_code == 3 && particle.active);
        assert(advance_debris_particle(particle, 15, 0));
        assert(particle.visual_code == 2 && !particle.active);

        DebrisParticleState palette_cycle{10, 10, 0, 0, 0, -1, 0x34, true};
        assert(!advance_debris_particle(palette_cycle, 15, 0));
        assert(palette_cycle.visual_code == 0x39);

        DebrisParticleState bounds{319, 100, 1, 0, 0, 100, 9, true};
        assert(advance_debris_particle(bounds, 0, 15));
        assert(!bounds.active && bounds.x == 320 && bounds.velocity_y == 1);
    }

    // The secondary debris bank uses the same state shape but always advances
    // its age and applies a rand()%10 < 3 gravity gate.
    {
        DebrisParticleState particle{30, 40, -2, 1, 4, 4, 4, true};
        assert(!advance_secondary_debris_particle(particle, 2));
        assert(particle.x == 28 && particle.y == 41);
        assert(particle.velocity_y == 2);
        assert(particle.age == 5 && particle.visual_code == 3 && particle.active);
        assert(advance_secondary_debris_particle(particle, 9));
        assert(particle.visual_code == 2 && !particle.active);
    }

    // junk1/junk2/wheel share one normal-entity updater. Frame step is a
    // family-specific signed byte at common offset +0x32; fully leaving the
    // screen retires the entity before gravity/animation work.
    {
        static_assert(canonical_debris_sprite_pool_size == 15);
        DebrisSpriteState sprite{10, 10, 1, 0, 7, 7, 0, 3, 1, true};
        assert(!advance_debris_sprite(sprite, 0));
        assert(sprite.x == 11 && sprite.y == 10 && sprite.velocity_y == 1);
        assert(sprite.current_frame == 1 && sprite.active);

        sprite.current_frame = 0;
        sprite.frame_step = -1;
        assert(!advance_debris_sprite(sprite, 127));
        assert(sprite.current_frame == 2);

        DebrisSpriteState offscreen{312, 10, 1, 0, 7, 7, 0, 3, 1, true};
        assert(advance_debris_sprite(offscreen, 0));
        assert(!offscreen.active && offscreen.x == 313 && offscreen.velocity_y == 0);
    }

    // Win32 active gameplay owns one simple cyclic scenery-row scalar. It
    // starts at 599, decrements only on shared gameplay phase 2, and wraps
    // exactly -1 back to 599. The ordering-information modal reuses the same
    // row but drives it from an independent three-step local phase counter.
    {
        assert(canonical_world_scroll_initial_row == 599);
        assert(advance_cyclic_world_scroll_row(599) == 598);
        assert(advance_cyclic_world_scroll_row(1) == 0);
        assert(advance_cyclic_world_scroll_row(0) == 599);

        assert(advance_gameplay_world_scroll_row(599, 0) == 599);
        assert(advance_gameplay_world_scroll_row(599, 1) == 599);
        assert(advance_gameplay_world_scroll_row(599, 2) == 598);
        assert(advance_gameplay_world_scroll_row(599, 3) == 599);

        assert(advance_ordering_information_scroll_phase(0) == 1);
        assert(advance_ordering_information_scroll_phase(1) == 2);
        assert(advance_ordering_information_scroll_phase(2) == 0);
        assert(advance_ordering_information_world_scroll_row(0, 2) == 599);
        assert(advance_ordering_information_world_scroll_row(123, 1) == 123);

        assert(canonical_drone_session_initial_x == 155);
        assert(canonical_drone_session_initial_y == -850);
        assert(drone_is_at_boss_approach_boundary(-200));
        assert(!drone_is_at_boss_approach_boundary(-199));
        assert(drone_reentry_y_for_processed_count(0) == -1050);
        assert(drone_reentry_y_for_processed_count(1) == -1200);
        assert(drone_reentry_y_for_processed_count(5) == -1800);
    }

    // 0x004033D0 is the exact update->presentation boundary for the ordinary
    // state-2 path: copy a 320x200 viewport out of the 320x600 scenery stack,
    // wrapping at world row 600 when the start row is above 400.
    {
        std::vector<std::uint8_t> world(drone::fidelity::scenery_world_bytes);
        std::vector<std::uint8_t> frame(drone::fidelity::logical_viewport_bytes);
        for (std::int32_t row = 0; row < drone::fidelity::scenery_world_height; ++row) {
            const auto value = static_cast<std::uint8_t>(row & 0xFF);
            const auto begin = static_cast<std::size_t>(row) * drone::fidelity::logical_width;
            for (std::int32_t x = 0; x < drone::fidelity::logical_width; ++x) {
                world[begin + static_cast<std::size_t>(x)] = value;
            }
        }

        assert(drone::fidelity::compose_scrolling_world_viewport(world, frame, 400));
        assert(frame.front() == static_cast<std::uint8_t>(400 & 0xFF));
        assert(frame.back() == static_cast<std::uint8_t>(599 & 0xFF));

        assert(drone::fidelity::compose_scrolling_world_viewport(world, frame, 401));
        assert(frame.front() == static_cast<std::uint8_t>(401 & 0xFF));
        assert(frame[198U * 320U] == static_cast<std::uint8_t>(599 & 0xFF));
        assert(frame[199U * 320U] == 0);

        assert(drone::fidelity::compose_scrolling_world_viewport(world, frame, 599));
        assert(frame.front() == static_cast<std::uint8_t>(599 & 0xFF));
        assert(frame[320] == 0 && frame[2U * 320U] == 1);
        assert(!drone::fidelity::compose_scrolling_world_viewport(world, frame, -1));
        assert(!drone::fidelity::compose_scrolling_world_viewport(world, frame, 600));

        // 0x00403560 reuses the same cyclic world only for rows 35..179 of
        // the ordering-information modal. UI pixels outside that band survive.
        std::fill(frame.begin(), frame.end(), 0xEE);
        assert(drone::fidelity::compose_ordering_information_world_background(
            world, frame, 455));
        assert(frame[34U * 320U] == 0xEE);
        assert(frame[35U * 320U] == static_cast<std::uint8_t>(455 & 0xFF));
        assert(frame[179U * 320U] == static_cast<std::uint8_t>(599 & 0xFF));
        assert(frame[180U * 320U] == 0xEE);

        std::fill(frame.begin(), frame.end(), 0xEE);
        assert(drone::fidelity::compose_ordering_information_world_background(
            world, frame, 456));
        assert(frame[35U * 320U] == static_cast<std::uint8_t>(456 & 0xFF));
        assert(frame[178U * 320U] == static_cast<std::uint8_t>(599 & 0xFF));
        assert(frame[179U * 320U] == 0);
        assert(frame[180U * 320U] == 0xEE);
    }

    // The ordinary Win32 state-2 body is now partitioned into stable semantic
    // scheduler landmarks. The first fidelity-presentation operation is world
    // viewport composition; pacing/present remain host responsibilities.
    {
        const auto& order = canonical_win32_gameplay_stage_order();
        assert(order.size() == canonical_win32_gameplay_stage_count);
        assert(order.front().stage == GameplayFrameStage::State2Entry);
        assert(order.back().stage == GameplayFrameStage::PresentFramebuffer);
        assert(canonical_win32_gameplay_stage_precedes(
            GameplayFrameStage::CollisionAndDestruction,
            GameplayFrameStage::DebrisParticleUpdate));
        assert(canonical_win32_gameplay_stage_precedes(
            GameplayFrameStage::DebrisSpriteUpdate,
            GameplayFrameStage::ComposeWorldViewport));
        assert(canonical_win32_gameplay_stage_precedes(
            GameplayFrameStage::ComposeWorldViewport,
            GameplayFrameStage::SpriteRendering));
        assert(canonical_win32_gameplay_stage_precedes(
            GameplayFrameStage::HudAndShieldRendering,
            GameplayFrameStage::HostPacing));
        assert(canonical_win32_gameplay_stage_precedes(
            GameplayFrameStage::HostPacing,
            GameplayFrameStage::PresentFramebuffer));
        assert(order[canonical_win32_gameplay_stage_index(
            GameplayFrameStage::ComposeWorldViewport)].domain ==
            GameplayFrameDomain::FidelityPresentation);
        assert(order[canonical_win32_gameplay_stage_index(
            GameplayFrameStage::HostPacing)].domain == GameplayFrameDomain::Host);
    }

    // Boss dispatch follows the six-entry original table exactly. The
    // canonical shareware campaign only reaches the first two slots before
    // its explicit two-level termination branch.
    {
        assert(boss_family_for_processed_drones(0) == BossFamily::LidTop);
        assert(boss_family_for_processed_drones(1) == BossFamily::Gemini);
        assert(boss_family_for_processed_drones(2) == BossFamily::RegisteredSlot2Unknown);
        assert(boss_family_for_processed_drones(3) == BossFamily::Spidey);
        assert(boss_family_for_processed_drones(4) == BossFamily::LidTop);
        assert(boss_family_for_processed_drones(5) == BossFamily::Bomber);
        assert(!boss_family_for_processed_drones(6));
        assert(shareware_campaign_reaches_boss_slot(0));
        assert(shareware_campaign_reaches_boss_slot(1));
        assert(!shareware_campaign_reaches_boss_slot(2));
        assert(!shareware_campaign_reaches_boss_slot(5));
    }

    return 0;
}
