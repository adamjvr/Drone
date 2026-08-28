#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace drone::gameplay {

// Stable semantic landmarks recovered from the ordinary Win32 state-2 path.
// This is intentionally a scheduler contract, not a line-for-line translation
// of the monolithic original function. Conditional/modal paths can skip stages.
enum class GameplayFrameStage : std::uint8_t {
    State2Entry,
    AdvanceSubstepPhase,
    FormationAndObjectCreation,
    TrajectoryGroups,
    DroneDetonationUpdate,
    BossDispatchAndUpdate,
    CollisionAndDestruction,
    DebrisParticleUpdate,
    DebrisSpriteUpdate,
    ComposeWorldViewport,
    SpriteRendering,
    DebrisParticleRendering,
    DroneDetonationRendering,
    ScaledEntityRendering,
    HudAndShieldRendering,
    HostPacing,
    PresentFramebuffer,
};

enum class GameplayFrameDomain : std::uint8_t {
    Simulation,
    FidelityPresentation,
    Host,
};

struct GameplayFrameStageDescriptor {
    GameplayFrameStage stage{};
    GameplayFrameDomain domain{};
};

inline constexpr std::size_t canonical_win32_gameplay_stage_count = 17;

[[nodiscard]] const std::array<GameplayFrameStageDescriptor,
                               canonical_win32_gameplay_stage_count>&
canonical_win32_gameplay_stage_order() noexcept;

[[nodiscard]] std::size_t canonical_win32_gameplay_stage_index(
    GameplayFrameStage stage) noexcept;

[[nodiscard]] bool canonical_win32_gameplay_stage_precedes(
    GameplayFrameStage earlier,
    GameplayFrameStage later) noexcept;

} // namespace drone::gameplay
