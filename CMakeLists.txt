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

if(DRONE_BUILD_TESTS)
    enable_testing()
    add_executable(drone_tests tests/test_formats.cpp)
    target_link_libraries(drone_tests PRIVATE drone_core)
    add_test(NAME drone_formats COMMAND drone_tests)

    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(Python3_Interpreter_FOUND)
        add_test(
            NAME drone_research_metadata
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_research_docs.py
        )
    endif()
endif()
