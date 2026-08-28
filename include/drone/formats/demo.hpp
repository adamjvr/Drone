#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace drone::formats {

// On disk, each replay frame is fourteen ASCII signed integers. Keeping the
// raw record is useful for archaeology and round-trip/debug tooling even as
// individual channels acquire semantic names.
using DemoRecord = std::array<std::int32_t, 14>;

inline constexpr std::int32_t demo_no_trajectory_event = 99;

struct DemoFrame {
    DemoRecord raw{};

    // Channels 1..6: replayed gameplay controls.
    bool left = false;
    bool right = false;
    bool launch_special = false;     // Up Arrow in the Win32 build.
    bool load_cycle_special = false; // Down Arrow.
    bool shield = false;             // Space.
    bool rapid_missile = false;      // Ctrl.

    // Channels 7..9: deterministic trajectory-group spawn checkpoint.
    // A group slot >= 99 is the canonical "no event" sentinel.
    std::int32_t trajectory_group_slot = demo_no_trajectory_event;
    std::int32_t trajectory_group_x_offset = 0;
    // Values 0..3 select one of four trajectory pointer families. 99 is the
    // canonical sentinel seen when no explicit selector is consumed.
    std::int32_t trajectory_path_family = demo_no_trajectory_event;

    // Channels 10..12: deterministic bomb-spawn checkpoint.
    bool bomb_spawned = false;
    std::int32_t bomb_x = 0;
    std::int32_t bomb_y = 0;

    // Channels 13..14: Drone entity position checkpoint.
    std::int32_t drone_x = 0;
    std::int32_t drone_y = 0;

    [[nodiscard]] bool has_trajectory_group_event() const noexcept {
        return trajectory_group_slot < demo_no_trajectory_event;
    }
    [[nodiscard]] bool has_explicit_trajectory_path_family() const noexcept {
        return trajectory_path_family < demo_no_trajectory_event;
    }
};

std::vector<DemoRecord> load_demo_dat(const std::filesystem::path& path);
DemoFrame decode_demo_record(const DemoRecord& record);
std::vector<DemoFrame> load_demo_frames(const std::filesystem::path& path);

} // namespace drone::formats
