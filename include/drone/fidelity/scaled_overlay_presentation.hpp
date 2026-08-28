#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace drone::fidelity {

enum class ScaledOverlaySubpass : std::uint8_t {
    MiniExplosionScaledPool,
    ExplosionScaledPool,
    ObjectiveDebris1,
    ObjectiveDebris2a,
    ObjectiveDebris3,
};

struct ScaledOverlaySubpassDescriptor {
    ScaledOverlaySubpass subpass{};
    std::uint32_t evidence_start{};
    std::uint32_t evidence_end{};
    std::uint32_t entity_root{};
    std::size_t fixed_element_count{};
    std::string_view asset{};
    bool requires_active_state{};
    bool requires_scaled_family_flag{};
    bool requires_objective_debris_flag{};
};

inline constexpr std::size_t canonical_win32_scaled_overlay_subpass_count = 5;

[[nodiscard]] const std::array<ScaledOverlaySubpassDescriptor,
                               canonical_win32_scaled_overlay_subpass_count>&
canonical_win32_scaled_overlay_subpasses() noexcept;

[[nodiscard]] bool effect_entity_uses_scaled_render_route(
    std::uint8_t activity_state,
    std::uint8_t family_flag_14e) noexcept;

struct ObjectiveScaledDebrisDescriptor {
    ScaledOverlaySubpass subpass{};
    std::uint32_t entity_root{};
    std::string_view asset{};
    std::int16_t source_width{};
    std::int16_t source_height{};
    std::uint8_t frame_count{};
    std::int32_t initial_velocity_x{};
    std::int32_t initial_velocity_y{};
};

inline constexpr std::size_t objective_scaled_debris_sprite_count = 3;

[[nodiscard]] const std::array<ObjectiveScaledDebrisDescriptor,
                               objective_scaled_debris_sprite_count>&
objective_scaled_debris_descriptors() noexcept;

struct ScaledOverlayGeometry {
    std::int32_t x{};
    std::int32_t y{};
    std::int32_t render_width{};
    std::int32_t render_height{};

    friend constexpr bool operator==(const ScaledOverlayGeometry&,
                                     const ScaledOverlayGeometry&) = default;
};

struct ScaledOverlayDestination {
    std::int32_t left{};
    std::int32_t top{};
    std::int32_t right{};
    std::int32_t bottom{};

    friend constexpr bool operator==(const ScaledOverlayDestination&,
                                     const ScaledOverlayDestination&) = default;
};

// The objective-destruction debris grows symmetrically only when the four-phase
// gameplay scheduler is at phase 2: width/height += 2 and x/y -= 1.
void advance_objective_scaled_debris_growth(
    ScaledOverlayGeometry& geometry,
    std::uint8_t gameplay_phase) noexcept;

// Original prefilter before the clipped scaled blitter. Note the strict left
// and top visibility tests and the historical 319/199 right/bottom thresholds.
[[nodiscard]] bool objective_scaled_debris_visible(
    const ScaledOverlayGeometry& geometry) noexcept;

[[nodiscard]] ScaledOverlayDestination objective_scaled_debris_destination(
    const ScaledOverlayGeometry& geometry) noexcept;

} // namespace drone::fidelity
