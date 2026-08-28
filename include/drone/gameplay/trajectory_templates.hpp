#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <drone/gameplay/trajectory.hpp>

namespace drone::gameplay {

// Semantic trajectory sources used by the 17 fixed Win32 formation templates.
// The two Generated* families are constructed at startup rather than loaded
// from FLY assets; their names intentionally describe only the established
// inclusive terminal index.
enum class TrajectoryPathFamily : std::uint8_t {
    Loop,
    LeftDive,
    Swarm,
    Swoop,
    NewCurly,
    Frisbee1,
    Frisbee2,
    LeftDrop,
    Generated402,
    Generated422,
};

struct TrajectoryCombatProfile {
    std::uint8_t destruction_threshold{};
    std::uint8_t destruction_burst_count{};
    std::int8_t score_value{};
};

struct TrajectoryFormationSlotTemplate {
    std::int16_t initial_path_index{};
    std::int16_t x_offset{};
    std::int16_t y_offset{};
};

inline constexpr std::size_t canonical_trajectory_group_count = 17;
inline constexpr std::size_t canonical_trajectory_group_max_slots = 9;

// Clean semantic catalog of the fixed Win32 group records initialized in the
// contiguous 0x00409060..0x00409CD5 setup region. This is data, not an ABI
// mirror of the original 0x2148-byte records.
struct TrajectoryGroupTemplate {
    std::uint8_t group_index{};
    TrajectoryPathFamily path_family{};

    // Static setup normally seeds X/Y from the same family used at runtime.
    // Group 15 is an established exception: it is wired to Generated422 but
    // its inactive slots are initially sampled from Generated402.
    TrajectoryPathFamily initial_sample_family{};

    std::int8_t entity_count{};
    TrajectoryGroupMode initial_mode{TrajectoryGroupMode::Inactive};
    std::uint8_t initial_active_entity_count{};
    std::int16_t group_x_offset{};
    std::int16_t group_y_offset{};
    std::int16_t stagger_interval{};
    std::int16_t path_end_index{};
    std::int16_t sprite_width{};
    std::int16_t sprite_height{};
    std::uint8_t frame_count{};
    TrajectoryCombatProfile combat{};
    TrajectoryEntityActivity initial_activity{TrajectoryEntityActivity::Inactive};

    // sprite_entity_init does not establish common-entity +0x36. Only the
    // primary Loop formation explicitly writes a static path step of +1 here.
    bool has_explicit_initial_path_step{};
    std::int16_t explicit_initial_path_step{};

    std::array<TrajectoryFormationSlotTemplate, canonical_trajectory_group_max_slots> slots{};
};

[[nodiscard]] const std::array<TrajectoryGroupTemplate, canonical_trajectory_group_count>&
canonical_trajectory_group_templates() noexcept;

[[nodiscard]] const TrajectoryGroupTemplate* canonical_trajectory_group_template(
    std::size_t group_index) noexcept;

} // namespace drone::gameplay
