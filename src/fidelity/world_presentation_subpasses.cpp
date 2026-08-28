#include <drone/fidelity/world_presentation_subpasses.hpp>

#include <array>
#include <cstddef>
#include <limits>

namespace drone::fidelity {
namespace {

constexpr std::array<WorldPresentationSubpassDescriptor,
                     canonical_win32_world_presentation_subpass_count>
    world_subpasses{{
        {WorldPresentationSubpass::MothershipComposite,
         WorldPresentationPrimitive::TransparentSpriteComposite,
         0x004100DDu, 0x00410230u, 0x00472598u, 0, true},
        {WorldPresentationSubpass::StingerEntity,
         WorldPresentationPrimitive::TransparentSprite,
         0x00410233u, 0x0041024Cu, 0x00434C10u, 0, true},
        {WorldPresentationSubpass::LidTopComposite,
         WorldPresentationPrimitive::TransparentSpriteComposite,
         0x0041024Fu, 0x0041027Du, 0x00446E00u, 0, true},
        {WorldPresentationSubpass::RegisteredBossSlot2Composite,
         WorldPresentationPrimitive::TransparentSpriteComposite,
         0x00410280u, 0x004102DCu, 0x00441618u, 0, true},
        {WorldPresentationSubpass::SpideyComposite,
         WorldPresentationPrimitive::TransparentSpriteComposite,
         0x004102DFu, 0x0041033Bu, 0x004402D0u, 0, true},
        {WorldPresentationSubpass::BomberComposite,
         WorldPresentationPrimitive::TransparentSpriteComposite,
         0x0041033Eu, 0x004103B6u, 0x00464BE8u, 0, true},
        {WorldPresentationSubpass::PairedFixedActorPool,
         WorldPresentationPrimitive::TransparentSpritePool,
         0x004103B9u, 0x0041043Cu, 0x0047E288u, 6, true},
        {WorldPresentationSubpass::PointParticleBank,
         WorldPresentationPrimitive::FixedPointPixelParticles,
         0x0041043Eu, 0x0041049Bu, 0x00434D80u, 650, true},
        {WorldPresentationSubpass::Drone,
         WorldPresentationPrimitive::TransparentSprite,
         0x0041049Du, 0x004104B7u, 0x00446080u, 0, true},
        {WorldPresentationSubpass::Flare,
         WorldPresentationPrimitive::TransparentSprite,
         0x004104B7u, 0x004104D4u, 0x00440E00u, 0, true},
        {WorldPresentationSubpass::Chute,
         WorldPresentationPrimitive::TransparentSprite,
         0x004104D7u, 0x004104F0u, 0x0045BDA8u, 0, true},
        {WorldPresentationSubpass::SpecialProjectile,
         WorldPresentationPrimitive::TransparentSprite,
         0x004104F3u, 0x00410511u, 0x0045A148u, 0, true},
        {WorldPresentationSubpass::GeminiProceduralBeam,
         WorldPresentationPrimitive::ProceduralFramebufferEffect,
         0x00410514u, 0x004107AEu, 0x00467538u, 0, true},
        {WorldPresentationSubpass::GeminiBodyHeadA,
         WorldPresentationPrimitive::TransparentSpriteComposite,
         0x004107AEu, 0x004107DBu, 0x00467538u, 0, true},
        {WorldPresentationSubpass::GeminiBodyHeadB,
         WorldPresentationPrimitive::TransparentSpriteComposite,
         0x004107DEu, 0x0041080Cu, 0x00467690u, 0, true},
        {WorldPresentationSubpass::MiniExplosionUnscaled,
         WorldPresentationPrimitive::TransparentSpritePool,
         0x0041080Fu, 0x00410864u, 0x00480318u, 0, true},
        {WorldPresentationSubpass::Explode1Unscaled,
         WorldPresentationPrimitive::TransparentSpritePool,
         0x00410866u, 0x004108BBu, 0x00446FC8u, 0, true},
        {WorldPresentationSubpass::DebrisParticlePixels,
         WorldPresentationPrimitive::DirectPixelEffect,
         0x004108BDu, 0x004108BDu, 0, 0, true},
        {WorldPresentationSubpass::SpriteDebrisTriplet,
         WorldPresentationPrimitive::TransparentSpritePool,
         0x004108C2u, 0x00410974u, 0x0042FCA0u, 15, true},
        {WorldPresentationSubpass::DroneDetonationRadialNoise,
         WorldPresentationPrimitive::DirectPixelEffect,
         0x0041097Au, 0x004109CEu, 0, 0, true},
        {WorldPresentationSubpass::RetroSpriteA,
         WorldPresentationPrimitive::TransparentSprite,
         0x004109D1u, 0x00410A11u, 0x004673E0u, 0, true},
        {WorldPresentationSubpass::RetroSpriteB,
         WorldPresentationPrimitive::TransparentSprite,
         0x00410A14u, 0x00410A53u, 0x00438C80u, 0, true},
        {WorldPresentationSubpass::TrajectoryGroups,
         WorldPresentationPrimitive::TrajectoryEntityBatch,
         0x00410A56u, 0x00410AEBu, 0x00495CF0u, 17, true},
        {WorldPresentationSubpass::RapidMissilePool,
         WorldPresentationPrimitive::TransparentSpritePool,
         0x00410AF1u, 0x00410B3Cu, 0x0042F200u, 0, true},
        {WorldPresentationSubpass::EnemyBombPool,
         WorldPresentationPrimitive::TransparentSpritePool,
         0x00410B3Eu, 0x00410B89u, 0x004651A0u, 0, true},
        {WorldPresentationSubpass::Player,
         WorldPresentationPrimitive::TransparentSprite,
         0x00410B8Bu, 0x00410BA5u, 0x00466B18u, 0, true},
        {WorldPresentationSubpass::PlayerDestructionExplosion,
         WorldPresentationPrimitive::TransparentSprite,
         0x00410BA8u, 0x00410BC1u, 0x00491CE0u, 0, true},
        {WorldPresentationSubpass::SecondaryImpactSpritePool,
         WorldPresentationPrimitive::TransparentSpritePool,
         0x00410BC4u, 0x00410C0Bu, 0x004605A0u, 15, true},
    }};

} // namespace

const std::array<WorldPresentationSubpassDescriptor,
                 canonical_win32_world_presentation_subpass_count>&
canonical_win32_world_presentation_subpasses() noexcept {
    return world_subpasses;
}

std::size_t canonical_win32_world_presentation_subpass_index(
    WorldPresentationSubpass subpass) noexcept {
    for (std::size_t i = 0; i < world_subpasses.size(); ++i) {
        if (world_subpasses[i].subpass == subpass) return i;
    }
    return std::numeric_limits<std::size_t>::max();
}

bool canonical_win32_world_presentation_precedes(
    WorldPresentationSubpass earlier,
    WorldPresentationSubpass later) noexcept {
    const auto earlier_index = canonical_win32_world_presentation_subpass_index(earlier);
    const auto later_index = canonical_win32_world_presentation_subpass_index(later);
    return earlier_index < later_index && later_index < world_subpasses.size();
}

const WorldPresentationSubpassDescriptor*
canonical_win32_world_presentation_descriptor(
    WorldPresentationSubpass subpass) noexcept {
    const auto index = canonical_win32_world_presentation_subpass_index(subpass);
    if (index >= world_subpasses.size()) return nullptr;
    return &world_subpasses[index];
}

} // namespace drone::fidelity
