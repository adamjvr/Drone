#pragma once

#include <cstdint>

namespace drone::gameplay {

// Recovered Win32 trajectory-owned entity semantics from update routine
// 0x00415FA0. These helpers intentionally model only the fields whose
// trajectory meaning is established; the same common entity offsets are
// reused for other purposes by some non-trajectory object families.
[[nodiscard]] std::int16_t advance_trajectory_index(
    std::int16_t index,
    std::int16_t step,
    std::int16_t end_index) noexcept;

// FLY AUX controls sprite animation while a normal trajectory entity is
// active. AUX <= 1 is a signed relative frame delta. AUX > 1 is an absolute
// frame selector encoded as (AUX - 2). The original then normalizes a frame
// at/above frame_count to zero and a negative frame to frame_count - 1.
[[nodiscard]] std::uint8_t apply_fly_aux_frame(
    std::uint8_t current_frame,
    std::uint8_t frame_count,
    std::int8_t aux) noexcept;

// Header byte +0x00 of the Win32 trajectory-group structure.
// Names describe the observed behavior rather than guessing original symbols.
enum class TrajectoryGroupMode : std::uint8_t {
    Inactive = 0,
    PersistentLoop = 1,
    RetireOnPathWrap = 2,
    BreakawayFlyOff = 10,
};

// Trajectory-family meanings recovered for common-entity byte +0x142.
enum class TrajectoryEntityActivity : std::uint8_t {
    Inactive = 0,
    FollowingPath = 1,
    AcquiringPath = 3,
};

// Clean semantic view of the trajectory-group lifecycle fields consumed by
// 0x00415FA0. It is deliberately not a packed ABI mirror.
struct TrajectoryGroupLifecycle {
    TrajectoryGroupMode mode{TrajectoryGroupMode::Inactive};
    std::int8_t entity_count{};             // original +0x01
    std::uint8_t active_entity_count{};     // original +0x02
    std::int16_t spawn_delay_counter{};     // original +0x0C
    std::int16_t spawn_delay_interval{};    // original +0x0E
    std::int16_t activated_entity_count{};  // original +0x10
};

struct TrajectoryActivationResult {
    bool activated{};
    std::int16_t entity_index{-1};
};

// Non-primary groups increment +0x0C every update. Equality with +0x0E
// activates exactly the next not-yet-activated inline entity, resets +0x0C,
// and increments both +0x02 and +0x10. Once all fixed slots have activated,
// the counter is intentionally allowed to run past the interval.
[[nodiscard]] TrajectoryActivationResult advance_trajectory_group_stagger(
    TrajectoryGroupLifecycle& group,
    bool primary_group) noexcept;

// Mode 2 removes only an entity that is already following its path when the
// just-advanced path index wraps. Activity 3 (acquiring the path) survives a
// wrap and keeps seeking the current sample.
[[nodiscard]] bool trajectory_wrap_retires_entity(
    TrajectoryGroupMode mode,
    TrajectoryEntityActivity activity) noexcept;

// Common group teardown used by path-exit, breakaway off-screen, and kill
// paths: decrement the byte-wide active count; when it reaches zero the group
// becomes inactive. Caller must invoke this only for an actually active entity.
[[nodiscard]] bool retire_trajectory_group_entity(
    TrajectoryGroupLifecycle& group) noexcept;

// Random breakaway conversion is gated by live play, update phase 2, a
// non-primary active group whose whole fixed formation has been activated,
// and the original rand()%300 < processed-Drone-count test.
[[nodiscard]] bool trajectory_group_can_enter_breakaway(
    const TrajectoryGroupLifecycle& group,
    bool primary_group,
    bool demo_playback,
    bool recording_enabled,
    std::int32_t update_phase,
    std::int32_t random_mod_300,
    std::int32_t processed_drone_count) noexcept;

struct TrajectoryBreakawayAxis {
    std::int32_t fixed_position{}; // 16.16 fixed point
    std::int32_t speed{0x8000};
    std::int16_t target{};
};

[[nodiscard]] TrajectoryBreakawayAxis make_trajectory_breakaway_axis(
    std::int32_t integer_position,
    std::int16_t target) noexcept;

// 0x00415FA0 accelerates each breakaway axis by 700 fixed-point units per
// update, caps speed at 0x28000, then moves toward the target.
[[nodiscard]] std::int32_t advance_trajectory_breakaway_axis(
    TrajectoryBreakawayAxis& axis) noexcept;

// Breakaway entities retire once their integer position crosses any of the
// original cleanup limits. Targets are chosen just beyond these limits.
[[nodiscard]] bool trajectory_breakaway_is_offscreen(
    std::int32_t x,
    std::int32_t y) noexcept;

// During update phase 2, horizontal breakaway motion animates opposite to the
// direction of travel: rightward decrements the frame, leftward increments it.
[[nodiscard]] std::uint8_t advance_trajectory_breakaway_frame(
    std::uint8_t current_frame,
    std::uint8_t frame_count,
    std::int32_t current_x,
    std::int16_t target_x) noexcept;

} // namespace drone::gameplay
