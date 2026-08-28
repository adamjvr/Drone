#pragma once

#include <drone/gameplay/demo_replay.hpp>
#include <drone/gameplay/player.hpp>

namespace drone::gameplay {

// Platform-independent semantic input consumed by the reconstructed gameplay
// core. The originals do not expose one canonical packed action bitfield:
// Win32 polls keyboard state directly, normalizes DirectInput joystick state
// into per-action bytes, and selects replay channels separately; DOS performs
// the same convergence through its keyboard/game-port paths. Independent
// booleans deliberately preserve simultaneous opposing directions.
struct GameplayInputFrame {
    PlayerDirectionalInput movement{};

    bool rapid_fire = false;
    bool shield = false;
    bool special_launch = false;
    bool special_load_cycle = false;

    // Live-only/meta controls. These are not encoded by the six replay-control
    // channels and therefore remain host input even during demo playback.
    bool pause = false;
    bool quit = false;
    bool nine_lives = false;
    bool toggle_sync = false;
};

// Combine two already-normalized live input sources. This models the original
// keyboard OR joystick behavior without leaking Win32 DirectInput or DOS
// game-port details into drone_core. Hosts may use it for keyboard+gamepad,
// keyboard+touch, or any equivalent pair of semantic sources.
[[nodiscard]] GameplayInputFrame merge_live_gameplay_input(
    const GameplayInputFrame& first,
    const GameplayInputFrame& second) noexcept;

// Apply one decoded original demo frame to live input using the executable's
// source-selection semantics. Playback REPLACES the six recorded controls:
// left, right, special launch, special load/cycle, shield, and rapid fire.
// Vertical movement and meta controls remain live because neither original
// replay format records them and the Win32/DOS gameplay paths consume vertical
// input outside the replay-selection blocks.
[[nodiscard]] GameplayInputFrame apply_demo_playback_input(
    const GameplayInputFrame& live,
    const DemoGameplayFrame& demo) noexcept;

} // namespace drone::gameplay
