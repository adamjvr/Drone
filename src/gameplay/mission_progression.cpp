#include <drone/gameplay/mission_progression.hpp>

#include <algorithm>

namespace drone::gameplay {

bool commit_disarmed_drone_at_boundary(
    MissionOutcomeState& state,
    const std::int32_t drone_y,
    const std::uint8_t drone_activity) noexcept {
    if (drone_y != canonical_drone_disarm_commit_y ||
        drone_activity == canonical_drone_destruction_activity ||
        state.processed_count >= state.outcomes.size()) {
        return false;
    }

    auto& current = state.outcomes[state.processed_count];
    if (current == DroneOutcome::Unresolved) {
        current = DroneOutcome::Disarmed;
    }
    ++state.processed_count;
    return true;
}

bool commit_detonated_drone(MissionOutcomeState& state) noexcept {
    if (state.processed_count >= state.outcomes.size()) {
        return false;
    }
    state.outcomes[state.processed_count] = DroneOutcome::Detonated;
    ++state.processed_count;
    return true;
}

std::int32_t advance_drone_settlement_tick(
    const std::int32_t current_tick,
    const std::int32_t gameplay_phase) noexcept {
    if (gameplay_phase == 2 && current_tick < canonical_drone_settlement_tick_cap) {
        return current_tick + 1;
    }
    return current_tick;
}

std::optional<MissionInterstitialPlan>
mission_interstitial_plan(const MissionOutcomeState& state) noexcept {
    if (state.processed_count == 0 || state.processed_count > state.outcomes.size()) {
        return std::nullopt;
    }
    const std::size_t count = state.processed_count;

    std::uint8_t disarmed = 0;
    std::uint8_t detonated = 0;
    for (std::size_t i = 0; i < count; ++i) {
        if (state.outcomes[i] == DroneOutcome::Disarmed) {
            ++disarmed;
        } else if (state.outcomes[i] == DroneOutcome::Detonated) {
            ++detonated;
        }
    }

    const auto last = state.outcomes[count - 1];
    if (last != DroneOutcome::Disarmed && last != DroneOutcome::Detonated) {
        return std::nullopt;
    }

    MissionInterstitialPlan plan{};
    plan.processed_count = static_cast<std::uint8_t>(count);
    plan.disarmed_count = disarmed;
    plan.detonated_count = detonated;

    if (last == DroneOutcome::Detonated) {
        plan.tone = MissionInterstitialTone::Bad;
        plan.sound = MissionInterstitialSound::Detonate;
        plan.result_ordinal = detonated;
    } else {
        plan.tone = MissionInterstitialTone::Good;
        plan.sound = MissionInterstitialSound::Deepness;
        plan.result_ordinal = disarmed;
    }

    switch (count) {
    case 1: plan.briefing = MissionBriefingCard::Mission1; break;
    case 2: plan.briefing = MissionBriefingCard::Mission2; break;
    case 3: plan.briefing = MissionBriefingCard::Mission3; break;
    case 4: plan.briefing = MissionBriefingCard::Mission4; break;
    case 5: plan.briefing = MissionBriefingCard::Mission5; break;
    case 6:
        plan.briefing = detonated == 0 ? MissionBriefingCard::Mission6Yes
                                      : MissionBriefingCard::Mission6No;
        break;
    default:
        return std::nullopt;
    }
    return plan;
}

std::optional<EncounterTransitionPlan>
win32_post_drone_transition_plan(
    const std::uint8_t processed_count,
    const std::uint8_t detonated_count) noexcept {
    switch (processed_count) {
    case 1:
        return EncounterTransitionPlan{
            SceneryTransitionPlan::DesertStack,
            EncounterTransitionTarget::Gemini,
            EncounterTransitionDisposition::ContinueCampaign,
            true,
            false};
    case 2:
        return EncounterTransitionPlan{
            SceneryTransitionPlan::SharewareTerminationDesertBottomOnly,
            EncounterTransitionTarget::Results,
            EncounterTransitionDisposition::EndRun,
            true,
            false};
    case 3:
        return EncounterTransitionPlan{
            SceneryTransitionPlan::IsleStack,
            EncounterTransitionTarget::Spidey,
            EncounterTransitionDisposition::ContinueCampaign,
            true,
            true};
    case 4:
        return EncounterTransitionPlan{
            SceneryTransitionPlan::HouseStack,
            EncounterTransitionTarget::LidTop,
            EncounterTransitionDisposition::ContinueCampaign,
            true,
            true};
    case 5:
        return EncounterTransitionPlan{
            SceneryTransitionPlan::NightStack,
            EncounterTransitionTarget::Bomber,
            EncounterTransitionDisposition::ContinueCampaign,
            true,
            true};
    case 6:
        if (detonated_count == 0) {
            return EncounterTransitionPlan{
                SceneryTransitionPlan::RiverStack,
                EncounterTransitionTarget::Mothership,
                EncounterTransitionDisposition::EnterMothershipEndgame,
                true,
                true};
        }
        return EncounterTransitionPlan{
            SceneryTransitionPlan::RegisteredTerminationNightBottomOnly,
            EncounterTransitionTarget::Results,
            EncounterTransitionDisposition::EndRun,
            true,
            true};
    default:
        return std::nullopt;
    }
}

} // namespace drone::gameplay
