#include <drone/gameplay/boss_encounter.hpp>

#include <drone/gameplay/gameplay_phase.hpp>

namespace drone::gameplay {
namespace {

bool begin_lid_top_destruction(LidTopBossLifecycleState& state) noexcept {
    if (state.lid_activity == boss_activity_destruction ||
        state.lid_activity == boss_activity_inactive) {
        return false;
    }

    // The producer has already validated the original activity/vulnerability
    // and collision gates. The clean owner begins the proven state-2 tail.
    state.lid_activity = boss_activity_destruction;
    state.lid_destruction_progress = 0;
    return true;
}

bool begin_gemini_side_destruction(
    GeminiBossSideLifecycleState& side,
    ScoreState& score,
    BossEncounterStepResult& result) noexcept {
    if (side.body_activity != boss_activity_active) {
        return false;
    }

    side.body_activity = boss_activity_destruction;
    side.body_destruction_progress = 0;
    apply_score_delta(score, canonical_shareware_boss_score_award);
    result.score_delta += canonical_shareware_boss_score_award;
    return true;
}

void advance_gemini_destruction_side(
    GeminiBossSideLifecycleState& side,
    const bool phase2,
    BossEncounterStepResult& result) noexcept {
    if (!phase2 || side.body_activity != boss_activity_destruction) {
        return;
    }

    ++side.body_destruction_progress;
    if (side.body_destruction_progress == gemini_body_destruction_phase2_ticks) {
        side.body_activity = boss_activity_inactive;
        ++result.components_retired;
    }
}

} // namespace

bool activate_shareware_boss_for_processed_drones(
    BossEncounterState& state,
    const std::uint8_t processed_drones) noexcept {
    if (state.family.has_value() ||
        !shareware_campaign_reaches_boss_slot(processed_drones)) {
        return false;
    }

    const auto family = boss_family_for_processed_drones(processed_drones);
    if (!family.has_value()) {
        return false;
    }

    switch (*family) {
        case BossFamily::LidTop:
            state = BossEncounterState{};
            state.family = BossFamily::LidTop;
            state.lid_top.top_activity = boss_activity_active;
            state.lid_top.lid_activity = lid_top_initial_lid_activity;
            return true;

        case BossFamily::Gemini:
            state = BossEncounterState{};
            state.family = BossFamily::Gemini;
            state.gemini.side_a.body_activity = boss_activity_active;
            state.gemini.side_a.head_activity = boss_activity_active;
            state.gemini.side_b.body_activity = boss_activity_active;
            state.gemini.side_b.head_activity = boss_activity_active;
            return true;

        case BossFamily::RegisteredSlot2Unknown:
        case BossFamily::Spidey:
        case BossFamily::Bomber:
            return false;
    }

    return false;
}

BossEncounterStepResult step_shareware_boss_encounter(
    BossEncounterState& state,
    const std::int32_t gameplay_substep_phase,
    const std::span<const SharewareBossDestructionTrigger> destruction_triggers,
    ScoreState& score) noexcept {
    BossEncounterStepResult result{};
    if (!state.family.has_value()) {
        return result;
    }

    const bool phase2 = is_win32_phase2(gameplay_substep_phase);

    if (*state.family == BossFamily::LidTop) {
        auto& lid_top = state.lid_top;

        // The top/root destruction block appears earlier in the original boss
        // update than the lid collision and 25-count lid destruction block. A
        // top destruction state created later in this call therefore starts its
        // 30 phase-2 tick retirement on a subsequent gameplay update.
        if (phase2 && lid_top.top_activity == boss_activity_destruction) {
            ++lid_top.top_destruction_progress;
            if (lid_top.top_destruction_progress == lid_top_top_destruction_phase2_ticks) {
                lid_top.top_activity = boss_activity_inactive;
                ++result.components_retired;
            }
        }

        for (const auto trigger : destruction_triggers) {
            if (trigger != SharewareBossDestructionTrigger::LidTopLid) {
                continue;
            }
            if (begin_lid_top_destruction(lid_top)) {
                ++result.destruction_transitions;
            }
        }

        // The original immediately reaches this block after a validated lid
        // collision sets activity 2, so a newly-entered destruction state is
        // counted once in the same logical update.
        if (lid_top.lid_activity == boss_activity_destruction) {
            ++lid_top.lid_destruction_progress;
            if (lid_top.lid_destruction_progress == lid_top_lid_destruction_updates) {
                apply_score_delta(score, canonical_shareware_boss_score_award);
                result.score_delta += canonical_shareware_boss_score_award;
                lid_top.lid_activity = boss_activity_inactive;
                lid_top.top_activity = boss_activity_destruction;
                lid_top.top_destruction_progress = 0;
                result.lid_top_motion_stop_requested = true;
                ++result.components_retired;
            }
        }

        return result;
    }

    if (*state.family == BossFamily::Gemini) {
        auto& gemini = state.gemini;

        // Each body's state-2 counter occurs before that same body's active
        // collision/damage branch. A newly destroyed side therefore remains at
        // counter zero until a later phase-2 update.
        advance_gemini_destruction_side(gemini.side_a, phase2, result);
        advance_gemini_destruction_side(gemini.side_b, phase2, result);

        for (const auto trigger : destruction_triggers) {
            bool accepted = false;
            switch (trigger) {
                case SharewareBossDestructionTrigger::GeminiSideA:
                    accepted = begin_gemini_side_destruction(gemini.side_a, score, result);
                    break;
                case SharewareBossDestructionTrigger::GeminiSideB:
                    accepted = begin_gemini_side_destruction(gemini.side_b, score, result);
                    break;
                case SharewareBossDestructionTrigger::LidTopLid:
                    break;
            }
            if (accepted) {
                ++result.destruction_transitions;
            }
        }
    }

    return result;
}

} // namespace drone::gameplay
