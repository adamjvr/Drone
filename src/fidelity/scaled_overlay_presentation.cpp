#include <drone/fidelity/scaled_overlay_presentation.hpp>

namespace drone::fidelity {
namespace {

constexpr std::array<ScaledOverlaySubpassDescriptor,
                     canonical_win32_scaled_overlay_subpass_count>
    scaled_subpasses{{
        {ScaledOverlaySubpass::MiniExplosionScaledPool,
         0x00410C15u, 0x00410C5Au, 0x00480318u, 110,
         "miniexp1.jba", true, true, false},
        {ScaledOverlaySubpass::ExplosionScaledPool,
         0x00410C5Cu, 0x00410CA9u, 0x00446FC8u, 165,
         "explode1.jba", true, true, false},
        {ScaledOverlaySubpass::ObjectiveDebris1,
         0x00410CABu, 0x00410D5Du, 0x00441928u, 1,
         "debris1.jba", false, false, true},
        {ScaledOverlaySubpass::ObjectiveDebris2a,
         0x00410D5Du, 0x00410DFDu, 0x004417D0u, 1,
         "debris2a.jba", false, false, true},
        {ScaledOverlaySubpass::ObjectiveDebris3,
         0x00410DFDu, 0x00410E9Du, 0x00441AC8u, 1,
         "debris3.jba", false, false, true},
    }};

constexpr std::array<ObjectiveScaledDebrisDescriptor,
                     objective_scaled_debris_sprite_count>
    objective_debris{{
        {ScaledOverlaySubpass::ObjectiveDebris1, 0x00441928u,
         "debris1.jba", 25, 18, 8, -3, 4},
        {ScaledOverlaySubpass::ObjectiveDebris2a, 0x004417D0u,
         "debris2a.jba", 27, 17, 16, -5, -1},
        {ScaledOverlaySubpass::ObjectiveDebris3, 0x00441AC8u,
         "debris3.jba", 26, 20, 16, 3, 1},
    }};

} // namespace

const std::array<ScaledOverlaySubpassDescriptor,
                 canonical_win32_scaled_overlay_subpass_count>&
canonical_win32_scaled_overlay_subpasses() noexcept {
    return scaled_subpasses;
}

bool effect_entity_uses_scaled_render_route(
    const std::uint8_t activity_state,
    const std::uint8_t family_flag_14e) noexcept {
    return activity_state == 1 && family_flag_14e == 1;
}

const std::array<ObjectiveScaledDebrisDescriptor,
                 objective_scaled_debris_sprite_count>&
objective_scaled_debris_descriptors() noexcept {
    return objective_debris;
}

void advance_objective_scaled_debris_growth(
    ScaledOverlayGeometry& geometry,
    const std::uint8_t gameplay_phase) noexcept {
    if (gameplay_phase != 2) return;
    geometry.render_width += 2;
    geometry.render_height += 2;
    --geometry.x;
    --geometry.y;
}

bool objective_scaled_debris_visible(
    const ScaledOverlayGeometry& geometry) noexcept {
    return geometry.x > -geometry.render_width &&
           geometry.y > -geometry.render_height &&
           geometry.x < 319 && geometry.y < 199;
}

ScaledOverlayDestination objective_scaled_debris_destination(
    const ScaledOverlayGeometry& geometry) noexcept {
    return {
        geometry.x,
        geometry.y,
        geometry.x + geometry.render_width,
        geometry.y + geometry.render_height,
    };
}

} // namespace drone::fidelity
