#include <drone/gameplay/demo_replay.hpp>

namespace drone::gameplay {

DemoGameplayFrame build_demo_gameplay_frame(const drone::formats::DemoFrame& frame) {
    DemoGameplayFrame out;
    out.horizontal_input.left = frame.left;
    out.horizontal_input.right = frame.right;
    out.launch_special = frame.launch_special;
    out.load_cycle_special = frame.load_cycle_special;
    out.shield = frame.shield;
    out.rapid_missile = frame.rapid_missile;

    out.trajectory.spawn = frame.has_trajectory_group_event();
    out.trajectory.group_slot = frame.trajectory_group_slot;
    out.trajectory.group_x_offset = frame.trajectory_group_x_offset;
    if (frame.has_explicit_trajectory_path_family()) {
        out.trajectory.path_family = frame.trajectory_path_family;
    }

    out.bomb.spawn = frame.bomb_spawned;
    out.bomb.x = frame.bomb_x;
    out.bomb.y = frame.bomb_y;

    out.drone.x = frame.drone_x;
    out.drone.y = frame.drone_y;
    return out;
}

} // namespace drone::gameplay
