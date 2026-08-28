#pragma once

#include <drone/formats/fly.hpp>
#include <drone/gameplay/scoring.hpp>
#include <drone/gameplay/trajectory.hpp>
#include <drone/gameplay/trajectory_templates.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace drone::gameplay {

// Asset data remains outside mutable session state. The encounter owns path
// indices/activity/combat state while a caller supplies immutable recovered FLY
// samples (or generated-path samples) for the current data set.
struct TrajectoryPathCatalogView {
    std::array<std::span<const formats::FlyRecord>, 10> families{};

    [[nodiscard]] std::span<const formats::FlyRecord> path(
        TrajectoryPathFamily family) const noexcept;
};

struct TrajectoryActorState {
    std::int32_t x{};
    std::int32_t y{};
    std::int16_t formation_x_offset{};
    std::int16_t formation_y_offset{};
    std::int16_t path_index{};
    std::int16_t path_step{};
    std::int16_t path_end_index{};
    std::int16_t sprite_width{};
    std::int16_t sprite_height{};
    std::uint8_t current_frame{};
    std::uint8_t frame_count{};
    TrajectoryEntityActivity activity{TrajectoryEntityActivity::Inactive};

    std::uint8_t damage_accumulator{};
    std::uint8_t destruction_threshold{};
    std::uint8_t destruction_burst_count{};
    std::int8_t score_value{};

    TrajectoryBreakawayAxis breakaway_x{};
    TrajectoryBreakawayAxis breakaway_y{};
};

struct TrajectoryGroupState {
    std::uint8_t group_index{};
    TrajectoryPathFamily path_family{};
    TrajectoryGroupLifecycle lifecycle{};
    std::int16_t group_x_offset{};
    std::int16_t group_y_offset{};
    std::array<TrajectoryActorState, canonical_trajectory_group_max_slots> actors{};
};

struct TrajectoryEncounterState {
    std::array<TrajectoryGroupState, canonical_trajectory_group_count> groups{};
    std::int32_t active_group_count{};
};

struct TrajectoryEncounterStepResult {
    std::size_t actors_activated{};
    std::size_t actors_escaped{};
    std::size_t groups_retired{};
    std::int32_t escape_score_delta{};
};

struct TrajectoryHitEvent {
    std::uint8_t group_index{};
    std::uint8_t actor_index{};
    std::uint8_t damage{3};
};

struct TrajectoryHitResult {
    bool accepted{};
    bool destroyed{};
    bool group_retired{};
    std::uint8_t destruction_burst_count{};
    std::int32_t score_delta{};
};

// Rebuild all 17 startup records from the established template catalog. Group
// 0 starts as the original persistent seven-member Loop formation; all others
// start inactive.
void reset_trajectory_encounter(
    TrajectoryEncounterState& encounter,
    const TrajectoryPathCatalogView* paths = nullptr);

// Find an inactive non-primary group, beginning at preferred_index and cycling
// through 1..16. This captures the live state-2 allocator's skip-busy behavior
// without coupling selection to CRT rand(). Returns -1 when no slot is free.
[[nodiscard]] std::int32_t select_inactive_trajectory_group(
    const TrajectoryEncounterState& encounter,
    std::int32_t preferred_index) noexcept;

// Reconstruct the common live transient-wave activation contract: mode 2,
// every fixed slot reset to inactive/path zero/step +1/damage zero, slot 0
// immediately active at sample zero, active/activated counts = 1, and the
// encounter active-group count incremented.
[[nodiscard]] bool activate_transient_trajectory_group(
    TrajectoryEncounterState& encounter,
    std::size_t group_index,
    const TrajectoryPathCatalogView& paths,
    std::int16_t group_x_offset = 0,
    std::int16_t group_y_offset = 0) noexcept;

// Advance normal path-following/acquiring entities plus stagger activation.
// Mode-2 path completion retires actors and applies the established negative
// score event. Breakaway mode uses the already-recovered fixed-point helpers.
[[nodiscard]] TrajectoryEncounterStepResult advance_trajectory_encounter(
    TrajectoryEncounterState& encounter,
    const TrajectoryPathCatalogView& paths,
    std::int32_t gameplay_phase,
    ScoreState& score) noexcept;

// Collision detection remains a separate producer because the original uses
// the actor's extracted opaque sprite mask. This dispatcher applies an already
// established hit to the continuously owned actor: byte damage accumulation,
// threshold destruction, score award, burst-count handoff and group teardown.
[[nodiscard]] TrajectoryHitResult apply_trajectory_hit(
    TrajectoryEncounterState& encounter,
    const TrajectoryHitEvent& hit,
    ScoreState& score) noexcept;

} // namespace drone::gameplay
