#include <drone/gameplay/trajectory_encounter.hpp>

#include <algorithm>
#include <cstdint>

namespace drone::gameplay {
namespace {

constexpr std::size_t family_index(TrajectoryPathFamily family) noexcept {
    return static_cast<std::size_t>(family);
}

const formats::FlyRecord* sample_at(
    const TrajectoryPathCatalogView& paths,
    TrajectoryPathFamily family,
    std::int16_t index) noexcept {
    if (index < 0) return nullptr;
    const auto records = paths.path(family);
    const auto i = static_cast<std::size_t>(index);
    return i < records.size() ? &records[i] : nullptr;
}

void set_actor_position_from_sample(
    TrajectoryActorState& actor,
    const TrajectoryGroupState& group,
    const formats::FlyRecord& sample) noexcept {
    actor.x = static_cast<std::int32_t>(sample.x) + group.group_x_offset + actor.formation_x_offset;
    actor.y = static_cast<std::int32_t>(sample.y) + group.group_y_offset + actor.formation_y_offset;
}

void initialize_actor_from_template(
    TrajectoryActorState& actor,
    const TrajectoryGroupTemplate& group_template,
    const TrajectoryFormationSlotTemplate& slot,
    std::size_t slot_index) noexcept {
    actor = TrajectoryActorState{};
    actor.formation_x_offset = slot.x_offset;
    actor.formation_y_offset = slot.y_offset;
    actor.path_index = slot.initial_path_index;
    actor.path_step = group_template.has_explicit_initial_path_step
        ? group_template.explicit_initial_path_step : 0;
    actor.path_end_index = group_template.path_end_index;
    actor.sprite_width = group_template.sprite_width;
    actor.sprite_height = group_template.sprite_height;
    actor.frame_count = group_template.frame_count;
    actor.activity = slot_index < group_template.initial_active_entity_count
        ? group_template.initial_activity : TrajectoryEntityActivity::Inactive;
    actor.destruction_threshold = group_template.combat.destruction_threshold;
    actor.destruction_burst_count = group_template.combat.destruction_burst_count;
    actor.score_value = group_template.combat.score_value;
}

bool retire_actor(
    TrajectoryEncounterState& encounter,
    TrajectoryGroupState& group,
    TrajectoryActorState& actor,
    const bool clear_damage = true) noexcept {
    if (actor.activity == TrajectoryEntityActivity::Inactive) return false;
    actor.activity = TrajectoryEntityActivity::Inactive;
    if (clear_damage) actor.damage_accumulator = 0;
    const bool group_retired = retire_trajectory_group_entity(group.lifecycle);
    if (group_retired && encounter.active_group_count > 0) --encounter.active_group_count;
    return group_retired;
}

} // namespace

std::span<const formats::FlyRecord> TrajectoryPathCatalogView::path(
    const TrajectoryPathFamily family) const noexcept {
    const auto index = family_index(family);
    return index < families.size() ? families[index] : std::span<const formats::FlyRecord>{};
}

void reset_trajectory_encounter(
    TrajectoryEncounterState& encounter,
    const TrajectoryPathCatalogView* paths) {
    encounter = TrajectoryEncounterState{};
    const auto& templates = canonical_trajectory_group_templates();

    for (std::size_t group_index = 0; group_index < templates.size(); ++group_index) {
        const auto& t = templates[group_index];
        auto& group = encounter.groups[group_index];
        group.group_index = t.group_index;
        group.path_family = t.path_family;
        group.group_x_offset = t.group_x_offset;
        group.group_y_offset = t.group_y_offset;
        group.lifecycle.mode = t.initial_mode;
        group.lifecycle.entity_count = t.entity_count;
        group.lifecycle.active_entity_count = t.initial_active_entity_count;
        group.lifecycle.spawn_delay_counter = 0;
        group.lifecycle.spawn_delay_interval = t.stagger_interval;
        group.lifecycle.activated_entity_count = t.initial_active_entity_count;

        for (std::size_t slot = 0; slot < group.actors.size(); ++slot) {
            initialize_actor_from_template(group.actors[slot], t, t.slots[slot], slot);
            if (paths != nullptr && slot < static_cast<std::size_t>(std::max<std::int8_t>(0, t.entity_count))) {
                if (const auto* sample = sample_at(*paths, t.initial_sample_family, group.actors[slot].path_index)) {
                    set_actor_position_from_sample(group.actors[slot], group, *sample);
                }
            }
        }
    }

    encounter.active_group_count =
        templates[0].initial_mode == TrajectoryGroupMode::Inactive ? 0 : 1;
}

std::int32_t select_inactive_trajectory_group(
    const TrajectoryEncounterState& encounter,
    std::int32_t preferred_index) noexcept {
    if (canonical_trajectory_group_count <= 1) return -1;
    if (preferred_index < 1 || preferred_index >= static_cast<std::int32_t>(canonical_trajectory_group_count)) {
        preferred_index = 1;
    }

    auto index = preferred_index;
    for (std::size_t attempts = 0; attempts < canonical_trajectory_group_count - 1; ++attempts) {
        if (encounter.groups[static_cast<std::size_t>(index)].lifecycle.mode == TrajectoryGroupMode::Inactive) {
            return index;
        }
        ++index;
        if (index >= static_cast<std::int32_t>(canonical_trajectory_group_count)) index = 1;
    }
    return -1;
}

bool activate_transient_trajectory_group(
    TrajectoryEncounterState& encounter,
    const std::size_t group_index,
    const TrajectoryPathCatalogView& paths,
    const std::int16_t group_x_offset,
    const std::int16_t group_y_offset) noexcept {
    if (group_index >= encounter.groups.size()) return false;
    auto& group = encounter.groups[group_index];
    if (group.lifecycle.mode != TrajectoryGroupMode::Inactive || group.lifecycle.entity_count <= 0) return false;

    group.lifecycle.mode = TrajectoryGroupMode::RetireOnPathWrap;
    group.lifecycle.active_entity_count = 1;
    group.lifecycle.spawn_delay_counter = 0;
    group.lifecycle.activated_entity_count = 1;
    group.group_x_offset = group_x_offset;
    group.group_y_offset = group_y_offset;

    const auto count = static_cast<std::size_t>(group.lifecycle.entity_count);
    for (std::size_t i = 0; i < count && i < group.actors.size(); ++i) {
        auto& actor = group.actors[i];
        actor.activity = TrajectoryEntityActivity::Inactive;
        actor.path_index = 0;
        actor.path_step = 1;
        actor.damage_accumulator = 0;
        actor.current_frame = 0;
    }

    auto& first = group.actors[0];
    first.activity = TrajectoryEntityActivity::FollowingPath;
    if (const auto* sample = sample_at(paths, group.path_family, 0)) {
        set_actor_position_from_sample(first, group, *sample);
    }
    ++encounter.active_group_count;
    return true;
}

TrajectoryBreakawayTransitionResult step_trajectory_breakaway_transitions(
    TrajectoryEncounterState& encounter,
    OriginalRandomState& random,
    const TrajectoryBreakawayTransitionContext& context) noexcept {
    TrajectoryBreakawayTransitionResult result{};

    // Win32 0x00415FDC..0x004160BB. Group 0 skips this branch entirely.
    // For every other live group the phase/demo/recording gates precede the
    // rand()%300 draw; the activated-count and already-mode-10 tests occur only
    // after a passing roll. This means a zero processed count still consumes one
    // draw per live non-primary group on phase 2.
    for (std::size_t group_index = 1; group_index < encounter.groups.size(); ++group_index) {
        auto& group = encounter.groups[group_index];
        if (group.lifecycle.mode == TrajectoryGroupMode::Inactive ||
            context.demo_playback_mode ||
            context.demo_recording_mode ||
            context.gameplay_phase != 2) {
            continue;
        }

        ++result.groups_checked;
        const auto roll = static_cast<std::int32_t>(original_random_mod(random, 300));
        ++result.random_draws_consumed;
        if (!trajectory_group_can_enter_breakaway(
                group.lifecycle,
                false,
                context.demo_playback_mode,
                context.demo_recording_mode,
                context.gameplay_phase,
                roll,
                context.processed_drone_count)) {
            continue;
        }

        group.lifecycle.mode = TrajectoryGroupMode::BreakawayFlyOff;
        ++result.groups_entered;

        const auto count = static_cast<std::size_t>(
            std::max<std::int8_t>(0, group.lifecycle.entity_count));
        for (std::size_t actor_index = 0; actor_index < count && actor_index < group.actors.size(); ++actor_index) {
            auto& actor = group.actors[actor_index];
            const auto x_target = original_random_mod(random, 10) < 5
                ? std::int16_t{-60}
                : std::int16_t{321};
            const auto y_target = original_random_mod(random, 100) < 25
                ? std::int16_t{-60}
                : std::int16_t{201};
            result.random_draws_consumed += 2;
            actor.breakaway_x = make_trajectory_breakaway_axis(actor.x, x_target);
            actor.breakaway_y = make_trajectory_breakaway_axis(actor.y, y_target);
            ++result.actor_axes_initialized;
        }
    }

    return result;
}

TrajectoryEncounterStepResult advance_trajectory_encounter(
    TrajectoryEncounterState& encounter,
    const TrajectoryPathCatalogView& paths,
    const std::int32_t gameplay_phase,
    ScoreState& score) noexcept {
    TrajectoryEncounterStepResult result{};

    for (std::size_t group_index = 0; group_index < encounter.groups.size(); ++group_index) {
        auto& group = encounter.groups[group_index];
        if (group.lifecycle.mode == TrajectoryGroupMode::Inactive) continue;

        const auto activation = advance_trajectory_group_stagger(group.lifecycle, group_index == 0);
        if (activation.activated && activation.entity_index >= 0 &&
            static_cast<std::size_t>(activation.entity_index) < group.actors.size()) {
            auto& actor = group.actors[static_cast<std::size_t>(activation.entity_index)];
            actor.activity = TrajectoryEntityActivity::FollowingPath;
            actor.path_index = 0;
            actor.path_step = 1;
            ++result.actors_activated;
        }

        const auto count = static_cast<std::size_t>(std::max<std::int8_t>(0, group.lifecycle.entity_count));
        for (std::size_t actor_index = 0; actor_index < count && actor_index < group.actors.size(); ++actor_index) {
            auto& actor = group.actors[actor_index];
            if (actor.activity == TrajectoryEntityActivity::Inactive) continue;

            if (group.lifecycle.mode == TrajectoryGroupMode::BreakawayFlyOff) {
                actor.x = advance_trajectory_breakaway_axis(actor.breakaway_x);
                actor.y = advance_trajectory_breakaway_axis(actor.breakaway_y);
                if (gameplay_phase == 2) {
                    actor.current_frame = advance_trajectory_breakaway_frame(
                        actor.current_frame, actor.frame_count, actor.x, actor.breakaway_x.target);
                }
                if (trajectory_breakaway_is_offscreen(actor.x, actor.y)) {
                    apply_score_delta(score, -static_cast<std::int32_t>(actor.score_value));
                    result.escape_score_delta -= actor.score_value;
                    if (retire_actor(encounter, group, actor)) ++result.groups_retired;
                    ++result.actors_escaped;
                }
                continue;
            }

            const auto previous_index = actor.path_index;
            actor.path_index = advance_trajectory_index(actor.path_index, actor.path_step, actor.path_end_index);
            const bool wrapped = actor.path_index == 0 && previous_index != 0;
            if (wrapped && trajectory_wrap_retires_entity(group.lifecycle.mode, actor.activity)) {
                apply_score_delta(score, -static_cast<std::int32_t>(actor.score_value));
                result.escape_score_delta -= actor.score_value;
                if (retire_actor(encounter, group, actor)) ++result.groups_retired;
                ++result.actors_escaped;
                continue;
            }

            const auto* sample = sample_at(paths, group.path_family, actor.path_index);
            if (sample == nullptr) continue;

            const auto target_x = static_cast<std::int32_t>(sample->x) + group.group_x_offset + actor.formation_x_offset;
            const auto target_y = static_cast<std::int32_t>(sample->y) + group.group_y_offset + actor.formation_y_offset;

            if (actor.activity == TrajectoryEntityActivity::AcquiringPath) {
                if (actor.x < target_x) actor.x += 2;
                if (actor.x > target_x) actor.x -= 2;
                if (actor.y < target_y) actor.y += 2;
                if (actor.y > target_y) actor.y -= 2;
                if (actor.x == target_x && actor.y == target_y) {
                    actor.activity = TrajectoryEntityActivity::FollowingPath;
                }
            } else {
                actor.x = target_x;
                actor.y = target_y;
            }

            if (gameplay_phase == 2) {
                actor.current_frame = apply_fly_aux_frame(
                    actor.current_frame, actor.frame_count, sample->aux);
            }
        }
    }

    return result;
}

TrajectoryHitResult apply_trajectory_hit(
    TrajectoryEncounterState& encounter,
    const TrajectoryHitEvent& hit,
    ScoreState& score) noexcept {
    TrajectoryHitResult result{};
    if (hit.group_index >= encounter.groups.size()) return result;
    auto& group = encounter.groups[hit.group_index];
    if (hit.actor_index >= group.actors.size()) return result;
    auto& actor = group.actors[hit.actor_index];
    if (group.lifecycle.mode == TrajectoryGroupMode::Inactive ||
        actor.activity == TrajectoryEntityActivity::Inactive) return result;

    result.accepted = true;
    actor.damage_accumulator = static_cast<std::uint8_t>(actor.damage_accumulator + hit.damage);
    if (actor.damage_accumulator < actor.destruction_threshold) return result;

    actor.damage_accumulator = 0;
    result.destroyed = true;
    result.destruction_burst_count = actor.destruction_burst_count;
    result.score_delta = actor.score_value;
    result.group_index = hit.group_index;
    result.actor_index = hit.actor_index;
    result.x = actor.x;
    result.y = actor.y;
    result.sprite_width = actor.sprite_width;
    result.sprite_height = actor.sprite_height;
    apply_score_delta(score, actor.score_value);
    result.group_retired = retire_actor(encounter, group, actor);
    return result;
}

TrajectoryHitResult destroy_trajectory_actor_direct(
    TrajectoryEncounterState& encounter,
    const std::uint8_t group_index,
    const std::uint8_t actor_index,
    ScoreState& score) noexcept {
    TrajectoryHitResult result{};
    if (group_index >= encounter.groups.size()) return result;
    auto& group = encounter.groups[group_index];
    if (actor_index >= group.actors.size()) return result;
    auto& actor = group.actors[actor_index];
    if (group.lifecycle.mode == TrajectoryGroupMode::Inactive ||
        actor.activity == TrajectoryEntityActivity::Inactive) return result;

    result.accepted = true;
    result.destroyed = true;
    result.destruction_burst_count = actor.destruction_burst_count;
    result.score_delta = actor.score_value;
    result.group_index = group_index;
    result.actor_index = actor_index;
    result.x = actor.x;
    result.y = actor.y;
    result.sprite_width = actor.sprite_width;
    result.sprite_height = actor.sprite_height;
    apply_score_delta(score, actor.score_value);
    result.group_retired = retire_actor(encounter, group, actor, false);
    return result;
}

} // namespace drone::gameplay
