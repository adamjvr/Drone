#include <drone/fidelity/presentation_order.hpp>

#include <array>
#include <cstddef>
#include <limits>

namespace drone::fidelity {
namespace {

constexpr std::array<GameplayPresentationPassDescriptor,
                     canonical_win32_presentation_pass_count>
    presentation_order{{
        {GameplayPresentationPass::ComposeWorldViewport,
         GameplayPresentationDomain::IndexedFramebuffer,
         0x004100D8u, 0x004100D8u, false},
        {GameplayPresentationPass::TransparentSpriteBatchBeforeDebris,
         GameplayPresentationDomain::IndexedFramebuffer,
         0x004100F1u, 0x0041089Fu, true},
        {GameplayPresentationPass::DebrisParticlePixels,
         GameplayPresentationDomain::IndexedFramebuffer,
         0x004108BDu, 0x004108BDu, true},
        {GameplayPresentationPass::TransparentSpriteBatchAfterDebris,
         GameplayPresentationDomain::IndexedFramebuffer,
         0x004108F2u, 0x0041095Eu, true},
        {GameplayPresentationPass::DroneDetonationRadialNoise,
         GameplayPresentationDomain::IndexedFramebuffer,
         0x004109AAu, 0x004109C9u, true},
        {GameplayPresentationPass::TransparentSpriteBatchAfterDetonation,
         GameplayPresentationDomain::IndexedFramebuffer,
         0x00410A0Cu, 0x00410BF3u, true},
        {GameplayPresentationPass::ScaledTransparentOverlays,
         GameplayPresentationDomain::IndexedFramebuffer,
         0x00410C3Eu, 0x00410E95u, true},
        {GameplayPresentationPass::HudScoreAndLivesText,
         GameplayPresentationDomain::IndexedFramebuffer,
         0x00410FCEu, 0x00411012u, false},
        {GameplayPresentationPass::DroneOutcomeStrip,
         GameplayPresentationDomain::IndexedFramebuffer,
         0x00411080u, 0x00411080u, true},
        {GameplayPresentationPass::HudAuxiliarySprite,
         GameplayPresentationDomain::IndexedFramebuffer,
         0x004110D7u, 0x004110D7u, true},
        {GameplayPresentationPass::SpecialTargetOverlay,
         GameplayPresentationDomain::IndexedFramebuffer,
         0x004111A4u, 0x004111A4u, true},
        {GameplayPresentationPass::ShieldMeter,
         GameplayPresentationDomain::IndexedFramebuffer,
         0x004111ACu, 0x004111ACu, false},
        {GameplayPresentationPass::PlayerShieldOverlay,
         GameplayPresentationDomain::IndexedFramebuffer,
         0x004111DEu, 0x004111DEu, true},
        {GameplayPresentationPass::SpecialWeaponStatusText,
         GameplayPresentationDomain::IndexedFramebuffer,
         0x00411351u, 0x004113AEu, true},
        {GameplayPresentationPass::PaletteAnimation,
         GameplayPresentationDomain::WorkingPalette,
         0x00411402u, 0x00411448u, true},
        {GameplayPresentationPass::HostPacing,
         GameplayPresentationDomain::Host,
         0x00411463u, 0x00411494u, true},
        {GameplayPresentationPass::PaletteUpload,
         GameplayPresentationDomain::Host,
         0x004114F6u, 0x00411556u, false},
        {GameplayPresentationPass::PresentFramebuffer,
         GameplayPresentationDomain::Host,
         0x004115A5u, 0x004115A5u, false},
    }};

} // namespace

const std::array<GameplayPresentationPassDescriptor,
                 canonical_win32_presentation_pass_count>&
canonical_win32_gameplay_presentation_order() noexcept {
    return presentation_order;
}

std::size_t canonical_win32_gameplay_presentation_index(
    GameplayPresentationPass pass) noexcept {
    for (std::size_t i = 0; i < presentation_order.size(); ++i) {
        if (presentation_order[i].pass == pass) return i;
    }
    return std::numeric_limits<std::size_t>::max();
}

bool canonical_win32_gameplay_presentation_precedes(
    GameplayPresentationPass earlier,
    GameplayPresentationPass later) noexcept {
    const auto earlier_index = canonical_win32_gameplay_presentation_index(earlier);
    const auto later_index = canonical_win32_gameplay_presentation_index(later);
    return earlier_index < later_index && later_index < presentation_order.size();
}

} // namespace drone::fidelity
