#include <drone/gameplay/stinger_targeting.hpp>

#include <cstdint>

namespace drone::gameplay {
namespace {

void install_target(
    StingerTargetState& state,
    const StingerTargetIdentity identity,
    const StingerTargetGeometry geometry) noexcept {
    state.identity = identity;
    state.geometry = geometry;
}

std::int64_t absolute_distance(
    const std::int32_t a,
    const std::int32_t b) noexcept {
    const auto delta = static_cast<std::int64_t>(a) - static_cast<std::int64_t>(b);
    return delta < 0 ? -delta : delta;
}

} // namespace

void reset_stinger_target(StingerTargetState& state) noexcept {
    state.identity = StingerTargetIdentity::DummyCenter;
    state.geometry = StingerTargetGeometry{canonical_stinger_dummy_target_x, 0};
}

std::int32_t stinger_target_desired_x(const StingerTargetState& state) noexcept {
    return state.geometry.x + static_cast<std::int32_t>(state.geometry.width) / 2;
}

StingerTargetSelectionResult select_stinger_target(
    StingerTargetState& state,
    const StingerTargetSelectionContext& context,
    const std::int32_t player_x) noexcept {
    const auto original_identity = state.identity;
    const auto original_geometry = state.geometry;

    if (context.mothership_panel_active) {
        install_target(
            state,
            StingerTargetIdentity::MothershipHole,
            context.mothership_hole);
    } else if (context.gemini_body_a_active || context.gemini_body_b_active) {
        if (!context.gemini_body_a_active) {
            install_target(
                state,
                StingerTargetIdentity::GeminiHeadB,
                context.gemini_head_b);
        } else if (!context.gemini_body_b_active) {
            install_target(
                state,
                StingerTargetIdentity::GeminiHeadA,
                context.gemini_head_a);
        } else {
            const auto distance_a = absolute_distance(context.gemini_head_a.x, player_x);
            const auto distance_b = absolute_distance(context.gemini_head_b.x, player_x);
            // Win32 uses `jge` after comparing A against B, so equality selects B.
            if (distance_a < distance_b) {
                install_target(
                    state,
                    StingerTargetIdentity::GeminiHeadA,
                    context.gemini_head_a);
            } else {
                install_target(
                    state,
                    StingerTargetIdentity::GeminiHeadB,
                    context.gemini_head_b);
            }
        }
    } else if (context.lid_top_top_active && context.lid_current_frame > 3) {
        install_target(
            state,
            StingerTargetIdentity::LidTopTop,
            context.lid_top_top);
    } else if (context.spidey_active) {
        install_target(state, StingerTargetIdentity::Spidey, context.spidey);
    } else if (context.registered_slot2_active) {
        install_target(
            state,
            StingerTargetIdentity::RegisteredSlot2,
            context.registered_slot2);
    } else if (context.bomber_active) {
        install_target(state, StingerTargetIdentity::Bomber, context.bomber);
    } else if (context.unknown_dynamic_hostile_active) {
        install_target(
            state,
            StingerTargetIdentity::UnknownDynamicHostile,
            context.unknown_dynamic_hostile);
    }

    const bool changed =
        state.identity != original_identity ||
        state.geometry.x != original_geometry.x ||
        state.geometry.width != original_geometry.width;

    return StingerTargetSelectionResult{
        .identity = state.identity,
        .geometry = state.geometry,
        .desired_x = stinger_target_desired_x(state),
        .target_changed = changed,
    };
}

} // namespace drone::gameplay
