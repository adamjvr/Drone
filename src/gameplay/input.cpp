#include <drone/gameplay/input.hpp>

namespace drone::gameplay {

GameplayInputFrame merge_live_gameplay_input(
    const GameplayInputFrame& first,
    const GameplayInputFrame& second) noexcept {

    GameplayInputFrame out;
    out.movement.left = first.movement.left || second.movement.left;
    out.movement.right = first.movement.right || second.movement.right;
    out.movement.up = first.movement.up || second.movement.up;
    out.movement.down = first.movement.down || second.movement.down;

    out.rapid_fire = first.rapid_fire || second.rapid_fire;
    out.shield = first.shield || second.shield;
    out.special_launch = first.special_launch || second.special_launch;
    out.special_load_cycle = first.special_load_cycle || second.special_load_cycle;

    out.pause = first.pause || second.pause;
    out.quit = first.quit || second.quit;
    out.nine_lives = first.nine_lives || second.nine_lives;
    out.toggle_sync = first.toggle_sync || second.toggle_sync;
    return out;
}

GameplayInputFrame apply_demo_playback_input(
    const GameplayInputFrame& live,
    const DemoGameplayFrame& demo) noexcept {

    GameplayInputFrame out = live;

    out.movement.left = demo.horizontal_input.left;
    out.movement.right = demo.horizontal_input.right;
    out.special_launch = demo.launch_special;
    out.special_load_cycle = demo.load_cycle_special;
    out.shield = demo.shield;
    out.rapid_fire = demo.rapid_missile;

    // movement.up/down and meta controls intentionally remain live.
    return out;
}

} // namespace drone::gameplay
