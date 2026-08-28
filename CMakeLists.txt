cmake_minimum_required(VERSION 3.20)
project(Drone LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DRONE_BUILD_TESTS "Build Drone format/core tests" ON)

add_library(drone_core
    src/formats/jba.cpp
    src/formats/clv.cpp
    src/formats/fly.cpp
    src/formats/demo.cpp
    src/formats/scores.cpp
    src/fidelity/indexed_framebuffer.cpp
    src/fidelity/framebuffer_snapshot.cpp
    src/fidelity/host_capture.cpp
    src/fidelity/hud_presentation.cpp
    src/fidelity/sprite_sheet.cpp
    src/fidelity/sprite_blit.cpp
    src/fidelity/font2.cpp
    src/fidelity/world_viewport.cpp
    src/fidelity/palette_effects.cpp
    src/fidelity/presentation_order.cpp
    src/fidelity/scaled_overlay_presentation.cpp
    src/fidelity/world_presentation_subpasses.cpp
    src/gameplay/collision.cpp
    src/gameplay/debris_effects.cpp
    src/gameplay/player.cpp
    src/gameplay/rapid_missile.cpp
    src/gameplay/special_weapon.cpp
    src/gameplay/demo_replay.cpp
    src/gameplay/input.cpp
    src/gameplay/enemy_bomb.cpp
    src/gameplay/shield.cpp
    src/gameplay/player_lifecycle.cpp
    src/gameplay/game_over.cpp
    src/gameplay/mission_outcome.cpp
    src/gameplay/mission_progression.cpp
    src/gameplay/drone_objective.cpp
    src/gameplay/drone_weapon_interaction.cpp
    src/gameplay/original_random.cpp
    src/gameplay/scoring.cpp
    src/gameplay/high_scores.cpp
    src/gameplay/post_game.cpp
    src/gameplay/boss_progression.cpp
    src/gameplay/boss_encounter.cpp
    src/gameplay/trajectory.cpp
    src/gameplay/trajectory_templates.cpp
    src/gameplay/trajectory_encounter.cpp
    src/gameplay/gameplay_phase.cpp
    src/gameplay/world_scroll.cpp
    src/gameplay/gameplay_update_order.cpp
    src/gameplay/game_state.cpp
    src/gameplay/game_session.cpp
)

target_include_directories(drone_core PUBLIC include)
target_compile_features(drone_core PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(drone_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(drone_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(drone_inspect tools/drone_inspect.cpp)
target_link_libraries(drone_inspect PRIVATE drone_core)

add_executable(drone_gameplay_probe tools/drone_gameplay_probe.cpp)
target_link_libraries(drone_gameplay_probe PRIVATE drone_core)

add_executable(drone_framecheck tools/drone_framecheck.cpp)
target_link_libraries(drone_framecheck PRIVATE drone_core)

add_executable(drone_session_probe tools/drone_session_probe.cpp)
target_link_libraries(drone_session_probe PRIVATE drone_core)

option(DRONE_BUILD_FIDELITY_HOST "Build the native indexed-framebuffer fidelity host" ON)

if(DRONE_BUILD_FIDELITY_HOST)
    if(WIN32)
        add_executable(drone_fidelity_host tools/drone_fidelity_host_win32.cpp)
        target_link_libraries(drone_fidelity_host PRIVATE drone_core)
    elseif(APPLE)
        enable_language(OBJCXX)
        add_executable(drone_fidelity_host tools/drone_fidelity_host_macos.mm)
        target_link_libraries(drone_fidelity_host PRIVATE drone_core "-framework Cocoa" "-framework CoreGraphics")
    elseif(UNIX)
        find_package(X11 QUIET)
        if(X11_FOUND)
            add_executable(drone_fidelity_host tools/drone_fidelity_host_x11.cpp)
            target_include_directories(drone_fidelity_host PRIVATE ${X11_INCLUDE_DIR})
            target_link_libraries(drone_fidelity_host PRIVATE drone_core ${X11_LIBRARIES})
        else()
            message(STATUS "X11 development files not found; drone_fidelity_host will not be built")
        endif()
    endif()
endif()

if(DRONE_BUILD_TESTS)
    enable_testing()
    add_executable(drone_tests tests/test_formats.cpp)
    target_link_libraries(drone_tests PRIVATE drone_core)
    add_test(NAME drone_formats COMMAND drone_tests)

    add_executable(drone_gameplay_tests tests/test_gameplay.cpp)
    target_link_libraries(drone_gameplay_tests PRIVATE drone_core)
    add_test(NAME drone_gameplay COMMAND drone_gameplay_tests)

    add_executable(drone_game_session_tests tests/test_game_session.cpp)
    target_link_libraries(drone_game_session_tests PRIVATE drone_core)
    add_test(NAME drone_game_session COMMAND drone_game_session_tests)

    add_executable(drone_drone_objective_tests tests/test_drone_objective.cpp)
    target_link_libraries(drone_drone_objective_tests PRIVATE drone_core)
    add_test(NAME drone_drone_objective COMMAND drone_drone_objective_tests)

    add_executable(drone_drone_weapon_interaction_tests tests/test_drone_weapon_interaction.cpp)
    target_link_libraries(drone_drone_weapon_interaction_tests PRIVATE drone_core)
    add_test(NAME drone_drone_weapon_interaction COMMAND drone_drone_weapon_interaction_tests)

    add_executable(drone_boss_encounter_tests tests/test_boss_encounter.cpp)
    target_link_libraries(drone_boss_encounter_tests PRIVATE drone_core)
    add_test(NAME drone_boss_encounter COMMAND drone_boss_encounter_tests)

    add_executable(drone_trajectory_encounter_tests tests/test_trajectory_encounter.cpp)
    target_link_libraries(drone_trajectory_encounter_tests PRIVATE drone_core)
    add_test(NAME drone_trajectory_encounter COMMAND drone_trajectory_encounter_tests)

    add_executable(drone_fidelity_tests tests/test_fidelity.cpp)
    target_link_libraries(drone_fidelity_tests PRIVATE drone_core)
    add_test(NAME drone_fidelity COMMAND drone_fidelity_tests)

    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(Python3_Interpreter_FOUND)
        add_test(
            NAME drone_research_metadata
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_research_docs.py
        )
        add_test(
            NAME drone_phase2_exit
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_phase2_exit.py
        )
        add_test(
            NAME drone_phase3_exit
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_phase3_exit.py
        )
        add_test(
            NAME drone_game_session_probe
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_game_session_probe.py $<TARGET_FILE:drone_session_probe>
        )
        add_test(
            NAME drone_framebuffer_fixture_tool
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_framebuffer_fixture.py
        )
        if(UNIX AND NOT APPLE AND TARGET drone_fidelity_host)
            add_test(
                NAME drone_linux_fidelity_host_capture
                COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_linux_fidelity_host.py $<TARGET_FILE:drone_fidelity_host>
            )
        endif()
    endif()
endif()
