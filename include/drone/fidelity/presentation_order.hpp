#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace drone::fidelity {

// Ordered presentation landmarks recovered from the ordinary Win32 state-2
// path. These are semantic renderer passes, not an ABI mirror or a claim that
// every conditional sprite inside a batch is always emitted.
enum class GameplayPresentationPass : std::uint8_t {
    ComposeWorldViewport,
    TransparentSpriteBatchBeforeDebris,
    DebrisParticlePixels,
    TransparentSpriteBatchAfterDebris,
    DroneDetonationRadialNoise,
    TransparentSpriteBatchAfterDetonation,
    ScaledTransparentOverlays,
    HudScoreAndLivesText,
    DroneOutcomeStrip,
    HudAuxiliarySprite,
    SpecialTargetOverlay,
    ShieldMeter,
    PlayerShieldOverlay,
    SpecialWeaponStatusText,
    PaletteAnimation,
    HostPacing,
    PaletteUpload,
    PresentFramebuffer,
};

enum class GameplayPresentationDomain : std::uint8_t {
    IndexedFramebuffer,
    WorkingPalette,
    Host,
};

struct GameplayPresentationPassDescriptor {
    GameplayPresentationPass pass{};
    GameplayPresentationDomain domain{};
    std::uint32_t evidence_start{};
    std::uint32_t evidence_end{};
    bool conditional{};
};

inline constexpr std::size_t canonical_win32_presentation_pass_count = 18;

[[nodiscard]] const std::array<GameplayPresentationPassDescriptor,
                               canonical_win32_presentation_pass_count>&
canonical_win32_gameplay_presentation_order() noexcept;

[[nodiscard]] std::size_t canonical_win32_gameplay_presentation_index(
    GameplayPresentationPass pass) noexcept;

[[nodiscard]] bool canonical_win32_gameplay_presentation_precedes(
    GameplayPresentationPass earlier,
    GameplayPresentationPass later) noexcept;

} // namespace drone::fidelity
