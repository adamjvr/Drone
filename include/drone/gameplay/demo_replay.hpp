#pragma once

#include <drone/formats/demo.hpp>
#include <drone/gameplay/player.hpp>

#include <cstdint>
#include <optional>
#include <cstddef>

namespace drone::gameplay {

struct TrajectoryReplayCheckpoint {
    bool spawn = false;
    std::int32_t group_slot = drone::formats::demo_no_trajectory_event;
    std::int32_t group_x_offset = 0;
    std::optional<std::int32_t> path_family;
};

struct BombReplayCheckpoint {
    bool spawn = false;
    std::int32_t x = 0;
    std::int32_t y = 0;
};

struct DroneReplayCheckpoint {
    std::int32_t x = 0;
    std::int32_t y = 0;
};

struct DemoGameplayFrame {
    PlayerDirectionalInput horizontal_input{};
    bool launch_special = false;
    bool load_cycle_special = false;
    bool shield = false;
    bool rapid_missile = false;
    TrajectoryReplayCheckpoint trajectory{};
    BombReplayCheckpoint bomb{};
    DroneReplayCheckpoint drone{};
};

// Convert one semantically decoded original replay frame into the clean game
// inputs/checkpoints we have actually established. A/Z vertical player input
// is intentionally absent: the fourteen-channel recorder does not encode it.
DemoGameplayFrame build_demo_gameplay_frame(const drone::formats::DemoFrame& frame);

// Both original executables use the same replay clock inside their active
// gameplay loop: the index starts at zero, is incremented once near the top
// of each gameplay update when playback or recording is active, and the
// session terminal condition is index >= 0x82F (2095). Consumers therefore
// address the post-increment index. The physical DAT corpus contains 2101
// records; the runtime cutoff is a separate executable behavior.
inline constexpr std::int32_t original_demo_terminal_index = 0x82f;

struct DemoReplayTimeline {
    std::int32_t index = 0;

    void reset() noexcept { index = 0; }

    [[nodiscard]] std::int32_t advance_gameplay_update() noexcept {
        return ++index;
    }

    [[nodiscard]] bool terminal() const noexcept {
        return index >= original_demo_terminal_index;
    }

    [[nodiscard]] std::size_t record_index() const noexcept {
        return static_cast<std::size_t>(index);
    }
};

} // namespace drone::gameplay
