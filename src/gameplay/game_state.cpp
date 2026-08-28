#include <drone/gameplay/game_state.hpp>

namespace drone::gameplay {

std::optional<GameState> game_state_for_main_menu_selection(
    MainMenuSelection selection) noexcept {
    switch (selection) {
    case MainMenuSelection::StartGame:
        return GameState::ActiveGameplay;
    case MainMenuSelection::Instructions:
        return GameState::Instructions;
    case MainMenuSelection::OrderingInformation:
        return GameState::OrderingInformation;
    case MainMenuSelection::HighScores:
        return GameState::HighScores;
    case MainMenuSelection::ConfigureJoystick:
        return std::nullopt;
    case MainMenuSelection::PlayDemo:
        return GameState::DemoLaunch;
    case MainMenuSelection::ExitDrone:
        return GameState::ExitTransition;
    }
    return std::nullopt;
}

bool game_state_has_direct_dispatch_case(GameState state) noexcept {
    const auto raw = static_cast<std::int32_t>(state);
    return raw >= 0 && raw <= 5;
}

bool game_state_is_main_menu_entry(GameState state) noexcept {
    return state == GameState::MainMenuResetEntry ||
           state == GameState::MainMenuReentry;
}

bool game_state_is_menu_modal(GameState state) noexcept {
    return state == GameState::Instructions ||
           state == GameState::OrderingInformation ||
           state == GameState::HighScores;
}

bool game_state_is_gameplay_overlay(GameState state) noexcept {
    return state == GameState::PauseOverlay ||
           state == GameState::QuitConfirmation ||
           state == GameState::NineLivesNotice;
}

GameState resume_gameplay_overlay(GameState state) noexcept {
    return game_state_is_gameplay_overlay(state) ? GameState::ActiveGameplay : state;
}

std::optional<DemoLaunchTransition> consume_demo_launch(GameState state) noexcept {
    if (state != GameState::DemoLaunch) {
        return std::nullopt;
    }
    return DemoLaunchTransition{};
}

} // namespace drone::gameplay
