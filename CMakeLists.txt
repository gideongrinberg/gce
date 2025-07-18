cmake_minimum_required(VERSION 3.31)
project(gce C CXX)
cmake_policy(SET CMP0169 OLD)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

# Download and build dependencies
FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.5
)

set(FETCHCONTENT_QUIET OFF)
set(BUILD_EXAMPLES OFF)
FetchContent_MakeAvailable(raylib)

FetchContent_Populate(
        rlImgui
        GIT_REPOSITORY https://github.com/raylib-extras/rlImGui
        GIT_TAG main
        SOURCE_DIR ${CMAKE_BINARY_DIR}/rlImgui-src
)

FetchContent_Populate(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui
        GIT_TAG v1.92.1-docking
        SOURCE_DIR ${CMAKE_BINARY_DIR}/imgui-src
)

set(imgui_SOURCE_DIR ${CMAKE_BINARY_DIR}/imgui-src)
set(rlImgui_SOURCE_DIR ${CMAKE_BINARY_DIR}/rlImgui-src)

add_library(imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp

        ${rlImgui_SOURCE_DIR}/rlImGui.cpp
)

target_link_libraries(imgui PUBLIC raylib)
target_include_directories(imgui PUBLIC
        ${imgui_SOURCE_DIR}
        ${rlImgui_SOURCE_DIR}
        ${raylib_SOURCE_DIR}/src
)

add_library(gce-core STATIC engine/position.c engine/position.h engine/tables.c engine/tables.h
        engine/engine.h engine/zobrist.h engine/zobrist.c)
target_include_directories(gce-core PUBLIC engine/)
target_compile_options(gce-core PRIVATE
        $<$<CONFIG:Release>:-Ofast>
)

add_executable(gce-perft perft/main.cpp)
target_link_libraries(gce-perft gce-core)
target_compile_options(gce-perft PRIVATE
        $<$<CONFIG:Release>:-Ofast>
)

add_executable(gce-test test/main.c test/tinytest.h)
target_link_libraries(gce-test PUBLIC gce-core)

add_executable(gce-gui
        gui/main.cpp
        gui/board.hpp
        gui/board.cpp
        gui/assets.h
        gui/assets.c
        gui/textures.h
        gui/textures.c
        gui/window.hpp
        gui/game.hpp
        gui/info.hpp)

target_compile_options(gce-gui PRIVATE
        $<$<CONFIG:Release>:-O3>
)

target_link_libraries(gce-gui PRIVATE gce-core raylib imgui)