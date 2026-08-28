#pragma once

#include <cstdint>
#include <optional>

namespace drone::gameplay {

// Evidence-backed meanings for the user-facing values stored in Win32
// game_state_raw (0x0042B188). This is a clean semantic type, not an ABI
// mirror: the original also uses subordinate modal control flow around the
// same scalar, so callers should not assume every value is a top-level
// dispatcher case.
enum class GameState : std::int32_t {
    ExitTransition = 0,
    MainMenuResetEntry = 1,
    ActiveGameplay = 2,
    Instructions = 3,
    MainMenuReentry = 4,
    PauseOverlay = 5,
    QuitConfirmation = 6,
    OrderingInformation = 7,
    HighScores = 8,
    DemoLaunch = 13,
    NineLivesNotice = 99,
};

// Exact visible main-menu selection order recovered from 0x00418AC0.
enum class MainMenuSelection : std::uint8_t {
    StartGame = 0,
    Instructions = 1,
    OrderingInformation = 2,
    HighScores = 3,
    ConfigureJoystick = 4,
    PlayDemo = 5,
    ExitDrone = 6,
};

// Configure Joystick is handled synchronously by the menu and does not assign
// a new game_state_raw value, hence nullopt for that selection.
[[nodiscard]] std::optional<GameState> game_state_for_main_menu_selection(
    MainMenuSelection selection) noexcept;

// The top-level dispatcher at 0x0040BA50 has a six-entry jump table for raw
// values 0..5. Values 6, 7, 8, 13 and 99 are consumed by subordinate/modal
// paths rather than being ordinary direct dispatcher cases.
[[nodiscard]] bool game_state_has_direct_dispatch_case(GameState state) noexcept;

[[nodiscard]] bool game_state_is_main_menu_entry(GameState state) noexcept;
[[nodiscard]] bool game_state_is_menu_modal(GameState state) noexcept;
[[nodiscard]] bool game_state_is_gameplay_overlay(GameState state) noexcept;

// Pause, quit-confirmation and the nine-lives notice all use R to resume the
// active gameplay state. For any other input state this helper leaves it
// unchanged so it is safe to use at a clean UI boundary.
[[nodiscard]] GameState resume_gameplay_overlay(GameState state) noexcept;

struct DemoLaunchTransition {
    GameState next_state{GameState::ActiveGameplay};
    bool demo_playback{true};
    std::int32_t demo_frame_index{0};
};

// Both menu-entry dispatcher paths consume raw state 13 by enabling demo
// playback, resetting the replay clock to zero, and entering state 2.
[[nodiscard]] std::optional<DemoLaunchTransition> consume_demo_launch(
    GameState state) noexcept;

} // namespace drone::gameplay
