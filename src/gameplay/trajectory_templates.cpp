#include <drone/gameplay/trajectory_templates.hpp>

#include <array>
#include <cstddef>

namespace drone::gameplay {
namespace {

using Slot = TrajectoryFormationSlotTemplate;
using Slots = std::array<Slot, canonical_trajectory_group_max_slots>;

[[nodiscard]] constexpr Slots zero_slots() noexcept {
    return {};
}

[[nodiscard]] constexpr Slots loop_slots() noexcept {
    Slots slots{};
    for (std::size_t i = 0; i < 7; ++i) {
        slots[i].initial_path_index = static_cast<std::int16_t>(53 * i);
    }
    return slots;
}

[[nodiscard]] constexpr Slots swarm_diamond_slots() noexcept {
    Slots slots{};
    slots[0] = {0, 0, -25};
    slots[1] = {0, 25, 0};
    slots[2] = {0, 0, 25};
    slots[3] = {0, -25, 0};
    return slots;
}

constexpr TrajectoryGroupTemplate make_template(
    std::uint8_t index,
    TrajectoryPathFamily family,
    std::int8_t count,
    std::int16_t stagger,
    std::int16_t end_index,
    std::int16_t width,
    std::int16_t height,
    std::uint8_t frame_count,
    Slots slots = zero_slots(),
    TrajectoryPathFamily initial_sample_family = TrajectoryPathFamily::Loop) noexcept {
    // initial_sample_family == Loop is used as a compact default sentinel only
    // when the runtime family itself is not Loop; correct it below.
    const auto sample_family =
        (initial_sample_family == TrajectoryPathFamily::Loop && family != TrajectoryPathFamily::Loop)
            ? family
            : initial_sample_family;

    return {
        index,
        family,
        sample_family,
        count,
        TrajectoryGroupMode::Inactive,
        0,
        0,
        0,
        stagger,
        end_index,
        width,
        height,
        frame_count,
        TrajectoryEntityActivity::Inactive,
        false,
        0,
        slots,
    };
}

constexpr auto kTemplates = [] {
    std::array<TrajectoryGroupTemplate, canonical_trajectory_group_count> groups{};

    groups[0] = {
        0,
        TrajectoryPathFamily::Loop,
        TrajectoryPathFamily::Loop,
        7,
        TrajectoryGroupMode::PersistentLoop,
        7,
        0,
        0,
        0,
        375,
        28,
        21,
        15,
        TrajectoryEntityActivity::FollowingPath,
        true,
        1,
        loop_slots(),
    };
    groups[1] = make_template(1, TrajectoryPathFamily::LeftDive, 6, 18, 117, 26, 26, 16);
    groups[2] = make_template(2, TrajectoryPathFamily::LeftDive, 7, 13, 117, 28, 21, 15);
    groups[3] = make_template(
        3, TrajectoryPathFamily::Swarm, 4, 1, 945, 35, 22, 16, swarm_diamond_slots());
    groups[4] = make_template(4, TrajectoryPathFamily::Swarm, 1, 1, 945, 44, 46, 16);
    groups[5] = make_template(5, TrajectoryPathFamily::Swoop, 7, 13, 185, 28, 21, 15);
    groups[6] = make_template(6, TrajectoryPathFamily::NewCurly, 9, 10, 230, 35, 30, 32);
    groups[7] = make_template(7, TrajectoryPathFamily::Frisbee1, 9, 9, 935, 14, 12, 32);
    groups[8] = make_template(8, TrajectoryPathFamily::Frisbee2, 9, 5, 425, 14, 12, 32);
    groups[9] = make_template(9, TrajectoryPathFamily::LeftDrop, 5, 16, 195, 44, 46, 16);
    groups[10] = make_template(10, TrajectoryPathFamily::LeftDrop, 6, 12, 195, 35, 22, 16);
    groups[11] = make_template(11, TrajectoryPathFamily::LeftDrop, 6, 12, 195, 26, 26, 16);
    groups[12] = make_template(12, TrajectoryPathFamily::LeftDive, 4, 18, 117, 31, 41, 32);
    groups[13] = make_template(13, TrajectoryPathFamily::LeftDrop, 5, 16, 195, 31, 41, 32);
    groups[14] = make_template(14, TrajectoryPathFamily::Generated402, 6, 11, 402, 16, 11, 16);
    groups[15] = make_template(
        15,
        TrajectoryPathFamily::Generated422,
        6,
        11,
        422,
        16,
        11,
        16,
        zero_slots(),
        TrajectoryPathFamily::Generated402);
    groups[16] = make_template(16, TrajectoryPathFamily::LeftDive, 6, 11, 117, 23, 23, 32);

    return groups;
}();

static_assert(kTemplates.size() == 17);
static_assert(kTemplates[0].slots[6].initial_path_index == 318);
static_assert(kTemplates[3].slots[0].y_offset == -25);
static_assert(kTemplates[3].slots[3].x_offset == -25);
static_assert(kTemplates[15].path_family == TrajectoryPathFamily::Generated422);
static_assert(kTemplates[15].initial_sample_family == TrajectoryPathFamily::Generated402);

} // namespace

const std::array<TrajectoryGroupTemplate, canonical_trajectory_group_count>&
canonical_trajectory_group_templates() noexcept {
    return kTemplates;
}

const TrajectoryGroupTemplate* canonical_trajectory_group_template(std::size_t group_index) noexcept {
    if (group_index >= kTemplates.size()) {
        return nullptr;
    }
    return &kTemplates[group_index];
}

} // namespace drone::gameplay
