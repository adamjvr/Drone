#include <drone/gameplay/boss_encounter.hpp>

#include <array>
#include <cassert>
#include <iostream>

int main() {
    using namespace drone::gameplay;

    // Only the two boss slots reachable by canonical shareware progression are
    // initialized. Registered-only slots stay intentionally out of Phase 4.
    {
        BossEncounterState state{};
        assert(activate_shareware_boss_for_processed_drones(state, 0));
        assert(state.family == BossFamily::LidTop);
        assert(state.lid_top.top_activity == boss_activity_active);
        assert(state.lid_top.lid_activity == lid_top_initial_lid_activity);
        assert(!activate_shareware_boss_for_processed_drones(state, 0));

        BossEncounterState gemini{};
        assert(activate_shareware_boss_for_processed_drones(gemini, 1));
        assert(gemini.family == BossFamily::Gemini);
        assert(gemini.gemini.side_a.body_activity == boss_activity_active);
        assert(gemini.gemini.side_a.head_activity == boss_activity_active);
        assert(gemini.gemini.side_b.body_activity == boss_activity_active);
        assert(gemini.gemini.side_b.head_activity == boss_activity_active);

        BossEncounterState registered_only{};
        assert(!activate_shareware_boss_for_processed_drones(registered_only, 2));
        assert(!registered_only.family.has_value());
    }

    // Lid/Top: an already-validated destruction transition enters lid activity
    // 2 and the same update counts as destruction progress 1. The exact +100
    // award lands at count 25, swaps lid off/top into destruction, and asks the
    // external movement producer to zero top motion. The top then retires only
    // after 30 phase-2 ticks, beginning on a later update.
    {
        BossEncounterState state{};
        ScoreState score{};
        assert(activate_shareware_boss_for_processed_drones(state, 0));

        const std::array trigger{SharewareBossDestructionTrigger::LidTopLid};
        auto step = step_shareware_boss_encounter(state, 1, trigger, score);
        assert(step.destruction_transitions == 1);
        assert(step.score_delta == 0);
        assert(state.lid_top.lid_activity == boss_activity_destruction);
        assert(state.lid_top.lid_destruction_progress == 1);
        assert(score.total == 0 && score.extra_life_progress == 0);

        // A repeated producer event cannot reset/duplicate an active tail.
        step = step_shareware_boss_encounter(state, 2, trigger, score);
        assert(step.destruction_transitions == 0);
        assert(state.lid_top.lid_destruction_progress == 2);
        assert(state.lid_top.top_destruction_progress == 0);

        for (int i = 0; i < 22; ++i) {
            step = step_shareware_boss_encounter(state, 0, {}, score);
            assert(step.score_delta == 0);
        }
        assert(state.lid_top.lid_destruction_progress == 24);

        step = step_shareware_boss_encounter(state, 0, {}, score);
        assert(step.score_delta == canonical_shareware_boss_score_award);
        assert(step.components_retired == 1);
        assert(step.lid_top_motion_stop_requested);
        assert(score.total == 100 && score.extra_life_progress == 100);
        assert(state.lid_top.lid_activity == boss_activity_inactive);
        assert(state.lid_top.top_activity == boss_activity_destruction);
        assert(state.lid_top.top_destruction_progress == 0);

        for (int i = 0; i < 29; ++i) {
            step = step_shareware_boss_encounter(state, 2, {}, score);
            assert(step.components_retired == 0);
        }
        assert(state.lid_top.top_destruction_progress == 29);
        step = step_shareware_boss_encounter(state, 2, {}, score);
        assert(step.components_retired == 1);
        assert(state.lid_top.top_activity == boss_activity_inactive);
    }

    // Gemini: the two halves have independent threshold-crossing destruction
    // branches. Each accepted transition awards +100 immediately and begins a
    // body-only 20 phase-2-tick retirement; never collapse this to one generic
    // +100 encounter award.
    {
        BossEncounterState state{};
        ScoreState score{};
        assert(activate_shareware_boss_for_processed_drones(state, 1));

        const std::array side_a{SharewareBossDestructionTrigger::GeminiSideA};
        auto step = step_shareware_boss_encounter(state, 2, side_a, score);
        assert(step.destruction_transitions == 1);
        assert(step.score_delta == 100);
        assert(score.total == 100 && score.extra_life_progress == 100);
        assert(state.gemini.side_a.body_activity == boss_activity_destruction);
        assert(state.gemini.side_a.body_destruction_progress == 0);
        assert(state.gemini.side_b.body_activity == boss_activity_active);

        // Non-phase-2 updates do not advance Gemini destruction counters.
        for (const int phase : {3, 0, 1}) {
            step = step_shareware_boss_encounter(state, phase, {}, score);
            assert(state.gemini.side_a.body_destruction_progress == 0);
        }

        for (int i = 0; i < 19; ++i) {
            step = step_shareware_boss_encounter(state, 2, {}, score);
            assert(step.components_retired == 0);
        }
        assert(state.gemini.side_a.body_destruction_progress == 19);
        step = step_shareware_boss_encounter(state, 2, {}, score);
        assert(step.components_retired == 1);
        assert(state.gemini.side_a.body_activity == boss_activity_inactive);

        const std::array side_b{SharewareBossDestructionTrigger::GeminiSideB};
        step = step_shareware_boss_encounter(state, 1, side_b, score);
        assert(step.destruction_transitions == 1);
        assert(step.score_delta == 100);
        assert(score.total == 200 && score.extra_life_progress == 200);
        assert(state.gemini.side_b.body_activity == boss_activity_destruction);
        assert(state.gemini.side_b.body_destruction_progress == 0);
    }

    std::cout << "Drone shareware boss encounter lifecycle tests passed\n";
    return 0;
}
