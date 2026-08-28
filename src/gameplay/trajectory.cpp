#include <drone/gameplay/trajectory.hpp>

#include <algorithm>
#include <cstdint>

namespace drone::gameplay {
namespace {

[[nodiscard]] constexpr std::int32_t fixed16_integer(std::int32_t fixed) noexcept {
    // Win32 uses SAR 16. Express arithmetic-shift/floor semantics without
    // relying on implementation-defined right shift of a negative value.
    if (fixed >= 0) {
        return fixed / 65536;
    }
    const auto magnitude = -static_cast<std::int64_t>(fixed);
    return -static_cast<std::int32_t>((magnitude + 65535) / 65536);
}

[[nodiscard]] constexpr std::int32_t integer_to_fixed16(std::int32_t value) noexcept {
    // Original uses SHL 16. Convert through uint32_t so negative inputs retain
    // defined two's-complement low-word behavior without signed-left-shift UB.
    return static_cast<std::int32_t>(static_cast<std::uint32_t>(value) << 16U);
}

} // namespace

std::int16_t advance_trajectory_index(
    std::int16_t index,
    std::int16_t step,
    std::int16_t end_index) noexcept {
    // The original uses a 16-bit ADD followed by a signed JLE. Perform the
    // addition in the unsigned domain so two's-complement wrap is defined.
    const auto advanced_bits = static_cast<std::uint16_t>(index) +
                               static_cast<std::uint16_t>(step);
    const auto advanced = static_cast<std::int16_t>(advanced_bits);
    return advanced > end_index ? std::int16_t{0} : advanced;
}

std::uint8_t apply_fly_aux_frame(
    std::uint8_t current_frame,
    std::uint8_t frame_count,
    std::int8_t aux) noexcept {
    if (frame_count == 0) {
        return 0;
    }

    std::uint8_t frame_bits{};
    if (aux > 1) {
        frame_bits = static_cast<std::uint8_t>(aux - 2);
    } else {
        // Original instruction is ADD BYTE PTR [current_frame], AL, so retain
        // byte-wide modulo-256 behavior before interpreting the result signed.
        frame_bits = static_cast<std::uint8_t>(
            static_cast<unsigned>(current_frame) +
            static_cast<unsigned>(static_cast<std::uint8_t>(aux)));
    }

    const auto signed_frame = static_cast<std::int8_t>(frame_bits);
    if (signed_frame >= static_cast<std::int16_t>(frame_count)) {
        return 0;
    }
    if (signed_frame < 0) {
        return static_cast<std::uint8_t>(frame_count - 1);
    }
    return static_cast<std::uint8_t>(signed_frame);
}

TrajectoryActivationResult advance_trajectory_group_stagger(
    TrajectoryGroupLifecycle& group,
    bool primary_group) noexcept {
    if (primary_group) {
        return {};
    }

    const auto counter_bits = static_cast<std::uint16_t>(group.spawn_delay_counter) + 1U;
    group.spawn_delay_counter = static_cast<std::int16_t>(counter_bits);
    if (group.spawn_delay_counter != group.spawn_delay_interval) {
        return {};
    }

    const auto signed_entity_count = static_cast<std::int16_t>(group.entity_count);
    if (group.activated_entity_count >= signed_entity_count) {
        return {};
    }

    const auto entity_index = group.activated_entity_count;
    group.activated_entity_count = static_cast<std::int16_t>(
        static_cast<std::uint16_t>(group.activated_entity_count) + 1U);
    group.active_entity_count = static_cast<std::uint8_t>(group.active_entity_count + 1U);
    group.spawn_delay_counter = 0;
    return {true, entity_index};
}

bool trajectory_wrap_retires_entity(
    TrajectoryGroupMode mode,
    TrajectoryEntityActivity activity) noexcept {
    return mode == TrajectoryGroupMode::RetireOnPathWrap &&
           activity == TrajectoryEntityActivity::FollowingPath;
}

bool retire_trajectory_group_entity(TrajectoryGroupLifecycle& group) noexcept {
    group.active_entity_count = static_cast<std::uint8_t>(group.active_entity_count - 1U);
    if (group.active_entity_count != 0) {
        return false;
    }
    group.mode = TrajectoryGroupMode::Inactive;
    return true;
}

bool trajectory_group_can_enter_breakaway(
    const TrajectoryGroupLifecycle& group,
    bool primary_group,
    bool demo_playback,
    bool recording_enabled,
    std::int32_t update_phase,
    std::int32_t random_mod_300,
    std::int32_t processed_drone_count) noexcept {
    if (group.mode == TrajectoryGroupMode::Inactive ||
        group.mode == TrajectoryGroupMode::BreakawayFlyOff ||
        primary_group || demo_playback || recording_enabled || update_phase != 2) {
        return false;
    }

    return group.activated_entity_count == static_cast<std::int16_t>(group.entity_count) &&
           random_mod_300 < processed_drone_count;
}

TrajectoryBreakawayAxis make_trajectory_breakaway_axis(
    std::int32_t integer_position,
    std::int16_t target) noexcept {
    return {integer_to_fixed16(integer_position), 0x8000, target};
}

std::int32_t advance_trajectory_breakaway_axis(TrajectoryBreakawayAxis& axis) noexcept {
    axis.speed = std::min<std::int32_t>(axis.speed + 700, 0x28000);

    const auto current = fixed16_integer(axis.fixed_position);
    if (current < axis.target) {
        axis.fixed_position += axis.speed;
    } else if (current > axis.target) {
        axis.fixed_position -= axis.speed;
    }
    return fixed16_integer(axis.fixed_position);
}

bool trajectory_breakaway_is_offscreen(std::int32_t x, std::int32_t y) noexcept {
    return x > 320 || y > 200 || x < -59 || y < -59;
}

std::uint8_t advance_trajectory_breakaway_frame(
    std::uint8_t current_frame,
    std::uint8_t frame_count,
    std::int32_t current_x,
    std::int16_t target_x) noexcept {
    if (frame_count == 0 || current_x == target_x) {
        return frame_count == 0 ? 0 : current_frame;
    }

    if (current_x < target_x) {
        return current_frame == 0
            ? static_cast<std::uint8_t>(frame_count - 1)
            : static_cast<std::uint8_t>(current_frame - 1U);
    }

    const auto next = static_cast<std::uint8_t>(current_frame + 1U);
    return next == frame_count ? 0 : next;
}

} // namespace drone::gameplay
