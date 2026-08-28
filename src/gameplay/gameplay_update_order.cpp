#include <drone/gameplay/gameplay_update_order.hpp>

#include <array>
#include <cstddef>
#include <limits>

namespace drone::gameplay {
namespace {

constexpr std::array<GameplayFrameStageDescriptor, canonical_win32_gameplay_stage_count>
    stage_order{{
        {GameplayFrameStage::State2Entry, GameplayFrameDomain::Simulation},
        {GameplayFrameStage::AdvanceSubstepPhase, GameplayFrameDomain::Simulation},
        {GameplayFrameStage::FormationAndObjectCreation, GameplayFrameDomain::Simulation},
        {GameplayFrameStage::TrajectoryGroups, GameplayFrameDomain::Simulation},
        {GameplayFrameStage::DroneDetonationUpdate, GameplayFrameDomain::Simulation},
        {GameplayFrameStage::BossDispatchAndUpdate, GameplayFrameDomain::Simulation},
        {GameplayFrameStage::CollisionAndDestruction, GameplayFrameDomain::Simulation},
        {GameplayFrameStage::DebrisParticleUpdate, GameplayFrameDomain::Simulation},
        {GameplayFrameStage::DebrisSpriteUpdate, GameplayFrameDomain::Simulation},
        {GameplayFrameStage::ComposeWorldViewport, GameplayFrameDomain::FidelityPresentation},
        {GameplayFrameStage::SpriteRendering, GameplayFrameDomain::FidelityPresentation},
        {GameplayFrameStage::DebrisParticleRendering, GameplayFrameDomain::FidelityPresentation},
        {GameplayFrameStage::DroneDetonationRendering, GameplayFrameDomain::FidelityPresentation},
        {GameplayFrameStage::ScaledEntityRendering, GameplayFrameDomain::FidelityPresentation},
        {GameplayFrameStage::HudAndShieldRendering, GameplayFrameDomain::FidelityPresentation},
        {GameplayFrameStage::HostPacing, GameplayFrameDomain::Host},
        {GameplayFrameStage::PresentFramebuffer, GameplayFrameDomain::Host},
    }};

} // namespace

const std::array<GameplayFrameStageDescriptor, canonical_win32_gameplay_stage_count>&
canonical_win32_gameplay_stage_order() noexcept {
    return stage_order;
}

std::size_t canonical_win32_gameplay_stage_index(GameplayFrameStage stage) noexcept {
    for (std::size_t i = 0; i < stage_order.size(); ++i) {
        if (stage_order[i].stage == stage) return i;
    }
    return std::numeric_limits<std::size_t>::max();
}

bool canonical_win32_gameplay_stage_precedes(
    GameplayFrameStage earlier,
    GameplayFrameStage later) noexcept {
    const auto earlier_index = canonical_win32_gameplay_stage_index(earlier);
    const auto later_index = canonical_win32_gameplay_stage_index(later);
    return earlier_index < later_index && later_index < stage_order.size();
}

} // namespace drone::gameplay
