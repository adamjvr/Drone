#pragma once

#include <drone/formats/fly.hpp>
#include <drone/gameplay/scoring.hpp>
#include <drone/gameplay/original_random.hpp>
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
    std::array<std::span<const formats::FlyRecord>, canonical_trajectory_path_family_count> families{};

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

struct TrajectoryBreakawayTransitionContext {
    bool demo_playback_mode = false;
    bool demo_recording_mode = false;
    std::int32_t gameplay_phase = 0;
    std::int32_t processed_drone_count = 0;
};

struct TrajectoryBreakawayTransitionResult {
    std::size_t groups_checked{};
    std::size_t groups_entered{};
    std::size_t actor_axes_initialized{};
    std::size_t random_draws_consumed{};
};

enum class TrajectoryHitSource : std::uint8_t {
    Unspecified = 0,
    RapidMissile = 1,
    SpecialWeapon = 2,
};

struct TrajectoryHitEvent {
    std::uint8_t group_index{};
    std::uint8_t actor_index{};
    std::uint8_t damage{3};
    TrajectoryHitSource source = TrajectoryHitSource::Unspecified;
};

struct TrajectoryHitResult {
    bool accepted{};
    bool destroyed{};
    bool group_retired{};
    std::uint8_t destruction_burst_count{};
    std::int32_t score_delta{};

    // Exact identity/geometry at the moment of destruction. Presentation needs
    // this because an actor that naturally exits a path may become inactive in
    // the same tick as a different actor is destroyed; inferring kill location
    // from inactive transitions is therefore incorrect.
    std::uint8_t group_index{};
    std::uint8_t actor_index{};
    std::int32_t x{};
    std::int32_t y{};
    std::int16_t sprite_width{};
    std::int16_t sprite_height{};
};

// Rebuild all 17 startup records from the established template catalog. Group
// 0 starts as the original persistent seven-member Loop formation; all others
// start inactive.
void reset_trajectory_encounter(
    TrajectoryEncounterState& encounter,
    const TrajectoryPathCatalogView* paths = nullptr);

// Find an inactive non-primary group, beginning at preferred_index and cycling
// through 1..16. This older helper remains useful for callers that deliberately
// exclude primary group 0; the exact live producer has its own 0-capable scan.
// Returns -1 when no slot is free.
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

// Reproduce the live per-group mode-10 gate that runs before stagger/path
// advancement. The original consumes rand()%300 for every live non-primary
// group on phase 2 even when processed_drone_count is zero or the group cannot
// yet transition; preserving those draws is important because the same CRT RNG
// drives later spawns/attacks. Successful transitions initialize every fixed
// slot's 16.16 breakaway axes and off-screen targets.
[[nodiscard]] TrajectoryBreakawayTransitionResult step_trajectory_breakaway_transitions(
    TrajectoryEncounterState& encounter,
    OriginalRandomState& random,
    const TrajectoryBreakawayTransitionContext& context) noexcept;

// Advance normal path-following/acquiring entities plus stagger activation.
// Mode-2 path completion retires actors and applies the established negative
// score event. Breakaway mode uses the already-recovered fixed-point helpers.
[[nodiscard]] TrajectoryEncounterStepResult advance_trajectory_encounter(
    TrajectoryEncounterState& encounter,
    const TrajectoryPathCatalogView& paths,
    std::int32_t gameplay_phase,
    ScoreState& score) noexcept;

// Shared threshold-damage dispatcher used by the now-native rapid-missile and
// Stinger-display trajectory collision producers. The producers own their
// distinct collision primitives; this helper owns byte damage accumulation,
// threshold destruction, score award, burst handoff and group teardown.
[[nodiscard]] TrajectoryHitResult apply_trajectory_hit(
    TrajectoryEncounterState& encounter,
    const TrajectoryHitEvent& hit,
    ScoreState& score) noexcept;

// The launched Probe/Stinger trajectory path destroys an actor immediately
// rather than accumulating damage. The original does not clear the actor's
// retained damage byte here, so primary-group replenishment may later reuse it.
[[nodiscard]] TrajectoryHitResult destroy_trajectory_actor_direct(
    TrajectoryEncounterState& encounter,
    std::uint8_t group_index,
    std::uint8_t actor_index,
    ScoreState& score) noexcept;

} // namespace drone::gameplay
