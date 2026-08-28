#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace drone::fidelity {

// Detailed semantic decomposition of the indexed-framebuffer world/effect work
// that sits inside the coarse Phase-3 presentation passes. This is evidence
// metadata for clean renderer orchestration, not a packed mirror of original
// globals or common-entity records.
enum class WorldPresentationSubpass : std::uint8_t {
    MothershipComposite,
    StingerEntity,
    LidTopComposite,
    RegisteredBossSlot2Composite,
    SpideyComposite,
    BomberComposite,
    PairedFixedActorPool,
    PointParticleBank,
    Drone,
    Flare,
    Chute,
    SpecialProjectile,
    GeminiProceduralBeam,
    GeminiBodyHeadA,
    GeminiBodyHeadB,
    MiniExplosionUnscaled,
    Explode1Unscaled,
    DebrisParticlePixels,
    SpriteDebrisTriplet,
    DroneDetonationRadialNoise,
    RetroSpriteA,
    RetroSpriteB,
    TrajectoryGroups,
    RapidMissilePool,
    EnemyBombPool,
    Player,
    PlayerDestructionExplosion,
    SecondaryImpactSpritePool,
};

enum class WorldPresentationPrimitive : std::uint8_t {
    TransparentSprite,
    TransparentSpriteComposite,
    TransparentSpritePool,
    FixedPointPixelParticles,
    ProceduralFramebufferEffect,
    DirectPixelEffect,
    TrajectoryEntityBatch,
};

struct WorldPresentationSubpassDescriptor {
    WorldPresentationSubpass subpass{};
    WorldPresentationPrimitive primitive{};
    std::uint32_t evidence_start{};
    std::uint32_t evidence_end{};

    // Address of a principal original owner/root when one is useful. Zero means
    // the pass is procedural or has no single representative root.
    std::uint32_t primary_root{};

    // Fixed maximum/count when established in the original loop. Zero means
    // singleton, composite, procedural, or runtime-counted.
    std::uint16_t fixed_element_count{};
    bool conditional{};
};

inline constexpr std::size_t canonical_win32_world_presentation_subpass_count = 28;

[[nodiscard]] const std::array<WorldPresentationSubpassDescriptor,
                               canonical_win32_world_presentation_subpass_count>&
canonical_win32_world_presentation_subpasses() noexcept;

[[nodiscard]] std::size_t canonical_win32_world_presentation_subpass_index(
    WorldPresentationSubpass subpass) noexcept;

[[nodiscard]] bool canonical_win32_world_presentation_precedes(
    WorldPresentationSubpass earlier,
    WorldPresentationSubpass later) noexcept;

[[nodiscard]] const WorldPresentationSubpassDescriptor*
canonical_win32_world_presentation_descriptor(
    WorldPresentationSubpass subpass) noexcept;

} // namespace drone::fidelity
